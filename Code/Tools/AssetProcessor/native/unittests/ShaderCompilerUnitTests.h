/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef SHADERCOMPILERUNITTEST_H
#define SHADERCOMPILERUNITTEST_H

#if !defined(Q_MOC_RUN)
#include "UnitTestRunner.h"

#include <AzCore/std/functional.h>

#include "native/shadercompiler/shadercompilerManager.h"
//#include "native/shadercompiler/shadercompilerMessages.h"
#include "native/utilities/UnitTestShaderCompilerServer.h"
#include <QString>
#include <QByteArray>
#endif

class ConnectionManager;

class ShaderCompilerManagerForUnitTest : public ShaderCompilerManager
{
public:
    explicit ShaderCompilerManagerForUnitTest(QObject* parent = 0) : ShaderCompilerManager(parent) {};
    
    // for this test, we override sendResponse and make it so that it just calls a callback instead of actually sending it to the connection manager.
    void sendResponse(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload) override
    {
        if (m_sendResponseCallbackFn)
        {
            m_sendResponseCallbackFn(connId, type, serial, payload);
        }
    }
    AZStd::function<void(unsigned int, unsigned int, unsigned int, QByteArray)> m_sendResponseCallbackFn;
};

class ShaderCompilerUnitTest
    : public UnitTestRun
{
    Q_OBJECT
public:
    ShaderCompilerUnitTest();
    ~ShaderCompilerUnitTest();
    virtual void StartTest() override;
    virtual int UnitTestPriority() const override;
    void ContructPayloadForShaderCompilerServer(QByteArray& payload);

Q_SIGNALS:
    void StartUnitTestForGoodShaderCompiler();
    void StartUnitTestForFirstBadShaderCompiler();
    void StartUnitTestForSecondBadShaderCompiler();
    void StartUnitTestForThirdBadShaderCompiler();

public Q_SLOTS:
    void UnitTestForGoodShaderCompiler();
    void UnitTestForFirstBadShaderCompiler();
    void UnitTestForSecondBadShaderCompiler();
    void UnitTestForThirdBadShaderCompiler();
    void VerifyPayloadForGoodShaderCompiler(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void VerifyPayloadForFirstBadShaderCompiler(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void VerifyPayloadForSecondBadShaderCompiler(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void VerifyPayloadForThirdBadShaderCompiler(unsigned int connId, unsigned int type, unsigned int serial, QByteArray payload);
    void ReceiveShaderCompilerErrorMessage(QString error, QString server, QString timestamp, QString payload);


private:
    UnitTestShaderCompilerServer m_server;
    ShaderCompilerManagerForUnitTest m_shaderCompilerManager;
    ConnectionManager* m_connectionManager;
    QByteArray m_testPayload;
    QString m_lastShaderCompilerErrorMessage;
    unsigned int m_connectionId = 0;
};

#endif // SHADERCOMPILERUNITTEST_H


