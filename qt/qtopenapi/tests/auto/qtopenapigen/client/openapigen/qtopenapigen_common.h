// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTOPENAPIGEN_COMMON_H
#define QTOPENAPIGEN_COMMON_H

#include <QtCore/qobject.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qdiriterator.h>
#include <QtTest/qtest.h>

namespace {
#ifndef CMAKE_GENERATED_DIR
#  error CMAKE_GENERATED_DIR definition must be set
#endif
constexpr QLatin1StringView CMakeGeneratedDir(CMAKE_GENERATED_DIR);
constexpr QLatin1StringView CmdLineGeneratedDir("cmd_line_generated");

#  ifndef BINARY_DIR
#    error BINARY_DIR definition must be set
#  endif
constexpr QLatin1StringView BinaryDir(BINARY_DIR);

QByteArray hash(const QByteArray &fileData)
{
    return QCryptographicHash::hash(fileData, QCryptographicHash::Sha1);
}

// Return size diff and first NOT equal line;
QByteArray doCompare(const QByteArrayList &actual, const QByteArrayList &expected)
{
    QByteArray ba;
    if (actual.size() != expected.size()) {
        ba.append(QString("Length count different: actual: %1, expected: %2")
                      .arg(actual.size())
                      .arg(expected.size())
                      .toUtf8());
    }

    for (int i = 0, n = expected.size(); i != n; ++i) {
        const QByteArray expectedLine = expected.at(i);
        // The version depends on the local setup, so skip it
        if (expectedLine.startsWith("#define OPENAPI_GENERATOR_VERSION"))
            continue;
        if (expectedLine != actual.at(i)) {
            ba.append("\n<<<<<< ACTUAL\n" + actual.at(i) + "\n======\n" + expectedLine
                      + "\n>>>>>> EXPECTED\n");
            break;
        }
    }
    return ba;
}

QByteArray msgCannotReadFile(const QFile &file)
{
    const QString result = QLatin1StringView("Could not read file: ")
    + QDir::toNativeSeparators(file.fileName()) + QLatin1StringView(": ") + file.errorString();
    return result.toLocal8Bit();
}

QByteArrayList splitToLines(const QByteArray &data)
{
    return data.split('\n');
}

QFileInfoList scanDirectoryRecursively(const QDir &dir)
{
    QFileInfoList result;
    QDirIterator it(dir.path(), QStringList() << "*.cpp" << "*.h", QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        result.append(it.fileInfo());
    }
    std::sort(result.begin(), result.end(), [&dir](const QFileInfo &lhs, const QFileInfo &rhs) {
        return dir.relativeFilePath(lhs.absoluteFilePath())
        < dir.relativeFilePath(rhs.absoluteFilePath());
    });
    return result;
}

void compareTwoFiles(const QString &expectedFileName,
                     const QString &actualFileName)
{
    QFile expectedResultFile(expectedFileName);
    QFile generatedFile(actualFileName);

    QVERIFY2(expectedResultFile.exists(), qPrintable(expectedResultFile.fileName()));
    QVERIFY2(generatedFile.exists(), qPrintable(expectedResultFile.fileName()));

    QVERIFY2(expectedResultFile.open(QIODevice::ReadOnly | QIODevice::Text),
             msgCannotReadFile(expectedResultFile).constData());
    QVERIFY2(generatedFile.open(QIODevice::ReadOnly | QIODevice::Text),
             msgCannotReadFile(generatedFile).constData());

    QByteArray expectedData = expectedResultFile.readAll();
    QByteArray generatedData = generatedFile.readAll();

    expectedResultFile.close();
    generatedFile.close();

    if (hash(expectedData).toHex() != hash(generatedData).toHex()) {
        const QByteArray diff = doCompare(splitToLines(generatedData),
                                          splitToLines(expectedData));
        // The difference is only in the generator version, simply return early
        // here, because all the checks will fail, but that's expected.
        if (expectedFileName.contains(QLatin1StringView("qoaicommonglobal.h"))
            && diff.isEmpty()) {
            return;
        }
        QCOMPARE_GT(diff.size(), 0);
        QFAIL(qPrintable(diff));
    }
    QCOMPARE_EQ(generatedData, expectedData);
}
}

#endif // QTOPENAPIGEN_COMMON_H
