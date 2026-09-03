// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtopenapigen_common.h"

using namespace Qt::StringLiterals;
namespace QtOpenAPI {

#ifndef CMAKE_GENERATOR_TESTS_CLIENT
#  define CMAKE_GENERATOR_TESTS_CLIENT
#endif
constexpr QLatin1StringView CMakeGeneratorTestsClient(CMAKE_GENERATOR_TESTS_CLIENT);

class QtOpenAPIGenerator : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void checkVersionIsNotEmpty();
    void cmakeGeneratedLibraries_data();
    void cmakeGeneratedLibraries();

private:
    QString m_expectedResultPath;
    QString m_cmakeExpectedResultPath;
    QString m_cmakeGeneratedPath;
};

void QtOpenAPIGenerator::initTestCase()
{
    m_expectedResultPath = QFINDTESTDATA("data");
    m_cmakeGeneratedPath = BinaryDir + '/'_L1 + CMakeGeneratedDir;
    m_cmakeExpectedResultPath = m_expectedResultPath + '/'_L1 + CMakeGeneratedDir;
}

void QtOpenAPIGenerator::checkVersionIsNotEmpty()
{
    QCOMPARE_NE(QT_OPENAPI_GENERATOR_VERSION, "");
}

void QtOpenAPIGenerator::cmakeGeneratedLibraries_data()
{
    QTest::addColumn<QString>("testFolder");
    QTest::addColumn<QString>("filePath");

    const QStringList tests = QString(CMakeGeneratorTestsClient).split(','_L1, Qt::SkipEmptyParts);
    for (const auto &testName : tests) {
        QDir testDir(m_cmakeExpectedResultPath + '/'_L1 + testName);
        const auto testFiles = scanDirectoryRecursively(testDir);
        for (const auto &testFile : testFiles) {
            auto relativePath = testDir.relativeFilePath(testFile.absoluteFilePath());
            QTest::addRow("%s: %s", testName.toUtf8().constData(),
                          relativePath.toUtf8().constData())
                << testName << relativePath;
        }
    }
}

void QtOpenAPIGenerator::cmakeGeneratedLibraries()
{
    QFETCH(QString, testFolder);
    QFETCH(QString, filePath);
    compareTwoFiles(m_cmakeExpectedResultPath + '/'_L1 + testFolder + '/'_L1 + filePath,
                    m_cmakeGeneratedPath + '/'_L1 + testFolder + '/'_L1 + filePath);
}

} // QtOpenAPI

QTEST_MAIN(QtOpenAPI::QtOpenAPIGenerator)
#include "tst_openapigen.moc"
