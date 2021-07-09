/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Session/SessionConfig.h>
#include <Credential/AWSCredentialBus.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <AWSCoreBus.h>
#include <AWSGameLiftClientFixture.h>
#include <AWSGameLiftClientManager.h>
#include <AWSGameLiftClientMocks.h>

#include <Request/AWSGameLiftCreateSessionOnQueueRequest.h>
#include <Request/AWSGameLiftCreateSessionRequest.h>
#include <Request/AWSGameLiftJoinSessionRequest.h>
#include <Request/AWSGameLiftSearchSessionsRequest.h>

#include <aws/gamelift/GameLiftClient.h>

using namespace AWSGameLift;

MATCHER_P(SearchSessionsResponseMatcher, expectedResponse, "")
{
    // Custome matcher for checking the SearchSessionsResponse type argument.
    AZ_UNUSED(result_listener);

    bool result = arg.m_nextToken == expectedResponse.m_nextToken;
    result &= arg.m_sessionConfigs.size() == expectedResponse.m_sessionConfigs.size();

    for (int index = 0; index < arg.m_sessionConfigs.size(); ++index)
    {
        result &= arg.m_sessionConfigs[index].m_creationTime == expectedResponse.m_sessionConfigs[index].m_creationTime;
        result &= arg.m_sessionConfigs[index].m_terminationTime == expectedResponse.m_sessionConfigs[index].m_terminationTime;
        result &= arg.m_sessionConfigs[index].m_creatorId == expectedResponse.m_sessionConfigs[index].m_creatorId;
        result &= arg.m_sessionConfigs[index].m_sessionProperties == expectedResponse.m_sessionConfigs[index].m_sessionProperties;
        result &= arg.m_sessionConfigs[index].m_sessionId == expectedResponse.m_sessionConfigs[index].m_sessionId;
        result &= arg.m_sessionConfigs[index].m_sessionName == expectedResponse.m_sessionConfigs[index].m_sessionName;
        result &= arg.m_sessionConfigs[index].m_dnsName == expectedResponse.m_sessionConfigs[index].m_dnsName;
        result &= arg.m_sessionConfigs[index].m_ipAddress == expectedResponse.m_sessionConfigs[index].m_ipAddress;
        result &= arg.m_sessionConfigs[index].m_port == expectedResponse.m_sessionConfigs[index].m_port;
        result &= arg.m_sessionConfigs[index].m_maxPlayer == expectedResponse.m_sessionConfigs[index].m_maxPlayer;
        result &= arg.m_sessionConfigs[index].m_currentPlayer == expectedResponse.m_sessionConfigs[index].m_currentPlayer;
        result &= arg.m_sessionConfigs[index].m_status == expectedResponse.m_sessionConfigs[index].m_status;
        result &= arg.m_sessionConfigs[index].m_statusReason == expectedResponse.m_sessionConfigs[index].m_statusReason;
    }

    return result;
}

