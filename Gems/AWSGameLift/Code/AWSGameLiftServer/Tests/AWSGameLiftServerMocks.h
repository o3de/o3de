/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AWSGameLiftPlayer.h>
#include <AWSGameLiftServerSystemComponent.h>
#include <AWSGameLiftServerManager.h>
#include <GameLiftServerSDKWrapper.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>

using namespace Aws::GameLift;
using namespace AWSGameLift;
using testing::_;
using testing::Invoke;
using testing::Return;
using testing::NiceMock;
using testing::Eq;

namespace UnitTest
{
    class GameLiftServerSDKWrapperMock
        : public GameLiftServerSDKWrapper
    {
    public:
        GameLiftServerSDKWrapperMock()
        {
            GenericOutcome successOutcome(nullptr);
            Server::InitSDKOutcome sdkOutcome(nullptr);

            ON_CALL(*this, InitSDK()).WillByDefault(Return(sdkOutcome));
            ON_CALL(*this, ProcessReady(_)).WillByDefault(Invoke(this, &GameLiftServerSDKWrapperMock::ProcessReadyMock));
            ON_CALL(*this, ProcessEnding()).WillByDefault(Return(successOutcome));
        }

        MOCK_METHOD1(AcceptPlayerSession, GenericOutcome(const std::string&));
        MOCK_METHOD0(ActivateGameSession, GenericOutcome());
        MOCK_METHOD1(DescribePlayerSessions, DescribePlayerSessionsOutcome(
            const Aws::GameLift::Server::Model::DescribePlayerSessionsRequest&));
        MOCK_METHOD0(InitSDK, Server::InitSDKOutcome());
        MOCK_METHOD1(ProcessReady, GenericOutcome(const Server::ProcessParameters& processParameters));
        MOCK_METHOD0(ProcessEnding, GenericOutcome());
        MOCK_METHOD1(RemovePlayerSession, GenericOutcome(const AZStd::string& playerSessionId));
        MOCK_METHOD0(GetTerminationTime, AZStd::string());
        MOCK_METHOD1(StartMatchBackfill, StartMatchBackfillOutcome(
            const Aws::GameLift::Server::Model::StartMatchBackfillRequest&));
        MOCK_METHOD1(StopMatchBackfill, GenericOutcome(
            const Aws::GameLift::Server::Model::StopMatchBackfillRequest&));


        GenericOutcome ProcessReadyMock(const Server::ProcessParameters& processParameters)
        {
            m_healthCheckFunc = processParameters.getOnHealthCheck();
            m_onStartGameSessionFunc = processParameters.getOnStartGameSession();
            m_onProcessTerminateFunc = processParameters.getOnProcessTerminate();
            m_onUpdateGameSessionFunc = processParameters.getOnUpdateGameSession();

            GenericOutcome successOutcome(nullptr);
            return successOutcome;
        }

        AZStd::function<bool()> m_healthCheckFunc;
        AZStd::function<void()> m_onProcessTerminateFunc;
        AZStd::function<void(Aws::GameLift::Server::Model::GameSession)> m_onStartGameSessionFunc;
        AZStd::function<void(Aws::GameLift::Server::Model::UpdateGameSession)> m_onUpdateGameSessionFunc;
    };

    class AWSGameLiftServerManagerMock
        : public AWSGameLiftServerManager
    {
    public:
        AWSGameLiftServerManagerMock()
        {
            AZStd::unique_ptr<NiceMock<GameLiftServerSDKWrapperMock>> gameLiftServerSDKWrapper =
                AZStd::make_unique<NiceMock<GameLiftServerSDKWrapperMock>>();
            m_gameLiftServerSDKWrapperMockPtr = gameLiftServerSDKWrapper.get();
            SetGameLiftServerSDKWrapper(AZStd::move(gameLiftServerSDKWrapper));
        }

        ~AWSGameLiftServerManagerMock()
        {
            m_gameLiftServerSDKWrapperMockPtr = nullptr;
        }

        void SetupTestMatchmakingData(const AZStd::string& matchmakingData, int maxPlayer = 10)
        {
            m_testGameSession.SetMatchmakerData(matchmakingData.c_str());
            m_testGameSession.SetMaximumPlayerSessionCount(maxPlayer);
            UpdateGameSessionData(m_testGameSession);
        }

        bool AddConnectedTestPlayer(const AzFramework::PlayerConnectionConfig& playerConnectionConfig)
        {
            return AddConnectedPlayer(playerConnectionConfig);
        }

        AZStd::vector<AWSGameLiftPlayer> GetTestServerMatchBackfillPlayers()
        {
            return GetActiveServerMatchBackfillPlayers();
        }

        NiceMock<GameLiftServerSDKWrapperMock>* m_gameLiftServerSDKWrapperMockPtr;
        Aws::GameLift::Server::Model::GameSession m_testGameSession;
    };

    class AWSGameLiftServerSystemComponentMock
        : public AWSGameLift::AWSGameLiftServerSystemComponent
    {
    public:
        AWSGameLiftServerSystemComponentMock()
        {
            SetGameLiftServerManager(AZStd::make_unique<NiceMock<AWSGameLiftServerManagerMock>>());

            ON_CALL(*this, Init()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::InitMock));
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::ActivateMock));
            ON_CALL(*this, Deactivate()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::DeactivateMock));
        }

        void InitMock()
        {
            AWSGameLift::AWSGameLiftServerSystemComponent::Init();
        }

        void ActivateMock()
        {
            AWSGameLift::AWSGameLiftServerSystemComponent::Activate();
        }

        void DeactivateMock()
        {
            AWSGameLift::AWSGameLiftServerSystemComponent::Deactivate();
        }

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());

        GameLiftServerProcessDesc m_serverProcessDesc;
    };
};
