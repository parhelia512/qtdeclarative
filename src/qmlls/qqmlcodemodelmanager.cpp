// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlcodemodelmanager_p.h"
#include "qqmllsplugin_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QmlLsp {

using namespace QQmlJS::Dom;
using namespace Qt::StringLiterals;

QQmlCodeModelManager::QQmlCodeModelManager(QObject *parent, QQmlToolingSharedSettings *settings)
    : QObject{ parent }, m_settings(settings), m_pluginLoader(QmlLSPluginInterface_iid, u"/qmlls"_s)
{
    const QByteArray defaultCodeModel;
    appendWorkspace(defaultCodeModel, ManagedByServer);
}

QQmlCodeModelManager::WorkspaceIterator
QQmlCodeModelManager::findWorkspaceForFile(const QByteArray &url)
{
    Q_ASSERT(!m_workspaces.empty());
    // if file was already opened before: re-use same CodeModel as last time
    if (auto it = m_file2CodeModel.find(url); it != m_file2CodeModel.end()) {
        const auto result = findWorkspace(it->second);
        Q_ASSERT(result != m_workspaces.end());
        return result;
    }

    long longestRootUrl = 0;
    WorkspaceIterator result = m_workspaces.begin();
    for (auto it = m_workspaces.begin(), end = m_workspaces.end(); it != end; ++it) {
        if (it->toBeClosed)
            continue;
        const QByteArray rootUrl = it->url;
        if (!url.startsWith(rootUrl))
            continue;

        if (rootUrl.size() == url.size())
            return it;

        const long rootUrlLength = rootUrl.length();
        if (rootUrlLength > longestRootUrl) {
            longestRootUrl = rootUrlLength;
            result = it;
        }
    }

    // check .qmlls.build.ini for a potentially better match
    if (const ModuleSetting moduleSetting =
                m_buildInformation.settingFor(QUrl::fromEncoded(url).toLocalFile());
        !moduleSetting.importPaths.isEmpty()) {
        const QByteArray rootUrl = QUrl::fromLocalFile(moduleSetting.sourceFolder).toEncoded();
        if (longestRootUrl < rootUrl.size()) {
            appendWorkspace(rootUrl, ManagedByServer);
            return --m_workspaces.end();
        }
    }
    Q_ASSERT(result != m_workspaces.end());
    return result;
}

QQmlCodeModel *QQmlCodeModelManager::findCodeModelForFile(const QByteArray &url)
{
    return findWorkspaceForFile(url)->codeModel.get();
}

QQmlCodeModelManager::WorkspaceMutableIterator
QQmlCodeModelManager::findWorkspace(const QByteArray &url)
{
    return std::find_if(m_workspaces.begin(), m_workspaces.end(),
                        [&url](const QQmlWorkspace &ws) { return ws.url == url; });
}

void QQmlCodeModelManager::appendWorkspace(const QByteArray &url, ManagedBy managedBy)
{
    QQmlWorkspace ws;
    ws.url = url;
    ws.codeModel = std::make_unique<QQmlCodeModel>(this, m_settings);
    ws.codeModel->setRootUrls({ url });

    // set default values
    if (!m_defaultImportPaths.isEmpty())
        ws.codeModel->setImportPaths(m_defaultImportPaths);
    if (m_defaultDisableCMakeCalls)
        ws.codeModel->disableCMakeCalls();
    if (!m_defaultDocumentationRootPath.isEmpty())
        ws.codeModel->setDocumentationRootPath(m_defaultDocumentationRootPath);

    // set values from already known .qmlls.build.ini files
    if (const QStringList importPaths =
                m_buildInformation.importPathsFor(QUrl::fromEncoded(url).toLocalFile());
        !importPaths.isEmpty()) {
        ws.codeModel->setImportPaths(importPaths);
    }

    QObject::connect(ws.codeModel.get(), &QQmlCodeModel::updatedSnapshot, this,
                     &QQmlCodeModelManager::updatedSnapshot);
    ws.managedByClient = managedBy == ManagedByClient;
    m_workspaces.emplace_back(std::move(ws));
}

QQmlCodeModelManager::WorkspaceIterator
QQmlCodeModelManager::workspaceFromBuildFolder(const QString &fileName,
                                               const QStringList &buildFolders)
{
    m_buildInformation.loadSettingsFrom(buildFolders);
    const ModuleSetting setting = m_buildInformation.settingFor(fileName);
    QByteArray url = QUrl::fromLocalFile(setting.sourceFolder).toEncoded();
    if (auto it = findWorkspace(url); it != m_workspaces.end())
        return it;
    appendWorkspace(url, ManagedByServer);
    return --m_workspaces.end();
}

