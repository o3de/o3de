/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpNetworkInterface.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <AzNetworking/AutoGen/CorePackets.AutoPackets.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Time/TimeSystem.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    using namespace AzNetworking;

    class TestTcpConnectionListener
        : public IConnectionListener
    {
    public:
        ConnectResult ValidateConnect([[maybe_unused]] const IpAddress& remoteAddress, [[maybe_unused]] const IPacketHeader& packetHeader, [[maybe_unused]] ISerializer& serializer) override
        {
            return ConnectResult::Accepted;
        }

        void OnConnect([[maybe_unused]] IConnection* connection) override
        {
            ;
        }

        PacketDispatchResult OnPacketReceived([[maybe_unused]] IConnection* connection, const IPacketHeader& packetHeader, [[maybe_unused]] ISerializer& serializer) override
        {
            EXPECT_TRUE((packetHeader.GetPacketType() == static_cast<PacketType>(CorePackets::PacketType::InitiateConnectionPacket))
                     || (packetHeader.GetPacketType() == static_cast<PacketType>(CorePackets::PacketType::HeartbeatPacket)));
            return PacketDispatchResult::Failure;
        }

        void OnPacketLost([[maybe_unused]] IConnection* connection, [[maybe_unused]] PacketId packetId) override
        {

        }

        void OnDisconnect([[maybe_unused]] IConnection* connection, [[maybe_unused]] DisconnectReason reason, [[maybe_unused]] TerminationEndpoint endpoint) override
        {

        }
    };

    class TestTcpClient
    {
    public:
        TestTcpClient()
        {
            AZStd::string name = AZStd::string::format("TcpClient%d", ++s_numClients);
            m_name = name;
            m_clientNetworkInterface = AZ::Interface<INetworking>::Get()->CreateNetworkInterface(m_name, ProtocolType::Tcp, TrustZone::ExternalClientToServer, m_connectionListener);
            m_clientNetworkInterface->Connect(IpAddress(127, 0, 0, 1, 12345));
        }

        ~TestTcpClient()
        {
            AZ::Interface<INetworking>::Get()->DestroyNetworkInterface(m_name);
        }

        AZ::Name m_name;
        TestTcpConnectionListener m_connectionListener;
        INetworkInterface* m_clientNetworkInterface;
        static inline int32_t s_numClients = 0;
    };

    class TestTcpServer
    {
    public:
        TestTcpServer()
        {
            m_serverNetworkInterface = AZ::Interface<INetworking>::Get()->CreateNetworkInterface(m_name, ProtocolType::Tcp, TrustZone::ExternalClientToServer, m_connectionListener);
            m_serverNetworkInterface->Listen(12345);
        }

        ~TestTcpServer()
        {
            AZ::Interface<INetworking>::Get()->DestroyNetworkInterface(m_name);
        }

        AZ::Name m_name = AZ::Name(AZStd::string_view("TcpServer"));
        TestTcpConnectionListener m_connectionListener;
        INetworkInterface* m_serverNetworkInterface;
    };

    class TcpTransportTests
        : public AllocatorsFixture
    {
    public:

        void SetUp() override
        {
            SetupAllocator();
            AZ::NameDictionary::Create();

            m_loggerComponent = AZStd::make_unique<AZ::LoggerSystemComponent>();
            m_timeSystem = AZStd::make_unique<AZ::TimeSystem>();
            m_networkingSystemComponent = AZStd::make_unique<AzNetworking::NetworkingSystemComponent>();
        }

        void TearDown() override
        {
            m_networkingSystemComponent.reset();
            m_timeSystem.reset();
            m_loggerComponent.reset();

            AZ::NameDictionary::Destroy();
            TeardownAllocator();
        }

        AZStd::unique_ptr<AZ::LoggerSystemComponent> m_loggerComponent;
        AZStd::unique_ptr<AZ::TimeSystem> m_timeSystem;
        AZStd::unique_ptr<AzNetworking::NetworkingSystemComponent> m_networkingSystemComponent;
    };

    #if AZ_TRAIT_DISABLE_FAILED_NETWORKING_TESTS
    TEST_F(TcpTransportTests, DISABLED_TestSingleClient)
    #else
    TEST_F(TcpTransportTests, SUITE_sandbox_TestSingleClient)
    #endif // AZ_TRAIT_DISABLE_FAILED_NETWORKING_TESTS
    {
        TestTcpServer testServer;
        TestTcpClient testClient;

        constexpr AZ::TimeMs TotalIterationTimeMs = AZ::TimeMs{ 5000 };
        const AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();
        for (;;)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(25));
            m_networkingSystemComponent->OnTick(0.0f, AZ::ScriptTimePoint());
            bool timeExpired = (AZ::GetElapsedTimeMs() - startTimeMs > TotalIterationTimeMs);
            bool canTerminate = (testServer.m_serverNetworkInterface->GetConnectionSet().GetConnectionCount() == 1)
                             && (testClient.m_clientNetworkInterface->GetConnectionSet().GetConnectionCount() == 1);
            if (canTerminate || timeExpired)
            {
                break;
            }
        }

        EXPECT_EQ(testServer.m_serverNetworkInterface->GetConnectionSet().GetConnectionCount(), 1);
        EXPECT_EQ(testClient.m_clientNetworkInterface->GetConnectionSet().GetConnectionCount(), 1);

        const AZ::TimeMs timeoutMs = AZ::TimeMs{ 100 };
        testClient.m_clientNetworkInterface->SetTimeoutMs(timeoutMs);
        EXPECT_EQ(testClient.m_clientNetworkInterface->GetTimeoutMs(), timeoutMs);

        EXPECT_TRUE(testServer.m_serverNetworkInterface->StopListening());
    }

    #if AZ_TRAIT_DISABLE_FAILED_NETWORKING_TESTS
    TEST_F(TcpTransportTests, DISABLED_TestMultipleClients)
    #else
    TEST_F(TcpTransportTests, SUITE_sandbox_TestMultipleClients)
    #endif // AZ_TRAIT_DISABLE_FAILED_NETWORKING_TESTS
    {
        constexpr uint32_t NumTestClients = 50;

        TestTcpServer testServer;
        TestTcpClient testClient[NumTestClients];

        constexpr AZ::TimeMs TotalIterationTimeMs = AZ::TimeMs{ 5000 };
        const AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();
        for (;;)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(25));
            m_networkingSystemComponent->OnTick(0.0f, AZ::ScriptTimePoint());
            bool timeExpired = (AZ::GetElapsedTimeMs() - startTimeMs > TotalIterationTimeMs);
            bool canTerminate = testServer.m_serverNetworkInterface->GetConnectionSet().GetConnectionCount() == NumTestClients;
            for (uint32_t i = 0; i < NumTestClients; ++i)
            {
                canTerminate &= testClient[i].m_clientNetworkInterface->GetConnectionSet().GetConnectionCount() == 1;
            }
            if (canTerminate || timeExpired)
            {
                break;
            }
        }

        EXPECT_EQ(testServer.m_serverNetworkInterface->GetConnectionSet().GetConnectionCount(), NumTestClients);
        for (uint32_t i = 0; i < NumTestClients; ++i)
        {
            EXPECT_EQ(testClient[i].m_clientNetworkInterface->GetConnectionSet().GetConnectionCount(), 1);
        }
    }
}
