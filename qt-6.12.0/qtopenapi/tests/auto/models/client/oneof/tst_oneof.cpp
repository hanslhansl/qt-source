// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qoaicommonglobal.h"

#include "../basicSchemaAlternatives/client/bankapi.h"
#include "../basicSchemaAlternatives/client/dataapi.h"
#include "../basicSchemaAlternatives/client/defaultapi.h"
#include "../basicSchemaAlternatives/client/dummytestvalue.h"
#include "../basicSchemaAlternatives/client/farmapi.h"
#include "../basicSchemaAlternatives/client/root.h"
#include "../basicSchemaAlternatives/client/rootmap.h"
#include "../basicSchemaAlternatives/client/rootmapmodels.h"
#include "../basicSchemaAlternatives/client/storeapi.h"

#include <QtCore/qbuffer.h>
#include <QtCore/qobject.h>
#include <QtNetwork/qnetworkrequestfactory.h>
#include <QtNetwork/qrestaccessmanager.h>
#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;
using namespace QtOpenApiCommon;

static QString mapAsString(const QMap<QString, QJsonValue> &map)
{
    QJsonObject obj;
    for (const auto &[key, val] : map.asKeyValueRange())
        obj.insert(key, val);
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

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
    QByteArray m_content;
};

QNetworkReply *LoggingNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                          const QNetworkRequest &originalReq,
                                                          QIODevice *outgoingData)
{
    // The QNAM::put/post/sendCustomRequest operations that take QByteArray
    // as a data parameter use QBuffer to wrap the byte array.
    // It is later passed as a QIODevice* to this method.
    // We cast the QIODevice to QBuffer, and store its content to intercept
    // the transferred data.
    QBuffer *buffer = qobject_cast<QBuffer*>(outgoingData);
    m_content = buffer ? buffer->data() : QByteArray();

    return QNetworkAccessManager::createRequest(op, originalReq, outgoingData);
}

class OneOfTest : public QObject {
    Q_OBJECT

    enum class BankApiPatchMode {
        UsePatchPaymentRequest,
        UseString,
        UseBool,
        UseDouble
    };

    enum class BankApiPostMode {
        UsePostPaymentRequest,
        UseCreditCard,
        UsePaypal
    };

    enum class StoreApiMode {
        UseCat,
        UseDog,
        UsePet
    };

    enum class FarmApiMode {
        UseBunny,
        UseDog,
        UseDuck,
        UsePostPet
    };

private:
    void generateAlternativeSchemasTestData();

private Q_SLOTS:
    void testAccountJsonConversionMethods_data();
    void testAccountJsonConversionMethods();
    void testAlternativeSchemasFunctions_data();
    void testAlternativeSchemasFunctions();
    void testAlternativeSchemasOptional_data();
    void testAlternativeSchemasOptional();
    void testAlternativeSchemasRequired_data();
    void testAlternativeSchemasRequired();
    void testClientValueJsonConversionMethods_data();
    void testClientValueJsonConversionMethods();
    void testCustomerJsonConversionMethods_data();
    void testCustomerJsonConversionMethods();
    void testInlineSchemasJsonConversionMethods_data();
    void testInlineSchemasJsonConversionMethods();
    void testInlineSchemasPatch_data();
    void testInlineSchemasPatch();
    void testInlineSchemasPost_data();
    void testInlineSchemasPost();
    void testOneOfPrimitiveJsonConversionMethods_data();
    void testOneOfPrimitiveJsonConversionMethods();
    void testPolymorphedRequestBody_data();
    void testPolymorphedRequestBody();
    void testPostAccountRequestJsonConversionMethods_data();
    void testPostAccountRequestJsonConversionMethods();
    void testPostFarmPetRequestJsonConversionMethods_data();
    void testPostFarmPetRequestJsonConversionMethods();
    void testDummyTestValue();
    void testRootJsonConversionMethods_data();
    void testRootJsonConversionMethods();
    void testRootMapJsonConversionMethods_data();
    void testRootMapJsonConversionMethods();
    void testRootMapModelsJsonConversionMethods_data();
    void testRootMapModelsJsonConversionMethods();
};

void OneOfTest::testAccountJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::Account>("account");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<QString>("oneOf0ExpectedJson");
    QTest::addColumn<QString>("oneOf1ExpectedJson");
    QTest::addColumn<bool>("isOneOfAccountOneOfValid");
    QTest::addColumn<bool>("isOneOfAccountOneOfSet");
    QTest::addColumn<bool>("isOneOfAccountOneOf1Valid");
    QTest::addColumn<bool>("isOneOfAccountOneOf1Set");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    // Account has two oneOf alternatives:
    // oneOf0: Account_oneOf (name: string optional, contractAvailable: bool optional) =>
    // no required
    // oneOf1: Account_oneOf_1 (firstname: string optional, lastname: string optional,
    // additionalProperties) - no required
    // Both have no required fields => both are always "valid"
    // This creates ambiguity with empty JSON since both alternatives are valid!

    SchemasModelsOneOf::Account account;
    QTest::newRow("Empty Account object")
        // account                        // expectedJson
        << account                        << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set a valid Account_oneOf via setter
    SchemasModelsOneOf::Account_oneOf acctOneOf0;
    acctOneOf0.setName("Test Account"_L1);
    acctOneOf0.setContractAvailable(true);
    account.setOneOfAccount_oneOf(acctOneOf0);
    QTest::newRow("Set a valid Account_oneOf object via setter")
        // account                        // expectedJson
        << account                        << acctOneOf0.asJson()
        // expectedJsonValue
        << acctOneOf0.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << acctOneOf0.asJson()            << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a valid Account_oneOf_1 via setter
    SchemasModelsOneOf::Account_oneOf_1 acctOneOf1;
    acctOneOf1.setFirstname("John"_L1);
    acctOneOf1.setLastname("Doe"_L1);
    account.setOneOfAccount_oneOf_1(acctOneOf1);
    QTest::newRow("Set a valid Account_oneOf_1 object via setter")
        // account                        // expectedJson
        << account                        << acctOneOf1.asJson()
        // expectedJsonValue
        << acctOneOf1.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << acctOneOf1.asJson()
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    const SchemasModelsOneOf::Account_oneOf emptyAcctOneOf0;
    account.setOneOfAccount_oneOf(emptyAcctOneOf0);
    QTest::newRow("Set an empty Account_oneOf object via setter")
        // account                        // expectedJson
        << account                        << emptyAcctOneOf0.asJson()
        // expectedJsonValue
        << emptyAcctOneOf0.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set an empty Account_oneOf_1 object. No required fields, so it's VALID.
    const SchemasModelsOneOf::Account_oneOf_1 emptyAcctOneOf1;
    account.setOneOfAccount_oneOf_1(emptyAcctOneOf1);
    QTest::newRow("Set an empty Account_oneOf_1 object via setter")
        // account                        // expectedJson
        << account                        << emptyAcctOneOf1.asJson()
        // expectedJsonValue
        << emptyAcctOneOf1.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with EMPTY data => ambiguity, both are valid
    // Resolution: first valid alternative wins (oneOf0)
    account.fromJsonValue(emptyAcctOneOf0.asJsonValue());
    QTest::newRow("Set an empty Account_oneOf via ::fromJsonValue()")
        // account                        // expectedJson
        << account                        << emptyAcctOneOf0.asJson()
        // expectedJsonValue
        << emptyAcctOneOf0.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // !!! NOTE: ambiguity - both coule be valid, but oneOf0 wins
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with EMPTY data => same ambiguity
    account.fromJson("{}"_L1);
    QTest::newRow("Set an empty JSON {} via ::fromJson()")
        // account                        // expectedJson
        << account                        << QString("{}"_L1)
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // !!! NOTE: ambiguity - both could be valid, oneOf0 wins
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with Account_oneOf specific field (name)
    const QString nameOnly("{\"name\":\"Acme Corp\"}"_L1);
    account.fromJson(nameOnly);
    QTest::newRow("Set Account_oneOf with name via ::fromJson()")
        // account                        // expectedJson
        << account                        << nameOnly
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << nameOnly                       << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with Account_oneOf specific field (contractAvailable)
    const QString contractOnly("{\"contractAvailable\":false}"_L1);
    account.fromJson(contractOnly);
    QTest::newRow("Set Account_oneOf with contractAvailable via ::fromJson()")
        // account                        // expectedJson
        << account                        << contractOnly
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << contractOnly                   << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with Account_oneOf_1 specific field (firstname)
    const QString firstnameOnly("{\"firstname\":\"Alice\"}"_L1);
    account.fromJson(firstnameOnly);
    QTest::newRow("Set Account_oneOf_1 with firstname via ::fromJson()")
        // account                        // expectedJson
        << account                        << firstnameOnly
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << firstnameOnly
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson with Account_oneOf_1 both fields
    const QString fullName("{\"firstname\":\"Bob\",\"lastname\":\"Smith\"}"_L1);
    account.fromJson(fullName);
    QTest::newRow("Set Account_oneOf_1 with both names via ::fromJson()")
        // account                        // expectedJson
        << account                        << fullName
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << fullName
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson with Account_oneOf both fields
    const QString fullAcct("{\"contractAvailable\":true,\"name\":\"Premium\"}"_L1);
    account.fromJson(fullAcct);
    QTest::newRow("Set Account_oneOf with both fields via ::fromJson()")
        // account                        // expectedJson
        << account                        << fullAcct
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << fullAcct                       << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with valid Account_oneOf object
    account.fromJsonValue(acctOneOf0.asJsonValue());
    QTest::newRow("Set a valid Account_oneOf via ::fromJsonValue()")
        // account                        // expectedJson
        << account                        << acctOneOf0.asJson()
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << acctOneOf0.asJson()            << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with valid Account_oneOf_1 object
    account.fromJsonValue(acctOneOf1.asJsonValue());
    QTest::newRow("Set a valid Account_oneOf_1 via ::fromJsonValue()")
        // account                        // expectedJson
        << account                        << acctOneOf1.asJson()
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << acctOneOf1.asJson()
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // invalid json! => the model is RESET to INITIAL state
    account.fromJson("{invalid json}"_L1);
    QTest::newRow("Set an invalid json via ::fromJson()")
        // account                        // expectedJson
        << account                        << QString("{}"_L1)
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-object JSON (array) => reset
    account.fromJson("[]"_L1);
    QTest::newRow("Account: set an unexpected array via ::fromJson()")
        // account                        // expectedJson
        << account                        << QString("{}"_L1)
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-object JSON (string) => reset
    account.fromJson("\"string\""_L1);
    QTest::newRow("Set an unexpected string via ::fromJson()")
        // account                        // expectedJson
        << account                        << QString("{}"_L1)
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Invalid field types in Account_oneOf: name should be string
    account.fromJson("{\"name\":123,\"contractAvailable\":true}"_L1);
    QTest::newRow("Invalid field 'name=123' in Account_oneOf via ::fromJson()")
        // account                        // expectedJson
        << account                        << QString("{\"contractAvailable\":true}"_L1)
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{\"contractAvailable\":true}"_L1) << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Invalid field types in Account_oneOf: contractAvailable should be bool
    account.fromJson("{\"name\":\"ok\",\"contractAvailable\":\"yes\"}"_L1);
    QTest::newRow("Invalid field 'contractAvailable=yes' in Account_oneOf via ::fromJson()")
        // account                        // expectedJson
        << account                        << QString("{\"name\":\"ok\"}"_L1)
        // expectedJsonValue
        << account.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{\"name\":\"ok\"}"_L1) << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Account_oneOf with only name set via setter
    SchemasModelsOneOf::Account_oneOf nameOnlyAcct;
    nameOnlyAcct.setName("PartialName"_L1);
    account.setOneOfAccount_oneOf(nameOnlyAcct);
    QTest::newRow("Set Account_oneOf with only name via setter")
        // account                        // expectedJson
        << account                        << nameOnlyAcct.asJson()
        // expectedJsonValue
        << nameOnlyAcct.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << nameOnlyAcct.asJson()          << QString("{}"_L1)
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << true                           << true
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Account_oneOf_1 with only lastname set via setter
    SchemasModelsOneOf::Account_oneOf_1 lastnameOnlyAcct;
    lastnameOnlyAcct.setLastname("OnlyLast"_L1);
    account.setOneOfAccount_oneOf_1(lastnameOnlyAcct);
    QTest::newRow("Set Account_oneOf_1 with only lastname via setter")
        // account                        // expectedJson
        << account                        << lastnameOnlyAcct.asJson()
        // expectedJsonValue
        << lastnameOnlyAcct.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << lastnameOnlyAcct.asJson()
        // isOneOfAccountOneOfValid       // isOneOfAccountOneOfSet
        << false                          << false
        // isOneOfAccountOneOf1Valid      // isOneOfAccountOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;
}

void OneOfTest::testAccountJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::Account, account);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(QString, oneOf0ExpectedJson);
    QFETCH(QString, oneOf1ExpectedJson);
    QFETCH(bool, isOneOfAccountOneOfValid);
    QFETCH(bool, isOneOfAccountOneOfSet);
    QFETCH(bool, isOneOfAccountOneOf1Valid);
    QFETCH(bool, isOneOfAccountOneOf1Set);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(account.asJson(), expectedJson);
    QCOMPARE(account.asJsonValue(), expectedJsonValue);
    QCOMPARE(account.getOneOfAccount_oneOf().asJson(), oneOf0ExpectedJson);
    QCOMPARE(account.isOneOfAccountOneOfValid(), isOneOfAccountOneOfValid);
    QCOMPARE(account.isOneOfAccountOneOfSet(), isOneOfAccountOneOfSet);
    QCOMPARE(account.getOneOfAccount_oneOf_1().asJson(), oneOf1ExpectedJson);
    QCOMPARE(account.isOneOfAccountOneOf1Valid(), isOneOfAccountOneOf1Valid);
    QCOMPARE(account.isOneOfAccountOneOf1Set(), isOneOfAccountOneOf1Set);
    QCOMPARE(account.isValid(), isValid);
    QCOMPARE(account.isSet(), isSet);
}

