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
        const QString &name, const QString &settingsName, const QString &description, WarningSeverity severity,
        EssentialCategory essential)
    : d_ptr{ new LoggerCategoryPrivate(name, settingsName, description, severity, essential) }
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

WarningSeverity LoggerCategory::severity() const
{
    Q_D(const LoggerCategory);
    return d->severity();
}

bool LoggerCategory::isEssential() const
{
    Q_D(const LoggerCategory);
    return d->isEssential();
}

LoggerWarningId LoggerCategory::id() const
{
    Q_D(const LoggerCategory);
    return d->id();
}

void LoggerCategory::setSeverity(WarningSeverity severity)
{
    Q_D(LoggerCategory);
    d->setSeverity(severity);
}

LoggerCategoryPrivate::LoggerCategoryPrivate(const QString &name, const QString &settingsName,
                                             const QString &description, WarningSeverity severity,
                                             LoggerCategory::EssentialCategory isEssential)
    : m_name(name), m_settingsName(settingsName), m_description(description), m_severity(severity)
    , m_isEssential(isEssential)
{
}

void LoggerCategoryPrivate::setSeverity(WarningSeverity severity)
{
    if (m_severity == severity)
        return;

    m_severity = severity;
    m_changed = true;
}

LoggerCategoryPrivate *LoggerCategoryPrivate::get(LoggerCategory *loggerCategory)
{
    Q_ASSERT(loggerCategory);
    return loggerCategory->d_func();
}

namespace LoggingUtils {

QString severityToString(const QQmlJS::LoggerCategory &category)
{
    switch (category.severity()) {
    case QQmlJS::WarningSeverity::Disable:
        return QStringLiteral("disable");
    case QQmlJS::WarningSeverity::Info:
        return QStringLiteral("info");
    case QQmlJS::WarningSeverity::Warning:
        return QStringLiteral("warning");
    case QQmlJS::WarningSeverity::Error:
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

static QString severityValueForCategory(const LoggerCategory &category,
                                     const QQmlToolingSettings &settings,
                                     QCommandLineParser *parser)
{
    const QString key = category.id().name().toString();

    // Essential categories have no option as their severity cannot be changed.
    if (!category.isEssential() && parser && parser->isSet(key))
        return parser->value(key);

    const QStringList settingsName = settingsNamesForCategory(category);
    for (const QString &settingsName : settingsName) {
        const QString value = lookInSettings(category, settings, settingsName);
        if (value.isEmpty())
            continue;

        // Do not try to set the severity if it's due to a default config option.
        // This way we can tell which options have actually been overwritten by the user.
        if (severityToString(category) == value)
            return QString();

        return value;
    }
    return QString();
}

bool applySeverityToCategory(const QStringView severity, LoggerCategory &category)
{
    if (severity == "disable"_L1) {
        category.setSeverity(QQmlJS::WarningSeverity::Disable);
        return true;
    }
    if (severity == "info"_L1) {
        category.setSeverity(QQmlJS::WarningSeverity::Info);
        return true;
    }
    if (severity == "warning"_L1) {
        category.setSeverity(QQmlJS::WarningSeverity::Warning);
        return true;
    }
    if (severity == "error"_L1) {
        category.setSeverity(QQmlJS::WarningSeverity::Error);
        return true;
    }

    return false;
};

/*!
\internal
Sets the category severity from a settings file and an optional parser.
Calls \c {parser->showHelp(-1)} for an invalid logging severity.
*/
void updateLogSeverities(QList<LoggerCategory> &categories,
                     const QQmlToolingSettings &settings,
                     QCommandLineParser *parser)
{
    bool success = true;
    for (auto &category : categories) {
        const QString value = severityValueForCategory(category, settings, parser);
        if (value.isEmpty())
            continue;

        const QString &name = category.id().name().toString();
        if (category.isEssential()) {
            qWarning() << "In order to ensure the proper function of qmllint, the severity of the "
                          "essential category %1 cannot be changed."_L1.arg(name);
            continue;
        }

        if (!applySeverityToCategory(value, category)) {
            qWarning() << "Invalid logging severity" << value << "provided for" << name
                       << "(allowed are: disable, info, warning, error).";
            success = false;
        }
    }
    if (!success && parser)
        parser->showHelp(-1);
}
} // namespace LoggingUtils

} // namespace QQmlJS

QT_END_NAMESPACE
