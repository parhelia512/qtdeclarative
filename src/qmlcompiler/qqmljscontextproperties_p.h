// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSCONTEXTPROPERTIES_P_H
#define QQMLJSCONTEXTPROPERTIES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/private/qflatmap_p.h>

#include <QtQml/private/qqmljssourcelocation_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
class LoggerCategory;

struct Q_QMLCOMPILER_EXPORT HeuristicContextProperty
{
    QString filename = {};
    SourceLocation location = SourceLocation{};
};

class Q_QMLCOMPILER_EXPORT HeuristicContextProperties
{
public:
    bool contains(const QString &name) const { return m_properties.contains(name); }
    qsizetype size() const { return m_properties.size(); }
    QList<HeuristicContextProperty> definitionsForName(const QString &name) const;

    static HeuristicContextProperties collectFromCppSourceDirs(const QList<QString> &cppSourceDirs);

private:
    struct Entry
    {
        QString name;
        QList<HeuristicContextProperty> definitions;
    };

    void add(const QString &name, const HeuristicContextProperty &property);
    void collectFromDirs(const QList<QString> &dirs);
    void collectFromFile(const QString &file);
    void grepFallback(const QList<QString> &rootUrls);
#if QT_CONFIG(process) && !defined(Q_OS_WINDOWS)
    void parseGrepOutput(const QString &output);
#endif

    QFlatMap<QString, QList<HeuristicContextProperty>> m_properties;
};

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSCONTEXTPROPERTIES_P_H
