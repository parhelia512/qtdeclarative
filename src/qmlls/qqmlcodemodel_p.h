// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLCODEMODEL_P_H
#define QQMLCODEMODEL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qlanguageserver_p.h"
#include "qtextdocument_p.h"
#include "qqmllshelputils_p.h"

#include <QObject>
#include <QHash>
#include <QtCore/qfilesystemwatcher.h>
#include <QtCore/private/qfactoryloader_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
class TextSynchronization;
namespace QmlLsp {

class OpenDocumentSnapshot
{
public:
    enum class DumpOption {
        NoCode = 0,
        LatestCode = 0x1,
        ValidCode = 0x2,
        AllCode = LatestCode | ValidCode
    };
    Q_DECLARE_FLAGS(DumpOptions, DumpOption)
    QStringList searchPath;
    QByteArray url;
    std::optional<int> docVersion;
    QQmlJS::Dom::DomItem doc;
    std::optional<int> validDocVersion;
    QQmlJS::Dom::DomItem validDoc;
    std::optional<int> scopeVersion;
    QDateTime scopeDependenciesLoadTime;
    bool scopeDependenciesChanged = false;
    QQmlJSScope::ConstPtr scope;
    QDebug dump(QDebug dbg, DumpOptions dump = DumpOption::NoCode);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(OpenDocumentSnapshot::DumpOptions)

class OpenDocument
{
public:
    OpenDocumentSnapshot snapshot;
    std::shared_ptr<Utils::TextDocument> textDocument;
};

struct RegisteredSemanticTokens
{
    QByteArray resultId = "0";
    QList<int> lastTokens;
};

struct ModuleSetting
{
    QString sourceFolder;
    QStringList importPaths;
};

using ModuleSettings = QList<ModuleSetting>;
class QQmllsBuildInformation
{
public:
    QQmllsBuildInformation();
    void loadSettingsFrom(const QStringList &buildPaths);
    QStringList importPathsFor(const QString &filePath);
    ModuleSetting settingFor(const QString &filePath);

private:
    QString m_docDir;
    ModuleSettings m_moduleSettings;
    QSet<QString> m_seenSettings;
};

class QQmlCodeModel : public QObject
{
    Q_OBJECT
public:
    enum class UrlLookup { Caching, ForceLookup };
    enum class State { Running, Stopping };

    explicit QQmlCodeModel(const QByteArray &rootUrl = {}, QObject *parent = nullptr,
                           QQmlToolingSharedSettings *settings = nullptr);
    ~QQmlCodeModel();
    QQmlJS::Dom::DomItem currentEnv() const { return m_currentEnv; };
    QQmlJS::Dom::DomItem validEnv() const { return m_validEnv; };
    OpenDocumentSnapshot snapshotByUrl(const QByteArray &url);
    OpenDocument openDocumentByUrl(const QByteArray &url);
    bool isEmpty() const;

    void addOpenToUpdate(const QByteArray &);
    void removeDirectory(const QByteArray &);
    // void updateDocument(const OpenDocument &doc);
    void newOpenFile(const QByteArray &url, int version, const QString &docText);
    void newDocForOpenFile(const QByteArray &url, int version, const QString &docText);
    void closeOpenFile(const QByteArray &url);
    QByteArray rootUrl() const;
    QStringList buildPathsForRootUrl(const QByteArray &url);
    QStringList buildPathsForFileUrl(const QByteArray &url);
    void setBuildPathsForRootUrl(QByteArray url, const QStringList &paths);
    QStringList importPathsForUrl(const QByteArray &);
    QStringList importPaths() const { return m_importPaths; };
    void setImportPaths(const QStringList &paths);
    QQmlToolingSharedSettings *settings() const { return m_settings; }
    QStringList findFilePathsFromFileNames(const QStringList &fileNames);
    static QStringList fileNamesToWatch(const QQmlJS::Dom::DomItem &qmlFile);
    void disableCMakeCalls();

    RegisteredSemanticTokens &registeredTokens();
    const RegisteredSemanticTokens &registeredTokens() const;
    QString documentationRootPath() const { return m_documentationRootPath; }
    void setDocumentationRootPath(const QString &path);

    QSet<QString> ignoreForWatching() const { return m_ignoreForWatching; }
    HelpManager *helpManager() { return &m_helpManager; }

Q_SIGNALS:
    void updatedSnapshot(const QByteArray &url);
    void documentationRootPathChanged(const QString &path);

private:
    void addDirectory(const QString &path, int leftDepth);
    bool openUpdateSome();
    void openUpdateStart();
    void openUpdateEnd();
    void openUpdate(const QByteArray &);
    void openNeedUpdate();
    QString url2Path(const QByteArray &url, UrlLookup options = UrlLookup::Caching);

    static bool callCMakeBuild(const QStringList &buildPaths);
    void addFileWatches(const QQmlJS::Dom::DomItem &qmlFile);
    enum CMakeStatus { RequiresInitialization, HasCMake, DoesNotHaveCMake };
    void initializeCMakeStatus(const QString &);

    mutable QMutex m_mutex;
    State m_state = State::Running;
    int m_nUpdateInProgress = 0;
    QStringList m_importPaths;
    QQmlJS::Dom::DomItem m_currentEnv;
    QQmlJS::Dom::DomItem m_validEnv;
    QByteArray m_lastOpenDocumentUpdated;
    QSet<QByteArray> m_openDocumentsToUpdate;
    QHash<QByteArray, QStringList> m_buildPathsForRootUrl;
    QByteArray m_rootUrl;
    QHash<QByteArray, QString> m_url2path;
    QHash<QString, QByteArray> m_path2url;
    QHash<QByteArray, OpenDocument> m_openDocuments;
    QQmlToolingSharedSettings *m_settings;
    QQmllsBuildInformation m_buildInformation;
    QFileSystemWatcher m_cppFileWatcher;
    bool m_rebuildRequired = true; // always trigger a rebuild on start
    CMakeStatus m_cmakeStatus = RequiresInitialization;
    RegisteredSemanticTokens m_tokens;
    QString m_documentationRootPath;
    QSet<QString> m_ignoreForWatching;
    HelpManager m_helpManager;
private slots:
    void onCppFileChanged(const QString &);
};

} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QQMLCODEMODEL_P_H
