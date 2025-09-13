// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlformatsettings_p.h"

#include <QCommandLineParser>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

QQmlFormatSettings::QQmlFormatSettings(const QString &toolName) : QQmlToolingSettings(toolName)
{
    addOption(s_useTabsSetting, false);
    addOption(s_indentWidthSetting, 4);
    addOption(s_maxColumnWidthSetting, -1);
    addOption(s_normalizeSetting, false);
    addOption(s_newlineSetting, QStringLiteral("native"), QStringList{ "unix", "windows", "macos", "native" });
    addOption(s_objectsSpacingSetting, false);
    addOption(s_functionsSpacingSetting, false);
    addOption(s_sortImportsSetting, false);
    addOption(s_singleLineEmptyObjectsSetting, false);
    addOption(s_semiColonRuleSetting, QStringLiteral("always"), QStringList{ "always", "essential" });
}

void QQmlFormatSettings::addOption(const QString &name, const QVariant &defaultValue, const QStringList &allowedValues)
{
    QQmlToolingSettings::addOption(name, defaultValue);
    if (defaultValue.typeId() == QMetaType::QString) {
        Q_ASSERT(!allowedValues.isEmpty());
        m_allowedValues[name] = allowedValues;
    }
}

bool QQmlFormatSettings::outputOptions() const
{
    QJsonObject root;
    QJsonArray optionsArray;
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it) {
        QJsonObject option;
        option[QStringLiteral("name")] = it.key();
        option[QStringLiteral("value")] = QJsonValue::fromVariant(it.value());
        option[QStringLiteral("hint")] = it.value().typeName();

        if (it.value().typeId() == QMetaType::QString)
            option[QStringLiteral("hint")] = m_allowedValues[it.key()].join(',');

        optionsArray.append(option);
    }
    root[QStringLiteral("options")] = optionsArray;

    QCommandLineParser::showMessageAndExit(QCommandLineParser::MessageType::Information,
                                           QJsonDocument(root).toJson(), EXIT_SUCCESS);
    return true;
}
