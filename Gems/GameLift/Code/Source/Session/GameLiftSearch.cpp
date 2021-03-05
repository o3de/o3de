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

#include <GameLift/Session/GameLiftSearch.h>
#include <GameLift/Session/GameLiftClientService.h>

#include <aws/core/utils/Outcome.h>

// To avoid the warning below
// Semaphore.h(50): warning C4251: 'Aws::Utils::Threading::Semaphore::m_mutex': class 'std::mutex' needs to have dll-interface to be used by clients of class 'Aws::Utils::Threading::Semaphore'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <aws/gamelift/model/SearchGameSessionsRequest.h>
#include <aws/gamelift/model/DescribeGameSessionQueuesRequest.h>
AZ_POP_DISABLE_WARNING

namespace GridMate
{
    const AZStd::string GameLiftFleetIdPrefix = "fleet/";

    const AZStd::string ExtractFleetIdFromFleetArn(AZStd::string fleetArn)
    {
        return fleetArn.substr(fleetArn.rfind(GameLiftFleetIdPrefix) + GameLiftFleetIdPrefix.size());
    }

    GameLiftSearch::GameLiftSearch(GameLiftClientService* service, const AZStd::shared_ptr<GameLiftRequestInterfaceContext> context)
        : GridSearch(service)
        , GameLiftRequestInterface(context)
    {
        m_isDone = true;
    }

    bool GameLiftSearch::Initialize()
    {
        if (!m_context->m_searchParams.m_queueName.empty())
        {
            StartDescribeGameSessionQueue();
        }
        else
        {
            StartSearchGameSession();
        }
        m_isDone = false;
        return true;
    }

    void GameLiftSearch::StartSearchGameSession()
    {
        Aws::GameLift::Model::SearchGameSessionsRequest request;
        m_context->m_searchParams.m_useFleetId ? request.SetFleetId(m_context->m_searchParams.m_fleetId.c_str())
            : request.SetAliasId(m_context->m_searchParams.m_aliasId.c_str());
        if (!m_context->m_searchParams.m_gameInstanceId.empty())
        {
            Aws::String filter("gameSessionId = ");
            filter += m_context->m_searchParams.m_gameInstanceId.c_str();
            request.SetFilterExpression(filter);
        }

        m_searchGameSessionsOutcomeCallable = m_context->m_gameLiftClient.lock()->SearchGameSessionsCallable(request);
    }

    void GameLiftSearch::WaitForSearchGameSession()
    {
        if (m_searchGameSessionsOutcomeCallable.valid()
            && m_searchGameSessionsOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto result = m_searchGameSessionsOutcomeCallable.get();
            if (result.IsSuccess())
            {
                auto gameSessions = result.GetResult().GetGameSessions();
                for (auto& gameSession : gameSessions)
                {
                    ProcessGameSessionResult(gameSession);
                }
            }
            else
            {
                AZ_TracePrintf("GameLift", "Session search failed with error: %s\n", result.GetError().GetMessage().c_str());
            }

            SearchDone();
        }
    }

    void GameLiftSearch::StartDescribeGameSessionQueue()
    {
        Aws::GameLift::Model::DescribeGameSessionQueuesRequest request;
        request.AddNames(m_context->m_searchParams.m_queueName.c_str());
        m_describeGameSessionQueueOutcomeCallable = m_context->m_gameLiftClient.lock()->DescribeGameSessionQueuesCallable(request);
    }

    void GameLiftSearch::WaitDescribeGameSessionQueue()
    {
        if (m_describeGameSessionQueueOutcomeCallable.valid()
            && m_describeGameSessionQueueOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto result = m_describeGameSessionQueueOutcomeCallable.get();
            if (result.IsSuccess())
            {
                if (result.GetResult().GetGameSessionQueues().size() == 0)
                {
                    Aws::String errorMessage = "No Queue found for queue name: %s";
                    Aws::Utils::StringUtils::Replace(errorMessage, "%s", m_context->m_searchParams.m_queueName.c_str());
                    AZ_TracePrintf("GameLift", errorMessage.c_str());
                    SearchDone();
                    return;
                }
                auto gameSessionQueue = result.GetResult().GetGameSessionQueues().front();
                if (m_context->m_searchParams.m_queueName.compare(gameSessionQueue.GetName().c_str()) == 0
                    && gameSessionQueue.GetDestinations().size() > 0)
                {
                    // Default to first fleet in queue. FleetId is used to describe game sessions.
                    AZStd::string fleetArn = gameSessionQueue.GetDestinations().size() > 0 ? gameSessionQueue.GetDestinations()[0].GetDestinationArn().c_str() : "";
                    m_context->m_searchParams.m_fleetId = ExtractFleetIdFromFleetArn(fleetArn);
                    m_context->m_searchParams.m_useFleetId = true;
                    StartSearchGameSession();
                }
            }
            else
            {
                AZ_TracePrintf("GameLift", "Game session queue search failed with error: %s\n", result.GetError().GetMessage().c_str());
                SearchDone();
            }
        }
    }

    unsigned int GameLiftSearch::GetNumResults() const
    {
        return static_cast<unsigned int>(m_results.size());
    }

    const SearchInfo* GameLiftSearch::GetResult(unsigned int index) const
    {
        return &m_results[index];
    }

    void GameLiftSearch::AbortSearch()
    {
        SearchDone();
    }

    void GameLiftSearch::SearchDone()
    {
        m_isDone = true;
    }

    void GameLiftSearch::Update()
    {
        if (m_isDone)
        {
            return;
        }

        WaitDescribeGameSessionQueue();

        WaitForSearchGameSession();
    }

    void GameLiftSearch::ProcessGameSessionResult(const Aws::GameLift::Model::GameSession& gameSession)
    {
        GameLiftSearchInfo info;
        info.m_fleetId = gameSession.GetFleetId().c_str();
        info.m_sessionId = gameSession.GetGameSessionId().c_str();
        info.m_numFreePublicSlots = gameSession.GetMaximumPlayerSessionCount() - gameSession.GetCurrentPlayerSessionCount();
        info.m_numUsedPublicSlots = gameSession.GetCurrentPlayerSessionCount();
        info.m_numPlayers = gameSession.GetCurrentPlayerSessionCount();
        info.m_port = gameSession.GetPort();

        auto& properties = gameSession.GetGameProperties();
        for (auto& prop : properties)
        {
            info.m_params[info.m_numParams].m_id = prop.GetKey().c_str();
            info.m_params[info.m_numParams].m_value = prop.GetValue().c_str();

            ++info.m_numParams;
        }

        m_results.push_back(info);
    }
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
