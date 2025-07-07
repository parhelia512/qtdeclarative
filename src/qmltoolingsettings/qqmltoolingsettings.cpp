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

QQmlToolingSettings::SearchResult QQmlToolingSettings::read(const QString &settingsFilePath)
{
#if QT_CONFIG(settings)
    if (!QFileInfo::exists(settingsFilePath))
        return { SearchResult::ResultType::NotFound, settingsFilePath };

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
    const QString path = QFileInfo(u".%1.ini"_s.arg(m_toolName)).absoluteFilePath();

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
QQmlToolingSettings::searchCurrentDirInCache(const QString &dirPath)
{
    return m_seenDirectories.contains(dirPath)
            ? read(m_seenDirectories[dirPath])
            : SearchResult{ SearchResult::ResultType::NotFound, {} };
}

QQmlToolingSettings::SearchResult QQmlToolingSettings::searchDefaultLocation(QSet<QString> *visitedDirs)
{
    const QString settingsFileName = u".%1.ini"_s.arg(m_toolName);
    // If we reach here, we didn't find the settings file in the current directory or any parent
    // directories. Now we will try to locate the settings file in the standard locations. First try
    // to locate settings file with the standard name.
    QString iniFile =
            QStandardPaths::locate(QStandardPaths::GenericConfigLocation, settingsFileName);
    // If not found, try alternate name format
    if (iniFile.isEmpty()) {
        iniFile = QStandardPaths::locate(QStandardPaths::GenericConfigLocation,
                                         u"%1.ini"_s.arg(m_toolName));
    }

    // If a valid settings file was found, read it and update directory cache
    const auto result = read(iniFile);

    // Update the seen directories cache unconditionally with the current result
    for (auto &dir : *visitedDirs)
        m_seenDirectories[dir] = result.iniFilePath;

    return result;
}

QQmlToolingSettings::SearchResult
QQmlToolingSettings::searchDirectoryHierarchy(QSet<QString> *visitedDirs, QDir dir,
                                              const QString &settingsFileName)
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
        const QString iniFile = dir.absoluteFilePath(settingsFileName);
        if (result = read(iniFile); result.isValid()) {
            for (const QString &visitedDir : *visitedDirs)
                m_seenDirectories[visitedDir] = result.iniFilePath;

            return result;
        }

        if (!dir.cdUp())
            break;
    }

    return result;
}

QQmlToolingSettings::SearchResult QQmlToolingSettings::search(const QString &path)
{
    SearchResult result;
#if QT_CONFIG(settings)
    QFileInfo fileInfo(path);
    QDir dir(fileInfo.isDir() ? path : fileInfo.dir());
    QSet<QString> visitedDirs;
    const QString settingsFileName = u".%1.ini"_s.arg(m_toolName);

    // Try to find settings in directory hierarchy
    if (result = searchDirectoryHierarchy(&visitedDirs, dir, settingsFileName); result.isValid())
        return result;

    // If we didn't find the settings file in the current directory or any parent directories,
    // try to locate the settings file in the standard locations
    if (result = searchDefaultLocation(&visitedDirs); result.isValid())
        return result;

#endif
    Q_UNUSED(path);
    return result;
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