void OneOfTest::testAlternativeSchemasFunctions_data()
{
    QTest::addColumn<SchemasModelsOneOf::Pet>("pet");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<QString>("oneOfCatExpectedJson");
    QTest::addColumn<QString>("oneOfDogExpectedJson");
    QTest::addColumn<SchemasModelsOneOf::Cat>("getOneOfCat");
    QTest::addColumn<SchemasModelsOneOf::Dog>("getOneOfDog");
    QTest::addColumn<bool>("isOneOfCatValid");
    QTest::addColumn<bool>("isOneOfCatSet");
    QTest::addColumn<bool>("isOneOfDogValid");
    QTest::addColumn<bool>("isOneOfDogSet");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    SchemasModelsOneOf::Pet pet;
    QTest::newRow("Empty Pet object")
        // pet                         // expectedJson
        << pet                         << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOfCatExpectedJson        // oneOfDogExpectedJson
        << QString("{}"_L1)            << QString("{}"_L1)
        // getOneOfCat                 // getOneOfDog
        << SchemasModelsOneOf::Cat()   << SchemasModelsOneOf::Dog()
        // isOneOfCatValid             // isOneOfCatSet
        << false                       << false
        // isOneOfDogValid             // isOneOfDogSet
        << false                       << false
        // isValid                     // isSet
        << false                       << false;

    SchemasModelsOneOf::Cat kitty;
    kitty.setHunts(false);
    kitty.setAge(7);

    SchemasModelsOneOf::Dog hund;
    hund.setBark(false);
    hund.setBreed("French Bulldog"_L1);

    // check setters work fine
    pet.setOneOfCat(kitty);
    QTest::newRow("Set a valid Cat object via setter")
        // pet                         // expectedJson
        << pet                         << kitty.asJson()
        // expectedJsonValue
        << kitty.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{\"age\":7,\"hunts\":false}"_L1)  << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << kitty                                      << SchemasModelsOneOf::Dog()
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // Dog has required field bark, so empty Dog is invalid
    pet.setOneOfDog(hund);
    QTest::newRow("Set a valid Dog object via setter")
        // pet                                        // expectedJson
        << pet                                        << hund.asJson()
        // expectedJsonValue
        << hund.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{\"bark\":false,\"breed\":\"French Bulldog\"}"_L1)
        // getOneOfCat                                // getOneOfDog
        << SchemasModelsOneOf::Cat()                  << hund
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << true                                       << true
        // isValid                                    // isSet
        << true                                       << true;

    // Set an empty Dog object, the dog has required fields, so an empty
    // dog is INVALID => entire Pet stays invalid
    const SchemasModelsOneOf::Dog emptyDog;
    pet.setOneOfDog(emptyDog);
    QTest::newRow("Set an empty Dog object via setter")
        // pet                                        // expectedJson
        << pet                                        << emptyDog.asJson()
        // expectedJsonValue
        << emptyDog.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << SchemasModelsOneOf::Cat()                  << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << false                                      << false;

    // Set an empty Cat object, the cat has no required fields, so an empty
    // cat is VALID => entire Pet becomes valid
    const SchemasModelsOneOf::Cat emptyCat;
    pet.setOneOfCat(emptyCat);
    QTest::newRow("Set an empty Cat object via setter")
        // pet                                        // expectedJson
        << pet                                        << emptyCat.asJson()
        // expectedJsonValue
        << emptyCat.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // fromJsonValue with EMPTY data.
    // Attempting to set an empty Dog via pet.fromJsonValue().
    // Dog defines required fields, so an empty object is INVALID for Dog.
    //
    // However, emptyDog.asJsonValue() produces '{}' because it is an empty object.
    //
    // The Pet::fromJsonValue() logic cannot determine whether '{}' represents
    // a Cat or a Dog. The only available approach is to try deserializing '{}'
    // against each schema and select the one that validates successfully.
    //
    // This leads to ambiguity, since '{}' is a valid Cat instance.
    //
    // To avoid such situations, the schema should define either a discriminator
    // or required fields for each Object in the YAML specification.
    //
    // This ambiguity is intentional here for testing purposes, to highlight
    // the importance of well-defined API schemas. Ensuring correctness of the
    // API design is the responsibility of the API author.
    pet.fromJsonValue(emptyDog.asJsonValue());
    QTest::newRow("Set an empty Dog via ::fromJsonValue(emptyDog.asJsonValue())")
        // pet                                        // expectedJson
        << pet                                        << emptyDog.asJson()
        // expectedJsonValue
        << emptyDog.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << emptyDog
        // !!! NOTE: ambiguity is here, cat is VALID!
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // fromJson with EMPTY data.
    // Same expectations like with
    // 'Set an empty Dog via pet.fromJsonValue(emptyDog.asJsonValue())'
    pet.fromJson(emptyDog.asJson());
    QTest::newRow("Set an empty Dog via ::fromJson(emptyDog.asJson())")
        // pet                                        // expectedJson
        << pet                                        << emptyDog.asJson()
        // expectedJsonValue
        << emptyDog.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << emptyDog
        // !!! NOTE: ambiguity is here, cat is VALID!
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // fromJson with EMPTY data.
    // Same expectations like with
    // 'Set an empty Dog via pet.fromJsonValue(emptyDog.asJsonValue())'
    pet.fromJson(emptyCat.asJson());
    QTest::newRow("Set an empty Cat via ::fromJson(emptyCat.asJson())")
        // pet                                        // expectedJson
        << pet                                        << emptyCat.asJson()
        // expectedJsonValue
        << emptyCat.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    pet.fromJsonValue(emptyCat.asJsonValue());
    QTest::newRow("Set an empty Cat via ::fromJsonValue(emptyCat.asJsonValue())")
        // pet                                        // expectedJson
        << pet                                        << emptyCat.asJson()
        // expectedJsonValue
        << emptyCat.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // check fromJson() works fine for VALID cat object
    const QString kittyCat("{\"age\":90,\"hunts\":false}"_L1);
    pet.fromJson(kittyCat);
    QTest::newRow("Set a valid Cat object via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << kittyCat
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << kittyCat                                   << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << SchemasModelsOneOf::Cat(kittyCat)          << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // check fromJsonValue() works fine for VALID cat object
    const SchemasModelsOneOf::Cat okCat(kittyCat);
    pet.fromJsonValue(okCat.asJsonValue());
    QTest::newRow("Set a valid Cat object via ::fromJsonValue()")
        // pet                                        // expectedJson
        << pet                                        << kittyCat
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << kittyCat                                   << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << SchemasModelsOneOf::Cat(kittyCat)          << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // check fromJson() works fine for VALID cat object
    const QString doggyDog("{\"bark\":false,\"breed\":\"French Bulldog\"}"_L1);
    pet.fromJson(doggyDog);
    QTest::newRow("Set a valid Dog object via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << doggyDog
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << doggyDog
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << SchemasModelsOneOf::Dog(doggyDog)
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << true                                       << true
        // isValid                                    // isSet
        << true                                       << true;

    // check fromJsonValue() works fine for VALID cat object
    const SchemasModelsOneOf::Dog okDog(doggyDog);
    pet.fromJsonValue(okDog.asJsonValue());
    QTest::newRow("Set a valid Dog object via ::fromJsonValue()")
        // pet                                        // expectedJson
        << pet                                        << doggyDog
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << doggyDog
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << okDog
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << true                                       << true
        // isValid                                    // isSet
        << true                                       << true;

    // invalid json!
    pet.fromJson("{invalid json}"_L1);
    QTest::newRow("Set an invalid json via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << QString("{}"_L1)
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << false                                      << false;

    // Test with non-object JSON (array)
    pet.fromJson("[]"_L1);
    QTest::newRow("Set an unexpected array [] via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << QString("{}"_L1)
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << false                                      << false;


    // Test with non-object JSON (string)
    pet.fromJson("\"just a string\""_L1);
    QTest::newRow("Set an unexpected string via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << QString("{}"_L1)
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << false                                      << false;

    // Missing Required Fields (Dog's 'bark' is required)
    // Dog without required 'bark' field - should not be valid
    SchemasModelsOneOf::Dog invalidDog;
    invalidDog.setBreed("Husky-kolbasky"_L1);
    pet.setOneOfDog(invalidDog);
    QTest::newRow("Set an invalid Dog via setter")
        // pet                                        // expectedJson
        << pet                                        << QString("{}"_L1)
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{\"breed\":\"Husky-kolbasky\"}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << invalidDog
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // The status here reflects the status of the model that was set.
        // The model's field was set by setter, so invalidDog.isSet() == true
        // but invalidDog.isValid() == false, because required field is not set.
        // isOneOfDogValid                            // isOneOfDogSet
        << invalidDog.isValid()                       << invalidDog.isSet()
        // isValid                                    // isSet
        << false                                      << true;

    // Try to parse JSON missing required field, the dog is invalid.
    pet.fromJson("{\"breed\":\"Retriever\"}"_L1);
    QTest::newRow("Set an invalid Dog via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << QString("{}"_L1)
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{\"breed\":\"Retriever\"}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << SchemasModelsOneOf::Dog("{\"breed\":\"Retriever\"}"_L1)
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << true
        // isValid                                    // isSet
        << false                                      << true;
    // Partial/Incomplete JSON Objects
    // Only one field for Cat (age without hunts),
    // should work since Cat has no required fields
    const QString notFullCat("{\"age\":5}"_L1);
    pet.fromJson(notFullCat);
    QTest::newRow("Set a valid non-full Cat via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << notFullCat
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << notFullCat                                 << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << SchemasModelsOneOf::Cat("{\"age\":5}"_L1)  << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // Extra/Unknown Fields in JSON
    // JSON with fields not in schema - should parse known fields and ignore unknown
    pet.fromJson("{\"age\":7,\"hunts\":false,\"name\":\"Fluffy\",\"color\":\"orange\"}"_L1);
    QString expected("{\"age\":7,\"hunts\":false}"_L1);
    QTest::newRow("Extra/Unknown Fields in JSON via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << expected
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << expected                                   << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << SchemasModelsOneOf::Cat(expected)          << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // Empty Strings in Fields
    expected = QString("{\"bark\":true,\"breed\":\"\"}"_L1);
    pet.fromJson(expected);
    QTest::newRow("An empty String in Dog breed field via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << expected
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << expected
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << SchemasModelsOneOf::Dog(expected)
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << true                                       << true
        // isValid                                    // isSet
        << true                                       << true;

    // Invalid Field Types for Cat(). Wrong type for age (string instead of integer)
    // Note: field of Cat type are not required, it means wrong fields can be omitted
    expected = QString("{\"age\":\"seven\",\"hunts\":false}"_L1);
    pet.fromJson(expected);
    QTest::newRow("Invalid field 'age=seven' in Cat via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << QString("{\"hunts\":false}"_L1)
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{\"hunts\":false}"_L1)            << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << SchemasModelsOneOf::Cat(expected)          << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // Invalid Field Types for Cat(). Wrong type for age (double instead of integer)
    // Note: field of Cat type are not required, it means wrong fields can be omitted
    expected = QString("{\"age\":0.1,\"hunts\":true}"_L1);
    pet.fromJson(expected);
    QTest::newRow("Invalid field 'age=0.1' in Cat via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << QString("{\"hunts\":true}"_L1)
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{\"hunts\":true}"_L1)             << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << SchemasModelsOneOf::Cat(expected)          << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    // Invalid Field Types in for Cat(). Wrong type for age (bool instead of integer)
    // Note: field of Cat type are not required, it means wrong fields can be omitted
    expected = QString("{\"age\":true,\"hunts\":true}"_L1);
    pet.fromJson(expected);
    QTest::newRow("Invalid field 'age=true' in Cat via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << QString("{\"hunts\":true}"_L1)
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{\"hunts\":true}"_L1)             << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << SchemasModelsOneOf::Cat(expected)          << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << true                                       << true
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << true                                       << true;

    expected = QString("{\"age\":true,\"hunts\":1}"_L1);
    pet.fromJson(expected);
    QTest::newRow("Invalid fields: 'age=true', hunts=1 in Cat via ::fromJson()")
        // pet                                        // expectedJson
        << pet                                        << QString("{}"_L1)
        // expectedJsonValue
        << pet.asJsonValue()
        // oneOfCatExpectedJson                       // oneOfDogExpectedJson
        << QString("{}"_L1)                           << QString("{}"_L1)
        // getOneOfCat                                // getOneOfDog
        << emptyCat                                   << emptyDog
        // isOneOfCatValid                            // isOneOfCatSet
        << false                                      << false
        // isOneOfDogValid                            // isOneOfDogSet
        << false                                      << false
        // isValid                                    // isSet
        << false                                      << false;
}

void OneOfTest::testAlternativeSchemasFunctions()
{
    QFETCH(SchemasModelsOneOf::Pet, pet);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(QString, oneOfCatExpectedJson);
    QFETCH(QString, oneOfDogExpectedJson);
    QFETCH(SchemasModelsOneOf::Cat, getOneOfCat);
    QFETCH(SchemasModelsOneOf::Dog, getOneOfDog);
    QFETCH(bool, isOneOfCatValid);
    QFETCH(bool, isOneOfCatSet);
    QFETCH(bool, isOneOfDogValid);
    QFETCH(bool, isOneOfDogSet);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(pet.asJson(), expectedJson);
    QCOMPARE(pet.asJsonValue(), expectedJsonValue);
    QCOMPARE(pet.getOneOfCat().asJson(), oneOfCatExpectedJson);
    QCOMPARE(pet.isOneOfCatValid(), isOneOfCatValid);
    QCOMPARE(pet.isOneOfCatSet(), isOneOfCatSet);
    QCOMPARE(pet.getOneOfDog().asJson(), oneOfDogExpectedJson);
    QCOMPARE(pet.getOneOfCat(), getOneOfCat);
    QCOMPARE(pet.getOneOfDog(), getOneOfDog);
    QCOMPARE(pet.isOneOfDogValid(), isOneOfDogValid);
    QCOMPARE(pet.isOneOfDogSet(), isOneOfDogSet);
    QCOMPARE(pet.isValid(), isValid);
    QCOMPARE(pet.isSet(), isSet);
}

void OneOfTest::testAlternativeSchemasOptional_data()
{
    generateAlternativeSchemasTestData();
}

void OneOfTest::testAlternativeSchemasOptional()
{
    QFETCH(SchemasModelsOneOf::Pet, pet);
    QFETCH(SchemasModelsOneOf::Cat, cat);
    QFETCH(SchemasModelsOneOf::Dog, dog);
    QFETCH(OneOfTest::StoreApiMode, mode);
    QFETCH(QString, expectedJson);

    bool done = true;
    SchemasModelsOneOf::StoreApi api;
    LoggingNetworkAccessManager logging(&api);
    QRestAccessManager restManager(&logging, &api);
    api.setRestAccessManager(&restManager);

    switch (mode) {
    case OneOfTest::StoreApiMode::UseCat:
    {
        api.postPetData(QtOpenApiCommon::OptionalParameter<SchemasModelsOneOf::Pet>(cat),
                        this, [&](const QRestReply &reply) {
                            done = reply.isSuccess();
                        });
    } break;
    case OneOfTest::StoreApiMode::UseDog:
    {
        api.postPetData(QtOpenApiCommon::OptionalParameter<SchemasModelsOneOf::Pet>(dog),
                        this, [&](const QRestReply &reply) {
                            done = reply.isSuccess();
                        });
    } break;
    case OneOfTest::StoreApiMode::UsePet:
    {
        api.postPetData(pet, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    }
    QCOMPARE(logging.m_content, expectedJson);
    QTRY_COMPARE_EQ(done, false);
}

void OneOfTest::generateAlternativeSchemasTestData()
{
    QTest::addColumn<SchemasModelsOneOf::Pet>("pet");
    QTest::addColumn<SchemasModelsOneOf::Cat>("cat");
    QTest::addColumn<SchemasModelsOneOf::Dog>("dog");
    QTest::addColumn<OneOfTest::StoreApiMode>("mode");
    QTest::addColumn<QString>("expectedJson");

    QTest::newRow("empty")
        << SchemasModelsOneOf::Pet{}
        << SchemasModelsOneOf::Cat{}
        << SchemasModelsOneOf::Dog{}
        << OneOfTest::StoreApiMode::UsePet
        << QString("{}"_L1);

    SchemasModelsOneOf::Pet pet;
    SchemasModelsOneOf::Cat kitty;
    kitty.setHunts(true);
    kitty.setAge(7);

    SchemasModelsOneOf::Dog hund;
    hund.setBark(false);
    hund.setBreed("Labrador"_L1);

    QTest::newRow("Use Cat object")
        << SchemasModelsOneOf::Pet{}
        << kitty
        << SchemasModelsOneOf::Dog{}
        << OneOfTest::StoreApiMode::UseCat
        << QString("{\"age\":7,\"hunts\":true}"_L1);

    QTest::newRow("Use Dog object")
        << SchemasModelsOneOf::Pet{}
        << SchemasModelsOneOf::Cat{}
        << hund
        << OneOfTest::StoreApiMode::UseDog
        << QString("{\"bark\":false,\"breed\":\"Labrador\"}"_L1);

    // Setting the cat the pet()
    kitty.setAge(9);
    pet.setOneOfCat(kitty);
    QTest::newRow("Set the cat data to a pet object")
        << pet
        << SchemasModelsOneOf::Cat{}
        << SchemasModelsOneOf::Dog{}
        << OneOfTest::StoreApiMode::UsePet
        << QString("{\"age\":9,\"hunts\":true}"_L1);

    hund.setBark(true);
    pet.setOneOfDog(hund);
    QTest::newRow("Set the dog data to a pet object")
        << pet
        << SchemasModelsOneOf::Cat{}
        << SchemasModelsOneOf::Dog{}
        << OneOfTest::StoreApiMode::UsePet
        << QString("{\"bark\":true,\"breed\":\"Labrador\"}"_L1);

    pet.setOneOfDog(SchemasModelsOneOf::Dog());
    QTest::newRow("Reset the pet data to an empty Dog()")
        << pet
        << SchemasModelsOneOf::Cat{}
        << SchemasModelsOneOf::Dog{}
        << OneOfTest::StoreApiMode::UsePet
        << QString("{}"_L1);

    SchemasModelsOneOf::Dog invalidDog;
    invalidDog.setBreed("Unknown"_L1);
    QTest::newRow("Construct the dog without setting the `required` field(bark)")
        << SchemasModelsOneOf::Pet{}
        << SchemasModelsOneOf::Cat{}
        << invalidDog
        << OneOfTest::StoreApiMode::UseDog
        // It's empty because the object is not valid, without setting the required field
        << QString("{}"_L1);

    // Construct the cat but skipping some fields.
    // NOTE: Cat type doesn't have `required` fields,
    // so it will not be empty.
    SchemasModelsOneOf::Cat cat;
    cat.setAge(5);
    QTest::newRow("Construct the cat with only age field set")
        << SchemasModelsOneOf::Pet{}
        << cat
        << SchemasModelsOneOf::Dog{}
        << OneOfTest::StoreApiMode::UseCat
        << QString("{\"age\":5}"_L1);
}

void OneOfTest::testAlternativeSchemasRequired_data()
{
    generateAlternativeSchemasTestData();
}

void OneOfTest::testAlternativeSchemasRequired()
{
    QFETCH(SchemasModelsOneOf::Pet, pet);
    QFETCH(SchemasModelsOneOf::Cat, cat);
    QFETCH(SchemasModelsOneOf::Dog, dog);
    QFETCH(OneOfTest::StoreApiMode, mode);
    QFETCH(QString, expectedJson);

    bool done = true;
    SchemasModelsOneOf::StoreApi api;
    LoggingNetworkAccessManager logging(&api);
    QRestAccessManager restManager(&logging, &api);
    api.setRestAccessManager(&restManager);

    switch (mode) {
    case OneOfTest::StoreApiMode::UseCat:
    {
        api.patchPetData(cat, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::StoreApiMode::UseDog:
    {
        api.patchPetData(dog, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::StoreApiMode::UsePet:
    {
        api.patchPetData(pet, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    }
    QCOMPARE(logging.m_content, expectedJson);
    QTRY_COMPARE_EQ(done, false);
}

void OneOfTest::testClientValueJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::Client_value>("clientValue");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<QString>("oneOf0ExpectedValue");
    QTest::addColumn<bool>("oneOf1ExpectedValue");
    QTest::addColumn<bool>("isOneOf0Valid");
    QTest::addColumn<bool>("isOneOf0Set");
    QTest::addColumn<bool>("isOneOf1Valid");
    QTest::addColumn<bool>("isOneOf1Set");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    // Client_value has two primitive oneOf alternatives:
    // oneOf0: string - always valid
    // oneOf1: boolean - always valid
    SchemasModelsOneOf::Client_value clientValue;
    QTest::newRow("Empty Client_value object")
        // clientValue                    // expectedJson
        << clientValue                    << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << false
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set a string value via setter
    clientValue.setOneOfQString("hello-client"_L1);
    QTest::newRow("Set a valid string via setter")
        // clientValue                    // expectedJson
        << clientValue                    << QString("\"hello-client\""_L1)
        // expectedJsonValue
        << QJsonValue("hello-client"_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString("hello-client"_L1)     << false
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a bool 'true' via setter
    clientValue.setOneOfBool(true);
    QTest::newRow("Set a valid bool 'true' via setter")
        // clientValue                    // expectedJson
        << clientValue                    << QString("true"_L1)
        // expectedJsonValue
        << QJsonValue(true)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << true
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set a bool 'false' via setter
    clientValue.setOneOfBool(false);
    QTest::newRow("Set a valid bool 'false' via setter")
        // clientValue                    // expectedJson
        << clientValue                    << QString("false"_L1)
        // expectedJsonValue
        << QJsonValue(false)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << false
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set an empty string via setter
    clientValue.setOneOfQString("");
    QTest::newRow("Set an empty string via setter")
        // clientValue                    // expectedJson
        << clientValue                    << QString("\"\""_L1)
        // expectedJsonValue
        << QJsonValue(""_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString(""_L1)                 << false
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a string value
    clientValue.fromJson("\"client-data\""_L1);
    QTest::newRow("Set a valid string via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("\"client-data\""_L1)
        // expectedJsonValue
        << QJsonValue("client-data"_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString("client-data"_L1)      << false
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a bool 'true' value
    clientValue.fromJson("true"_L1);
    QTest::newRow("Set a valid bool 'true' via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("true"_L1)
        // expectedJsonValue
        << QJsonValue(true)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << true
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a bool 'false' value
    clientValue.fromJson("false"_L1);
    QTest::newRow("Set a valid bool 'false' via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("false"_L1)
        // expectedJsonValue
        << QJsonValue(false)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << false
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with string
    clientValue.fromJsonValue(QJsonValue("via-json-value"_L1));
    QTest::newRow("Set a valid string via ::fromJsonValue()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("\"via-json-value\""_L1)
        // expectedJsonValue
        << QJsonValue("via-json-value"_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString("via-json-value"_L1)   << false
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with bool
    clientValue.fromJsonValue(QJsonValue(true));
    QTest::newRow("Set a valid bool 'true' via ::fromJsonValue()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("true"_L1)
        // expectedJsonValue
        << QJsonValue(true)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << true
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // invalid json! => the model is RESET to INITIAL state
    clientValue.fromJson("{invalid json}"_L1);
    QTest::newRow("Set an invalid json via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << false
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-primitive JSON (object) => doesn't match string or bool
    clientValue.fromJson("{\"key\":\"value\"}"_L1);
    QTest::newRow("Set an unexpected object via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << false
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with array JSON => doesn't match string or bool
    clientValue.fromJson("[1, 2]"_L1);
    QTest::newRow("Client_value: set an unexpected array via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << false
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with null JSON => doesn't match string or bool
    clientValue.fromJson("null"_L1);
    QTest::newRow("Set a null value via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << false
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with number JSON => doesn't match string or bool
    clientValue.fromJson("42"_L1);
    QTest::newRow("Set an unexpected number via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << false
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson with a string that looks like a bool (but is quoted => string)
    clientValue.fromJson("\"true\""_L1);
    QTest::newRow("Set a string 'true' (quoted) via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("\"true\""_L1)
        // expectedJsonValue
        << QJsonValue("true"_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString("true"_L1)             << false
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a string that looks like a number (but is quoted => string)
    clientValue.fromJson("\"42\""_L1);
    QTest::newRow("Set a string '42' (quoted) via ::fromJson()")
        // clientValue                    // expectedJson
        << clientValue                    << QString("\"42\""_L1)
        // expectedJsonValue
        << QJsonValue("42"_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString("42"_L1)               << false
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Override string with bool via setter
    clientValue.setOneOfQString("will be overridden"_L1);
    clientValue.setOneOfBool(true);
    QTest::newRow("Override string with bool via setter")
        // clientValue                    // expectedJson
        << clientValue                    << QString("true"_L1)
        // expectedJsonValue
        << QJsonValue(true)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString()                      << true
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Override bool with string via setter
    clientValue.setOneOfBool(false);
    clientValue.setOneOfQString("override-back"_L1);
    QTest::newRow("Override bool with string via setter")
        // clientValue                    // expectedJson
        << clientValue                    << QString("\"override-back\""_L1)
        // expectedJsonValue
        << QJsonValue("override-back"_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << QString("override-back"_L1)    << false
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // String with special characters
    clientValue.setOneOfQString("line1\nline2\ttab"_L1);
    QTest::newRow("Set a string with special characters via setter")
        // clientValue                    // expectedJson
        << clientValue                    << QString("\"line1\\nline2\\ttab\""_L1)
        // expectedJsonValue
        << QJsonValue("line1\nline2\ttab"_L1)
        // oneOf0ExpectedValue
        << QString("line1\nline2\ttab"_L1)
        // oneOf1ExpectedValue
        << false
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;
}

void OneOfTest::testClientValueJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::Client_value, clientValue);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(QString, oneOf0ExpectedValue);
    QFETCH(bool, oneOf1ExpectedValue);
    QFETCH(bool, isOneOf0Valid);
    QFETCH(bool, isOneOf0Set);
    QFETCH(bool, isOneOf1Valid);
    QFETCH(bool, isOneOf1Set);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(clientValue.asJson(), expectedJson);
    QCOMPARE(clientValue.asJsonValue(), expectedJsonValue);
    QCOMPARE(clientValue.getOneOfQString(), oneOf0ExpectedValue);
    QCOMPARE(clientValue.isOneOfQStringValid(), isOneOf0Valid);
    QCOMPARE(clientValue.isOneOfQStringSet(), isOneOf0Set);
    QCOMPARE(clientValue.getOneOfBool(), oneOf1ExpectedValue);
    QCOMPARE(clientValue.isOneOfBoolValid(), isOneOf1Valid);
    QCOMPARE(clientValue.isOneOfBoolSet(), isOneOf1Set);
    QCOMPARE(clientValue.isValid(), isValid);
    QCOMPARE(clientValue.isSet(), isSet);
}

void OneOfTest::testCustomerJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::PostSearchData_request_customer>("customer");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<qint32>("oneOf0ExpectedValue");
    QTest::addColumn<QString>("oneOf1ExpectedValue");
    QTest::addColumn<QString>("oneOf2ExpectedJson");
    QTest::addColumn<QString>("oneOf3ExpectedJson");
    QTest::addColumn<bool>("isOneOf0Valid");
    QTest::addColumn<bool>("isOneOf0Set");
    QTest::addColumn<bool>("isOneOf1Valid");
    QTest::addColumn<bool>("isOneOf1Set");
    QTest::addColumn<bool>("isOneOf2Valid");
    QTest::addColumn<bool>("isOneOf2Set");
    QTest::addColumn<bool>("isOneOf3Valid");
    QTest::addColumn<bool>("isOneOf3Set");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    // PostSearchData_request_customer has 4 oneOf alternatives:
    // oneOf0: qint32 (customer ID) - primitive, always valid
    // oneOf1: QString (phone number) - primitive, always valid
    // oneOf2: PostSearchData_request_customer_oneOf (userName REQUIRED, channel optional)
    // oneOf3: PostSearchData_request_customer_oneOf_1 (customerId REQUIRED)
    SchemasModelsOneOf::PostSearchData_request_customer customer;
    QTest::newRow("Empty PostSearchData_request_customer object")
        // customer                       // expectedJson
        << customer                       << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set an integer (customer ID) via setter
    customer.setOneOfQint32(12345);
    QTest::newRow("Set a valid integer (customer ID) via setter")
        // customer                       // expectedJson
        << customer                       << QString("12345"_L1)
        // expectedJsonValue
        << QJsonValue(12345)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(12345)                  << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a string (phone number) via setter
    customer.setOneOfQString("+1234567890"_L1);
    QTest::newRow("Set a valid string (phone number) via setter")
        // customer                       // expectedJson
        << customer                       << QString("\"+1234567890\""_L1)
        // expectedJsonValue
        << QJsonValue("+1234567890"_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString("+1234567890"_L1)
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a valid object with userName (required) via setter
    SchemasModelsOneOf::PostSearchData_request_customer_oneOf userObj;
    userObj.setUserName("john_doe"_L1);
    userObj.setChannel("web"_L1);
    customer.setOneOfPostSearchData_request_customer_oneOf(userObj);
    QTest::newRow("Set a valid userName object via setter")
        // customer                       // expectedJson
        << customer                       << userObj.asJson()
        // expectedJsonValue
        << userObj.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << userObj.asJson()               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << true                           << true
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a valid object with customerId (required) via setter
    SchemasModelsOneOf::PostSearchData_request_customer_oneOf_1 customerIdObj;
    customerIdObj.setCustomerId("CUST-001"_L1);
    customer.setOneOfPostSearchData_request_customer_oneOf_1(customerIdObj);
    QTest::newRow("Set a valid customerId object via setter")
        // customer                       // expectedJson
        << customer                       << customerIdObj.asJson()
        // expectedJsonValue
        << customerIdObj.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << customerIdObj.asJson()
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set an empty userName object (missing required userName) => invalid
    SchemasModelsOneOf::PostSearchData_request_customer_oneOf emptyUserObj;
    customer.setOneOfPostSearchData_request_customer_oneOf(emptyUserObj);
    QTest::newRow("Set an empty userName object via setter")
        // customer                       // expectedJson
        << customer                       << emptyUserObj.asJson()
        // expectedJsonValue
        << emptyUserObj.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set an empty customerId object (missing required customerId) => invalid
    SchemasModelsOneOf::PostSearchData_request_customer_oneOf_1 emptyCustomerIdObj;
    customer.setOneOfPostSearchData_request_customer_oneOf_1(emptyCustomerIdObj);
    QTest::newRow("Set an empty customerId object via setter")
        // customer                       // expectedJson
        << customer                       << emptyCustomerIdObj.asJson()
        // expectedJsonValue
        << emptyCustomerIdObj.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set zero integer via setter (still valid - it's a legitimate value)
    customer.setOneOfQint32(0);
    QTest::newRow("Set zero integer via setter")
        // customer                       // expectedJson
        << customer                       << QString("0"_L1)
        // expectedJsonValue
        << QJsonValue(0)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set negative integer via setter
    customer.setOneOfQint32(-42);
    QTest::newRow("Set negative integer via setter")
        // customer                       // expectedJson
        << customer                       << QString("-42"_L1)
        // expectedJsonValue
        << QJsonValue(-42)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(-42)                    << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with an integer value
    customer.fromJson("99999"_L1);
    QTest::newRow("Set a valid integer via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << QString("99999"_L1)
        // expectedJsonValue
        << QJsonValue(99999)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(99999)                  << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a string value
    customer.fromJson("\"+79001234567\""_L1);
    QTest::newRow("Set a valid phone number string via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << QString("\"+79001234567\""_L1)
        // expectedJsonValue
        << QJsonValue("+79001234567"_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString("+79001234567"_L1)
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a valid userName object
    const QString validUserJson("{\"channel\":\"mobile\",\"userName\":\"jane_doe\"}"_L1);
    customer.fromJson(validUserJson);
    QTest::newRow("Set a valid userName object via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << validUserJson
        // expectedJsonValue
        << customer.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << validUserJson                  << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << true                           << true
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a valid customerId object
    const QString validCustIdJson("{\"customerId\":\"CUST-999\"}"_L1);
    customer.fromJson(validCustIdJson);
    QTest::newRow("Set a valid customerId object via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << validCustIdJson
        // expectedJsonValue
        << customer.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << validCustIdJson
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson with userName object only (without channel - channel is optional)
    const QString userOnlyJson("{\"userName\":\"admin\"}"_L1);
    customer.fromJson(userOnlyJson);
    QTest::newRow("Set userName object without channel via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << userOnlyJson
        // expectedJsonValue
        << customer.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << userOnlyJson                   << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << true                           << true
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with integer
    customer.fromJsonValue(QJsonValue(777));
    QTest::newRow("Set a valid integer via ::fromJsonValue()")
        // customer                       // expectedJson
        << customer                       << QString("777"_L1)
        // expectedJsonValue
        << QJsonValue(777)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(777)                    << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with string
    customer.fromJsonValue(QJsonValue("+4912345"_L1));
    QTest::newRow("Set a valid string via ::fromJsonValue()")
        // customer                       // expectedJson
        << customer                       << QString("\"+4912345\""_L1)
        // expectedJsonValue
        << QJsonValue("+4912345"_L1)
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString("+4912345"_L1)
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with valid userName object
    customer.fromJsonValue(userObj.asJsonValue());
    QTest::newRow("Set a valid userName object via ::fromJsonValue()")
        // customer                       // expectedJson
        << customer                       << userObj.asJson()
        // expectedJsonValue
        << customer.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << userObj.asJson()               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << true                           << true
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with valid customerId object
    customer.fromJsonValue(customerIdObj.asJsonValue());
    QTest::newRow("Set a valid customerId object via ::fromJsonValue()")
        // customer                       // expectedJson
        << customer                       << customerIdObj.asJson()
        // expectedJsonValue
        << customer.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << customerIdObj.asJson()
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // invalid json! => the model is RESET to INITIAL state
    customer.fromJson("{invalid json}"_L1);
    QTest::newRow("Set an invalid json via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with array JSON => doesn't match any alternative
    customer.fromJson("[]"_L1);
    QTest::newRow("PostSearchData_request_customer: set an unexpected array via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson with empty object "{}" => neither userName nor customerId objects
    // are valid (both have required fields). Does not match int or string either.
    customer.fromJson("{}"_L1);
    QTest::newRow("Set an empty object {} via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << QString("{}"_L1)
        // expectedJsonValue
        << customer.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson with a bool value => doesn't match int, string, or objects
    customer.fromJson("true"_L1);
    QTest::newRow("Set an unexpected bool via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson with a double value => doesn't match qint32 (not integer)
    customer.fromJson("3.14"_L1);
    QTest::newRow("Set a double via ::fromJson() (no double alternative)")
        // customer                       // expectedJson
        << customer                       << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                           << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << false
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Invalid userName object - missing required userName, has only channel
    customer.fromJson("{\"channel\":\"email\"}"_L1);
    QTest::newRow("Set userName object missing required 'userName' via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << QString("{}"_L1)
        // expectedJsonValue
        << customer.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{\"channel\":\"email\"}"_L1) << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << true
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << true;

    // Extra/Unknown Fields in userName object JSON
    customer.fromJson("{\"userName\":\"user1\",\"channel\":\"sms\",\"extra\":\"field\"}"_L1);
    QString expected("{\"channel\":\"sms\",\"userName\":\"user1\"}"_L1);
    QTest::newRow("Extra/Unknown Fields in userName object via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << expected
        // expectedJsonValue
        << customer.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << expected                       << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << true                           << true
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set userName object with invalid type for userName field (int instead of string)
    customer.fromJson("{\"userName\":123,\"channel\":\"web\"}"_L1);
    QTest::newRow("Invalid 'userName=123' in userName object via ::fromJson()")
        // customer                       // expectedJson
        << customer                       << QString("{}"_L1)
        // expectedJsonValue
        << customer.asJsonValue()
        // oneOf0ExpectedValue            // oneOf1ExpectedValue
        << qint32(0)                      << QString()
        // oneOf2ExpectedJson             // oneOf3ExpectedJson
        << QString("{\"channel\":\"web\"}"_L1) << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isOneOf2Valid                  // isOneOf2Valid
        << false                          << true
        // isOneOf3Valid                  // isOneOf3Set
        << false                          << false
        // isValid                        // isSet
        << false                          << true;
}

void OneOfTest::testCustomerJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::PostSearchData_request_customer, customer);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(qint32, oneOf0ExpectedValue);
    QFETCH(QString, oneOf1ExpectedValue);
    QFETCH(QString, oneOf2ExpectedJson);
    QFETCH(QString, oneOf3ExpectedJson);
    QFETCH(bool, isOneOf0Valid);
    QFETCH(bool, isOneOf0Set);
    QFETCH(bool, isOneOf1Valid);
    QFETCH(bool, isOneOf1Set);
    QFETCH(bool, isOneOf2Valid);
    QFETCH(bool, isOneOf2Set);
    QFETCH(bool, isOneOf3Valid);
    QFETCH(bool, isOneOf3Set);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(customer.asJson(), expectedJson);
    QCOMPARE(customer.asJsonValue(), expectedJsonValue);
    QCOMPARE(customer.getOneOfQint32(), oneOf0ExpectedValue);
    QCOMPARE(customer.isOneOfQint32Valid(), isOneOf0Valid);
    QCOMPARE(customer.isOneOfQint32Set(), isOneOf0Set);
    QCOMPARE(customer.getOneOfQString(), oneOf1ExpectedValue);
    QCOMPARE(customer.isOneOfQStringValid(), isOneOf1Valid);
    QCOMPARE(customer.isOneOfQStringSet(), isOneOf1Set);
    QCOMPARE(customer.getOneOfPostSearchData_request_customer_oneOf().asJson(), oneOf2ExpectedJson);
    QCOMPARE(customer.isOneOfPostSearchDataRequestCustomerOneOfValid(), isOneOf2Valid);
    QCOMPARE(customer.isOneOfPostSearchDataRequestCustomerOneOfSet(), isOneOf2Set);
    QCOMPARE(customer.getOneOfPostSearchData_request_customer_oneOf_1().asJson(), oneOf3ExpectedJson);
    QCOMPARE(customer.isOneOfPostSearchDataRequestCustomerOneOf1Valid(), isOneOf3Valid);
    QCOMPARE(customer.isOneOfPostSearchDataRequestCustomerOneOf1Set(), isOneOf3Set);
    QCOMPARE(customer.isValid(), isValid);
    QCOMPARE(customer.isSet(), isSet);
}

void OneOfTest::testInlineSchemasJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::PostPaymentData_request>("postRequest");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<QString>("oneOf0ExpectedJson");
    QTest::addColumn<QString>("oneOf1ExpectedJson");
    QTest::addColumn<SchemasModelsOneOf::PostPaymentData_request_oneOf>("getOneOfPostPaymentData_request_oneOf");
    QTest::addColumn<SchemasModelsOneOf::PostPaymentData_request_oneOf_1>("getOneOfPostPaymentData_request_oneOf_1");
    QTest::addColumn<bool>("isOneOfPostPaymentDataRequestOneOfValid");
    QTest::addColumn<bool>("isOneOfPostPaymentDataRequestOneOfSet");
    QTest::addColumn<bool>("isOneOfPostPaymentDataRequestOneOf1Valid");
    QTest::addColumn<bool>("isOneOfPostPaymentDataRequestOneOf1Set");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    // PostPaymentData_request has two inline object schemas:
    // oneOf0: PostPaymentData_request_oneOf with cardNumber (string, REQUIRED)
    // and cardAvailability (bool)
    // oneOf1: PostPaymentData_request_oneOf_1 with paypalEmail (string)
    // and paypalAvailability (bool)
    SchemasModelsOneOf::PostPaymentData_request postRequest;
    QTest::newRow("Empty PostPaymentData_request object")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << SchemasModelsOneOf::PostPaymentData_request_oneOf()
        // getOneOfPostPaymentData_request_oneOf_1
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1()
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << false                          << false;

    // Create valid credit card (oneOf0) - has required 'cardNumber'
    SchemasModelsOneOf::PostPaymentData_request_oneOf creditCard;
    creditCard.setCardNumber("1234-5678-9012-3456"_L1);
    creditCard.setCardAvailability(true);

    // Create valid paypal (oneOf1) - no required fields
    SchemasModelsOneOf::PostPaymentData_request_oneOf_1 paypal;
    paypal.setPaypalEmail("user@example.com"_L1);
    paypal.setPaypalAvailability(true);

    // Set a valid credit card via setter
    postRequest.setOneOfPostPaymentData_request_oneOf(creditCard);
    QTest::newRow("Set a valid CreditCard object via setter")
        // postRequest                    // expectedJson
        << postRequest                    << creditCard.asJson()
        // expectedJsonValue
        << creditCard.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << creditCard.asJson()            << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << creditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1()
        // isOneOfPostPaymentDataRequestOneOfValid
        << true
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << true                           << true;

    // Set a valid paypal via setter (overrides credit card)
    postRequest.setOneOfPostPaymentData_request_oneOf_1(paypal);
    QTest::newRow("Set a valid Paypal object via setter")
        // postRequest                    // expectedJson
        << postRequest                    << paypal.asJson()
        // expectedJsonValue
        << paypal.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << paypal.asJson()
        // getOneOfPostPaymentData_request_oneOf
        << SchemasModelsOneOf::PostPaymentData_request_oneOf()
        // getOneOfPostPaymentData_request_oneOf_1
        << paypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // Set an empty CreditCard object. CreditCard has required 'cardNumber',
    // so an empty credit card is INVALID => entire PostPaymentData_request stays invalid
    const SchemasModelsOneOf::PostPaymentData_request_oneOf emptyCreditCard;
    postRequest.setOneOfPostPaymentData_request_oneOf(emptyCreditCard);
    QTest::newRow("Set an empty CreditCard object via setter")
        // postRequest                    // expectedJson
        << postRequest                    << emptyCreditCard.asJson()
        // expectedJsonValue
        << emptyCreditCard.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1()
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << false                          << false;

    // Set an empty Paypal object. Paypal has no required fields,
    // so an empty paypal is VALID => entire PostPaymentData_request becomes valid
    const SchemasModelsOneOf::PostPaymentData_request_oneOf_1 emptyPaypal;
    postRequest.setOneOfPostPaymentData_request_oneOf_1(emptyPaypal);
    QTest::newRow("Set an empty Paypal object via setter")
        // postRequest                    // expectedJson
        << postRequest                    << emptyPaypal.asJson()
        // expectedJsonValue
        << emptyPaypal.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with EMPTY data.
    // Attempting to set an empty CreditCard via postRequest.fromJsonValue().
    // CreditCard defines required field 'cardNumber', so an empty object is INVALID
    // for CreditCard.
    //
    // However, emptyPaypal.asJsonValue() produces '{}' because it is an empty object.
    //
    // The PostPaymentData_request::fromJsonValue() logic cannot determine whether '{}'
    // represents a CreditCard or Paypal. It tries each schema:
    // - CreditCard: INVALID (missing required cardNumber)
    // - Paypal: VALID (no required fields)
    //
    // So '{}' resolves to Paypal (oneOf1).
    postRequest.fromJsonValue(emptyCreditCard.asJsonValue());
    QTest::newRow("Set an empty CreditCard via ::fromJsonValue(emptyCreditCard.asJsonValue())")
        // postRequest                    // expectedJson
        << postRequest                    << emptyCreditCard.asJson()
        // expectedJsonValue
        << emptyCreditCard.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // !!! NOTE: ambiguity resolved to Paypal (oneOf1) since CreditCard is invalid
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson with EMPTY data. Same expectations as above.
    postRequest.fromJson(emptyCreditCard.asJson());
    QTest::newRow("Set an empty CreditCard via ::fromJson(emptyCreditCard.asJson())")
        // postRequest                    // expectedJson
        << postRequest                    << emptyCreditCard.asJson()
        // expectedJsonValue
        << emptyCreditCard.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // ::fromJson with EMPTY Paypal data. Same expectations.
    postRequest.fromJson(emptyPaypal.asJson());
    QTest::newRow("Set an empty Paypal via ::fromJson(emptyPaypal.asJson())")
        // postRequest                    // expectedJson
        << postRequest                    << emptyPaypal.asJson()
        // expectedJsonValue
        << emptyPaypal.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    postRequest.fromJsonValue(emptyPaypal.asJsonValue());
    QTest::newRow("Set an empty Paypal via ::fromJsonValue(emptyPaypal.asJsonValue())")
        // postRequest                    // expectedJson
        << postRequest                    << emptyPaypal.asJson()
        // expectedJsonValue
        << emptyPaypal.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // check ::fromJson() works fine for VALID credit card object
    const QString validCard("{\"cardAvailability\":true,\"cardNumber\":\"1234-5678-9012-3456\"}"_L1);
    postRequest.fromJson(validCard);
    QTest::newRow("Set a valid CreditCard object via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << validCard
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << validCard                      << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << SchemasModelsOneOf::PostPaymentData_request_oneOf(validCard)
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << true
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << true                           << true;

    // check ::fromJsonValue() works fine for VALID credit card object
    const SchemasModelsOneOf::PostPaymentData_request_oneOf okCard(validCard);
    postRequest.fromJsonValue(okCard.asJsonValue());
    QTest::newRow("Set a valid CreditCard object via ::fromJsonValue()")
        // postRequest                    // expectedJson
        << postRequest                    << validCard
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << validCard                      << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << okCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << true
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << true                           << true;

    // check ::fromJson() works fine for VALID paypal object
    // Note:  an empty credit card (emptyCreditCard) is not valid and not set,
    // because it has a required field.
    const QString validPaypal("{\"paypalAvailability\":true,\"paypalEmail\":\"user@example.com\"}"_L1);
    postRequest.fromJson(validPaypal);
    QTest::newRow("Set a valid Paypal object via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << validPaypal
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << validPaypal
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1(validPaypal)
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // check ::fromJsonValue() works fine for VALID paypal object
    const SchemasModelsOneOf::PostPaymentData_request_oneOf_1 okPaypal(validPaypal);
    postRequest.fromJsonValue(okPaypal.asJsonValue());
    QTest::newRow("Set a valid Paypal object via ::fromJsonValue()")
        // postRequest                    // expectedJson
        << postRequest                    << validPaypal
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << validPaypal
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << okPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // invalid json! => the model is RESET to INITIAL state
    postRequest.fromJson("{invalid json}"_L1);
    QTest::newRow("Set an invalid json via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-object JSON (array) => the model is RESET to INITIAL state
    postRequest.fromJson("[]"_L1);
    QTest::newRow("Set an unexpected array [] via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-object JSON (string) => the model is RESET to INITIAL state
    postRequest.fromJson("\"just a string\""_L1);
    QTest::newRow("Set an unexpected string via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << false                          << false;

    // Missing Required Fields (CreditCard's 'cardNumber' is required)
    // CreditCard without required 'cardNumber' field - should not be valid
    SchemasModelsOneOf::PostPaymentData_request_oneOf invalidCard;
    invalidCard.setCardAvailability(true);
    postRequest.setOneOfPostPaymentData_request_oneOf(invalidCard);
    QTest::newRow("Set an invalid CreditCard via setter")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson                            // oneOf1ExpectedJson
        << QString("{\"cardAvailability\":true}"_L1)     << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << invalidCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // The status here reflects the status of the model that was set.
        // The model's field was set by setter, so invalidCard.isSet() == true
        // but invalidCard.isValid() == false, because required field is not set.
        // isOneOfPostPaymentDataRequestOneOfValid
        << invalidCard.isValid()
        // isOneOfPostPaymentDataRequestOneOfSet
        << invalidCard.isSet()
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << false                          << true;

    // Try to parse JSON missing required field for CreditCard.
    // cardNumber is required but missing => CreditCard is invalid.
    // Paypal has paypalAvailability (bool) but not cardAvailability,
    // so this doesn't match Paypal either since field name differs.
    postRequest.fromJson("{\"cardAvailability\":true}"_L1);
    QTest::newRow("Set an invalid CreditCard via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson                        // oneOf1ExpectedJson
        << QString("{\"cardAvailability\":true}"_L1) << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << SchemasModelsOneOf::PostPaymentData_request_oneOf("{\"cardAvailability\":true}"_L1)
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << false                          << true;

    // Partial/Incomplete JSON Objects
    // Only cardNumber for CreditCard (without cardAvailability),
    // should work since cardNumber is required and provided
    const QString cardOnly("{\"cardNumber\":\"9999-8888-7777-6666\"}"_L1);
    postRequest.fromJson(cardOnly);
    QTest::newRow("Set a valid non-full CreditCard via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << cardOnly
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << cardOnly                       << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << SchemasModelsOneOf::PostPaymentData_request_oneOf(cardOnly)
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << true
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << true                           << true;

    // Only paypalEmail for Paypal (without paypalAvailability),
    // should work since Paypal has no required fields
    const QString emailOnly("{\"paypalEmail\":\"test@test.com\"}"_L1);
    postRequest.fromJson(emailOnly);
    QTest::newRow("Set a valid non-full Paypal via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << emailOnly
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << emailOnly
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1(emailOnly)
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // Extra/Unknown Fields in JSON
    // JSON with fields not in schema - should parse known fields and ignore unknown
    postRequest.fromJson("{\"cardNumber\":\"4444-3333\",\"cardAvailability\":false,"
                         "\"unknown\":\"field\",\"extra\":123}"_L1);
    QString expected("{\"cardAvailability\":false,\"cardNumber\":\"4444-3333\"}"_L1);
    QTest::newRow("Extra/Unknown Fields in CreditCard JSON via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << expected
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << expected                       << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << SchemasModelsOneOf::PostPaymentData_request_oneOf(expected)
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << true
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << true                           << true;

    // Extra/Unknown Fields in Paypal JSON
    postRequest.fromJson("{\"paypalEmail\":\"a@b.com\",\"paypalAvailability\""
                         ":true,\"foo\":\"bar\"}"_L1);
    expected = QString("{\"paypalAvailability\":true,\"paypalEmail\":\"a@b.com\"}"_L1);
    QTest::newRow("Extra/Unknown Fields in Paypal JSON via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << expected
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << expected
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1(expected)
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // Empty String in CreditCard cardNumber field
    // cardNumber is required and present (even though empty string), so it's valid
    expected = QString("{\"cardNumber\":\"\"}"_L1);
    postRequest.fromJson(expected);
    QTest::newRow("An empty String in CreditCard cardNumber field via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << expected
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << expected                       << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << SchemasModelsOneOf::PostPaymentData_request_oneOf(expected)
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << true
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << true                           << true;

    // Invalid Field Types for CreditCard. Wrong type for cardNumber (integer instead of string)
    // The CreditCard is set, but invalid, because the required field is ommited due to invalid
    // data. The entire model returns {}, the getter returns the data was valid on setting:
    // {\"cardAvailability\":true}
    expected = QString("{\"cardNumber\":12345,\"cardAvailability\":true}"_L1);
    postRequest.fromJson(expected);
    QTest::newRow("Invalid field 'cardNumber=12345' in CreditCard via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson                          // oneOf1ExpectedJson
        << QString("{\"cardAvailability\":true}"_L1)   << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << SchemasModelsOneOf::PostPaymentData_request_oneOf(expected)
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << false                          << true;

    // Invalid Field Types for CreditCard. Wrong type for cardAvailability (string instead of bool)
    expected = QString("{\"cardNumber\":\"5555\",\"cardAvailability\":\"yes\"}"_L1);
    postRequest.fromJson(expected);
    QTest::newRow("Invalid field 'cardAvailability=yes' in CreditCard via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{\"cardNumber\":\"5555\"}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson                      // oneOf1ExpectedJson
        << QString("{\"cardNumber\":\"5555\"}"_L1) << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << SchemasModelsOneOf::PostPaymentData_request_oneOf(expected)
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << true
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << true                           << true;

    // Invalid Field Types for Paypal. Wrong type for paypalAvailability (integer instead of bool)
    // Note: paypalAvailability is optional, so wrong type is just omitted
    expected = QString("{\"paypalEmail\":\"x@y.com\",\"paypalAvailability\":1}"_L1);
    postRequest.fromJson(expected);
    QTest::newRow("Invalid field 'paypalAvailability=1' in Paypal via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{\"paypalEmail\":\"x@y.com\"}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{\"paypalEmail\":\"x@y.com\"}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1(expected)
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // Invalid Field Types for Paypal. Wrong type for paypalEmail (bool instead of string)
    // Note: paypalEmail is optional, so wrong type is just omitted
    expected = QString("{\"paypalEmail\":true,\"paypalAvailability\":false}"_L1);
    postRequest.fromJson(expected);
    QTest::newRow("Invalid field 'paypalEmail=true' in Paypal via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{\"paypalAvailability\":false}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{\"paypalAvailability\":false}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1(expected)
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // All fields have wrong types - both oneOfs fail to parse valid data
    expected = QString("{\"cardNumber\":true,\"cardAvailability\":\"yes\"}"_L1);
    postRequest.fromJson(expected);
    QTest::newRow("All invalid fields in CreditCard via ::fromJson()")
        // postRequest                    // expectedJson
        << postRequest                    << QString("{}"_L1)
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << false                          << false;

    // CreditCard with only required field set via setter
    SchemasModelsOneOf::PostPaymentData_request_oneOf cardOnly2;
    cardOnly2.setCardNumber("0000-1111-2222-3333"_L1);
    postRequest.setOneOfPostPaymentData_request_oneOf(cardOnly2);
    QTest::newRow("Set CreditCard with only cardNumber via setter")
        // postRequest                    // expectedJson
        << postRequest                    << cardOnly2.asJson()
        // expectedJsonValue
        << cardOnly2.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << cardOnly2.asJson()             << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << cardOnly2
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << true
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << true                           << true;

    // Paypal with only paypalAvailability set via setter
    SchemasModelsOneOf::PostPaymentData_request_oneOf_1 availOnly;
    availOnly.setPaypalAvailability(false);
    postRequest.setOneOfPostPaymentData_request_oneOf_1(availOnly);
    QTest::newRow("Set Paypal with only paypalAvailability via setter")
        // postRequest                    // expectedJson
        << postRequest                    << availOnly.asJson()
        // expectedJsonValue
        << availOnly.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << availOnly.asJson()
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << availOnly
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with valid CreditCard data
    postRequest.fromJsonValue(creditCard.asJsonValue());
    QTest::newRow("Set a valid CreditCard via ::fromJsonValue(creditCard.asJsonValue())")
        // postRequest                    // expectedJson
        << postRequest                    << creditCard.asJson()
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << creditCard.asJson()            << QString("{}"_L1)
        // getOneOfPostPaymentData_request_oneOf
        << creditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << emptyPaypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << true
        // isOneOfPostPaymentDataRequestOneOfSet
        << true
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << false
        // isOneOfPostPaymentDataRequestOneOf1Set
        << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with valid Paypal data
    postRequest.fromJsonValue(paypal.asJsonValue());
    QTest::newRow("Set a valid Paypal via ::fromJsonValue(paypal.asJsonValue())")
        // postRequest                    // expectedJson
        << postRequest                    << paypal.asJson()
        // expectedJsonValue
        << postRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << paypal.asJson()
        // getOneOfPostPaymentData_request_oneOf
        << emptyCreditCard
        // getOneOfPostPaymentData_request_oneOf_1
        << paypal
        // isOneOfPostPaymentDataRequestOneOfValid
        << false
        // isOneOfPostPaymentDataRequestOneOfSet
        << false
        // isOneOfPostPaymentDataRequestOneOf1Valid
        << true
        // isOneOfPostPaymentDataRequestOneOf1Set
        << true
        // isValid                        // isSet
        << true                           << true;
}

void OneOfTest::testInlineSchemasJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::PostPaymentData_request, postRequest);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(QString, oneOf0ExpectedJson);
    QFETCH(QString, oneOf1ExpectedJson);
    QFETCH(SchemasModelsOneOf::PostPaymentData_request_oneOf,
           getOneOfPostPaymentData_request_oneOf);
    QFETCH(SchemasModelsOneOf::PostPaymentData_request_oneOf_1,
           getOneOfPostPaymentData_request_oneOf_1);
    QFETCH(bool, isOneOfPostPaymentDataRequestOneOfValid);
    QFETCH(bool, isOneOfPostPaymentDataRequestOneOfSet);
    QFETCH(bool, isOneOfPostPaymentDataRequestOneOf1Valid);
    QFETCH(bool, isOneOfPostPaymentDataRequestOneOf1Set);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(postRequest.asJson(), expectedJson);
    QCOMPARE(postRequest.asJsonValue(), expectedJsonValue);
    QCOMPARE(postRequest.getOneOfPostPaymentData_request_oneOf().asJson(),
             oneOf0ExpectedJson);
    QCOMPARE(postRequest.isOneOfPostPaymentDataRequestOneOfValid(),
             isOneOfPostPaymentDataRequestOneOfValid);
    QCOMPARE(postRequest.isOneOfPostPaymentDataRequestOneOfSet(),
             isOneOfPostPaymentDataRequestOneOfSet);
    QCOMPARE(postRequest.getOneOfPostPaymentData_request_oneOf_1().asJson(),
             oneOf1ExpectedJson);
    QCOMPARE(postRequest.isOneOfPostPaymentDataRequestOneOf1Valid(),
             isOneOfPostPaymentDataRequestOneOf1Valid);
    QCOMPARE(postRequest.isOneOfPostPaymentDataRequestOneOf1Set(),
             isOneOfPostPaymentDataRequestOneOf1Set);
    QCOMPARE(postRequest.getOneOfPostPaymentData_request_oneOf(),
             getOneOfPostPaymentData_request_oneOf);
    QCOMPARE(postRequest.getOneOfPostPaymentData_request_oneOf_1(),
             getOneOfPostPaymentData_request_oneOf_1);
    QCOMPARE(postRequest.isValid(), isValid);
    QCOMPARE(postRequest.isSet(), isSet);
}

void OneOfTest::testInlineSchemasPatch_data()
{
    QTest::addColumn<SchemasModelsOneOf::PatchPaymentData_request>("patchPaymentRequest");
    QTest::addColumn<QString>("stringValue");
    QTest::addColumn<bool>("boolValue");
    QTest::addColumn<double>("doubleValue");
    QTest::addColumn<OneOfTest::BankApiPatchMode>("mode");
    QTest::addColumn<QString>("expectedJson");

    // Empty patchPaymentRequest
    QTest::newRow("empty PatchPaymentData_request")
        << SchemasModelsOneOf::PatchPaymentData_request{}
        << QString()
        << false
        << 0.0
        << OneOfTest::BankApiPatchMode::UsePatchPaymentRequest
        << QString("{}"_L1);

    // Test patchPaymentData with string value
    QTest::newRow("string value")
        << SchemasModelsOneOf::PatchPaymentData_request{}
        << QString("test-string-value"_L1)
        << false
        << 0.0
        << OneOfTest::BankApiPatchMode::UseString
        << QString("\"test-string-value\""_L1);

    // Test patchPaymentData with bool value
    QTest::newRow("bool value true")
        << SchemasModelsOneOf::PatchPaymentData_request{}
        << QString()
        << true
        << 0.0
        << OneOfTest::BankApiPatchMode::UseBool
        << QString("true"_L1);

    QTest::newRow("bool value false")
        << SchemasModelsOneOf::PatchPaymentData_request{}
        << QString()
        << false
        << 0.0
        << OneOfTest::BankApiPatchMode::UseBool
        << QString("false"_L1);

    // Test patchPaymentData with double value
    QTest::newRow("double value")
        << SchemasModelsOneOf::PatchPaymentData_request{}
        << QString()
        << false
        << 42.5
        << OneOfTest::BankApiPatchMode::UseDouble
        << QString("42.5"_L1);

    // Test PatchPaymentData_request with string set
    SchemasModelsOneOf::PatchPaymentData_request patchRequest;
    patchRequest.setOneOfQString("request-string"_L1);
    QTest::newRow("PatchPaymentData_request with string")
        << patchRequest
        << QString()
        << false
        << 0.0
        << OneOfTest::BankApiPatchMode::UsePatchPaymentRequest
        << QString("\"request-string\""_L1);

    // Test PatchPaymentData_request with bool set
    patchRequest.setOneOfBool(true);

    QTest::newRow("PatchPaymentData_request with bool")
        << patchRequest
        << QString()
        << false
        << 0.0
        << OneOfTest::BankApiPatchMode::UsePatchPaymentRequest
        << QString("true"_L1);

    // Test PatchPaymentData_request with double set
    patchRequest.setOneOfDouble(99.99);

    QTest::newRow("PatchPaymentData_request with double")
        << patchRequest
        << QString()
        << false
        << 0.0
        << OneOfTest::BankApiPatchMode::UsePatchPaymentRequest
        << QString("99.99"_L1);
}

void OneOfTest::testInlineSchemasPatch()
{
    QFETCH(SchemasModelsOneOf::PatchPaymentData_request, patchPaymentRequest);
    QFETCH(QString, stringValue);
    QFETCH(bool, boolValue);
    QFETCH(double, doubleValue);
    QFETCH(OneOfTest::BankApiPatchMode, mode);
    QFETCH(QString, expectedJson);

    bool done = true;
    SchemasModelsOneOf::BankApi bankApi;
    LoggingNetworkAccessManager logging(&bankApi);
    QRestAccessManager restManager(&logging, &bankApi);
    bankApi.setRestAccessManager(&restManager);

    switch (mode) {
    case OneOfTest::BankApiPatchMode::UsePatchPaymentRequest:
    {
        bankApi.patchPaymentData(patchPaymentRequest, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::BankApiPatchMode::UseString:
    {
        bankApi.patchPaymentData(stringValue, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::BankApiPatchMode::UseBool:
    {
        bankApi.patchPaymentData(boolValue, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::BankApiPatchMode::UseDouble:
    {
        bankApi.patchPaymentData(doubleValue, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    }
    QCOMPARE(logging.m_content, expectedJson);
    QTRY_COMPARE_EQ(done, false);
}

void OneOfTest::testInlineSchemasPost_data()
{
    QTest::addColumn<SchemasModelsOneOf::PostPaymentData_request>("postPaymentRequest");
    QTest::addColumn<SchemasModelsOneOf::PostPaymentData_request_oneOf>("creditCard");
    QTest::addColumn<SchemasModelsOneOf::PostPaymentData_request_oneOf_1>("paypal");
    QTest::addColumn<OneOfTest::BankApiPostMode>("mode");
    QTest::addColumn<QString>("expectedJson");

    // Empty postPaymentRequest
    QTest::newRow("empty PostPaymentData_request")
        << SchemasModelsOneOf::PostPaymentData_request{}
        << SchemasModelsOneOf::PostPaymentData_request_oneOf{}
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1{}
        << OneOfTest::BankApiPostMode::UsePostPaymentRequest
        << QString("{}"_L1);

    // Test Credit Card payment with required field
    SchemasModelsOneOf::PostPaymentData_request_oneOf creditCard;
    creditCard.setCardNumber("1234-5678-9012-3456"_L1);
    creditCard.setCardAvailability(true);

    QTest::newRow("credit card with required field")
        << SchemasModelsOneOf::PostPaymentData_request{}
        << creditCard
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1{}
        << OneOfTest::BankApiPostMode::UseCreditCard
        << QString("{\"cardAvailability\":true,\"cardNumber\":\"1234-5678-9012-3456\"}"_L1);

    // Test PayPal payment
    SchemasModelsOneOf::PostPaymentData_request_oneOf_1 paypal;
    paypal.setPaypalEmail("user@example.com"_L1);
    paypal.setPaypalAvailability(true);

    QTest::newRow("paypal payment")
        << SchemasModelsOneOf::PostPaymentData_request{}
        << SchemasModelsOneOf::PostPaymentData_request_oneOf{}
        << paypal
        << OneOfTest::BankApiPostMode::UsePaypal
        << QString("{\"paypalAvailability\":true,\"paypalEmail\":\"user@example.com\"}"_L1);

    // Test PostPaymentData_request with credit card set
    SchemasModelsOneOf::PostPaymentData_request postRequest;
    postRequest.setOneOfPostPaymentData_request_oneOf(creditCard);

    QTest::newRow("PostPaymentData_request with credit card")
        << postRequest
        << SchemasModelsOneOf::PostPaymentData_request_oneOf{}
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1{}
        << OneOfTest::BankApiPostMode::UsePostPaymentRequest
        << QString("{\"cardAvailability\":true,\"cardNumber\":\"1234-5678-9012-3456\"}"_L1);

    // Test PostPaymentData_request with PayPal set
    postRequest.setOneOfPostPaymentData_request_oneOf_1(paypal);

    QTest::newRow("PostPaymentData_request with paypal")
        << postRequest
        << SchemasModelsOneOf::PostPaymentData_request_oneOf{}
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1{}
        << OneOfTest::BankApiPostMode::UsePostPaymentRequest
        << QString("{\"paypalAvailability\":true,\"paypalEmail\":\"user@example.com\"}"_L1);

    // Test credit card without required field (cardNumber is required)
    SchemasModelsOneOf::PostPaymentData_request_oneOf invalidCreditCard;
    invalidCreditCard.setCardAvailability(true);

    QTest::newRow("invalid credit card missing required field")
        << SchemasModelsOneOf::PostPaymentData_request{}
        << invalidCreditCard
        << SchemasModelsOneOf::PostPaymentData_request_oneOf_1{}
        << OneOfTest::BankApiPostMode::UseCreditCard
        << QString("{}"_L1);

    // Test PayPal with only email (paypalAvailability is optional)
    SchemasModelsOneOf::PostPaymentData_request_oneOf_1 partialPaypal;
    partialPaypal.setPaypalEmail("partial@example.com"_L1);

    QTest::newRow("paypal with only email")
        << SchemasModelsOneOf::PostPaymentData_request{}
        << SchemasModelsOneOf::PostPaymentData_request_oneOf{}
        << partialPaypal
        << OneOfTest::BankApiPostMode::UsePaypal
        << QString("{\"paypalEmail\":\"partial@example.com\"}"_L1);
}

void OneOfTest::testInlineSchemasPost()
{
    QFETCH(SchemasModelsOneOf::PostPaymentData_request, postPaymentRequest);
    QFETCH(SchemasModelsOneOf::PostPaymentData_request_oneOf, creditCard);
    QFETCH(SchemasModelsOneOf::PostPaymentData_request_oneOf_1, paypal);
    QFETCH(OneOfTest::BankApiPostMode, mode);
    QFETCH(QString, expectedJson);

    bool done = true;
    SchemasModelsOneOf::BankApi bankApi;
    LoggingNetworkAccessManager logging(&bankApi);
    QRestAccessManager restManager(&logging, &bankApi);
    bankApi.setRestAccessManager(&restManager);

    switch (mode) {
    case OneOfTest::BankApiPostMode::UsePostPaymentRequest:
    {
        bankApi.postPaymentData(postPaymentRequest, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::BankApiPostMode::UseCreditCard:
    {
        bankApi.postPaymentData(creditCard, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::BankApiPostMode::UsePaypal:
    {
        bankApi.postPaymentData(paypal, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    }
    QCOMPARE(logging.m_content, expectedJson);
    QTRY_COMPARE_EQ(done, false);
}

void OneOfTest::testOneOfPrimitiveJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::PatchPaymentData_request>("patchRequest");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<QString>("getOneOfQString");
    QTest::addColumn<bool>("getOneOfBool");
    QTest::addColumn<double>("getOneOfDouble");
    QTest::addColumn<bool>("isOneOfQStringValid");
    QTest::addColumn<bool>("isOneOfQStringSet");
    QTest::addColumn<bool>("isOneOfBoolValid");
    QTest::addColumn<bool>("isOneOfBoolSet");
    QTest::addColumn<bool>("isOneOfDoubleValid");
    QTest::addColumn<bool>("isOneOfDoubleSet");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    // PatchPaymentData_request has three primitive oneOf alternatives:
    // oneOf0: string
    // oneOf1: boolean
    // oneOf2: number (double)
    // All primitives have no "required fields" concept, so they are always "valid".
    SchemasModelsOneOf::PatchPaymentData_request patchRequest;
    QTest::newRow("Empty PatchPaymentData_request object")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set a string value via setter
    const QString hello("hello-world"_L1);
    patchRequest.setOneOfQString(hello);
    QTest::newRow("Set a valid string via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("\"hello-world\""_L1)
        // expectedJsonValue
        << QJsonValue(hello)
        // getOneOfQString                // getOneOfBool
        << hello                          << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << true                           << true
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a bool value via setter (overrides string)
    patchRequest.setOneOfBool(true);
    QTest::newRow("Set a valid bool 'true' via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("true"_L1)
        // expectedJsonValue
        << QJsonValue(true)
        // getOneOfQString                // getOneOfBool
        << QString()                      << true
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << true                           << true
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a bool 'false' value via setter
    patchRequest.setOneOfBool(false);
    QTest::newRow("Set a valid bool 'false' via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("false"_L1)
        // expectedJsonValue
        << QJsonValue(false)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << true                           << true
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a double value via setter (overrides bool)
    patchRequest.setOneOfDouble(42.5);
    QTest::newRow("Set a valid double '42.5' via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("42.5"_L1)
        // expectedJsonValue
        << QJsonValue(42.5)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 42.5
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set a negative double value via setter
    patchRequest.setOneOfDouble(-99.99);
    QTest::newRow("Set a valid double '-99.99' via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("-99.99"_L1)
        // expectedJsonValue
        << QJsonValue(-99.99)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << -99.99
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set a zero double value via setter
    patchRequest.setOneOfDouble(0.0);
    QTest::newRow("Set a valid double '0.0' via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("0"_L1)
        // expectedJsonValue
        << QJsonValue(0.0)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set an empty string via setter
    patchRequest.setOneOfQString("");
    QTest::newRow("Set an empty string via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("\"\""_L1)
        // expectedJsonValue
        << QJsonValue(""_L1)
        // getOneOfQString                // getOneOfBool
        << QString(""_L1)                 << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << true                           << true
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a string value
    patchRequest.fromJson("\"payment-token-abc\""_L1);
    QTest::newRow("Set a valid string via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("\"payment-token-abc\""_L1)
        // expectedJsonValue
        << QJsonValue("payment-token-abc"_L1)
        // getOneOfQString                // getOneOfBool
        << QString("payment-token-abc"_L1) << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << true                           << true
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a bool 'true' value
    patchRequest.fromJson("true"_L1);
    QTest::newRow("Set a valid bool 'true' via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("true"_L1)
        // expectedJsonValue
        << QJsonValue(true)
        // getOneOfQString                // getOneOfBool
        << QString()                      << true
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << true                           << true
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a bool 'false' value
    patchRequest.fromJson("false"_L1);
    QTest::newRow("Set a valid bool 'false' via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("false"_L1)
        // expectedJsonValue
        << QJsonValue(false)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << true                           << true
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a double value
    patchRequest.fromJson("3.14159"_L1);
    QTest::newRow("Set a valid double '3.14159' via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("3.14159"_L1)
        // expectedJsonValue
        << QJsonValue(3.14159)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 3.14159
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a negative double value
    patchRequest.fromJson("-100.5"_L1);
    QTest::newRow("Set a valid double '-100.5' via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("-100.5"_L1)
        // expectedJsonValue
        << QJsonValue(-100.5)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << -100.5
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson with an integer value (should match double/number)
    patchRequest.fromJson("42"_L1);
    QTest::newRow("Set an integer '42' via ::fromJson() (matches double)")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("42"_L1)
        // expectedJsonValue
        << QJsonValue(42.0)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 42.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with a string value
    patchRequest.fromJsonValue(QJsonValue("json-value-string"_L1));
    QTest::newRow("Set a valid string via ::fromJsonValue()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("\"json-value-string\""_L1)
        // expectedJsonValue
        << QJsonValue("json-value-string"_L1)
        // getOneOfQString                // getOneOfBool
        << QString("json-value-string"_L1) << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << true                           << true
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with a bool value
    patchRequest.fromJsonValue(QJsonValue(true));
    QTest::newRow("Set a valid bool 'true' via ::fromJsonValue()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("true"_L1)
        // expectedJsonValue
        << QJsonValue(true)
        // getOneOfQString                // getOneOfBool
        << QString()                      << true
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << true                           << true
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with a double value
    patchRequest.fromJsonValue(QJsonValue(99.99));
    QTest::newRow("Set a valid double '99.99' via ::fromJsonValue()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("99.99"_L1)
        // expectedJsonValue
        << QJsonValue(99.99)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 99.99
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // invalid json! => the model is RESET to INITIAL state
    patchRequest.fromJson("{invalid json}"_L1);
    QTest::newRow("Set an invalid json via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-primitive JSON (object) => doesn't match any primitive type
    patchRequest.fromJson("{\"key\":\"value\"}"_L1);
    QTest::newRow("Set an unexpected object via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-primitive JSON (array) => doesn't match any primitive type
    patchRequest.fromJson("[1, 2, 3]"_L1);
    QTest::newRow("PatchPaymentData_request: set an unexpected array via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with null JSON value => doesn't match string/bool/double
    patchRequest.fromJson("null"_L1);
    QTest::newRow("Set a null value via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJsonValue with null QJsonValue => doesn't match primitives
    patchRequest.fromJsonValue(QJsonValue());
    QTest::newRow("Set a null QJsonValue via ::fromJsonValue()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJsonValue with QJsonObject => doesn't match primitives
    patchRequest.fromJsonValue(QJsonValue(QJsonObject()));
    QTest::newRow("Set an empty QJsonObject via ::fromJsonValue()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson with an empty string JSON => should match string (oneOf0)
    patchRequest.fromJson("\"\""_L1);
    QTest::newRow("Set an empty string via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("\"\""_L1)
        // expectedJsonValue
        << QJsonValue(""_L1)
        // getOneOfQString                // getOneOfBool
        << QString(""_L1)                 << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << true                           << true
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a very large double
    patchRequest.fromJson("1.7976931348623157e+308"_L1);
    QTest::newRow("Set a very large double via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("1.7976931348623157e+308"_L1)
        // expectedJsonValue
        << QJsonValue(1.7976931348623157e+308)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 1.7976931348623157e+308
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a string that looks like a number (but is quoted => string)
    patchRequest.fromJson("\"42.5\""_L1);
    QTest::newRow("Set a string '42.5' (quoted) via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("\"42.5\""_L1)
        // expectedJsonValue
        << QJsonValue("42.5"_L1)
        // getOneOfQString                // getOneOfBool
        << QString("42.5"_L1)             << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << true                           << true
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with a string that looks like a bool (but is quoted => string)
    patchRequest.fromJson("\"true\""_L1);
    QTest::newRow("Set a string 'true' (quoted) via ::fromJson()")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("\"true\""_L1)
        // expectedJsonValue
        << QJsonValue("true"_L1)
        // getOneOfQString                // getOneOfBool
        << QString("true"_L1)             << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << true                           << true
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with zero (integer form) => matches double
    patchRequest.fromJson("0"_L1);
    QTest::newRow("Set zero '0' via ::fromJson() (matches double)")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("0"_L1)
        // expectedJsonValue
        << QJsonValue(0.0)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set string with special characters via setter
    patchRequest.setOneOfQString("hello \"world\" \n\ttab"_L1);
    QTest::newRow("Set a string with special characters via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("\"hello \\\"world\\\" \\n\\ttab\""_L1)
        // expectedJsonValue
        << QJsonValue("hello \"world\" \n\ttab"_L1)
        // getOneOfQString                // getOneOfBool
        << QString("hello \"world\" \n\ttab"_L1) << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << true                           << true
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set double via setter then override with string via setter
    patchRequest.setOneOfDouble(123.456);
    patchRequest.setOneOfQString("override"_L1);
    QTest::newRow("Override double with string via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("\"override\""_L1)
        // expectedJsonValue
        << QJsonValue("override"_L1)
        // getOneOfQString                // getOneOfBool
        << QString("override"_L1)         << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << true                           << true
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set string via setter then override with bool via setter
    patchRequest.setOneOfQString("will be overridden"_L1);
    patchRequest.setOneOfBool(false);
    QTest::newRow("Override string with bool 'false' via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("false"_L1)
        // expectedJsonValue
        << QJsonValue(false)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.0
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << true                           << true
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set bool via setter then override with double via setter
    patchRequest.setOneOfBool(true);
    patchRequest.setOneOfDouble(0.001);
    QTest::newRow("Override bool with double '0.001' via setter")
        // patchRequest                   // expectedJson
        << patchRequest                   << QString("0.001"_L1)
        // expectedJsonValue
        << QJsonValue(0.001)
        // getOneOfQString                // getOneOfBool
        << QString()                      << false
        // getOneOfDouble
        << 0.001
        // isOneOfQStringValid            // isOneOfQStringSet
        << false                          << false
        // isOneOfBoolValid               // isOneOfBoolSet
        << false                          << false
        // isOneOfDoubleValid             // isOneOfDoubleSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;
}

void OneOfTest::testOneOfPrimitiveJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::PatchPaymentData_request, patchRequest);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(QString, getOneOfQString);
    QFETCH(bool, getOneOfBool);
    QFETCH(double, getOneOfDouble);
    QFETCH(bool, isOneOfQStringValid);
    QFETCH(bool, isOneOfQStringSet);
    QFETCH(bool, isOneOfBoolValid);
    QFETCH(bool, isOneOfBoolSet);
    QFETCH(bool, isOneOfDoubleValid);
    QFETCH(bool, isOneOfDoubleSet);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(patchRequest.asJson(), expectedJson);
    QCOMPARE(patchRequest.asJsonValue(), expectedJsonValue);
    QCOMPARE(patchRequest.getOneOfQString(), getOneOfQString);
    QCOMPARE(patchRequest.isOneOfQStringValid(), isOneOfQStringValid);
    QCOMPARE(patchRequest.isOneOfQStringSet(), isOneOfQStringSet);
    QCOMPARE(patchRequest.getOneOfBool(), getOneOfBool);
    QCOMPARE(patchRequest.isOneOfBoolValid(), isOneOfBoolValid);
    QCOMPARE(patchRequest.isOneOfBoolSet(), isOneOfBoolSet);
    QCOMPARE(patchRequest.getOneOfDouble(), getOneOfDouble);
    QCOMPARE(patchRequest.isOneOfDoubleValid(), isOneOfDoubleValid);
    QCOMPARE(patchRequest.isOneOfDoubleSet(), isOneOfDoubleSet);
    QCOMPARE(patchRequest.isValid(), isValid);
    QCOMPARE(patchRequest.isSet(), isSet);
}

void OneOfTest::testPolymorphedRequestBody_data()
{
    QTest::addColumn<SchemasModelsOneOf::PostFarmPet_request>("pet");
    QTest::addColumn<SchemasModelsOneOf::Dog>("dog");
    QTest::addColumn<SchemasModelsOneOf::Duck>("duck");
    QTest::addColumn<SchemasModelsOneOf::Bunny>("bunny");
    QTest::addColumn<OneOfTest::FarmApiMode>("mode");
    QTest::addColumn<QString>("expectedJson");

    // Empty
    QTest::newRow("empty duck, bunny, dog and farm pet")
        << SchemasModelsOneOf::PostFarmPet_request{}
        << SchemasModelsOneOf::Dog{}
        << SchemasModelsOneOf::Duck{}
        << SchemasModelsOneOf::Bunny{}
        << OneOfTest::FarmApiMode::UsePostPet
        << QString("{}"_L1);

    SchemasModelsOneOf::PostFarmPet_request model;
    SchemasModelsOneOf::Duck duck;
    duck.setHunts(true);
    duck.setAge(9);

    SchemasModelsOneOf::Bunny bunny;
    bunny.setBreedType("Wild"_L1);
    bunny.setWeight(5);

    QTest::newRow("Set Bunny object")
        << SchemasModelsOneOf::PostFarmPet_request{}
        << SchemasModelsOneOf::Dog{}
        << SchemasModelsOneOf::Duck{}
        << bunny
        << OneOfTest::FarmApiMode::UseBunny
        << QString("{\"breed-type\":\"Wild\",\"weight\":5}"_L1);

    QTest::newRow("Set Duck object")
        << SchemasModelsOneOf::PostFarmPet_request{}
        << SchemasModelsOneOf::Dog{}
        << duck
        << SchemasModelsOneOf::Bunny{}
        << OneOfTest::FarmApiMode::UseDuck
        << QString("{\"age\":9,\"hunts\":true}"_L1);

    model.setOneOfDuck(duck);
    QTest::newRow("Set PostFarmPet_request object with duck data")
        << model
        << SchemasModelsOneOf::Dog{}
        << SchemasModelsOneOf::Duck{}
        << SchemasModelsOneOf::Bunny{}
        << OneOfTest::FarmApiMode::UsePostPet
        << QString("{\"age\":9,\"hunts\":true}"_L1);

    model.setOneOfBunny(bunny);
    QTest::newRow("Set PostFarmPet_request object with bunny data")
        << model
        << SchemasModelsOneOf::Dog{}
        << SchemasModelsOneOf::Duck{}
        << SchemasModelsOneOf::Bunny{}
        << OneOfTest::FarmApiMode::UsePostPet
        << QString("{\"breed-type\":\"Wild\",\"weight\":5}"_L1);

    model.setOneOfBunny(SchemasModelsOneOf::Bunny{});
    QTest::newRow("Reset PostFarmPet_request object to empty")
        << model
        << SchemasModelsOneOf::Dog{}
        << SchemasModelsOneOf::Duck{}
        << SchemasModelsOneOf::Bunny{}
        << OneOfTest::FarmApiMode::UsePostPet
        << QString("{}"_L1);

    SchemasModelsOneOf::Dog dog;
    dog.setBreed("Dingo"_L1);
    // Should fail because bark is not set, but bark is required
    QTest::newRow("Set incomplete Dog() object")
        << SchemasModelsOneOf::PostFarmPet_request{}
        << dog
        << SchemasModelsOneOf::Duck{}
        << SchemasModelsOneOf::Bunny{}
        << OneOfTest::FarmApiMode::UseDog
        << QString("{}"_L1);

    dog.setBark(true);
    QTest::newRow("Set full Dog() object")
        << SchemasModelsOneOf::PostFarmPet_request{}
        << dog
        << SchemasModelsOneOf::Duck{}
        << SchemasModelsOneOf::Bunny{}
        << OneOfTest::FarmApiMode::UseDog
        << QString("{\"bark\":true,\"breed\":\"Dingo\"}"_L1);
}

void OneOfTest::testPolymorphedRequestBody()
{
    QFETCH(SchemasModelsOneOf::PostFarmPet_request, pet);
    QFETCH(SchemasModelsOneOf::Dog, dog);
    QFETCH(SchemasModelsOneOf::Duck, duck);
    QFETCH(SchemasModelsOneOf::Bunny, bunny);
    QFETCH(OneOfTest::FarmApiMode, mode);
    QFETCH(QString, expectedJson);

    bool done = true;
    SchemasModelsOneOf::FarmApi farmApi;
    LoggingNetworkAccessManager logging(&farmApi);
    QRestAccessManager restManager(&logging, &farmApi);
    farmApi.setRestAccessManager(&restManager);

    switch (mode) {
    case OneOfTest::FarmApiMode::UsePostPet:
    {
        farmApi.postFarmPet(pet, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::FarmApiMode::UseDuck:
    {
        farmApi.postFarmPet(duck, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::FarmApiMode::UseBunny:
    {
        farmApi.postFarmPet(bunny, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    case OneOfTest::FarmApiMode::UseDog:
    {
        farmApi.postFarmPet(dog, this, [&](const QRestReply &reply) {
            done = reply.isSuccess();
        });
    } break;
    }
    QCOMPARE(logging.m_content, expectedJson);
    QTRY_COMPARE_EQ(done, false);
}

void OneOfTest::testPostAccountRequestJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::PostAccount_request>("postAccountRequest");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<QString>("oneOf0ExpectedJson");
    QTest::addColumn<QString>("oneOf1ExpectedJson");
    QTest::addColumn<bool>("isOneOf0Valid");
    QTest::addColumn<bool>("isOneOf0Set");
    QTest::addColumn<bool>("isOneOf1Valid");
    QTest::addColumn<bool>("isOneOf1Set");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    // PostAccount_request has two oneOf alternatives:
    // oneOf0: Account (which is itself a oneOf - see testAccountJsonConversionMethods)
    // oneOf1: QMap<QString, QJsonValue> (object with additionalProperties: true)
    //
    // An empty Account is a oneOf with nothing set => Account.isValid() = false
    // An empty QMap is valid for additionalProperties: true (no required properties)
    SchemasModelsOneOf::PostAccount_request postAccountRequest;
    QTest::newRow("Empty PostAccount_request object")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set a valid Account via setter (Account with Account_oneOf set)
    SchemasModelsOneOf::Account validAccount;
    SchemasModelsOneOf::Account_oneOf acctData;
    acctData.setName("Test Corp"_L1);
    acctData.setContractAvailable(true);
    validAccount.setOneOfAccount_oneOf(acctData);
    postAccountRequest.setOneOfAccount(validAccount);
    QTest::newRow("Set a valid Account (with Account_oneOf) via setter")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << validAccount.asJson()
        // expectedJsonValue
        << validAccount.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << validAccount.asJson()          << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a valid Account via setter (Account with Account_oneOf_1 set)
    SchemasModelsOneOf::Account validAccount2;
    SchemasModelsOneOf::Account_oneOf_1 acctData2;
    acctData2.setFirstname("Jane"_L1);
    acctData2.setLastname("Doe"_L1);
    validAccount2.setOneOfAccount_oneOf_1(acctData2);
    postAccountRequest.setOneOfAccount(validAccount2);
    QTest::newRow("Set a valid Account (with Account_oneOf_1) via setter")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << validAccount2.asJson()
        // expectedJsonValue
        << validAccount2.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << validAccount2.asJson()          << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a QMap (additionalProperties object) via setter
    QMap<QString, QJsonValue> freeformObj;
    freeformObj.insert("customField1"_L1, QJsonValue("value1"_L1));
    freeformObj.insert("customField2"_L1, QJsonValue(42));
    freeformObj.insert("nested"_L1, QJsonValue(true));
    postAccountRequest.setOneOfQMapQStringQJsonValue(freeformObj);
    const QString expectedTestString(
        "{\"customField1\":\"value1\",\"customField2\":42,\"nested\":true}"_L1);
    QTest::newRow("Set a valid QMap via setter")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << expectedTestString
        // expectedJsonValue
        << postAccountRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << expectedTestString
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set an empty QMap via setter - empty map should still be valid
    const QMap<QString, QJsonValue> emptyMap;
    postAccountRequest.setOneOfQMapQStringQJsonValue(emptyMap);
    QTest::newRow("Set an empty QMap via setter")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << postAccountRequest.asJson()
        // expectedJsonValue
        << postAccountRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set an empty Account (which is itself invalid - no alternative selected)
    // NOTE: Account itself is also a OneOf type object! If it is empty, it doesn't
    // hold any type inside and cannot be valid.
    const SchemasModelsOneOf::Account emptyAccount;
    postAccountRequest.setOneOfAccount(emptyAccount);
    QTest::newRow("Set an empty Account via setter")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << emptyAccount.asJson()
        // expectedJsonValue
        << emptyAccount.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson with Account_oneOf data (name field identifies it)
    const QString acctJson("{\"contractAvailable\":true,\"name\":\"FromJson Corp\"}"_L1);
    postAccountRequest.fromJson(acctJson);
    QTest::newRow("Set Account_oneOf data via ::fromJson()")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << acctJson
        // expectedJsonValue
        << postAccountRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << acctJson                       << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with Account_oneOf_1 data (firstname/lastname fields)
    const QString nameJson("{\"firstname\":\"Bob\",\"lastname\":\"Jones\"}"_L1);
    postAccountRequest.fromJson(nameJson);
    QTest::newRow("Set Account_oneOf_1 data via ::fromJson()")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << nameJson
        // expectedJsonValue
        << postAccountRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << nameJson                       << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with freeform object data (fields that don't match Account)
    const QString freeformJson("{\"arbitraryKey\":\"arbitraryValue\",\"number\":99}"_L1);
    postAccountRequest.fromJson(freeformJson);
    QTest::newRow("Set freeform object data via ::fromJson()")
            // postAccountRequest             // expectedJson
            << postAccountRequest             << postAccountRequest.asJson()
            // expectedJsonValue
            << postAccountRequest.asJsonValue()
            // oneOf0ExpectedJson
            << QString("{}"_L1)
            // oneOf1ExpectedJson
            << freeformJson
            // isOneOf0Valid                      // isOneOf0Set
            << false                              << false
            // isOneOf1Valid                      // isOneOf1Set
            << true                               << true
            // isValid                            // isSet
            << true                               << true;

    // fromJsonValue with valid Account
    postAccountRequest.fromJsonValue(validAccount.asJsonValue());
    QTest::newRow("Set a valid Account via ::fromJsonValue()")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << validAccount.asJson()
        // expectedJsonValue
        << postAccountRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << validAccount.asJson()          << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // invalid json! => the model is RESET to INITIAL state
    postAccountRequest.fromJson("{invalid json}"_L1);
    QTest::newRow("Set an invalid json via ::fromJson()")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << QString("{}"_L1)
        // expectedJsonValue
        << postAccountRequest.asJsonValue()
        // oneOf0ExpectedJson
        << QString("{}"_L1)
        // oneOf1ExpectedJson
        << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-object JSON (array) => reset
    postAccountRequest.fromJson("[]"_L1);
    QTest::newRow("PostAccount_request: set an unexpected array via ::fromJson()")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << QString("{}"_L1)
        // expectedJsonValue
        << postAccountRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-object JSON (string) => reset
    postAccountRequest.fromJson("\"string\""_L1);
    QTest::newRow("Set an unexpected string via ::fromJson()")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << QString("{}"_L1)
        // expectedJsonValue
        << postAccountRequest.asJsonValue()
        // oneOf0ExpectedJson
        << QString("{}"_L1)
        // oneOf1ExpectedJson
        << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << false                          << false
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson with empty object "{}" => Account (oneOf0) has both alternatives
    // with no required fields, so empty Account resolves internally. But Account
    // itself needs a selected alternative to be valid. The QMap (oneOf1) is always
    // valid with empty. This is ambiguous. Account is selected because it is first.
    postAccountRequest.fromJson("{}"_L1);
    QTest::newRow("Set an empty object {} via ::fromJson()")
        // postAccountRequest             // expectedJson
        << postAccountRequest             << postAccountRequest.asJson()
        // expectedJsonValue
        << postAccountRequest.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // isOneOf0Valid                  // isOneOf0Set
        << true                           << true
        // isOneOf1Valid                  // isOneOf1Set
        << false                          << false
        // isValid                        // isSet
        << true                           << true;
}

void OneOfTest::testPostAccountRequestJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::PostAccount_request, postAccountRequest);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(QString, oneOf0ExpectedJson);
    QFETCH(QString, oneOf1ExpectedJson);
    QFETCH(bool, isOneOf0Valid);
    QFETCH(bool, isOneOf0Set);
    QFETCH(bool, isOneOf1Valid);
    QFETCH(bool, isOneOf1Set);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(postAccountRequest.asJson(), expectedJson);
    QCOMPARE(postAccountRequest.asJsonValue(), expectedJsonValue);
    QCOMPARE(postAccountRequest.getOneOfAccount().asJson(), oneOf0ExpectedJson);
    QCOMPARE(mapAsString(postAccountRequest.getOneOfQMapQStringQJsonValue()), oneOf1ExpectedJson);
    QCOMPARE(postAccountRequest.isOneOfAccountValid(), isOneOf0Valid);
    QCOMPARE(postAccountRequest.isOneOfAccountSet(), isOneOf0Set);
    QCOMPARE(postAccountRequest.isOneOfQMapQStringQJsonValueValid(), isOneOf1Valid);
    QCOMPARE(postAccountRequest.isOneOfQMapQStringQJsonValueSet(), isOneOf1Set);
    QCOMPARE(postAccountRequest.isValid(), isValid);
    QCOMPARE(postAccountRequest.isSet(), isSet);
}

void OneOfTest::testPostFarmPetRequestJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::PostFarmPet_request>("postFarmPetRequest");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<QString>("oneOfBunnyExpectedJson");
    QTest::addColumn<QString>("oneOfDuckExpectedJson");
    QTest::addColumn<QString>("oneOfDogExpectedJson");
    QTest::addColumn<SchemasModelsOneOf::Bunny>("getOneOfBunny");
    QTest::addColumn<SchemasModelsOneOf::Duck>("getOneOfDuck");
    QTest::addColumn<SchemasModelsOneOf::Dog>("getOneOfDog");
    QTest::addColumn<bool>("isOneOfBunnyValid");
    QTest::addColumn<bool>("isOneOfBunnySet");
    QTest::addColumn<bool>("isOneOfDuckValid");
    QTest::addColumn<bool>("isOneOfDuckSet");
    QTest::addColumn<bool>("isOneOfDogValid");
    QTest::addColumn<bool>("isOneOfDogSet");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    SchemasModelsOneOf::PostFarmPet_request req;
    QTest::newRow("Empty PostFarmPet_request object")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << SchemasModelsOneOf::Bunny()    << SchemasModelsOneOf::Duck()
        // getOneOfDog
        << SchemasModelsOneOf::Dog()
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // no required fields
    SchemasModelsOneOf::Bunny bunny;
    bunny.setWeight(5);
    bunny.setBreedType("Himalayan"_L1);

    // no required fields
    SchemasModelsOneOf::Duck duck;
    duck.setHunts(false);
    duck.setAge(7);

    // has required 'bark' - bool
    SchemasModelsOneOf::Dog hund;
    hund.setBark(false);
    hund.setBreed("French Bulldog"_L1);

    // check setters work fine with valid objects
    req.setOneOfBunny(bunny);
    QTest::newRow("Set a valid Bunny object via setter")
        // postFarmPetRequest             // expectedJson
        << req                            << bunny.asJson()
        // expectedJsonValue
        << bunny.asJsonValue()
        // oneOfBunnyExpectedJson
        << bunny.asJson()
        // oneOfDuckExpectedJson             // oneOfDogExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << bunny                          << SchemasModelsOneOf::Duck()
        // getOneOfDog
        << SchemasModelsOneOf::Dog()
        // isOneOfBunnyValid              // isOneOfBunnySet
        << true                           << true
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    req.setOneOfDuck(duck);
    QTest::newRow("Set a valid Duck object via setter")
        // postFarmPetRequest             // expectedJson
        << req                            << duck.asJson()
        // expectedJsonValue
        << duck.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << duck.asJson()
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << SchemasModelsOneOf::Bunny()    << duck
        // getOneOfDog
        << SchemasModelsOneOf::Dog()
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << true                           << true
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    req.setOneOfDog(hund);
    QTest::newRow("Set a valid Dog object via setter")
        // postFarmPetRequest             // expectedJson
        << req                            << hund.asJson()
        // expectedJsonValue
        << hund.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << hund.asJson()
        // getOneOfBunny                  // getOneOfDuck
        << SchemasModelsOneOf::Bunny()    << SchemasModelsOneOf::Duck()
        // getOneOfDog
        << hund
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set an empty Dog object, the dog has required fields, so an empty
    // dog is INVALID => entire PostFarmPet_request stays invalid
    const SchemasModelsOneOf::Dog emptyDog;
    req.setOneOfDog(emptyDog);
    QTest::newRow("Set an empty Dog object via setter")
        // postFarmPetRequest             // expectedJson
        << req                            << emptyDog.asJson()
        // expectedJsonValue
        << emptyDog.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << SchemasModelsOneOf::Bunny()    << SchemasModelsOneOf::Duck()
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set an empty Bunny object, the bunny has no required fields, so an empty
    // bunny is VALID => entire PostFarmPet_request becomes valid
    const SchemasModelsOneOf::Bunny emptyBunny;
    req.setOneOfBunny(emptyBunny);
    QTest::newRow("Set an empty Bunny object via setter")
        // postFarmPetRequest             // expectedJson
        << req                            << emptyBunny.asJson()
        // expectedJsonValue
        << emptyBunny.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << SchemasModelsOneOf::Duck()
        // getOneOfDog
        << SchemasModelsOneOf::Dog()
        // isOneOfBunnyValid              // isOneOfBunnySet
        << true                           << true
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set an empty Duck object, the duck has no required fields, so an empty
    // duck is VALID => entire PostFarmPet_request becomes valid
    const SchemasModelsOneOf::Duck emptyDuck;
    req.setOneOfDuck(emptyDuck);
    QTest::newRow("Set an empty Duck object via setter")
        // postFarmPetRequest             // expectedJson
        << req                            << emptyDuck.asJson()
        // expectedJsonValue
        << emptyDuck.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << SchemasModelsOneOf::Dog()
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << true                           << true
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue with EMPTY data.
    // Attempting to set an empty Dog via req.fromJsonValue().
    // Dog defines required field 'bark', so an empty object is INVALID for Dog.
    //
    // However, emptyDog.asJsonValue() produces '{}' because it is an empty object.
    //
    // The PostFarmPet_request::fromJsonValue() logic cannot determine whether '{}'
    // represents a Bunny, Duck, or Dog. The only available approach is to try
    // deserializing '{}' against each schema and select the one that validates
    // successfully.
    //
    // This leads to ambiguity, since '{}' is valid for both Bunny and Duck
    // (neither has required fields). Bunny is first, so it wins.
    //
    // To avoid such situations, the schema should define either a discriminator
    // or required fields for each Object in the YAML specification.
    //
    // This ambiguity is intentional here for testing purposes, to highlight
    // the importance of well-defined API schemas. Ensuring correctness of the
    // API design is the responsibility of the API author.
    req.fromJsonValue(emptyDog.asJsonValue());
    QTest::newRow("Set an empty Dog via ::fromJsonValue(emptyDog.asJsonValue())")
        // postFarmPetRequest             // expectedJson
        << req                            << emptyDog.asJson()
        // expectedJsonValue
        << emptyDog.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << emptyDog
        // !!! NOTE: ambiguity between Bunny and Duck resolved to Bunny
        // (because it's first in the list)
        // isOneOfBunnyValid              // isOneOfBunnySet
        << true                           << true
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson with EMPTY data. Same expectations as with
    // 'Set an empty Dog via req.fromJsonValue(emptyDog.asJsonValue())'
    req.fromJson(emptyDog.asJson());
    QTest::newRow("Set an empty Dog via ::fromJson(emptyDog.asJson())")
        // postFarmPetRequest             // expectedJson
        << req                            << emptyDog.asJson()
        // expectedJsonValue
        << emptyDog.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << emptyDog
        // !!! NOTE: ambiguity between Bunny and Duck, Bunny wins
        // isOneOfBunnyValid              // isOneOfBunnySet
        << true                           << true
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // ::fromJson with EMPTY Bunny data. Same expectations as above.
    req.fromJson(emptyBunny.asJson());
    QTest::newRow("Set an empty Bunny via ::fromJson(emptyBunny.asJson())")
        // postFarmPetRequest             // expectedJson
        << req                            << emptyBunny.asJson()
        // expectedJsonValue
        << emptyBunny.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << true                           << true
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    req.fromJsonValue(emptyBunny.asJsonValue());
    QTest::newRow("Set an empty Bunny via ::fromJsonValue(emptyBunny.asJsonValue())")
        // postFarmPetRequest             // expectedJson
        << req                            << emptyBunny.asJson()
        // expectedJsonValue
        << emptyBunny.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << true                           << true
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    const QString bunnyWeight("{\"weight\":5}"_L1);
    req.fromJson(bunnyWeight);
    QTest::newRow("Set a valid Bunny object via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << bunnyWeight
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << bunnyWeight                    << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                         // getOneOfDuck
        << SchemasModelsOneOf::Bunny(bunnyWeight) << emptyDuck
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << true                           << true
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // check fromJsonValue() works fine for VALID bunny object
    const SchemasModelsOneOf::Bunny okBunny(bunnyWeight);
    req.fromJsonValue(okBunny.asJsonValue());
    QTest::newRow("Set a valid Bunny object via ::fromJsonValue()")
        // postFarmPetRequest             // expectedJson
        << req                            << bunnyWeight
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << bunnyWeight                    << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                         // getOneOfDuck
        << SchemasModelsOneOf::Bunny(bunnyWeight) << emptyDuck
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << true                           << true
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // check fromJson() works fine for VALID duck object
    const QString duckyDuck("{\"age\":7,\"hunts\":false}"_L1);
    req.fromJson(duckyDuck);
    QTest::newRow("Set a valid Duck object via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << duckyDuck
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << duckyDuck
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << SchemasModelsOneOf::Duck(duckyDuck)
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << true                           << true
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // check ::fromJsonValue() works fine for VALID duck object
    const SchemasModelsOneOf::Duck okDuck(duckyDuck);
    req.fromJsonValue(okDuck.asJsonValue());
    QTest::newRow("Set a valid Duck object via ::fromJsonValue()")
        // postFarmPetRequest             // expectedJson
        << req                            << duckyDuck
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << duckyDuck
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << SchemasModelsOneOf::Duck(duckyDuck)
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << true                           << true
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // check fromJson() works fine for VALID dog object
    const QString doggyDog("{\"bark\":false}"_L1);
    req.fromJson(doggyDog);
    QTest::newRow("Set a valid Dog object via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << doggyDog
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << doggyDog
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << SchemasModelsOneOf::Dog(doggyDog)
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << true                           << true
        // isValid                       // isSet
        << true                           << true;

    // check fromJsonValue() works fine for VALID dog object
    const SchemasModelsOneOf::Dog okDog(doggyDog);
    req.fromJsonValue(okDog.asJsonValue());
    QTest::newRow("Set a valid Dog object via ::fromJsonValue()")
        // postFarmPetRequest             // expectedJson
        << req                            << doggyDog
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << doggyDog
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << SchemasModelsOneOf::Dog(doggyDog)
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // invalid json! => the model is RESET to INITIAL state
    req.fromJson("{invalid json}"_L1);
    QTest::newRow("Set an invalid json via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{}"_L1)
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-object JSON (array) => the model is RESET to INITIAL state
    req.fromJson("[]"_L1);
    QTest::newRow("Set an unexpected array [] via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{}"_L1)
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Test with non-object JSON (string) => the model is RESET to INITIAL state
    req.fromJson("\"just a string\""_L1);
    QTest::newRow("Set an unexpected string via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{}"_L1)
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Missing Required Fields (Dog's 'bark' is required)
    // Dog without required 'bark' field - should not be valid
    SchemasModelsOneOf::Dog invalidDog;
    invalidDog.setBreed("Husky-kolbasky"_L1);
    req.setOneOfDog(invalidDog);
    QTest::newRow("Set an invalid Dog via setter")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{}"_L1)
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << invalidDog.asJson()
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << invalidDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // The status here reflects the status of the model that was set.
        // The model's field was set by setter, so invalidDog.isSet() == true
        // but invalidDog.isValid() == false, because required field is not set.
        // isOneOfDogValid                // isOneOfDogSet
        << invalidDog.isValid()           << invalidDog.isSet()
        // isValid                        // isSet
        << false                          << true;

    // Try to parse JSON with only 'breed' field
    // Dog also has 'breed', but bark is missing => Dog is invalid.
    const QString testValue("{\"breed\":\"Retriever\"}"_L1);
    req.fromJson(testValue);
    QTest::newRow("Set a breed-only Dog JSON via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{}"_L1)
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << testValue
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << SchemasModelsOneOf::Dog(testValue)
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << true
        // isValid                        // isSet
        << false                          << true;

    // Partial/Incomplete JSON Objects
    // Only weight field for Bunny (without breed-type),
    // should work since Bunny has no required fields
    const QString notFullBunny("{\"weight\":3}"_L1);
    req.fromJson(notFullBunny);
    QTest::newRow("Set a valid non-full Bunny via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << notFullBunny
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << notFullBunny                   << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                          // getOneOfDuck
        << SchemasModelsOneOf::Bunny(notFullBunny) << emptyDuck
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << true                           << true
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Extra/Unknown Fields in JSON
    // JSON with fields not in schema - should parse known fields and ignore unknown
    // Using Duck fields + unknown fields to avoid breed ambiguity with Dog
    req.fromJson("{\"age\":7,\"hunts\":false,\"name\":\"Donald\",\"color\":\"white\"}"_L1);
    QString expected("{\"age\":7,\"hunts\":false}"_L1);
    QTest::newRow("Extra/Unknown Fields in JSON via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << expected
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << expected
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << SchemasModelsOneOf::Duck(expected)
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << true                           << true
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Invalid Field Types for Duck(). Wrong type for age (string instead of integer)
    // Note: fields of Duck type are not required, it means wrong fields can be omitted
    expected = QString("{\"age\":\"seven\",\"hunts\":false}"_L1);
    req.fromJson(expected);
    QTest::newRow("Invalid field 'age=seven' in Duck via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{\"hunts\":false}"_L1)
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{\"hunts\":false}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << SchemasModelsOneOf::Duck(expected)
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << true                           << true
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Invalid Field Types for Duck(). Wrong type for age (double instead of integer)
    // Note: fields of Duck type are not required, it means wrong fields can be omitted
    expected = QString("{\"age\":0.1,\"hunts\":true}"_L1);
    req.fromJson(expected);
    QTest::newRow("Invalid field 'age=0.1' in Duck via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{\"hunts\":true}"_L1)
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{\"hunts\":true}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << SchemasModelsOneOf::Duck(expected)
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << true                           << true
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Invalid Field Types for Duck(). Wrong type for age (bool instead of integer)
    // Note: fields of Duck type are not required, it means wrong fields can be omitted
    expected = QString("{\"age\":true,\"hunts\":true}"_L1);
    req.fromJson(expected);
    QTest::newRow("Invalid field 'age=true' in Duck via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{\"hunts\":true}"_L1)
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{\"hunts\":true}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << SchemasModelsOneOf::Duck(expected)
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << true                           << true
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    expected = QString("{\"age\":true,\"hunts\":1}"_L1);
    req.fromJson(expected);
    QTest::newRow("Invalid fields: 'age=true', hunts=1 in Duck via ::fromJson()")
        // postFarmPetRequest             // expectedJson
        << req                            << QString("{}"_L1)
        // expectedJsonValue
        << req.asJsonValue()
        // oneOfBunnyExpectedJson         // oneOfDuckExpectedJson
        << QString("{}"_L1)               << QString("{}"_L1)
        // oneOfDogExpectedJson
        << QString("{}"_L1)
        // getOneOfBunny                  // getOneOfDuck
        << emptyBunny                     << emptyDuck
        // getOneOfDog
        << emptyDog
        // isOneOfBunnyValid              // isOneOfBunnySet
        << false                          << false
        // isOneOfDuckValid               // isOneOfDuckSet
        << false                          << false
        // isOneOfDogValid                // isOneOfDogSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;
}

void OneOfTest::testPostFarmPetRequestJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::PostFarmPet_request, postFarmPetRequest);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(QString, oneOfBunnyExpectedJson);
    QFETCH(QString, oneOfDuckExpectedJson);
    QFETCH(QString, oneOfDogExpectedJson);
    QFETCH(SchemasModelsOneOf::Bunny, getOneOfBunny);
    QFETCH(SchemasModelsOneOf::Duck, getOneOfDuck);
    QFETCH(SchemasModelsOneOf::Dog, getOneOfDog);
    QFETCH(bool, isOneOfBunnyValid);
    QFETCH(bool, isOneOfBunnySet);
    QFETCH(bool, isOneOfDuckValid);
    QFETCH(bool, isOneOfDuckSet);
    QFETCH(bool, isOneOfDogValid);
    QFETCH(bool, isOneOfDogSet);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(postFarmPetRequest.asJson(), expectedJson);
    QCOMPARE(postFarmPetRequest.asJsonValue(), expectedJsonValue);
    QCOMPARE(postFarmPetRequest.getOneOfBunny().asJson(), oneOfBunnyExpectedJson);
    QCOMPARE(postFarmPetRequest.isOneOfBunnyValid(), isOneOfBunnyValid);
    QCOMPARE(postFarmPetRequest.isOneOfBunnySet(), isOneOfBunnySet);
    QCOMPARE(postFarmPetRequest.getOneOfDuck().asJson(), oneOfDuckExpectedJson);
    QCOMPARE(postFarmPetRequest.isOneOfDuckValid(), isOneOfDuckValid);
    QCOMPARE(postFarmPetRequest.isOneOfDuckSet(), isOneOfDuckSet);
    QCOMPARE(postFarmPetRequest.getOneOfDog().asJson(), oneOfDogExpectedJson);
    QCOMPARE(postFarmPetRequest.getOneOfBunny(), getOneOfBunny);
    QCOMPARE(postFarmPetRequest.getOneOfDuck(), getOneOfDuck);
    QCOMPARE(postFarmPetRequest.getOneOfDog(), getOneOfDog);
    QCOMPARE(postFarmPetRequest.isOneOfDogValid(), isOneOfDogValid);
    QCOMPARE(postFarmPetRequest.isOneOfDogSet(), isOneOfDogSet);
    QCOMPARE(postFarmPetRequest.isValid(), isValid);
    QCOMPARE(postFarmPetRequest.isSet(), isSet);
}

void OneOfTest::testDummyTestValue()
{
    SchemasModelsOneOf::DummyTestValue dummy;
    QCOMPARE(dummy.asJson(), "{}"_L1);
    QCOMPARE(dummy.asJsonValue(), QJsonValue(QJsonValue::Object));
    QCOMPARE(dummy.isValid(), false);
    QCOMPARE(dummy.isSet(), false);
}

// Helper: serialize QList<QList<qint32>> to compact JSON string
static QString listListInt32AsJson(const QList<QList<qint32>> &v)
{
    QJsonArray outer;
    for (const auto &inner : v) {
        QJsonArray arr;
        for (const auto &i : inner)
            arr.append(i);
        outer.append(arr);
    }
    return QString::fromUtf8(QJsonValue(outer).toJson(QJsonDocument::Compact));
}

void OneOfTest::testRootJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::Root>("root");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<qint32>("oneOf0ExpectedValue");
    QTest::addColumn<QString>("oneOf1ExpectedJson");
    QTest::addColumn<QString>("oneOf2ExpectedJson");
    QTest::addColumn<bool>("isOneOfQint32Valid");
    QTest::addColumn<bool>("isOneOfQint32Set");
    QTest::addColumn<bool>("isOneOfQListQListqint32Valid");
    QTest::addColumn<bool>("isOneOfQListQListqint32Set");
    QTest::addColumn<bool>("isOneOfContainerValid");
    QTest::addColumn<bool>("isOneOfContainerSet");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    // Root has three oneOf alternatives:
    // oneOf0: qint32                   - primitive, always valid when set
    // oneOf1: QList<QList<qint32>>     - nested list, always valid when set
    // oneOf2: Container                - object, valid if Container is valid

    SchemasModelsOneOf::Root root;
    QTest::newRow("Empty Root object")
        // root                         // expectedJson
        << root                         << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << false                        << false;

    // Set a valid qint32 via setter
    root.setOneOfQint32(42);
    QTest::newRow("Set qint32=42 via setter")
        // root                         // expectedJson
        << root                         << QString("42"_L1)
        // expectedJsonValue
        << QJsonValue(42)
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(42)                   << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << true                         << true
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // Set qint32 = 0 via setter (zero is a legitimate value)
    root.setOneOfQint32(0);
    QTest::newRow("Set qint32=0 via setter")
        // root                         // expectedJson
        << root                         << QString("0"_L1)
        // expectedJsonValue
        << QJsonValue(0)
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << true                         << true
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // Set negative qint32 via setter
    root.setOneOfQint32(-7);
    QTest::newRow("Set qint32=-7 via setter")
        // root                         // expectedJson
        << root                         << QString("-7"_L1)
        // expectedJsonValue
        << QJsonValue(-7)
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(-7)                   << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << true                         << true
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // Set a non-empty QList<QList<qint32>> via setter
    const QList<QList<qint32>> nestedList = {{1, 2}, {3, 4}};
    root.setOneOfQListQListqint32(nestedList);
    QTest::newRow("Set QList<QList<qint32>>={{1,2},{3,4}} via setter")
        // root                         // expectedJson
        << root                         << QString("[[1,2],[3,4]]"_L1)
        // expectedJsonValue
        << QJsonValue::fromJson("[[1,2],[3,4]]")
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[[1,2],[3,4]]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << true                         << true
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // Set an empty QList<QList<qint32>> via setter (empty list is still a valid value)
    const QList<QList<qint32>> emptyList;
    root.setOneOfQListQListqint32(emptyList);
    QTest::newRow("Set empty QList<QList<qint32>> via setter")
        // root                         // expectedJson
        << root                         << QString("[]"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonArray())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << true                         << true
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // Set a valid Container (Container_oneOf with Duck value) via setter
    SchemasModelsOneOf::Duck duck;
    duck.setHunts(true);
    duck.setAge(5);
    SchemasModelsOneOf::Container_oneOf containerOneOf;
    containerOneOf.setValue(duck);
    SchemasModelsOneOf::Container validContainer;
    validContainer.setOneOfContainer_oneOf(containerOneOf);
    const QString duckContainerJson("{\"value\":{\"age\":5,\"hunts\":true}}"_L1);
    root.setOneOfContainer(validContainer);
    QTest::newRow("Set valid Container (Container_oneOf with Duck) via setter")
        // root                         // expectedJson
        << root                         << duckContainerJson
        // expectedJsonValue
        << validContainer.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << duckContainerJson
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << true                         << true
        // isValid                      // isSet
        << true                         << true;

    // Set valid Container (QList<Dog>) via setter.
    // Root.asJson() for this case returns an array (Container serializes as array
    // when the active variant is QList<Dog>).
    SchemasModelsOneOf::Dog dogForContainer;
    dogForContainer.setBark(true);
    dogForContainer.setBreed("Lab"_L1);
    SchemasModelsOneOf::Container dogListContainer;
    dogListContainer.setOneOfQListDog({dogForContainer});
     const QString dogListContainerJson("[{\"bark\":true,\"breed\":\"Lab\"}]"_L1);
    root.setOneOfContainer(dogListContainer);
    QTest::newRow("Set valid Container (QList<Dog>) via setter")
        // root                         // expectedJson
        << root                         << dogListContainerJson
        // expectedJsonValue
        << dogListContainer.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << dogListContainerJson
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << true                         << true
        // isValid                      // isSet
        << true                         << true;

    // Set an empty (invalid) Container via setter.
    // Empty Container has isValid()=false and isSet()=false,
    // so Root should have Container not set and not valid.
    const SchemasModelsOneOf::Container emptyContainer;
    root.setOneOfContainer(emptyContainer);
    QTest::newRow("Set empty (invalid) Container via setter")
        // root                         // expectedJson
        << root                         << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << false                        << false;

    // fromJson: integer value
    root.fromJson("42"_L1);
    QTest::newRow("Set qint32=42 via ::fromJson()")
        // root                         // expectedJson
        << root                         << QString("42"_L1)
        // expectedJsonValue
        << QJsonValue(42)
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(42)                   << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << true                         << true
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // fromJson: zero integer
    root.fromJson("0"_L1);
    QTest::newRow("Set qint32=0 via ::fromJson()")
        // root                         // expectedJson
        << root                         << QString("0"_L1)
        // expectedJsonValue
        << QJsonValue(0)
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << true                         << true
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // fromJson: negative integer
    root.fromJson("-100"_L1);
    QTest::newRow("Set qint32=-100 via ::fromJson()")
        // root                         // expectedJson
        << root                         << QString("-100"_L1)
        // expectedJsonValue
        << QJsonValue(-100)
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(-100)                 << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << true                         << true
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // fromJson: 3.14 => json.isDouble() but not an integer => qint32 fails => reset
    root.fromJson("3.14"_L1);
    QTest::newRow("Set double 3.14 via ::fromJson() (no double alternative)")
        // root                         // expectedJson
        << root                         << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << false                        << false;

    // fromJson: nested array [[1,2],[3,4]]
    root.fromJson("[[1,2],[3,4]]"_L1);
    QTest::newRow("Set QList<QList<qint32>> via ::fromJson()")
        // root                         // expectedJson
        << root                         << QString("[[1,2],[3,4]]"_L1)
        // expectedJsonValue
        << root.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[[1,2],[3,4]]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << true                         << true
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // fromJson: empty array => empty QList<QList<qint32>>, valid
    root.fromJson("[]"_L1);
    QTest::newRow("Set empty array via ::fromJson() => empty QList<QList<qint32>>")
        // root                         // expectedJson
        << root                         << QString("[]"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonArray())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << true                         << true
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // fromJson: array of ints [1,2,3] => each element is int, not QList<qint32>
    // => QList<QList<qint32>> parse fails => reset
    root.fromJson("[1,2,3]"_L1);
    QTest::newRow("Set [1,2,3] via ::fromJson() => fails QList<QList<qint32>>")
        // root                         // expectedJson
        << root                         << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << false                        << false;

    // fromJson: Container_oneOf JSON object with a Duck value
    root.fromJson(duckContainerJson);
    QTest::newRow("Set Container (Container_oneOf with Duck) via ::fromJson()")
        // root                         // expectedJson
        << root                         << duckContainerJson
        // expectedJsonValue
        << root.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << duckContainerJson
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << true                         << true
        // isValid                      // isSet
        << true                         << true;

    // => Root Container is set and valid
    root.fromJson("{}"_L1);
    QTest::newRow("Set empty object {} via ::fromJson() => Container with empty object")
        // root                         // expectedJson
        << root                         << QString("{}"_L1)
        // expectedJsonValue
        << root.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << true                         << true
        // isValid                      // isSet
        << true                         << true;

    root.fromJson(dogListContainerJson);
    QTest::newRow("Set Container(QList<Dog>) JSON via ::fromJson() => no round-trip")
        // root                         // expectedJson
        << root                         << dogListContainerJson
        // expectedJsonValue
        << root.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << dogListContainerJson
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << true                         << true
        // isValid                      // isSet
        << true                         << true;

    // fromJson: bool value => no bool alternative => reset
    root.fromJson("true"_L1);
    QTest::newRow("Set bool 'true' via ::fromJson() => no bool alternative")
        // root                         // expectedJson
        << root                         << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << false                        << false;

    // fromJson: string value => no string alternative => reset
    root.fromJson("\"hello\""_L1);
    QTest::newRow("Set string via ::fromJson() => no string alternative")
        // root                         // expectedJson
        << root                         << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << false                        << false;

    // fromJson: invalid JSON => reset
    root.fromJson("{invalid json}"_L1);
    QTest::newRow("Set invalid JSON via ::fromJson() => reset")
        // root                         // expectedJson
        << root                         << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << false                        << false;

    // fromJson: null => reset
    root.fromJson("null"_L1);
    QTest::newRow("Set null via ::fromJson() => reset")
        // root                         // expectedJson
        << root                         << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << false                        << false;

    // fromJsonValue: integer
    root.fromJsonValue(QJsonValue(99));
    QTest::newRow("Set qint32=99 via ::fromJsonValue()")
        // root                         // expectedJson
        << root                         << QString("99"_L1)
        // expectedJsonValue
        << QJsonValue(99)
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(99)                   << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << true                         << true
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // fromJsonValue: nested array
    root.fromJsonValue(QJsonValue::fromJson("[[1,2],[3,4]]"));
    QTest::newRow("Set QList<QList<qint32>> via ::fromJsonValue()")
        // root                         // expectedJson
        << root                         << QString("[[1,2],[3,4]]"_L1)
        // expectedJsonValue
        << root.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[[1,2],[3,4]]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << true                         << true
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // fromJsonValue: valid Container object
    root.fromJsonValue(validContainer.asJsonValue());
    QTest::newRow("Set valid Container via ::fromJsonValue()")
        // root                         // expectedJson
        << root                         << duckContainerJson
        // expectedJsonValue
        << root.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << duckContainerJson
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << true                         << true
        // isValid                      // isSet
        << true                         << true;

    // Implicit construction from qint32
    SchemasModelsOneOf::Root rootFromInt(7);
    QTest::newRow("Implicitly construct Root from qint32=7")
        // root                         // expectedJson
        << rootFromInt                  << QString("7"_L1)
        // expectedJsonValue
        << QJsonValue(7)
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(7)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << true                         << true
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // Implicit construction from QList<QList<qint32>>
    SchemasModelsOneOf::Root rootFromList(nestedList);
    QTest::newRow("Implicitly construct Root from QList<QList<qint32>>")
        // rootFromList                 // expectedJson
        << rootFromList                 << QString("[[1,2],[3,4]]"_L1)
        // expectedJsonValue
        << rootFromList.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[[1,2],[3,4]]"_L1)
        // oneOf2ExpectedJson
        << QString("{}"_L1)
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << true                         << true
        // isOneOfContainerValid        // isOneOfContainerSet
        << false                        << false
        // isValid                      // isSet
        << true                         << true;

    // Implicit construction from Container
    SchemasModelsOneOf::Root rootFromContainer(validContainer);
    QTest::newRow("Implicitly construct Root from valid Container")
        // rootFromContainer            // expectedJson
        << rootFromContainer            << duckContainerJson
        // expectedJsonValue
        << rootFromContainer.asJsonValue()
        // oneOf0ExpectedValue          // oneOf1ExpectedJson
        << qint32(0)                    << QString("[]"_L1)
        // oneOf2ExpectedJson
        << duckContainerJson
        // isOneOfQint32Valid           // isOneOfQint32Set
        << false                        << false
        // isOneOfQListQListqint32Valid // isOneOfQListQListqint32Set
        << false                        << false
        // isOneOfContainerValid        // isOneOfContainerSet
        << true                         << true
        // isValid                      // isSet
        << true                         << true;
}

void OneOfTest::testRootJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::Root, root);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(qint32, oneOf0ExpectedValue);
    QFETCH(QString, oneOf1ExpectedJson);
    QFETCH(QString, oneOf2ExpectedJson);
    QFETCH(bool, isOneOfQint32Valid);
    QFETCH(bool, isOneOfQint32Set);
    QFETCH(bool, isOneOfQListQListqint32Valid);
    QFETCH(bool, isOneOfQListQListqint32Set);
    QFETCH(bool, isOneOfContainerValid);
    QFETCH(bool, isOneOfContainerSet);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(root.asJson(), expectedJson);
    QCOMPARE(root.asJsonValue(), expectedJsonValue);
    QCOMPARE(root.getOneOfQint32(), oneOf0ExpectedValue);
    QCOMPARE(root.isOneOfQint32Valid(), isOneOfQint32Valid);
    QCOMPARE(root.isOneOfQint32Set(), isOneOfQint32Set);
    QCOMPARE(listListInt32AsJson(root.getOneOfQListQListqint32()), oneOf1ExpectedJson);
    QCOMPARE(root.isOneOfQListQListqint32Valid(), isOneOfQListQListqint32Valid);
    QCOMPARE(root.isOneOfQListQListqint32Set(), isOneOfQListQListqint32Set);
    QCOMPARE(root.getOneOfContainer().asJson(), oneOf2ExpectedJson);
    QCOMPARE(root.isOneOfContainerValid(), isOneOfContainerValid);
    QCOMPARE(root.isOneOfContainerSet(), isOneOfContainerSet);
    QCOMPARE(root.isValid(), isValid);
    QCOMPARE(root.isSet(), isSet);
}

// Helper: serialize QMap<QString, QMap<QString, qint32>> to compact JSON string
static QString mapMapInt32AsJson(const QMap<QString, QMap<QString, qint32>> &v)
{
    QJsonObject outer;
    for (const auto &[key, innerMap] : v.asKeyValueRange()) {
        QJsonObject inner;
        for (const auto &[innerKey, val] : innerMap.asKeyValueRange())
            inner.insert(innerKey, val);
        outer.insert(key, inner);
    }
    return QString::fromUtf8(QJsonDocument(outer).toJson(QJsonDocument::Compact));
}

// Helper: serialize QList<QMap<QString, qint32>> to compact JSON string
static QString listMapInt32AsJson(const QList<QMap<QString, qint32>> &v)
{
    QJsonArray arr;
    for (const auto &map : v) {
        QJsonObject obj;
        for (const auto &[key, val] : map.asKeyValueRange())
            obj.insert(key, val);
        arr.append(obj);
    }
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void OneOfTest::testRootMapJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::RootMap>("rootMap");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<QString>("oneOf0ExpectedJson");
    QTest::addColumn<QString>("oneOf1ExpectedJson");
    QTest::addColumn<bool>("isOneOfMapOfMapsValid");
    QTest::addColumn<bool>("isOneOfMapOfMapsSet");
    QTest::addColumn<bool>("isOneOfArrayOfMapsValid");
    QTest::addColumn<bool>("isOneOfArrayOfMapsSet");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    // RootMap has two oneOf alternatives:
    // oneOf0: QMap<QString, QMap<QString, qint32>> - map of maps
    // oneOf1: QList<QMap<QString, qint32>>         - array of maps

    SchemasModelsOneOf::RootMap rootMap;
    QTest::newRow("Empty RootMap object")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set a valid map-of-maps via setter
    QMap<QString, QMap<QString, qint32>> mapOfMaps;
    mapOfMaps.insert("group1"_L1, {{"a"_L1, 1}, {"b"_L1, 2}});
    mapOfMaps.insert("group2"_L1, {{"c"_L1, 3}});
    rootMap.setOneOfQMapQStringQMapQStringqint32(mapOfMaps);
    const QString mapOfMapsJson("{\"group1\":{\"a\":1,\"b\":2},\"group2\":{\"c\":3}}"_L1);
    QTest::newRow("Set valid map-of-maps via setter")
        // rootMap                        // expectedJson
        << rootMap                        << mapOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfMapsJson                  << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set an empty map-of-maps via setter (empty map is still valid)
    const QMap<QString, QMap<QString, qint32>> emptyMapOfMaps;
    rootMap.setOneOfQMapQStringQMapQStringqint32(emptyMapOfMaps);
    QTest::newRow("Set empty map-of-maps via setter")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a valid array-of-maps via setter
    QList<QMap<QString, qint32>> arrayOfMaps;
    arrayOfMaps.append({{"x"_L1, 10}, {"y"_L1, 20}});
    arrayOfMaps.append({{"z"_L1, 30}});
    rootMap.setOneOfQListQMapQStringqint32(arrayOfMaps);
    const QString arrayOfMapsJson("[{\"x\":10,\"y\":20},{\"z\":30}]"_L1);
    QTest::newRow("Set valid array-of-maps via setter")
        // rootMap                        // expectedJson
        << rootMap                        << arrayOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapsJson
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set an empty array-of-maps via setter (empty list is still valid)
    const QList<QMap<QString, qint32>> emptyArrayOfMaps;
    rootMap.setOneOfQListQMapQStringqint32(emptyArrayOfMaps);
    QTest::newRow("Set empty array-of-maps via setter")
        // rootMap                        // expectedJson
        << rootMap                        << QString("[]"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonArray())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set single-entry map-of-maps via setter
    QMap<QString, QMap<QString, qint32>> singleMapOfMaps;
    singleMapOfMaps.insert("only"_L1, {{"val"_L1, 42}});
    rootMap.setOneOfQMapQStringQMapQStringqint32(singleMapOfMaps);
    const QString singleMapOfMapsJson("{\"only\":{\"val\":42}}"_L1);
    QTest::newRow("Set single-entry map-of-maps via setter")
        // rootMap                        // expectedJson
        << rootMap                        << singleMapOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << singleMapOfMapsJson            << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set single-entry array-of-maps via setter
    QList<QMap<QString, qint32>> singleArrayOfMaps;
    singleArrayOfMaps.append({{"key"_L1, 99}});
    rootMap.setOneOfQListQMapQStringqint32(singleArrayOfMaps);
    const QString singleArrayOfMapsJson("[{\"key\":99}]"_L1);
    QTest::newRow("Set single-entry array-of-maps via setter")
        // rootMap                        // expectedJson
        << rootMap                        << singleArrayOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << singleArrayOfMapsJson
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson: valid map-of-maps JSON
    rootMap.fromJson(mapOfMapsJson);
    QTest::newRow("Set valid map-of-maps via ::fromJson()")
        // rootMap                        // expectedJson
        << rootMap                        << mapOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfMapsJson                  << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson: valid array-of-maps JSON
    rootMap.fromJson(arrayOfMapsJson);
    QTest::newRow("Set valid array-of-maps via ::fromJson()")
        // rootMap                        // expectedJson
        << rootMap                        << arrayOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapsJson
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson: empty object {} => map-of-maps
    rootMap.fromJson("{}"_L1);
    QTest::newRow("Set empty object {} via ::fromJson() => map-of-maps")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson: empty array [] => array-of-maps
    rootMap.fromJson("[]"_L1);
    QTest::newRow("Set empty array [] via ::fromJson() => empty array-of-maps")
        // rootMap                        // expectedJson
        << rootMap                        << QString("[]"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonArray())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson: array of ints [1,2,3] => each element is int, not a map
    // => QList<QMap<QString, qint32>> parse fails => reset
    rootMap.fromJson("[1,2,3]"_L1);
    QTest::newRow("Set [1,2,3] via ::fromJson() => fails array-of-maps")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: integer value => no integer alternative => reset
    rootMap.fromJson("42"_L1);
    QTest::newRow("Set integer via ::fromJson() => no integer alternative")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: bool value => no bool alternative => reset
    rootMap.fromJson("true"_L1);
    QTest::newRow("Set bool 'true' via ::fromJson() => no bool alternative")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: string value => no string alternative => reset
    rootMap.fromJson("\"hello\""_L1);
    QTest::newRow("Set string via ::fromJson() => no string alternative")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: invalid JSON => reset
    rootMap.fromJson("{invalid json}"_L1);
    QTest::newRow("Set invalid JSON via ::fromJson() => reset")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: null => reset
    rootMap.fromJson("null"_L1);
    QTest::newRow("Set null via ::fromJson() => reset")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJsonValue: valid map-of-maps object
    rootMap.fromJsonValue(QJsonValue::fromJson(mapOfMapsJson.toUtf8()));
    QTest::newRow("Set valid map-of-maps via ::fromJsonValue()")
        // rootMap                        // expectedJson
        << rootMap                        << mapOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfMapsJson                  << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue: valid array-of-maps
    rootMap.fromJsonValue(QJsonValue::fromJson(arrayOfMapsJson.toUtf8()));
    QTest::newRow("Set valid array-of-maps via ::fromJsonValue()")
        // rootMap                        // expectedJson
        << rootMap                        << arrayOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapsJson
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue: empty QJsonObject => map-of-maps
    rootMap.fromJsonValue(QJsonValue(QJsonObject()));
    QTest::newRow("Set empty QJsonObject via ::fromJsonValue() => map-of-maps")
        // rootMap                        // expectedJson
        << rootMap                        << QString("{}"_L1)
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue: empty QJsonArray => array-of-maps
    rootMap.fromJsonValue(QJsonValue(QJsonArray()));
    QTest::newRow("Set empty QJsonArray via ::fromJsonValue() => array-of-maps")
        // rootMap                        // expectedJson
        << rootMap                        << QString("[]"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonArray())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Implicit construction from QMap<QString, QMap<QString, qint32>>
    SchemasModelsOneOf::RootMap rootFromMap(mapOfMaps);
    QTest::newRow("Implicitly construct RootMap from map-of-maps")
        // rootFromMap                    // expectedJson
        << rootFromMap                    << mapOfMapsJson
        // expectedJsonValue
        << rootFromMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfMapsJson                  << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Implicit construction from QList<QMap<QString, qint32>>
    SchemasModelsOneOf::RootMap rootFromList(arrayOfMaps);
    QTest::newRow("Implicitly construct RootMap from array-of-maps")
        // rootFromList                   // expectedJson
        << rootFromList                   << arrayOfMapsJson
        // expectedJsonValue
        << rootFromList.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapsJson
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Override map-of-maps with array-of-maps via setter
    rootMap.setOneOfQMapQStringQMapQStringqint32(mapOfMaps);
    rootMap.setOneOfQListQMapQStringqint32(arrayOfMaps);
    QTest::newRow("Override map-of-maps with array-of-maps via setter")
        // rootMap                        // expectedJson
        << rootMap                        << arrayOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapsJson
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Override array-of-maps with map-of-maps via setter
    rootMap.setOneOfQListQMapQStringqint32(arrayOfMaps);
    rootMap.setOneOfQMapQStringQMapQStringqint32(mapOfMaps);
    QTest::newRow("Override array-of-maps with map-of-maps via setter")
        // rootMap                        // expectedJson
        << rootMap                        << mapOfMapsJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfMapsJson                  << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson: map-of-maps with negative values
    const QString negMapJson("{\"neg\":{\"val\":-5}}"_L1);
    rootMap.fromJson(negMapJson);
    QTest::newRow("Set map-of-maps with negative values via ::fromJson()")
        // rootMap                        // expectedJson
        << rootMap                        << negMapJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << negMapJson                     << QString("[]"_L1)
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << true                           << true
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson: array-of-maps with empty inner maps
    const QString emptyInnerJson("[{},{}]"_L1);
    rootMap.fromJson(emptyInnerJson);
    QTest::newRow("Set array-of-maps with empty inner maps via ::fromJson()")
        // rootMap                        // expectedJson
        << rootMap                        << emptyInnerJson
        // expectedJsonValue
        << rootMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << emptyInnerJson
        // isOneOfMapOfMapsValid          // isOneOfMapOfMapsSet
        << false                          << false
        // isOneOfArrayOfMapsValid        // isOneOfArrayOfMapsSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;
}

void OneOfTest::testRootMapJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::RootMap, rootMap);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(QString, oneOf0ExpectedJson);
    QFETCH(QString, oneOf1ExpectedJson);
    QFETCH(bool, isOneOfMapOfMapsValid);
    QFETCH(bool, isOneOfMapOfMapsSet);
    QFETCH(bool, isOneOfArrayOfMapsValid);
    QFETCH(bool, isOneOfArrayOfMapsSet);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(rootMap.asJson(), expectedJson);
    QCOMPARE(rootMap.asJsonValue(), expectedJsonValue);
    QCOMPARE(mapMapInt32AsJson(rootMap.getOneOfQMapQStringQMapQStringqint32()),
             oneOf0ExpectedJson);
    QCOMPARE(rootMap.isOneOfQMapQStringQMapQStringqint32Valid(),
             isOneOfMapOfMapsValid);
    QCOMPARE(rootMap.isOneOfQMapQStringQMapQStringqint32Set(),
             isOneOfMapOfMapsSet);
    QCOMPARE(listMapInt32AsJson(rootMap.getOneOfQListQMapQStringqint32()),
             oneOf1ExpectedJson);
    QCOMPARE(rootMap.isOneOfQListQMapQStringqint32Valid(),
             isOneOfArrayOfMapsValid);
    QCOMPARE(rootMap.isOneOfQListQMapQStringqint32Set(),
             isOneOfArrayOfMapsSet);
    QCOMPARE(rootMap.isValid(), isValid);
    QCOMPARE(rootMap.isSet(), isSet);
}

// Helper: serialize QMap<QString, Dog> to compact JSON string
static QString mapDogAsJson(const QMap<QString, SchemasModelsOneOf::Dog> &v)
{
    QJsonObject outer;
    for (const auto &[key, dog] : v.asKeyValueRange())
        outer.insert(key, dog.asJsonValue());
    return QString::fromUtf8(QJsonDocument(outer).toJson(QJsonDocument::Compact));
}

// Helper: serialize QList<QMap<QString, Duck>> to compact JSON string
static QString listMapDuckAsJson(const QList<QMap<QString, SchemasModelsOneOf::Duck>> &v)
{
    QJsonArray arr;
    for (const auto &map : v) {
        QJsonObject obj;
        for (const auto &[key, duck] : map.asKeyValueRange())
            obj.insert(key, duck.asJsonValue());
        arr.append(obj);
    }
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void OneOfTest::testRootMapModelsJsonConversionMethods_data()
{
    QTest::addColumn<SchemasModelsOneOf::RootMapModels>("rootMapModels");
    QTest::addColumn<QString>("expectedJson");
    QTest::addColumn<QJsonValue>("expectedJsonValue");
    QTest::addColumn<QString>("oneOf0ExpectedJson");
    QTest::addColumn<QString>("oneOf1ExpectedJson");
    QTest::addColumn<bool>("isOneOfMapOfDogsValid");
    QTest::addColumn<bool>("isOneOfMapOfDogsSet");
    QTest::addColumn<bool>("isOneOfArrayOfMapDucksValid");
    QTest::addColumn<bool>("isOneOfArrayOfMapDucksSet");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<bool>("isSet");

    // RootMapModels has two oneOf alternatives:
    // oneOf0: QMap<QString, Dog>         - map of Dog models
    // oneOf1: QList<QMap<QString, Duck>> - array of maps of Duck models
    SchemasModelsOneOf::RootMapModels rootMapModels;
    QTest::newRow("Empty RootMapModels object")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // Set a valid map-of-Dogs via setter
    SchemasModelsOneOf::Dog dog1;
    dog1.setBark(true);
    dog1.setBreed("Husky"_L1);
    SchemasModelsOneOf::Dog dog2;
    dog2.setBark(false);
    dog2.setBreed("Dingo"_L1);
    QMap<QString, SchemasModelsOneOf::Dog> mapOfDogs;
    mapOfDogs.insert("rex"_L1, dog1);
    mapOfDogs.insert("fido"_L1, dog2);
    rootMapModels.setOneOfQMapQStringDog(mapOfDogs);
    const QString mapOfDogsJson = mapDogAsJson(mapOfDogs);
    QTest::newRow("Set valid map-of-Dogs via setter")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << mapOfDogsJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfDogsJson                  << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set an empty map-of-Dogs via setter (empty map is still valid)
    const QMap<QString, SchemasModelsOneOf::Dog> emptyMapOfDogs;
    rootMapModels.setOneOfQMapQStringDog(emptyMapOfDogs);
    QTest::newRow("Set empty map-of-Dogs via setter")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set a valid array-of-maps-of-Ducks via setter
    SchemasModelsOneOf::Duck duck1;
    duck1.setHunts(true);
    duck1.setAge(3);
    SchemasModelsOneOf::Duck duck2;
    duck2.setHunts(false);
    duck2.setAge(5);
    QList<QMap<QString, SchemasModelsOneOf::Duck>> arrayOfMapDucks;
    arrayOfMapDucks.append({{"donald"_L1, duck1}, {"daisy"_L1, duck2}});
    arrayOfMapDucks.append({{"scrooge"_L1, duck1}});
    rootMapModels.setOneOfQListQMapQStringDuck(arrayOfMapDucks);
    const QString arrayOfMapDucksJson = listMapDuckAsJson(arrayOfMapDucks);
    QTest::newRow("Set valid array-of-map-Ducks via setter")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << arrayOfMapDucksJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapDucksJson
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set an empty array-of-maps-of-Ducks via setter (empty list is still valid)
    const QList<QMap<QString, SchemasModelsOneOf::Duck>> emptyArrayOfMapDucks;
    rootMapModels.setOneOfQListQMapQStringDuck(emptyArrayOfMapDucks);
    QTest::newRow("Set empty array-of-map-Ducks via setter")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("[]"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonArray())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Set single-entry map-of-Dogs via setter
    QMap<QString, SchemasModelsOneOf::Dog> singleMapOfDogs;
    singleMapOfDogs.insert("buddy"_L1, dog1);
    rootMapModels.setOneOfQMapQStringDog(singleMapOfDogs);
    const QString singleMapOfDogsJson = mapDogAsJson(singleMapOfDogs);
    QTest::newRow("Set single-entry map-of-Dogs via setter")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << singleMapOfDogsJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << singleMapOfDogsJson            << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Set single-entry array-of-maps-of-Ducks via setter
    QList<QMap<QString, SchemasModelsOneOf::Duck>> singleArrayOfMapDucks;
    singleArrayOfMapDucks.append({{"quack"_L1, duck2}});
    rootMapModels.setOneOfQListQMapQStringDuck(singleArrayOfMapDucks);
    const QString singleArrayOfMapDucksJson = listMapDuckAsJson(singleArrayOfMapDucks);
    QTest::newRow("Set single-entry array-of-map-Ducks via setter")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << singleArrayOfMapDucksJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << singleArrayOfMapDucksJson
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson: valid map-of-Dogs JSON
    rootMapModels.fromJson(mapOfDogsJson);
    QTest::newRow("Set valid map-of-Dogs via ::fromJson()")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << mapOfDogsJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfDogsJson                  << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson: valid array-of-maps-of-Ducks JSON
    rootMapModels.fromJson(arrayOfMapDucksJson);
    QTest::newRow("Set valid array-of-map-Ducks via ::fromJson()")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << arrayOfMapDucksJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapDucksJson
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson: empty object {}
    rootMapModels.fromJson("{}"_L1);
    QTest::newRow("Set empty object {} via ::fromJson()")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson: empty array [] => array-of-maps-of-Ducks
    rootMapModels.fromJson("[]"_L1);
    QTest::newRow("Set empty array [] via ::fromJson() => empty array-of-map-Ducks")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("[]"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonArray())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJson: array of ints [1,2,3] => each element is int, not a map
    rootMapModels.fromJson("[1,2,3]"_L1);
    QTest::newRow("Set [1,2,3] via ::fromJson() => fails array-of-map-Ducks")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: integer value => no integer alternative => reset
    rootMapModels.fromJson("42"_L1);
    QTest::newRow("Set integer via ::fromJson() => no integer alternative")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: bool value => no bool alternative => reset
    rootMapModels.fromJson("true"_L1);
    QTest::newRow("Set bool 'true' via ::fromJson() => no bool alternative")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: string value => no string alternative => reset
    rootMapModels.fromJson("\"hello\""_L1);
    QTest::newRow("Set string via ::fromJson() => no string alternative")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: invalid JSON => reset
    rootMapModels.fromJson("{invalid json}"_L1);
    QTest::newRow("Set invalid JSON via ::fromJson() => reset")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJson: null => reset
    rootMapModels.fromJson("null"_L1);
    QTest::newRow("Set null via ::fromJson() => reset")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonObject())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << false                          << false;

    // fromJsonValue: valid map-of-Dogs object
    rootMapModels.fromJsonValue(QJsonValue::fromJson(mapOfDogsJson.toUtf8()));
    QTest::newRow("Set valid map-of-Dogs via ::fromJsonValue()")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << mapOfDogsJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfDogsJson                  << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue: valid array-of-maps-of-Ducks
    rootMapModels.fromJsonValue(QJsonValue::fromJson(arrayOfMapDucksJson.toUtf8()));
    // !!! ERROR: map-of-Dogs valid/set flags are not reset when array-of-Ducks wins
    QTest::newRow("Set valid array-of-map-Ducks via ::fromJsonValue()")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << arrayOfMapDucksJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapDucksJson
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue: empty QJsonObject => map-of-Dogs wins
    rootMapModels.fromJsonValue(QJsonValue(QJsonObject()));
    QTest::newRow("Set empty QJsonObject via ::fromJsonValue()")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("{}"_L1)
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJsonValue: empty QJsonArray => array-of-maps-of-Ducks
    rootMapModels.fromJsonValue(QJsonValue(QJsonArray()));
    QTest::newRow("Set empty QJsonArray via ::fromJsonValue() => array-of-map-Ducks")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << QString("[]"_L1)
        // expectedJsonValue
        << QJsonValue(QJsonArray())
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Implicit construction from QMap<QString, Dog>
    SchemasModelsOneOf::RootMapModels rootFromMap(mapOfDogs);
    QTest::newRow("Implicitly construct RootMapModels from map-of-Dogs")
        // rootFromMap                    // expectedJson
        << rootFromMap                    << mapOfDogsJson
        // expectedJsonValue
        << rootFromMap.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfDogsJson                  << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // Implicit construction from QList<QMap<QString, Duck>>
    SchemasModelsOneOf::RootMapModels rootFromList(arrayOfMapDucks);
    QTest::newRow("Implicitly construct RootMapModels from array-of-map-Ducks")
        // rootFromList                   // expectedJson
        << rootFromList                   << arrayOfMapDucksJson
        // expectedJsonValue
        << rootFromList.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapDucksJson
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Override map-of-Dogs with array-of-maps-of-Ducks via setter
    rootMapModels.setOneOfQMapQStringDog(mapOfDogs);
    rootMapModels.setOneOfQListQMapQStringDuck(arrayOfMapDucks);
    QTest::newRow("Override map-of-Dogs with array-of-map-Ducks via setter")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << arrayOfMapDucksJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << arrayOfMapDucksJson
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;

    // Override array-of-maps-of-Ducks with map-of-Dogs via setter
    rootMapModels.setOneOfQListQMapQStringDuck(arrayOfMapDucks);
    rootMapModels.setOneOfQMapQStringDog(mapOfDogs);
    QTest::newRow("Override array-of-map-Ducks with map-of-Dogs via setter")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << mapOfDogsJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << mapOfDogsJson                  << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson: map with Dog that has only required field (bark)
    const QString minimalDogMapJson("{\"spot\":{\"bark\":true}}"_L1);
    rootMapModels.fromJson(minimalDogMapJson);
    QTest::newRow("Set map-of-Dogs with minimal Dog (bark only) via ::fromJson()")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << minimalDogMapJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << minimalDogMapJson              << QString("[]"_L1)
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << true                           << true
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << false                          << false
        // isValid                        // isSet
        << true                           << true;

    // fromJson: array with maps of Ducks with empty inner maps
    const QString emptyInnerDucksJson("[{},{}]"_L1);
    rootMapModels.fromJson(emptyInnerDucksJson);
    QTest::newRow("Set array-of-map-Ducks with empty inner maps via ::fromJson()")
        // rootMapModels                  // expectedJson
        << rootMapModels                  << emptyInnerDucksJson
        // expectedJsonValue
        << rootMapModels.asJsonValue()
        // oneOf0ExpectedJson             // oneOf1ExpectedJson
        << QString("{}"_L1)               << emptyInnerDucksJson
        // isOneOfMapOfDogsValid          // isOneOfMapOfDogsSet
        << false                          << false
        // isOneOfArrayOfMapDucksValid    // isOneOfArrayOfMapDucksSet
        << true                           << true
        // isValid                        // isSet
        << true                           << true;
}

void OneOfTest::testRootMapModelsJsonConversionMethods()
{
    QFETCH(SchemasModelsOneOf::RootMapModels, rootMapModels);
    QFETCH(QString, expectedJson);
    QFETCH(QJsonValue, expectedJsonValue);
    QFETCH(QString, oneOf0ExpectedJson);
    QFETCH(QString, oneOf1ExpectedJson);
    QFETCH(bool, isOneOfMapOfDogsValid);
    QFETCH(bool, isOneOfMapOfDogsSet);
    QFETCH(bool, isOneOfArrayOfMapDucksValid);
    QFETCH(bool, isOneOfArrayOfMapDucksSet);
    QFETCH(bool, isValid);
    QFETCH(bool, isSet);

    QCOMPARE(rootMapModels.asJson(), expectedJson);
    QCOMPARE(rootMapModels.asJsonValue(), expectedJsonValue);
    QCOMPARE(mapDogAsJson(rootMapModels.getOneOfQMapQStringDog()),
             oneOf0ExpectedJson);
    QCOMPARE(rootMapModels.isOneOfQMapQStringDogValid(),
             isOneOfMapOfDogsValid);
    QCOMPARE(rootMapModels.isOneOfQMapQStringDogSet(),
             isOneOfMapOfDogsSet);
    QCOMPARE(listMapDuckAsJson(rootMapModels.getOneOfQListQMapQStringDuck()),
             oneOf1ExpectedJson);
    QCOMPARE(rootMapModels.isOneOfQListQMapQStringDuckValid(),
             isOneOfArrayOfMapDucksValid);
    QCOMPARE(rootMapModels.isOneOfQListQMapQStringDuckSet(),
             isOneOfArrayOfMapDucksSet);
    QCOMPARE(rootMapModels.isValid(), isValid);
    QCOMPARE(rootMapModels.isSet(), isSet);
}

QTEST_MAIN(OneOfTest)
#include "tst_oneof.moc"
