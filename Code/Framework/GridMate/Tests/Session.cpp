/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>

#include "Tests.h"

#include <GridMate/Session/LANSession.h>
#include <GridMate/Carrier/DefaultSimulator.h>
#include <GridMate/Carrier/Driver.h>

#include <GridMateTests_Traits_Platform.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/conversions.h>

using namespace GridMate;

//#define AZ_LAN_TEST_MAIN_THREAD_BLOCKED

#define AZ_TEST_LANLATENCY_ENABLE_MONSTER_BUFFER

namespace UnitTest
{
    /*
    * Utility function to tick the replica manager
    */
    void UpdateReplicaManager(ReplicaManager* replicaManager)
    {
        if (replicaManager)
        {
            replicaManager->Unmarshal();
            replicaManager->UpdateFromReplicas();
            replicaManager->UpdateReplicas();
            replicaManager->Marshal();
        }
    }

    class DISABLED_LANSessionMatchmakingParamsTest
        : public GridMateMPTestFixture
        , public SessionEventBus::MultiHandler
    {
        void OnSessionCreated(GridSession* gridSession) override
        {
            AZ_TEST_ASSERT(m_hostSession == nullptr);
            AZ_TEST_ASSERT(gridSession->IsHost());
            m_hostSession = gridSession;
        }

    public:
        DISABLED_LANSessionMatchmakingParamsTest(bool useIPv6 = false)
            : m_hostSession(nullptr)
            , m_clientGridMate(nullptr)
        {
            m_driverType = useIPv6 ? Driver::BSD_AF_INET6 : Driver::BSD_AF_INET;

            //////////////////////////////////////////////////////////////////////////
            // Create all grid mates
            SessionEventBus::MultiHandler::BusConnect(m_gridMate);
            StartGridMateService<LANSessionService>(m_gridMate, SessionServiceDesc());
            AZ_TEST_ASSERT(GridMate::LANSessionServiceBus::FindFirstHandler(m_gridMate) != nullptr);

            m_clientGridMate = GridMateCreate(GridMateDesc());
            AZ_TEST_ASSERT(m_clientGridMate);
            SessionEventBus::MultiHandler::BusConnect(m_clientGridMate);
            StartGridMateService<LANSessionService>(m_clientGridMate, SessionServiceDesc());
            AZ_TEST_ASSERT(GridMate::LANSessionServiceBus::FindFirstHandler(m_clientGridMate) != nullptr);
            //////////////////////////////////////////////////////////////////////////
        }
        ~DISABLED_LANSessionMatchmakingParamsTest() override
        {
            SessionEventBus::MultiHandler::BusDisconnect(m_gridMate);
            SessionEventBus::MultiHandler::BusDisconnect(m_clientGridMate);
            StopGridMateService<LANSessionService>(m_gridMate);

            GridMateDestroy(m_clientGridMate);
        }

        void run()
        {
            TestCarrierDesc carrierDesc;
            carrierDesc.m_enableDisconnectDetection = true;
            carrierDesc.m_threadUpdateTimeMS = 10;
            carrierDesc.m_familyType = m_driverType;

            // Start the host with one parameter of each type
            LANSessionParams sp;
            sp.m_topology = ST_PEER_TO_PEER;
            sp.m_numPublicSlots = 2;
            sp.m_port = k_hostPort;
            sp.m_params[sp.m_numParams].m_id = "VT_INT32";
            sp.m_params[sp.m_numParams].SetValue(static_cast<AZ::s32>(32));
            sp.m_numParams++;
            sp.m_params[sp.m_numParams].m_id = "VT_INT64";
            sp.m_params[sp.m_numParams].SetValue(static_cast<AZ::s64>(64));
            sp.m_numParams++;
            sp.m_params[sp.m_numParams].m_id = "VT_FLOAT";
            sp.m_params[sp.m_numParams].SetValue(32.f);
            sp.m_numParams++;
            sp.m_params[sp.m_numParams].m_id = "VT_DOUBLE";
            sp.m_params[sp.m_numParams].SetValue(static_cast<double>(64.0));
            sp.m_numParams++;
            sp.m_params[sp.m_numParams].m_id = "VT_STRING";
            sp.m_params[sp.m_numParams].SetValue("string");
            sp.m_numParams++;
            GridSession* hostSession = nullptr;
            EBUS_EVENT_ID_RESULT(hostSession,m_gridMate,LANSessionServiceBus,HostSession,sp,carrierDesc);
            AZ_TEST_ASSERT(hostSession);

            // Wait for session to be hosted
            while (m_hostSession != hostSession)
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                m_gridMate->Update();
                UpdateReplicaManager(hostSession->GetReplicaMgr());
            }

            // Perfrom searches
            GridSearch* searchHandle = nullptr;
            LANSearchParams searchParams;
            searchParams.m_serverPort = k_hostPort;
            searchParams.m_listenPort = 0;
            searchParams.m_numParams = sp.m_numParams;
            for (unsigned int iParam = 0; iParam < searchParams.m_numParams; ++iParam)
            {
                static_cast<GridSessionParam&>(searchParams.m_params[iParam]) = sp.m_params[iParam];
                searchParams.m_params[iParam].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
            }

            for (unsigned int iParam = 0; iParam < searchParams.m_numParams; ++iParam)
            {
                // Change parameter iParam to force a mismatch
                searchParams.m_params[iParam].SetValue(0);

                EBUS_EVENT_ID_RESULT(searchHandle,m_clientGridMate,LANSessionServiceBus,StartGridSearch,searchParams);
                while (!searchHandle->IsDone())
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));

                    m_gridMate->Update();
                    UpdateReplicaManager(hostSession->GetReplicaMgr());

