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
#include <GridMate/Carrier/SecureSocketDriver.h>
#include <GridMate/Carrier/DefaultHandshake.h>
#include <AzCore/std/string/memorytoascii.h>

using namespace GridMate;

namespace Certificates
{
    extern const char* g_untrustedCertPEM;
    extern const char* g_untrustedPrivateKeyPEM;
}

/*
* SocketDriverProvider
* Applies default drivers to tests
*/
class SocketDriverProvider
{
public:
    virtual SocketDriver* CreateDriverForJoin() { return nullptr; }
    virtual SocketDriver* CreateDriverForHost() { return nullptr; }
};

#if AZ_TRAIT_GRIDMATE_TEST_WITH_SECURE_SOCKET_DRIVER

/*
* SecureDriverProvider
* Applies SecureSocketDriver to tests
*/
template<class ClientDriver = SecureSocketDriver, class HostDriver = SecureSocketDriver>
class SecureDriverProvider : public SocketDriverProvider
{
public:
    virtual ~SecureDriverProvider()
    {
        // Cleaning up
        while (!m_drivers.empty())
        {
            SecureSocketDriver* s = m_drivers.back();
            m_drivers.pop_back();
            delete s;
        }
    }

    SocketDriver* CreateDriverForJoin() override
    {
        SecureSocketDesc secDescJoin;
        secDescJoin.m_certificateAuthorityPEM = Certificates::g_untrustedCertPEM;
        m_drivers.push_back(aznew ClientDriver(secDescJoin));
        return m_drivers.back();
    }

    SocketDriver* CreateDriverForHost() override
    {
        SecureSocketDesc secDescHost;
        secDescHost.m_certificatePEM = Certificates::g_untrustedCertPEM;
        secDescHost.m_privateKeyPEM = Certificates::g_untrustedPrivateKeyPEM;
        m_drivers.push_back(aznew HostDriver(secDescHost));
        return m_drivers.back();
    }

protected:
    AZStd::vector<SecureSocketDriver*> m_drivers;
};

#endif

class CarrierCallbacksHandler
    : public CarrierEventBus::Handler
{
public:
    CarrierCallbacksHandler()
        : m_carrier(nullptr)
        , m_connectionID(InvalidConnectionID)
        , m_disconnectID(InvalidConnectionID)
        , m_incommingConnectionID(InvalidConnectionID)
        , m_errorCode(-1)
    {
    }

    ~CarrierCallbacksHandler() override
    {
        CarrierEventBus::Handler::BusDisconnect();
    }

    void Activate(Carrier* carrier)
    {
        m_carrier = carrier;
        CarrierEventBus::Handler::BusConnect(carrier->GetGridMate());
    }

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

        CarrierEventsBase  cdrToString;

        AZ_TracePrintf("CarrierTest", "OnFailedToConnect: Carrier:0x%p ConnectionID:0x%p Reason Code:%d (0x%02x) ReasonDef:%s\n",
            carrier,
            id,
            reason, reason,
            cdrToString.ReasonToString(reason).c_str() );

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

    Carrier*        m_carrier;
    ConnectionID    m_connectionID;
    ConnectionID    m_disconnectID;
    ConnectionID    m_incommingConnectionID;
    int             m_errorCode;
};



namespace UnitTest
{
    //static const unsigned int k_serverCarrier = 0;
    //static const unsigned int k_clientCarrier = 1;

