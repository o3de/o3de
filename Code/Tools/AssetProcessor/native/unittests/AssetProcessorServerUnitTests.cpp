/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/unittests/AssetProcessorServerUnitTests.h>
#include <native/unittests/UnitTestUtils.h>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <native/connection/connection.h>
#include <native/connection/connectionManager.h>
#include <native/utilities/assetUtils.h> // for AssetUtilities::ComputeProjectName
#include <native/utilities/BatchApplicationManager.h>
#include <native/utilities/ApplicationServer.h>
#include <native/utilities/BatchApplicationServer.h>
#include <native/utilities/PlatformConfiguration.h>
#include <QApplication>

#define FEATURE_TEST_LISTEN_PORT 12125

// Enable this define only if you are debugging a deadlock/timing issue etc in the AssetProcessorConnection, which you are unable to reproduce otherwise.
// enabling this define will result in a lot more number of connections that connect/disconnect with AP and therefore will result in the unit tests taking a lot more time to complete
// if you do enable this, consider disabling the timeout detection in AssetProcessorTests ("Legacy test deadlocked or timed out.") since it can take a long time to run.
//#define DEBUG_ASSETPROCESSORCONNECTION
#if defined(DEBUG_ASSETPROCESSORCONNECTION)
#define NUMBER_OF_CONNECTION 16
#define NUMBER_OF_TRIES 10
#define NUMBER_OF_ITERATION  100
#else
// NUMBER_OF_CONNECTION is how many parallel threads to create that will be starting and killing connections
#define NUMBER_OF_CONNECTION 4
// NUMBER_OF_TRIES is how many times each thread tries to disconnect and reconnect before finishing.
#define NUMBER_OF_TRIES 5
// NUMBER_OF_ITERATION is how many times the entire test is restarted
#define NUMBER_OF_ITERATION 2
#endif

namespace UnitTest
{
    AssetProcessorServerUnitTest::AssetProcessorServerUnitTest()
        : AssetProcessorUnitTestBase()
    {
    }

    AssetProcessorServerUnitTest::~AssetProcessorServerUnitTest()
    {
    }

    void AssetProcessorServerUnitTest::SetUp()
    {
        AssetProcessorUnitTestBase::SetUp();

        m_applicationServer = AZStd::make_unique<BatchApplicationServer>();
        m_applicationServer->startListening(FEATURE_TEST_LISTEN_PORT); // a port that is not the normal port
        connect(m_applicationServer.get(), SIGNAL(newIncomingConnection(qintptr)), ConnectionManager::Get(), SLOT(NewConnection(qintptr)));
    }

    void AssetProcessorServerUnitTest::RunAssetProcessorConnectionStressTest(bool failNegotiation)
    {
        AZStd::string azBranchToken;
        AzFramework::ApplicationRequests::Bus::Broadcast(
            &AzFramework::ApplicationRequests::CalculateBranchTokenForEngineRoot, azBranchToken);

        QString branchToken(azBranchToken.c_str());
        
        if (failNegotiation)
        {
            branchToken = branchToken.append("invalid"); // invalid branch token will result in negotiation to fail
        }

        AZStd::atomic_int numberOfConnection(0);
        AZStd::atomic_bool failureOccurred = false;

        enum : int
        {
            totalConnections = NUMBER_OF_CONNECTION * NUMBER_OF_TRIES
        };

        AZStd::function<void(int)> StartConnection =
            [&branchToken, &numberOfConnection, &failureOccurred, failNegotiation](int numTimeWait)
        {
            for (int idx = 0; idx < NUMBER_OF_TRIES; ++idx)
            {
                AzFramework::AssetSystem::AssetProcessorConnection connection;
                connection.Configure(
                    branchToken.toUtf8().data(), "pc", "UNITTEST",
                    AssetUtilities::ComputeProjectName()
                        .toUtf8()
                        .constData()); // UNITTEST identifier will skip the processID validation during negotiation
                connection.Connect("127.0.0.1", FEATURE_TEST_LISTEN_PORT);
                while (!connection.IsConnected() && !connection.NegotiationFailed())
                {
                    AZStd::this_thread::yield();
                }

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(numTimeWait + idx));
                numberOfConnection.fetch_sub(1);

                if (connection.NegotiationFailed() != failNegotiation)
                {
                    failureOccurred = true;
                }

                EXPECT_EQ(connection.NegotiationFailed(), failNegotiation);
            }
        };

