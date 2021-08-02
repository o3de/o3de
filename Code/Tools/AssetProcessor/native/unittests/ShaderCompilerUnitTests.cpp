/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ShaderCompilerUnitTests.h"
#include "native/connection/connectionManager.h"
#include "native/connection/connection.h"
#include "native/utilities/assetUtils.h"

#define UNIT_TEST_CONNECT_PORT 12125

ShaderCompilerUnitTest::ShaderCompilerUnitTest()
{
    m_connectionManager = ConnectionManager::Get();
    connect(this, SIGNAL(StartUnitTestForGoodShaderCompiler()), this, SLOT(UnitTestForGoodShaderCompiler()));
    connect(this, SIGNAL(StartUnitTestForFirstBadShaderCompiler()), this, SLOT(UnitTestForFirstBadShaderCompiler()));
    connect(this, SIGNAL(StartUnitTestForSecondBadShaderCompiler()), this, SLOT(UnitTestForSecondBadShaderCompiler()));
    connect(this, SIGNAL(StartUnitTestForThirdBadShaderCompiler()), this, SLOT(UnitTestForThirdBadShaderCompiler()));
    connect(&m_shaderCompilerManager, SIGNAL(sendErrorMessageFromShaderJob(QString, QString, QString, QString)), this, SLOT(ReceiveShaderCompilerErrorMessage(QString, QString, QString, QString)));

    m_shaderCompilerManager.setIsUnitTesting(true);
    m_connectionManager->RegisterService(AssetUtilities::ComputeCRC32Lowercase("ShaderCompilerProxyRequest"), AZStd::bind(&ShaderCompilerManager::process, &m_shaderCompilerManager, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4));
    ContructPayloadForShaderCompilerServer(m_testPayload);
}

ShaderCompilerUnitTest::~ShaderCompilerUnitTest()
{
    m_connectionManager->removeConnection(m_connectionId);
}

void ShaderCompilerUnitTest::ContructPayloadForShaderCompilerServer(QByteArray& payload)
{
    QString testString = "This is a test string";
    QString testServerList = "127.0.0.3,198.51.100.0,127.0.0.1"; // note - 198.51.100.0 is in the 'test' range that will never be assigned to anyone.
    unsigned int testServerListLength = static_cast<unsigned int>(testServerList.size());
    unsigned short testServerPort = 12348;
    unsigned int testRequestId = 1;
    qint64 testStringLength = static_cast<qint64>(testString.size());
    payload.resize(static_cast<unsigned int>(testStringLength));
    memcpy(payload.data(), (testString.toStdString().c_str()), testStringLength);
    unsigned int payloadSize = payload.size();
    payload.resize(payloadSize + 1 + static_cast<unsigned int>(testServerListLength) + 1 + sizeof(unsigned short) + sizeof(unsigned int) + sizeof(unsigned int));
    char* dataStart = payload.data() + payloadSize;
    *dataStart = 0;// null
    memcpy(payload.data() + payloadSize + 1, (testServerList.toStdString().c_str()), testServerListLength);
    dataStart += 1 + testServerListLength;
    *dataStart = 0; //null
    memcpy(payload.data() + payloadSize + 1 + testServerListLength + 1, reinterpret_cast<char*>(&testServerPort), sizeof(unsigned short));
    memcpy(payload.data() + payloadSize + 1 + testServerListLength + 1 + sizeof(unsigned short), reinterpret_cast<char*>(&testServerListLength), sizeof(unsigned int));
    memcpy(payload.data() + payloadSize + 1 + testServerListLength + 1 + sizeof(unsigned short) + sizeof(unsigned int), reinterpret_cast<char*>(&testRequestId), sizeof(unsigned int));
}

void ShaderCompilerUnitTest::StartTest()
{
    m_connectionId = m_connectionManager->addConnection();
    Connection* connection = m_connectionManager->getConnection(m_connectionId);
    connection->SetPort(UNIT_TEST_CONNECT_PORT);
    connection->SetIpAddress("127.0.0.1");
    connection->SetAutoConnect(true);
    UnitTestForGoodShaderCompiler();
}

int ShaderCompilerUnitTest::UnitTestPriority() const
{
    return -4;
}

void ShaderCompilerUnitTest::UnitTestForGoodShaderCompiler()
{
    AZ_TracePrintf("ShaderCompilerUnitTest", "  ... Starting test of 'good' shader compiler...\n");
    m_shaderCompilerManager.m_sendResponseCallbackFn = AZStd::bind(&ShaderCompilerUnitTest::VerifyPayloadForGoodShaderCompiler, this , AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4);
    m_server.Init("127.0.0.1", 12348);
    m_server.setServerStatus(UnitTestShaderCompilerServer::GoodServer);
    m_connectionManager->SendMessageToService(m_connectionId, AssetUtilities::ComputeCRC32Lowercase("ShaderCompilerProxyRequest"), 0, m_testPayload);
}

void ShaderCompilerUnitTest::UnitTestForFirstBadShaderCompiler()
{
    AZ_TracePrintf("ShaderCompilerUnitTest", "  ... Starting test of 'bad' shader compiler... (Incomplete Payload)\n");
    m_shaderCompilerManager.m_sendResponseCallbackFn = AZStd::bind(&ShaderCompilerUnitTest::VerifyPayloadForFirstBadShaderCompiler, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4);
    m_server.setServerStatus(UnitTestShaderCompilerServer::BadServer_SendsIncompletePayload);
    m_connectionManager->SendMessageToService(m_connectionId, AssetUtilities::ComputeCRC32Lowercase("ShaderCompilerProxyRequest"), 0, m_testPayload);
}

void ShaderCompilerUnitTest::UnitTestForSecondBadShaderCompiler()
{
    AZ_TracePrintf("ShaderCompilerUnitTest", "  ... Starting test of 'bad' shader compiler... (Payload followed by disconnection)\n");
    m_shaderCompilerManager.m_sendResponseCallbackFn = AZStd::bind(&ShaderCompilerUnitTest::VerifyPayloadForSecondBadShaderCompiler, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4);
    m_server.setServerStatus(UnitTestShaderCompilerServer::BadServer_ReadsPayloadAndDisconnect);
    m_connectionManager->SendMessageToService(m_connectionId, AssetUtilities::ComputeCRC32Lowercase("ShaderCompilerProxyRequest"), 0, m_testPayload);
}

void ShaderCompilerUnitTest::UnitTestForThirdBadShaderCompiler()
{
    AZ_TracePrintf("ShaderCompilerUnitTest", "  ... Starting test of 'bad' shader compiler... (Connect but disconnect without data)\n");
    m_shaderCompilerManager.m_sendResponseCallbackFn = AZStd::bind(&ShaderCompilerUnitTest::VerifyPayloadForThirdBadShaderCompiler, this, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3, AZStd::placeholders::_4);
    m_server.setServerStatus(UnitTestShaderCompilerServer::BadServer_DisconnectAfterConnect);
    m_server.startServer();
    m_connectionManager->SendMessageToService(m_connectionId, AssetUtilities::ComputeCRC32Lowercase("ShaderCompilerProxyRequest"), 0, m_testPayload);
}

void ShaderCompilerUnitTest::VerifyPayloadForGoodShaderCompiler(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload)
{
    (void) connId;
    (void) type;
    (void) serial;
    m_shaderCompilerManager.m_sendResponseCallbackFn = nullptr;

    unsigned int messageSize;
    quint8 status;
    QByteArray payloadToCheck;
    unsigned int requestId;
    memcpy((&messageSize), payload.data(), sizeof(unsigned int));
    memcpy((&status), payload.data() + sizeof(unsigned int), sizeof(unsigned char));
    payloadToCheck.resize(messageSize);
    memcpy((payloadToCheck.data()), payload.data() + sizeof(unsigned int) + sizeof(unsigned char), messageSize);
    memcpy((&requestId), payload.data() + sizeof(unsigned int) + sizeof(unsigned char) + messageSize, sizeof(unsigned int));
    QString outgoingTestString = "Test string validated";
    if (QString::compare(QString(payloadToCheck), outgoingTestString, Qt::CaseSensitive) != 0)
    {
        Q_EMIT UnitTestFailed("Unit Test for Good Shader Compiler Failed");
        return;
    }
    Q_EMIT StartUnitTestForFirstBadShaderCompiler();
}

void ShaderCompilerUnitTest::VerifyPayloadForFirstBadShaderCompiler(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload)
{
    (void) connId;
    (void) type;
    (void) serial;
    m_shaderCompilerManager.m_sendResponseCallbackFn = nullptr;
    QString error = "Remote IP is taking too long to respond: 127.0.0.1";
    if ((payload.size() != 4) || (QString::compare(m_lastShaderCompilerErrorMessage, error, Qt::CaseSensitive) != 0))
    {
        Q_EMIT UnitTestFailed("Unit Test for First Bad Shader Compiler Failed");
        return;
    }
    m_lastShaderCompilerErrorMessage.clear();
    Q_EMIT StartUnitTestForSecondBadShaderCompiler();
}

void ShaderCompilerUnitTest::VerifyPayloadForSecondBadShaderCompiler(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload)
{
    (void) connId;
    (void) type;
    (void) serial;
    m_shaderCompilerManager.m_sendResponseCallbackFn = nullptr;
    QString error = "Remote IP is taking too long to respond: 127.0.0.1";
    if ((payload.size() != 4) || (QString::compare(m_lastShaderCompilerErrorMessage, error, Qt::CaseSensitive) != 0))
    {
        Q_EMIT UnitTestFailed("Unit Test for Second Bad Shader Compiler Failed");
        return;
    }
    m_lastShaderCompilerErrorMessage.clear();
    Q_EMIT StartUnitTestForThirdBadShaderCompiler();
}

void ShaderCompilerUnitTest::VerifyPayloadForThirdBadShaderCompiler(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload)
{
    (void) connId;
    (void) type;
    (void) serial;
    m_shaderCompilerManager.m_sendResponseCallbackFn = nullptr;
    QString error = "Remote IP is taking too long to respond: 127.0.0.1";
    if ((payload.size() != 4) || (QString::compare(m_lastShaderCompilerErrorMessage, error, Qt::CaseSensitive) != 0))
    {
        Q_EMIT UnitTestFailed("Unit Test for Third Bad Shader Compiler Failed");
        return;
    }
    m_lastShaderCompilerErrorMessage.clear();
    Q_EMIT UnitTestPassed();
}

void ShaderCompilerUnitTest::ReceiveShaderCompilerErrorMessage(QString error, QString server, QString timestamp, QString payload)
{
    (void) server;
    (void) timestamp;
    (void) payload;
    m_lastShaderCompilerErrorMessage = error;
}


REGISTER_UNIT_TEST(ShaderCompilerUnitTest)

