// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljsloggingutils_p.h"

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtCore/qcommandlineparser.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QQmlSA::LoggerWarningId
    \inmodule QtQmlCompiler

    \brief A wrapper around a string literal to uniquely identify
    warning categories in the \c{QQmlSA} framework.
*/

/*!
    \fn QQmlSA::LoggerWarningId::LoggerWarningId(QAnyStringView name)
    Constructs a LoggerWarningId object with logging catergory name \a name.
*/

/*!
    \fn QAnyStringView QQmlSA::LoggerWarningId::name() const
    Returns the name of the wrapped warning category.
*/

namespace QQmlJS {

LoggerCategory::LoggerCategory() : d_ptr{ new LoggerCategoryPrivate } { }

LoggerCategory::LoggerCategory(
        const QString &name, const QString &settingsName, const QString &description, WarningLevel level,
        bool isDefault)
    : d_ptr{ new LoggerCategoryPrivate(name, settingsName, description, level, isDefault) }
{
}

LoggerCategory::LoggerCategory(const LoggerCategory &other)
    : d_ptr{ new LoggerCategoryPrivate{ *other.d_func() } }
{
}

LoggerCategory::LoggerCategory(LoggerCategory &&) noexcept = default;

LoggerCategory &LoggerCategory::operator=(const LoggerCategory &other)
{
    *d_func() = *other.d_func();
    return *this;
}

LoggerCategory &LoggerCategory::operator=(LoggerCategory &&) noexcept = default;

LoggerCategory::~LoggerCategory() = default;

QString LoggerCategory::name() const
{
    Q_D(const LoggerCategory);
    return d->name();
}

QString LoggerCategory::settingsName() const
{
    Q_D(const LoggerCategory);
    return d->settingsName();
}

QString LoggerCategory::description() const
{
    Q_D(const LoggerCategory);
    return d->description();
}

WarningLevel LoggerCategory::level() const
{
    Q_D(const LoggerCategory);
    return d->level();
}

bool LoggerCategory::isDefault() const
{
    Q_D(const LoggerCategory);
    return d->isDefault();
}

LoggerWarningId LoggerCategory::id() const
{
    Q_D(const LoggerCategory);
    return d->id();
}

void LoggerCategory::setLevel(WarningLevel level)
{
    Q_D(LoggerCategory);
    d->setLevel(level);
}

LoggerCategoryPrivate::LoggerCategoryPrivate(const QString &name, const QString &settingsName,
                                             const QString &description, WarningLevel level,
                                             bool isDefault)
    : m_name(name), m_settingsName(settingsName), m_description(description), m_level(level)
    , m_isDefault(isDefault)
{
}

void LoggerCategoryPrivate::setLevel(WarningLevel level)
{
    if (m_level == level)
        return;

    m_level = level;
    m_changed = true;
}

LoggerCategoryPrivate *LoggerCategoryPrivate::get(LoggerCategory *loggerCategory)
{
    Q_ASSERT(loggerCategory);
    return loggerCategory->d_func();
}

namespace LoggingUtils {

QString levelToString(const QQmlJS::LoggerCategory &category)
{
    switch (category.level()) {
    case QQmlJS::WarningLevel::Disable:
        return QStringLiteral("disable");
    case QQmlJS::WarningLevel::Info:
        return QStringLiteral("info");
    case QQmlJS::WarningLevel::Warning:
        return QStringLiteral("warning");
    case QQmlJS::WarningLevel::Error:
        return QStringLiteral("error");
    default:
        Q_UNREACHABLE();
        break;
    }
};

static QStringList settingsNamesForCategory(const LoggerCategory &category)
{
    const QString name = category.settingsName();
    QStringList result{ QStringLiteral("Warnings/") + name };
    const QString sliced = "Warnings/"_L1 + name.sliced(name.indexOf(u'.') + 1);
    if (sliced != result.last())
        result.append(sliced);
    return result;
}

static QString lookInSettings(const LoggerCategory &category, const QQmlToolingSettings &settings,
                              const QString &settingsName)
{
    if (settings.isSet(settingsName))
        return settings.value(settingsName).toString();
    static constexpr QLatin1String propertyAliasCyclesKey = "Warnings/PropertyAliasCycles"_L1;

    // keep compatibility to deprecated settings
    if (category.name() == qmlAliasCycle.name() || category.name() == qmlUnresolvedAlias.name()) {
        if (settings.isSet(propertyAliasCyclesKey)) {
            qWarning()
                    << "Detected deprecated setting name \"PropertyAliasCycles\". Use %1 or %2 instead."_L1
                               .arg(qmlAliasCycle.name(), qmlUnresolvedAlias.name());
            return settings.value(propertyAliasCyclesKey).toString();
        }
    }
    return {};
}

static QString levelValueForCategory(const LoggerCategory &category,
                                     const QQmlToolingSettings &settings,
                                     QCommandLineParser *parser)
{
    const QString key = category.id().name().toString();
    if (parser && parser->isSet(key))
        return parser->value(key);

    const QStringList settingsName = settingsNamesForCategory(category);
    for (const QString &settingsName : settingsName) {
        const QString value = lookInSettings(category, settings, settingsName);
        if (value.isEmpty())
            continue;

        // Do not try to set the levels if it's due to a default config option.
        // This way we can tell which options have actually been overwritten by the user.
        if (levelToString(category) == value)
            return QString();

        return value;
    }
    return QString();
}

bool applyLevelToCategory(const QStringView level, LoggerCategory &category)
{
    // TODO yes we want to allow downgrading errors. But not essentials
    // you can't downgrade errors
    if (category.level() == QQmlJS::WarningLevel::Error && level != "error"_L1)
        return false;

    if (level == "disable"_L1) {
        category.setLevel(QQmlJS::WarningLevel::Disable);
        return true;
    }
    if (level == "info"_L1) {
        category.setLevel(QQmlJS::WarningLevel::Info);
        return true;
    }
    if (level == "warning"_L1) {
        category.setLevel(QQmlJS::WarningLevel::Warning);
        return true;
    }
    if (level == "error"_L1) {
        category.setLevel(QQmlJS::WarningLevel::Error);
        return true;
    }

    return false;
};

/*!
\internal
Sets the category levels from a settings file and an optional parser.
Calls \c {parser->showHelp(-1)} for invalid logging levels.
*/
void updateLogLevels(QList<LoggerCategory> &categories,
                     const QQmlToolingSettings &settings,
                     QCommandLineParser *parser)
{
    bool success = true;
    for (auto &category : categories) {
        if (category.isDefault())
            continue;

        const QString value = levelValueForCategory(category, settings, parser);
        if (value.isEmpty())
            continue;

        if (!applyLevelToCategory(value, category)) {
            qWarning() << "Invalid logging level" << value << "provided for"
                       << category.id().name().toString()
                       << "(allowed are: disable, info, warning, error)\n."
                          "You can't change categories that have level \"error\" by default."; // TODO we want that to be changeable
            success = false;
        }
    }
    if (!success && parser)
        parser->showHelp(-1);
}
} // namespace LoggingUtils

} // namespace QQmlJS

QT_END_NAMESPACE