        AZStd::vector<AZStd::thread> assetProcessorConnectionList;
        for (int iteration = 0; iteration < NUMBER_OF_ITERATION; ++iteration)
        {
#if defined(DEBUG_ASSETPROCESSORCONNECTION)
            printf("Iteration %4i/%4i...\n", iteration, NUMBER_OF_ITERATION);
#endif
            numberOfConnection = totalConnections;

            for (int idx = 0; idx < NUMBER_OF_CONNECTION; ++idx)
            {
                // each thread should sleep after each test for a different amount of time so that they
                // end up trying all different overlapping parts of the code.
                int sleepTime = iteration * (idx + 1);
                assetProcessorConnectionList.push_back(AZStd::thread(AZStd::bind(StartConnection, sleepTime)));
            };

            // We need to process all events, since AssetProcessorServer is also on the same thread
            while (numberOfConnection.load() && !failureOccurred)
            {
                QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
                QCoreApplication::processEvents();
            }

            EXPECT_FALSE(failureOccurred);

            for (int idx = 0; idx < NUMBER_OF_CONNECTION; ++idx)
            {
                if (assetProcessorConnectionList[idx].joinable())
                {
                    assetProcessorConnectionList[idx].join();
                }
            }

            assetProcessorConnectionList.clear();
        }
    }

    void AssetProcessorServerUnitTest::AssetProcessorConnectionStressTest()
    {
        // UnitTest for testing the AssetProcessorConnection by creating lot of connections that connects to AP and then disconnecting them
        // at different times This test should detect any deadlocks that can arise due to rapidly connecting/disconnecting connections
        UnitTestUtils::AssertAbsorber assertAbsorber;

        // Testing the case when negotiation succeeds
        RunAssetProcessorConnectionStressTest(false);

        EXPECT_EQ(assertAbsorber.m_numErrorsAbsorbed, 0);
        EXPECT_EQ(assertAbsorber.m_numAssertsAbsorbed, 0);

        // Testing the case when negotiation fails
        RunAssetProcessorConnectionStressTest(true);

        EXPECT_EQ(assertAbsorber.m_numErrorsAbsorbed, 0);
        EXPECT_EQ(assertAbsorber.m_numAssertsAbsorbed, 0);
    }

    void AssetProcessorServerUnitTest::ConnectionErrorForNonProxyMode(unsigned int connId, QString error)
    {
        if ((connId == 10 || connId == 11))
        {
            if (QString::compare(error, "Attempted to negotiate with self") == 0)
            {
                m_gotNegotiationWithSelfError = true;
            }
            ++m_numberOfDisconnectionReceived;
        }

        if (m_numberOfDisconnectionReceived == 2)
        {
            ConnectionManager::Get()->removeConnection(m_connectionId);
            disconnect(m_connection);
            EXPECT_TRUE(m_gotNegotiationWithSelfError);
            if (m_gotNegotiationWithSelfError)
            {
                AssetProcessorConnectionStressTest();
            }
            m_eventWasPosted = true;
        }
    }

    TEST_F(AssetProcessorServerUnitTest, RunFirstPartOfUnitTestsForAssetProcessorServer)
    {
        m_numberOfDisconnectionReceived = 0;
        m_connection = connect(
            ConnectionManager::Get(), SIGNAL(ConnectionError(unsigned int, QString)), this,
            SLOT(ConnectionErrorForNonProxyMode(unsigned int, QString)));
        m_connectionId = ConnectionManager::Get()->addConnection();
        Connection* connection = ConnectionManager::Get()->getConnection(m_connectionId);
        connection->SetPort(FEATURE_TEST_LISTEN_PORT);
        connection->SetIpAddress("127.0.0.1");
        connection->SetAutoConnect(true);

        
        QElapsedTimer time;
        time.start();

        // This is still time out based because it's waiting on a few error events to continue.
        // The actual test runs when ConnectionErrorForNonProxyMode is triggered with the expected
        // error messages. A shorter timeout can make this get missed in some environments.
        const int testTimeoutMS = 120 * 1000;
        while (time.elapsed() < testTimeoutMS && !m_eventWasPosted)
        {
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
            QCoreApplication::processEvents();
        }
        EXPECT_TRUE(m_eventWasPosted);
    }

} // namespace UnitTest

