/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSGameLiftServerFixture.h>
#include <AWSGameLiftServerMocks.h>

#include <AzCore/Interface/Interface.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Session/SessionConfig.h>
#include <AzFramework/Session/SessionNotifications.h>

namespace UnitTest
{
    static constexpr const char TEST_SERVER_MATCHMAKING_DATA[] =
R"({
    "matchId":"testmatchid",
    "matchmakingConfigurationArn":"testmatchconfig",
    "teams":[
        {"name":"testteam",
         "players":[
             {"playerId":"testplayer",
              "attributes":{
                  "skills":{
                      "attributeType":"STRING_DOUBLE_MAP",
                      "valueAttribute":{"test1":10.0,"test2":20.0,"test3":30.0,"test4":40.0}
                  },
                  "mode":{
                      "attributeType":"STRING",
                      "valueAttribute":"testmode"
                  },
                  "level":{
                      "attributeType":"DOUBLE",
                      "valueAttribute":10.0
                  },
                  "items":{
                      "attributeType":"STRING_LIST",
                      "valueAttribute":["test1","test2","test3"]
                  }
              }},
             {"playerId":"secondplayer",
              "attributes":{
                  "mode":{
                      "attributeType":"STRING",
                      "valueAttribute":"testmode"
                  }
              }}
         ]}
    ]
})";

    Aws::GameLift::Server::Model::StartMatchBackfillRequest GetTestStartMatchBackfillRequest()
    {
        Aws::GameLift::Server::Model::StartMatchBackfillRequest request;
        request.SetMatchmakingConfigurationArn("testmatchconfig");
        Aws::GameLift::Server::Model::Player player;
        player.SetPlayerId("testplayer");
        player.SetTeam("testteam");
        player.AddPlayerAttribute("mode", Aws::GameLift::Server::Model::AttributeValue("testmode"));
        player.AddPlayerAttribute("level", Aws::GameLift::Server::Model::AttributeValue(10.0));
        auto sdmValue = Aws::GameLift::Server::Model::AttributeValue::ConstructStringDoubleMap();
        sdmValue.AddStringAndDouble("test1", 10.0);
        player.AddPlayerAttribute("skills", sdmValue);
        auto slValue = Aws::GameLift::Server::Model::AttributeValue::ConstructStringList();
        slValue.AddString("test1");
        player.AddPlayerAttribute("items", slValue);
        player.AddLatencyInMs("testregion", 10);
        request.AddPlayer(player);
        request.SetTicketId("testticket");
        return request;
    }

    AWSGameLiftPlayer GetTestGameLiftPlayer()
    {
        AWSGameLiftPlayer player;
        player.m_team = "testteam";
        player.m_playerId = "testplayer";
        player.m_playerAttributes.emplace("mode", "{\"S\": \"testmode\"}");
        player.m_playerAttributes.emplace("level", "{\"N\": 10.0}");
        player.m_playerAttributes.emplace("skills", "{\"SDM\": {\"test1\":10.0}}");
        player.m_playerAttributes.emplace("items", "{\"SL\": [\"test1\"]}");
        player.m_latencyInMs.emplace("testregion", 10);
        return player;
    }

    MATCHER_P(StartMatchBackfillRequestMatcher, expectedRequest, "")
    {
        // Custome matcher for checking the SearchSessionsResponse type argument.
        AZ_UNUSED(result_listener);
        if (strcmp(arg.GetGameSessionArn().c_str(), expectedRequest.GetGameSessionArn().c_str()) != 0)
        {
            return false;
        }
        if (strcmp(arg.GetMatchmakingConfigurationArn().c_str(), expectedRequest.GetMatchmakingConfigurationArn().c_str()) != 0)
        {
            return false;
        }
        if (strcmp(arg.GetTicketId().c_str(), expectedRequest.GetTicketId().c_str()) != 0)
        {
            return false;
        }
        if (arg.GetPlayers().size() != expectedRequest.GetPlayers().size())
        {
            return false;
        }
        for (int playerIndex = 0; playerIndex < expectedRequest.GetPlayers().size(); playerIndex++)
        {
            auto actualPlayerAttributes = arg.GetPlayers()[playerIndex].GetPlayerAttributes();
            auto expectedPlayerAttributes = expectedRequest.GetPlayers()[playerIndex].GetPlayerAttributes();
            if (actualPlayerAttributes.size() != expectedPlayerAttributes.size())
            {
                return false;
            }
            for (auto attributePair : expectedPlayerAttributes)
            {
                if (actualPlayerAttributes.find(attributePair.first) == actualPlayerAttributes.end())
                {
                    return false;
                }
                if (!(attributePair.second.GetType() == actualPlayerAttributes[attributePair.first].GetType() &&
                      (attributePair.second.GetS() == actualPlayerAttributes[attributePair.first].GetS() ||
                       attributePair.second.GetN() == actualPlayerAttributes[attributePair.first].GetN() ||
                       attributePair.second.GetSL() == actualPlayerAttributes[attributePair.first].GetSL() ||
                       attributePair.second.GetSDM() == actualPlayerAttributes[attributePair.first].GetSDM())))
                {
                    return false;
                }
            }

            auto actualLatencies = arg.GetPlayers()[playerIndex].GetLatencyInMs();
            auto expectedLatencies = expectedRequest.GetPlayers()[playerIndex].GetLatencyInMs();
            if (actualLatencies.size() != expectedLatencies.size())
            {
                return false;
            }
            for (auto latencyPair : expectedLatencies)
            {
                if (actualLatencies.find(latencyPair.first) == actualLatencies.end())
                {
                    return false;
                }
                if (latencyPair.second != actualLatencies[latencyPair.first])
                {
                    return false;
                }
            }
        }

        return true;
    }

    class SessionNotificationsHandlerMock
        : public AzFramework::SessionNotificationBus::Handler
    {
    public:
        SessionNotificationsHandlerMock()
        {
            AzFramework::SessionNotificationBus::Handler::BusConnect();
        }

        ~SessionNotificationsHandlerMock()
        {
            AzFramework::SessionNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(OnSessionHealthCheck, bool());
        MOCK_METHOD1(OnCreateSessionBegin, bool(const AzFramework::SessionConfig&));
        MOCK_METHOD0(OnCreateSessionEnd, void());
        MOCK_METHOD0(OnDestroySessionBegin, bool());
        MOCK_METHOD0(OnDestroySessionEnd, void());
        MOCK_METHOD2(OnUpdateSessionBegin, void(const AzFramework::SessionConfig&, const AZStd::string&));
        MOCK_METHOD0(OnUpdateSessionEnd, void());
    };

    class GameLiftServerManagerTest
        : public AWSGameLiftServerFixture
    {
    public:
        void SetUp() override
        {
            AWSGameLiftServerFixture::SetUp();

            GameLiftServerProcessDesc serverDesc;
            m_serverManager = AZStd::make_unique<NiceMock<AWSGameLiftServerManagerMock>>();

            // Set up the file IO and alias
            m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);
            m_localFileIO->SetAlias("@log@", AZ_TRAIT_TEST_ROOT_FOLDER);
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);

            m_serverManager.reset();

            AWSGameLiftServerFixture::TearDown();
        }

        AZStd::unique_ptr<NiceMock<AWSGameLiftServerManagerMock>> m_serverManager;
        AZ::IO::FileIOBase* m_priorFileIO;
        AZ::IO::FileIOBase* m_localFileIO;
    };

    TEST_F(GameLiftServerManagerTest, InitializeGameLiftServerSDK_InitializeTwice_InitSDKCalledOnce)
    {
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), InitSDK()).Times(1);
        m_serverManager->InitializeGameLiftServerSDK();

        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->InitializeGameLiftServerSDK();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, NotifyGameLiftProcessReady_SDKNotInitialized_FailToNotifyGameLift)
    {
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessReady(testing::_)).Times(0);

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_serverManager->NotifyGameLiftProcessReady());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, NotifyGameLiftProcessReady_SDKInitialized_ProcessReadyNotificationSent)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessReady(testing::_)).Times(1);

        EXPECT_TRUE(m_serverManager->NotifyGameLiftProcessReady());
    }

    TEST_F(GameLiftServerManagerTest, NotifyGameLiftProcessReady_ProcessReadyFails_TerminationNotificationSent)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessReady(testing::_))
            .Times(1)
            .WillOnce(testing::Return(Aws::GameLift::GenericOutcome()));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(1);
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(m_serverManager->NotifyGameLiftProcessReady());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, OnProcessTerminate_OnDestroySessionBeginReturnsFalse_FailToNotifyGameLift)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        if (!AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get())
        {
            AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Register(m_serverManager.get());
        }

        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(false));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), GetTerminationTime()).Times(1);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(0);
        EXPECT_CALL(handlerMock, OnDestroySessionEnd()).Times(0);

        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onProcessTerminateFunc();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_FALSE(AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get());
    }

    TEST_F(GameLiftServerManagerTest, OnProcessTerminate_OnDestroySessionBeginReturnsTrue_TerminationNotificationSent)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        if (!AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get())
        {
            AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Register(m_serverManager.get());
        }

        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), GetTerminationTime()).Times(1);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding())
            .Times(1)
            .WillOnce(testing::Return(Aws::GameLift::GenericOutcome(nullptr)));
        EXPECT_CALL(handlerMock, OnDestroySessionEnd()).Times(1);

        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onProcessTerminateFunc();

        EXPECT_FALSE(AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get());
    }

    TEST_F(GameLiftServerManagerTest, OnProcessTerminate_OnDestroySessionBeginReturnsTrue_TerminationNotificationSentButFail)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        if (!AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get())
        {
            AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Register(m_serverManager.get());
        }

        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), GetTerminationTime()).Times(1);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding())
            .Times(1)
            .WillOnce(testing::Return(Aws::GameLift::GenericOutcome()));
        EXPECT_CALL(handlerMock, OnDestroySessionEnd()).Times(0);

        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onProcessTerminateFunc();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_FALSE(AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get());
    }

    TEST_F(GameLiftServerManagerTest, OnHealthCheck_OnSessionHealthCheckReturnsTrue_CallbackFunctionReturnsTrue)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnSessionHealthCheck()).Times(1).WillOnce(testing::Return(true));
        EXPECT_TRUE(m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_healthCheckFunc());
    }

    TEST_F(GameLiftServerManagerTest, OnHealthCheck_OnSessionHealthCheckReturnsFalseAndTrue_CallbackFunctionReturnsFalse)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock1;
        EXPECT_CALL(handlerMock1, OnSessionHealthCheck()).Times(1).WillOnce(testing::Return(false));
        SessionNotificationsHandlerMock handlerMock2;
        EXPECT_CALL(handlerMock2, OnSessionHealthCheck()).Times(1).WillOnce(testing::Return(true));
        EXPECT_FALSE(m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_healthCheckFunc());
    }

    TEST_F(GameLiftServerManagerTest, OnHealthCheck_OnSessionHealthCheckReturnsFalse_CallbackFunctionReturnsFalse)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnSessionHealthCheck()).Times(1).WillOnce(testing::Return(false));
        EXPECT_FALSE(m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_healthCheckFunc());
    }

    TEST_F(GameLiftServerManagerTest, OnStartGameSession_OnCreateSessionBeginReturnsFalse_TerminationNotificationSent)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnCreateSessionBegin(testing::_)).Times(1).WillOnce(testing::Return(false));
        EXPECT_CALL(handlerMock, OnCreateSessionEnd()).Times(0);
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(1);
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onStartGameSessionFunc(Aws::GameLift::Server::Model::GameSession());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, OnStartGameSession_ActivateGameSessionSucceeds_RegisterAsHandler)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnCreateSessionBegin(testing::_)).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(handlerMock, OnCreateSessionEnd()).Times(1);
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ActivateGameSession())
            .Times(1)
            .WillOnce(testing::Return(Aws::GameLift::GenericOutcome(nullptr)));
        Aws::GameLift::Server::Model::GameSession testSession;
        Aws::GameLift::Server::Model::GameProperty testProperty;
        testProperty.SetKey("testKey");
        testProperty.SetValue("testValue");
        testSession.AddGameProperties(testProperty);
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onStartGameSessionFunc(testSession);
        EXPECT_TRUE(AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get());
        m_serverManager->HandleDestroySession();
    }

    TEST_F(GameLiftServerManagerTest, OnStartGameSession_ActivateGameSessionFails_TerminationNotificationSent)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnCreateSessionBegin(testing::_)).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(handlerMock, OnCreateSessionEnd()).Times(0);
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ActivateGameSession())
            .Times(1)
            .WillOnce(testing::Return(Aws::GameLift::GenericOutcome()));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(1);
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onStartGameSessionFunc(Aws::GameLift::Server::Model::GameSession());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, OnUpdateGameSession_TriggerWithUnknownReason_OnUpdateSessionGetCalledOnce)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnUpdateSessionBegin(testing::_, testing::_)).Times(1);
        EXPECT_CALL(handlerMock, OnUpdateSessionEnd()).Times(1);

        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onUpdateGameSessionFunc(
            Aws::GameLift::Server::Model::UpdateGameSession(
                Aws::GameLift::Server::Model::GameSession(),
                Aws::GameLift::Server::Model::UpdateReason::UNKNOWN,
                "testticket"));
    }

    TEST_F(GameLiftServerManagerTest, OnUpdateGameSession_TriggerWithEmptyMatchmakingData_OnUpdateSessionGetCalledOnce)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnUpdateSessionBegin(testing::_, testing::_)).Times(1);
        EXPECT_CALL(handlerMock, OnUpdateSessionEnd()).Times(1);

        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onUpdateGameSessionFunc(
            Aws::GameLift::Server::Model::UpdateGameSession(
                Aws::GameLift::Server::Model::GameSession(),
                Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED,
                "testticket"));
    }

    TEST_F(GameLiftServerManagerTest, OnUpdateGameSession_TriggerWithValidMatchmakingData_OnUpdateSessionGetCalledOnce)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnUpdateSessionBegin(testing::_, testing::_)).Times(1);
        EXPECT_CALL(handlerMock, OnUpdateSessionEnd()).Times(1);

        Aws::GameLift::Server::Model::GameSession gameSession;
        gameSession.SetMatchmakerData(TEST_SERVER_MATCHMAKING_DATA);
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onUpdateGameSessionFunc(
            Aws::GameLift::Server::Model::UpdateGameSession(
                gameSession, Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED, "testticket"));
    }

    TEST_F(GameLiftServerManagerTest, OnUpdateGameSession_TriggerWithInvalidMatchmakingData_OnUpdateSessionGetCalledOnce)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady();
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnUpdateSessionBegin(testing::_, testing::_)).Times(1);
        EXPECT_CALL(handlerMock, OnUpdateSessionEnd()).Times(1);

        Aws::GameLift::Server::Model::GameSession gameSession;
        gameSession.SetMatchmakerData("{invalid}");
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onUpdateGameSessionFunc(
            Aws::GameLift::Server::Model::UpdateGameSession(
                gameSession, Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED, "testticket"));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, ValidatePlayerJoinSession_CallWithInvalidConnectionConfig_GetFalseResultAndExpectedErrorLog)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto result = m_serverManager->ValidatePlayerJoinSession(AzFramework::PlayerConnectionConfig());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(result);
    }

    TEST_F(GameLiftServerManagerTest, ValidatePlayerJoinSession_CallWithDuplicatedConnectionId_GetFalseResultAndExpectedErrorLog)
    {
        AzFramework::PlayerConnectionConfig connectionConfig1;
        connectionConfig1.m_playerConnectionId = 123;
        connectionConfig1.m_playerSessionId = "dummyPlayerSessionId1";
        GenericOutcome successOutcome(nullptr);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), AcceptPlayerSession(testing::_))
            .Times(1)
            .WillOnce(Return(successOutcome));
        m_serverManager->ValidatePlayerJoinSession(connectionConfig1);
        AzFramework::PlayerConnectionConfig connectionConfig2;
        connectionConfig2.m_playerConnectionId = 123;
        connectionConfig2.m_playerSessionId = "dummyPlayerSessionId2";
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto result = m_serverManager->ValidatePlayerJoinSession(connectionConfig2);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(result);
    }

    TEST_F(GameLiftServerManagerTest, ValidatePlayerJoinSession_CallWithValidConnectionConfigButErrorOutcome_GetFalseResultAndExpectedErrorLog)
    {
        AzFramework::PlayerConnectionConfig connectionConfig;
        connectionConfig.m_playerConnectionId = 123;
        connectionConfig.m_playerSessionId = "dummyPlayerSessionId1";
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), AcceptPlayerSession(testing::_)).Times(1);
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto result = m_serverManager->ValidatePlayerJoinSession(connectionConfig);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(result);
    }

    TEST_F(GameLiftServerManagerTest, ValidatePlayerJoinSession_CallWithValidConnectionConfigAndSuccessOutcome_GetTrueResult)
    {
        AzFramework::PlayerConnectionConfig connectionConfig;
        connectionConfig.m_playerConnectionId = 123;
        connectionConfig.m_playerSessionId = "dummyPlayerSessionId1";
        GenericOutcome successOutcome(nullptr);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), AcceptPlayerSession(testing::_))
            .Times(1)
            .WillOnce(Return(successOutcome));
        auto result = m_serverManager->ValidatePlayerJoinSession(connectionConfig);
        EXPECT_TRUE(result);
    }

    TEST_F(GameLiftServerManagerTest, ValidatePlayerJoinSession_CallWithFirstErrorSecondSuccess_GetFirstFalseSecondTrueResult)
    {
        AzFramework::PlayerConnectionConfig connectionConfig1;
        connectionConfig1.m_playerConnectionId = 123;
        connectionConfig1.m_playerSessionId = "dummyPlayerSessionId1";
        GenericOutcome successOutcome(nullptr);
        Aws::GameLift::GameLiftError error;
        GenericOutcome errorOutcome(error);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), AcceptPlayerSession(testing::_))
            .Times(2)
            .WillOnce(Return(errorOutcome))
            .WillOnce(Return(successOutcome));
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto result = m_serverManager->ValidatePlayerJoinSession(connectionConfig1);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(result);
        AzFramework::PlayerConnectionConfig connectionConfig2;
        connectionConfig2.m_playerConnectionId = 123;
        connectionConfig2.m_playerSessionId = "dummyPlayerSessionId2";
        result = m_serverManager->ValidatePlayerJoinSession(connectionConfig2);
        EXPECT_TRUE(result);
    }

    TEST_F(GameLiftServerManagerTest, ValidatePlayerJoinSession_CallWithMultithread_GetFirstTrueAndRestFalse)
    {
        int testThreadNumber = 5;
        GenericOutcome successOutcome(nullptr);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), AcceptPlayerSession(testing::_))
            .Times(1)
            .WillOnce(Return(successOutcome));
        AZStd::vector<AZStd::thread> testThreadPool;
        AZStd::atomic<int> trueCount = 0;
        for (int index = 0; index < testThreadNumber; index++)
        {
            testThreadPool.emplace_back(AZStd::thread([&]() {
                AzFramework::PlayerConnectionConfig connectionConfig;
                connectionConfig.m_playerConnectionId = 123;
                connectionConfig.m_playerSessionId = "dummyPlayerSessionId";
                auto result = m_serverManager->ValidatePlayerJoinSession(connectionConfig);
                if (result)
                {
                    trueCount++;
                }
            }));
        }
        for (auto& testThread : testThreadPool)
        {
            testThread.join();
        }
        EXPECT_TRUE(trueCount == 1);
    }

    TEST_F(GameLiftServerManagerTest, HandlePlayerLeaveSession_CallWithInvalidConnectionConfig_GetExpectedErrorLog)
    {
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), RemovePlayerSession(testing::_)).Times(0);

        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->HandlePlayerLeaveSession(AzFramework::PlayerConnectionConfig());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, HandlePlayerLeaveSession_CallWithNonExistentPlayerConnectionId_GetExpectedErrorLog)
    {
        AzFramework::PlayerConnectionConfig connectionConfig;
        connectionConfig.m_playerConnectionId = 123;
        connectionConfig.m_playerSessionId = "dummyPlayerSessionId";
        auto result = m_serverManager->AddConnectedTestPlayer(connectionConfig);
        EXPECT_TRUE(result);

        AzFramework::PlayerConnectionConfig connectionConfig1;
        connectionConfig1.m_playerConnectionId = 456;
        connectionConfig1.m_playerSessionId = "dummyPlayerSessionId";

        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), RemovePlayerSession(testing::_)).Times(0);

        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->HandlePlayerLeaveSession(connectionConfig1);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, HandlePlayerLeaveSession_CallWithValidConnectionConfigButErrorOutcome_GetExpectedErrorLog)
    {
        AzFramework::PlayerConnectionConfig connectionConfig;
        connectionConfig.m_playerConnectionId = 123;
        connectionConfig.m_playerSessionId = "dummyPlayerSessionId";
        auto result = m_serverManager->AddConnectedTestPlayer(connectionConfig);
        EXPECT_TRUE(result);

        Aws::GameLift::GameLiftError error;
        GenericOutcome errorOutcome(error);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), RemovePlayerSession(testing::_))
            .Times(1)
            .WillOnce(Return(errorOutcome));

        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->HandlePlayerLeaveSession(connectionConfig);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, HandlePlayerLeaveSession_CallWithValidConnectionConfigAndSuccessOutcome_RemovePlayerSessionNotificationSent)
    {
        AzFramework::PlayerConnectionConfig connectionConfig;
        connectionConfig.m_playerConnectionId = 123;
        connectionConfig.m_playerSessionId = "dummyPlayerSessionId";
        auto result = m_serverManager->AddConnectedTestPlayer(connectionConfig);
        EXPECT_TRUE(result);

        GenericOutcome successOutcome(nullptr);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), RemovePlayerSession(testing::_))
            .Times(1)
            .WillOnce(Return(successOutcome));

        m_serverManager->HandlePlayerLeaveSession(connectionConfig);
    }

    TEST_F(GameLiftServerManagerTest, HandlePlayerLeaveSession_CallWithMultithread_OnlyOneNotificationIsSent)
    {
        AzFramework::PlayerConnectionConfig connectionConfig;
        connectionConfig.m_playerConnectionId = 123;
        connectionConfig.m_playerSessionId = "dummyPlayerSessionId";
        auto result = m_serverManager->AddConnectedTestPlayer(connectionConfig);
        EXPECT_TRUE(result);

        int testThreadNumber = 5;
        GenericOutcome successOutcome(nullptr);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), RemovePlayerSession(testing::_))
            .Times(1)
            .WillOnce(Return(successOutcome));

        AZStd::vector<AZStd::thread> testThreadPool;
        AZ_TEST_START_TRACE_SUPPRESSION;
        for (int index = 0; index < testThreadNumber; index++)
        {
            testThreadPool.emplace_back(AZStd::thread(
                [&]()
                {
                    m_serverManager->HandlePlayerLeaveSession(connectionConfig);
                }));
        }
        for (auto& testThread : testThreadPool)
        {
            testThread.join();
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(testThreadNumber - 1); // The player is only disconnected once.
    }

    TEST_F(GameLiftServerManagerTest, UpdateGameSessionData_CallWithInvalidMatchmakingData_GetExpectedError)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->SetupTestMatchmakingData("{invalid}");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, GetActiveServerMatchBackfillPlayers_CallWithInvalidMatchmakingData_GetEmptyResult)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->SetupTestMatchmakingData("{invalid}");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        auto actualResult = m_serverManager->GetTestServerMatchBackfillPlayers();
        EXPECT_TRUE(actualResult.empty());
    }

    TEST_F(GameLiftServerManagerTest, GetActiveServerMatchBackfillPlayers_CallWithEmptyMatchmakingData_GetEmptyResult)
    {
        m_serverManager->SetupTestMatchmakingData("");

        auto actualResult = m_serverManager->GetTestServerMatchBackfillPlayers();
        EXPECT_TRUE(actualResult.empty());
    }

    TEST_F(GameLiftServerManagerTest, GetActiveServerMatchBackfillPlayers_CallButDescribePlayerError_GetEmptyResult)
    {
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        Aws::GameLift::GameLiftError error;
        Aws::GameLift::DescribePlayerSessionsOutcome errorOutcome(error);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), DescribePlayerSessions(testing::_))
            .Times(1)
            .WillOnce(Return(errorOutcome));

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->GetTestServerMatchBackfillPlayers();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_TRUE(actualResult.empty());
    }

    TEST_F(GameLiftServerManagerTest, GetActiveServerMatchBackfillPlayers_CallButNoActivePlayer_GetEmptyResult)
    {
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        Aws::GameLift::Server::Model::DescribePlayerSessionsResult result;
        Aws::GameLift::DescribePlayerSessionsOutcome successOutcome(result);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), DescribePlayerSessions(testing::_))
            .Times(1)
            .WillOnce(Return(successOutcome));

        auto actualResult = m_serverManager->GetTestServerMatchBackfillPlayers();
        EXPECT_TRUE(actualResult.empty());
    }

    TEST_F(GameLiftServerManagerTest, GetActiveServerMatchBackfillPlayers_CallWithValidMatchmakingData_GetExpectedResult)
    {
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        Aws::GameLift::Server::Model::PlayerSession playerSession;
        playerSession.SetPlayerId("testplayer");
        Aws::GameLift::Server::Model::DescribePlayerSessionsResult result;
        result.AddPlayerSessions(playerSession);
        Aws::GameLift::DescribePlayerSessionsOutcome successOutcome(result);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), DescribePlayerSessions(testing::_))
            .Times(1)
            .WillOnce(Return(successOutcome));

        auto actualResult = m_serverManager->GetTestServerMatchBackfillPlayers();
        EXPECT_TRUE(actualResult.size() == 1);
        EXPECT_TRUE(actualResult[0].m_team == "testteam");
        EXPECT_TRUE(actualResult[0].m_playerId == "testplayer");
        EXPECT_TRUE(actualResult[0].m_playerAttributes.size() == 4);
    }

    TEST_F(GameLiftServerManagerTest, GetActiveServerMatchBackfillPlayers_CallWithMultiDescribePlayerButError_GetEmptyResult)
    {
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA, 50);

        Aws::GameLift::GameLiftError error;
        Aws::GameLift::DescribePlayerSessionsOutcome errorOutcome(error);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), DescribePlayerSessions(testing::_))
            .Times(1)
            .WillOnce(Return(errorOutcome));

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->GetTestServerMatchBackfillPlayers();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_TRUE(actualResult.empty());
    }

    TEST_F(GameLiftServerManagerTest, GetActiveServerMatchBackfillPlayers_CallWithMultiDescribePlayer_GetExpectedResult)
    {
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA, 50);

        Aws::GameLift::Server::Model::PlayerSession playerSession1;
        playerSession1.SetPlayerId("testplayer");
        Aws::GameLift::Server::Model::DescribePlayerSessionsResult result1;
        result1.AddPlayerSessions(playerSession1);
        result1.SetNextToken("testtoken");
        Aws::GameLift::DescribePlayerSessionsOutcome successOutcome1(result1);

        Aws::GameLift::Server::Model::PlayerSession playerSession2;
        playerSession2.SetPlayerId("playernotinmatch");
        Aws::GameLift::Server::Model::DescribePlayerSessionsResult result2;
        result2.AddPlayerSessions(playerSession2);
        Aws::GameLift::DescribePlayerSessionsOutcome successOutcome2(result2);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), DescribePlayerSessions(testing::_))
            .WillOnce(Return(successOutcome1))
            .WillOnce(Return(successOutcome2));

        auto actualResult = m_serverManager->GetTestServerMatchBackfillPlayers();
        EXPECT_TRUE(actualResult.size() == 1);
        EXPECT_TRUE(actualResult[0].m_team == "testteam");
        EXPECT_TRUE(actualResult[0].m_playerId == "testplayer");
        EXPECT_TRUE(actualResult[0].m_playerAttributes.size() == 4);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_SDKNotInitialized_GetExpectedError)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StartMatchBackfill("testticket", {});
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_CallWithEmptyMatchmakingData_GetExpectedError)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData("");

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StartMatchBackfill("testticket", {});
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_CallWithInvalidPlayerAttribute_GetExpectedError)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        AWSGameLiftPlayer testPlayer = GetTestGameLiftPlayer();
        testPlayer.m_playerAttributes.clear();
        testPlayer.m_playerAttributes.emplace("invalidattribute", "{invalid}");

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StartMatchBackfill("testticket", { testPlayer });
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_CallWithWrongPlayerAttributeType_GetExpectedError)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        AWSGameLiftPlayer testPlayer = GetTestGameLiftPlayer();
        testPlayer.m_playerAttributes.clear();
        testPlayer.m_playerAttributes.emplace("invalidattribute", "{\"SDM\": [\"test1\"]}");

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StartMatchBackfill("testticket", { testPlayer });
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_CallWithUnexpectedPlayerAttributeType_GetExpectedError)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        AWSGameLiftPlayer testPlayer = GetTestGameLiftPlayer();
        testPlayer.m_playerAttributes.clear();
        testPlayer.m_playerAttributes.emplace("invalidattribute", "{\"UNEXPECTED\": [\"test1\"]}");

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StartMatchBackfill("testticket", { testPlayer });
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_CallWithWrongSLPlayerAttributeValue_GetExpectedError)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        AWSGameLiftPlayer testPlayer = GetTestGameLiftPlayer();
        testPlayer.m_playerAttributes.clear();
        testPlayer.m_playerAttributes.emplace("invalidattribute", "{\"SL\": [10.0]}");

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StartMatchBackfill("testticket", { testPlayer });
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_CallWithWrongSDMPlayerAttributeValue_GetExpectedError)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        AWSGameLiftPlayer testPlayer = GetTestGameLiftPlayer();
        testPlayer.m_playerAttributes.clear();
        testPlayer.m_playerAttributes.emplace("invalidattribute", "{\"SDM\": {10.0: \"test1\"}}");

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StartMatchBackfill("testticket", { testPlayer });
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_CallWithValidPlayersData_GetExpectedResult)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        Aws::GameLift::Server::Model::StartMatchBackfillResult backfillResult;
        Aws::GameLift::StartMatchBackfillOutcome backfillSuccessOutcome(backfillResult);
        Aws::GameLift::Server::Model::StartMatchBackfillRequest request = GetTestStartMatchBackfillRequest();

        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), StartMatchBackfill(StartMatchBackfillRequestMatcher(request)))
            .Times(1)
            .WillOnce(Return(backfillSuccessOutcome));

        AWSGameLiftPlayer testPlayer = GetTestGameLiftPlayer();
        auto actualResult = m_serverManager->StartMatchBackfill("testticket", {testPlayer});
        EXPECT_TRUE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_CallWithoutGivingPlayersData_GetExpectedResult)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        Aws::GameLift::Server::Model::PlayerSession playerSession;
        playerSession.SetPlayerId("testplayer");
        Aws::GameLift::Server::Model::DescribePlayerSessionsResult result;
        result.AddPlayerSessions(playerSession);
        Aws::GameLift::DescribePlayerSessionsOutcome successOutcome(result);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), DescribePlayerSessions(testing::_))
            .Times(1)
            .WillOnce(Return(successOutcome));

        Aws::GameLift::Server::Model::StartMatchBackfillResult backfillResult;
        Aws::GameLift::StartMatchBackfillOutcome backfillSuccessOutcome(backfillResult);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), StartMatchBackfill(testing::_))
            .Times(1)
            .WillOnce(Return(backfillSuccessOutcome));

        auto actualResult = m_serverManager->StartMatchBackfill("testticket", {});
        EXPECT_TRUE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StartMatchBackfill_CallButStartBackfillFail_GetExpectedError)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        Aws::GameLift::Server::Model::PlayerSession playerSession;
        playerSession.SetPlayerId("testplayer");
        Aws::GameLift::Server::Model::DescribePlayerSessionsResult result;
        result.AddPlayerSessions(playerSession);
        Aws::GameLift::DescribePlayerSessionsOutcome successOutcome(result);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), DescribePlayerSessions(testing::_))
            .Times(1)
            .WillOnce(Return(successOutcome));

        Aws::GameLift::GameLiftError error;
        Aws::GameLift::StartMatchBackfillOutcome errorOutcome(error);
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), StartMatchBackfill(testing::_))
            .Times(1)
            .WillOnce(Return(errorOutcome));

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StartMatchBackfill("testticket", {});
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StopMatchBackfill_SDKNotInitialized_GetExpectedError)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StopMatchBackfill("testticket");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StopMatchBackfill_CallWithEmptyMatchmakingData_GetExpectedError)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData("");

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StopMatchBackfill("testticket");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StopMatchBackfill_CallAndSuccessOutcome_GetExpectedResult)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), StopMatchBackfill(testing::_))
            .Times(1)
            .WillOnce(Return(Aws::GameLift::GenericOutcome(nullptr)));

        auto actualResult = m_serverManager->StopMatchBackfill("testticket");
        EXPECT_TRUE(actualResult);
    }

    TEST_F(GameLiftServerManagerTest, StopMatchBackfill_CallButErrorOutcome_GetExpectedError)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->SetupTestMatchmakingData(TEST_SERVER_MATCHMAKING_DATA);

        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), StopMatchBackfill(testing::_))
            .Times(1)
            .WillOnce(Return(Aws::GameLift::GenericOutcome()));

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = m_serverManager->StopMatchBackfill("testticket");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(actualResult);
    }
} // namespace UnitTest
