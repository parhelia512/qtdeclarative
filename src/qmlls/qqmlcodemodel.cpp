// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:trusted-sources

#include "qqmlcodemodel_p.h"
#include "qqmllsplugin_p.h"
#include "qtextdocument_p.h"
#include "qqmllsutils_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qthreadpool.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qprocess.h>
#include <QtCore/qdiriterator.h>

#if QT_CONFIG(settings)
#  include <QtCore/qsettings.h>
#endif

#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlCompiler/private/qqmljsutils_p.h>

#include <memory>
#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QmlLsp {

Q_STATIC_LOGGING_CATEGORY(codeModelLog, "qt.languageserver.codemodel")

using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

/*!
\internal
\class QQmlCodeModel

The code model offers a view of the current state of the current files, and traks open files.
All methods are threadsafe, and generally return immutable or threadsafe objects that can be
worked on from any thread (unless otherwise noted).
The idea is the let all other operations be as lock free as possible, concentrating all tricky
synchronization here.

\section2 Global views
\list
\li currentEnv() offers a view that contains the latest version of all the loaded files
\li validEnv() is just like current env but stores only the valid (meaning correctly parsed,
  not necessarily without errors) version of a file, it is normally a better choice to load the
  dependencies/symbol information from
\endlist

\section2 OpenFiles
\list
\li snapshotByUrl() returns an OpenDocumentSnapshot of an open document. From it you can get the
  document, its latest valid version, scope, all connected to a specific version of the document
  and immutable. The signal updatedSnapshot() is called every time a snapshot changes (also for
  every partial change: document change, validDocument change, scope change).
\li openDocumentByUrl() is a lower level and more intrusive access to OpenDocument objects. These
  contains the current snapshot, and shared pointer to a Utils::TextDocument. This is *always* the
  current version of the document, and has line by line support.
  Working on it is more delicate and intrusive, because you have to explicitly acquire its mutex()
  before *any* read or write/modification to it.
  It has a version nuber which is supposed to always change and increase.
  It is mainly used for highlighting/indenting, and is immediately updated when the user edits a
  document. Its use should be avoided if possible, preferring the snapshots.
\endlist

\section2 Parallelism/Theading
Most operations are not parallel and usually take place in the main thread (but are still thread
safe).
There is one task that is executed in parallel: OpenDocumentUpdate.
OpenDocumentUpdate keeps the snapshots of the open documents up to date.

There is always a tension between being responsive, using all threads available, and avoid to hog
too many resources. One can choose different parallelization strategies, we went with a flexiable
approach.
We have (private) functions that execute part of the work: openUpdateSome(). These
do all locking needed, get some work, do it without locks, and at the end update the state of the
code model. If there is more work, then they return true. Thus while (xxxSome()); works until there
is no work left.

The internal addOpenToUpdate() add more work to do.

openNeedUpdate() checks if there is work to do, and if yes ensure that a
worker thread (or more) that work on it exist.
*/

QQmlCodeModel::QQmlCodeModel(const QByteArray &rootUrl, QObject *parent,
                             QQmlToolingSharedSettings *settings)
    : QObject{ parent },
      m_rootUrl(rootUrl),
      m_currentEnv(std::make_shared<DomEnvironment>(
              QLibraryInfo::paths(QLibraryInfo::QmlImportsPath),
              DomEnvironment::Option::SingleThreaded, DomCreationOption::Extended)),
      m_validEnv(std::make_shared<DomEnvironment>(
              m_currentEnv.ownerAs<DomEnvironment>()->loadPaths(),
              DomEnvironment::Option::SingleThreaded, DomCreationOption::Extended)),
      m_settings(settings)
{
}

/*!
\internal
Disable the functionality that uses CMake, and remove the already watched paths if there are some.
*/
void QQmlCodeModel::disableCMakeCalls()
{
    QMutexLocker guard(&m_mutex);
    m_cmakeStatus = DoesNotHaveCMake;
    if (const QStringList toRemove = m_cppFileWatcher.files(); !toRemove.isEmpty())
        m_cppFileWatcher.removePaths(toRemove);
    QObject::disconnect(&m_cppFileWatcher, &QFileSystemWatcher::fileChanged, nullptr, nullptr);
}

