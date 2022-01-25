/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Tests.h"

#include <GridMate/Carrier/StreamSocketDriver.h>
#include <AzCore/Math/Random.h>

using namespace GridMate;

#define TEST_WITH_EXTERNAL_HOSTS 0

namespace UnitTest
{
    bool ConnectStreamSocketDriverServerClient(StreamSocketDriver& server, StreamSocketDriver& client, StreamSocketDriver::SocketDriverAddressPtr& serverAddress, const AZ::u32 attempts)
    {
        for (AZ::u32 i = 0; i < attempts; ++i)
        {
            server.Update();
            client.Update();

            if (server.GetNumberOfConnections() > 0 && client.IsConnectedTo(serverAddress))
            {
                return true;
            }
        }
        return false;
    }

    bool ConnectStreamSocketInitializeAndConnect(StreamSocketDriver& server, StreamSocketDriver& client, const AZ::u32 attempts)
    {
        server.Initialize();
        server.StartListen(1);

        client.Initialize();
        auto serverAddressName = SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());
        auto serverAddress = AZStd::static_pointer_cast<SocketDriverAddress>(client.CreateDriverAddress(serverAddressName));
        client.ConnectTo(serverAddress);

        return ConnectStreamSocketDriverServerClient(server, client, serverAddress, attempts);
    }

    class StreamSocketOperationTests
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
#if TEST_WITH_EXTERNAL_HOSTS
            // using blocking sockets

            // create and bind
            GridMate::SocketDriverCommon::SocketType theSocket;
            {
                SocketAddressInfo addressInfo;
                addressInfo.Resolve(nullptr, 0, Driver::BSDSocketFamilyType::BSD_AF_INET, false, GridMate::SocketAddressInfo::AdditionalOptionFlags::Passive);
                theSocket = SocketOperations::CreateSocket(false, Driver::BSDSocketFamilyType::BSD_AF_INET);
                AZ_TEST_ASSERT(SocketOperations::Bind(theSocket, addressInfo.GetAddressInfo()->ai_addr, addressInfo.GetAddressInfo()->ai_addrlen) == GridMate::Driver::EC_OK);
            }
            // connect and send and receive
            {
                GridMate::SocketOperations::ConnectionResult connectionResult;
                SocketAddressInfo addressInfo;
                auto flags = GridMate::SocketAddressInfo::AdditionalOptionFlags::Passive;
                AZ_TEST_ASSERT(addressInfo.Resolve("www.github.com", 80, Driver::BSDSocketFamilyType::BSD_AF_INET, false, flags));
                AZ_TEST_ASSERT(SocketOperations::Connect(theSocket, addressInfo.GetAddressInfo()->ai_addr, addressInfo.GetAddressInfo()->ai_addrlen, connectionResult) == GridMate::Driver::EC_OK);

                // happy path
                char buf[] = { "GET http://www.github.com/ HTTP/1.0\r\nUser-Agent: HTTPTool/1.0\r\n\r\n\r\n" };
                AZ::u32 bytesSent = sizeof(buf);
                AZ_TEST_ASSERT(SocketOperations::Send(theSocket, &buf[0], sizeof(buf), bytesSent) == GridMate::Driver::EC_OK);
                AZ_TEST_ASSERT(bytesSent > 0);

                char getBuffer[1024];
                AZ::u32 bytesToGet = sizeof(getBuffer);
                AZ_TEST_ASSERT(SocketOperations::Receive(theSocket, &getBuffer[0], bytesToGet) == GridMate::Driver::EC_OK);
                AZ_TEST_ASSERT(bytesToGet > 0);

                // fails expected
                AZ_TEST_ASSERT(SocketOperations::Send(theSocket, &buf[0], 0, bytesSent) != GridMate::Driver::EC_OK);
                AZ_TEST_ASSERT(SocketOperations::Send(theSocket, &buf[0], static_cast<AZ::u32>(-29), bytesSent) != GridMate::Driver::EC_OK);
                bytesToGet = 0;
                AZ_TEST_ASSERT(SocketOperations::Receive(theSocket, &getBuffer[0], bytesToGet) != GridMate::Driver::EC_OK);
                bytesToGet = static_cast<AZ::u32>(-29);
                AZ_TEST_ASSERT(SocketOperations::Receive(theSocket, &getBuffer[0], bytesToGet) != GridMate::Driver::EC_OK);
            }
