/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#pragma once

#include <GameLift/GameLiftBus.h>

#if defined(BUILD_GAMELIFT_CLIENT)
#include <GameLift/Session/GameLiftClientServiceBus.h>
#include <GameLift/Session/GameLiftSearch.h>
#endif

#if defined(BUILD_GAMELIFT_SERVER)
#include <GameLift/Session/GameLiftServerService.h>
#endif

#include <Multiplayer/IMultiplayerGem.h>
#include <Mocks/ISystemMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/INetworkMock.h>
#include <SFunctor.h>

#include <gmock/gmock.h>

#include "Multiplayer/MultiplayerLobbyServiceWrapper/MultiplayerLobbyLANServiceWrapper.h"
#include "Canvas/MultiplayerDedicatedHostTypeSelectionCanvas.h"
#include "Canvas/MultiplayerGameLiftLobbyCanvas.h"
#include "Canvas/MultiplayerBusyAndErrorCanvas.h"
#include "Canvas/MultiplayerLANGameLobbyCanvas.h"
#include "Canvas/MultiplayerJoinServerView.h"
#include "Canvas/MultiplayerCreateServerView.h"
#include "Canvas/MultiplayerGameLiftFlextMatchView.h"

namespace UnitTest
{
    using testing::_;
    using testing::Invoke;
    using testing::Return;

    class MultiplayerLANGameLobbyCanvasMock
        : public Multiplayer::MultiplayerLANGameLobbyCanvas
    {
    public:
        MultiplayerLANGameLobbyCanvasMock()
        {
            Multiplayer::MultiplayerJoinServerViewContext joinContext;
            m_joinServerScreen = aznew Multiplayer::MultiplayerJoinServerView(joinContext, AZ::EntityId(4));
            Multiplayer::MultiplayerCreateServerViewContext createServerContext;
            m_createServerScreen = aznew Multiplayer::MultiplayerCreateServerView(createServerContext, AZ::EntityId(5));

            ON_CALL(*this, GetMapName()).WillByDefault(Return(m_testMapName));
            ON_CALL(*this, GetServerName()).WillByDefault(Return(m_testServerName));
            ON_CALL(*this, GetSelectedServerResult()).WillByDefault(Return(0));
        }

        MOCK_METHOD0(Show, void());
        MOCK_METHOD0(Hide, void());
        MOCK_CONST_METHOD0(GetMapName, LyShine::StringType());
        MOCK_CONST_METHOD0(GetServerName, LyShine::StringType());
        MOCK_METHOD0(GetSelectedServerResult, int());
        MOCK_METHOD1(DisplaySearchResults, void(const GridMate::GridSearch*));
        MOCK_METHOD0(ClearSearchResults, void());

    protected:
        const char* m_testMapName = "TestMap";
        const char* m_testServerName = "TestServer";
    };

    class MultiplayerBusyAndErrorCanvasMock
        : public Multiplayer::MultiplayerBusyAndErrorCanvas
    {
    public:
        MultiplayerBusyAndErrorCanvasMock()
        {

        }
        MOCK_METHOD1(ShowError, void(const char* error));
        MOCK_METHOD1(DismissError, void(bool force));
        MOCK_METHOD0(ShowBusyScreen, void());
        MOCK_METHOD1(DismissBusyScreen, void(bool force));
    };