class AWSResourceMappingRequestsHandlerMock
    : public AWSCore::AWSResourceMappingRequestBus::Handler
{
public:
    AWSResourceMappingRequestsHandlerMock()
    {
        AWSCore::AWSResourceMappingRequestBus::Handler::BusConnect();
    }

    ~ AWSResourceMappingRequestsHandlerMock()
    {
        AWSCore::AWSResourceMappingRequestBus::Handler::BusDisconnect();
    }

    MOCK_CONST_METHOD0(GetDefaultRegion, AZStd::string());
    MOCK_CONST_METHOD0(GetDefaultAccountId, AZStd::string());
    MOCK_CONST_METHOD1(GetResourceAccountId, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD1(GetResourceNameId, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD1(GetResourceRegion, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD1(GetResourceType, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD1(GetServiceUrlByServiceName, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD2(GetServiceUrlByRESTApiIdAndStage, AZStd::string(const AZStd::string&, const AZStd::string&));
    MOCK_METHOD1(ReloadConfigFile, void(bool));
};

class AWSCredentialRequestsHandlerMock
    : public AWSCore::AWSCredentialRequestBus::Handler
{
public:
    AWSCredentialRequestsHandlerMock()
    {
        AWSCore::AWSCredentialRequestBus::Handler::BusConnect();
    }

    ~AWSCredentialRequestsHandlerMock()
    {
        AWSCore::AWSCredentialRequestBus::Handler::BusDisconnect();
    }

    MOCK_CONST_METHOD0(GetCredentialHandlerOrder, int());
    MOCK_METHOD0(GetCredentialsProvider, std::shared_ptr<Aws::Auth::AWSCredentialsProvider>());
};

class AWSCoreRequestsHandlerMock
    : public AWSCore::AWSCoreRequestBus::Handler
{
public:
    AWSCoreRequestsHandlerMock()
    {
        AWSCore::AWSCoreRequestBus::Handler::BusConnect();
    }

    ~AWSCoreRequestsHandlerMock()
    {
        AWSCore::AWSCoreRequestBus::Handler::BusDisconnect();
    }

    MOCK_METHOD0(GetDefaultJobContext, AZ::JobContext*());
    MOCK_METHOD0(GetDefaultConfig, AWSCore::AwsApiJobConfig*());
};

class TestAWSGameLiftClientManager
    : public AWSGameLiftClientManager
{
public:
    TestAWSGameLiftClientManager()
    {
        m_gameliftClientMockPtr = nullptr;
    }
    ~TestAWSGameLiftClientManager()
    {
        m_gameliftClientMockPtr = nullptr;
    }

    void SetUpMockClient()
    {
        m_gameliftClientMockPtr = AZStd::make_shared<GameLiftClientMock>();
        SetGameLiftClient(m_gameliftClientMockPtr);
    }

    AZStd::shared_ptr<GameLiftClientMock> m_gameliftClientMockPtr;
};

class AWSGameLiftClientManagerTest
    : public AWSGameLiftClientFixture
{
protected:
    void SetUp() override
    {
        AWSGameLiftClientFixture::SetUp();

        m_gameliftClientManager = AZStd::make_unique<TestAWSGameLiftClientManager>();
        m_gameliftClientManager->SetUpMockClient();
        m_gameliftClientManager->ActivateManager();
    }

    void TearDown() override
    {
        m_gameliftClientManager->DeactivateManager();
        m_gameliftClientManager.reset();

        AWSGameLiftClientFixture::TearDown();
    }

    AWSGameLiftSearchSessionsRequest GetValidSearchSessionsRequest()
    {
        AWSGameLiftSearchSessionsRequest request;
        request.m_aliasId = "dummyAliasId";
        request.m_fleetId = "dummyFleetId";
        request.m_location = "dummyLocation";
        request.m_filterExpression = "dummyFilterExpression";
        request.m_sortExpression = "dummySortExpression";
        request.m_maxResult = 1;
        request.m_nextToken = "dummyNextToken";

        return request;
    }

    Aws::GameLift::Model::SearchGameSessionsOutcome GetValidSearchGameSessionsOutcome()
    {
        Aws::GameLift::Model::GameProperty gameProperty;
        gameProperty.SetKey("dummyKey");
        gameProperty.SetValue("dummyValue");
        Aws::Vector<Aws::GameLift::Model::GameProperty> gameProperties = { gameProperty };

        Aws::GameLift::Model::GameSession gameSession;
        gameSession.SetCreationTime(Aws::Utils::DateTime(0.0));
        gameSession.SetTerminationTime(Aws::Utils::DateTime(0.0));
        gameSession.SetCreatorId("dummyCreatorId");
        gameSession.SetGameProperties(gameProperties);
        gameSession.SetGameSessionId("dummyGameSessionId");
        gameSession.SetName("dummyGameSessionName");
        gameSession.SetIpAddress("dummyIpAddress");
        gameSession.SetPort(0);
        gameSession.SetMaximumPlayerSessionCount(2);
        gameSession.SetCurrentPlayerSessionCount(1);
        gameSession.SetStatus(Aws::GameLift::Model::GameSessionStatus::TERMINATED);
        gameSession.SetStatusReason(Aws::GameLift::Model::GameSessionStatusReason::INTERRUPTED);
        // TODO: Update the AWS Native SDK to set the new game session attributes.
        // gameSession.SetDnsName("dummyDnsName");

        Aws::GameLift::Model::SearchGameSessionsResult result;
        result.SetNextToken("dummyNextToken");
        result.SetGameSessions({ gameSession });

        return Aws::GameLift::Model::SearchGameSessionsOutcome(result);
    }

    AzFramework::SearchSessionsResponse GetValidSearchSessionsResponse()
    {
        AzFramework::SessionConfig sessionConfig;
        sessionConfig.m_creationTime = 0;
        sessionConfig.m_terminationTime = 0;
        sessionConfig.m_creatorId = "dummyCreatorId";
        sessionConfig.m_sessionProperties["dummyKey"] = "dummyValue";
        sessionConfig.m_sessionId = "dummyGameSessionId";
        sessionConfig.m_sessionName = "dummyGameSessionName";
        sessionConfig.m_ipAddress = "dummyIpAddress";
        sessionConfig.m_port = 0;
        sessionConfig.m_maxPlayer = 2;
        sessionConfig.m_currentPlayer = 1;
        sessionConfig.m_status = "Terminated";
        sessionConfig.m_statusReason = "Interrupted";
        // TODO: Update the AWS Native SDK to set the new game session attributes.
        // sessionConfig.m_dnsName = "dummyDnsName";

        AzFramework::SearchSessionsResponse response;
        response.m_nextToken = "dummyNextToken";
        response.m_sessionConfigs = { sessionConfig };

        return response;
    }

public:
    AZStd::unique_ptr<TestAWSGameLiftClientManager> m_gameliftClientManager;
};

TEST_F(AWSGameLiftClientManagerTest, ConfigureGameLiftClient_CallWithoutRegion_GetFalseAsResult)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = m_gameliftClientManager->ConfigureGameLiftClient("");
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftClientManagerTest, ConfigureGameLiftClient_CallWithoutCredential_GetFalseAsResult)
{
    AWSResourceMappingRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultRegion()).Times(1).WillOnce(::testing::Return("us-west-2"));
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = m_gameliftClientManager->ConfigureGameLiftClient("");
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftClientManagerTest, ConfigureGameLiftClient_CallWithRegionAndCredential_GetTrueAsResult)
{
    AWSCredentialRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetCredentialsProvider())
        .Times(1)
        .WillOnce(::testing::Return(std::make_shared<Aws::Auth::SimpleAWSCredentialsProvider>("dummyAccess", "dummySecret", "")));
    auto result = m_gameliftClientManager->ConfigureGameLiftClient("us-west-2");
    EXPECT_TRUE(result);
}

TEST_F(AWSGameLiftClientManagerTest, CreatePlayerId_CreateWithoutBracketsOrDashes_GetExpectedResult)
{
    auto result = m_gameliftClientManager->CreatePlayerId(false, false);
    EXPECT_FALSE(result.starts_with("{"));
    EXPECT_FALSE(result.ends_with("}"));
    EXPECT_FALSE(result.contains("-"));
}

TEST_F(AWSGameLiftClientManagerTest, CreatePlayerId_CreateWithBrackets_GetExpectedResult)
{
    auto result = m_gameliftClientManager->CreatePlayerId(true, false);
    EXPECT_TRUE(result.starts_with("{"));
    EXPECT_TRUE(result.ends_with("}"));
    EXPECT_FALSE(result.contains("-"));
}

TEST_F(AWSGameLiftClientManagerTest, CreatePlayerId_CreateWithDashes_GetExpectedResult)
{
    auto result = m_gameliftClientManager->CreatePlayerId(false, true);
    EXPECT_FALSE(result.starts_with("{"));
    EXPECT_FALSE(result.ends_with("}"));
    EXPECT_TRUE(result.contains("-"));
}

TEST_F(AWSGameLiftClientManagerTest, CreatePlayerId_CreateWithBracketsAndDashes_GetExpectedResult)
{
    auto result = m_gameliftClientManager->CreatePlayerId(true, true);
    EXPECT_TRUE(result.starts_with("{"));
    EXPECT_TRUE(result.ends_with("}"));
    EXPECT_TRUE(result.contains("-"));
}

TEST_F(AWSGameLiftClientManagerTest, CreateSession_CallWithoutClientSetup_GetEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->ConfigureGameLiftClient("");
    AWSGameLiftCreateSessionRequest request;
    request.m_aliasId = "dummyAlias";
    auto response = m_gameliftClientManager->CreateSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(2); // capture 2 error message
    EXPECT_TRUE(response == "");
}

TEST_F(AWSGameLiftClientManagerTest, CreateSession_CallWithInvalidRequest_GetEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto response = m_gameliftClientManager->CreateSession(AzFramework::CreateSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_TRUE(response == "");
}

TEST_F(AWSGameLiftClientManagerTest, CreateSession_CallWithValidRequest_GetSuccessOutcome)
{
    AWSGameLiftCreateSessionRequest request;
    request.m_aliasId = "dummyAlias";
    Aws::GameLift::Model::CreateGameSessionResult result;
    result.SetGameSession(Aws::GameLift::Model::GameSession());
    Aws::GameLift::Model::CreateGameSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreateGameSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    m_gameliftClientManager->CreateSession(request);
}

TEST_F(AWSGameLiftClientManagerTest, CreateSession_CallWithValidRequest_GetErrorOutcome)
{
    AWSGameLiftCreateSessionRequest request;
    request.m_aliasId = "dummyAlias";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::CreateGameSessionOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreateGameSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->CreateSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionAsync_CallWithInvalidRequest_GetNotificationWithEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnCreateSessionAsyncComplete(AZStd::string())).Times(1);
    m_gameliftClientManager->CreateSessionAsync(AzFramework::CreateSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionAsync_CallWithValidRequest_GetNotificationWithSuccessOutcome)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftCreateSessionRequest request;
    request.m_aliasId = "dummyAlias";
    Aws::GameLift::Model::CreateGameSessionResult result;
    result.SetGameSession(Aws::GameLift::Model::GameSession());
    Aws::GameLift::Model::CreateGameSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreateGameSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnCreateSessionAsyncComplete(::testing::_)).Times(1);
    m_gameliftClientManager->CreateSessionAsync(request);
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionAsync_CallWithValidRequest_GetNotificationWithErrorOutcome)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftCreateSessionRequest request;
    request.m_aliasId = "dummyAlias";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::CreateGameSessionOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreateGameSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnCreateSessionAsyncComplete(AZStd::string(""))).Times(1);
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->CreateSessionAsync(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionOnQueue_CallWithoutClientSetup_GetEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->ConfigureGameLiftClient("");
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_queueName = "dummyQueue";
    request.m_placementId = "dummyPlacementId";
    auto response = m_gameliftClientManager->CreateSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(2); // capture 2 error message
    EXPECT_TRUE(response == "");
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionOnQueue_CallWithValidRequest_GetSuccessOutcome)
{
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_queueName = "dummyQueue";
    request.m_placementId = "dummyPlacementId";
    Aws::GameLift::Model::StartGameSessionPlacementResult result;
    result.SetGameSessionPlacement(Aws::GameLift::Model::GameSessionPlacement());
    Aws::GameLift::Model::StartGameSessionPlacementOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), StartGameSessionPlacement(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    m_gameliftClientManager->CreateSession(request);
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionOnQueue_CallWithValidRequest_GetErrorOutcome)
{
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_queueName = "dummyQueue";
    request.m_placementId = "dummyPlacementId";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::StartGameSessionPlacementOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), StartGameSessionPlacement(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->CreateSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionOnQueueAsync_CallWithValidRequest_GetNotificationWithSuccessOutcome)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_queueName = "dummyQueue";
    request.m_placementId = "dummyPlacementId";
    Aws::GameLift::Model::StartGameSessionPlacementResult result;
    result.SetGameSessionPlacement(Aws::GameLift::Model::GameSessionPlacement());
    Aws::GameLift::Model::StartGameSessionPlacementOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), StartGameSessionPlacement(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnCreateSessionAsyncComplete(::testing::_)).Times(1);
    m_gameliftClientManager->CreateSessionAsync(request);
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionOnQueueAsync_CallWithValidRequest_GetNotificationWithErrorOutcome)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_queueName = "dummyQueue";
    request.m_placementId = "dummyPlacementId";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::StartGameSessionPlacementOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), StartGameSessionPlacement(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnCreateSessionAsyncComplete(AZStd::string(""))).Times(1);
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->CreateSessionAsync(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithoutClientSetup_GetFalseResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->ConfigureGameLiftClient("");
    AWSGameLiftJoinSessionRequest request;
    request.m_playerId = "dummyPlayerId";
    request.m_sessionId = "dummySessionId";
    auto response = m_gameliftClientManager->JoinSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(2); // capture 2 error message
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithInvalidRequest_GetFalseResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto response = m_gameliftClientManager->JoinSession(AzFramework::JoinSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithValidRequestButNoRequestHandler_GetSuccessOutcomeButFalseResponse)
{
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto response = m_gameliftClientManager->JoinSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithValidRequest_GetErrorOutcomeAndFalseResponse)
{
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto response = m_gameliftClientManager->JoinSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithValidRequestAndRequestHandler_GetSuccessOutcomeButFalseResponse)
{
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerJoinSession(::testing::_)).Times(1).WillOnce(::testing::Return(false));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    auto response = m_gameliftClientManager->JoinSession(request);
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithValidRequestAndRequestHandler_GetSuccessOutcomeAndTrueResponse)
{
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerJoinSession(::testing::_)).Times(1).WillOnce(::testing::Return(true));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    auto response = m_gameliftClientManager->JoinSession(request);
    EXPECT_TRUE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithInvalidRequest_GetNotificationWithFalseResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(false)).Times(1);
    m_gameliftClientManager->JoinSessionAsync(AzFramework::JoinSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithValidRequestButNoRequestHandler_GetSuccessOutcomeButNotificationWithFalseResponse)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(false)).Times(1);
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->JoinSessionAsync(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithValidRequest_GetErrorOutcomeAndNotificationWithFalseResponse)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(false)).Times(1);
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->JoinSessionAsync(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithValidRequestAndRequestHandler_GetSuccessOutcomeButNotificationWithFalseResponse)
{
    AWSCoreRequestsHandlerMock coreHandlerMock;
    EXPECT_CALL(coreHandlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerJoinSession(::testing::_)).Times(1).WillOnce(::testing::Return(false));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(false)).Times(1);
    m_gameliftClientManager->JoinSessionAsync(request);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithValidRequestAndRequestHandler_GetSuccessOutcomeAndNotificationWithTrueResponse)
{
    AWSCoreRequestsHandlerMock coreHandlerMock;
    EXPECT_CALL(coreHandlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerJoinSession(::testing::_)).Times(1).WillOnce(::testing::Return(true));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(true)).Times(1);
    m_gameliftClientManager->JoinSessionAsync(request);
}