#endif // TEST_WITH_EXTERNAL_HOSTS
        }
    };

    class StreamSocketDriverTestsCreateDelete
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            StreamSocketDriver driver;
            AZ_TEST_ASSERT(driver.GetMaxNumConnections() == 32);
        }
    };

    class StreamSocketDriverTestsBindSocketEmpty
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            {
                StreamSocketDriver server(32);
                auto ret = server.Initialize();
                AZ_TEST_ASSERT(ret == GridMate::Driver::EC_OK);
            }
            {
                StreamSocketDriver client(1);
                auto ret = client.Initialize();
                AZ_TEST_ASSERT(ret == GridMate::Driver::EC_OK);
            }
        }
    };

    class StreamSocketDriverTestsSimpleLockStepConnection
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            StreamSocketDriver server(32);
            auto serverInitialize = server.Initialize();
            AZ_TEST_ASSERT(serverInitialize == GridMate::Driver::EC_OK);

            StreamSocketDriver client(1);
            auto clientInitialize = client.Initialize();
            AZ_TEST_ASSERT(clientInitialize == GridMate::Driver::EC_OK);

            AZ_TEST_ASSERT(server.StartListen(255) == GridMate::Driver::EC_OK);
            auto serverAddressName = SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());
            auto driveAddress = client.CreateDriverAddress(serverAddressName);
            auto serverAddress = AZStd::static_pointer_cast<SocketDriverAddress>(driveAddress);
            AZ_TEST_ASSERT(client.ConnectTo(serverAddress) == GridMate::Driver::EC_OK);

            bool didConnect = false;
            const int kNumTimes = 100;
            for (int i = 0; i < kNumTimes; ++i)
            {
                server.Update();
                client.Update();

                if (server.GetNumberOfConnections() > 0 && client.IsConnectedTo(serverAddress))
                {
                    didConnect = true;
                    break;
                }
            }
            AZ_TEST_ASSERT(didConnect);
        }
    };

    class StreamSocketDriverTestsEstablishConnectAndSend
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            StreamSocketDriver server(2);
            auto serverInitialize = server.Initialize(GridMate::Driver::BSDSocketFamilyType::BSD_AF_INET, "0.0.0.0", 29920, false, 0, 0);
            AZ_TEST_ASSERT(serverInitialize == GridMate::Driver::EC_OK);

            StreamSocketDriver client(1);
            auto clientInitialize = client.Initialize();
            AZ_TEST_ASSERT(clientInitialize == GridMate::Driver::EC_OK);

            AZ_TEST_ASSERT(server.StartListen(2) == GridMate::Driver::EC_OK);
            auto serverAddressName = SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());
            auto driveAddress = client.CreateDriverAddress(serverAddressName);
            auto serverAddress = AZStd::static_pointer_cast<SocketDriverAddress>(driveAddress);
            AZ_TEST_ASSERT(client.ConnectTo(serverAddress) == GridMate::Driver::EC_OK);

            bool didConnect = false;
            const int kNumTimes = 1000;
            for (int i = 0; i < kNumTimes; ++i)
            {
                server.Update();
                client.Update();

                if (server.GetNumberOfConnections() > 0 && client.IsConnectedTo(serverAddress))
                {
                    didConnect = true;
                    break;
                }
            }
            AZ_TEST_ASSERT(didConnect);

            char packet[] = { "Hello Server" };
            bool didSendPacket = false;
            for (int i = 0; i < kNumTimes; ++i)
            {
                server.Update();
                client.Update();

                AZStd::intrusive_ptr<DriverAddress> from;
                char buffer[64];
                AZ::u32 bytesRead = server.Receive(buffer, sizeof(buffer), from);
                if (bytesRead > 0)
                {
                    AZ_TEST_ASSERT(bytesRead == sizeof(packet));
                    didSendPacket = memcmp(buffer, packet, sizeof(packet)) == 0;
                    break;
                }
                AZ_TEST_ASSERT(client.Send(driveAddress, packet, sizeof(packet)) == GridMate::Driver::EC_OK);
            }
            AZ_TEST_ASSERT(didSendPacket);
        }
    };

    class StreamSocketDriverTestsManyRandomPackets
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            StreamSocketDriver server(2, 1024);
            server.Initialize();
            server.StartListen(2);
            auto serverAddressName = SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());

            StreamSocketDriver client(1);
            client.Initialize();
            auto socketAddress = AZStd::static_pointer_cast<SocketDriverAddress>(client.CreateDriverAddress(serverAddressName));
            client.ConnectTo(socketAddress);

            bool didConnect = ConnectStreamSocketDriverServerClient(server, client, socketAddress, 100);
            AZ_TEST_ASSERT(didConnect);

            AZ::BetterPseudoRandom rand;
            using TestPacket = AZStd::vector<char>;
            using PacketQueue = AZStd::queue<TestPacket>;