    class MultiplayerGameLiftLobbyCanvasMock
        : public Multiplayer::MultiplayerGameLiftLobbyCanvas
    {
    public:
        MultiplayerGameLiftLobbyCanvasMock()
        {
            Multiplayer::MultiplayerJoinServerViewContext joinContext;
            m_joinServerScreen = aznew Multiplayer::MultiplayerJoinServerView(joinContext, AZ::EntityId(1));
            Multiplayer::MultiplayerCreateServerViewContext createServerContext;
            m_createServerScreen = aznew Multiplayer::MultiplayerCreateServerView(createServerContext, AZ::EntityId(2));
            Multiplayer::MultiplayerGameLiftFlextMatchViewContext flexMatchContext;
            m_flexMatchScreen = aznew Multiplayer::MultiplayerGameLiftFlextMatchView(flexMatchContext, AZ::EntityId(3));

            ON_CALL(*this, GetMapName()).WillByDefault(Return(m_testMapName));
            ON_CALL(*this, GetServerName()).WillByDefault(Return(m_testServerName));
            ON_CALL(*this, GetSelectedServerResult()).WillByDefault(Return(0));
        }

        MOCK_METHOD0(Show, void());
        MOCK_METHOD0(Hide, void());
        MOCK_CONST_METHOD0(GetMapName, LyShine::StringType());
        MOCK_CONST_METHOD0(GetServerName, LyShine::StringType());
        MOCK_METHOD0(GetSelectedServerResult, int());
        MOCK_METHOD1(DisplaySearchResults, void(const GridMate::GridSearch*));
        MOCK_METHOD0(ClearSearchResults, void());
    protected:
        const char* m_testMapName = "TestMap";
        const char* m_testServerName = "TestServer";
    };

    class MultiplayerDedicatedHostTypeSelectionCanvasMock
        : public Multiplayer::MultiplayerDedicatedHostTypeSelectionCanvas
    {
    public:
        MultiplayerDedicatedHostTypeSelectionCanvasMock()
        {
        }

        MOCK_METHOD0(Show, void());
        MOCK_METHOD0(Hide, void());
    };

