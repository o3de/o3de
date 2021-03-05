/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#if defined(BUILD_GAMELIFT_CLIENT)

#include <GameLift/Session/GameLiftGameSessionPlacementRequest.h>
#include <GameLift/Session/GameLiftClientService.h>

#include <aws/core/utils/Outcome.h>

// To avoid the warning below
// Semaphore.h(50): warning C4251: 'Aws::Utils::Threading::Semaphore::m_mutex': class 'std::mutex' needs to have dll-interface to be used by clients of class 'Aws::Utils::Threading::Semaphore'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <aws/gamelift/model/CreateGameSessionRequest.h>
#include <aws/gamelift/model/StartGameSessionPlacementRequest.h>
#include <aws/gamelift/model/StartGameSessionPlacementResult.h>
#include <aws/gamelift/model/GameSessionPlacementState.h>
#include <aws/gamelift/model/DescribeGameSessionDetailsRequest.h>
#include <aws/gamelift/model/DescribeGameSessionPlacementRequest.h>
AZ_POP_DISABLE_WARNING

namespace GridMate
{
    GameLiftGameSessionPlacementRequest::GameLiftGameSessionPlacementRequest(GameLiftClientService* service, const AZStd::shared_ptr<GameLiftRequestInterfaceContext> context)
        : GameLiftSearch(service, context)
    {
        m_isDone = true;
        m_queueSessionState = GameSessionPlacementState::Unknown;
    }

    void GameLiftGameSessionPlacementRequest::AbortSearch()
    {
        SearchDone();
    }

    bool GameLiftGameSessionPlacementRequest::Initialize()
    {
        if (m_queueSessionState != GameSessionPlacementState::Unknown)
        {
            return false;
        }

        m_queueSessionState = GameSessionPlacementState::StartPlacement;
        Aws::Vector<Aws::GameLift::Model::GameProperty> gameProperties;
        for (AZStd::size_t paramIndex = 0; paramIndex < m_context->m_requestParams.m_numParams; ++paramIndex)
        {
            Aws::GameLift::Model::GameProperty prop;
            prop.SetKey(m_context->m_requestParams.m_params[paramIndex].m_id.c_str());
            prop.SetValue(m_context->m_requestParams.m_params[paramIndex].m_value.c_str());
            gameProperties.push_back(prop);
        }

        Aws::GameLift::Model::StartGameSessionPlacementRequest placementRequest;
        placementRequest.SetGameSessionQueueName(m_context->m_requestParams.m_queueName.c_str());
        placementRequest.WithMaximumPlayerSessionCount(m_context->m_requestParams.m_numPublicSlots + m_context->m_requestParams.m_numPrivateSlots)
            .WithGameSessionName(m_context->m_requestParams.m_instanceName.c_str())
            .WithGameProperties(gameProperties)
            .WithPlacementId(AZ::Uuid::Create().ToString<AZStd::string>(false, false).c_str());

        m_startGameSessionPlacementOutcomeCallable = m_context->m_gameLiftClient.lock()->StartGameSessionPlacementCallable(placementRequest);

        m_isDone = false;
        return true;
    }

    void GameLiftGameSessionPlacementRequest::SearchDone()
    {
        m_queueSessionState = GameSessionPlacementState::Unknown;
        GameLiftSearch::SearchDone();
    }

    void GameLiftGameSessionPlacementRequest::StartGameSessionPlacement()
    {
        // Poll with 0 delay to see if callable is ready
        if (m_startGameSessionPlacementOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            Aws::GameLift::Model::StartGameSessionPlacementOutcome placementResult = m_startGameSessionPlacementOutcomeCallable.get();
            if (!placementResult.IsSuccess())
            {
                AZ_TracePrintf("GameLift", "Session placement failed with error: %s\n", placementResult.GetError().GetMessage().c_str());
                SearchDone();
                return;
            }

            m_placementId = placementResult.GetResult().GetGameSessionPlacement().GetPlacementId().c_str();
            m_queueSessionState = GameSessionPlacementState::WaitForPlacement;
        }
    }