                    m_clientGridMate->Update();
                }
                AZ_TEST_ASSERT(searchHandle->GetNumResults() == 0);
                searchHandle->Release();

                // Restore the parameter
                static_cast<GridSessionParam&>(searchParams.m_params[iParam]) = sp.m_params[iParam];
            }

            // Perform search with all matching parameters
            EBUS_EVENT_ID_RESULT(searchHandle,m_clientGridMate,LANSessionServiceBus,StartGridSearch,searchParams);
            while (!searchHandle->IsDone())
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));

                m_gridMate->Update();
                UpdateReplicaManager(hostSession->GetReplicaMgr());

                m_clientGridMate->Update();
            }
            AZ_TEST_ASSERT(searchHandle->GetNumResults() == 1);
            AZ_TEST_ASSERT(searchHandle->GetResult(0)->m_sessionId == m_hostSession->GetId());
            searchHandle->Release();

            // Perform search with no parameters
            searchParams.m_numParams = 0;
            EBUS_EVENT_ID_RESULT(searchHandle,m_clientGridMate,LANSessionServiceBus,StartGridSearch,searchParams);
            while (!searchHandle->IsDone())
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));

                m_gridMate->Update();
                UpdateReplicaManager(hostSession->GetReplicaMgr());

                m_clientGridMate->Update();
            }
            AZ_TEST_ASSERT(searchHandle->GetNumResults() == 1);
            AZ_TEST_ASSERT(searchHandle->GetResult(0)->m_sessionId == m_hostSession->GetId());
            searchHandle->Release();
        }

        static const int k_hostPort = 5450;

        Driver::BSDSocketFamilyType m_driverType;
        GridSession* m_hostSession;
        IGridMate* m_clientGridMate;
    };

    class DISABLED_LANSessionTest
        : public GridMateMPTestFixture
    {
        class TestPeerInfo
            : public SessionEventBus::Handler
        {
        public:
            TestPeerInfo()
                : m_gridMate(nullptr)
                , m_lanSearch(nullptr)
                , m_session(nullptr)
                , m_connections(0) {}

            // Callback that notifies the title when a game search's query have completed.
            void OnGridSearchComplete(GridSearch* gridSearch) override
            {
                AZ_TEST_ASSERT(gridSearch->IsDone() == true);
            }

            // Callback that notifies the title when a new player joins the game session.
            void OnMemberJoined(GridSession* session, GridMember* member) override
            {
                if (session == m_session)
                {
                    if (member != m_session->GetMyMember())
                    {
                        m_connections++;
                    }
                }
            }
            // Callback that notifies the title that a player is leaving the game session. member pointer is NOT valid after the callback returns.
            void OnMemberLeaving(GridSession* session, GridMember* member) override
            {
                if (session == m_session)
                {
                    if (member != m_session->GetMyMember())
                    {
                        m_connections--;
                    }
                }
            }
            /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
            void OnMemberKicked(GridSession* session, GridMember* member, AZ::u8 reason) override
            {
                (void)session;
                (void)member;
                (void)reason;
            }

            void OnSessionError(GridSession* session, const AZStd::string& /*errorMsg*/) override
            {
                (void)session;
#ifndef AZ_LAN_TEST_MAIN_THREAD_BLOCKED  // we will receive an error is we block for a long time
                AZ_TEST_ASSERT(false);
#endif
            }
            // Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
            void OnSessionDelete(GridSession* session) override
            {
                if (session == m_session)
                {
                    m_session = nullptr;
                }
            }

            IGridMate* m_gridMate;
            GridSearch* m_lanSearch;
            GridSession* m_session;
            int m_connections;
        };

    public:
        DISABLED_LANSessionTest(bool useIPv6 = false)
        {
            m_driverType = useIPv6 ? Driver::BSD_AF_INET6 : Driver::BSD_AF_INET;
            m_doSessionParamsTest = k_numMachines > 1;

            //////////////////////////////////////////////////////////////////////////
            // Create all grid mates
            m_peers[0].m_gridMate = m_gridMate;
            m_peers[0].SessionEventBus::Handler::BusConnect(m_peers[0].m_gridMate);
            for (int i = 1; i < k_numMachines; ++i)
            {
                GridMateDesc desc;
                m_peers[i].m_gridMate = GridMateCreate(desc);
                AZ_TEST_ASSERT(m_peers[i].m_gridMate);

                m_peers[i].SessionEventBus::Handler::BusConnect(m_peers[i].m_gridMate);
            }
            //////////////////////////////////////////////////////////////////////////

            for (int i = 0; i < k_numMachines; ++i)
            {
                // start the multiplayer service (session mgr, extra allocator, etc.)
                StartGridMateService<LANSessionService>(m_peers[i].m_gridMate, SessionServiceDesc());
                AZ_TEST_ASSERT(LANSessionServiceBus::FindFirstHandler(m_peers[i].m_gridMate) != nullptr);
            }
        }
        ~DISABLED_LANSessionTest() override
        {
            StopGridMateService<LANSessionService>(m_peers[0].m_gridMate);

            for (int i = 1; i < k_numMachines; ++i)
            {
                if (m_peers[i].m_gridMate)
                {
                    m_peers[i].SessionEventBus::Handler::BusDisconnect();
                    GridMateDestroy(m_peers[i].m_gridMate);
                }
            }

            //StopDrilling();

            // this will stop the first IGridMate which owns the memory allocators.
            m_peers[0].SessionEventBus::Handler::BusDisconnect();
        }

        void run()
        {
            //StartDrilling("lansession");

            TestCarrierDesc carrierDesc;
            carrierDesc.m_enableDisconnectDetection = /*false*/ true;
            carrierDesc.m_threadUpdateTimeMS = 10;
            carrierDesc.m_familyType = m_driverType;

            // On platforms without loopback, first search for an existing session and if none is found, host one.
            // Otherwise the first gridmate instance will host and the rest will join.
            int numMachines = k_numMachines;
            if (numMachines == 1)
            {
                LANSearchParams searchParams;
                searchParams.m_serverPort = k_hostPort;
                searchParams.m_listenPort = k_hostPort;
                searchParams.m_numParams = 1;
                searchParams.m_params[0].m_id = "Param2";
                searchParams.m_params[0].SetValue(25);
                searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                searchParams.m_familyType = m_driverType;

                EBUS_EVENT_ID_RESULT(m_peers[0].m_lanSearch,m_peers[0].m_gridMate,LANSessionServiceBus,StartGridSearch,searchParams);
                while (!m_peers[0].m_lanSearch->IsDone())
                {
                    m_peers[0].m_gridMate->Update();
                    Update();
                }
                int numResults = m_peers[0].m_lanSearch->GetNumResults();
                if (numResults == 0)
                {
                    // We will host a session... no result
                    LANSessionParams sp;
                    sp.m_topology = ST_PEER_TO_PEER;
                    sp.m_numPublicSlots = 64;
                    sp.m_port = k_hostPort;
                    sp.m_numParams = 2;
                    sp.m_params[0].m_id = "Param1";
                    sp.m_params[0].SetValue(15);
                    sp.m_params[1].m_id = "Param2";
                    sp.m_params[1].SetValue(25);
                    sp.m_flags = LANSessionParams::SF_HOST_MIGRATION_NO_EMPTY_SESSIONS;

                    EBUS_EVENT_ID_RESULT(m_peers[0].m_session,m_peers[0].m_gridMate,LANSessionServiceBus,HostSession,sp,carrierDesc);
                    m_peers[0].m_lanSearch->Release();
                }
                else
                {
                    // we found a session join it
                    EBUS_EVENT_ID_RESULT(m_peers[0].m_session,m_peers[0].m_gridMate,LANSessionServiceBus,JoinSessionBySearchInfo,static_cast<const LANSearchInfo&>(*m_peers[0].m_lanSearch->GetResult(0)),JoinParams(),carrierDesc);
                    m_peers[0].m_lanSearch->Release();
                }
                m_peers[0].m_lanSearch = nullptr;
            }
            else
            {
                LANSessionParams sp;
                sp.m_topology = ST_PEER_TO_PEER;
                sp.m_numPublicSlots = 64;
                sp.m_port = k_hostPort;
                sp.m_numParams = 2;
                sp.m_params[0].m_id = "Param1";
                sp.m_params[0].SetValue(15);
                sp.m_params[1].m_id = "Param2";
                sp.m_params[1].SetValue(25);
                sp.m_flags = LANSessionParams::SF_HOST_MIGRATION_NO_EMPTY_SESSIONS;
                EBUS_EVENT_ID_RESULT(m_peers[k_host].m_session,m_peers[k_host].m_gridMate,LANSessionServiceBus,HostSession,sp,carrierDesc);

                int listenPort = k_hostPort;
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (i == k_host)
                    {
                        continue;
                    }

                    LANSearchParams searchParams;
                    searchParams.m_serverPort = k_hostPort;
                    searchParams.m_listenPort = listenPort == k_hostPort ? 0 : ++listenPort;  // first client will use ephemeral port
                                                                                              // the rest specify return ports
                    searchParams.m_numParams = 1;
                    searchParams.m_params[0].m_id = "Param2";
                    searchParams.m_params[0].SetValue(25);
                    searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                    searchParams.m_familyType = m_driverType;
                    EBUS_EVENT_ID_RESULT(m_peers[i].m_lanSearch,m_peers[i].m_gridMate,LANSessionServiceBus,StartGridSearch,searchParams);
                }
            }

            int maxNumUpdates = 500;
            int numUpdates = 0;
            TimeStamp time = AZStd::chrono::system_clock::now();
            bool sessionParamsTestDone = false;
            while (numUpdates <= maxNumUpdates)
            {
                //////////////////////////////////////////////////////////////////////////
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_peers[i].m_gridMate)
                    {
                        m_peers[i].m_gridMate->Update();
                        if (m_peers[i].m_session)
                        {
                            UpdateReplicaManager(m_peers[i].m_session->GetReplicaMgr());
                        }
                    }
                }
                Update();
                //////////////////////////////////////////////////////////////////////////

                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_peers[i].m_lanSearch && m_peers[i].m_lanSearch->IsDone())
                    {
                        AZ_TEST_ASSERT(m_peers[i].m_lanSearch->GetNumResults() == 1);
                        EBUS_EVENT_ID_RESULT(m_peers[i].m_session,m_peers[i].m_gridMate,LANSessionServiceBus,JoinSessionBySearchInfo,static_cast<const LANSearchInfo&>(*m_peers[i].m_lanSearch->GetResult(0)),JoinParams(),carrierDesc);

                        m_peers[i].m_lanSearch->Release();
                        m_peers[i].m_lanSearch = nullptr;
                    }
                }

#if defined(AZ_LAN_TEST_MAIN_THREAD_BLOCKED)
                if (numUpdates == 200)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::seconds(7));
                }
                int numOfNullSessions = 0;
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_peers[i].session == nullptr)
                    {
                        ++numOfNullSessions;
                    }
                }
                if (numOfNullSessions == k_numMachines)
                {
                    break;
                }
