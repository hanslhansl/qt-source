// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../client/testapi.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;
using namespace QtOpenApiCommon;

#define CALL_TEST_FILE_OPERATION(OPERATION, PARAM, EXPECTED_CONTENT, EXPECTED_FILENAME)         \
{                                                                                               \
    bool done = false;                                                                          \
    OPERATION(PARAM, this, [&](const QRestReply &reply, const QOAIHttpFileElement &summary) {   \
        if (!(done = reply.isSuccess())) {                                                      \
            qWarning() << "Error happened while issuing request:" << reply.error()              \
                       << reply.errorString();                                                  \
        }                                                                                       \
        QVERIFY(summary.isSet());                                                               \
        if (!EXPECTED_FILENAME.startsWith("unnamed"_L1))                                        \
            QCOMPARE(summary.requestFilename(), EXPECTED_FILENAME);                             \
        QCOMPARE(summary.loadFromLocalFile(), EXPECTED_CONTENT);                                \
    });                                                                                         \
    QTRY_COMPARE_EQ(done, true);                                                                \
}

namespace QtOpenAPI {

static QProcess serverProcess;
void startServerProcess()
{
    serverProcess.setWorkingDirectory(SERVER_DIR);
    serverProcess.start(SERVER_PATH);
    if (!serverProcess.waitForStarted()) {
        qFatal() << "Couldn't start the server: " << serverProcess.errorString();
        exit(EXIT_FAILURE);
    }
    // give the process some time to properly start up the server
    QThread::currentThread()->msleep(1000);
}

static QJsonValue getObjectValue(const QString &summary, const QString &key)
{
    const QJsonDocument doc = QJsonDocument::fromJson(summary.toUtf8());
    if (!doc.isNull() && doc.isObject()) {
        const QJsonObject obj = doc.object();
        return obj.value(key);
    }
    return QJsonValue();
}

static QByteArray readFile(const QString &filename)
{
    QByteArray fileContent;
    QString filePath = QDir(SERVER_DIR).filePath(filename);
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
        qWarning("Failed to open %s", qPrintable(filename));
    else
        fileContent = file.readAll();

    file.close();
    return fileContent;
}

class Responses : public TestApi {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        if (serverProcess.state() != QProcess::ProcessState::Running)
            startServerProcess();
    }
    void jsonResponse();
    void textResponse();
    void pdfResponse();
    void imageResponse();
    void octetStreamResponse();
    void emptyResponseBody();
    void contentDispositionChecks_data();
    void contentDispositionChecks();
    void fileConflictPolicyOverwrite();
    void fileConflictPolicyRename();
    void fileConflictPolicyError();
    void cleanupTestCase();
};

void Responses::jsonResponse()
{
    bool done = false;

    applicationJsonStringResponse(this, [&](const QRestReply &reply, const QString &summary) {
        if (!(done = reply.isSuccess())) {
                qWarning() << "Error happened while issuing request : " << reply.error()
                           << reply.errorString();
        }
        QCOMPARE(getObjectValue(summary, "status"_L1).toString(), "OK"_L1);
    });
    QTRY_COMPARE_EQ(done, true);

    User user1, user2;
    user1.setName("user1");
    user1.setId(1);
    user2.setName("user2");
    user2.setId(2);

    done = false;
    applicationJsonArrayResponse(this, [&](const QRestReply &reply,
                                           const QList<User> &summary) {
        if (!(done = reply.isSuccess())) {
            qWarning() << "Error happened while issuing request : " << reply.error()
                       << reply.errorString();
        }
        QCOMPARE(summary[0].getName(), user1.getName());
        QCOMPARE(summary[0].getId(), user1.getId());
        QCOMPARE(summary[1].getName(), user2.getName());
        QCOMPARE(summary[1].getId(), user2.getId());
    });
    QTRY_COMPARE_EQ(done, true);

    done = false;
    applicationJsonMapResponse(this, [&](const QRestReply &reply,
                                           const QMap<QString, User> &summary) {
        if (!(done = reply.isSuccess())) {
            qWarning() << "Error happened while issuing request : " << reply.error()
                       << reply.errorString();
        }
        QVERIFY(summary.size() == 2);
        QCOMPARE(summary["first"].getName(), user1.getName());
        QCOMPARE(summary["first"].getId(), user1.getId());
        QCOMPARE(summary["second"].getName(), user2.getName());
        QCOMPARE(summary["second"].getId(), user2.getId());
    });
    QTRY_COMPARE_EQ(done, true);

    done = false;
    applicationJsonEncodedObjectResponse(
        this, [&](const QRestReply &reply,
                  const ApplicationJsonEncodedObjectResponse_200_response &summary) {
        if (!(done = reply.isSuccess())) {
            qWarning() << "Error happened while issuing request : " << reply.error()
                       << reply.errorString();
        }
        QCOMPARE(summary.getStatus(), "OK"_L1);
        QCOMPARE(summary.getValue(), 22);
    });
    QTRY_COMPARE_EQ(done, true);
}