QQmlCodeModel::~QQmlCodeModel()
{
    QObject::disconnect(&m_cppFileWatcher, &QFileSystemWatcher::fileChanged, nullptr, nullptr);
    while (true) {
        bool shouldWait;
        {
            QMutexLocker l(&m_mutex);
            m_openDocumentsToUpdate.clear();
            shouldWait = m_nUpdateInProgress != 0;
        }
        if (!shouldWait)
            break;
        QThread::yieldCurrentThread();
    }
}

OpenDocumentSnapshot QQmlCodeModel::snapshotByUrl(const QByteArray &url)
{
    return openDocumentByUrl(url).snapshot;
}

void QQmlCodeModel::removeDirectory(const QByteArray &url)
{
    const QString path = url2Path(url);
    if (auto validEnvPtr = m_validEnv.ownerAs<DomEnvironment>())
        validEnvPtr->removePath(path);
    if (auto currentEnvPtr = m_currentEnv.ownerAs<DomEnvironment>())
        currentEnvPtr->removePath(path);
}

QString QQmlCodeModel::url2Path(const QByteArray &url, UrlLookup options)
{
    QString res;
    {
        QMutexLocker l(&m_mutex);
        res = m_url2path.value(url);
    }
    if (!res.isEmpty() && options == UrlLookup::Caching)
        return res;
    QUrl qurl(QString::fromUtf8(url));
    QFileInfo f(qurl.toLocalFile());
    QString cPath = f.canonicalFilePath();
    if (cPath.isEmpty())
        cPath = f.filePath();
    {
        QMutexLocker l(&m_mutex);
        if (!res.isEmpty() && res != cPath)
            m_path2url.remove(res);
        m_url2path.insert(url, cPath);
        m_path2url.insert(cPath, url);
    }
    return cPath;
}

void QQmlCodeModel::newOpenFile(const QByteArray &url, int version, const QString &docText)
{
    {
        QMutexLocker l(&m_mutex);
        auto &openDoc = m_openDocuments[url];
        if (!openDoc.textDocument)
            openDoc.textDocument = std::make_shared<Utils::TextDocument>();
        QMutexLocker l2(openDoc.textDocument->mutex());
        openDoc.textDocument->setVersion(version);
        openDoc.textDocument->setPlainText(docText);
    }
    addOpenToUpdate(url, NormalUpdate);
    openNeedUpdate();
}

OpenDocument QQmlCodeModel::openDocumentByUrl(const QByteArray &url)
{
    QMutexLocker l(&m_mutex);
    return m_openDocuments.value(url);
}

bool QQmlCodeModel::isEmpty() const
{
    QMutexLocker l(&m_mutex);
    return m_openDocuments.isEmpty();
}

RegisteredSemanticTokens &QQmlCodeModel::registeredTokens()
{
    QMutexLocker l(&m_mutex);
    return m_tokens;
}

const RegisteredSemanticTokens &QQmlCodeModel::registeredTokens() const
{
    QMutexLocker l(&m_mutex);
    return m_tokens;
}

void QQmlCodeModel::openNeedUpdate()
{
    qCDebug(codeModelLog) << "openNeedUpdate";
    const int maxThreads = 1;
    {
        QMutexLocker l(&m_mutex);
        if (m_openDocumentsToUpdate.isEmpty() || m_nUpdateInProgress >= maxThreads)
            return;
        if (++m_nUpdateInProgress == 1)
            openUpdateStart();
    }
    QThreadPool::globalInstance()->start([this]() {
        while (openUpdateSome()) { }
    });
}

bool QQmlCodeModel::openUpdateSome()
{
    qCDebug(codeModelLog) << "openUpdateSome start";
    QByteArray toUpdate;
    UpdatePolicy policy;
    {
        QMutexLocker l(&m_mutex);
        if (m_openDocumentsToUpdate.isEmpty()) {
            if (--m_nUpdateInProgress == 0)
                openUpdateEnd();
            return false;
        }
        const auto it = m_openDocumentsToUpdate.begin();
        toUpdate = it.key();
        policy = it.value();
        m_openDocumentsToUpdate.erase(it);
    }
    bool hasMore = false;
    {
        auto guard = qScopeGuard([this, &hasMore]() {
            QMutexLocker l(&m_mutex);
            if (m_openDocumentsToUpdate.isEmpty()) {
                if (--m_nUpdateInProgress == 0)
                    openUpdateEnd();
                hasMore = false;
            } else {
                hasMore = true;
            }
        });
        openUpdate(toUpdate, policy);
    }
    return hasMore;
}