TEST_F(AWSGameLiftClientManagerTest, SearchSessions_CallWithValidRequestAndErrorOutcome_GetErrorWithEmptyResponse)
{
    AWSGameLiftSearchSessionsRequest request = GetValidSearchSessionsRequest();

    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::SearchGameSessionsOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), SearchGameSessions(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = m_gameliftClientManager->SearchSessions(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_TRUE(result.m_sessionConfigs.size() == 0);
}

TEST_F(AWSGameLiftClientManagerTest, SearchSessions_CallWithValidRequestAndSuccessOutcome_GetNotificationWithValidResponse)
{
    AWSGameLiftSearchSessionsRequest request = GetValidSearchSessionsRequest();

    Aws::GameLift::Model::SearchGameSessionsOutcome outcome = GetValidSearchGameSessionsOutcome();
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), SearchGameSessions(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    AzFramework::SearchSessionsResponse expectedResponse = GetValidSearchSessionsResponse();
    auto result = m_gameliftClientManager->SearchSessions(request);
    EXPECT_TRUE(result.m_sessionConfigs.size() != 0);
}

TEST_F(AWSGameLiftClientManagerTest, SearchSessionsAsync_CallWithoutClientSetup_GetErrorWithEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    EXPECT_FALSE(m_gameliftClientManager->ConfigureGameLiftClient(""));
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message

    AWSGameLiftSearchSessionsRequest request = GetValidSearchSessionsRequest();

    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock,
        OnSearchSessionsAsyncComplete(SearchSessionsResponseMatcher(AzFramework::SearchSessionsResponse()))).Times(1);

    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->SearchSessionsAsync(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, SearchSessionsAsync_CallWithInvalidRequest_GetErrorWithEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock,
        OnSearchSessionsAsyncComplete(SearchSessionsResponseMatcher(AzFramework::SearchSessionsResponse()))).Times(1);

    m_gameliftClientManager->SearchSessionsAsync(AzFramework::SearchSessionsRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, SearchSessionsAsync_CallWithValidRequestAndErrorOutcome_GetErrorWithEmptyResponse)
{
    AWSGameLiftSearchSessionsRequest request = GetValidSearchSessionsRequest();

    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));

    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::SearchGameSessionsOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), SearchGameSessions(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock,
        OnSearchSessionsAsyncComplete(SearchSessionsResponseMatcher(AzFramework::SearchSessionsResponse()))).Times(1);

    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->SearchSessionsAsync(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, SearchSessionsAsync_CallWithValidRequestAndSuccessOutcome_GetNotificationWithValidResponse)
{
    AWSGameLiftSearchSessionsRequest request = GetValidSearchSessionsRequest();

    AWSCoreRequestsHandlerMock coreHandlerMock;
    EXPECT_CALL(coreHandlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));

    Aws::GameLift::Model::SearchGameSessionsOutcome outcome = GetValidSearchGameSessionsOutcome();
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), SearchGameSessions(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));

    AzFramework::SearchSessionsResponse expectedResponse = GetValidSearchSessionsResponse();
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock,
        OnSearchSessionsAsyncComplete(SearchSessionsResponseMatcher(expectedResponse))).Times(1);

    m_gameliftClientManager->SearchSessionsAsync(request);
}

TEST_F(AWSGameLiftClientManagerTest, LeaveSession_CallWithInterfaceNotRegistered_GetExpectedError)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->LeaveSession();
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, LeaveSession_CallWithInterfaceRegistered_LeaveSessionRequestSent)
{
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerLeaveSession).Times(1);

    m_gameliftClientManager->LeaveSession();
}

TEST_F(AWSGameLiftClientManagerTest, LeaveSessionAsync_CallWithInterfaceNotRegistered_GetExpectedError)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->LeaveSessionAsync();
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, LeaveSessionAsync_CallWithInterfaceRegistered_LeaveSessionAsyncRequestSentAndGetNotification)
{
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerLeaveSession).Times(1);
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnLeaveSessionAsyncComplete()).Times(1);

    m_gameliftClientManager->LeaveSessionAsync();
}