void Responses::textResponse()
{
    bool done = false;

    // Plain text response
    textPlainStringResponse(this, [&](const QRestReply &reply, const QString &summary) {
        if (!(done = reply.isSuccess())) {
            qWarning() << "Error happened while issuing request : " << reply.error()
                       << reply.errorString();
        }
        QCOMPARE(summary, "Hello plain text"_L1);
    });
    QTRY_COMPARE_EQ(done, true);
}

void Responses::pdfResponse()
{
    QByteArray expectedPdfContent = readFile("test.pdf"_L1);
    const QString dirPath = workingDirectory() + QDir::separator();
    // clean up
    auto cleanup = qScopeGuard([&] {
        QFile::remove(dirPath + "compressed_example1.pdf"_L1);
        QFile::remove(dirPath + "unnamed"_L1);
        QFile::remove(dirPath + "example1.pdf"_L1);
    });
    Q_UNUSED(cleanup);
    CALL_TEST_FILE_OPERATION(applicationPdfInlineResponse, "test.pdf"_L1, expectedPdfContent,
                            "unnamed"_L1);
    CALL_TEST_FILE_OPERATION(applicationPdfSaveResponse, "test.pdf"_L1, expectedPdfContent,
                            "example1.pdf"_L1);

    CALL_TEST_FILE_OPERATION(applicationEncodedPdfSaveResponse, "test.pdf"_L1, expectedPdfContent,
                            "compressed_example1.pdf"_L1);
}

void Responses::imageResponse()
{
    QByteArray expectedImage = readFile("testImage.jpg"_L1);
    const QString dirPath = workingDirectory() + QDir::separator();
    // clean up
    auto cleanup = qScopeGuard([&] {
        QFile::remove(dirPath + "testImage.jpg"_L1);
        QFile::remove(dirPath + "example2.jpg"_L1);
        QFile::remove(dirPath + "unnamed"_L1);
        QFile::remove(dirPath + "example3.png"_L1);
    });
    Q_UNUSED(cleanup);
    CALL_TEST_FILE_OPERATION(inlineImageResponse, "jpegImage"_L1, expectedImage, "unnamed"_L1);
    CALL_TEST_FILE_OPERATION(saveImageResponse, "jpegImage"_L1, expectedImage, "example2.jpg"_L1);

    expectedImage = readFile("testImage.png"_L1);
    CALL_TEST_FILE_OPERATION(inlineImageResponse, "pngImage"_L1, expectedImage, "unnamed"_L1);
    CALL_TEST_FILE_OPERATION(saveImageResponse, "pngImage"_L1, expectedImage, "example3.png"_L1);
}

void Responses::octetStreamResponse()
{
    QByteArray expectedBinData = readFile("test.bin"_L1);;
    const QString dirPath = workingDirectory() + QDir::separator();
    // clean up
    auto cleanup = qScopeGuard([&] {
        QFile::remove(dirPath + "example.bin"_L1);
    });
    Q_UNUSED(cleanup);
    CALL_TEST_FILE_OPERATION(applicationOctetStreamResponse, "test.bin"_L1, expectedBinData,
                             "example.bin"_L1);
}

void Responses::emptyResponseBody()
{
    bool done = false;
    emptyResponse(this, [&](const QRestReply &reply) {
        if (!(done = reply.isSuccess())) {
            qWarning() << "Error happened while issuing request : " << reply.error()
                       << reply.errorString();
        }
        QCOMPARE(reply.httpStatus(), 204);
    });
    QTRY_COMPARE_EQ(done, true);
}

