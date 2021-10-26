/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Tests.h"
#include <AzCore/std/parallel/thread.h>

#include <GridMate/Carrier/DefaultSimulator.h>
#include <GridMate/Carrier/SocketDriver.h>
#include <GridMate/Carrier/StreamSocketDriver.h>
#include <GridMate/Carrier/DefaultHandshake.h>

using namespace GridMate;

static const AZ::u32 kMaxPacketSize = 1500;
static const AZ::u32 kInboundBufferSize = 1024 * 64;
static const AZ::u32 kOutboundBufferSize = 1024 * 64;

class SocketDriverSupplier
{
public:
    virtual SocketDriver* CreateDriverForJoin(CarrierDesc& desc, AZ::u32 recvBuffSize = kMaxPacketSize, AZ::u32 sendBuffSize = kMaxPacketSize, AZ::u32 maxPacketSize = kMaxPacketSize)
    {
        desc.m_threadInstantResponse = false;       //Instant response not supported by StreamSocketDriver
        if (desc.m_driverReceiveBufferSize == 0)
        {
            desc.m_driverReceiveBufferSize = recvBuffSize;
        }
        if (desc.m_driverSendBufferSize == 0)
        {
            desc.m_driverSendBufferSize = sendBuffSize;
        }
        return aznew StreamSocketDriver(1, maxPacketSize, kInboundBufferSize, kOutboundBufferSize);
    }
    virtual SocketDriver* CreateDriverForHost(CarrierDesc& desc, AZ::u32 recvBuffSize = kMaxPacketSize, AZ::u32 sendBuffSize = kMaxPacketSize, AZ::u32 maxPacketSize = kMaxPacketSize)
    {
        desc.m_threadInstantResponse = false;       //Instant response not supported by StreamSocketDriver
        if (desc.m_driverReceiveBufferSize == 0)
        {
            desc.m_driverReceiveBufferSize = recvBuffSize;
        }
        if (desc.m_driverSendBufferSize == 0)
        {
            desc.m_driverSendBufferSize = sendBuffSize;
        }
        return aznew StreamSocketDriver(8, maxPacketSize, kInboundBufferSize, kOutboundBufferSize);
    }
};

class CarrierStreamCallbacksHandler
    : public CarrierEventBus::Handler
    , public StreamSocketDriverEventsBus::Handler
{
public:
    CarrierStreamCallbacksHandler()
        : m_carrier(nullptr)
        , m_connectionID(InvalidConnectionID)
        , m_disconnectID(InvalidConnectionID)
        , m_incommingConnectionID(InvalidConnectionID)
        , m_errorCode(-1)
        , m_active(false)
    {
    }

    ~CarrierStreamCallbacksHandler() override
    {
        if (m_active)
        {
            CarrierEventBus::Handler::BusDisconnect();
            StreamSocketDriverEventsBus::Handler::BusDisconnect();
        }
    }

    void Activate(Carrier* carrier, Driver* driver)
    {
        m_active = true;
        m_carrier = carrier;
        m_driver = driver;
        CarrierEventBus::Handler::BusConnect(carrier->GetGridMate());
        StreamSocketDriverEventsBus::Handler::BusConnect(carrier->GetGridMate());
    }

    //
    // StreamSocketDriverEventsBus
    //
    void OnConnectionEstablished(const SocketDriverAddress& address) override
    {
        if (m_driver == address.GetDriver())
        {
            AZ_TracePrintf("GridMate", "OnConnectionEstablished to %s\n", address.ToString().c_str());
        }
    }
    void OnConnectionDisconnected(const SocketDriverAddress& address) override
    {
        if (m_driver == address.GetDriver())
        {
            AZ_TracePrintf("GridMate", "OnConnectionEstablished to %s\n", address.ToString().c_str());
        }
    }

    //
    // CarrierEventBus
    //
    void OnIncomingConnection(Carrier* carrier, ConnectionID id) override
    {
        if (carrier != m_carrier)
        {
            return;     // not for us
        }
        m_incommingConnectionID = id;
    }

    void OnFailedToConnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) override
    {
        (void)id;
        (void)reason;
        if (carrier != m_carrier)
        {
            return;     // not for us
        }
        AZ_TEST_ASSERT(false);
    }

    void OnConnectionEstablished(Carrier* carrier, ConnectionID id) override
    {
        if (carrier != m_carrier)
        {
            return;     // not for us
        }
        m_connectionID = id;
    }

    void OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) override
    {
        (void)reason;
        if (carrier != m_carrier)
        {
            return;     // not for us
        }
        m_disconnectID = id;
    }

    void OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error) override
    {
        (void)id;
        if (carrier != m_carrier)
        {
            return;     // not for us
        }
        m_errorCode = static_cast<int>(error.m_errorCode);
    }

    void OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error) override
    {
        (void)carrier;
        (void)id;
        (void)error;
        //Ignore security warnings in unit tests
    }

    Driver*         m_driver;
    Carrier*        m_carrier;
    ConnectionID    m_connectionID;
    ConnectionID    m_disconnectID;
    ConnectionID    m_incommingConnectionID;
    int             m_errorCode;
    bool            m_active;
};

