// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qmltoolingsettings : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qmltoolingsettings();

private Q_SLOTS:
    void searchConfig_data();
    void searchConfig();
};

tst_qmltoolingsettings::tst_qmltoolingsettings() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

void tst_qmltoolingsettings::searchConfig_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("toolName");
    QTest::addColumn<QQmlToolingSettings::SearchResult>("expectedResult");

    QTest::newRow("sameFolderConfig") << testFile("B/B1/test.qml") << "qmlformat"
                                      << QQmlToolingSettings::SearchResult{
                                             QQmlToolingSettings::SearchResult::ResultType::Found,
                                             testFile("B/B1/.qmlformat.ini")
                                         };
    QTest::newRow("parentFolderConfig") << testFile("B/B2/test.qml") << "qmlformat"
                                        << QQmlToolingSettings::SearchResult{
                                               QQmlToolingSettings::SearchResult::ResultType::Found,
                                               testFile("B/.qmlformat.ini")
                                           };
    QTest::newRow("parentFolderConfigDifferentTool")
            << testFile("B/B2/test.qml") << "qmlls"
            << QQmlToolingSettings::SearchResult{
                   QQmlToolingSettings::SearchResult::ResultType::NotFound, QString()
               };

    QTest::newRow("missingConfig")
            << testFile("A/test.qml") << "qmlformat"
            << QQmlToolingSettings::SearchResult{
                   QQmlToolingSettings::SearchResult::ResultType::NotFound, QString()
               };
}

void tst_qmltoolingsettings::searchConfig()
{
    QFETCH(QString, fileName);
    QFETCH(QString, toolName);
    QFETCH(QQmlToolingSettings::SearchResult, expectedResult);

    QQmlToolingSettings settings(toolName);
    QQmlToolingSettings::SearchResult actualResult = settings.search(fileName);

    QCOMPARE(actualResult.type, expectedResult.type);
    QCOMPARE(actualResult.iniFilePath, expectedResult.iniFilePath);
}

QTEST_MAIN(tst_qmltoolingsettings)
#include "tst_qmltoolingsettings.moc"
