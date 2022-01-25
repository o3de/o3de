/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETPROCESSORSERVERUNITTEST_H
#define ASSETPROCESSORSERVERUNITTEST_H

#if !defined(Q_MOC_RUN)
#include "UnitTestRunner.h"
#include "native/utilities/IniConfiguration.h"
#include <AzCore/std/smart_ptr/unique_ptr.h>
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
