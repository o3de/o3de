/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#define USE_OLD_TEST 0

#if USE_OLD_TEST

#if !defined(Q_MOC_RUN)
#include "UnitTestRunner.h"
#include "native/utilities/IniConfiguration.h"
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <QString>
#endif

class ApplicationServer;
class ConnectionManager;

class AssetProcessorServerUnitTest : public UnitTestRun
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
#else


#include <AzCore/UnitTest/TestTypes.h>
#include <native/utilities/IniConfiguration.h>
#include <QObject>

class ApplicationServer;
class ConnectionManager;

namespace AZ
{
    class ComponentApplication;
}

namespace AzFramework
{
    class Application;
}

namespace UnitTestUtils
{
    class AssertAbsorber;
}

namespace UnitTest
{
    class UnitTestAppManager;

    class AssetProcessorServerUnitTest
        : public QObject
        , public ScopedAllocatorSetupFixture
    {
        Q_OBJECT
    public:
        AssetProcessorServerUnitTest();
        ~AssetProcessorServerUnitTest();

    public Q_SLOTS:
        void AssetProcessorConnectionStressTest();
        void ConnectionErrorForNonProxyMode(unsigned int connId, QString error);

    protected:
        void SetUp() override;
        void TearDown() override;

        void RunAssetProcessorConnectionStressTest(bool failNegotiation);
        AZStd::unique_ptr<ApplicationServer> m_applicationServer;
        AZStd::unique_ptr<AzFramework::Application> m_application;
        IniConfiguration m_iniConfiguration;

        AZStd::unique_ptr<UnitTestAppManager> m_batchApplicationManager;
        
        QMetaObject::Connection m_connection;

        int m_numberOfDisconnectionReceived = 0;
        unsigned int m_connectionId = 0;

        bool m_gotNegotiationWithSelfError = false;
        bool m_eventWasPosted = false;
    };
} // namespace UnitTest
#endif