namespace UnitTest
{
    class DISABLED_CarrierStreamBasicTest
        : public GridMateMPTestFixture
        , protected SocketDriverSupplier
    {
    public:
        void run()
        {
#if AZ_TRAIT_GRIDMATE_TEST_SOCKET_IPV6_SUPPORT_ENABLED
            bool useIpv6 = true;
#else
            bool useIpv6 = false;
#endif

            CarrierStreamCallbacksHandler clientCB, serverCB;
            TestCarrierDesc serverCarrierDesc, clientCarrierDesc;

            AZStd::string str("Hello this is a carrier test!");

            const char* targetAddress = "127.0.0.1";

            if (useIpv6)
            {
                clientCarrierDesc.m_familyType = Driver::BSD_AF_INET6;
                serverCarrierDesc.m_familyType = Driver::BSD_AF_INET6;
                targetAddress = "::1";
            }

            clientCarrierDesc.m_enableDisconnectDetection = false;
            serverCarrierDesc.m_enableDisconnectDetection = false;

            clientCarrierDesc.m_driver = CreateDriverForJoin(clientCarrierDesc);
            serverCarrierDesc.m_driver = CreateDriverForHost(serverCarrierDesc);

            clientCarrierDesc.m_port = 4427;
            serverCarrierDesc.m_port = 4433;

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            clientCB.Activate(clientCarrier, clientCarrierDesc.m_driver);

            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);
            serverCB.Activate(serverCarrier, serverCarrierDesc.m_driver);

            // stream socket driver has explicit connection calls
            auto serverName = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->IPPortToAddress(targetAddress, serverCarrierDesc.m_port);
            auto serverAddr = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->CreateDriverAddress(serverName);
            static_cast<StreamSocketDriver*>(serverCarrierDesc.m_driver)->StartListen(100);
            static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->ConnectTo(AZStd::static_pointer_cast<SocketDriverAddress>(serverAddr));

            //////////////////////////////////////////////////////////////////////////
            // Test carriers [0 is server, 1 is client]
            bool isClientDone = false;
            bool isServerDone = false;
            bool isDisconnect = false;
            char clientBuffer[kMaxPacketSize];
            char serverBuffer[kMaxPacketSize];

            Carrier::ReceiveResult receiveResult;
            ConnectionID connId = InvalidConnectionID;
            int maxNumUpdates = 2000;
            int numUpdates = 0;
            while (numUpdates <= maxNumUpdates)
            {
                // Client
                if (!isClientDone)
                {
                    if (connId == InvalidConnectionID)
                    {
                        connId = clientCarrier->Connect(targetAddress, serverCarrierDesc.m_port);
                        AZ_TEST_ASSERT(connId != InvalidConnectionID);
                    }
                    else
                    {
                        if (connId != AllConnections && clientCB.m_connectionID == connId)
                        {
                            clientCarrier->Send(str.c_str(), static_cast<unsigned>(str.length() + 1), clientCB.m_connectionID);
                            connId = AllConnections;
                        }

                        if (clientCB.m_connectionID != InvalidConnectionID)
                        {
                            receiveResult = clientCarrier->Receive(clientBuffer, AZ_ARRAY_SIZE(clientBuffer), clientCB.m_connectionID);
                            if (receiveResult.m_state == Carrier::ReceiveResult::RECEIVED)
                            {
                                AZ_TEST_ASSERT(memcmp(str.c_str(), clientBuffer, str.length()) == 0)
                                isClientDone = true;
                            }
                        }
                    }
                }

                // Server
                if (!isServerDone)
                {
                    if (serverCB.m_connectionID != InvalidConnectionID)
                    {
                        AZ_TEST_ASSERT(serverCB.m_incommingConnectionID == serverCB.m_connectionID);
                        receiveResult = serverCarrier->Receive(serverBuffer, AZ_ARRAY_SIZE(serverBuffer), serverCB.m_connectionID);
                        if (receiveResult.m_state == Carrier::ReceiveResult::RECEIVED)
                        {
                            serverCarrier->Send(str.c_str(), static_cast<unsigned>(str.length() + 1), connId);
                            AZ_TEST_ASSERT(memcmp(str.c_str(), serverBuffer, str.length()) == 0);
                            isServerDone = true;
                        }
                    }
                }

                serverCarrier->Update();
                clientCarrier->Update();

                if ((clientCB.m_disconnectID != InvalidConnectionID && serverCB.m_disconnectID != InvalidConnectionID) ||
                    clientCB.m_errorCode != -1 || serverCB.m_errorCode != -1)
                {
                    break;
                }

                if (!isDisconnect && isClientDone && isServerDone && numUpdates > 50 /* we need 1 sec to update statistics */)
                {
                    // check statistics
                    Carrier::Statistics clientStats, clientStatsLifeTime, clientStatsLastSecond;
                    Carrier::Statistics serverStats, serverStatsLifeTime, serverStatsLastSecond;
                    Carrier::ConnectionStates clientState = clientCarrier->QueryStatistics(clientCB.m_connectionID, &clientStatsLastSecond, &clientStatsLifeTime);
                    Carrier::ConnectionStates serverState = serverCarrier->QueryStatistics(serverCB.m_connectionID, &serverStatsLastSecond, &serverStatsLifeTime);

                    clientStats = clientStatsLifeTime;
                    clientStats.m_rtt = (clientStats.m_rtt + clientStatsLastSecond.m_rtt) * .5f;
                    clientStats.m_packetSend += clientStatsLastSecond.m_packetSend;
                    clientStats.m_dataSend += clientStatsLastSecond.m_dataSend;

                    serverStats = serverStatsLifeTime;
                    serverStats.m_rtt = (serverStats.m_rtt + serverStatsLastSecond.m_rtt) * .5f;
                    serverStats.m_packetSend += serverStatsLastSecond.m_packetSend;
                    serverStats.m_dataSend += serverStatsLastSecond.m_dataSend;

                    AZ_TEST_ASSERT(clientState == Carrier::CST_CONNECTED);
                    AZ_TEST_ASSERT(serverState == Carrier::CST_CONNECTED);
                    AZ_TEST_ASSERT(clientStats.m_rtt > 0);
                    AZ_TEST_ASSERT(serverStats.m_rtt > 0);
                    AZ_TEST_ASSERT(clientStats.m_packetSend > 0);
                    AZ_TEST_ASSERT(serverStats.m_packetSend > 0);
                    AZ_TEST_ASSERT(clientStats.m_dataSend > str.length() + 1);
                    AZ_TEST_ASSERT(serverStats.m_dataSend > str.length() + 1);

                    // Disconnect the server and test that the disconnect message will reach the client too.
                    serverCarrier->Disconnect(serverCB.m_connectionID);

                    isDisconnect = true;
                }

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                numUpdates++;
            }
            DefaultCarrier::Destroy(clientCarrier);
            DefaultCarrier::Destroy(serverCarrier);
            //////////////////////////////////////////////////////////////////////////
            AZ_TEST_ASSERT(isServerDone && isClientDone);
        }
    };

    class DISABLED_CarrierStreamAsyncHandshakeTest
        : public GridMateMPTestFixture
        , protected SocketDriverSupplier
    {
    public:

        static const unsigned int kHandshakeTimeoutMsec = 5000;
        static const unsigned int kVersion = 1;

        struct AsyncHandshake : public DefaultHandshake
        {
            AsyncHandshake()
                : DefaultHandshake(kHandshakeTimeoutMsec, kVersion)
                , m_isDone(false)
                , m_numPendingRequests(0)
            {

            }

            HandshakeErrorCode OnReceiveRequest(ConnectionID id, ReadBuffer& rb, WriteBuffer& wb) override
            {
                if (!m_isDone)
                {
                    ++m_numPendingRequests;
                    return HandshakeErrorCode::PENDING;
                }

                return DefaultHandshake::OnReceiveRequest(id, rb, wb);
            }

            void Done()
            {
                m_isDone = true;
            }

            bool m_isDone;
            unsigned int m_numPendingRequests;
        };

        void run()
        {
            CarrierStreamCallbacksHandler clientCB, serverCB;
            TestCarrierDesc serverCarrierDesc, clientCarrierDesc;

            AZStd::string str("Hello this is a carrier test!");
            clientCarrierDesc.m_driver = CreateDriverForJoin(clientCarrierDesc);
            serverCarrierDesc.m_driver = CreateDriverForHost(serverCarrierDesc);

            clientCarrierDesc.m_port = 4427;
            serverCarrierDesc.m_port = 4429;

            AsyncHandshake serverHandshake;
            serverCarrierDesc.m_handshake = &serverHandshake;

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            clientCB.Activate(clientCarrier, clientCarrierDesc.m_driver);

            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);
            serverCB.Activate(serverCarrier, serverCarrierDesc.m_driver);

            // stream socket driver has explicit connection calls
            const char* targetAddress = "127.0.0.1";
            static_cast<StreamSocketDriver*>(serverCarrierDesc.m_driver)->StartListen(100);
            auto serverName = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->IPPortToAddress(targetAddress, serverCarrierDesc.m_port);
            auto serverAddr = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->CreateDriverAddress(serverName);
            static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->ConnectTo(AZStd::static_pointer_cast<SocketDriverAddress>(serverAddr));

            char buffer[kMaxPacketSize];

            ConnectionID connId = InvalidConnectionID;
            int maxNumUpdates = 2000;
            int numUpdates = 0;

            bool clientReceived = false;
            bool serverReceived = false;

            while (numUpdates++ < maxNumUpdates)
            {
                if (numUpdates == 1)
                {
                    connId = clientCarrier->Connect(targetAddress, serverCarrierDesc.m_port);
                    AZ_TEST_ASSERT(connId != InvalidConnectionID);
                }
                else if (numUpdates == 200)
                {
                    serverHandshake.Done();
                }
                else if (numUpdates == 400) // should be connected by now
                {
                    AZ_TEST_ASSERT(serverCB.m_connectionID != InvalidConnectionID);
                    AZ_TEST_ASSERT(clientCB.m_connectionID == connId);
                    AZ_TEST_ASSERT(serverHandshake.m_numPendingRequests > 2); // checking we received multiple requests before accepted it

                    serverHandshake.m_numPendingRequests = 0;

                    serverCarrier->Send(str.c_str(), static_cast<unsigned int>(str.size()), serverCB.m_connectionID);
                    clientCarrier->Send(str.c_str(), static_cast<unsigned int>(str.size()), clientCB.m_connectionID);
                }
                else if (numUpdates > 400)
                {
                    Carrier::ReceiveResult result = clientCarrier->Receive(buffer, sizeof(buffer), clientCB.m_connectionID);
                    if (result.m_state == Carrier::ReceiveResult::RECEIVED && result.m_numBytes == str.size())
                    {
                        clientReceived = strncmp(str.c_str(), buffer, result.m_numBytes) == 0;
                    }

                    result = serverCarrier->Receive(buffer, sizeof(buffer), serverCB.m_connectionID);
                    if (result.m_state == Carrier::ReceiveResult::RECEIVED && result.m_numBytes == str.size())
                    {
                        serverReceived = strncmp(str.c_str(), buffer, result.m_numBytes) == 0;
                    }

                    if (clientReceived && serverReceived) // end test
                    {
                        break;
                    }
                }

                serverCarrier->Update();
                clientCarrier->Update();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
            }

            AZ_TEST_ASSERT(serverHandshake.m_numPendingRequests == 0); // no new requests after connection accomplished
            AZ_TEST_ASSERT(clientReceived);
            AZ_TEST_ASSERT(serverReceived);

            DefaultCarrier::Destroy(clientCarrier);
            DefaultCarrier::Destroy(serverCarrier);
        }
    };

    class CarrierStreamStressTest
        : public GridMateMPTestFixture
        , protected SocketDriverSupplier
        , public ::testing::Test
    {
    public:
    };

    TEST_F(CarrierStreamStressTest, DISABLED_Stress_Test)
    {
        CarrierStreamCallbacksHandler clientCB, serverCB;
        UnitTest::TestCarrierDesc serverCarrierDesc, clientCarrierDesc;

        AZStd::string str("Hello this is a carrier stress test!");

        clientCarrierDesc.m_enableDisconnectDetection = /*false*/ true;
        serverCarrierDesc.m_enableDisconnectDetection = /*false*/ true;
        clientCarrierDesc.m_threadUpdateTimeMS = 5;
        serverCarrierDesc.m_threadUpdateTimeMS = 5;
        clientCarrierDesc.m_port = 4427;
        serverCarrierDesc.m_port = 4430;

        clientCarrierDesc.m_driver = CreateDriverForJoin(clientCarrierDesc);
        serverCarrierDesc.m_driver = CreateDriverForHost(serverCarrierDesc);

        Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
        clientCB.Activate(clientCarrier, clientCarrierDesc.m_driver);

        Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);
        serverCB.Activate(serverCarrier, serverCarrierDesc.m_driver);

        // stream socket driver has explicit connection calls
        const char* targetAddress = "127.0.0.1";
        static_cast<StreamSocketDriver*>(serverCarrierDesc.m_driver)->StartListen(100);
        auto serverName = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->IPPortToAddress(targetAddress, serverCarrierDesc.m_port);
        auto serverAddr = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->CreateDriverAddress(serverName);
        static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->ConnectTo(AZStd::static_pointer_cast<SocketDriverAddress>(serverAddr));

        //////////////////////////////////////////////////////////////////////////
        // Test carriers [0 is server, 1 is client]
        char serverBuffer[kMaxPacketSize];

        ConnectionID connId = InvalidConnectionID;
        int numUpdates = 0;
        int numSend = 0;
        int numRecv = 0;
        int numUpdatesLastPrint = 0;
        while (numRecv < 70000)
        {
            // Client
            if (connId == InvalidConnectionID)
            {
                connId = clientCarrier->Connect(targetAddress, serverCarrierDesc.m_port);
                AZ_TEST_ASSERT(connId != InvalidConnectionID);
            }
            else
            {
                if (connId != AllConnections && clientCB.m_connectionID == connId)
                {
                    clientCarrier->Send(str.c_str(), static_cast<unsigned>(str.length() + 1), clientCB.m_connectionID);
                    numSend++;
                }
            }

            // Server
            if (serverCB.m_connectionID != InvalidConnectionID)
            {
                AZ_TEST_ASSERT(serverCB.m_incommingConnectionID == serverCB.m_connectionID);

                Carrier::ReceiveResult result;
                while ((result = serverCarrier->Receive(serverBuffer, AZ_ARRAY_SIZE(serverBuffer), serverCB.m_connectionID)).m_state == Carrier::ReceiveResult::RECEIVED)
                {
                    AZ_TEST_ASSERT(memcmp(str.c_str(), serverBuffer, str.length()) == 0);
                    numRecv++;
                }
            }

            serverCarrier->Update();
            clientCarrier->Update();

            if ((clientCB.m_disconnectID != InvalidConnectionID && serverCB.m_disconnectID != InvalidConnectionID) ||
                clientCB.m_errorCode != -1 || serverCB.m_errorCode != -1)
            {
                break;
            }

            if (numUpdates - numUpdatesLastPrint == 5000)
            {
                numUpdatesLastPrint = numUpdates;
                AZ_Printf("GridMate", "numSend:%d numRecv:%d\n", numSend, numRecv);

                // check statistics
                Carrier::Statistics clientStats, clientStatsLifeTime, clientStatsLastSecond;
                Carrier::Statistics serverStats, serverStatsLifeTime, serverStatsLastSecond;
                clientCarrier->QueryStatistics(clientCB.m_connectionID, &clientStatsLastSecond, &clientStatsLifeTime);
                serverCarrier->QueryStatistics(serverCB.m_connectionID, &serverStatsLastSecond, &serverStatsLifeTime);

                clientStats = clientStatsLifeTime;
                clientStats.m_rtt = (clientStats.m_rtt + clientStatsLastSecond.m_rtt) * .5f;
                clientStats.m_packetSend += clientStatsLastSecond.m_packetSend;
                clientStats.m_dataSend += clientStatsLastSecond.m_dataSend;

                serverStats = serverStatsLifeTime;
                serverStats.m_rtt = (serverStats.m_rtt + serverStatsLastSecond.m_rtt) * .5f;
                serverStats.m_packetSend += serverStatsLastSecond.m_packetSend;
                serverStats.m_dataSend += serverStatsLastSecond.m_dataSend;

                AZ_Printf("GridMate", "Server rtt %.2f ms numPkgSent %d dataSend %d\n", serverStats.m_rtt, serverStats.m_packetSend, serverStats.m_dataSend);
                AZ_Printf("GridMate", "Client rtt %.2f ms numPkgSent %d dataSend %d\n", clientStats.m_rtt, clientStats.m_packetSend, clientStats.m_dataSend);
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));
            numUpdates++;
        }
        DefaultCarrier::Destroy(clientCarrier);
        DefaultCarrier::Destroy(serverCarrier);
        //////////////////////////////////////////////////////////////////////////
    }

    class DISABLED_CarrierStreamTest
        : public GridMateMPTestFixture
        , protected SocketDriverSupplier
    {
    public:
        void run()
        {
            //////////////////////////////////////////////////////////////////////////
            // Setup simulators
            DefaultSimulator clientSimulator;
            clientSimulator.Enable();
            clientSimulator.SetOutgoingLatency(150, 150); // in milliseconds
            clientSimulator.SetOutgoingPacketLoss(5, 5);  // drop 1 every X packets
            clientSimulator.SetOutgoingReorder(true);    // enable reorder

            clientSimulator.SetIncomingLatency(200, 200);
            clientSimulator.SetIncomingPacketLoss(7, 7);
            clientSimulator.SetIncomingReorder(true);
            clientSimulator.Enable();
            //////////////////////////////////////////////////////////////////////////

            CarrierStreamCallbacksHandler clientCB, serverCB;
            TestCarrierDesc serverCarrierDesc, clientCarrierDesc;

            clientCarrierDesc.m_port = 4427;
            clientCarrierDesc.m_driver = CreateDriverForJoin(clientCarrierDesc, 16 * 1024, 16 * 1024, kMaxPacketSize);
            //clientCarrierDesc.m_simulator = &clientSimulator;
            serverCarrierDesc.m_port = 4431;
            serverCarrierDesc.m_driver = CreateDriverForHost(serverCarrierDesc, 16 * 1024, 16 * 1024, kMaxPacketSize);

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            clientCB.Activate(clientCarrier, clientCarrierDesc.m_driver);
            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);
            serverCB.Activate(serverCarrier, serverCarrierDesc.m_driver);

            //////////////////////////////////////////////////////////////////////////
            // stream socket driver has explicit connection calls
            const char* targetAddress = "127.0.0.1";
            static_cast<StreamSocketDriver*>(serverCarrierDesc.m_driver)->StartListen(100);
            auto serverName = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->IPPortToAddress(targetAddress, serverCarrierDesc.m_port);
            auto serverAddr = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->CreateDriverAddress(serverName);
            static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->ConnectTo(AZStd::static_pointer_cast<SocketDriverAddress>(serverAddr));
            //////////////////////////////////////////////////////////////////////////

            unsigned int intArray[10240];
            for (unsigned int i = 0; i < AZ_ARRAY_SIZE(intArray); ++i)
            {
                intArray[i] = i;
            }

            //////////////////////////////////////////////////////////////////////////
            // Test carriers [0 is server, 1 is client]
            Carrier::ReceiveResult receiveResult;
            bool isClientDone = false;
            bool isServerDone = false;
            bool isDisconnect = false;
            ConnectionID connId = InvalidConnectionID;
            char clientBuffer[65 * 1024];
            char serverBuffer[65 * 1024];
            const int maxNumUpdates = 5000;
            int numUpdates = 0;
            bool isPrintStatus = false;
            while (numUpdates <= maxNumUpdates)
            {
                // Client
                if (!isClientDone)
                {
                    if (connId == InvalidConnectionID)
                    {
                        connId = clientCarrier->Connect(targetAddress, serverCarrierDesc.m_port);
                        AZ_TEST_ASSERT(connId != InvalidConnectionID);
                    }
                    else
                    {
                        if (connId != AllConnections && clientCB.m_connectionID == connId)
                        {
                            clientCarrier->Send((char*)intArray, sizeof(intArray), clientCB.m_connectionID);
                            connId = AllConnections;
                        }

                        if (clientCB.m_connectionID != InvalidConnectionID)
                        {
                            /////////////////////////////////////////////////////////////////////
                            // Test Receive functions buffer overflow and buffer size
                            unsigned int queryBufferSize = clientCarrier->QueryNextReceiveMessageMaxSize(clientCB.m_connectionID);
                            if (queryBufferSize > 0) // if there is message waiting on to be received
                            {
                                // we can receive messages only of intArray size
                                AZ_TEST_ASSERT(queryBufferSize >= sizeof(intArray));
                            }

                            receiveResult = clientCarrier->Receive(clientBuffer, 100, clientCB.m_connectionID); // receive with smaller buffer
                            switch (receiveResult.m_state)
                            {
                                case Carrier::ReceiveResult::NO_MESSAGE_TO_RECEIVE:
                                {
                                    // make sure the query return 0 if we have no message to receive
                                    AZ_TEST_ASSERT(queryBufferSize == 0);
                                    break;
                                }
                                case Carrier::ReceiveResult::UNSUFFICIENT_BUFFER_SIZE:
                                {
                                    // we should have a message waiting if we fail to receive it
                                    AZ_TEST_ASSERT(queryBufferSize > 0);
                                    break;
                                }
                                case Carrier::ReceiveResult::RECEIVED:
                                {
                                    // we have small buffer we should never be able to receive a message
                                    AZ_TEST_ASSERT(false);
                                    break;
                                }
                                default:
                                    break;
                            }
                            /////////////////////////////////////////////////////////////////////

                            receiveResult = clientCarrier->Receive(clientBuffer, AZ_ARRAY_SIZE(clientBuffer), clientCB.m_connectionID);
                            if (receiveResult.m_state == Carrier::ReceiveResult::RECEIVED)
                            {
                                AZ_TEST_ASSERT(queryBufferSize >= receiveResult.m_numBytes); // make sure the query was correct
                                AZ_TEST_ASSERT(memcmp(intArray, clientBuffer, sizeof(intArray)) == 0);
                                isClientDone = true;
                            }
                        }
                    }
                }

                // Server
                if (!isServerDone)
                {
                    if (serverCB.m_connectionID != InvalidConnectionID)
                    {
                        receiveResult = serverCarrier->Receive(serverBuffer, AZ_ARRAY_SIZE(serverBuffer), serverCB.m_connectionID);
                        if (receiveResult.m_state == Carrier::ReceiveResult::RECEIVED)
                        {
                            AZ_TEST_ASSERT(memcmp(intArray, serverBuffer, sizeof(intArray)) == 0);
                            serverCarrier->Send((char*)intArray, sizeof(intArray), connId);
                            isServerDone = true;
                        }
                    }
                }

                serverCarrier->Update();
                clientCarrier->Update();

                if (!isPrintStatus && connId == AllConnections && clientCB.m_connectionID != InvalidConnectionID)
                {
                    clientCarrier->DebugStatusReport(clientCB.m_connectionID);
                    serverCarrier->DebugStatusReport(serverCB.m_connectionID);
                    isPrintStatus = true;
                }

                if (!isDisconnect && isClientDone && isServerDone && numUpdates > 50)
                {
                    // check statistics
                    Carrier::Statistics clientStats, clientStatsLifeTime, clientStatsLastSecond;
                    Carrier::Statistics serverStats, serverStatsLifeTime, serverStatsLastSecond;
                    Carrier::ConnectionStates clientState = clientCarrier->QueryStatistics(clientCB.m_connectionID, &clientStatsLastSecond, &clientStatsLifeTime);
                    Carrier::ConnectionStates serverState = serverCarrier->QueryStatistics(serverCB.m_connectionID, &serverStatsLastSecond, &serverStatsLifeTime);

                    clientStats = clientStatsLifeTime;
                    clientStats.m_rtt = (clientStats.m_rtt + clientStatsLastSecond.m_rtt) * .5f;
                    clientStats.m_packetSend += clientStatsLastSecond.m_packetSend;
                    clientStats.m_dataSend += clientStatsLastSecond.m_dataSend;

                    serverStats = serverStatsLifeTime;
                    serverStats.m_rtt = (serverStats.m_rtt + serverStatsLastSecond.m_rtt) * .5f;
                    serverStats.m_packetSend += serverStatsLastSecond.m_packetSend;
                    serverStats.m_dataSend += serverStatsLastSecond.m_dataSend;

                    AZ_TEST_ASSERT(clientState == Carrier::CST_CONNECTED && serverState == Carrier::CST_CONNECTED);
                    AZ_TEST_ASSERT(clientStats.m_rtt > 0);
                    AZ_TEST_ASSERT(serverStats.m_rtt > 0);
                    AZ_TEST_ASSERT(clientStats.m_packetSend > 0);
                    AZ_TEST_ASSERT(serverStats.m_packetSend > 0);
                    AZ_TEST_ASSERT(clientStats.m_dataSend > sizeof(intArray));
                    AZ_TEST_ASSERT(serverStats.m_dataSend > sizeof(intArray));

                    // Disconnect the server and test that the disconnect message will reach the client too.
                    serverCarrier->Disconnect(serverCB.m_connectionID);

                    isDisconnect = true;
                }

                if ((clientCB.m_disconnectID != InvalidConnectionID && serverCB.m_disconnectID != InvalidConnectionID) ||
                    clientCB.m_errorCode != -1 || serverCB.m_errorCode != -1)
                {
                    break;
                }

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                numUpdates++;
            }

            DefaultCarrier::Destroy(clientCarrier);
            DefaultCarrier::Destroy(serverCarrier);
            AZ_TEST_ASSERT(isServerDone && isClientDone);
            //////////////////////////////////////////////////////////////////////////
        }
    };

    class DISABLED_CarrierStreamDisconnectDetectionTest
        : public GridMateMPTestFixture
        , protected SocketDriverSupplier
    {
    public:
        void run()
        {
            // Setup simulators
            DefaultSimulator clientSimulator;
            clientSimulator.SetOutgoingPacketLoss(2, 2); // drop 50% packets

            TestCarrierDesc serverCarrierDesc;
            serverCarrierDesc.m_port = 4432;
            serverCarrierDesc.m_enableDisconnectDetection = true;
            serverCarrierDesc.m_disconnectDetectionPacketLossThreshold = 0.4f; // disconnect once hit 40% loss
            serverCarrierDesc.m_disconnectDetectionRttThreshold = 50; // disconnect once hit 50 msec rtt
            serverCarrierDesc.m_driver = CreateDriverForHost(serverCarrierDesc);

            TestCarrierDesc clientCarrierDesc = serverCarrierDesc;
            clientCarrierDesc.m_port = 4427;
            clientCarrierDesc.m_simulator = &clientSimulator;
            clientCarrierDesc.m_driver = CreateDriverForJoin(clientCarrierDesc);

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);

            //////////////////////////////////////////////////////////////////////////
            // stream socket driver has explicit connection calls
            const char* targetAddress = "127.0.0.1";
            static_cast<StreamSocketDriver*>(serverCarrierDesc.m_driver)->StartListen(100);
            auto serverName = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->IPPortToAddress(targetAddress, serverCarrierDesc.m_port);
            auto serverAddr = static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->CreateDriverAddress(serverName);
            static_cast<StreamSocketDriver*>(clientCarrierDesc.m_driver)->ConnectTo(AZStd::static_pointer_cast<SocketDriverAddress>(serverAddr));
            //////////////////////////////////////////////////////////////////////////

            for (int testCaseNum = 0; testCaseNum < 2; ++testCaseNum)
            {
                if (testCaseNum == 0)
                {
                    AZ_TracePrintf("GridMate", "Simulating bad packet loss...\n");
                    clientSimulator.SetIncomingPacketLoss(2, 2); // drop ~50% packets
                }
                else if (testCaseNum == 1)
                {
                    AZ_TracePrintf("GridMate", "Simulating bad latency...\n");
                    // ~60 msec rtt
                    clientSimulator.SetIncomingLatency(30, 30);
                    clientSimulator.SetOutgoingLatency(30, 30);
                    clientSimulator.SetIncomingPacketLoss(0, 0);
                }

                clientCarrier->Connect(targetAddress, serverCarrierDesc.m_port); // connecting client -> server
                int numUpdates = 0;
                while (serverCarrier->GetNumConnections() == 0 && numUpdates++ <= 1000) // wait until connected
                {
                    clientCarrier->Update();
                    serverCarrier->Update();

                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                }

                AZ_TEST_ASSERT(serverCarrier->GetNumConnections() == 1); // check if connected
                clientSimulator.Enable(); // enabling bad traffic conditions

                numUpdates = 0;
                while ((serverCarrier->GetNumConnections() == 1 || clientCarrier->GetNumConnections() == 1) // wait until both disconnect
                       && numUpdates++ <= 2000) // max 20 sec
                {
                    if (!(numUpdates % 100) && serverCarrier->GetNumConnections() == 1)
                    {
                        TrafficControl::Statistics stats;
                        serverCarrier->QueryStatistics(serverCarrier->DebugGetConnectionId(0), nullptr, &stats);
                        AZ_TracePrintf("GridMate", "  Server -> Client: rtt=%.0f msec, packetLoss=%.0f%%\n", stats.m_rtt, stats.m_packetLoss  * 100.f);
                    }

                    clientCarrier->Update();
                    serverCarrier->Update();

                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                }

                AZ_TEST_ASSERT(serverCarrier->GetNumConnections() == 0); // disconnected
                clientSimulator.Disable();
            }

            DefaultCarrier::Destroy(clientCarrier);
            DefaultCarrier::Destroy(serverCarrier);
        }
    };

    class DISABLED_CarrierStreamMultiChannelTest
        : public GridMateMPTestFixture
        , protected SocketDriverSupplier
    {
    public:
        /*
        * Sends reliable messages across different channels to each other
        */
        void run()
        {
            // initialize transport
            int basePort = 4427;
            enum
            {
                c1,
                c2,
                nCarriers
            };

            int         nMsgSent[nCarriers];
            int         nMsgReceived[nCarriers];
            Carrier*    carriers[nCarriers];
            Driver*     drivers[nCarriers];
            CarrierStreamCallbacksHandler carrierHandlers[nCarriers];

            //////////////////////////////////////////////////////////////////////////
            const char* targetAddress = "127.0.0.1";
            unsigned int serverCarrierDescPort = 0;

            for (int i = 0; i < nCarriers; ++i)
            {
                // Create carriers
                TestCarrierDesc desc;
                desc.m_enableDisconnectDetection = true;
                desc.m_port = basePort + i;
                if (i == c1)
                {
                    drivers[i] = CreateDriverForHost(desc);
                    serverCarrierDescPort = desc.m_port;
                }
                else
                {
                    drivers[i] = CreateDriverForJoin(desc);
                }
                desc.m_driver = drivers[i];
                carriers[i] = DefaultCarrier::Create(desc, m_gridMate);
                carrierHandlers[i].Activate(carriers[i], desc.m_driver);
                nMsgSent[i] = nMsgReceived[i] = 0;
            }

            // stream socket driver has explicit connection calls
            for (int k = 0; k < nCarriers; ++k)
            {
                if (k == c1)
                {
                    static_cast<StreamSocketDriver*>(drivers[k])->StartListen(100);
                }
                else
                {
                    auto serverName = static_cast<StreamSocketDriver*>(drivers[k])->IPPortToAddress(targetAddress, serverCarrierDescPort);
                    auto serverAddr = static_cast<StreamSocketDriver*>(drivers[k])->CreateDriverAddress(serverName);
                    static_cast<StreamSocketDriver*>(drivers[k])->ConnectTo(AZStd::static_pointer_cast<SocketDriverAddress>(serverAddr));
                }
            }
            //////////////////////////////////////////////////////////////////////////

            carriers[c2]->Connect(targetAddress, basePort + c1);

            int maxNumUpdates = 100;
            int numUpdates = 0;
            while (numUpdates <= maxNumUpdates)
            {
                //////////////////////////////////////////////////////////////////////////
                Update();

                for (int iCarrier = 0; iCarrier < nCarriers; ++iCarrier)
                {
                    if (carrierHandlers[iCarrier].m_connectionID != InvalidConnectionID)
                    {
                        for (unsigned int iConn = 0; iConn < carriers[iCarrier]->GetNumConnections(); ++iConn)
                        {
                            ConnectionID connId = carriers[iCarrier]->DebugGetConnectionId(iConn);
                            for (unsigned char iChannel = 0; iChannel < 3; ++iChannel)
                            {
                                char buf[kMaxPacketSize];

                                // receive
                                Carrier::ReceiveResult receiveResult = carriers[iCarrier]->Receive(buf, AZ_ARRAY_SIZE(buf), connId, iChannel);
                                if (receiveResult.m_state == Carrier::ReceiveResult::RECEIVED)
                                {
                                    nMsgReceived[iCarrier]++;
                                }

                                // send something
                                if (numUpdates < 50)
                                {
                                    azsnprintf(buf, 1024, "Msg %d", nMsgSent[iCarrier]++);
                                    carriers[iCarrier]->Send(buf, (unsigned int)strlen(buf) + 1, connId, Carrier::SEND_RELIABLE, Carrier::PRIORITY_NORMAL, iChannel);
                                }
                            }
                        }
                    }
                    carriers[iCarrier]->Update();
                }

                //////////////////////////////////////////////////////////////////////////

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                numUpdates++;
            }

            int nSent = 0;
            int nReceived = 0;
            for (int i = 0; i < nCarriers; ++i)
            {
                nSent += nMsgSent[i];
                nReceived += nMsgReceived[i];
                DefaultCarrier::Destroy(carriers[i]);
            }
            AZ_TEST_ASSERT(nSent > 0);
            AZ_TEST_ASSERT(nSent == nReceived);
        }
    };
}

GM_TEST_SUITE(CarrierStreamSuite)
    GM_TEST(DISABLED_CarrierStreamBasicTest)
    GM_TEST(DISABLED_CarrierStreamTest)
    GM_TEST(DISABLED_CarrierStreamAsyncHandshakeTest)
    GM_TEST(DISABLED_CarrierStreamMultiChannelTest)
GM_TEST_SUITE_END()
