/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Session/ISessionRequests.h>
#include <AzFramework/Session/ISessionHandlingRequests.h>
#include <AzTest/AzTest.h>

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/GameLiftClient.h>
#include <aws/gamelift/GameLiftErrors.h>
#include <aws/gamelift/model/CreateGameSessionRequest.h>
#include <aws/gamelift/model/CreateGameSessionResult.h>
#include <aws/gamelift/model/CreatePlayerSessionRequest.h>
#include <aws/gamelift/model/CreatePlayerSessionResult.h>
#include <aws/gamelift/model/DescribeMatchmakingRequest.h>
#include <aws/gamelift/model/DescribeMatchmakingResult.h>
#include <aws/gamelift/model/SearchGameSessionsRequest.h>
#include <aws/gamelift/model/SearchGameSessionsResult.h>
#include <aws/gamelift/model/StartGameSessionPlacementRequest.h>
#include <aws/gamelift/model/StartGameSessionPlacementResult.h>

using namespace Aws::GameLift;

class GameLiftClientMock
    : public GameLiftClient
{
public:
    GameLiftClientMock()
        : GameLiftClient(Aws::Auth::AWSCredentials())
    {
    }

    MOCK_CONST_METHOD1(CreateGameSession, Model::CreateGameSessionOutcome(const Model::CreateGameSessionRequest&));
    MOCK_CONST_METHOD1(CreatePlayerSession, Model::CreatePlayerSessionOutcome(const Model::CreatePlayerSessionRequest&));
    MOCK_CONST_METHOD1(DescribeMatchmaking, Model::DescribeMatchmakingOutcome(const Model::DescribeMatchmakingRequest&));
    MOCK_CONST_METHOD1(SearchGameSessions, Model::SearchGameSessionsOutcome(const Model::SearchGameSessionsRequest&));
    MOCK_CONST_METHOD1(StartGameSessionPlacement, Model::StartGameSessionPlacementOutcome(const Model::StartGameSessionPlacementRequest&));
};

class SessionAsyncRequestNotificationsHandlerMock
    : public AzFramework::SessionAsyncRequestNotificationBus::Handler
{
public:
    SessionAsyncRequestNotificationsHandlerMock()
    {
        AzFramework::SessionAsyncRequestNotificationBus::Handler::BusConnect();
    }

    ~SessionAsyncRequestNotificationsHandlerMock()
    {
        AzFramework::SessionAsyncRequestNotificationBus::Handler::BusDisconnect();
    }

    MOCK_METHOD1(OnCreateSessionAsyncComplete, void(const AZStd::string&));
    MOCK_METHOD1(OnSearchSessionsAsyncComplete, void(const AzFramework::SearchSessionsResponse&));
    MOCK_METHOD1(OnJoinSessionAsyncComplete, void(bool));
    MOCK_METHOD0(OnLeaveSessionAsyncComplete, void());
};

class SessionHandlingClientRequestsMock
    : public AzFramework::ISessionHandlingClientRequests
{
public:
    SessionHandlingClientRequestsMock()
    {
        AZ::Interface<AzFramework::ISessionHandlingClientRequests>::Register(this);
    }

    virtual ~SessionHandlingClientRequestsMock()
    {
        AZ::Interface<AzFramework::ISessionHandlingClientRequests>::Unregister(this);
    }

    MOCK_METHOD1(RequestPlayerJoinSession, bool(const AzFramework::SessionConnectionConfig&));
    MOCK_METHOD0(RequestPlayerLeaveSession, void());
};