void QQmlCodeModel::openUpdateStart()
{
    qCDebug(codeModelLog) << "openUpdateStart";
}

void QQmlCodeModel::openUpdateEnd()
{
    qCDebug(codeModelLog) << "openUpdateEnd";
}

/*!
\internal
Performs initialization for m_cmakeStatus, including testing for CMake on the current system.
*/
void QQmlCodeModel::initializeCMakeStatus(const QString &pathForSettings)
{
    if (m_settings) {
        const QString cmakeCalls = u"no-cmake-calls"_s;
        m_settings->search(pathForSettings, { QString(), verbose() });
        if (m_settings->isSet(cmakeCalls) && m_settings->value(cmakeCalls).toBool()) {
            qWarning() << "Disabling CMake calls via .qmlls.ini setting.";
            QMutexLocker guard(&m_mutex);
            m_cmakeStatus = DoesNotHaveCMake;
            return;
        }
    }

    QProcess process;
    process.setProgram(u"cmake"_s);
    process.setArguments({ u"--version"_s });
    process.start();
    process.waitForFinished();

    {
        QMutexLocker guard(&m_mutex);
        m_cmakeStatus = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0
                ? HasCMake
                : DoesNotHaveCMake;

        if (m_cmakeStatus == DoesNotHaveCMake) {
            qWarning() << "Disabling CMake calls because CMake was not found.";
            return;
        }
    }

    QObject::connect(&m_cppFileWatcher, &QFileSystemWatcher::fileChanged, this,
                     &QQmlCodeModel::onCppFileChanged);
}

/*!
\internal
For each build path that is a also a CMake build path, call CMake with \l cmakeBuildCommand to
generate/update the .qmltypes, qmldir and .qrc files.
It is assumed here that the number of build folders is usually no more than one, so execute the
CMake builds one at a time.

If CMake cannot be executed, false is returned. This may happen when CMake does not exist on the
current system, when the target executed by CMake does not exist (for example when something else
than qt_add_qml_module is used to setup the module in CMake), or the when the CMake build itself
fails.
*/
bool QQmlCodeModel::callCMakeBuild(const QStringList &buildPaths)
{
    bool success = true;
    for (const auto &path : buildPaths) {
        if (!QFileInfo::exists(path + u"/.cmake"_s))
            continue;

        QProcess process;
        const auto command = QQmlLSUtils::cmakeBuildCommand(path);
        process.setProgram(command.first);
        process.setArguments(command.second);
        qCDebug(codeModelLog) << "Running" << process.program() << process.arguments();
        process.start();

        // TODO: run process concurrently instead of blocking qmlls
        success &= process.waitForFinished();
        success &= (process.exitCode() == 0);
        qCDebug(codeModelLog) << process.program() << process.arguments() << "terminated with"
                              << process.exitCode();
    }
    return success;
}

/*!
\internal
Iterate the entire source directory to find all C++ files that have their names in fileNames, and
return all the found file paths.

This is an overapproximation and might find unrelated files with the same name.
*/
QStringList QQmlCodeModel::findFilePathsFromFileNames(const QStringList &_fileNamesToSearch,
                                                      const QSet<QString> &ignoredFilePaths)
{
    QStringList fileNamesToSearch{ _fileNamesToSearch };

    {
        QMutexLocker guard(&m_mutex);
        // ignore files that were not found last time
        fileNamesToSearch.erase(std::remove_if(fileNamesToSearch.begin(), fileNamesToSearch.end(),
                                               [this](const QString &fileName) {
                                                   return m_ignoreForWatching.contains(fileName);
                                               }),
                                fileNamesToSearch.end());
    }

    const QString rootDir = QUrl(QString::fromUtf8(m_rootUrl)).toLocalFile();
    qCDebug(codeModelLog) << "Searching for files to watch in workspace folder" << rootDir;

    const QStringList result =
            QQmlLSUtils::findFilePathsFromFileNames(rootDir, fileNamesToSearch, ignoredFilePaths);

    QMutexLocker guard(&m_mutex);
    for (const auto &fileName : fileNamesToSearch) {
        if (std::none_of(result.begin(), result.end(),
                         [&fileName](const QString &path) { return path.endsWith(fileName); })) {
            m_ignoreForWatching.insert(fileName);
        }
    }
    return result;
}