#define MAX_PACKET_SIZE 128

            const auto fnCreatePayload = [&](char* buffer, int size)
            {
                uint32_t randKey;
                rand.GetRandom(randKey);
                size_t numChars = randKey % size;
                rand.GetRandom(buffer, numChars);
                return numChars;
            };

            const auto fnReadAndCompare = [](PacketQueue& packetList, StreamSocketDriver& driver, AZStd::intrusive_ptr<DriverAddress>& from)
            {
                GridMate::Driver::ResultCode rc;
                char buffer[MAX_PACKET_SIZE];
                AZ::u32 bytesRead = driver.Receive(buffer, sizeof(buffer), from, &rc);
                AZ_TEST_ASSERT(rc == GridMate::Driver::EC_OK);
                while (bytesRead > 0)
                {
                    auto tester = packetList.front();
                    packetList.pop();
                    AZ_TEST_ASSERT(memcmp(&buffer[0], &tester[0], bytesRead) == 0);
                    bytesRead = driver.Receive(buffer, sizeof(buffer), from, &rc);
                    AZ_TEST_ASSERT(rc == GridMate::Driver::EC_OK);
                }
            };

            PacketQueue toServerPacketList;
            PacketQueue toClientPacketList;
            AZStd::intrusive_ptr<DriverAddress> clientAddress;

            const int kNumTimes = 500;
            for (int i = 0; i < kNumTimes; ++i)
            {
                server.Update();
                client.Update();

                // do reads
                AZStd::intrusive_ptr<DriverAddress> from;
                fnReadAndCompare(toServerPacketList, server, clientAddress);
                fnReadAndCompare(toClientPacketList, client, from);

                // do write
                if (i == 0 || (i % 2) == 0)
                {
                    char buffer[MAX_PACKET_SIZE];
                    size_t numToSend = fnCreatePayload(buffer, sizeof(buffer));
                    if (numToSend > 0)
                    {
                        TestPacket testPacket(&buffer[0], &buffer[numToSend]);
                        toServerPacketList.push(testPacket);
                        AZ_TEST_ASSERT(client.Send(socketAddress, &buffer[0], (AZ::u32)numToSend) == GridMate::Driver::EC_OK);
                    }
                }
                else if (clientAddress && clientAddress->GetPort() > 0)
                {
                    char buffer[MAX_PACKET_SIZE];
                    size_t numToSend = fnCreatePayload(buffer, sizeof(buffer));
                    if (numToSend > 0)
                    {
                        TestPacket testPacket(&buffer[0], &buffer[numToSend]);
                        toClientPacketList.push(testPacket);
                        AZ_TEST_ASSERT(server.Send(clientAddress, &buffer[0], (AZ::u32)numToSend) == GridMate::Driver::EC_OK);
                    }
                }
            }
