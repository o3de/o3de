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
        MOCK_METHOD0(OnDestroySessionBegin, bool());
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
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(1);

        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onProcessTerminateFunc();

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
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ActivateGameSession())
            .Times(1)
            .WillOnce(testing::Return(Aws::GameLift::GenericOutcome()));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(1);
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onStartGameSessionFunc(Aws::GameLift::Server::Model::GameSession());
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
} // namespace UnitTest
