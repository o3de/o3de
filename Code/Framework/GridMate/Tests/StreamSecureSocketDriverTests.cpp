/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Tests.h"

#include <GridMate/Carrier/StreamSecureSocketDriver.h>

#if AZ_TRAIT_GRIDMATE_ENABLE_OPENSSL

#include <AzCore/Math/Random.h>
#include <AzCore/State/HSM.h>

#include <array>

using namespace GridMate;

namespace Certificates
{
    extern const char* g_untrustedCertPEM;
    extern const char* g_untrustedPrivateKeyPEM;
}

namespace UnitTest
{
    bool ConnectStreamSecureSocketDriverServerClient(GridMate::StreamSecureSocketDriver& server, GridMate::StreamSecureSocketDriver& client, const AZ::u32 attempts)
    {
        auto serverAddressName = GridMate::SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());
        auto driverAddress = AZStd::static_pointer_cast<GridMate::SocketDriverAddress>(client.CreateDriverAddress(serverAddressName));

        for (AZ::u32 i = 0; i < attempts; ++i)
        {
            server.Update();
            client.Update();

            if (server.GetNumberOfConnections() > 0 && client.IsConnectedTo(driverAddress))
            {
                return true;
            }
        }
        return false;
    }

    bool InitializeSecurityForServer(GridMate::StreamSecureSocketDriver& server, AZ::u16 port = 0)
    {
        GridMate::StreamSecureSocketDriver::StreamSecureSocketDriverDesc desc;
        desc.m_certificatePEM = Certificates::g_untrustedCertPEM;
        desc.m_privateKeyPEM = Certificates::g_untrustedPrivateKeyPEM;
        auto ret = server.InitializeSecurity(GridMate::Driver::BSD_AF_INET, nullptr, port, 1024 * 64, 1024 * 64, desc);
        AZ_TEST_ASSERT(ret == GridMate::Driver::EC_OK);
        return (ret == GridMate::Driver::EC_OK);
    }

    bool InitializeSecurityAndConnectForClient(GridMate::StreamSecureSocketDriver& client, const GridMate::StreamSecureSocketDriver& server)
    {
        GridMate::StreamSecureSocketDriver::StreamSecureSocketDriverDesc desc;
        desc.m_certificateAuthorityPEM = Certificates::g_untrustedCertPEM;
        auto ret = client.InitializeSecurity(GridMate::Driver::BSD_AF_INET, nullptr, 0, 1024 * 64, 1024 * 64, desc);
        AZ_TEST_ASSERT(ret == GridMate::Driver::EC_OK);

        auto serverAddressName = GridMate::SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());
        auto driverAddress = AZStd::static_pointer_cast<GridMate::SocketDriverAddress>(client.CreateDriverAddress(serverAddressName));
        ret = client.ConnectTo(driverAddress);
        AZ_TEST_ASSERT(ret == GridMate::Driver::EC_OK);
        return (ret == GridMate::Driver::EC_OK);
    }

    bool ConnectStreamSecureSocketInitializeAndConnect(GridMate::StreamSecureSocketDriver& server, GridMate::StreamSecureSocketDriver& client, const AZ::u32 attempts)
    {
        if(InitializeSecurityForServer(server))
        {
            server.StartListen(32);
            if (InitializeSecurityAndConnectForClient(client, server))
            {
                return ConnectStreamSecureSocketDriverServerClient(server, client, attempts);
            }
        }
        return false;
    }

    using TestPacket = AZStd::vector<char>;
    using PacketQueue = AZStd::queue<TestPacket>;
    using SocketAddressPtr = AZStd::intrusive_ptr<GridMate::SocketDriverAddress>;
    using DriverAddressPtr = AZStd::intrusive_ptr<GridMate::DriverAddress>;

    template <const AZ::u32 SIZE = 256>
    struct TestPacketGenerator
    {
        size_t CreatePayload(char* buffer)
        {
            uint32_t randomSize;
            m_rand.GetRandom(randomSize);
            size_t numChars = (randomSize % SIZE) + 1;
            m_rand.GetRandom(buffer, numChars);
            return numChars;
        }

        TestPacket& Generate()
        {
            size_t bytesSize = CreatePayload(m_buffer.data());
            m_packetQueue.push({ &m_buffer[0], &m_buffer[0] + bytesSize });
            return m_packetQueue.back();
        }

        AZ::BetterPseudoRandom m_rand;
        PacketQueue m_packetQueue;
        std::array<char, SIZE> m_buffer;
    };

    class DISABLED_StreamSecureSocketDriverTestsBindSocketEmpty
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            {
                StreamSecureSocketDriver::StreamSecureSocketDriverDesc desc;
                desc.m_certificatePEM = Certificates::g_untrustedCertPEM;
                desc.m_privateKeyPEM = Certificates::g_untrustedPrivateKeyPEM;
                StreamSecureSocketDriver server(32);
                auto ret = server.InitializeSecurity(1, nullptr, 0, 1024 * 64, 1024 * 64, desc);
                AZ_TEST_ASSERT(ret == GridMate::Driver::EC_OK);
            }
            {
                StreamSecureSocketDriver::StreamSecureSocketDriverDesc desc;
                desc.m_certificateAuthorityPEM = Certificates::g_untrustedCertPEM;
                StreamSecureSocketDriver client(1);
                auto ret = client.InitializeSecurity(1, nullptr, 0, 1024 * 64, 1024 * 64, desc);
                AZ_TEST_ASSERT(ret == GridMate::Driver::EC_OK);
            }
        }
    };

    class DISABLED_StreamSecureSocketDriverTestsConnection
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            StreamSecureSocketDriver server(32);
            StreamSecureSocketDriver client(1);
            AZ_TEST_ASSERT(ConnectStreamSecureSocketInitializeAndConnect(server, client, 1000));
        }
    };

    class DISABLED_StreamSecureSocketDriverTestsConnectionAndHelloWorld
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            StreamSecureSocketDriver server(32);
            StreamSecureSocketDriver client(1);
            if (!ConnectStreamSecureSocketInitializeAndConnect(server, client, 1000))
            {
                AZ_TEST_ASSERT(false && "Could not connect");
            }

            auto serverAddressName = SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());
            auto serverAddress = AZStd::static_pointer_cast<SocketDriverAddress>(client.CreateDriverAddress(serverAddressName));

            char packet[] = { "Hello Server" };
            const int kNumTimes = 100;
            int i;
            for (i = 0; i < kNumTimes; ++i)
            {
                server.Update();
                client.Update();

                if (i == 0)
                {
                    AZ_TEST_ASSERT(client.Send(serverAddress, packet, sizeof(packet)) == GridMate::Driver::EC_OK);
                }

                AZStd::intrusive_ptr<DriverAddress> from;
                char buffer[64];
                AZ::u32 bytesRead = server.Receive(buffer, sizeof(buffer), from);
                // got a packet?
                if (bytesRead > 0)
                {
                    AZ_TEST_ASSERT(bytesRead == sizeof(packet));
                    AZ_TEST_ASSERT(0 == memcmp(buffer, packet, sizeof(packet)));
                    break;
                }
            }
            AZ_TEST_ASSERT(i < kNumTimes && "Did not send packet");
        }
    };

    class DISABLED_StreamSecureSocketDriverTestsPingPong
        : public GridMateMPTestFixture
    {
    public:
        static const AZ::u32 kPacketSize = 128;

        struct Service
        {
            StreamSecureSocketDriver m_driver;
            TestPacketGenerator<kPacketSize> m_packetGenerator;
            PacketQueue m_sentPackets;
            PacketQueue m_receivedPackets;
            SocketAddressPtr m_thisAddress;

            DriverAddressPtr GetPacket()
            {
                DriverAddressPtr f;
                char buffer[kPacketSize];
                AZ::u32 bytesRead = m_driver.Receive(buffer, sizeof(buffer), f);
                if (bytesRead > 0)
                {
                    m_receivedPackets.push({ &buffer[0], &buffer[0] + bytesRead });
                    return f;
                }
                return {};
            }

            bool SendNewPacketToServer()
            {
                AZ_TEST_ASSERT(m_driver.GetNumberOfConnections() == 1);
                auto firstAddress = *m_driver.m_addressMap.begin();
                DriverAddressPtr serverAddress = m_driver.CreateDriverAddress(firstAddress.ToString());
                return SendNewPacketTo(serverAddress);
            }

            bool SendNewPacketTo(DriverAddressPtr target)
            {
                return SendPacketTo(target, { m_packetGenerator.Generate() });
            }

            bool SendPacketTo(DriverAddressPtr target, const TestPacket& packet)
            {
                m_sentPackets.push(packet);
                return m_driver.Send(target, &packet[0], static_cast<AZ::u32>(packet.size())) == Driver::EC_OK;
            }
        };

        // state machine
        enum TestState
        {
            TS_TOP,
            TS_START,              // starts by sending a packet from PING to SERVER
            TS_SERVER_GET_PING,    // SERVER waiting for packet from PING client
            TS_PING_GET_SERVER,    // PING waits for packet from SERVER
            TS_SERVER_GET_PONG,    // SERVER waiting for packet from PONG client
            TS_PONG_GET_SERVER,    // PONG waits for packet from SERVER
            TS_IN_ERROR            // state machine has gone into error mode, fails the test
        };
        // machine events
        enum TestEvents
        {
            TE_UPDATE = 1,
        };

        Service m_server;
        Service m_clientPing;
        Service m_clientPong;
        AZ::HSM m_stateMachine;

        bool HaltMachineInError(AZ::HSM& sm, const char* msg)
        {
            (void)msg;
            AZ_TracePrintf("GridMateTest", "Failed %s while in state %d \n", msg, sm.GetCurrentState());
            AZ_TEST_ASSERT(false);
            sm.Transition(TS_IN_ERROR);
            return true;
        }

        bool OnStateTop(AZ::HSM& sm, const AZ::HSM::Event& e)
        {
            (void)sm;
            (void)e;
            return false;
        }

        bool OnStateStart(AZ::HSM& sm, const AZ::HSM::Event& e)
        {
            if (e.id != TE_UPDATE)
            {
                return true;
            }

            if (!ConnectStreamSecureSocketInitializeAndConnect(m_server.m_driver, m_clientPing.m_driver, 1000))
            {
                return HaltMachineInError(sm, "Could not init m_clientPing.m_driver");
            }
            if (!InitializeSecurityAndConnectForClient(m_clientPong.m_driver, m_server.m_driver))
            {
                return HaltMachineInError(sm, "Could not connect m_clientPong.m_driver");
            }
            if (!ConnectStreamSecureSocketDriverServerClient(m_server.m_driver, m_clientPong.m_driver, 1000))
            {
                return HaltMachineInError(sm, "Could not connect m_clientPong.m_driver");
            }
            auto serverAddressName = SocketDriverCommon::IPPortToAddressString("127.0.0.1", m_server.m_driver.GetPort());
            m_server.m_thisAddress = AZStd::static_pointer_cast<SocketDriverAddress>(m_server.m_driver.CreateDriverAddress(serverAddressName));

            if (m_clientPing.SendNewPacketToServer())
            {
                sm.Transition(TS_SERVER_GET_PING);
                return true;
            }
            return HaltMachineInError(sm, "Could not send first packet to server");
        }

        bool OnStateServerGetPing(AZ::HSM& sm, const AZ::HSM::Event& e)
        {
            if (e.id != TE_UPDATE)
            {
                return true;
            }

            auto pingClientAddress = m_server.GetPacket();
            if (pingClientAddress == nullptr)
            {
                // nothing to do yet
                return false;
            }
            m_clientPing.m_thisAddress = AZStd::static_pointer_cast<SocketDriverAddress>(pingClientAddress);

            // next state depends on if PONG has already sent a packet or not
            if (m_clientPong.m_thisAddress == nullptr)
            {
                if (m_clientPong.SendNewPacketToServer())
                {
                    sm.Transition(TS_SERVER_GET_PONG);
                    return true;
                }
            }
            else
            {
                // send the last packet to PONG
                if (m_server.SendPacketTo(m_clientPong.m_thisAddress, m_server.m_receivedPackets.back()))
                {
                    sm.Transition(TS_PONG_GET_SERVER);
                    return true;
                }
            }

            return HaltMachineInError(sm, "Unexpected state or return value");
        }

        bool OnStatePingGetServer(AZ::HSM& sm, const AZ::HSM::Event& e)
        {
            if (e.id != TE_UPDATE)
            {
                return true;
            }

            if (m_clientPing.GetPacket() == nullptr)
            {
                return false;
            }

            if (m_clientPing.SendNewPacketToServer())
            {
                sm.Transition(TS_SERVER_GET_PING);
                return true;
            }

            return HaltMachineInError(sm, "Unexpected state or return value");
        }

        bool OnStateServerGetPong(AZ::HSM& sm, const AZ::HSM::Event& e)
        {
            if (e.id != TE_UPDATE)
            {
                return true;
            }

            auto pongClientAddress = m_server.GetPacket();
            if (pongClientAddress == nullptr)
            {
                // nothing to do yet
                return false;
            }
            m_clientPong.m_thisAddress = AZStd::static_pointer_cast<SocketDriverAddress>(pongClientAddress);

            AZ_TEST_ASSERT(m_clientPong.m_thisAddress != nullptr);
            AZ_TEST_ASSERT(m_clientPing.m_thisAddress != nullptr);

            // relay the packet to PING
            if (m_server.SendPacketTo(m_clientPing.m_thisAddress, m_server.m_receivedPackets.back()))
            {
                sm.Transition(TS_PING_GET_SERVER);
                return true;
            }

            return HaltMachineInError(sm, "Unexpected state or return value");
        }

        bool OnStatePongGetServer(AZ::HSM& sm, const AZ::HSM::Event& e)
        {
            if (e.id != TE_UPDATE)
            {
                return true;
            }

            if (m_clientPong.GetPacket() == nullptr)
            {
                return false;
            }

            if (m_clientPong.SendNewPacketToServer())
            {
                sm.Transition(TS_SERVER_GET_PONG);
                return true;
            }

            return HaltMachineInError(sm, "Unexpected state or return value");
        }

        bool OnStateInError(AZ::HSM& sm, const AZ::HSM::Event& e)
        {
            if (e.id != TE_UPDATE)
            {
                return true;
            }
            AZ_TracePrintf("GridMateTest", "Test failed\n");
            sm.Transition(TS_TOP);
            return true;
        }

        void BuildStateMachine()
        {
            m_stateMachine.SetStateHandler(AZ_HSM_STATE_NAME(TS_TOP), AZ::HSM::StateHandler(this, &DISABLED_StreamSecureSocketDriverTestsPingPong::OnStateTop), AZ::HSM::InvalidStateId, TS_START);
            m_stateMachine.SetStateHandler(AZ_HSM_STATE_NAME(TS_START), AZ::HSM::StateHandler(this, &DISABLED_StreamSecureSocketDriverTestsPingPong::OnStateStart), TS_TOP);
            m_stateMachine.SetStateHandler(AZ_HSM_STATE_NAME(TS_SERVER_GET_PING), AZ::HSM::StateHandler(this, &DISABLED_StreamSecureSocketDriverTestsPingPong::OnStateServerGetPing), TS_TOP);
            m_stateMachine.SetStateHandler(AZ_HSM_STATE_NAME(TS_PING_GET_SERVER), AZ::HSM::StateHandler(this, &DISABLED_StreamSecureSocketDriverTestsPingPong::OnStatePingGetServer), TS_TOP);
            m_stateMachine.SetStateHandler(AZ_HSM_STATE_NAME(TS_SERVER_GET_PONG), AZ::HSM::StateHandler(this, &DISABLED_StreamSecureSocketDriverTestsPingPong::OnStateServerGetPong), TS_TOP);
            m_stateMachine.SetStateHandler(AZ_HSM_STATE_NAME(TS_PONG_GET_SERVER), AZ::HSM::StateHandler(this, &DISABLED_StreamSecureSocketDriverTestsPingPong::OnStatePongGetServer), TS_TOP);
            m_stateMachine.SetStateHandler(AZ_HSM_STATE_NAME(TS_IN_ERROR), AZ::HSM::StateHandler(this, &DISABLED_StreamSecureSocketDriverTestsPingPong::OnStateInError), TS_TOP);
            m_stateMachine.Start();
        }

        void UpdateMachine()
        {
            m_server.m_driver.Update();
            m_clientPing.m_driver.Update();
            m_clientPong.m_driver.Update();
            m_stateMachine.Dispatch(TE_UPDATE);
        }

        void CompareResults()
        {
            m_clientPing.m_sentPackets.pop(); // the first one was a dummy packet
            AZ_TEST_ASSERT(m_clientPing.m_sentPackets.size() == m_clientPong.m_receivedPackets.size());

            while (!m_clientPing.m_sentPackets.empty())
            {
                auto packetPing = m_clientPing.m_sentPackets.front();
                auto packetPong = m_clientPong.m_receivedPackets.front();
                AZ_TEST_ASSERT(packetPing == packetPong);
                m_clientPing.m_sentPackets.pop();
                m_clientPong.m_receivedPackets.pop();
            }

            while (!m_clientPing.m_receivedPackets.empty())
            {
                auto packetPing = m_clientPing.m_receivedPackets.front();
                auto packetPong = m_clientPong.m_sentPackets.front();
                AZ_TEST_ASSERT(packetPing == packetPong);
                m_clientPing.m_receivedPackets.pop();
                m_clientPong.m_sentPackets.pop();
            }
        }

        void run()
        {
            BuildStateMachine();
            const int kNumTimes = 256;
            for (int loop = 0; loop < kNumTimes; ++loop)
            {
                UpdateMachine();
                if (m_stateMachine.GetCurrentState() == TS_IN_ERROR)
                {
                    AZ_TEST_ASSERT(false && "Error happened");
                    break;
                }
            }
            CompareResults();
        }
    };
}

GM_TEST_SUITE(StreamSecureSocketDriverTests)
    GM_TEST(DISABLED_StreamSecureSocketDriverTestsBindSocketEmpty);
    GM_TEST(DISABLED_StreamSecureSocketDriverTestsConnection);
    GM_TEST(DISABLED_StreamSecureSocketDriverTestsConnectionAndHelloWorld);
    GM_TEST(DISABLED_StreamSecureSocketDriverTestsPingPong);
GM_TEST_SUITE_END()

#endif // AZ_TRAIT_GRIDMATE_ENABLE_OPENSSL