void QQmlCodeModelManager::disableCMakeCalls()
{
    m_defaultDisableCMakeCalls = true;
    for (const auto &ws : m_workspaces)
        ws.codeModel->disableCMakeCalls();
}

OpenDocumentSnapshot QQmlCodeModelManager::snapshotByUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->snapshotByUrl(url);
}

void QQmlCodeModelManager::removeDirectory(const QByteArray &url)
{
    findCodeModelForFile(url)->removeDirectory(url);
}

void QQmlCodeModelManager::newOpenFile(const QByteArray &url, int version, const QString &docText)
{
    const auto ws = findWorkspaceForFile(url);
    m_file2CodeModel[url] = ws->url;
    ws->codeModel->newOpenFile(url, version, docText);
}

OpenDocument QQmlCodeModelManager::openDocumentByUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->openDocumentByUrl(url);
}

RegisteredSemanticTokens &QQmlCodeModelManager::registeredTokens(const QByteArray &url)
{
    return findCodeModelForFile(url)->registeredTokens();
}

void QQmlCodeModelManager::newDocForOpenFile(const QByteArray &url, int version,
                                             const QString &docText)
{
    findCodeModelForFile(url)->newDocForOpenFile(url, version, docText);
}

void QQmlCodeModelManager::closeOpenFile(const QByteArray &url)
{
    m_file2CodeModel.erase(url);
    const auto it = findWorkspaceForFile(url);
    it->codeModel->closeOpenFile(url);

    // don't close the default workspace
    if (it->url.isEmpty())
        return;

    // close empty WS when managed by server or when client marked ws as toBeClosed.
    if ((it->managedByClient && it->toBeClosed) || !it->managedByClient) {
        if (it->codeModel->isEmpty())
            m_workspaces.erase(it);
    }
}

QList<QByteArray> QQmlCodeModelManager::rootUrls() const
{
    QList<QByteArray> result;
    result.reserve(m_workspaces.size());
    for (const QQmlWorkspace &ws : m_workspaces) {
        result << ws.url;
    }
    return result;
}

void QQmlCodeModelManager::addRootUrls(const QList<QByteArray> &urls)
{
    for (const QByteArray &url : urls) {
        if (const auto it = findWorkspace(url); it != m_workspaces.end()) {
            it->toBeClosed = false;
            continue;
        }

        appendWorkspace(url, ManagedByClient);
    }
}

void QQmlCodeModelManager::removeRootUrls(const QList<QByteArray> &urls)
{
    for (const QByteArray &url : urls) {
        if (auto it = findWorkspace(url); it != m_workspaces.end() && it->managedByClient)
            it->toBeClosed = true;
    }
}

QStringList QQmlCodeModelManager::importPathsForUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->importPathsForUrl(url);
}

QStringList QQmlCodeModelManager::buildPathsForFileUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->buildPathsForFileUrl(url);
}

void QQmlCodeModelManager::setDocumentationRootPath(const QString &path)
{
    m_defaultDocumentationRootPath = path;
    for (const auto &ws : m_workspaces)
        ws.codeModel->setDocumentationRootPath(path);
}

void QQmlCodeModelManager::setBuildPathsForRootUrl(const QByteArray &url, const QStringList &paths)
{
    m_buildInformation.loadSettingsFrom(paths);

    const auto ws = findWorkspaceForFile(url);
    if (ws->url == url) {
        if (const QStringList importPaths =
                    m_buildInformation.importPathsFor(QUrl::fromEncoded(url).toLocalFile());
            !importPaths.isEmpty()) {
            ws->codeModel->setImportPaths(importPaths);
        }
    }
    ws->codeModel->setBuildPathsForRootUrl(url, paths);
}

void QQmlCodeModelManager::addOpenToUpdate(const QByteArray &url)
{
    findCodeModelForFile(url)->addOpenToUpdate(url);
}
void QQmlCodeModelManager::setImportPaths(const QStringList &paths)
{
    m_defaultImportPaths = paths;
    for (const auto &ws : m_workspaces)
        ws.codeModel->setImportPaths(paths);
}

HelpManager *QQmlCodeModelManager::helpManagerForUrl(const QByteArray &url)
{
    return findCodeModelForFile(url)->helpManager();
}

} // namespace QmlLsp

QT_END_NAMESPACE