#undef MAX_PACKET_SIZE
        }
    };

    class DISABLED_StreamSocketDriverTestsTooManyConnections
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            using ClientList = AZStd::vector<StreamSocketDriver*>;

            const auto fnUpdate = [](StreamSocketDriver& server, ClientList& clients)
            {
                server.Update();
                for (auto& c : clients)
                {
                    c->Update();
                }
            };

            const AZ::u32 maxConnections = 4;
            StreamSocketDriver server(maxConnections);
            server.Initialize();
            server.StartListen(maxConnections + 1);
            auto serverAddressName = SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());

            ClientList clientList;
            const AZ::u32 tooManyConnections = 32;
            for (int i = 0; i < tooManyConnections; ++i)
            {
                auto c = aznew StreamSocketDriver(1);
                c->Initialize();
                auto serverAddress = c->CreateDriverAddress(serverAddressName);
                if (c->ConnectTo(AZStd::static_pointer_cast<SocketDriverAddress>(serverAddress)) == GridMate::Driver::EC_OK)
                {
                    clientList.emplace_back(c);
                }
                else
                {
                    delete c;
                }
                fnUpdate(server, clientList);
            }

            const AZ::u32 nUpdates = 100;
            for (AZ::u32 i = 0; i < nUpdates; ++i)
            {
                fnUpdate(server, clientList);
                AZ_TEST_ASSERT(server.GetNumberOfConnections() <= maxConnections);
            }

            for (auto& c : clientList)
            {
                delete c;
            }
        }
    };

    class StreamSocketDriverTestsClientToInvalidServer
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            const auto fnUpdateDrivers = [](StreamSocketDriver& server, StreamSocketDriver& client, const AZ::u32 nCount)
            {
                for (AZ::u32 i = 0; i < nCount; ++i)
                {
                    server.Update();
                    client.Update();
                }
            };

            StreamSocketDriver server(1);
            server.Initialize();
            server.StartListen(1);
            auto serverAddressName = SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());

            StreamSocketDriver client(1);
            client.Initialize();
            auto serverAddress = AZStd::static_pointer_cast<SocketDriverAddress>(client.CreateDriverAddress(serverAddressName));
            client.ConnectTo(serverAddress);

            bool wasConnected = false;
            const AZ::u32 kMaxTries = 10;
            for (AZ::u32 i = 0; i < kMaxTries; ++i)
            {
                fnUpdateDrivers(server, client, 20);
                if (client.IsConnectedTo(serverAddress))
                {
                    wasConnected = true;
                    break;
                }
            }
            AZ_TEST_ASSERT(wasConnected);

            AZ_TEST_ASSERT(client.DisconnectFrom(serverAddress) == GridMate::Driver::EC_OK);
            for (AZ::u32 i = 0; i < kMaxTries; ++i)
            {
                // allow for graceful disconnect
                fnUpdateDrivers(server, client, 20);
            }
            AZ_TEST_ASSERT(client.IsConnectedTo(serverAddress) == false);

            // try to connect to a bogus server address
            auto bogusAddress = AZStd::static_pointer_cast<SocketDriverAddress>(client.CreateDriverAddress("127.0.0.1|1"));
            // the attempt should succeed...
            AZ_TEST_ASSERT(client.ConnectTo(bogusAddress) == GridMate::Driver::EC_OK);

            //... but it should not go into 'connected mode'
            for (AZ::u32 i = 0; i < kMaxTries; ++i)
            {
                fnUpdateDrivers(server, client, 20);
                AZ_TEST_ASSERT(client.IsConnectedTo(bogusAddress) == false);
            }

            //... now reconnect to the server
            client.ConnectTo(serverAddress);

            wasConnected = false;
            for (AZ::u32 i = 0; i < kMaxTries; ++i)
            {
                fnUpdateDrivers(server, client, 20);
                if (client.IsConnectedTo(serverAddress))
                {
                    wasConnected = true;
                    break;
                }
            }
            AZ_TEST_ASSERT(wasConnected);
        }
    };

    class StreamSocketDriverTestsManySends
        : public GridMateMPTestFixture
    {
    public:
        void run()
        {
            StreamSocketDriver server(1);
            StreamSocketDriver client(1);
            bool isConnected = ConnectStreamSocketInitializeAndConnect(server, client, 100);
            AZ_TEST_ASSERT(isConnected);

            auto serverName = SocketDriverCommon::IPPortToAddressString("127.0.0.1", server.GetPort());
            auto serverAddr = client.CreateDriverAddress(serverName);

            AZ::BetterPseudoRandom rand;
            using TestPacket = AZStd::vector<char>;
            using PacketQueue = AZStd::queue<TestPacket>;

            const auto fnCreatePayload = [&](char* buffer, int size)
            {
                uint32_t randKey;
                rand.GetRandom(randKey);
                size_t numChars = (randKey % size) + 1;
                rand.GetRandom(buffer, numChars);
                return numChars;
            };

            const AZ::u32 kManyPackets = 1024;
            const AZ::u32 kMaxPacketSize = 128;
            PacketQueue sentPackets;
            for (int i = 0; i < kManyPackets; ++i)
            {
                char buffer[kMaxPacketSize];
                size_t numToSend = fnCreatePayload(buffer, sizeof(buffer));
                TestPacket testPacket(&buffer[0], &buffer[numToSend]);
                if (client.Send(serverAddr, buffer, static_cast<AZ::u32>(numToSend)) == GridMate::Driver::EC_OK)
                {
                    sentPackets.push(testPacket);
                }
                else
                {
                    client.Update();
                    server.Update();
                }
            }

            AZ::u32 numAttempts = 2000;
            GridMate::Driver::ResultCode resultCode;
            AZStd::intrusive_ptr<GridMate::DriverAddress> from;
            char buffer[kMaxPacketSize];
            client.Update();
            server.Update();
            AZ::u32 numBytes = server.Receive(buffer, sizeof(buffer), from, &resultCode);
            while (!sentPackets.empty())
            {
                AZ_TEST_ASSERT(numAttempts > 0);
                if (numAttempts == 0)
                {
                    break;
                }
                --numAttempts;

                if (numBytes > 0)
                {
                    auto tester = sentPackets.front();
                    sentPackets.pop();
                    auto nCompare = memcmp(&buffer[0], &tester[0], numBytes);
                    if (nCompare != 0)
                    {
                        --numAttempts;
                    }
                    AZ_TEST_ASSERT(nCompare == 0);
                }
                client.Update();
                server.Update();
                numBytes = server.Receive(buffer, sizeof(buffer), from, &resultCode);
            }
        }
    };
}

#if !AZ_TRAIT_GRIDMATE_UNIT_TEST_DISABLE_STREAM_SOCKET_DRIVER_TESTS

GM_TEST_SUITE(StreamSocketDriverTests)

    GM_TEST(StreamSocketOperationTests);
    GM_TEST(StreamSocketDriverTestsCreateDelete);
    GM_TEST(StreamSocketDriverTestsBindSocketEmpty);
    GM_TEST(StreamSocketDriverTestsSimpleLockStepConnection);
    GM_TEST(StreamSocketDriverTestsEstablishConnectAndSend);
    GM_TEST(StreamSocketDriverTestsManyRandomPackets);
    GM_TEST(DISABLED_StreamSocketDriverTestsTooManyConnections);
    GM_TEST(StreamSocketDriverTestsClientToInvalidServer);
    GM_TEST(StreamSocketDriverTestsManySends);

GM_TEST_SUITE_END()

#endif // !AZ_TRAIT_GRIDMATE_UNIT_TEST_DISABLE_STREAM_SOCKET_DRIVER_TESTS
