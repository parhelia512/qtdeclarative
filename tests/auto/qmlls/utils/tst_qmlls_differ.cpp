// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_differ.h"

#include <QtQmlLS/private/qqmldiffer_p.h>
#include <QtQmlLS/private/qqmlsemantictokens_p.h>

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

void tst_qmlls_differ::applyDiffs_data()
{
    using namespace QmlHighlighting;
    QTest::addColumn<QString>("text1");
    QTest::addColumn<QString>("text2");
    QTest::addColumn<QmlHighlighting::HighlightsContainer>("lastHighlights");
    QTest::addColumn<QmlHighlighting::HighlightsContainer>("newHighlights");

    // We only need to test the QQmlJS::SourceLocation part of HighlightToken.
    // Thus, use dummy data for the rest of the fields.
    const auto makeToken = [](qsizetype offset, qsizetype length, int startLine, int startColumn) {
        return HighlightToken(QQmlJS::SourceLocation(offset, length, startLine, startColumn),
                              QmlHighlightKind::Unknown);
    };
    // Test cases for insertions
    {
        const QString code = QStringLiteral("line1 line11\nline2\nline3");
        QmlHighlighting::HighlightsContainer codeTokens;
        codeTokens.insert(0, makeToken(0, 5, 1, 1));
        codeTokens.insert(6, makeToken(6, 6, 1, 7));
        codeTokens.insert(13, makeToken(13, 5, 2, 1));
        codeTokens.insert(19, makeToken(19, 5, 3, 1));

        {
            // Appending shouldn't affect the existing tokens.
            QString modifiedCode(code);
            modifiedCode.append(" new");
            QTest::newRow("single line insert at end no overlap")
                    << code << modifiedCode << codeTokens << codeTokens;
        }
        {
            // Appending but touches the last token. It should expand the last token length.
            QString modifiedCode(code);
            modifiedCode.append("new");
            QmlHighlighting::HighlightsContainer modifiedCodeTokens = codeTokens;
            auto &lastToken = modifiedCodeTokens.last();
            lastToken.loc.length += 3;
            QTest::newRow("single line insert at end overlaps last token")
                    << code << modifiedCode << codeTokens << modifiedCodeTokens;
        }
        {
            // Insert at the start of the first line.
            // Offset of all tokens should be shifted,
            // plus columns of the first line tokens should be shifted.
            QString modifiedCode(code);
            modifiedCode.prepend("new");
            QmlHighlighting::HighlightsContainer modifiedCodeTokens;
            modifiedCodeTokens.insert(0, makeToken(0, 8, 1, 1));
            modifiedCodeTokens.insert(9, makeToken(9, 6, 1, 10));
            modifiedCodeTokens.insert(16, makeToken(16, 5, 2, 1));
            modifiedCodeTokens.insert(22, makeToken(22, 5, 3, 1));
            QTest::newRow("single line insert at start overlaps first token")
                    << code << modifiedCode << codeTokens << modifiedCodeTokens;
        }
        {
            QString modifiedCode(code);
            modifiedCode.prepend("ne ");
            QmlHighlighting::HighlightsContainer modifiedCodeTokens;
            modifiedCodeTokens.insert(3, makeToken(3, 5, 1, 4));
            modifiedCodeTokens.insert(9, makeToken(9, 6, 1, 10));
            modifiedCodeTokens.insert(16, makeToken(16, 5, 2, 1));
            modifiedCodeTokens.insert(22, makeToken(22, 5, 3, 1));
            QTest::newRow("single line insert at start no overlap")
                    << code << modifiedCode << codeTokens << modifiedCodeTokens;
        }
        {
            // Insert at the middle of the second token of the first line.
            // The first token should remain unchanged; the rest should be shifted correctly.
            QString modifiedCode(code);
            modifiedCode.insert(7, "new");
            QmlHighlighting::HighlightsContainer modifiedCodeTokens;
            modifiedCodeTokens.insert(0, makeToken(0, 5, 1, 1));
            modifiedCodeTokens.insert(6, makeToken(6, 9, 1, 7));
            modifiedCodeTokens.insert(16, makeToken(16, 5, 2, 1));
            modifiedCodeTokens.insert(22, makeToken(22, 5, 3, 1));
            QTest::newRow("single line insert at middle")
                    << code << modifiedCode << codeTokens << modifiedCodeTokens;
        }
        {
            // Insert at the start of the first line.
            // Offset of all tokens should be shifted,
            // plus columns of the first line tokens should be shifted.
            // Line numbers of all should be shifted due to multi-line insert.
            /// CCC part prepends to line1. line1 changes its offset and length and column.
            QString modifiedCode(code);
            modifiedCode.prepend("A\nBB\nCCC");
            QmlHighlighting::HighlightsContainer modifiedCodeTokens;
            modifiedCodeTokens.insert(5, makeToken(5, 8, 3, 1));
            modifiedCodeTokens.insert(14, makeToken(14, 6, 3, 10));
            modifiedCodeTokens.insert(21, makeToken(21, 5, 4, 1));
            modifiedCodeTokens.insert(27, makeToken(27, 5, 5, 1));
            QTest::newRow("multi line insert at start")
                    << code << modifiedCode << codeTokens << modifiedCodeTokens;
        }
        {
            // Insert at the middle of the first line.
            // Token that is being modified should be dropped from highlights.
            // The offset, line, and columns of the second token should be affected.
            // The offset and line of the rest should be affected.
            QString modifiedCode(code);
            modifiedCode.insert(5, "A\nBB\nCCC");
            QmlHighlighting::HighlightsContainer modifiedCodeTokens;
            modifiedCodeTokens.insert(0, makeToken(0, 6, 1, 1));
            modifiedCodeTokens.insert(14, makeToken(14, 6, 3, 5));
            modifiedCodeTokens.insert(21, makeToken(21, 5, 4, 1));
            modifiedCodeTokens.insert(27, makeToken(27, 5, 5, 1));
            QTest::newRow("multi line insert at the middle of the first line")
                    << code << modifiedCode << codeTokens << modifiedCodeTokens;
        }
    }
}

