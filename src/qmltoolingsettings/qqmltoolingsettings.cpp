// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltoolingsettings_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qset.h>
#if QT_CONFIG(settings)
#include <QtCore/qsettings.h>
#endif
#include <QtCore/qstandardpaths.h>

using namespace Qt::StringLiterals;

void QQmlToolingSettings::addOption(const QString &name, QVariant defaultValue)
{
    if (defaultValue.isValid()) {
        m_values[name] = defaultValue;
    }
}

QQmlToolingSettings::QQmlToolingSettings(const QString &toolName)
    : m_searcher(u".%1.ini"_s.arg(toolName), u"%1.ini"_s.arg(toolName))
{
}

QQmlToolingSettings::SearchResult QQmlToolingSettings::read(const QString &settingsFilePath)
{
#if QT_CONFIG(settings)
    Q_ASSERT(QFileInfo::exists(settingsFilePath));

    if (m_currentSettingsPath == settingsFilePath)
        return { SearchResult::ResultType::Found, settingsFilePath };

    QSettings settings(settingsFilePath, QSettings::IniFormat);
    for (const QString &key : settings.allKeys())
        m_values[key] = settings.value(key).toString();

    m_currentSettingsPath = settingsFilePath;
    return { SearchResult::ResultType::Found, settingsFilePath };
#else
    Q_UNUSED(settingsFilePath);
    return { SearchResult::ResultType::Error, {} };
#endif
}

bool QQmlToolingSettings::writeDefaults() const
{
#if QT_CONFIG(settings)
    const QString path = QFileInfo(m_searcher.localSettingsFile()).absoluteFilePath();

    QSettings settings(path, QSettings::IniFormat);
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it) {
        settings.setValue(it.key(), it.value().isNull() ? QString() : it.value());
    }

    settings.sync();

    if (settings.status() != QSettings::NoError) {
        qWarning() << "Failed to write default settings to" << path
                   << "Error:" << settings.status();
        return false;
    }

    qInfo() << "Wrote default settings to" << path;
    return true;
#else
    return false;
#endif
}

QQmlToolingSettings::SearchResult
QQmlToolingSettings::Searcher::searchCurrentDirInCache(const QString &dirPath)
{
    const auto it = m_seenDirectories.constFind(dirPath);
    return it != m_seenDirectories.constEnd()
            ? SearchResult{ SearchResult::ResultType::Found, *it }
            : SearchResult{ SearchResult::ResultType::NotFound, {} };
}

QQmlToolingSettings::SearchResult
QQmlToolingSettings::Searcher::searchDefaultLocation(QSet<QString> *visitedDirs)
{
    // If we reach here, we didn't find the settings file in the current directory or any parent
    // directories. Now we will try to locate the settings file in the standard locations. First try
    // to locate settings file with the standard name.
    QString iniFile =
            QStandardPaths::locate(QStandardPaths::GenericConfigLocation, m_localSettingsFile);
    // If not found, try alternate name format
    if (iniFile.isEmpty()) {
        iniFile =
                QStandardPaths::locate(QStandardPaths::GenericConfigLocation, m_globalSettingsFile);
    }

    // Update the seen directories cache unconditionally with the current result
    for (auto &dir : *visitedDirs)
        m_seenDirectories[dir] = iniFile;

    return SearchResult{ iniFile.isEmpty() ? SearchResult::ResultType::NotFound
                                           : SearchResult::ResultType::Found,
                iniFile };
}

QQmlToolingSettings::SearchResult
QQmlToolingSettings::Searcher::searchDirectoryHierarchy(QSet<QString> *visitedDirs, QDir dir)
{
    SearchResult result = { SearchResult::ResultType::NotFound, {} };
    while (dir.exists() && dir.isReadable()) {
        const QString dirPath = dir.absolutePath();

        // Check if we have already seen this directory
        // If we have, we can use the cached INI file path
        // to avoid unnecessary file system operations
        // This is useful for large directory trees where the settings file might be in a parent
        // directory
        result = searchCurrentDirInCache(dirPath);
        if (result.isValid())
            return result;

        visitedDirs->insert(dirPath);

        // Check if the settings file exists in the current directory
        // If it does, read it and update the seen directories cache
        const QString iniFile = dir.absoluteFilePath(m_localSettingsFile);
        if (QFileInfo::exists(iniFile)) {
            for (const QString &visitedDir : *visitedDirs)
                m_seenDirectories[visitedDir] = iniFile;

            return { SearchResult::ResultType::Found, iniFile };
        }

        if (!dir.cdUp())
            break;
    }

    return result;
}

QQmlToolingSettings::SearchResult QQmlToolingSettings::Searcher::search(const QString &path)
{
    SearchResult result;
#if QT_CONFIG(settings)
    QFileInfo fileInfo(path);
    QDir dir(fileInfo.isDir() ? path : fileInfo.dir());
    QSet<QString> visitedDirs;

    // Try to find settings in directory hierarchy
    if (result = searchDirectoryHierarchy(&visitedDirs, dir); result.isValid())
        return result;

    // If we didn't find the settings file in the current directory or any parent directories,
    // try to locate the settings file in the standard locations
    if (result = searchDefaultLocation(&visitedDirs); result.isValid())
        return result;

#endif
    Q_UNUSED(path);
    return result;
}

QQmlToolingSettings::SearchResult QQmlToolingSettings::search(const QString &path)
{
    const SearchResult result = m_searcher.search(path);
    return result.isValid() ? read(result.iniFilePath) : result;
}

QVariant QQmlToolingSettings::value(const QString &name) const
{
    return m_values.value(name);
}

bool QQmlToolingSettings::isSet(const QString &name) const
{
    if (!m_values.contains(name))
        return false;

    QVariant variant = m_values[name];

    // Unset is encoded as an empty string
    return !(variant.canConvert(QMetaType(QMetaType::QString)) && variant.toString().isEmpty());
}