void Responses::contentDispositionChecks_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("extFilename");
    QTest::addColumn<QString>("expectedFilename");

    QTest::newRow("unix_path")
            << u"/usr/bin/some_dir/../some_binary.bin"_s
            << QString()
            << u"some_binary.bin"_s;
    QTest::newRow("windows_path")
            << u"\"C:\\Windows\\System32\\some_dir\\..\\system_file.bin\""_s
            << QString()
            << u"system_file.bin"_s;

    QTest::newRow(".") << u"."_s << QString() << u"_"_s;
    QTest::newRow("..") << u".."_s << QString() << u"__"_s;
    QTest::newRow("filename_with_many_dots")
            << u".some..file.name.ext"_s
            << QString()
            << u".some__file.name.ext"_s;
    QTest::newRow("forbidden_chars")
            << u"\"file~:*?\"<>|.txt\""_s
            << QString()
            << u"file________.txt"_s;

    // we cannot send control chars as-is, so use the extFilename and
    // percent-encode them. They should be decoded when parsing response, and
    // then substituted with "_".
    QByteArray controlChars;
    for (char c = 0x00; c < 0x20; ++c)
        controlChars.append(c);
    controlChars.append(0x7f);

    const QString extFilenameControlChars =
            u"ISO-8859-1'en-US'"_s + QString::fromLatin1(controlChars.toPercentEncoding());
    QTest::newRow("control_chars")
            << u"fallback"_s << extFilenameControlChars << QString(33, QLatin1Char('_'));

    // use some characters that can be represented in Latin1 and UTF-* encodings
    const QString nonAsciiFilename = u"fileÄÖßæï"_s;
    const QByteArray latin1PercentEncoded = nonAsciiFilename.toLatin1().toPercentEncoding();
    QTest::newRow("ext_latin1")
            << u"fallback"_s
            << u"ISO-8859-1''"_s + QString::fromLatin1(latin1PercentEncoded)
            << nonAsciiFilename;

    // after toPercentEncoding() it is ASCII-only
    const QByteArray utf8PercentEncoded = nonAsciiFilename.toUtf8().toPercentEncoding();
    QTest::newRow("ext_utf-8")
            << u"fallback"_s
            << u"UTF-8''"_s + QString::fromLatin1(utf8PercentEncoded)
            << nonAsciiFilename;

    const QByteArray utf16PercentEncoded =
            QByteArray(reinterpret_cast<const char *>(nonAsciiFilename.utf16()),
                       nonAsciiFilename.size() * 2).toPercentEncoding();
    QTest::newRow("ext_utf-16")
            << u"fallback"_s
            << u"UTF-16''"_s + QString::fromLatin1(utf16PercentEncoded)
            << nonAsciiFilename;

    const std::u32string u32str = nonAsciiFilename.toStdU32String();
    const QByteArray utf32PercentEncoded =
            QByteArray(reinterpret_cast<const char *>(u32str.data()),
                       u32str.size() * 4).toPercentEncoding();
    QTest::newRow("ext_utf-32")
            << u"fallback"_s
            << u"\"UTF-32''%1\""_s.arg(QString::fromLatin1(utf32PercentEncoded))
            << nonAsciiFilename;

    QTest::newRow("ext_no_lang_fallback")
            << u"fallback"_s
            << u"utf-8'data"_s /* misses a second ' */
            << u"fallback"_s;

    QTest::newRow("ext_unknown_encoding_fallback")
            << u"fallback"_s
            << u"win-1252'en-US'data"_s
            << u"fallback"_s;

    QTest::newRow("ext_utf-16_invalid_len_fallback")
            << u"fallback"_s
            << u"utf-16'en-US'abc"_s /* 3 bytes are not a valid utf-16 string */
            << u"fallback"_s;

    QTest::newRow("ext_utf-32_invalid_len_fallback")
            << u"fallback"_s
            << u"utf-32'en-US'abc"_s /* 3 bytes are not a valid utf-32 string */
            << u"fallback"_s;
}

void Responses::contentDispositionChecks()
{
    QFETCH(const QString, filename);
    QFETCH(const QString, extFilename);
    QFETCH(const QString, expectedFilename);

    const QString dirPath = workingDirectory() + QDir::separator();
    // clean up
    auto cleanup = qScopeGuard([&] {
        QFile::remove(dirPath + expectedFilename);
    });
    Q_UNUSED(cleanup);
    bool done = false;
    contentDispositionCheck(
        OptionalParameter<QString>{filename}, OptionalParameter<QString>{extFilename}, this,
        [&](const QRestReply &reply, const QOAIHttpFileElement &summary) {
            if (!(done = reply.isSuccess())) {
                qWarning() << "Error happened while issuing request:"
                << reply.error() << reply.errorString();
            }
            QCOMPARE(reply.httpStatus(), 200);
            QCOMPARE(summary.filename(), dirPath + expectedFilename);
        });
    QTRY_COMPARE_EQ(done, true);
}

void Responses::fileConflictPolicyOverwrite()
{
    // Default policy: Overwrite. Downloading the same file twice should
    // overwrite the first download.
    setFileConflictPolicy(FileConflictPolicy::Overwrite);
    const QByteArray expectedContent = readFile("test.pdf"_L1);
    const QString dirPath = workingDirectory() + QDir::separator();
    const QString expectedFile = dirPath + u"example1.pdf"_s;
    // Clean up
    auto cleanup = qScopeGuard([&] {
        QFile::remove(expectedFile);
        setFileConflictPolicy(FileConflictPolicy::Rename);
    });
    Q_UNUSED(cleanup);

    // First download
    CALL_TEST_FILE_OPERATION(applicationPdfSaveResponse, "test.pdf"_L1,
                             expectedContent, "example1.pdf"_L1);
    QVERIFY(QFile::exists(expectedFile));

    // Second download — should overwrite, same filename
    CALL_TEST_FILE_OPERATION(applicationPdfSaveResponse, "test.pdf"_L1,
                             expectedContent, "example1.pdf"_L1);
    QVERIFY(QFile::exists(expectedFile));
}

