/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef ASSETPROCESSORSERVERUNITTEST_H
#define ASSETPROCESSORSERVERUNITTEST_H

#if !defined(Q_MOC_RUN)
#include "UnitTestRunner.h"
#include "native/utilities/IniConfiguration.h"
#include <QString>
#endif

class ApplicationServer;
class ConnectionManager;

class AssetProcessorServerUnitTest
    : public UnitTestRun
{
    Q_OBJECT
public:
    AssetProcessorServerUnitTest();
    ~AssetProcessorServerUnitTest();
    void AssetProcessorServerTest();
    virtual void StartTest() override;
    virtual int UnitTestPriority() const override;

public Q_SLOTS:
    void RunFirstPartOfUnitTestsForAssetProcessorServer();
    void AssetProcessorConnectionStressTest();
    void ConnectionErrorForNonProxyMode(unsigned int connId, QString error);

private:
    void RunAssetProcessorConnectionStressTest(bool failNegotiation);
    AZStd::unique_ptr<ApplicationServer> m_applicationServer;
    ConnectionManager* m_connectionManager;
    IniConfiguration m_iniConfiguration;

    QMetaObject::Connection m_connection;

    int m_numberOfDisconnectionReceived = 0;
    unsigned int m_connectionId = 0;

    bool m_gotNegotiationWithSelfError = false;
};

#endif // ASSETPROCESSORSERVERUNITTEST_H
