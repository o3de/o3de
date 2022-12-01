/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif
#include <QObject>

#include <native/unittests/AssetProcessorUnitTests.h>

class ApplicationServer;

namespace AzFramework
{
    class Application;
}

namespace UnitTest
{
    class AssetProcessorServerUnitTest
        : public QObject
        , public AssetProcessorUnitTestBase
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

        void RunAssetProcessorConnectionStressTest(bool failNegotiation);
        AZStd::unique_ptr<ApplicationServer> m_applicationServer;
        AZStd::unique_ptr<AzFramework::Application> m_application;

        AZStd::unique_ptr<AssetProcessorUnitTestAppManager> m_batchApplicationManager;
        
        QMetaObject::Connection m_connection;

        int m_numberOfDisconnectionReceived = 0;
        unsigned int m_connectionId = 0;

        bool m_gotNegotiationWithSelfError = false;
        bool m_eventWasPosted = false;
    };
} // namespace UnitTest