void tst_qmlls_differ::applyDiffs()
{
    QFETCH(QString, text1);
    QFETCH(QString, text2);
    QFETCH(QmlHighlighting::HighlightsContainer, lastHighlights);
    QFETCH(QmlHighlighting::HighlightsContainer, newHighlights);

    [&] {
        Differ differ;
        const auto diffs = differ.diff(text1, text2);
        QmlHighlighting::Utils::applyDiffs(lastHighlights, diffs);
        QCOMPARE(lastHighlights, newHighlights);
    }();

    if (QTest::currentTestFailed()) {
        if (lastHighlights.keys() != newHighlights.keys()) {
            qDebug() << "Different offsets in highlights.";
            qDebug() << "Actual offsets:" << lastHighlights.keys();
            qDebug() << "Expected offsets:" << newHighlights.keys();
        } else {
            auto [actual, expected] = std::mismatch(lastHighlights.begin(), lastHighlights.end(),
                                                    newHighlights.begin(), newHighlights.end());
            if (actual != lastHighlights.end() && expected != newHighlights.end()) {

                const auto msg = [](const QString &title, int actualValue, int expectedValue) {
                    return QString("%1 : [Actual %2, Expected %3]")
                            .arg(title)
                            .arg(actualValue)
                            .arg(expectedValue);
                };
                qDebug() << msg("Offset", actual->loc.offset, expected->loc.offset);
                qDebug() << msg("Length", actual->loc.length, expected->loc.length);
                qDebug() << msg("StartLine", actual->loc.startLine, expected->loc.startLine);
                qDebug() << msg("StartColumn", actual->loc.startColumn, expected->loc.startColumn);
                qDebug() << msg("Kind", static_cast<int>(actual->kind),
                                static_cast<int>(expected->kind));
                qDebug() << msg("Modifiers", static_cast<int>(actual->modifiers),
                                static_cast<int>(expected->modifiers));
            }
        }
    }
}

QTEST_MAIN(tst_qmlls_differ)
