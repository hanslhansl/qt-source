// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "defaultapi.h"
#include "myenum.h"

#include <QtCore/qlist.h>
#include <QtCore/qobject.h>
#include <QtCore/QMetaEnum>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkrequestfactory.h>
#include <QtNetwork/qrestaccessmanager.h>
#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;
using namespace QtOpenAPI;

class LoggingNetworkAccessManager : public QNetworkAccessManager
{
public:
    LoggingNetworkAccessManager(QObject *parent = nullptr)
        : QNetworkAccessManager(parent)
    {}
    ~LoggingNetworkAccessManager() override
    {}

protected:
    QNetworkReply *createRequest(QNetworkAccessManager::Operation op,
                                 const QNetworkRequest &originalReq,
                                 QIODevice *outgoingData = nullptr) override;

public:
    QString m_operationPath;
    QHttpHeaders m_headers;
};

QNetworkReply *LoggingNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                          const QNetworkRequest &originalReq,
                                                          QIODevice *outgoingData)
{
    const QUrl fullUrl = originalReq.url();
    m_headers = originalReq.headers() ;
    m_operationPath = fullUrl.toString();

    return QNetworkAccessManager::createRequest(op, originalReq, outgoingData);
}

class tst_UnsafeCharactersEscape : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_manager = new LoggingNetworkAccessManager(this);
        m_api.setRestAccessManager(new QRestAccessManager(m_manager, this));
    }

    // server URL: verifies the injected server URL is stored escaped and
    // the generated code compiles.
    void serverUrlCheck();

    // path: verifies the injected path key is stored escaped
    // and the generated code compiles.
    void pathInjection();

    // mediaType: verifies the mediaType sanitization code path
    // is exercised and the generated code compiles.
    void mediaTypeEscaping();

    void testEnumObject();

private:
    LoggingNetworkAccessManager *m_manager;
    DefaultApi m_api;
};

void tst_UnsafeCharactersEscape::serverUrlCheck()
{
    const QString testStr("http://legit.example.com\"_s, u\"ok\"_s, "
        "QMap<QString, QOAIServerVariable>())); volatile int _r1_injected = 42; "
        "Q_UNUSED(_r1_injected); "
        "defaultConf.append(QOAIServerConfiguration(u\"http://fallback.example.com"_L1);

    const QList<QOAIServerConfiguration> initialConfig
        = m_api.serverConfigurations("healthCheck"_L1);
    for (qsizetype index = 0; index < initialConfig.size(); index++) {
        QCOMPARE(testStr, initialConfig.at(index).urlTemplate());
    }
}

// ---------------------------------------------------------------------------
// path escaping
// ---------------------------------------------------------------------------
void tst_UnsafeCharactersEscape::pathInjection()
{
    QVERIFY(m_api.operations().contains("getItems"_L1));
    bool done = true;
    m_api.getItems(this, [&](const QRestReply &reply, const QString &) {
        done = reply.isSuccess();
    });
    QTRY_COMPARE_EQ(done, false);
    QCOMPARE(m_manager->m_operationPath,
             "http:////fallback.example.com/items%22_s; volatile "
             "int _gap1_injected = 1; Q_UNUSED(_gap1_injected); QString _unused = u%22"_L1);
}

// ---------------------------------------------------------------------------
// mediaType escaping
// ---------------------------------------------------------------------------
void tst_UnsafeCharactersEscape::mediaTypeEscaping()
{
    QVERIFY(m_api.operations().contains("uploadFile"_L1));
    bool done = true;
    // Plain text response
    m_api.uploadFile("Plain text"_L1, this, [&](const QRestReply &reply, const QString &) {
        done = reply.isSuccess();
    });
    QTRY_COMPARE_EQ(done, false);
    QCOMPARE(m_manager->m_headers.value("content-type"_L1),
             "application/json\\\"_L1); volatile int _gap2_injected = 1; "
             "Q_UNUSED(_gap2_injected); auto _unused = QLatin1StringView(\\\""_L1);
}

// ---------------------------------------------------------------------------
// enumobject escaping
// ---------------------------------------------------------------------------
void tst_UnsafeCharactersEscape::testEnumObject()
{
    // example how enum's content are being handled by the generator
    QVERIFY(MyEnum::eMyEnum::STATUS2__S_VOLATILE_INT__STATUS_INJECTED_1_Q_UNUSED__STATUS_INJECTED_QSTRING__UNUSED_U_);
}

QTEST_MAIN(tst_UnsafeCharactersEscape)
#include "tst_safecharacters.moc"