/*!
\internal
Find all C++ file names (not path, for file paths call \l findFilePathsFromFileNames on the result
of this method) that this qmlFile relies on.
*/
QStringList QQmlCodeModel::fileNamesToWatch(const DomItem &qmlFile)
{
    const QmlFile *file = qmlFile.as<QmlFile>();
    if (!file)
        return {};

    auto resolver = file->typeResolver();
    if (!resolver)
        return {};

    auto types = resolver->importedTypes();

    QStringList result;
    for (const auto &type : types) {
        if (!type.scope)
            continue;
        // note: the factory only loads composite types
        const bool isComposite = type.scope.factory() || type.scope->isComposite();
        if (isComposite)
            continue;

        const QString filePath = QFileInfo(type.scope->filePath()).fileName();
        if (!filePath.isEmpty())
            result << filePath;
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

/*!
\internal
Add watches for all C++ files that this qmlFile relies on, so a rebuild can be triggered when they
are modified.
*/
void QQmlCodeModel::addFileWatches(const DomItem &qmlFile)
{
    const auto filesToWatch = fileNamesToWatch(qmlFile);

    // remove already watched files to avoid a warning later on
    const QStringList alreadyWatchedFiles = m_cppFileWatcher.files();
    const QSet<QString> alreadyWatchedFilesSet(alreadyWatchedFiles.begin(),
                                               alreadyWatchedFiles.end());
    QStringList filepathsToWatch = findFilePathsFromFileNames(filesToWatch, alreadyWatchedFilesSet);

    if (filepathsToWatch.isEmpty())
        return;

    QMutexLocker guard(&m_mutex);
    const auto unwatchedPaths = m_cppFileWatcher.addPaths(filepathsToWatch);
    if (!unwatchedPaths.isEmpty()) {
        qCDebug(codeModelLog) << "Cannot watch paths" << unwatchedPaths << "from requested"
                              << filepathsToWatch;
    }
}

void QQmlCodeModel::onCppFileChanged(const QString &)
{
    QMutexLocker guard(&m_mutex);
    m_rebuildRequired = true;
}

enum VersionCheckResult {
    ClosedDocument,
    VersionLowerThanDocument,
    VersionLowerThanSnapshot,
    VersionOk,
};

enum VersionCheckResultForValidDocument {
    VersionLowerThanValidSnapshot,
    VersionOkForValidDocument,
};

VersionCheckResult checkVersion(const OpenDocument& doc, int version)
{
    if (!doc.textDocument)
        return ClosedDocument;

    {
        QMutexLocker guard2(doc.textDocument->mutex());
        if (doc.textDocument->version() && *doc.textDocument->version() > version)
            return VersionLowerThanDocument;
    }

    if (doc.snapshot.docVersion && *doc.snapshot.docVersion >= version)
        return VersionLowerThanSnapshot;

    return VersionOk;
}

static VersionCheckResultForValidDocument checkVersionForValidDocument(const OpenDocument &doc,
                                                                       int version)
{
    if (doc.snapshot.validDocVersion && *doc.snapshot.validDocVersion >= version)
        return VersionLowerThanValidSnapshot;

    return VersionOkForValidDocument;
}

static void updateItemInSnapshot(const DomItem &item, const DomItem &validItem,
                                 const QByteArray &url, OpenDocument *doc, int version,
                                 UpdatePolicy policy)
{
    switch (policy == ForceUpdate ? VersionOk : checkVersion(*doc, version)) {
    case ClosedDocument:
        qCWarning(lspServerLog) << "Ignoring update to closed document" << QString::fromUtf8(url);
        return;
    case VersionLowerThanDocument:
        qCWarning(lspServerLog) << "Version" << version << "of document" << QString::fromUtf8(url)
                                << "is not the latest anymore";
        return;
    case VersionLowerThanSnapshot:
        qCWarning(lspServerLog) << "Skipping update of current doc to obsolete version" << version
                                << "of document" << QString::fromUtf8(url);
        return;
    case VersionOk:
        doc->snapshot.docVersion = version;
        doc->snapshot.doc = item;
        break;
    }

    if (!item.field(Fields::isValid).value().toBool(false)) {
        qCWarning(lspServerLog) << "avoid update of validDoc to " << version << "of document"
                                << QString::fromUtf8(url) << "as it is invalid";
        return;
    }

    switch (policy == ForceUpdate ? VersionOkForValidDocument
                                  : checkVersionForValidDocument(*doc, version)) {
    case VersionLowerThanValidSnapshot:
        qCWarning(lspServerLog) << "Skipping update of valid doc to obsolete version" << version
                                << "of document" << QString::fromUtf8(url);
        return;
    case VersionOkForValidDocument:
        doc->snapshot.validDocVersion = version;
        doc->snapshot.validDoc = validItem;
        break;
    }
}

void QQmlCodeModel::newDocForOpenFile(const QByteArray &url, int version, const QString &docText,
                                      QmlLsp::UpdatePolicy policy)
{
    qCDebug(codeModelLog) << "updating doc" << url << "to version" << version << "("
                          << docText.size() << "chars)";

    const QString fPath = url2Path(url, UrlLookup::ForceLookup);
    if (cmakeStatus() == RequiresInitialization)
        initializeCMakeStatus(fPath);

    DomItem newCurrent = m_currentEnv.makeCopy(DomItem::CopyOption::EnvConnected).item();
    QStringList loadPaths = buildPathsForFileUrl(url);

    if (cmakeStatus() == HasCMake && !loadPaths.isEmpty() && rebuildRequired()) {
        callCMakeBuild(loadPaths);
        QMutexLocker guard(&m_mutex);
        m_rebuildRequired = false;
    }

    loadPaths.append(importPathsForUrl(url));
    if (std::shared_ptr<DomEnvironment> newCurrentPtr = newCurrent.ownerAs<DomEnvironment>()) {
        newCurrentPtr->setLoadPaths(loadPaths);
    }

    // if the documentation root path is not set through the commandline,
    // try to set it from the settings file (.qmlls.ini file)
    if (documentationRootPath().isEmpty() && m_settings) {
        // note: settings already searched current file in importPathsForFile() call above
        const QString docDir = QStringLiteral(u"docDir");
        if (m_settings->isSet(docDir))
            setDocumentationRootPath(m_settings->value(docDir).toString());
    }

    Path p;
    auto newCurrentPtr = newCurrent.ownerAs<DomEnvironment>();
    newCurrentPtr->loadFile(FileToLoad::fromMemory(newCurrentPtr, fPath, docText),
                            [&p, this](Path, const DomItem &, const DomItem &newValue) {
                                const DomItem file = newValue.fileObject();
                                p = file.canonicalPath();
                                // Force population of the file by accessing isValid field. We
                                // don't want to populate the file after adding the file to the
                                // snapshot in updateItemInSnapshot.
                                file.field(Fields::isValid);
                                if (cmakeStatus() == HasCMake)
                                    addFileWatches(file);
                            });
    newCurrentPtr->loadPendingDependencies();
    if (p) {
        newCurrent.commitToBase(m_validEnv.ownerAs<DomEnvironment>());
        QMutexLocker l(&m_mutex);
        updateItemInSnapshot(m_currentEnv.path(p), m_validEnv.path(p), url, &m_openDocuments[url],
                             version, policy);
    }
    if (codeModelLog().isDebugEnabled()) {
        qCDebug(codeModelLog) << "Finished update doc of " << url << "to version" << version;
        snapshotByUrl(url).dump(qDebug() << "postSnapshot",
                                OpenDocumentSnapshot::DumpOption::AllCode);
    }
    // we should update the scope in the future thus call addOpen(url)
    emit updatedSnapshot(url, policy);
}

void QQmlCodeModel::closeOpenFile(const QByteArray &url)
{
    QMutexLocker l(&m_mutex);
    m_openDocuments.remove(url);
}

QByteArray QQmlCodeModel::rootUrl() const
{
    QMutexLocker l(&m_mutex);
    return m_rootUrl;
}

QStringList QQmlCodeModel::buildPaths()
{
    QMutexLocker l(&m_mutex);
    return m_buildPaths;
}

QStringList QQmlCodeModel::importPathsForUrl(const QByteArray &url)
{
    const QString fileName = url2Path(url);
    QStringList result = importPaths();

    const QString importPaths = u"importPaths"_s;
    if (m_settings && m_settings->search(fileName, { QString(), verbose() }).isValid()
        && m_settings->isSet(importPaths)) {
        result.append(m_settings->value(importPaths).toString().split(QDir::listSeparator()));
    }

    const QStringList buildPath = buildPathsForFileUrl(url);
    QMutexLocker l(&m_mutex);
    m_buildInformation.loadSettingsFrom(buildPath);
    result.append(m_buildInformation.importPathsFor(fileName));

    return result;
}

void QQmlCodeModel::setImportPaths(const QStringList &importPaths)
{
    if (const auto &env = m_currentEnv.ownerAs<DomEnvironment>())
        env->setLoadPaths(importPaths);
    if (const auto &env = m_validEnv.ownerAs<DomEnvironment>())
        env->setLoadPaths(importPaths);
}

QStringList QQmlCodeModel::importPaths() const
{
    if (const auto &env = m_currentEnv.ownerAs<DomEnvironment>())
        return env->loadPaths();
    if (const auto &env = m_validEnv.ownerAs<DomEnvironment>())
        return env->loadPaths();
    return {};
}

static QStringList withDependentBuildDirectories(QStringList &&buildPaths)
{
    // add dependent build directories
    QStringList res;
    std::reverse(buildPaths.begin(), buildPaths.end());
    const int maxDeps = 4;
    while (!buildPaths.isEmpty()) {
        res += buildPaths.takeLast();
        const QString &bPath = res.constLast();
        const QString bPathExtended = bPath + u"/_deps";
        if (QFile::exists(bPathExtended) && bPath.count(u"/_deps/"_s) < maxDeps) {
            for (const auto &fileInfo :
                 QDirListing{ bPathExtended, QDirListing::IteratorFlag::DirsOnly }) {
                buildPaths.append(fileInfo.absoluteFilePath());
            }
        }
    }
    return res;
}

QStringList QQmlCodeModel::buildPathsForFileUrl(const QByteArray &url)
{
    if (QStringList result = buildPaths(); !result.isEmpty())
        return withDependentBuildDirectories(std::move(result));

    // fallback: look in the user settings (.qmlls.ini files in the source directory)
    if (!m_settings || !m_settings->search(url2Path(url), { QString(), verbose() }).isValid())
        return {};

    constexpr QLatin1String buildDir = "buildDir"_L1;
    if (!m_settings->isSet(buildDir))
        return {};

    return withDependentBuildDirectories(m_settings->value(buildDir).toString().split(
            QDir::listSeparator(), Qt::SkipEmptyParts));
}

void QQmlCodeModel::setDocumentationRootPath(const QString &path)
{
    {
        QMutexLocker l(&m_mutex);
        if (m_documentationRootPath == path)
            return;
        m_documentationRootPath = path;
    }
    m_helpManager.setDocumentationRootPath(path);
}

void QQmlCodeModel::setBuildPaths(const QStringList &paths)
{
    QMutexLocker l(&m_mutex);
    m_buildPaths = paths;
}

void QQmlCodeModel::openUpdate(const QByteArray &url, UpdatePolicy policy)
{
    std::optional<int> rNow = 0;
    QString docText;

    {
        QMutexLocker l(&m_mutex);
        OpenDocument &doc = m_openDocuments[url];
        std::shared_ptr<Utils::TextDocument> document = doc.textDocument;
        if (!document)
            return;
        {
            QMutexLocker l2(document->mutex());
            rNow = document->version();
        }
        if (!rNow
            || (policy != ForceUpdate && doc.snapshot.docVersion
                && *doc.snapshot.docVersion == *rNow)) {
            return;
        }

        {
            QMutexLocker l2(doc.textDocument->mutex());
            rNow = doc.textDocument->version();
            docText = doc.textDocument->toPlainText();
        }
    }
    newDocForOpenFile(url, *rNow, docText, policy);
}

void QQmlCodeModel::addOpenToUpdate(const QByteArray &url, UpdatePolicy policy)
{
    {
        QMutexLocker l(&m_mutex);
        m_openDocumentsToUpdate[url] = policy;
    }
    openNeedUpdate();
}

QDebug OpenDocumentSnapshot::dump(QDebug dbg, DumpOptions options)
{
    dbg.noquote().nospace() << "{";
    dbg << "  url:" << QString::fromUtf8(url) << "\n";
    dbg << "  docVersion:" << (docVersion ? QString::number(*docVersion) : u"*none*"_s) << "\n";
    if (options & DumpOption::LatestCode) {
        dbg << "  doc: ------------\n"
            << doc.field(Fields::code).value().toString() << "\n==========\n";
    } else {
        dbg << u"  doc:"
            << (doc ? u"%1chars"_s.arg(doc.field(Fields::code).value().toString().size())
                    : u"*none*"_s)
            << "\n";
    }
    dbg << "  validDocVersion:"
        << (validDocVersion ? QString::number(*validDocVersion) : u"*none*"_s) << "\n";
    if (options & DumpOption::ValidCode) {
        dbg << "  validDoc: ------------\n"
            << validDoc.field(Fields::code).value().toString() << "\n==========\n";
    } else {
        dbg << u"  validDoc:"
            << (validDoc ? u"%1chars"_s.arg(validDoc.field(Fields::code).value().toString().size())
                         : u"*none*"_s)
            << "\n";
    }
    dbg << "  scopeDependenciesLoadTime:" << scopeDependenciesLoadTime << "\n";
    dbg << "  scopeDependenciesChanged" << scopeDependenciesChanged << "\n";
    dbg << "}";
    return dbg;
}

void QQmllsBuildInformation::loadSettingsFrom(const QStringList &buildPaths)
{
#if QT_CONFIG(settings)
    for (const QString &path : buildPaths) {
        if (m_seenSettings.contains(path))
            continue;
        m_seenSettings.insert(path);

        const QString iniPath = QString(path).append("/.qt/.qmlls.build.ini"_L1);
        if (!QFile::exists(iniPath))
            continue;

        QSettings settings(iniPath, QSettings::IniFormat);
        m_docDir = settings.value("docDir"_L1).toString();
        for (const QString &group : settings.childGroups()) {
            settings.beginGroup(group);

            ModuleSetting moduleSetting;
            moduleSetting.sourceFolder = group;
            moduleSetting.sourceFolder.replace("<SLASH>"_L1, "/"_L1);
            moduleSetting.importPaths = settings.value("importPaths"_L1)
                                                .toString()
                                                .split(QDir::listSeparator(), Qt::SkipEmptyParts);
            m_moduleSettings.append(moduleSetting);
            settings.endGroup();
        }
    }
#else
    Q_UNUSED(buildPaths);
#endif
}

QStringList QQmllsBuildInformation::importPathsFor(const QString &filePath)
{
    return settingFor(filePath).importPaths;
}

ModuleSetting QQmllsBuildInformation::settingFor(const QString &filePath)
{
    ModuleSetting result;
    qsizetype longestMatch = 0;
    for (const ModuleSetting &setting : m_moduleSettings) {
        const qsizetype matchLength = setting.sourceFolder.size();
        if (filePath.startsWith(setting.sourceFolder) && matchLength > longestMatch) {
            result = setting;
            longestMatch = matchLength;
        }
    }
    return result;
}

QQmllsBuildInformation::QQmllsBuildInformation() { }

} // namespace QmlLsp

QT_END_NAMESPACE
