// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_differ.h"

#include <QtQmlLS/private/qqmldiffer_p.h>

using namespace QQmlLSUtils;
using namespace Qt::StringLiterals;

static QString commandString(Diff::Command com)
{
    if (com == Diff::Command::Delete)
        return "Delete"_L1;
    else if (com == Diff::Command::Insert)
        return "Insert"_L1;
    return "Equal"_L1;
}

static QString toString(const Diff &diff)
{
    return QString("%1 \"%2\"").arg(commandString(diff.command)).arg(diff.text);
}

tst_qmlls_differ::tst_qmlls_differ()
{
}

void tst_qmlls_differ::computeDiff_data()
{
    QTest::addColumn<QString>("text1");
    QTest::addColumn<QString>("text2");
    QTest::addColumn<QList<QQmlLSUtils::Diff>>("expectedDiff");

    QTest::newRow("no change")
            << "line1\nline2\nline3"
            << "line1\nline2\nline3"
            << QList<Diff> { Diff(Diff::Equal, "line1\nline2\nline3") };

    QTest::newRow("insert at start")
            << "line1\nline2\nline3"
            << "newline0\nline1\nline2\nline3"
            << QList<Diff> { Diff(Diff::Insert, "newline0\n"),
                             Diff(Diff::Equal, "line1\nline2\nline3") };

    QTest::newRow("insert at end")
            << "line1\nline2\nline3"
            << "line1\nline2\nline3\nnewline4"
            << QList<Diff> { Diff(Diff::Equal, "line1\nline2\nline3"),
                             Diff(Diff::Insert, "\nnewline4") };

    QTest::newRow("insert in middle")
            << "line1\nline2\nline3"
            << "line1\nnewline2.5\nline2\nline3"
            << QList<Diff> { Diff(Diff::Equal, "line1\n"),
                             Diff(Diff::Insert, "newline2.5\n"),
                             Diff(Diff::Equal, "line2\nline3") };

    QTest::newRow("delete at start")
            << "line1\nline2\nline3"
            << "line2\nline3"
            << QList<Diff> { Diff(Diff::Delete, "line1\n"),
                             Diff(Diff::Equal, "line2\nline3") };

    QTest::newRow("delete at end")
            << "line1\nline2\nline3"
            << "line1\nline2"
            << QList<Diff> { Diff(Diff::Equal, "line1\nline2"),
                             Diff(Diff::Delete, "\nline3") };

    QTest::newRow("delete in middle")
            << "line1\nline2\nline3"
            << "line1\nline3"
            << QList<Diff> { Diff(Diff::Equal, "line1\nline"),
                             Diff(Diff::Delete, "2\nline"),
                             Diff(Diff::Equal, "3") };
}


void tst_qmlls_differ::computeDiff()
{
    QFETCH(QString, text1);
    QFETCH(QString, text2);
    QFETCH(QList<Diff>, expectedDiff);

    Differ differ;
    const auto diff = differ.diff(text1, text2);

    [&]{
        QCOMPARE(diff, expectedDiff);
    }();

    if (QTest::currentTestFailed()) {
        qDebug() << "text1:" << text1;
        qDebug() << "text2:" << text2;
        qDebug() << "computed diff:";
        for (const auto &d : diff)
            qDebug() << "  " << toString(d);
        qDebug() << "expectedDiff:";
        for (const auto &d : expectedDiff)
            qDebug() << "  " << toString(d);
    }
}

QTEST_MAIN(tst_qmlls_differ)