    template<class SocketProvider = SocketDriverProvider, int ticksBeforeCheck = 50>
    class CarrierBasicTestTemplate
        : public GridMateMPTestFixture
        , public SocketProvider
    {
    public:
        void run()
        {
            AZ_TracePrintf("CarrierTest", "Initlizing test run \"%s\"\n", ::testing::UnitTest::GetInstance()->current_test_info()->name());
            CarrierCallbacksHandler clientCB, serverCB;
            TestCarrierDesc serverCarrierDesc, clientCarrierDesc;

            AZStd::string str("Hello this is a carrier test!");

            const char* targetAddress = "127.0.0.1";

#if AZ_TRAIT_GRIDMATE_TEST_SOCKET_IPV6_SUPPORT_ENABLED
            clientCarrierDesc.m_familyType = Driver::BSD_AF_INET6;
            serverCarrierDesc.m_familyType = Driver::BSD_AF_INET6;
            targetAddress = "::1";
#endif

            clientCarrierDesc.m_enableDisconnectDetection = false;
            serverCarrierDesc.m_enableDisconnectDetection = false;

            clientCarrierDesc.m_driver = SocketProvider::CreateDriverForJoin();
            serverCarrierDesc.m_driver = SocketProvider::CreateDriverForHost();

            clientCarrierDesc.m_port = 4427;
            serverCarrierDesc.m_port = 4428;

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            clientCB.Activate(clientCarrier);

            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);
            serverCB.Activate(serverCarrier);

            AZ_TracePrintf("CarrierTest", "Starting test run \"%s\"\n", ::testing::UnitTest::GetInstance()->current_test_info()->name());
            //////////////////////////////////////////////////////////////////////////
            // Test carriers [0 is server, 1 is client]
            bool isClientDone = false;
            bool isServerDone = false;
            bool isDisconnect = false;
            char clientBuffer[1500];
            char serverBuffer[1500];

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

                if (!isDisconnect && isClientDone && isServerDone && numUpdates > ticksBeforeCheck /* give enough time to close handshake */)
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

            AZ_TracePrintf("CarrierTest", "Completed test run \"%s\"\n", ::testing::UnitTest::GetInstance()->current_test_info()->name());

            //////////////////////////////////////////////////////////////////////////
            AZ_TEST_ASSERT(isServerDone && isClientDone);
        }
    };

    template<class SocketProvider = SocketDriverProvider>
    class CarrierAsyncHandshakeTestTemplate
        : public GridMateMPTestFixture
        , protected SocketProvider
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
            CarrierCallbacksHandler clientCB, serverCB;
            TestCarrierDesc serverCarrierDesc, clientCarrierDesc;

            AZStd::string str("Hello this is a carrier test!");
            clientCarrierDesc.m_driver = SocketProvider::CreateDriverForJoin();
            serverCarrierDesc.m_driver = SocketProvider::CreateDriverForHost();

            clientCarrierDesc.m_port = 4427;
            serverCarrierDesc.m_port = 4428;

            AsyncHandshake serverHandshake;
            serverCarrierDesc.m_handshake = &serverHandshake;

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            clientCB.Activate(clientCarrier);

            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);
            serverCB.Activate(serverCarrier);

            char buffer[1500];

            ConnectionID connId = InvalidConnectionID;
            int maxNumUpdates = 2000;
            int numUpdates = 0;

            bool clientReceived = false;
            bool serverReceived = false;

            while (numUpdates++ < maxNumUpdates)
            {
                if (numUpdates == 1)
                {
                    connId = clientCarrier->Connect("127.0.0.1", serverCarrierDesc.m_port);
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

    template<class SocketProvider = SocketDriverProvider>
    class CarrierStressTestTemplate
        : public GridMateMPTestFixture
        , protected SocketProvider
    {
    public:
        void run()
        {
            CarrierCallbacksHandler clientCB, serverCB;
            TestCarrierDesc serverCarrierDesc, clientCarrierDesc;

            AZStd::string str("Hello this is a carrier stress test!");

            clientCarrierDesc.m_enableDisconnectDetection = false;
            serverCarrierDesc.m_enableDisconnectDetection = false;
            clientCarrierDesc.m_threadUpdateTimeMS = 5;
            serverCarrierDesc.m_threadUpdateTimeMS = 5;
            //clientCarrierDesc.m_threadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
            //serverCarrierDesc.m_threadPriority = THREAD_PRIORITY_HIGHEST;
            clientCarrierDesc.m_port = 4427;
            serverCarrierDesc.m_port = 4428;

            clientCarrierDesc.m_driver = SocketProvider::CreateDriverForJoin();
            serverCarrierDesc.m_driver = SocketProvider::CreateDriverForHost();

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            clientCB.Activate(clientCarrier);

            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);
            serverCB.Activate(serverCarrier);

            //////////////////////////////////////////////////////////////////////////
            // Test carriers [0 is server, 1 is client]
            //char clientBuffer[1500];
            char serverBuffer[1500];

            ConnectionID connId = InvalidConnectionID;
            //int maxNumUpdates = 2000;
            int numUpdates = 0;
            int numSend = 0;
            int numRecv = 0;
            int numUpdatesLastPrint = 0;
            while (numRecv < 70000)
            {
                // Client
                if (connId == InvalidConnectionID)
                {
                    connId = clientCarrier->Connect("127.0.0.1", serverCarrierDesc.m_port);
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
                    /*Carrier::ConnectionStates clientState = */ clientCarrier->QueryStatistics(clientCB.m_connectionID, &clientStatsLastSecond, &clientStatsLifeTime);
                    /*Carrier::ConnectionStates serverState = */ serverCarrier->QueryStatistics(serverCB.m_connectionID, &serverStatsLastSecond, &serverStatsLifeTime);

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
    };

    template<class SocketProvider = SocketDriverProvider>
    class CarrierTestTemplate
        : public GridMateMPTestFixture
        , protected SocketProvider
    {
    public:
        void run()
        {
            //////////////////////////////////////////////////////////////////////////
            // Setup simulators
            DefaultSimulator clientSimulator;
            clientSimulator.Enable();
            clientSimulator.SetOutgoingLatency(150, 150); // in miliseconds
            clientSimulator.SetOutgoingPacketLoss(5, 5);  // drop 1 every X packets
            clientSimulator.SetOutgoingReorder(true);    // enable reorder

            clientSimulator.SetIncomingLatency(200, 200);
            clientSimulator.SetIncomingPacketLoss(7, 7);
            clientSimulator.SetIncomingReorder(true);
            clientSimulator.Enable();
            //////////////////////////////////////////////////////////////////////////

            CarrierCallbacksHandler clientCB, serverCB;
            TestCarrierDesc serverCarrierDesc, clientCarrierDesc;

            clientCarrierDesc.m_port = 4427;
            clientCarrierDesc.m_driver = SocketProvider::CreateDriverForJoin();
            //clientCarrierDesc.m_simulator = &clientSimulator;
            serverCarrierDesc.m_port = 4428;
            serverCarrierDesc.m_driver = SocketProvider::CreateDriverForHost();

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            clientCB.Activate(clientCarrier);
            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);
            serverCB.Activate(serverCarrier);

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
            int maxNumUpdates = 5000;
            int numUpdates = 0;
            bool isPrintStatus = false;
            //int isSkipPrint = 33;
            while (numUpdates <= maxNumUpdates)
            {
                // Client
                if (!isClientDone)
                {
                    if (connId == InvalidConnectionID)
                    {
                        connId = clientCarrier->Connect("127.0.0.1", serverCarrierDesc.m_port);
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
                                // we can reive messages only of intArray size
                                AZ_TEST_ASSERT(queryBufferSize >= sizeof(intArray));
                            }

                            receiveResult = clientCarrier->Receive(clientBuffer, 100, clientCB.m_connectionID); // receive with smaller buffer
                            switch (receiveResult.m_state)
                            {
                            case Carrier::ReceiveResult::NO_MESSAGE_TO_RECEIVE:
                            {
                                AZ_TEST_ASSERT(queryBufferSize == 0);     // make sure the query return 0 if we have no message to receive
                            } break;
                            case Carrier::ReceiveResult::UNSUFFICIENT_BUFFER_SIZE:
                            {
                                AZ_TEST_ASSERT(queryBufferSize > 0);     // we should have a message waiting if we fail to receive it
                            } break;
                            case Carrier::ReceiveResult::RECEIVED:
                            {
                                AZ_TEST_ASSERT(false);     // we have small buffer we should never be able to receive a message
                            } break;
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

    template<class SocketProvider = SocketDriverProvider>
    class CarrierDisconnectDetectionTestTemplate
        : public GridMateMPTestFixture
        , protected SocketProvider
    {
    public:
        void run()
        {
            // Setup simulators
            DefaultSimulator clientSimulator;
            clientSimulator.SetOutgoingPacketLoss(2, 2); // drop 50% packets

            TestCarrierDesc serverCarrierDesc;
            serverCarrierDesc.m_port = 4428;
            serverCarrierDesc.m_enableDisconnectDetection = true;
            serverCarrierDesc.m_disconnectDetectionPacketLossThreshold = 0.4f; // disconnect once hit 40% loss
            serverCarrierDesc.m_disconnectDetectionRttThreshold = 50; // disconnect once hit 50 msec rtt
            serverCarrierDesc.m_driver = SocketProvider::CreateDriverForHost();

            TestCarrierDesc clientCarrierDesc = serverCarrierDesc;
            clientCarrierDesc.m_port = 4427;
            clientCarrierDesc.m_simulator = &clientSimulator;
            clientCarrierDesc.m_driver = SocketProvider::CreateDriverForJoin();

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);

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

                clientCarrier->Connect("127.0.0.1", serverCarrierDesc.m_port); // connecting client -> server
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

    /*
     * Sends reliable messages across different channels to each other
     */
    template<class SocketProvider = SocketDriverProvider>
    class CarrierMultiChannelTestTemplate
        : public GridMateMPTestFixture
        , protected SocketProvider
    {
    public:
        void run()
        {
            AZ_TracePrintf("GridMate", "\n\n");

            // initialize transport
            int basePort = 4427;
            enum
            {
                c1,
                c2,
                nCarriers
            };

            int                     nMsgSent[nCarriers];
            int                     nMsgReceived[nCarriers];
            CarrierCallbacksHandler carrierHandlers[nCarriers];
            Carrier*                carriers[nCarriers];

            for (int i = 0; i < nCarriers; ++i)
            {
                // Create carriers
                TestCarrierDesc desc;
                desc.m_port = basePort + i;
                desc.m_driver = i == c1 ? SocketProvider::CreateDriverForHost() : SocketProvider::CreateDriverForJoin();
                desc.m_enableDisconnectDetection = true;
                carriers[i] = DefaultCarrier::Create(desc, m_gridMate);
                carrierHandlers[i].Activate(carriers[i]);

                nMsgSent[i] = nMsgReceived[i] = 0;
            }

            carriers[c2]->Connect("127.0.0.1", basePort + c1);

            int maxNumUpdates = 100;
            int numUpdates = 0;
            //TimeStamp time = AZStd::chrono::system_clock::now();
            while (numUpdates <= maxNumUpdates)
            {
                //////////////////////////////////////////////////////////////////////////
                Update();

                for (int iCarrier = 0; iCarrier < nCarriers; ++iCarrier)
                {
                    if (carrierHandlers[iCarrier].m_connectionID != InvalidConnectionID)
                    {
                        //AZ_TracePrintf("GridMate", "Updating carrier %d...\n", iCarrier);
                        for (unsigned int iConn = 0; iConn < carriers[iCarrier]->GetNumConnections(); ++iConn)
                        {
                            ConnectionID connId = carriers[iCarrier]->DebugGetConnectionId(iConn);
                            for (unsigned char iChannel = 0; iChannel < 3; ++iChannel)
                            {
                                char buf[1500];

                                // receive
                                Carrier::ReceiveResult receiveResult = carriers[iCarrier]->Receive(buf, AZ_ARRAY_SIZE(buf), connId, iChannel);
                                if (receiveResult.m_state == Carrier::ReceiveResult::RECEIVED)
                                {
                                    nMsgReceived[iCarrier]++;
                                    //AZ_TracePrintf("GridMate", "\t\tCarrier %d Received '%s' from channel %d.\n", iCarrier, buf, (int)iChannel);
                                }

                                // send something
                                if (numUpdates < 50)
                                {
                                    azsnprintf(buf, 1024, "Msg %d", nMsgSent[iCarrier]++);
                                    //AZ_TracePrintf("GridMate", "\tCarrier %d sending '%s' on channel %d.\n", iCarrier, buf, (int)iChannel);
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
                //if (numUpdates % 10 == 0)
                //  AZ_TracePrintf("Gridmate", "%d\n", numUpdates);
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

    /**
    * Stress tests multiple simultaneous Carriers
    */
    template<class SocketProvider = SocketDriverProvider>
    class CarrierMultiStressTestTemplate
        : public GridMateMPTestFixture
        , protected SocketProvider
    {
#define ThousandByteString "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"\
                                            "01234567890123456789012345678901234567890123456789"
    public:
        void run()
        {
            AZ_TracePrintf("GridMate", "CarrierMultiStressTest\n\n");

            // initialize transport
            const int k_numChannels = 1;
            const int basePort = 8080;  //Server port
            const int nCarriers = 101;  //0 is the server
            const int k_connectionTime = 50;    //Do not send for connection time at the start
            const int k_cleanupTime = 50;       //Do not send for cleanup time at the end
            const int maxNumUpdates = 100 + k_connectionTime + k_cleanupTime;
            const int k_numMessagesPerUpdate = 2;
            const int k_maxMsgSize = 1024;       //Shortens the raw application data
            const auto reliability = Carrier::SEND_UNRELIABLE;
            char buf[1500];

            int                     nMsgSent[nCarriers];
            int                     nMsgReceived[nCarriers];
            CarrierCallbacksHandler carrierHandlers[nCarriers];
            Carrier*                carriers[nCarriers];

            for (int i = 0; i < nCarriers; ++i)
            {
                // Create carriers
                TestCarrierDesc desc;
                {
                    desc.m_threadInstantResponse = true;
                    desc.m_threadUpdateTimeMS = 30;
                    desc.m_enableDisconnectDetection = false;
                }
                desc.m_port = basePort + i;
                desc.m_driver = (i == 0) ? SocketProvider::CreateDriverForHost() : SocketProvider::CreateDriverForJoin();
                AZ_TracePrintf("GridMate", "Opening %d\n", basePort + i);

                carriers[i] = DefaultCarrier::Create(desc, m_gridMate);
                carrierHandlers[i].Activate(carriers[i]);

                nMsgSent[i] = nMsgReceived[i] = 0;

                if ((i > 0))
                {
                    AZ_TracePrintf("GridMate", "Connecting from %d to %d\n", basePort + i, basePort);
                    carriers[i]->Connect("127.0.0.1", basePort);
                }
            }

            int numUpdates = 0;
            auto testStartTime = AZStd::chrono::system_clock::now();
            //TimeStamp time = AZStd::chrono::system_clock::now();
            while (numUpdates <= maxNumUpdates)
            {
                auto updateStartTime = AZStd::chrono::system_clock::now();
                //////////////////////////////////////////////////////////////////////////
                Update();

                for (int iCarrier = 0; iCarrier < nCarriers; ++iCarrier)
                {
                    if (carrierHandlers[iCarrier].m_connectionID != InvalidConnectionID && (numUpdates >= k_connectionTime))
                    {
                        //AZ_TracePrintf("GridMate", "Updating carrier %d...\n", iCarrier);
                        for (unsigned int iConn = 0; iConn < carriers[iCarrier]->GetNumConnections(); ++iConn)
                        {
                            ConnectionID connId = carriers[iCarrier]->DebugGetConnectionId(iConn);
                            for (unsigned char iChannel = 0; iChannel < k_numChannels; ++iChannel)
                            {
                                // receive
                                Carrier::ReceiveResult receiveResult = carriers[iCarrier]->Receive(buf, AZ_ARRAY_SIZE(buf), connId, iChannel);
                                while (receiveResult.m_state == Carrier::ReceiveResult::RECEIVED && receiveResult.m_numBytes > 0)
                                {
                                    nMsgReceived[iCarrier] += receiveResult.m_numBytes;
                                    receiveResult = carriers[iCarrier]->Receive(buf, AZ_ARRAY_SIZE(buf), connId, iChannel);
                                    //AZ_TracePrintf("GridMate", "\t\tCarrier %d Received '%s' from channel %d.\n", iCarrier, buf, (int)iChannel);
                                }

                                // send something
                                //if (numUpdates > k_connectionTime && numUpdates < maxNumUpdates - 10)
                                if (numUpdates < maxNumUpdates - k_cleanupTime)
                                {
                                    for (int i = 0; i < k_numMessagesPerUpdate; i++)
                                    {
                                        azsnprintf(buf, k_maxMsgSize,
                                            ThousandByteString
                                            "Msg %d", nMsgSent[iCarrier]);
                                        //AZ_TracePrintf("GridMate", "\tCarrier %d sending '%s' on channel %d.\n", iCarrier, buf, (int)iChannel);
                                        carriers[iCarrier]->Send(buf, (unsigned int)strlen(buf) + 1, connId, reliability, Carrier::PRIORITY_NORMAL, iChannel);
                                        nMsgSent[iCarrier] += int(strlen(buf) + 1);
                                    }
                                }
                            }
                        }
                    }
                    carriers[iCarrier]->Update();
                }

                //////////////////////////////////////////////////////////////////////////
                AZStd::chrono::milliseconds updateDuration = AZStd::chrono::system_clock::now() - updateStartTime;
                if (updateDuration.count() < 30)
                {
                    auto dur = updateDuration.count();
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30 - dur));
                }
                else
                {
                    AZStd::this_thread::yield();
                }
                numUpdates++;
                //if (numUpdates % 10 == 0)
                //  AZ_TracePrintf("Gridmate", "%d\n", numUpdates);
            }

            auto testDurationUS = (AZStd::chrono::system_clock::now() - testStartTime).count();
            long long int nSentBytes = 0;
            long long int nReceivedBytes = 0;
            for (int i = 0; i < nCarriers; ++i)
            {
                nSentBytes += nMsgSent[i];
                nReceivedBytes += nMsgReceived[i];
                DefaultCarrier::Destroy(carriers[i]);
                AZ_TEST_ASSERT(nMsgSent[i] > 0);
            }
            AZ_Printf("GridMate", "App MBytes sent/rcvd %.2f / %.2f dur %.2fS Mbps %.2f / %.2f\n",
                float(nSentBytes) / 1000000, float(nReceivedBytes) / 1000000, float(testDurationUS) / 1000000, float(nSentBytes * 8) / testDurationUS, float(nReceivedBytes * 8) / testDurationUS);
#if !defined(AZ_DEBUG_BUILD)
            AZ_TEST_ASSERT(testDurationUS < 8000000);       //Expected duration 6000000uS + margin
#else
            AZ_TEST_ASSERT(testDurationUS < 10000000);       //Expected duration 8000000uS + margin
#endif
            AZ_TEST_ASSERT(nSentBytes == nReceivedBytes);   //Nothing lost
        }
    };

    /*** Congestion control back pressure test */
    template<class SocketProvider = SocketDriverProvider>
    class CarrierBackpressureTestTemplate
        : public GridMateMPTestFixture
        , protected SocketProvider
        , public CarrierEventBus::Handler
    {
        // Test parameters
        static const AZ::u32 packetLossInterval = 2;  ///< Interval for lost packets (1 in X)
        static const AZ::u32 owttMS = 30;             ///< one-way-trip-time in MS

    public:
        void run()
        {
            CarrierEventBus::Handler::BusConnect(m_gridMate);
            char buf[1500];
            memset(buf, 0, sizeof(buf));

            // Setup simulators
            // Either use the DefaultSimulator with a fixed rate to simulate congestion
            // Or custom with specific drops injected

            DefaultSimulator clientSimulator;
            clientSimulator.SetOutgoingPacketLoss(packetLossInterval, packetLossInterval);
            //clientSimulator.SetOutgoingBandwidth(102400, 1024);   //100Mbps; has no effect in this test
            clientSimulator.SetIncomingLatency(owttMS, owttMS);
            clientSimulator.SetOutgoingLatency(owttMS, owttMS);

            TestCarrierDesc serverCarrierDesc;
            //serverCarrierDesc.m_threadInstantResponse = true;
            serverCarrierDesc.m_port = 4428;
            serverCarrierDesc.m_enableDisconnectDetection = true;
            serverCarrierDesc.m_disconnectDetectionPacketLossThreshold = 0.9f; // disconnect once hit 90% loss
            serverCarrierDesc.m_driver = SocketProvider::CreateDriverForHost();

            TestCarrierDesc clientCarrierDesc = serverCarrierDesc;
            //clientCarrierDesc.m_threadInstantResponse = true;
            clientCarrierDesc.m_port = 4427;
            clientCarrierDesc.m_simulator = &clientSimulator;
            clientCarrierDesc.m_disconnectDetectionPacketLossThreshold = 0.9f; // disconnect once hit 90% loss
            clientCarrierDesc.m_driver = SocketProvider::CreateDriverForJoin();

            carriers[0].carrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            carriers[0].isClient = true;
            carriers[1].carrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);

            Carrier* clientCarrier = carriers[0].carrier;
            Carrier* serverCarrier = carriers[1].carrier;

            //for (int testCaseNum = 0; testCaseNum < 1; ++testCaseNum)
            {
                int numUpdates = 0;
                int nMsgReceived = 0, nMsgSent = 0;

                clientCarrier->Connect("127.0.0.1", serverCarrierDesc.m_port); // loopback connect client -> server

                for (int attempts = 0; serverCarrier->GetNumConnections() == 0 && attempts <= 1000; attempts++) // wait until connected
                {
                    clientCarrier->Update();
                    serverCarrier->Update();

                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                }
                AZ_TEST_ASSERT(serverCarrier->GetNumConnections() == 1); // Must have connected

                ConnectionID clientId = clientCarrier->DebugGetConnectionId(0);
                ConnectionID serverId = serverCarrier->DebugGetConnectionId(0);

                //clientSimulator.Enable(); // After connecting enable bad traffic conditions
                static const int updatesPerSecond = 100;
                while (numUpdates++ <= 10*updatesPerSecond)
                {
                    AZ_TEST_ASSERT(serverCarrier->GetNumConnections() == 1); // Still connected
                    AZ_TEST_ASSERT(clientCarrier->GetNumConnections() == 1); // Still connected

                    if ( numUpdates == 1*updatesPerSecond)
                    {
                        clientSimulator.Enable(); // After stabilizing enable bad traffic conditions
                        carriers[0].eventualDecrease = true;
                        carriers[0].passed = false;
                    }
                    const unsigned char iChannel = 0; //Only one channel for this test
                    {
                        // receive server
                        Carrier::ReceiveResult receiveResult = serverCarrier->Receive(buf, AZ_ARRAY_SIZE(buf), serverId, iChannel);
                        if (receiveResult.m_state == Carrier::ReceiveResult::RECEIVED)
                        {
                            nMsgReceived++;
                            //AZ_TracePrintf("GridMate", "\t\tCarrier %d Received '%s' from channel %d.\n", iCarrier, buf, (int)iChannel);
                        }

                        // send something from client
                        //if (numUpdates < 50)
                        {
                            azsnprintf(buf, 1024, "Msg %d", nMsgSent++);
                            //AZ_TracePrintf("GridMate", "\tCarrier %d sending '%s' on channel %d.\n", iCarrier, buf, (int)iChannel);
                            clientCarrier->Send(buf, (unsigned int)strlen(buf) + 1, clientId, Carrier::SEND_RELIABLE, Carrier::PRIORITY_NORMAL, iChannel);
                        }
                    }

                    //AZ_TracePrintf("GridMate", "  Server -> Client:");

                    //Log every 100th update (5/sec)
                    if (!(numUpdates % 100) && serverCarrier->GetNumConnections() == 1)
                    {
                        TrafficControl::Statistics stats, sessionStats;
                        Carrier::FlowInformation flowInfo;
                        serverCarrier->QueryStatistics(serverCarrier->DebugGetConnectionId(0), &stats, &sessionStats, nullptr, nullptr, &flowInfo);
                        AZ_TracePrintf("GridMate", "  Server -> Client: rtt=%.0f msec, packetLoss=%.0f%%/%.0f%%, cwnd=%u\n",
                            stats.m_rtt, stats.m_packetLoss  * 100.f, sessionStats.m_packetLoss  * 100.f, static_cast<AZ::u32>(flowInfo.m_congestionWindow));
                    }
                    if (!(numUpdates % 100) && clientCarrier->GetNumConnections() == 1)
                    {
                        TrafficControl::Statistics stats, sessionStats;
                        Carrier::FlowInformation flowInfo;
                        clientCarrier->QueryStatistics(clientCarrier->DebugGetConnectionId(0), &stats, &sessionStats, nullptr, nullptr, &flowInfo);
                        AZ_TracePrintf("GridMate", "  Client -> Server: rtt=%.0f msec, packetLoss=%.0f%%/%.0f%%, cwnd=%u\n",
                            stats.m_rtt, stats.m_packetLoss  * 100.f, sessionStats.m_packetLoss  * 100.f, static_cast<AZ::u32>(flowInfo.m_congestionWindow));
                    }

                    clientCarrier->Update();
                    serverCarrier->Update();

                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1000 / updatesPerSecond));

                    if ( numUpdates == 4*updatesPerSecond)
                    {
                        //AZ_TEST_ASSERT(passedAllTests());
                        clientSimulator.SetOutgoingPacketLoss(0, 0);    // After stabilizing disable bad traffic conditions
                                                                        //WARN: disabling the simulator causes very high jitter
                                                                        //and ruins the test data
                        carriers[0].eventualIncrease = true;
                        carriers[0].passed = false;
                    }
                }

                AZ_TEST_ASSERT(serverCarrier->GetNumConnections() == 1); // Still connected
                clientSimulator.Disable();
            }

            AZ_TEST_ASSERT(passedAllTests());

            DefaultCarrier::Destroy(clientCarrier);
            DefaultCarrier::Destroy(serverCarrier);
        }

        protected:
            struct CarrierTest
            {
                Carrier* carrier;
                AZ::u32 bytesPerSecond = 0;        ///< 0 is uninitialized
                AZ::u32 maxBPS = 0;
                AZ::u32 minBPS = UINT32_MAX;
                bool isClient = false;
                bool eventualDecrease = false;      ///< when loss simulator is enabled, this carrier must eventually decrease
                bool eventualIncrease = false;      ///< when loss simulator is disabled, this carrier must eventually increase
                bool passed = true;                 ///< set to false when test is started
            };

            //////////////////////////////////////////////////////////////////////////
            // CarrierEventBus
            void OnFailedToConnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) override
            {
                (void)carrier;
                (void)id;
                (void)reason;
                AZ_Assert(false, "Test failed to connect!");
            };
            void OnConnectionEstablished(Carrier* carrier, ConnectionID id) override
            {
                (void)id;
                if (!isOurCarrier(carrier))
                {
                    return;
                }
                AZ_Assert(carrier, "NULL carrier!");
                CarrierEventBus::Handler::BusConnect(carrier->GetGridMate());
            };
            void OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) override
            {
                (void)id;
                (void)reason;
                if (!isOurCarrier(carrier))
                {
                    return;
                }
                AZ_Assert(carrier, "NULL carrier!");
                CarrierEventBus::Handler::BusDisconnect(carrier->GetGridMate());
            };
            void OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error) override
            {
                (void)carrier;
                (void)id;
                (void)error;
                AZ_Assert(false, "Test reported driver error!");
            };
            void OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error) override
            {
                (void)carrier;
                (void)id;
                (void)error;
                //Ignore security warnings in unit tests
            };

            void OnRateChange(Carrier* carrier, ConnectionID id, AZ::u32 sendLimitBytesPerSec) override
            {
                CarrierTest* test = isOurCarrier(carrier);
                if ( !test )
                {
                    return;
                }
                AZ_Assert(carrier, "NULL carrier!");
                AZ_Assert(carrier->GetNumConnections() == 1, "Rate change reported on carrier with no connections!");

                TrafficControl::Statistics stats, sessionStats;
                Carrier::FlowInformation flowInfo;
                carrier->QueryStatistics(id, &stats, &sessionStats, nullptr, nullptr, &flowInfo);


                if (test->bytesPerSecond && test->eventualDecrease && (sendLimitBytesPerSec < test->bytesPerSecond))
                {
                    test->passed = true;        //Simple test only verifies that it reported congestion once
                                                //due to different congestion control/avoidance implementations
                    //AZ_TracePrintf("GridMate", "  Rate change: %s rtt=%.0f msec, packetLoss=%.0f%%/%.0f%%, BytesPerSec=%u\n",
                    //    (test->isClient ? "Client" : "Server"), stats.m_rtt, stats.m_packetLoss  * 100.f, sessionStats.m_packetLoss  * 100.f, sendLimitBytesPerSec);
                    test->minBPS = AZStd::GetMin(test->minBPS, sendLimitBytesPerSec);
                }
                if (test->bytesPerSecond && test->eventualIncrease && (sendLimitBytesPerSec > test->bytesPerSecond))
                {
                    test->passed = true;        //Simple test only verifies that after congestion clears up the rate increases
                    //AZ_TracePrintf("GridMate", "  Rate change: %s rtt=%.0f msec, packetLoss=%.0f%%/%.0f%%, BytesPerSec=%u\n",
                    //    (test->isClient ? "Client" : "Server"), stats.m_rtt, stats.m_packetLoss  * 100.f, sessionStats.m_packetLoss  * 100.f, sendLimitBytesPerSec);
                    test->maxBPS = AZStd::GetMax(test->maxBPS, sendLimitBytesPerSec);
                }
                AZ_Assert(sendLimitBytesPerSec > 1000, "Should not allow decreasing below 1000Bps! Attempted %u", sendLimitBytesPerSec);

                test->bytesPerSecond = sendLimitBytesPerSec;


            };
            //End CarrierEventBus
            //////////////////////////////////////////////////////////////////////////
        private:
            CarrierTest carriers[2];               ///< 0 = client, 1 = server

            CarrierTest* isOurCarrier(Carrier* carrier)
            {
                for (CarrierTest& cr : carriers)
                {
                    if (cr.carrier == carrier)
                    {
                        return &cr;
                    }
                }
                return nullptr;
            }
            bool passedAllTests() const
            {
                for (const CarrierTest& cr : carriers)
                {
                    AZ_TracePrintf("GridMate", " Carrier %p : %s Min=%u, Max=%u\n", cr.carrier, (cr.isClient?"Client":"Server"), cr.minBPS, cr.maxBPS);
                    if (cr.isClient)
                    {
                        //TODO: tested to work with custom congestion control but disabled for now until congestion control enabled
                        //AZ_TEST_ASSERT(cr.passed);
                        //AZ_TEST_ASSERT(cr.minBPS < (0.1*cr.maxBPS));
                    }
                }
                return true;
            }

    };

    template<class SocketProvider = SocketDriverProvider>
    class CarrierACKTestTemplate
        : public GridMateMPTestFixture
        , protected SocketProvider
    {
    public:
        void run()
        {
            if (!ReplicaTarget::IsAckEnabled())
            {
                return;
            }

#if AZ_TRAIT_GRIDMATE_TEST_SOCKET_IPV6_SUPPORT_ENABLED
            bool useIpv6 = true;
#else
            bool useIpv6 = false;
#endif

            CarrierCallbacksHandler clientCB, serverCB;
            CarrierDesc serverCarrierDesc, clientCarrierDesc;

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

            clientCarrierDesc.m_driver = SocketProvider::CreateDriverForJoin();
            serverCarrierDesc.m_driver = SocketProvider::CreateDriverForHost();

            clientCarrierDesc.m_port = 4427;
            serverCarrierDesc.m_port = 4428;

            Carrier* clientCarrier = DefaultCarrier::Create(clientCarrierDesc, m_gridMate);
            clientCB.Activate(clientCarrier);

            Carrier* serverCarrier = DefaultCarrier::Create(serverCarrierDesc, m_gridMate);
            serverCB.Activate(serverCarrier);

            //////////////////////////////////////////////////////////////////////////
            // Test carriers [0 is server, 1 is client]
            bool isClientDone = false;
            bool isServerDone = false;
            bool isDisconnect = false;
            char clientBuffer[1500];
            char serverBuffer[1500];

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
                            auto m_callback = AZStd::make_unique<ACKCallback>((++m_targetStamp), &m_currentStamp);
                            clientCarrier->SendWithCallback(str.c_str(), static_cast<unsigned>(str.length() + 1), AZStd::move(m_callback), clientCB.m_connectionID, Carrier::SEND_UNRELIABLE);
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
                    AZ_TEST_ASSERT(m_targetStamp == m_currentStamp);

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
    protected:

        /** Modelled after TargetCallback
         */
        class ACKCallback final : public CarrierACKCallback
        {
        public:
            ACKCallback(unsigned int stamp, unsigned int *currentStamp)
                : m_stamp(stamp)
                , m_currentStamp(currentStamp) {}

            AZ_FORCE_INLINE void Run() override
            {
                AZ_Assert(m_stamp >= *m_currentStamp, "Cannot perform retrograde increase on replica state. Possible network re-ordering: %u<%u.", m_stamp, m_currentStamp);

                //AZ_Printf("Callback", "Recvd TargetCallback!");
                *m_currentStamp = m_stamp;
            }
        private:
            AZ::u32 m_stamp;
            AZ::u32 *m_currentStamp;
        };

        unsigned int m_currentStamp = 1;
        unsigned int m_targetStamp = 2;
        //AZStd::shared_ptr<ACKCallback> m_callback;
    };

    //Create specific tests
    using CarrierBasicTest = CarrierBasicTestTemplate<>;
    using CarrierTest = CarrierTestTemplate<>;
    using DISABLED_CarrierDisconnectDetectionTest = CarrierDisconnectDetectionTestTemplate<>;
    using DISABLED_CarrierAsyncHandshakeTest = CarrierAsyncHandshakeTestTemplate<>;
    using DISABLED_CarrierStressTest = CarrierStressTestTemplate<>;
    using DISABLED_CarrierMultiChannelTest = CarrierMultiChannelTestTemplate<>;
    using DISABLED_CarrierMultiStressTest = CarrierMultiStressTestTemplate<>;
    using DISABLED_CarrierBackpressureTest = CarrierBackpressureTestTemplate<>;
    using DISABLED_CarrierACKTest = CarrierACKTestTemplate<>;

#if AZ_TRAIT_GRIDMATE_TEST_WITH_SECURE_SOCKET_DRIVER

    /** Drops DTLS messages in handshake sequence order
     */
    template<bool isClient>
    class SecureSocketHandshakeDrop : public SecureSocketDriver
    {
    public:
        SecureSocketHandshakeDrop(SecureSocketDesc desc)
            : SecureSocketDriver(desc)
            , m_handshakeSeqToDiscard(0)
            , m_discardChangeCipherSpec(true)
            , m_finishedCookieExchange(false)
            , m_discardFinish(true)
            , m_isClient(isClient)
        {}
        void ProcessIncoming() override
        {
            SecureSocketDriver::ProcessIncoming();
        }
        void ProcessOutgoing() override
        {
            //Replaces call to FlushConnectionBuffersToSocket() with identical code that drops the specific handshake messages
            for (AZStd::pair<SocketDriverAddress, Connection*>& addrConn : m_connections)
            {
                Connection* connection = addrConn.second;
                connection->FlushOutgoingDTLSDgrams();
                int packetsToDrop = 1;  //only drop 1 packet per flight
                while (CanSend())
                {
                    AZ::s32 bytesRead = connection->GetDTLSDgram(m_tempSocketWriteBuffer, m_maxTempBufferSize);
                    if (bytesRead <= 0)
                    {
                        break;
                    }

                    ////Drop check
                    if (ConnectionSecurity::IsHandshake(m_tempSocketWriteBuffer, bytesRead))
                    {
                        AZ::u16 sequenceNum = *(reinterpret_cast<AZ::u16*>(m_tempSocketWriteBuffer + 17)); //Sequence # in header offset by 17
                        AZStd::endian_swap<AZ::u16>(sequenceNum);
                        const char* type = "";
                        if (sequenceNum < 6)
                        {
                            type = ConnectionSecurity::TypeToString(m_tempSocketWriteBuffer, bytesRead);
                        }

                        if (packetsToDrop > 0)
                        {
                            if (sequenceNum == m_handshakeSeqToDiscard)
                            {
                                AZ_TracePrintf("GridMate", "[%08x] HShake Seq %d %s (DROPPED)\n", this, sequenceNum, type);

                                ++m_handshakeSeqToDiscard;
                                //Move back to 0 after cookie exchange
                                if (m_isClient && !m_finishedCookieExchange && sequenceNum == 1)
                                {
                                    m_finishedCookieExchange = true;
                                    m_handshakeSeqToDiscard = 0;
                                }
                                --packetsToDrop;
                                continue;
                            }
                            else if (m_discardFinish && sequenceNum > 5)    //Finish message
                            {
                                AZ_TracePrintf("GridMate", "[%08x] HShake Seq %d %s (DROPPED)\n", this, sequenceNum, type);
                                m_discardFinish = false;
                                --packetsToDrop;
                                continue;
                            }
                        }

                        AZ_TracePrintf("GridMate", "[%08x] HShake Seq %d %s\n", this, sequenceNum, type);
                        //AZ_TracePrintf("GridMate", "%s\n", AZStd::MemoryToASCII::ToString(m_tempSocketWriteBuffer, bytesRead, 64).c_str());
                    }
                    else if (ConnectionSecurity::IsChangeCipherSpec(m_tempSocketWriteBuffer, bytesRead))
                    {
                        if (packetsToDrop > 0 && m_discardChangeCipherSpec)
                        {
                            AZ_TracePrintf("GridMate", "[%08x] ChangeCipherSpec (DROPPED) \n", this, m_port);
                            m_discardChangeCipherSpec = false;
                            --packetsToDrop;
                            continue;
                        }
                        AZ_TracePrintf("GridMate", "[%08x] ChangeCipherSpec\n", this, m_port);
                    }
                    ////END Drop check

                    DriverAddress* addr = static_cast<DriverAddress*>(&addrConn.first);
                    SocketDriver::Send(addr, m_tempSocketWriteBuffer, bytesRead);
                    connection->m_dbgDgramsSent++;
                }
            }
        }
    private:
        int     m_handshakeSeqToDiscard;
        bool    m_discardChangeCipherSpec;
        bool    m_discardFinish;
        bool    m_finishedCookieExchange;
        bool    m_isClient;
    };

    using SecureProviderBadClient = SecureDriverProvider<SecureSocketHandshakeDrop<true>>;
    using SecureProviderBadHost = SecureDriverProvider<SecureSocketDriver, SecureSocketHandshakeDrop<false>>;
    using SecureProviderBadBoth = SecureDriverProvider<SecureSocketHandshakeDrop<true>, SecureSocketHandshakeDrop<false>>;

    using DISABLED_CarrierSecureSocketHandshakeTestClient = CarrierBasicTestTemplate<SecureProviderBadClient, 200>;
    using DISABLED_CarrierSecureSocketHandshakeTestHost = CarrierBasicTestTemplate<SecureProviderBadHost, 200>;
    using DISABLED_CarrierSecureSocketHandshakeTestBoth = CarrierBasicTestTemplate<SecureProviderBadBoth, 200>;

    //Create secure socket variants of tests
    using CarrierBasicTestSecure = CarrierBasicTestTemplate<SecureDriverProvider<>>;
    using CarrierTestSecure = CarrierTestTemplate<SecureDriverProvider<>>;
    using DISABLED_CarrierDisconnectDetectionTestSecure = CarrierDisconnectDetectionTestTemplate<SecureDriverProvider<>>;
    using DISABLED_CarrierAsyncHandshakeTestSecure = CarrierAsyncHandshakeTestTemplate<SecureDriverProvider<>>;
    using DISABLED_CarrierStressTestSecure = CarrierStressTestTemplate<SecureDriverProvider<>>;
    using DISABLED_CarrierMultiChannelTestSecure = CarrierMultiChannelTestTemplate<SecureDriverProvider<>>;
    using DISABLED_CarrierMultiStressTestSecure = CarrierMultiStressTestTemplate<SecureDriverProvider<>>;
    using DISABLED_CarrierBackpressureTestSecure = CarrierBackpressureTestTemplate<SecureDriverProvider<>>;
    using DISABLED_CarrierACKTestSecure = CarrierACKTestTemplate<SecureDriverProvider<>>;

#endif
}

struct GridMateCarrierTestFixture 
    : public ::testing::Test
    , public UnitTest::GridMateMPTestFixture
{
};

namespace GridMate
{
    namespace Platform
    {
        const char* GetSocketErrorString(int error, SocketErrorBuffer& array);
    }
}

    // The following test is not valid on Posix systems

#if AZ_TRAIT_DISABLE_FAILED_GRIDMATE_TESTS
TEST_F(GridMateCarrierTestFixture, DISABLED_Test_GetSocketErrorString)
#else
TEST_F(GridMateCarrierTestFixture, Test_GetSocketErrorString)
#endif // AZ_TRAIT_DISABLE_FAILED_GRIDMATE_TESTS
{
    SocketErrorBuffer buffer;
    const char* socketErrorString = GridMate::Platform::GetSocketErrorString(AZ_EWOULDBLOCK, buffer);

    SocketErrorBuffer expectedBuffer;

#if AZ_TRAIT_USE_POSIX_STRERROR_R
    static constexpr char posixErrorWouldBlockPosixErrStr[] = "Resource temporarily unavailable";
    azsnprintf(expectedBuffer.data(), expectedBuffer.size()-1 , "%s", posixErrorWouldBlockPosixErrStr);
#else
    azsnprintf(expectedBuffer.data(), expectedBuffer.size()-1 , "%ld", AZ_EWOULDBLOCK);
#endif // !AZ_TRAIT_USE_POSIX_STRERROR_R
    EXPECT_STREQ(expectedBuffer.data(), socketErrorString);
    EXPECT_STREQ(expectedBuffer.data(), buffer.data());

}


GM_TEST_SUITE(CarrierSuite)
#if !AZ_TRAIT_GRIDMATE_UNIT_TEST_DISABLE_CARRIER_SESSION_TESTS
GM_TEST(CarrierBasicTest)
GM_TEST(CarrierTest)
#endif //AZ_TRAIT_GRIDMATE_UNIT_TEST_DISABLE_CARRIER_SESSION_TESTS
GM_TEST(DISABLED_CarrierAsyncHandshakeTest)
#if !defined(AZ_DEBUG_BUILD) // this test is a little slow for debug
GM_TEST(DISABLED_CarrierStressTest)
GM_TEST(DISABLED_CarrierMultiStressTest)
#endif
GM_TEST(DISABLED_CarrierMultiChannelTest)
GM_TEST(DISABLED_CarrierBackpressureTest)
GM_TEST(DISABLED_CarrierACKTest)


#if AZ_TRAIT_GRIDMATE_TEST_WITH_SECURE_SOCKET_DRIVER
GM_TEST(DISABLED_CarrierBasicTestSecure)
GM_TEST(DISABLED_CarrierSecureSocketHandshakeTestClient)
GM_TEST(DISABLED_CarrierSecureSocketHandshakeTestHost)
GM_TEST(DISABLED_CarrierSecureSocketHandshakeTestBoth)
GM_TEST(CarrierTestSecure)
GM_TEST(DISABLED_CarrierAsyncHandshakeTestSecure)
#if !defined(AZ_DEBUG_BUILD) // this test is a little slow for debug
GM_TEST(DISABLED_CarrierStressTestSecure)
GM_TEST(DISABLED_CarrierMultiStressTestSecure)
#endif
GM_TEST(DISABLED_CarrierMultiChannelTestSecure)
GM_TEST(DISABLED_CarrierBackpressureTestSecure)
GM_TEST(DISABLED_CarrierACKTestSecure)

#endif

GM_TEST_SUITE_END()