    class MockGameLiftRequestBus
        : public GameLift::GameLiftRequestBus::Handler
    {
    public:
        MockGameLiftRequestBus()
        {
            GameLift::GameLiftRequestBus::Handler::BusConnect();
        }

        ~MockGameLiftRequestBus()
        {
            GameLift::GameLiftRequestBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD0(IsGameLiftServer, bool());
#if defined(BUILD_GAMELIFT_CLIENT)
        MOCK_METHOD1(StartClientService, GridMate::GameLiftClientService*(const GridMate::GameLiftClientServiceDesc& desc));
        MOCK_METHOD0(StopClientService, void());
        MOCK_METHOD0(GetClientService, GridMate::GameLiftClientService*());
#endif

#if defined(BUILD_GAMELIFT_SERVER)
        MOCK_METHOD1(StartServerService, GridMate::GameLiftServerService*(const GridMate::GameLiftServerServiceDesc& desc));
        MOCK_METHOD0(StopServerService, void());
        MOCK_METHOD0(GetServerService, GridMate::GameLiftServerService*());
#endif
    };

    class MockSession
        : public GridMate::GridSession
    {
    public:

        using GridMate::GridSession::m_state;

        MockSession(GridMate::SessionService* service)
            : GridSession(service)
        {
        }

        MOCK_METHOD4(CreateRemoteMember, GridMate::GridMember*(const GridMate::string&, GridMate::ReadBuffer&, GridMate::RemotePeerMode, GridMate::ConnectionID));
        MOCK_METHOD1(OnSessionParamChanged, void(const GridMate::GridSessionParam&));
        MOCK_METHOD1(OnSessionParamRemoved, void(const GridMate::string&));
    };

    class MockSessionService
        : public GridMate::SessionService
    {
    public:
        MockSessionService()
            : SessionService(GridMate::SessionServiceDesc())
        {
        }

        ~MockSessionService()
        {
            m_activeSearches.clear();
            m_gridMate = nullptr;
        }

        MOCK_CONST_METHOD0(IsReady, bool());
    };

    class MockGridSearch
        : public GridMate::GridSearch
    {
    public:
        GM_CLASS_ALLOCATOR(MockGridSearch);

        MockGridSearch(GridMate::SessionService* sessionService)
            : GridSearch(sessionService)
        {
            m_isDone = true;
            ON_CALL(*this, GetNumResults()).WillByDefault(Invoke([this]() { return m_results.size(); }));
            ON_CALL(*this, GetResult(_)).WillByDefault(Invoke(
                [this](unsigned int index) -> const GridMate::SearchInfo*
            {
                return &m_results[index];

            }));
        }

        void AddSearchResult()
        {
            m_results.push_back();
        }

        MOCK_CONST_METHOD0(GetNumResults, unsigned int());
        MOCK_CONST_METHOD1(GetResult, const GridMate::SearchInfo*(unsigned int index));
        MOCK_METHOD0(AbortSearch, void());

        GridMate::vector<GridMate::SearchInfo> m_results;
    };

#if defined(BUILD_GAMELIFT_CLIENT)
    class MockGameLiftClientServiceBus
        : public GridMate::GameLiftClientServiceBus::Handler
    {
    public:
        MockGameLiftClientServiceBus()
        {
            ON_CALL(*this, JoinSessionBySearchInfo(_, _)).WillByDefault(Invoke(this, &MockGameLiftClientServiceBus::DefaultJoinSessionBySearchInfo));
            ON_CALL(*this, RequestSession(_)).WillByDefault(Invoke(this, &MockGameLiftClientServiceBus::DefaultRequestSession));
            ON_CALL(*this, StartMatchmaking(_)).WillByDefault(Invoke(this, &MockGameLiftClientServiceBus::DefaultStartMatchmaking));
            ON_CALL(*this, StartSearch(_)).WillByDefault(Invoke(this, &MockGameLiftClientServiceBus::DefaultStartSearch));
        }

        MOCK_METHOD2(JoinSessionBySearchInfo, GridMate::GridSession*(const GridMate::GameLiftSearchInfo& params, const GridMate::CarrierDesc& carrierDesc));
        MOCK_METHOD1(RequestSession, GridMate::GridSearch*(const GridMate::GameLiftSessionRequestParams& params));
        MOCK_METHOD1(StartMatchmaking, GridMate::GridSearch*(const AZStd::string& matchmakingConfig));
        MOCK_METHOD1(StartSearch, GridMate::GameLiftSearch*(const GridMate::GameLiftSearchParams& params));
        MOCK_METHOD1(QueryGameLiftSession, GridMate::GameLiftClientSession*(const GridMate::GridSession* session));
        MOCK_METHOD1(QueryGameLiftSearch, GridMate::GameLiftSearch*(const GridMate::GridSearch* search));

        void Start(GridMate::IGridMate* gridMate)
        {
            m_sessionService = AZStd::make_unique<MockSessionService>();
            GridMate::GameLiftClientServiceBus::Handler::BusConnect(gridMate);
        }

        void Stop()
        {
            m_sessionService.reset();
            GridMate::GameLiftClientServiceBus::Handler::BusDisconnect();
        }

        GridMate::GridSession* DefaultJoinSessionBySearchInfo([[maybe_unused]] const GridMate::GameLiftSearchInfo& params, [[maybe_unused]] const GridMate::CarrierDesc& carrierDesc)
        {
            m_session = AZStd::make_unique<MockSession>(m_sessionService.get());
            return m_session.get();
        }

        GridMate::GridSearch* DefaultRequestSession([[maybe_unused]] const GridMate::GameLiftSessionRequestParams& params)
        {
            EXPECT_EQ(nullptr, m_search);

            m_search = aznew MockGridSearch(m_sessionService.get());
            return reinterpret_cast<GridMate::GridSearch*>(m_search);
        }

        GridMate::GridSearch* DefaultStartMatchmaking([[maybe_unused]] const AZStd::string& matchmakingConfig)
        {
            EXPECT_EQ(nullptr, m_search);

            m_search = aznew MockGridSearch(m_sessionService.get());
            return reinterpret_cast<GridMate::GridSearch*>(m_search);
        }

        GridMate::GameLiftSearch* DefaultStartSearch([[maybe_unused]] const GridMate::GameLiftSearchParams& params)
        {
            EXPECT_EQ(nullptr, m_search);

            m_search = aznew MockGridSearch(m_sessionService.get());
            return reinterpret_cast<GridMate::GameLiftSearch*>(m_search);
        }

        MockGridSearch* m_search = nullptr;

        AZStd::unique_ptr<MockSession> m_session;

        AZStd::unique_ptr<MockSessionService> m_sessionService;
    };
#endif

    class MultiplayerLobbyLANServiceWrapperMock
        : public Multiplayer::MultiplayerLobbyLANServiceWrapper
    {
    public:
        MultiplayerLobbyLANServiceWrapperMock(const AZ::EntityId& multiplayerLobbyEntityId) : Multiplayer::MultiplayerLobbyLANServiceWrapper(multiplayerLobbyEntityId)
        {
            m_sessionService = AZStd::make_unique<MockSessionService>();
            ON_CALL(*this, StartSessionService(_)).WillByDefault(Return(true));
            ON_CALL(*this, CreateServer(_,_)).WillByDefault(Invoke(
                [this]([[maybe_unused]] GridMate::IGridMate* gridMate, [[maybe_unused]] GridMate::CarrierDesc& carrierDesc)
                {
                    return GetGridSessionMock();
                }
            ));

            ON_CALL(*this, JoinSession(_, _,_)).WillByDefault(Invoke(
                [this]([[maybe_unused]] GridMate::IGridMate* gridMate, [[maybe_unused]] GridMate::CarrierDesc& carrierDesc, [[maybe_unused]] const GridMate::SearchInfo* searchInfo)
                {
                    return GetGridSessionMock();
                }
            ));

            ON_CALL(*this, ListServers(_)).WillByDefault(Invoke(
                [this]([[maybe_unused]] GridMate::IGridMate* gridMate)
                {
                    return GetGridSearchMock();
                }
            ));
        }

        MOCK_METHOD1(StartSessionService, bool(GridMate::IGridMate* gridMate));
        MOCK_METHOD1(StopSessionService, void(GridMate::IGridMate* gridMate));
        MOCK_METHOD2(CreateServer, GridMate::GridSession*(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc));
        MOCK_METHOD1(ListServers, GridMate::GridSearch*(GridMate::IGridMate* gridMate));
        MOCK_METHOD3(JoinSession, GridMate::GridSession*(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo));

        GridMate::GridSession* GetGridSessionMock()
        {
            m_session = AZStd::make_unique<MockSession>(m_sessionService.get());
            return m_session.get();
        }

        GridMate::GridSearch* GetGridSearchMock()
        {
            EXPECT_EQ(nullptr, m_search);

            m_search = aznew MockGridSearch(m_sessionService.get());
            return reinterpret_cast<GridMate::GridSearch*>(m_search);
        }

    private:
        AZStd::unique_ptr<MockSession> m_session;

        AZStd::unique_ptr<MockSessionService> m_sessionService;
        MockGridSearch* m_search = nullptr;
    };

    class MockMultiplayerRequestBus
        : public Multiplayer::MultiplayerRequestBus::Handler
    {
    public:
        MockMultiplayerRequestBus()
        {
            ON_CALL(*this, GetSession()).WillByDefault(Return(m_session));
            ON_CALL(*this, RegisterSession(_)).WillByDefault(Invoke(this, &MockMultiplayerRequestBus::DefaultRegisterSession));
            Multiplayer::MultiplayerRequestBus::Handler::BusConnect();
        }

        ~MockMultiplayerRequestBus()
        {
            Multiplayer::MultiplayerRequestBus::Handler::BusDisconnect();
        }

        MOCK_CONST_METHOD0(IsNetSecEnabled, bool());
        MOCK_CONST_METHOD0(IsNetSecVerifyClient, bool());
        MOCK_METHOD1(RegisterSecureDriver, void(GridMate::SecureSocketDriver* driver));
        MOCK_METHOD0(GetSession, GridMate::GridSession*());
        MOCK_METHOD1(RegisterSession, void(GridMate::GridSession* gridSession));
        MOCK_METHOD0(GetSimulator, GridMate::Simulator*());
        MOCK_METHOD0(EnableSimulator, void());
        MOCK_METHOD0(DisableSimulator, void());

        void DefaultRegisterSession(GridMate::GridSession* gridSession)
        {
            m_session = gridSession;
        }

        GridMate::GridSession* m_session = nullptr;
    };

    class MockCVar : public ICVar {
    public:
        MockCVar() {}
        MockCVar(const char* name, const char* value) : m_name(name), m_type(CVAR_STRING), m_strVal(value) { InitDefaultBehavior(); }
        MockCVar(const char* name, int value) : m_name(name), m_type(CVAR_INT), m_intVal(value) { InitDefaultBehavior(); }
        MockCVar(const char* name, int64 value) : m_name(name), m_type(CVAR_INT), m_intVal(value) { InitDefaultBehavior(); }
        MockCVar(const char* name, float value) : m_name(name), m_type(CVAR_FLOAT), m_floatVal(value) { InitDefaultBehavior(); }

        void InitDefaultBehavior()
        {
            using testing::Return;

            ON_CALL(*this, GetIVal()).WillByDefault(Return(static_cast<int>(m_intVal)));
            ON_CALL(*this, GetI64Val()).WillByDefault(Return(m_intVal));
            ON_CALL(*this, GetFVal()).WillByDefault(Return(m_floatVal));
            ON_CALL(*this, GetString()).WillByDefault(Return(m_strVal.c_str()));
        }

        MOCK_METHOD0(Release, void());
        MOCK_CONST_METHOD0(GetIVal, int());
        MOCK_CONST_METHOD0(GetI64Val, int64());
        MOCK_CONST_METHOD0(GetFVal, float());
        MOCK_CONST_METHOD0(GetString, const char*());
        MOCK_CONST_METHOD0(GetDataProbeString, const char*());
        MOCK_METHOD0(Reset, void());
        MOCK_METHOD1(Set, void(const char* s));
        MOCK_METHOD1(ForceSet, void(const char* s));
        MOCK_METHOD1(Set, void(const float f));
        MOCK_METHOD1(Set, void(const int i));
        MOCK_METHOD1(ClearFlags, void(int flags));
        MOCK_CONST_METHOD0(GetFlags, int());
        MOCK_METHOD1(SetFlags, int(int flags));
        MOCK_METHOD0(GetType, int());
        MOCK_CONST_METHOD0(GetName, const char*());
        MOCK_METHOD0(GetHelp, const char*());
        MOCK_CONST_METHOD0(IsConstCVar, bool());
        MOCK_METHOD1(SetOnChangeCallback, void(ConsoleVarFunc pChangeFunc));
        MOCK_METHOD1(AddOnChangeFunctor, uint64(const SFunctor& pChangeFunctor));
        MOCK_CONST_METHOD0(GetNumberOfOnChangeFunctors, uint64());
        MOCK_CONST_METHOD1(GetOnChangeFunctor, const SFunctor&(uint64 nFunctorId));
        MOCK_METHOD1(RemoveOnChangeFunctor, bool(const uint64 nFunctorId));
        MOCK_CONST_METHOD0(GetOnChangeCallback, ConsoleVarFunc());
        MOCK_CONST_METHOD1(GetMemoryUsage, void(class ICrySizer* pSizer));
        MOCK_CONST_METHOD0(GetRealIVal, int());
        MOCK_METHOD2(SetLimits, void(float min, float max));
        MOCK_METHOD2(GetLimits, void(float& min, float& max));
        MOCK_METHOD0(HasCustomLimits, bool());
        MOCK_CONST_METHOD2(DebugLog, void(const int iExpectedValue, const EConsoleLogMode mode));
        MOCK_METHOD1(SetDataProbeString, void(const char* pDataProbeString));

        AZStd::string m_name;
        int m_type = 0;
        int64 m_intVal = 0;
        float m_floatVal = 0;
        AZStd::string m_strVal;
    };

    class MockConsole : public ConsoleMock {
    public:
        MockConsole()
        {
            ON_CALL(*this, RegisterString(_, _, _, _, _)).WillByDefault(Invoke(this, &MockConsole::RegisterCVar<const char*>));
            ON_CALL(*this, RegisterInt(_, _, _, _, _)).WillByDefault(Invoke(this, &MockConsole::RegisterCVar<int>));
            ON_CALL(*this, RegisterInt64(_, _, _, _, _)).WillByDefault(Invoke(this, &MockConsole::RegisterCVar<int64>));
            ON_CALL(*this, RegisterFloat(_, _, _, _, _)).WillByDefault(Invoke(this, &MockConsole::RegisterCVar<float>));

            ON_CALL(*this, GetCVar(_)).WillByDefault(Invoke(
                [this](const char* name) -> ICVar*
                {
                    auto it = m_cvars.find(name);
                    if (it == m_cvars.end())
                    {
                        return nullptr;
                    }

                    return it->second.get();
                }));
        }

        template <class T>
        ICVar* RegisterCVar(const char* name, T value, [[maybe_unused]] int flags = 0, [[maybe_unused]] const char* help = "", [[maybe_unused]] ConsoleVarFunc changeFunc = nullptr)
        {
            auto inserted = m_cvars.emplace(name, AZStd::make_unique<testing::NiceMock<MockCVar>>(name, value));
            return inserted.first->second.get();
        }

        using MockCVarContainer = AZStd::unordered_map<AZStd::string, AZStd::unique_ptr<MockCVar>>;
        MockCVarContainer m_cvars;
    };

    class MockSystem
        : public SystemMock
        , public CrySystemRequestBus::Handler
    {
    public:
        MockSystem()
        {
            CrySystemRequestBus::Handler::BusConnect();
        }

        ~MockSystem()
        {
            CrySystemRequestBus::Handler::BusDisconnect();
        }

        // CrySystemRequestBus override
        ISystem* GetCrySystem() override
        {
            return this;
        }
    };

    class MultiplayerGameSessionAllocatorsFixture
        : public AllocatorsTestFixture
    {
        NetworkMock m_testNetwork;
        SSystemGlobalEnvironment* m_oldEnv;
        SSystemGlobalEnvironment m_testSystemGlobalEnvironment;
        GridMate::IGridMate* m_gridMate;

    public:
        MultiplayerGameSessionAllocatorsFixture()
            : AllocatorsTestFixture()
            , m_gridMate(nullptr)
            , m_oldEnv(gEnv)
        {
        }

        virtual ~MultiplayerGameSessionAllocatorsFixture()
        {
            gEnv = m_oldEnv;
        }

        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();
            GetGridMate();

            // faking the global environment
            m_testSystemGlobalEnvironment.pNetwork = &m_testNetwork;
            gEnv = &m_testSystemGlobalEnvironment;
        }

        void TearDown() override
        {
            if (m_gridMate)
            {
                GridMate::GridMateDestroy(m_gridMate);
                AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
                m_gridMate = nullptr;
                m_testSystemGlobalEnvironment.pNetwork = nullptr;
            }

            AllocatorsTestFixture::TearDown();
        }

        GridMate::IGridMate* GetGridMate()
        {
            if (m_gridMate == nullptr)
            {
                m_gridMate = GridMate::GridMateCreate(GridMate::GridMateDesc());
                m_testNetwork.m_gridMate = m_gridMate;

                AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();
            }
            return m_gridMate;
        }
    };

#if defined(BUILD_GAMELIFT_CLIENT)
    class MultiplayerClientSessionAllocatorFixture
        : public MultiplayerGameSessionAllocatorsFixture
    {
    public:
        void SetUp() override
        {
            MultiplayerGameSessionAllocatorsFixture::SetUp();
            m_console = AZStd::make_unique<testing::NiceMock<UnitTest::MockConsole>>();
            gEnv->pConsole = m_console.get();

            ON_CALL(m_system, GetIConsole()).WillByDefault(testing::Return(gEnv->pConsole));
            ON_CALL(m_system, GetINetwork()).WillByDefault(testing::Return(gEnv->pNetwork));

            m_gameLiftClientServiceBus.Start(MultiplayerGameSessionAllocatorsFixture::GetGridMate());
        }

        void TearDown() override
        {
            m_gameLiftClientServiceBus.Stop();

            m_console.reset();
            gEnv->pConsole = nullptr;
            MultiplayerGameSessionAllocatorsFixture::TearDown();
        }

    protected:
        void ApplyCVars()
        {
            // Setting params used in Multiplayer::Utils
            m_console->RegisterCVar("cl_clientport", m_clientPort);
            m_console->RegisterCVar("gm_securityData", m_securityData);
            m_console->RegisterCVar("gm_ipversion", m_ipVersion);
            m_console->RegisterCVar("gm_version", m_version);
            m_console->RegisterCVar("gm_disconnectDetection", m_disconnectDetection);
            m_console->RegisterInt("gm_maxSearchResults", 5, 0 ,"", nullptr);
            m_console->RegisterInt("gm_version", 1, 0, "", nullptr);

            // Gamelift specific CVars
            m_console->RegisterCVar("gamelift_aws_access_key", m_testGameLiftAWSAccessKey);
            m_console->RegisterCVar("gamelift_aws_secret_key", m_testGameLiftAWSecretKey);
            m_console->RegisterCVar("gamelift_fleet_id", m_testGameLiftFleetId);
            m_console->RegisterCVar("gamelift_queue_name", m_testGameLiftQueueName);
            m_console->RegisterCVar("gamelift_endpoint", m_testGameLiftEndpoint);
            m_console->RegisterCVar("gamelift_aws_region", m_testGameLiftRegion);
            m_console->RegisterCVar("gamelift_alias_id", m_testGameLiftAlias);
            m_console->RegisterCVar("gamelift_player_id", m_testGameLiftPlayerId);
            m_console->RegisterCVar("gamelift_uselocalserver", false);
            m_console->RegisterCVar("gamelift_matchmaking_config_name", m_testGameLiftMatchmakingConfig);
        }

        testing::NiceMock<UnitTest::MockGameLiftRequestBus> m_gameLiftRequestBus;
        testing::NiceMock<UnitTest::MockGameLiftClientServiceBus> m_gameLiftClientServiceBus;
        testing::NiceMock<UnitTest::MockMultiplayerRequestBus> m_multiplayerRequestBus;
        testing::NiceMock<UnitTest::MockSystem> m_system;
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockConsole>> m_console;

        int m_clientPort = 0;
        const char* m_securityData = "";
        const char* m_ipVersion = "IPV4";
        const char* m_version = "";
        int m_disconnectDetection = 0;
        const char* m_testGameLiftFleetId = "fleet-TestFleetId";
        const char* m_testGameLiftMatchmakingConfig = "MSTestConfig";
        const char* m_testGameLiftAWSAccessKey = "A";
        const char* m_testGameLiftAWSecretKey = "A";
        const char* m_testGameLiftQueueName = "TestQueue";
        const char* m_testGameLiftEndpoint = "gamelift.us-west-2.amazonaws.com";
        const char* m_testGameLiftRegion = "us-west-2";
        const char* m_testGameLiftAlias = "TestAlias";
        const char* m_testGameLiftPlayerId = "TestPlayer";
    };
#endif
} // namespace UnitTest