    void GameLiftGameSessionPlacementRequest::WaitForGameSessionPlacement()
    {
        if (!m_describeGameSessionPlacementCallable.valid()) {
            Aws::GameLift::Model::DescribeGameSessionPlacementRequest describePlacementRequest;
            describePlacementRequest.WithPlacementId(m_placementId);
            m_describeGameSessionPlacementCallable = m_context->m_gameLiftClient.lock()->DescribeGameSessionPlacementCallable(describePlacementRequest);
        }

        // Poll with 0 delay to see if callable is ready
        if (m_describeGameSessionPlacementCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            Aws::GameLift::Model::DescribeGameSessionPlacementOutcome describePlacementResult = m_describeGameSessionPlacementCallable.get();

            if (!describePlacementResult.IsSuccess())
            {
                AZ_TracePrintf("GameLift", "Placement not able to describe: %s\n", m_placementId.c_str());
                SearchDone();
                return;
            }
            Aws::GameLift::Model::GameSessionPlacement placement = describePlacementResult.GetResult().GetGameSessionPlacement();
            if (placement.GetStatus() == Aws::GameLift::Model::GameSessionPlacementState::FULFILLED)
            {
                m_gameSessionId = placement.GetGameSessionId();
                m_queueSessionState = GameSessionPlacementState::AddGameSessionSearchResult;
            }
            else if (placement.GetStatus() == Aws::GameLift::Model::GameSessionPlacementState::TIMED_OUT ||
                placement.GetStatus() == Aws::GameLift::Model::GameSessionPlacementState::CANCELLED)
            {
                AZ_TracePrintf("GameLift", "Failed to describe placement: %s\n", m_placementId.c_str());
                SearchDone();
                return;
            }

        }
    }

    const Aws::GameLift::Model::GameSession GameLiftGameSessionPlacementRequest::GetPlacedGameSession()
    {
        Aws::GameLift::Model::DescribeGameSessionDetailsRequest gameSessionRequest;
        gameSessionRequest.WithGameSessionId(m_gameSessionId);

        Aws::GameLift::Model::DescribeGameSessionDetailsOutcome describeSessionOutcome = m_context->m_gameLiftClient.lock()->DescribeGameSessionDetails(gameSessionRequest);

        if (!describeSessionOutcome.IsSuccess())
        {
            AZ_TracePrintf("GameLift", "Game Session not able to describe: %s\n", m_gameSessionId.c_str());
            SearchDone();
            return Aws::GameLift::Model::GameSession();
        }

        auto gameSessionDetails = describeSessionOutcome.GetResult().GetGameSessionDetails();
        if (gameSessionDetails.size() <= 0)
        {
            AZ_TracePrintf("GameLift", "No Session found: %s\n", m_gameSessionId.c_str());
            SearchDone();
            return Aws::GameLift::Model::GameSession();
        }

        return gameSessionDetails[0].GetGameSession();
    }

    void GameLiftGameSessionPlacementRequest::AddGameSessionSearchResult(const Aws::GameLift::Model::GameSession &gameSession)
    {
        GameLiftSearchInfo info;
        info.m_fleetId = gameSession.GetFleetId().c_str();
        info.m_sessionId = gameSession.GetGameSessionId().c_str();
        info.m_numFreePublicSlots = gameSession.GetMaximumPlayerSessionCount() - gameSession.GetCurrentPlayerSessionCount();
        info.m_numUsedPublicSlots = gameSession.GetCurrentPlayerSessionCount();
        info.m_numPlayers = gameSession.GetCurrentPlayerSessionCount();

        auto const& properties = gameSession.GetGameProperties();
        for (auto const& prop : properties)
        {
            info.m_params[info.m_numParams].m_id = prop.GetKey().c_str();
            info.m_params[info.m_numParams].m_value = prop.GetValue().c_str();

            ++info.m_numParams;
        }

        m_results.push_back(info);
    }

    void GameLiftGameSessionPlacementRequest::Update()
    {
        if (m_isDone)
        {
            return;
        }
        
        switch (m_queueSessionState)
        {
            case GameSessionPlacementState::StartPlacement:
            {
                StartGameSessionPlacement();
                break;
            }
            case GameSessionPlacementState::WaitForPlacement:
            {
                WaitForGameSessionPlacement();
                break;
            }
            case GameSessionPlacementState::AddGameSessionSearchResult:
            {
                auto gameSession = GetPlacedGameSession();
                if (!gameSession.GetGameSessionId().empty())
                {
                    AddGameSessionSearchResult(gameSession);
                    SearchDone();
                }
                break;
            }
            case GameSessionPlacementState::Unknown:
            {
                AZ_TracePrintf("GameLift", "Unknown state is not expected for queueName: %s\n", m_context->m_requestParams.m_queueName.c_str());
                SearchDone();
                break;
            }
        }
    }
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