#endif

                if (m_doSessionParamsTest)
                {
                    if (!sessionParamsTestDone)
                    {
                        if (m_peers[k_host].m_connections == k_numMachines - 1)
                        {
                            // Set param1 to 16
                            GridSessionParam param1;
                            param1.m_id = "Param1";
                            param1.SetValue(16);
                            m_peers[k_host].m_session->SetParam(param1);

                            // Remove param2
                            m_peers[k_host].m_session->RemoveParam("Param2");

                            // Add a param
                            GridSessionParam param3;
                            param3.m_id = "Param3";
                            param3.SetValue("val3");
                            m_peers[k_host].m_session->SetParam(param3);

                            sessionParamsTestDone = true;
                        }
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Debug Info
                TimeStamp now = AZStd::chrono::system_clock::now();
                if (AZStd::chrono::milliseconds(now - time).count() > 1000)
                {
                    time = now;
                    for (int i = 0; i < k_numMachines; ++i)
                    {
                        if (m_peers[i].m_session == nullptr)
                        {
                            continue;
                        }

                        if (m_peers[i].m_session->IsHost())
                        {
                            AZ_Printf("GridMate", "------ Host %d ------\n", i);
                        }
                        else
                        {
                            AZ_Printf("GridMate", "------ Client %d ------\n", i);
                        }

                        AZ_Printf("GridMate", "Session %s Members: %d Host: %s Clock: %d\n", m_peers[i].m_session->GetId().c_str(), m_peers[i].m_session->GetNumberOfMembers(), m_peers[i].m_session->IsHost() ? "yes" : "no", m_peers[i].m_session->GetTime());
                        for (unsigned int iMember = 0; iMember < m_peers[i].m_session->GetNumberOfMembers(); ++iMember)
                        {
                            GridMember* member = m_peers[i].m_session->GetMemberByIndex(iMember);
                            AZ_Printf("GridMate", "  Member: %s(%s) Host: %s Local: %s\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no");
                        }
                        AZ_Printf("GridMate", "\n");
                    }
                }
                //////////////////////////////////////////////////////////////////////////

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                numUpdates++;
            }

            // This check only applies to local tests
            if (numMachines > 1)
            {
                for (int i = 0; i < k_numMachines; ++i)
                {
                    AZ_TEST_ASSERT(m_peers[i].m_connections == k_numMachines - 1);

                    if (m_doSessionParamsTest)
                    {
                        unsigned int nParams = m_peers[i].m_session->GetNumParams();
                        AZ_TEST_ASSERT(nParams == 2);
                        bool hasParam3 = false;
                        for (unsigned int iParam = 0; iParam < nParams; ++iParam)
                        {
                            const GridSessionParam& param = m_peers[i].m_session->GetParam(iParam);
                            AZ_TEST_ASSERT(param.m_id != "Param2");
                            if (param.m_id == "Param1")
                            {
                                AZ_TEST_ASSERT(param.m_value == "16");
                            }
                            else if (param.m_id == "Param3")
                            {
                                hasParam3 = true;
                                AZ_TEST_ASSERT(param.m_value == "val3");
                            }
                        }
                        AZ_TEST_ASSERT(hasParam3);
                    }
                }
            }
        }

        static const int k_numMachines = AZ_TRAIT_GRIDMATE_TEST_NUM_MACHINES;
        static const int k_host = 0;
        static const int k_hostPort = 5450;

        TestPeerInfo m_peers[k_numMachines];
        Driver::BSDSocketFamilyType m_driverType;
        bool m_doSessionParamsTest;
    };

    class DISABLED_LANSessionTestIPv6
        : public DISABLED_LANSessionTest
    {
    public:
        DISABLED_LANSessionTestIPv6()
            : DISABLED_LANSessionTest(true) {}
    };

    class DISABLED_LANMultipleSessionTest
        : public GridMateMPTestFixture
        , public SessionEventBus::Handler
    {
        static const int k_numMachines = 3;
        static const int k_host = 0;
        static const int k_numSessions = 2;
        static const int k_hostPort = 5450;
        GridSession* m_sessions[k_numMachines * k_numSessions];
        GridSearch*  m_lanSearch[k_numMachines * k_numSessions];
        IGridMate*   m_gridMates[k_numMachines];
    public:
        // Callback that notifies the title when a game search's query have completed.
        void OnGridSearchComplete(GridSearch* gridSearch) override
        {
            AZ_TEST_ASSERT(gridSearch->IsDone() == true);
        }
        // Callback that notifies the title when a new player joins the game session.
        void OnMemberJoined(GridSession* session, GridMember* member) override
        {
            (void)session;
            (void)member;
        }
        // Callback that notifies the title that a player is leaving the game session. member pointer is NOT valid after the callback returns.
        void OnMemberLeaving(GridSession* session, GridMember* member) override
        {
            (void)session;
            (void)member;
        }
        /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
        void OnMemberKicked(GridSession* session, GridMember* member, AZ::u8 reason) override
        {
            (void)session;
            (void)member;
            (void)reason;
        }
        void OnSessionError(GridSession* session, const AZStd::string& /*errorMsg*/) override
        {
            (void)session;
            AZ_TEST_ASSERT(false);
        }
        // Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
        void OnSessionDelete(GridSession* session) override
        {
            int i;
            for (i = 0; i < k_numMachines; ++i)
            {
                if (m_sessions[i] == session)
                {
                    break;
                }
            }

            AZ_TEST_ASSERT(i < k_numMachines * k_numSessions);
            m_sessions[i] = nullptr;
        }

        DISABLED_LANMultipleSessionTest()
            : GridMateMPTestFixture(200 * 1024 * 1024)
        {
            //////////////////////////////////////////////////////////////////////////
            // Create all grid mates
            m_gridMates[0] = m_gridMate;
            for (int i = 1; i < k_numMachines; ++i)
            {
                GridMateDesc desc;
                m_gridMates[i] = GridMateCreate(desc);
                AZ_TEST_ASSERT(m_gridMates[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            // Hook to session event bus.
            SessionEventBus::Handler::BusConnect(m_gridMate);

            for (int i = 0; i < k_numMachines; ++i)
            {
                // start the multiplayer service (session mgr, extra allocator, etc.)
                StartGridMateService<LANSessionService>(m_gridMates[i], SessionServiceDesc());
                AZ_TEST_ASSERT(LANSessionServiceBus::FindFirstHandler(m_gridMates[i]) != nullptr);
            }
        }

        ~DISABLED_LANMultipleSessionTest() override
        {
            GridMate::StopGridMateService<GridMate::LANSessionService>(m_gridMates[0]);

            for (int i = 1; i < k_numMachines; ++i)
            {
                if (m_gridMates[i])
                {
                    GridMateDestroy(m_gridMates[i]);
                }
            }

            // Unhook from session event bus.
            SessionEventBus::Handler::BusDisconnect();
        }

        void run()
        {
            TestCarrierDesc carrierDesc;
            carrierDesc.m_enableDisconnectDetection = /*false*/ true;
            carrierDesc.m_threadUpdateTimeMS = 10;
            carrierDesc.m_connectionTimeoutMS = 15000;

            memset(m_sessions, 0, sizeof(m_sessions));
            memset(m_lanSearch, 0, sizeof(m_lanSearch));
            for (int iSession = 0; iSession < k_numSessions; ++iSession)
            {
                int hostId = k_host + iSession * k_numMachines;
                int hostPort = k_hostPort + iSession * 20; // space them out so we can easily check the data
                LANSessionParams sp;
                sp.m_topology = ST_PEER_TO_PEER;
                sp.m_numPublicSlots = 64;
                sp.m_port = hostPort;
                sp.m_numParams = 2;
                sp.m_params[0].m_id = "Param1";
                sp.m_params[0].SetValue(15);
                sp.m_params[1].m_id = "Param2";
                sp.m_params[1].SetValue(25);
                sp.m_flags = LANSessionParams::SF_HOST_MIGRATION_NO_EMPTY_SESSIONS;
                EBUS_EVENT_ID_RESULT(m_sessions[hostId],m_gridMates[k_host],LANSessionServiceBus,HostSession,sp,carrierDesc);

                int listenPort = hostPort;
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (i == k_host)
                    {
                        continue;
                    }

                    LANSearchParams searchParams;
                    searchParams.m_serverPort = hostPort;
                    searchParams.m_listenPort = listenPort == hostPort ? 0 : ++listenPort;  // first client will use ephemeral port
                                                                                            // the rest specify return ports
                    searchParams.m_numParams = 1;
                    searchParams.m_params[0].m_id = "Param2";
                    searchParams.m_params[0].SetValue(25);
                    searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                    int fi = i + iSession * k_numMachines;
                    EBUS_EVENT_ID_RESULT(m_lanSearch[fi],m_gridMates[i],LANSessionServiceBus,StartGridSearch,searchParams);
                }
            }

            int maxNumUpdates = 500;
            int numUpdates = 0;
            TimeStamp time = AZStd::chrono::system_clock::now();
            while (numUpdates <= maxNumUpdates)
            {
                //////////////////////////////////////////////////////////////////////////
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_gridMates[i])
                    {
                        m_gridMates[i]->Update();
                    }
                }
                Update();
                //////////////////////////////////////////////////////////////////////////

                for (int iSession = 0; iSession < k_numSessions; ++iSession)
                {
                    for (int iMachine = 0; iMachine < k_numMachines; ++iMachine)
                    {
                        // Update searches
                        int i = iMachine + iSession * k_numMachines;
                        if (m_lanSearch[i] && m_lanSearch[i]->IsDone())
                        {
                            if (m_lanSearch[i]->GetNumResults() == 1)
                            {
                                EBUS_EVENT_ID_RESULT(m_sessions[i],m_gridMates[iMachine],LANSessionServiceBus,JoinSessionBySearchInfo,static_cast<const LANSearchInfo&>(*m_lanSearch[i]->GetResult(0)),JoinParams(),carrierDesc);
                            }

                            m_lanSearch[i]->Release();
                            m_lanSearch[i] = nullptr;
                        }

                        // Update replica managers
                        if (m_sessions[i])
                        {
                            UpdateReplicaManager(m_sessions[i]->GetReplicaMgr());
                        }
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Debug Info
                TimeStamp now = AZStd::chrono::system_clock::now();
                if (AZStd::chrono::milliseconds(now - time).count() > 1000)
                {
                    time = now;
                    for (int iSession = 0; iSession < k_numSessions; ++iSession)
                    {
                        AZ_Printf("GridMate", "------ Session %d ------\n", iSession);

                        for (int iMachine = 0; iMachine < k_numMachines; ++iMachine)
                        {
                            int i = iSession * k_numMachines + iMachine;

                            if (m_sessions[i] == nullptr)
                            {
                                continue;
                            }

                            if (k_host == i)
                            {
                                AZ_Printf("GridMate", " ------ Host %d ------\n", i);
                            }
                            else
                            {
                                AZ_Printf("GridMate", " ------ Client %d ------\n", i);
                            }

                            AZ_Printf("GridMate", " Session %s Members: %d Host: %s Clock: %d\n", m_sessions[i]->GetId().c_str(), m_sessions[i]->GetNumberOfMembers(), m_sessions[i]->IsHost() ? "yes" : "no", m_sessions[i]->GetTime());
                            for (unsigned int iMember = 0; iMember < m_sessions[i]->GetNumberOfMembers(); ++iMember)
                            {
                                GridMember* member = m_sessions[i]->GetMemberByIndex(iMember);
                                AZ_Printf("GridMate", "   Member: %s(%s) Host: %s Local: %s\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no");
                            }
                            AZ_Printf("GridMate", "\n");
                        }
                    }
                }
                //////////////////////////////////////////////////////////////////////////

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                numUpdates++;
            }
        }
    };


    /**
     * Testing session with low latency. This is special mode usually used by tools and communication channels
     * where we try to response instantly on messages.
     */
    class DISABLED_LANLatencySessionTest
        : public GridMateMPTestFixture
        , public SessionEventBus::Handler
    {
        static const int k_numMachines = 2;
        static const int k_host = 0;
        static const int k_hostPort = 5450;
        GridSession*   m_sessions[k_numMachines];
        GridSearch*    m_lanSearch[k_numMachines];
        IGridMate*     m_gridMates[k_numMachines];
    public:
        // Callback that notifies the title when a game search's query have completed.
        void OnGridSearchComplete(GridSearch* gridSearch) override
        {
            AZ_TEST_ASSERT(gridSearch->IsDone() == true);
        }
        // Callback that notifies the title when a new player joins the game session.
        void OnMemberJoined(GridSession* session, GridMember* member) override
        {
            (void)session;
            (void)member;
        }
        // Callback that notifies the title that a player is leaving the game session. member pointer is NOT valid after the callback returns.
        void OnMemberLeaving(GridSession* session, GridMember* member) override
        {
            (void)session;
            (void)member;
        }
        /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
        void OnMemberKicked(GridSession* session, GridMember* member, AZ::u8 reason) override
        {
            (void)session;
            (void)member;
            (void)reason;
        }
        void OnSessionError(GridSession* session, const AZStd::string& /*errorMsg*/) override
        {
            (void)session;
#ifndef AZ_LAN_TEST_MAIN_THREAD_BLOCKED  // we will receive an error is we block for a long time
            AZ_TEST_ASSERT(false);
#endif
        }
        // Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
        void OnSessionDelete(GridSession* session) override
        {
            int i;
            for (i = 0; i < k_numMachines; ++i)
            {
                if (m_sessions[i] == session)
                {
                    break;
                }
            }

            AZ_TEST_ASSERT(i < k_numMachines);
            m_sessions[i] = nullptr;
        }

        DISABLED_LANLatencySessionTest()
#ifdef AZ_TEST_LANLATENCY_ENABLE_MONSTER_BUFFER
            : GridMateMPTestFixture(50 * 1024 * 1024)
#endif
        {
            //////////////////////////////////////////////////////////////////////////
            // Create all grid mates
            m_gridMates[0] = m_gridMate;
            for (int i = 1; i < k_numMachines; ++i)
            {
                GridMateDesc desc;
                m_gridMates[i] = GridMateCreate(desc);
                AZ_TEST_ASSERT(m_gridMates[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            // Hook to session event bus.
            SessionEventBus::Handler::BusConnect(m_gridMate);

            for (int i = 0; i < k_numMachines; ++i)
            {
                // start the multiplayer service (session mgr, extra allocator, etc.)
                StartGridMateService<LANSessionService>(m_gridMates[i], SessionServiceDesc());
                AZ_TEST_ASSERT(LANSessionServiceBus::FindFirstHandler(m_gridMates[i]) != nullptr);
            }
        }

        ~DISABLED_LANLatencySessionTest() override
        {
            StopGridMateService<LANSessionService>(m_gridMates[0]);

            for (int i = 1; i < k_numMachines; ++i)
            {
                if (m_gridMates[i])
                {
                    GridMateDestroy(m_gridMates[i]);
                }
            }

            //StopDrilling();

            // Unhook from session event bus.
            SessionEventBus::Handler::BusDisconnect();
        }

        void run()
        {
            TestCarrierDesc carrierDesc;
            carrierDesc.m_enableDisconnectDetection = true;
            carrierDesc.m_threadUpdateTimeMS = 10;
            carrierDesc.m_threadInstantResponse = true;  // enable low latency mode
            carrierDesc.m_driverIsFullPackets = true;  // test sending 64k packets (this is useful for LAN only)
            //carrierDesc.m_driverSendBufferSize = 1 * 1024 * 1024;
            carrierDesc.m_driverReceiveBufferSize = 1 * 1024 * 1024;

            //StartDrilling("lansession");

            memset(m_sessions, 0, sizeof(m_sessions));
            LANSessionParams sp;
            sp.m_topology = ST_PEER_TO_PEER;
            sp.m_numPublicSlots = 64;
            sp.m_port = k_hostPort;
            sp.m_numParams = 2;
            sp.m_params[0].m_id = "Param1";
            sp.m_params[0].SetValue(15);
            sp.m_params[1].m_id = "Param2";
            sp.m_params[1].SetValue(25);
            sp.m_flags = LANSessionParams::SF_HOST_MIGRATION_NO_EMPTY_SESSIONS;
            EBUS_EVENT_ID_RESULT(m_sessions[k_host],m_gridMates[k_host],LANSessionServiceBus,HostSession,sp,carrierDesc);

            memset(m_lanSearch, 0, sizeof(m_lanSearch));
            int listenPort = k_hostPort;
            for (int i = 0; i < k_numMachines; ++i)
            {
                if (i == k_host)
                {
                    continue;
                }

                LANSearchParams searchParams;
                searchParams.m_serverPort = k_hostPort;
                searchParams.m_listenPort = listenPort == k_hostPort ? 0 : ++listenPort;    // first client will use ephemeral port
                                                                                            // the rest specify return ports
                searchParams.m_numParams = 1;
                searchParams.m_params[0].m_id = "Param2";
                searchParams.m_params[0].SetValue(25);
                searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                EBUS_EVENT_ID_RESULT(m_lanSearch[i],m_gridMates[i],LANSessionServiceBus,StartGridSearch,searchParams);
            }

            int maxNumUpdates = 500 /*25000*/;
            int numUpdates = 0;
            TimeStamp time = AZStd::chrono::system_clock::now();
#ifdef AZ_TEST_LANLATENCY_ENABLE_MONSTER_BUFFER
            const int monsterBufferSize = /*10*/ 1 * 1024 * 1024;
            char* monsterBufferSend = (char*)azmalloc(monsterBufferSize);
            char* value = monsterBufferSend;
            for (int i = 0; i < monsterBufferSize; ++i, ++value)
            {
                *value = (char)i;
            }
            char* monsterBufferReceive = (char*)azmalloc(monsterBufferSize);
            int numLastSend = 0;
#endif
            int numMStoSleep = /*30*/ 16;
            while (numUpdates <= maxNumUpdates)
            {
                //////////////////////////////////////////////////////////////////////////
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_gridMates[i])
                    {
                        m_gridMates[i]->Update();

                        if (m_sessions[i])
                        {
                            UpdateReplicaManager(m_sessions[i]->GetReplicaMgr());
                        }
                    }
                }
                Update();
                //////////////////////////////////////////////////////////////////////////

                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_lanSearch[i] && m_lanSearch[i]->IsDone())
                    {
                        if (m_lanSearch[i]->GetNumResults() == 1)
                        {
                            EBUS_EVENT_ID_RESULT(m_sessions[i],m_gridMates[i],LANSessionServiceBus,JoinSessionBySearchInfo,static_cast<const LANSearchInfo&>(*m_lanSearch[i]->GetResult(0)),JoinParams(),carrierDesc);
                        }

                        m_lanSearch[i]->Release();
                        m_lanSearch[i] = nullptr;
                    }
                }

                if (numUpdates >= 150)
                {
#ifdef AZ_TEST_LANLATENCY_ENABLE_MONSTER_BUFFER
                    if (numUpdates % /*150*/ 50 == /*149*/ 49)
                    {
                        if (m_sessions[0] != nullptr)
                        {
                            numLastSend = numUpdates;
                            for (unsigned int iMember = 0; iMember < m_sessions[0]->GetNumberOfMembers(); ++iMember)
                            {
                                GridMember* member = m_sessions[0]->GetMemberByIndex(iMember);
                                if (!member->IsLocal())
                                {
                                    member->SendBinary(monsterBufferSend, monsterBufferSize);
                                }
                            }
                        }
                    }
                    else
                    {
                        if (m_sessions[1] != nullptr)
                        {
                            for (unsigned int iMember = 0; iMember < m_sessions[1]->GetNumberOfMembers(); ++iMember)
                            {
                                GridMember* member = m_sessions[1]->GetMemberByIndex(iMember);
                                if (!member->IsLocal())
                                {
                                    Carrier::ReceiveResult result = member->ReceiveBinary(monsterBufferReceive, monsterBufferSize);
                                    if (result.m_state == Carrier::ReceiveResult::RECEIVED)
                                    {
                                        AZ_TEST_ASSERT(memcmp(monsterBufferReceive, monsterBufferSend, monsterBufferSize) == 0);
                                        memset(monsterBufferReceive, 0, monsterBufferSize);
                                        AZ_Printf("GridMate", "Monster buffer process time ~%d ms\n", (numUpdates - numLastSend) * numMStoSleep);
                                    }
                                }
                            }
                        }
                    }
#else // !AZ_TEST_LANLATENCY_ENABLE_MONSTER_BUFFER
                    const char* session0SendData = "Hello";
                    const char* session1SendData = "Bye";
                    char dataBuffer[64];
                    if (sessions[0] != nullptr)
                    {
                        for (unsigned int iMember = 0; iMember < sessions[0]->GetNumberOfMembers(); ++iMember)
                        {
                            GridMember* member = sessions[0]->GetMember(iMember);
                            if (!member->IsLocal())
                            {
                                member->SendBinary(session0SendData, static_cast<unsigned int>(strlen(session0SendData)));
                                Carrier::ReceiveResult receiveResult = member->ReceiveBinary(dataBuffer, AZ_ARRAY_SIZE(dataBuffer));
                                if (result.m_state == Carrier::ReceiveResult::RECEIVED)
                                {
                                    AZ_TEST_ASSERT(strncmp(session1SendData, dataBuffer, strlen(session1SendData)) == 0);
                                }
                            }
                        }
                    }
                    if (sessions[1] != nullptr)
                    {
                        for (unsigned int iMember = 0; iMember < sessions[1]->GetNumberOfMembers(); ++iMember)
                        {
                            GridMember* member = sessions[1]->GetMember(iMember);
                            if (!member->IsLocal())
                            {
                                member->SendBinary(session1SendData, static_cast<unsigned int>(strlen(session1SendData)));
                                Carrier::ReceiveResult receiveResult = member->ReceiveBinary(dataBuffer, AZ_ARRAY_SIZE(dataBuffer));
                                if (result.m_state == Carrier::ReceiveResult::RECEIVED)
                                {
                                    AZ_TEST_ASSERT(strncmp(session0SendData, dataBuffer, static_cast<unsigned int>(strlen(session0SendData))) == 0);
                                }
                            }
                        }
                    }
#endif // AZ_TEST_LANLATENCY_ENABLE_MONSTER_BUFFER
                }

                //////////////////////////////////////////////////////////////////////////
                // Debug Info
                TimeStamp now = AZStd::chrono::system_clock::now();
                if (AZStd::chrono::milliseconds(now - time).count() > 1000)
                {
                    time = now;
                    for (int i = 0; i < k_numMachines; ++i)
                    {
                        if (m_sessions[i] == nullptr)
                        {
                            continue;
                        }

                        if (k_host == i)
                        {
                            AZ_Printf("GridMate", "------ Host %d ------\n", i);
                        }
                        else
                        {
                            AZ_Printf("GridMate", "------ Client %d ------\n", i);
                        }

                        AZ_Printf("GridMate", "Session %s Members: %d Host: %s Clock: %d\n", m_sessions[i]->GetId().c_str(), m_sessions[i]->GetNumberOfMembers(), m_sessions[i]->IsHost() ? "yes" : "no", m_sessions[i]->GetTime());
                        for (unsigned int iMember = 0; iMember < m_sessions[i]->GetNumberOfMembers(); ++iMember)
                        {
                            GridMember* member = m_sessions[i]->GetMemberByIndex(iMember);

                            // check statistics
                            if (member->IsLocal())
                            {
                                AZ_Printf("GridMate", "  Member: %s(%s) Host: %s Local: %s\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no");
                            }
                            else
                            {
                                Carrier* carrier = m_sessions[i]->GetCarrier();
                                ConnectionID connId = member->GetConnectionId();
                                Carrier::Statistics statsLifeTime, statsLastSecond;
                                Carrier::Statistics effectiveStatsLifeTime, effectiveStatsLastSecond;
                                statsLifeTime.m_rtt = 9999.99f;
                                statsLifeTime.m_dataReceived = statsLifeTime.m_dataSend = 0;
                                statsLifeTime.m_packetLost = 0;
                                effectiveStatsLifeTime.m_rtt = 9999.99f;
                                effectiveStatsLifeTime.m_dataReceived = effectiveStatsLifeTime.m_dataSend = 0;
                                effectiveStatsLifeTime.m_packetLost = 0;
                                if (connId != InvalidConnectionID)
                                {
                                    /// Check the effective data
                                    carrier->QueryStatistics(connId, &statsLastSecond, &statsLifeTime, &effectiveStatsLastSecond, &effectiveStatsLifeTime);

                                    //statsLifeTime.rtt = (statsLifeTime.rtt + statsLastSecond.rtt) * .5f;
                                    statsLifeTime.m_packetSend += statsLastSecond.m_packetSend;
                                    statsLifeTime.m_dataSend += statsLastSecond.m_dataSend;

                                    //effectiveStatsLifeTime.rtt = (effectiveStatsLifeTime.rtt + effectiveStatsLastSecond.rtt) * .5f;
                                    effectiveStatsLifeTime.m_packetSend += effectiveStatsLastSecond.m_packetSend;
                                    effectiveStatsLifeTime.m_dataSend += effectiveStatsLastSecond.m_dataSend;

                                    // the first spike (while we allocate is more otherwise it seems correct)
                                    //AZ_TEST_ASSERT(effectiveStatsLifeTime.rtt<=10.0f); // Check if our low latency mode is actually low latency
                                }

                                AZ_Printf("GridMate", "  Member: %s(%s) Host: %s Local: %s Rtt:%.2f Send:%u Received:%u Loss:%d eRtt:%.2f eSend:%u eReceived:%u eLoss:%d\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no", statsLifeTime.m_rtt, statsLifeTime.m_dataSend, statsLifeTime.m_dataReceived, statsLifeTime.m_packetLost, effectiveStatsLifeTime.m_rtt, effectiveStatsLifeTime.m_dataSend, effectiveStatsLifeTime.m_dataReceived, effectiveStatsLifeTime.m_packetLost);
                            }
                        }

                        AZ_Printf("GridMate", "\n");
                    }
                }
                //////////////////////////////////////////////////////////////////////////

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(numMStoSleep));
                numUpdates++;
            }

#ifdef AZ_TEST_LANLATENCY_ENABLE_MONSTER_BUFFER
            azfree(monsterBufferSend);
            azfree(monsterBufferReceive);
#endif //
        }
    };


    /**
     * Simulating most common host migration scenarios.
     * 1. We start a session with 3 members.
     * 2. We drop the host (by blocking it's connection). (we are left with 2 members after migration)
     * 3. After host migration has completed we add additional 3 members. (after join we have 5 members)
     * 4. We drop the new host. (after migration we have 4 members)
     * 5. After host migration we drop the new host again. (after migration we have 3 members).
     * Session should be fully operational at the end with 3 members left.
     */
    class LANSessionMigarationTestTest
        : public SessionEventBus::Handler
        , public GridMateMPTestFixture
    {
        static const int k_numInitialMembers = 3;
        static const int k_numSecondMembers = 3;
        static const int k_numMachines = k_numInitialMembers + k_numSecondMembers;

        static const int k_hostPort = 5450;

        GridSession* m_sessions[k_numMachines];
        GridSearch*  m_lanSearch[k_numMachines];
        IGridMate*   m_gridMates[k_numMachines];
        DefaultSimulator m_simulators[k_numMachines];

        int m_host;
        int m_numUpdates;
    public:
        // Callback that notifies the title when a game search's query have completed.
        void OnGridSearchComplete(GridSearch* gridSearch) override
        {
            AZ_TEST_ASSERT(gridSearch->IsDone() == true);
        }
        // Callback that notifies the title when a new player joins the game session.
        void OnMemberJoined(GridSession* session, GridMember* member) override
        {
            (void)session;
            (void)member;
        }
        // Callback that notifies the title that a player is leaving the game session. member pointer is NOT valid after the callback returns.
        void OnMemberLeaving(GridSession* session, GridMember* member) override
        {
            (void)member;
            if (session->GetNumberOfMembers() == 2) // if the last member (not us) is leaving kill the session!
            {
                session->Leave(false);
            }
        }
        /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
        void OnMemberKicked(GridSession* session, GridMember* member, AZ::u8 reason) override
        {
            (void)session;
            (void)member;
            (void)reason;
        }
        void OnSessionError(GridSession* session, const AZStd::string& /*errorMsg*/) override
        {
            (void)session;
            AZ_TEST_ASSERT(false);
        }
        // Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
        void OnSessionDelete(GridSession* session) override
        {
            int i;
            for (i = 0; i < k_numMachines; ++i)
            {
                if (m_sessions[i] == session)
                {
                    break;
                }
            }

            AZ_TEST_ASSERT(i < k_numMachines);
            m_sessions[i] = nullptr;
        }

        void OnMigrationStart(GridSession* session) override
        {
            (void)session;
            AZ_TracePrintf("GridMate", "Migration start on %s at frame %d\n", session->GetMyMember()->GetId().ToAddress().c_str(), m_numUpdates);
        }
        void OnMigrationElectHost(GridSession* session, GridMember*& newHost) override
        {
            (void)session;
            (void)newHost;
            AZ_TracePrintf("GridMate", "Migration elect host on %s at frame %d\n", session->GetMyMember()->GetId().ToAddress().c_str(), m_numUpdates);
        }
        void OnMigrationEnd(GridSession* session, GridMember* newHost) override
        {
            (void)session;
            (void)newHost;
            AZ_TracePrintf("GridMate", "Migration end on %s, new host %s at frame %d\n", session->GetMyMember()->GetId().ToAddress().c_str(), session->GetHost()->GetId().ToAddress().c_str(), m_numUpdates);
            for (int i = 0; i < k_numMachines; ++i)
            {
                if (m_sessions[i] == session)
                {
                    if (session->GetMyMember() == newHost)
                    {
                        m_host = i;
                        return;
                    }
                }
            }
        }

        LANSessionMigarationTestTest()
        {
            //////////////////////////////////////////////////////////////////////////
            // Create all grid mates
            m_gridMates[0] = m_gridMate;
            for (int i = 1; i < k_numMachines; ++i)
            {
                GridMateDesc desc;
                m_gridMates[i] = GridMateCreate(desc);
                AZ_TEST_ASSERT(m_gridMates[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            // Hook to session event bus.
            SessionEventBus::Handler::BusConnect(m_gridMate);

            for (int i = 0; i < k_numMachines; ++i)
            {
                // start the multiplayer service (session mgr, extra allocator, etc.)
                StartGridMateService<LANSessionService>(m_gridMates[i], SessionServiceDesc());
                AZ_TEST_ASSERT(LANSessionServiceBus::FindFirstHandler(m_gridMates[i]) != nullptr);
            }

            //StartDrilling("lanmigration");
        }

        ~LANSessionMigarationTestTest() override
        {
            StopGridMateService<LANSessionService>(m_gridMates[0]);

            for (int i = 1; i < k_numMachines; ++i)
            {
                if (m_gridMates[i])
                {
                    GridMateDestroy(m_gridMates[i]);
                }
            }

            // Unhook from session event bus.
            SessionEventBus::Handler::BusDisconnect();

            //StopDrilling();
        }

        void run()
        {
            TestCarrierDesc carrierDesc;
            carrierDesc.m_enableDisconnectDetection = /*false*/ true;
            carrierDesc.m_threadUpdateTimeMS = 10;
            carrierDesc.m_simulator = &m_simulators[0];

            m_host = 1; // first we set the second session/machine to be the host (so we can test non zero host)

            memset(m_sessions, 0, sizeof(m_sessions));
            LANSessionParams sp;
            sp.m_topology = ST_PEER_TO_PEER;
            sp.m_numPublicSlots = 64;
            sp.m_port = k_hostPort;
            sp.m_numParams = 2;
            sp.m_params[0].m_id = "Param1";
            sp.m_params[0].SetValue(15);
            sp.m_params[1].m_id = "Param2";
            sp.m_params[1].SetValue(25);
            EBUS_EVENT_ID_RESULT(m_sessions[m_host],m_gridMates[m_host],LANSessionServiceBus,HostSession,sp,carrierDesc);

            memset(m_lanSearch, 0, sizeof(m_lanSearch));
            int listenPort = k_hostPort;
            int numSessionsUsed = 0;
            // 1. We start a session with 3 members. (1 host 2 joins)
            for (; numSessionsUsed < k_numInitialMembers; ++numSessionsUsed)
            {
                if (numSessionsUsed == m_host)
                {
                    continue;
                }

                LANSearchParams searchParams;
                searchParams.m_serverPort = k_hostPort;
                searchParams.m_listenPort = listenPort == k_hostPort ? 0 : ++listenPort;    // first client will use ephemeral port
                                                                                            // the rest specify return ports
                searchParams.m_numParams = 1;
                searchParams.m_params[0].m_id = "Param2";
                searchParams.m_params[0].SetValue(25);
                searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                EBUS_EVENT_ID_RESULT(m_lanSearch[numSessionsUsed],m_gridMates[numSessionsUsed],LANSessionServiceBus,StartGridSearch,searchParams);
            }

            int maxNumUpdates = 1000;
            m_numUpdates = 0;
            TimeStamp time = AZStd::chrono::system_clock::now();
            while (m_numUpdates <= maxNumUpdates)
            {
                //////////////////////////////////////////////////////////////////////////
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_gridMates[i])
                    {
                        m_gridMates[i]->Update();

                        if (m_sessions[i])
                        {
                            UpdateReplicaManager(m_sessions[i]->GetReplicaMgr());
                        }
                    }
                }
                Update();
                //////////////////////////////////////////////////////////////////////////

                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_lanSearch[i] && m_lanSearch[i]->IsDone())
                    {
                        if (m_lanSearch[i]->GetNumResults() == 1)
                        {
                            carrierDesc.m_simulator = &m_simulators[i];
                            EBUS_EVENT_ID_RESULT(m_sessions[i],m_gridMates[i],LANSessionServiceBus,JoinSessionBySearchInfo,static_cast<const LANSearchInfo&>(*m_lanSearch[i]->GetResult(0)),JoinParams(),carrierDesc);
                        }

                        m_lanSearch[i]->Release();
                        m_lanSearch[i] = nullptr;
                    }
                }

                // 2. We drop the host (by blocking it's connection). (we are left with 2 members after migration)
                if (m_numUpdates == 150)
                {
                    // Block 100% the host (simulate connection drop)
                    m_simulators[m_host].SetOutgoingPacketLoss(1, 1);
                    m_simulators[m_host].SetIncomingPacketLoss(1, 1);
                    m_simulators[m_host].Enable();
                }

                // 3. After host migration has completed we add additional 3 members. (after join we have 5 members)
                if (m_numUpdates == 400)
                {
                    for (; numSessionsUsed < k_numMachines; ++numSessionsUsed)
                    {
                        //if(numSessionsUsed==host) continue;

                        LANSearchParams searchParams;
                        searchParams.m_serverPort = k_hostPort;
                        searchParams.m_listenPort = listenPort++;
                        searchParams.m_numParams = 1;
                        searchParams.m_params[0].m_id = "Param2";
                        searchParams.m_params[0].SetValue(25);
                        searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                        EBUS_EVENT_ID_RESULT(m_lanSearch[numSessionsUsed],m_gridMates[numSessionsUsed],LANSessionServiceBus,StartGridSearch,searchParams);
                    }
                }

                // 4. We drop the new host. (after migration we have 4 members)
                if (m_numUpdates == 600)
                {
                    // Block 100% the host (simulate connection drop)
                    m_simulators[m_host].SetOutgoingPacketLoss(1, 1);
                    m_simulators[m_host].SetIncomingPacketLoss(1, 1);
                    m_simulators[m_host].Enable();
                }

                // 5. After host migration we drop the new host again. (after migration we have 3 members).
                if (m_numUpdates == 800)
                {
                    // Block 100% the host (simulate connection drop)
                    m_simulators[m_host].SetOutgoingPacketLoss(1, 1);
                    m_simulators[m_host].SetIncomingPacketLoss(1, 1);
                    m_simulators[m_host].Enable();
                }

                //////////////////////////////////////////////////////////////////////////
                // Debug Info
                TimeStamp now = AZStd::chrono::system_clock::now();
                if (AZStd::chrono::milliseconds(now - time).count() > 1000)
                {
                    time = now;
                    for (int i = 0; i < k_numMachines; ++i)
                    {
                        if (m_sessions[i] == nullptr)
                        {
                            continue;
                        }

                        if (m_host == i)
                        {
                            AZ_Printf("GridMate", "------ Host %d ------\n", i);
                        }
                        else
                        {
                            AZ_Printf("GridMate", "------ Client %d ------\n", i);
                        }

                        AZ_Printf("GridMate", "Session %s Members: %d Host: %s Clock: %d\n", m_sessions[i]->GetId().c_str(), m_sessions[i]->GetNumberOfMembers(), m_sessions[i]->IsHost() ? "yes" : "no", m_sessions[i]->GetTime());
                        for (unsigned int iMember = 0; iMember < m_sessions[i]->GetNumberOfMembers(); ++iMember)
                        {
                            GridMember* member = m_sessions[i]->GetMemberByIndex(iMember);
                            AZ_Printf("GridMate", "  Member: %s(%s) Host: %s Local: %s\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no");
                        }
                        AZ_Printf("GridMate", "\n");
                    }
                }
                //////////////////////////////////////////////////////////////////////////

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                m_numUpdates++;
            }
        }
    };
    /**
     * Simulating less common conditions (peer drops during host migration).
     * 1. We start a session with 5 members.
     * 2. We drop the host.
     * 3. Shortly after the host (before host election has completed) we drop the next
     * best host candidate (that everybody vote for) - this should trigger a re vote for the next it line.
     * 4. Shortly after (while still in host election) we drop the next best host candidate,
     * which should trigger another re vote.
     * 4.1. The old host and the 2 dropped peers, became the hosts of their own sessions (in this test just remove all of those sessions)
     * 4.2. After host migration has completed we have 2 members and in the session left.
     * 5. We join a 2 new members to the session.
     * Session should be fully operational at the end with 4 members in it.
     */
    class LANSessionMigarationTestTest2
        : public SessionEventBus::Handler
        , public GridMateMPTestFixture
    {
        static const int k_numInitialMembers = 5;
        static const int k_numSecondMembers = 2;
        static const int k_numMachines = k_numInitialMembers + k_numSecondMembers;

        static const int k_hostPort = 5450;

        GridSession* m_sessions[k_numMachines];
        GridSearch*  m_lanSearch[k_numMachines];
        IGridMate*   m_gridMates[k_numMachines];
        DefaultSimulator m_simulators[k_numMachines];

        int m_host;
        int m_numUpdates;
    public:
        // Callback that notifies the title when a game search's query have completed.
        void OnGridSearchComplete(GridSearch* gridSearch) override
        {
            AZ_TEST_ASSERT(gridSearch->IsDone() == true);
        }
        // Callback that notifies the title when a new player joins the game session.
        void OnMemberJoined(GridSession* session, GridMember* member) override
        {
            (void)session;
            (void)member;
        }
        // Callback that notifies the title that a player is leaving the game session. member pointer is NOT valid after the callback returns.
        void OnMemberLeaving(GridSession* session, GridMember* member) override
        {
            (void)member;
            if (session->GetNumberOfMembers() == 2) // if the last member (not us) is leaving kill the session!
            {
                session->Leave(false);
            }
        }
        /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
        void OnMemberKicked(GridSession* session, GridMember* member, AZ::u8 reason) override
        {
            (void)session;
            (void)member;
            (void)reason;
        }
        void OnSessionError(GridSession* session, const AZStd::string& /*errorMsg*/) override
        {
            (void)session;
            // On this test we will get a open port error because we have multiple hosts. This is ok, since we test migration here!
            //AZ_TEST_ASSERT(false);
        }
        // Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
        void OnSessionDelete(GridSession* session) override
        {
            int i;
            for (i = 0; i < k_numMachines; ++i)
            {
                if (m_sessions[i] == session)
                {
                    break;
                }
            }

            AZ_TEST_ASSERT(i < k_numMachines);
            m_sessions[i] = nullptr;
        }

        void OnMigrationStart(GridSession* session) override
        {
            (void)session;
            AZ_TracePrintf("GridMate", "Migration start on %s at frame %d\n", session->GetMyMember()->GetId().ToAddress().c_str(), m_numUpdates);
        }
        void OnMigrationElectHost(GridSession* session, GridMember*& newHost) override
        {
            (void)session;
            (void)newHost;
            AZ_TracePrintf("GridMate", "Migration elect host on %s at frame %d\n", session->GetMyMember()->GetId().ToAddress().c_str(), m_numUpdates);
        }
        void OnMigrationEnd(GridSession* session, GridMember* newHost) override
        {
            (void)session;
            (void)newHost;
            AZ_TracePrintf("GridMate", "Migration end on %s, new host %s at frame %d\n", session->GetMyMember()->GetId().ToAddress().c_str(), session->GetHost()->GetId().ToAddress().c_str(), m_numUpdates);
            for (int i = 0; i < k_numMachines; ++i)
            {
                if (m_sessions[i] == session)
                {
                    if (session->GetMyMember() == newHost)
                    {
                        m_host = i;
                        return;
                    }
                }
            }
        }
        LANSessionMigarationTestTest2()
        {
            //////////////////////////////////////////////////////////////////////////
            // Create all grid mates
            m_gridMates[0] = m_gridMate;
            for (int i = 1; i < k_numMachines; ++i)
            {
                GridMateDesc desc;
                m_gridMates[i] = GridMateCreate(desc);
                AZ_TEST_ASSERT(m_gridMates[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            // Hook to session event bus.
            SessionEventBus::Handler::BusConnect(m_gridMate);

            // 1. We start a session with 5 members. (1 host 4 peers)
            for (int i = 0; i < k_numMachines; ++i)
            {
                // start the multiplayer service (session mgr, extra allocator, etc.)
                StartGridMateService<LANSessionService>(m_gridMates[i], SessionServiceDesc());
                AZ_TEST_ASSERT(LANSessionServiceBus::FindFirstHandler(m_gridMates[i]) != nullptr);
            }

            //StartDrilling("lanmigration2");
        }
        ~LANSessionMigarationTestTest2() override
        {
            StopGridMateService<LANSessionService>(m_gridMates[0]);

            for (int i = 1; i < k_numMachines; ++i)
            {
                if (m_gridMates[i])
                {
                    GridMateDestroy(m_gridMates[i]);
                }
            }

            // Unhook from session event bus.
            SessionEventBus::Handler::BusDisconnect();

            //StopDrilling();
        }
        void run()
        {
            TestCarrierDesc carrierDesc;
            carrierDesc.m_enableDisconnectDetection = /*false*/ true;
            carrierDesc.m_threadUpdateTimeMS = 10;
            carrierDesc.m_simulator = &m_simulators[0];

            m_host = 1; // first we set the second session/machine to be the host (so we can test non zero host)

            memset(m_sessions, 0, sizeof(m_sessions));
            LANSessionParams sp;
            sp.m_topology = ST_PEER_TO_PEER;
            sp.m_numPublicSlots = 64;
            sp.m_port = k_hostPort;
            sp.m_numParams = 2;
            sp.m_params[0].m_id = "Param1";
            sp.m_params[0].SetValue(15);
            sp.m_params[1].m_id = "Param2";
            sp.m_params[1].SetValue(25);
            EBUS_EVENT_ID_RESULT(m_sessions[m_host],m_gridMates[m_host],LANSessionServiceBus,HostSession,sp,carrierDesc);

            memset(m_lanSearch, 0, sizeof(m_lanSearch));
            int listenPort = k_hostPort;
            int numSessionsUsed = 0;
            for (; numSessionsUsed < k_numInitialMembers; ++numSessionsUsed)
            {
                if (numSessionsUsed == m_host)
                {
                    continue;
                }

                LANSearchParams searchParams;
                searchParams.m_serverPort = k_hostPort;
                searchParams.m_listenPort = listenPort == k_hostPort ? 0 : ++listenPort;    // first client will use ephemeral port
                                                                                            // the rest specify return ports
                searchParams.m_numParams = 1;
                searchParams.m_params[0].m_id = "Param2";
                searchParams.m_params[0].SetValue(25);
                searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                EBUS_EVENT_ID_RESULT(m_lanSearch[numSessionsUsed],m_gridMates[numSessionsUsed],LANSessionServiceBus,StartGridSearch,searchParams);
            }

            int maxNumUpdates = 800;
            m_numUpdates = 0;
            TimeStamp time = AZStd::chrono::system_clock::now();
            while (m_numUpdates <= maxNumUpdates)
            {
                //////////////////////////////////////////////////////////////////////////
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_gridMates[i])
                    {
                        m_gridMates[i]->Update();

                        if (m_sessions[i])
                        {
                            UpdateReplicaManager(m_sessions[i]->GetReplicaMgr());
                        }
                    }
                }
                Update();
                //////////////////////////////////////////////////////////////////////////

                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_lanSearch[i] && m_lanSearch[i]->IsDone())
                    {
                        if (m_lanSearch[i]->GetNumResults() == 1)
                        {
                            carrierDesc.m_simulator = &m_simulators[i];
                            EBUS_EVENT_ID_RESULT(m_sessions[i],m_gridMates[i],LANSessionServiceBus,JoinSessionBySearchInfo,static_cast<const LANSearchInfo&>(*m_lanSearch[i]->GetResult(0)),JoinParams(),carrierDesc);
                        }

                        m_lanSearch[i]->Release();
                        m_lanSearch[i] = nullptr;
                    }
                }

                // 2. We drop the host.
                if (m_numUpdates == 150)
                {
                    // Block 100% the host (simulate connection drop)
                    m_simulators[m_host].SetOutgoingPacketLoss(1, 1);
                    m_simulators[m_host].SetIncomingPacketLoss(1, 1);
                    m_simulators[m_host].Enable();
                }

                // 3. Shortly after the host (before host election has completed) we drop the next
                // best host candidate (that everybody vote for) - this should trigger a re vote for the next it line.
                if (m_numUpdates == 155)
                {
                    GridMember* memberToDisconnect = m_sessions[m_host]->GetMemberByIndex(1);
                    int memberSession = -1;
                    for (int i = 0; i < k_numMachines; ++i)
                    {
                        if (m_sessions[i])
                        {
                            if (m_sessions[i]->GetMyMember()->GetId() == memberToDisconnect->GetId())
                            {
                                memberSession = i;
                                // Block 100% the host (simulate connection drop)
                                m_simulators[memberSession].SetOutgoingPacketLoss(1, 1);
                                m_simulators[memberSession].SetIncomingPacketLoss(1, 1);
                                m_simulators[memberSession].Enable();
                                break;
                            }
                        }
                    }
                    AZ_TEST_ASSERT(memberSession != -1);
                }

                // 4. Shortly after (while still in host election) we drop the next best host candidate,
                // which should trigger another re vote.
                if (m_numUpdates == 160)
                {
                    GridMember* memberToDisconnect = m_sessions[m_host]->GetMemberByIndex(2);
                    int memberSession = -1;
                    for (int i = 0; i < k_numMachines; ++i)
                    {
                        if (m_sessions[i])
                        {
                            if (m_sessions[i]->GetMyMember()->GetId() == memberToDisconnect->GetId())
                            {
                                memberSession = i;
                                // Block 100% the host (simulate connection drop)
                                m_simulators[memberSession].SetOutgoingPacketLoss(1, 1);
                                m_simulators[memberSession].SetIncomingPacketLoss(1, 1);
                                m_simulators[memberSession].Enable();
                                break;
                            }
                        }
                    }
                    AZ_TEST_ASSERT(memberSession != -1);
                }

                // 5. We join a 2 new members to the session.
                if (m_numUpdates == 600)
                {
                    for (; numSessionsUsed < k_numMachines; ++numSessionsUsed)
                    {
                        //if(numSessionsUsed==host) continue;

                        LANSearchParams searchParams;
                        searchParams.m_serverPort = k_hostPort;
                        searchParams.m_listenPort = listenPort++;
                        searchParams.m_numParams = 1;
                        searchParams.m_params[0].m_id = "Param2";
                        searchParams.m_params[0].SetValue(25);
                        searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                        EBUS_EVENT_ID_RESULT(m_lanSearch[numSessionsUsed],m_gridMates[numSessionsUsed],LANSessionServiceBus,StartGridSearch,searchParams);
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Debug Info
                TimeStamp now = AZStd::chrono::system_clock::now();
                if (AZStd::chrono::milliseconds(now - time).count() > 1000)
                {
                    time = now;
                    for (int i = 0; i < k_numMachines; ++i)
                    {
                        if (m_sessions[i] == nullptr)
                        {
                            continue;
                        }

                        if (m_host == i)
                        {
                            AZ_Printf("GridMate", "------ Host %d ------\n", i);
                        }
                        else
                        {
                            AZ_Printf("GridMate", "------ Client %d ------\n", i);
                        }

                        AZ_Printf("GridMate", "Session %s Members: %d Host: %s Clock: %d\n", m_sessions[i]->GetId().c_str(), m_sessions[i]->GetNumberOfMembers(), m_sessions[i]->IsHost() ? "yes" : "no", m_sessions[i]->GetTime());
                        for (unsigned int iMember = 0; iMember < m_sessions[i]->GetNumberOfMembers(); ++iMember)
                        {
                            GridMember* member = m_sessions[i]->GetMemberByIndex(iMember);
                            AZ_Printf("GridMate", "  Member: %s(%s) Host: %s Local: %s\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no");
                        }
                        AZ_Printf("GridMate", "\n");
                    }
                }
                //////////////////////////////////////////////////////////////////////////

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                m_numUpdates++;
            }
        }
    };

    /**
     * Simulating less common conditions (make one peer detect host disconnection and announce
     * itself as the new host so quickly that other peer(s) reject the host migration).
     * 1. We start a session with 3 members.
     * 2. Terminate connection on next host candidate to the host.
     * 2.1. Shortly after client 2 should receive new host announcement and reject the migration (disconnect from alleged new host)
     * 2.2. New host terminates because nobody else followed it.
     * 3. Add 2 new joins to the original session.
     * Original session should remain fully operational with 4 members in it.
     */
    class LANSessionMigarationTestTest3
        : public SessionEventBus::Handler
        , public GridMateMPTestFixture
    {
        static const int k_numInitialMembers = 3;
        static const int k_numSecondMembers = 2;
        static const int k_numMachines = k_numInitialMembers + k_numSecondMembers;

        static const int k_hostPort = 5450;

        GridSession* m_sessions[k_numMachines];
        GridSearch*  m_lanSearch[k_numMachines];
        IGridMate*   m_gridMates[k_numMachines];
        DefaultSimulator m_simulators[k_numMachines];

        int m_host;
        int m_numUpdates;
    public:
        // Callback that notifies the title when a game search's query have completed.
        void OnGridSearchComplete(GridSearch* gridSearch) override
        {
            AZ_TEST_ASSERT(gridSearch->IsDone() == true);
        }
        // Callback that notifies the title when a new player joins the game session.
        void OnMemberJoined(GridSession* session, GridMember* member) override
        {
            (void)session;
            (void)member;
        }
        // Callback that notifies the title that a player is leaving the game session. member pointer is NOT valid after the callback returns.
        void OnMemberLeaving(GridSession* session, GridMember* member) override
        {
            (void)member;
            if (session->GetNumberOfMembers() == 2) // if the last member (not us) is leaving kill the session!
            {
                session->Leave(false);
            }
        }
        /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
        void OnMemberKicked(GridSession* session, GridMember* member, AZ::u8 reason) override
        {
            (void)session;
            (void)member;
            (void)reason;
        }
        void OnSessionError(GridSession* session, const AZStd::string& /*errorMsg*/) override
        {
            (void)session;
            // On this test we will get a open port error because we have multiple hosts. This is ok, since we test migration here!
            //AZ_TEST_ASSERT(false);
        }
        // Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
        void OnSessionDelete(GridSession* session) override
        {
            int i;
            for (i = 0; i < k_numMachines; ++i)
            {
                if (m_sessions[i] == session)
                {
                    break;
                }
            }

            AZ_TEST_ASSERT(i < k_numMachines);
            m_sessions[i] = nullptr;
        }
        void OnMigrationStart(GridSession* session) override
        {
            (void)session;
            AZ_TracePrintf("GridMate", "Migration start on %s at frame %d\n", session->GetMyMember()->GetId().ToAddress().c_str(), m_numUpdates);
        }
        void OnMigrationElectHost(GridSession* session, GridMember*& newHost) override
        {
            (void)session;
            (void)newHost;
            AZ_TracePrintf("GridMate", "Migration elect host on %s at frame %d\n", session->GetMyMember()->GetId().ToAddress().c_str(), m_numUpdates);
        }
        void OnMigrationEnd(GridSession* session, GridMember* newHost) override
        {
            (void)session;
            (void)newHost;
            AZ_TracePrintf("GridMate", "Migration end on %s, new host %s at frame %d\n", session->GetMyMember()->GetId().ToAddress().c_str(), session->GetHost()->GetId().ToAddress().c_str(), m_numUpdates);
            for (int i = 0; i < k_numMachines; ++i)
            {
                if (m_sessions[i] == session)
                {
                    if (session->GetMyMember() == newHost)
                    {
                        m_host = i;
                        return;
                    }
                }
            }
        }
        LANSessionMigarationTestTest3()
        {
            //////////////////////////////////////////////////////////////////////////
            // Create all grid mates
            m_gridMates[0] = m_gridMate;
            for (int i = 1; i < k_numMachines; ++i)
            {
                GridMateDesc desc;
                m_gridMates[i] = GridMateCreate(desc);
                AZ_TEST_ASSERT(m_gridMates[i]);
            }
            //////////////////////////////////////////////////////////////////////////

            // Hook to session event bus.
            SessionEventBus::Handler::BusConnect(m_gridMate);

            for (int i = 0; i < k_numMachines; ++i)
            {
                // start the multiplayer service (session mgr, extra allocator, etc.)
                StartGridMateService<LANSessionService>(m_gridMates[i], SessionServiceDesc());
                AZ_TEST_ASSERT(LANSessionServiceBus::FindFirstHandler(m_gridMates[i]) != nullptr);
            }

            //StartDrilling("lanmigration2");
        }

        ~LANSessionMigarationTestTest3() override
        {
            StopGridMateService<LANSessionService>(m_gridMates[0]);

            for (int i = 1; i < k_numMachines; ++i)
            {
                if (m_gridMates[i])
                {
                    GridMateDestroy(m_gridMates[i]);
                }
            }

            // Unhook from session event bus.
            SessionEventBus::Handler::BusDisconnect();

            //StopDrilling();
        }

        void run()
        {
            TestCarrierDesc carrierDesc;
            carrierDesc.m_enableDisconnectDetection = /*false*/ true;
            carrierDesc.m_threadUpdateTimeMS = 10;
            carrierDesc.m_simulator = &m_simulators[0];

            m_host = 1; // first we set the second session/machine to be the host (so we can test non zero host)

            // 1. We start a session with 3 members.
            memset(m_sessions, 0, sizeof(m_sessions));
            LANSessionParams sp;
            sp.m_topology = ST_PEER_TO_PEER;
            sp.m_numPublicSlots = 64;
            sp.m_port = k_hostPort;
            sp.m_numParams = 2;
            sp.m_params[0].m_id = "Param1";
            sp.m_params[0].SetValue(15);
            sp.m_params[1].m_id = "Param2";
            sp.m_params[1].SetValue(25);
            EBUS_EVENT_ID_RESULT(m_sessions[m_host],m_gridMates[m_host],LANSessionServiceBus,HostSession,sp,carrierDesc);

            memset(m_lanSearch, 0, sizeof(m_lanSearch));
            int listenPort = k_hostPort;
            int numSessionsUsed = 0;
            for (; numSessionsUsed < k_numInitialMembers; ++numSessionsUsed)
            {
                if (numSessionsUsed == m_host)
                {
                    continue;
                }

                LANSearchParams searchParams;
                searchParams.m_serverPort = k_hostPort;
                searchParams.m_listenPort = listenPort == k_hostPort ? 0 : ++listenPort;    // first client will use ephemeral port
                                                                                            // the rest specify return ports
                searchParams.m_numParams = 1;
                searchParams.m_params[0].m_id = "Param2";
                searchParams.m_params[0].SetValue(25);
                searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                EBUS_EVENT_ID_RESULT(m_lanSearch[numSessionsUsed],m_gridMates[numSessionsUsed],LANSessionServiceBus,StartGridSearch,searchParams);
            }

            int maxNumUpdates = 600;
            m_numUpdates = 0;
            TimeStamp time = AZStd::chrono::system_clock::now();
            while (m_numUpdates <= maxNumUpdates)
            {
                //////////////////////////////////////////////////////////////////////////
                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_gridMates[i])
                    {
                        m_gridMates[i]->Update();

                        if (m_sessions[i])
                        {
                            UpdateReplicaManager(m_sessions[i]->GetReplicaMgr());
                        }
                    }
                }
                Update();
                //////////////////////////////////////////////////////////////////////////

                for (int i = 0; i < k_numMachines; ++i)
                {
                    if (m_lanSearch[i] && m_lanSearch[i]->IsDone())
                    {
                        if (m_lanSearch[i]->GetNumResults() == 1)
                        {
                            carrierDesc.m_simulator = &m_simulators[i];
                            EBUS_EVENT_ID_RESULT(m_sessions[i],m_gridMates[i],LANSessionServiceBus,JoinSessionBySearchInfo,static_cast<const LANSearchInfo&>(*m_lanSearch[i]->GetResult(0)),JoinParams(),carrierDesc);
                        }

                        m_lanSearch[i]->Release();
                        m_lanSearch[i] = nullptr;
                    }
                }

                // 2. Terminate connection on next host candidate to the host.
                if (m_numUpdates == 150)
                {
                    GridMember* memberToDisconnect = m_sessions[m_host]->GetMemberByIndex(1);
                    int memberSession = -1;
                    for (int i = 0; i < k_numMachines; ++i)
                    {
                        if (m_sessions[i])
                        {
                            if (m_sessions[i]->GetMyMember()->GetId() == memberToDisconnect->GetId())
                            {
                                memberSession = i;
                                break;
                            }
                        }
                    }
                    AZ_TEST_ASSERT(memberSession != -1);
                    for (unsigned int i = 0; i < m_sessions[memberSession]->GetNumberOfMembers(); i++)
                    {
                        GridMember* member = m_sessions[memberSession]->GetMemberByIndex(i);
                        if (member->GetId() == m_sessions[m_host]->GetMyMember()->GetId())
                        {
                            // we found the host member, kill the connection to it! (so we detect it first)
                            m_sessions[memberSession]->GetCarrier()->DebugDeleteConnection(member->GetConnectionId());
                            break;
                        }
                    }
                }

                // 3. Add 2 new joins to the original session.
                if (m_numUpdates == 400)
                {
                    for (; numSessionsUsed < k_numMachines; ++numSessionsUsed)
                    {
                        //if(numSessionsUsed==host) continue;

                        LANSearchParams searchParams;
                        searchParams.m_serverPort = k_hostPort;
                        searchParams.m_listenPort = listenPort++;
                        searchParams.m_numParams = 1;
                        searchParams.m_params[0].m_id = "Param2";
                        searchParams.m_params[0].SetValue(25);
                        searchParams.m_params[0].m_op = GridSessionSearchOperators::SSO_OPERATOR_EQUAL;
                        EBUS_EVENT_ID_RESULT(m_lanSearch[numSessionsUsed],m_gridMates[numSessionsUsed],LANSessionServiceBus,StartGridSearch,searchParams);
                    }
                }

                //////////////////////////////////////////////////////////////////////////
                // Debug Info
                TimeStamp now = AZStd::chrono::system_clock::now();
                if (AZStd::chrono::milliseconds(now - time).count() > 1000)
                {
                    time = now;
                    for (int i = 0; i < k_numMachines; ++i)
                    {
                        if (m_sessions[i] == nullptr)
                        {
                            continue;
                        }

                        if (m_host == i)
                        {
                            AZ_Printf("GridMate", "------ Host %d ------\n", i);
                        }
                        else
                        {
                            AZ_Printf("GridMate", "------ Client %d ------\n", i);
                        }

                        AZ_Printf("GridMate", "Session %s Members: %d Host: %s Clock: %d\n", m_sessions[i]->GetId().c_str(), m_sessions[i]->GetNumberOfMembers(), m_sessions[i]->IsHost() ? "yes" : "no", m_sessions[i]->GetTime());
                        for (unsigned int iMember = 0; iMember < m_sessions[i]->GetNumberOfMembers(); ++iMember)
                        {
                            GridMember* member = m_sessions[i]->GetMemberByIndex(iMember);
                            AZ_Printf("GridMate", "  Member: %s(%s) Host: %s Local: %s\n", member->GetName().c_str(), member->GetId().ToString().c_str(), member->IsHost() ? "yes" : "no", member->IsLocal() ? "yes" : "no");
                        }
                        AZ_Printf("GridMate", "\n");
                    }
                }
                //////////////////////////////////////////////////////////////////////////

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                m_numUpdates++;
            }
        }
    };

}

GM_TEST_SUITE(SessionSuite)
GM_TEST(DISABLED_LANSessionMatchmakingParamsTest)
GM_TEST(DISABLED_LANSessionTest)
#if (AZ_TRAIT_GRIDMATE_TEST_SOCKET_IPV6_SUPPORT_ENABLED)
GM_TEST(DISABLED_LANSessionTestIPv6)
#endif
GM_TEST(DISABLED_LANMultipleSessionTest)
GM_TEST(DISABLED_LANLatencySessionTest)

// Manually enabled tests (require 2+ machines and online services)
//GM_TEST(LANSessionMigarationTestTest)
//GM_TEST(LANSessionMigarationTestTest2)
//GM_TEST(LANSessionMigarationTestTest3)

GM_TEST_SUITE_END()