void Responses::fileConflictPolicyRename()
{
    // Rename policy: second download of the same filename should create
    // a new file with a numeric suffix.
    setFileConflictPolicy(FileConflictPolicy::Rename);
    const QByteArray expectedContent = readFile("test.pdf"_L1);
    const QString dirPath = workingDirectory() + QDir::separator();
    const QString originalFile = dirPath + u"example1.pdf"_s;
    const QString renamedFile = dirPath + u"example1(1).pdf"_s;

    const QString originalArchFile("file.tar.gz"_L1);
    const QString renamedArchFile("file(1).tar.gz"_L1);
    // Clean up
    auto cleanup = qScopeGuard([&] {
        QFile::remove(originalFile);
        QFile::remove(renamedFile);
        QFile::remove(dirPath + originalArchFile);
        QFile::remove(dirPath + renamedArchFile);
    });
    Q_UNUSED(cleanup);

    // First download — creates example1.pdf
    CALL_TEST_FILE_OPERATION(applicationPdfSaveResponse, "test.pdf"_L1,
                             expectedContent, "example1.pdf"_L1);
    QVERIFY(QFile::exists(originalFile));
    QVERIFY(!QFile::exists(renamedFile));

    // Second download — should create example1 (1).pdf
    CALL_TEST_FILE_OPERATION(applicationPdfSaveResponse, "test.pdf"_L1,
                             expectedContent, "example1(1).pdf"_L1);
    QVERIFY(QFile::exists(originalFile));
    QVERIFY(QFile::exists(renamedFile));

    // test renaming a file with multiple extension - '.tar.gz'
    bool done = false;
    // After this call file.tar.gz is created
    contentDispositionCheck(
        OptionalParameter<QString>{originalArchFile}, OptionalParameter<QString>{}, this,
        [&](const QRestReply &reply, const QOAIHttpFileElement &summary) {
            if (!(done = reply.isSuccess())) {
                qWarning() << "Error happened while issuing request:"
                           << reply.error() << reply.errorString();
            }
            QCOMPARE(reply.httpStatus(), 200);
            QCOMPARE(summary.filename(), dirPath + originalArchFile);
        });
    QTRY_COMPARE_EQ(done, true);
    QVERIFY(QFile::exists(dirPath + originalArchFile));

    done = false;
    // After this call file(1).tar.gz is created
    contentDispositionCheck(
        OptionalParameter<QString>{renamedArchFile}, OptionalParameter<QString>{}, this,
        [&](const QRestReply &reply, const QOAIHttpFileElement &summary) {
            if (!(done = reply.isSuccess())) {
                qWarning() << "Error happened while issuing request:"
                           << reply.error() << reply.errorString();
            }
            QCOMPARE(reply.httpStatus(), 200);
            QCOMPARE(summary.filename(), dirPath + renamedArchFile);
        });
    QTRY_COMPARE_EQ(done, true);
    QVERIFY(QFile::exists(dirPath + renamedArchFile));
}

void Responses::fileConflictPolicyError()
{
    // Error policy: if the file already exists, the download should be skipped
    // and the existing file should remain untouched.
    setFileConflictPolicy(FileConflictPolicy::Error);
    const QString dirPath = workingDirectory() + QDir::separator();
    const QString targetFile = dirPath + u"example1.pdf"_s;
    auto cleanup = qScopeGuard([&] {
        // Clean up
        QFile::remove(targetFile);
        setFileConflictPolicy(FileConflictPolicy::Rename);
    });
    Q_UNUSED(cleanup);

    // Pre-create a file with known content
    const QByteArray originalContent = "original content that must not be overwritten";
    {
        QFile f(targetFile);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(originalContent);
    }
    QVERIFY(QFile::exists(targetFile));

    // Attempt download — should skip file writing due to conflict policy
    bool done = false;
    applicationPdfSaveResponse(u"test.pdf"_s, this,
        [&](const QRestReply &reply, const QOAIHttpFileElement &summary) {
            done = reply.isSuccess();
            // The HTTP request succeeds, but no file element is produced
            QVERIFY(!summary.isSet());
        });
    QTRY_COMPARE_EQ(done, true);

    // Verify original file content is preserved
    QFile f(targetFile);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), originalContent);
}

void Responses::cleanupTestCase()
{
    if (serverProcess.state() == QProcess::ProcessState::Running) {
        serverProcess.kill();
        serverProcess.waitForFinished();
    }
}

} // namespace QtOpenAPI

QTEST_MAIN(QtOpenAPI::Responses)
#include "tst_responses.moc"
