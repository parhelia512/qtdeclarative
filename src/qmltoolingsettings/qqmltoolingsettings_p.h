// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLTOOLINGSETTINGS_P_H
#define QQMLTOOLINGSETTINGS_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmutex.h>
#include <QtCore/qset.h>
#include <QtCore/qdir.h>

QT_BEGIN_NAMESPACE

class QQmlToolingSettings
{
public:
    struct SearchResult
    {
        enum class ResultType { Found, NotFound, Error };
        ResultType type = ResultType::NotFound;
        QString iniFilePath;
        bool isValid() const { return type == ResultType::Found && !iniFilePath.isEmpty(); }
    };
    QQmlToolingSettings(const QString &toolName) : m_toolName(toolName) { }

    void addOption(const QString &name, const QVariant defaultValue = QVariant());

    bool writeDefaults() const;
    SearchResult search(const QString &path);

    QVariant value(const QString &name) const;
    bool isSet(const QString &name) const;

private:
    QString m_toolName;
    QString m_currentSettingsPath;
    QHash<QString, QString> m_seenDirectories;
    QVariantHash m_values;

    SearchResult read(const QString &settingsFilePath);
    SearchResult searchDefaultLocation(QSet<QString> *visitedDirs);
    SearchResult searchCurrentDirInCache(const QString &dirPath);
    SearchResult searchDirectoryHierarchy(QSet<QString> *visitedDir, QDir dir,
                                          const QString &settingsFileName);
};

class QQmlToolingSharedSettings : private QQmlToolingSettings
{
public:
    QQmlToolingSharedSettings(const QString &toolName) : QQmlToolingSettings(toolName) { }

    void addOption(const QString &name, const QVariant defaultValue = QVariant())
    {
        QMutexLocker lock(&m_mutex);
        QQmlToolingSettings::addOption(name, defaultValue);
    }

    bool writeDefaults() const
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::writeDefaults();
    }

    SearchResult search(const QString &path)
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::search(path);
    }

    QVariant value(const QString &name) const
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::value(name);
    }

    bool isSet(const QString &name) const
    {
        QMutexLocker lock(&m_mutex);
        return QQmlToolingSettings::isSet(name);
    }

private:
    mutable QMutex m_mutex;
};

QT_END_NAMESPACE

#endif // QQMLTOOLINGSETTINGS_P_H
