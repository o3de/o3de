/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Session/SessionConfig.h>

#include <Activity/AWSGameLiftSearchSessionsActivity.h>
#include <AWSGameLiftSessionConstants.h>

namespace AWSGameLift
{
    namespace SearchSessionsActivity
    {
        Aws::GameLift::Model::SearchGameSessionsRequest BuildAWSGameLiftSearchGameSessionsRequest(
            const AWSGameLiftSearchSessionsRequest& searchSessionsRequest)
        {
            Aws::GameLift::Model::SearchGameSessionsRequest request;
            // Optional attributes
            if (!searchSessionsRequest.m_filterExpression.empty())
            {
                request.SetFilterExpression(searchSessionsRequest.m_filterExpression.c_str());
            }
            if (!searchSessionsRequest.m_sortExpression.empty())
            {
                request.SetSortExpression(searchSessionsRequest.m_sortExpression.c_str());
            }
            if (searchSessionsRequest.m_maxResult > 0)
            {
                request.SetLimit(searchSessionsRequest.m_maxResult);
            }
            if (!searchSessionsRequest.m_nextToken.empty())
            {
                request.SetNextToken(searchSessionsRequest.m_nextToken.c_str());
            }

            // Required attributes
            if (!searchSessionsRequest.m_aliasId.empty())
            {
                request.SetAliasId(searchSessionsRequest.m_aliasId.c_str());
            }
            if (!searchSessionsRequest.m_fleetId.empty())
            {
                request.SetFleetId(searchSessionsRequest.m_fleetId.c_str());
            }
            // TODO: Update the AWS Native SDK to accept the new request parameter.
            //request.SetLocation(searchSessionsRequest.m_location.c_str());

            AZ_TracePrintf(AWSGameLiftSearchSessionsActivityName,
                "Built SearchGameSessionsRequest with FilterExpression=%s, SortExpression=%s, Limit=%d, NextToken=%s, AliasId=%s and FleetId=%s",
                request.GetFilterExpression().c_str(),
                request.GetSortExpression().c_str(),
                request.GetLimit(),
                request.GetNextToken().c_str(),
                request.GetAliasId().c_str(),
                request.GetFleetId().c_str());
            
            return request;
        }

        AzFramework::SearchSessionsResponse SearchSessions(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftSearchSessionsRequest& searchSessionsRequest)
        {
            AZ_TracePrintf(AWSGameLiftSearchSessionsActivityName, "Requesting SearchGameSessions against Amazon GameLift service ...");

            AzFramework::SearchSessionsResponse response;
            Aws::GameLift::Model::SearchGameSessionsRequest request = BuildAWSGameLiftSearchGameSessionsRequest(searchSessionsRequest);
            Aws::GameLift::Model::SearchGameSessionsOutcome outcome = gameliftClient.SearchGameSessions(request);
            AZ_TracePrintf(AWSGameLiftSearchSessionsActivityName, "SearchGameSessions request against Amazon GameLift service is complete");

            if (outcome.IsSuccess())
            {
                response = SearchSessionsActivity::ParseResponse(outcome.GetResult());
            }
            else
            {
                AZ_Error(AWSGameLiftSearchSessionsActivityName, false, AWSGameLiftErrorMessageTemplate,
                    outcome.GetError().GetExceptionName().c_str(), outcome.GetError().GetMessage().c_str());
            }

            return response;
        }

        AzFramework::SearchSessionsResponse ParseResponse(
            const Aws::GameLift::Model::SearchGameSessionsResult& gameLiftSearchSessionsResult)
        {
            AzFramework::SearchSessionsResponse response;
            response.m_nextToken = gameLiftSearchSessionsResult.GetNextToken().c_str();

            for (const Aws::GameLift::Model::GameSession& gameSession : gameLiftSearchSessionsResult.GetGameSessions())
            {
                AzFramework::SessionConfig session;
                session.m_creationTime = gameSession.GetCreationTime().Millis();
                session.m_creatorId = gameSession.GetCreatorId().c_str();
                session.m_currentPlayer = gameSession.GetCurrentPlayerSessionCount();
                session.m_ipAddress = gameSession.GetIpAddress().c_str();
                session.m_maxPlayer = gameSession.GetMaximumPlayerSessionCount();
                session.m_port = static_cast<uint16_t>(gameSession.GetPort());
                session.m_sessionId = gameSession.GetGameSessionId().c_str();
                session.m_sessionName = gameSession.GetName().c_str();
                session.m_status = AWSGameLiftSessionStatusNames[(int)gameSession.GetStatus()];
                session.m_statusReason = AWSGameLiftSessionStatusReasons[(int)gameSession.GetStatusReason()];
                session.m_terminationTime = gameSession.GetTerminationTime().Millis();
                session.m_matchmakingData = gameSession.GetMatchmakerData().c_str();
                // TODO: Update the AWS Native SDK to get the new game session attributes.
                //session.m_dnsName = gameSession.GetDnsName();

                for (const auto& gameProperty : gameSession.GetGameProperties())
                {
                    session.m_sessionProperties[gameProperty.GetKey().c_str()] = gameProperty.GetValue().c_str();
                }

                response.m_sessionConfigs.emplace_back(AZStd::move(session));
            }

            return response;
        };

        bool ValidateSearchSessionsRequest(const AzFramework::SearchSessionsRequest& searchSessionsRequest)
        {
            auto gameliftSearchSessionsRequest = azrtti_cast<const AWSGameLiftSearchSessionsRequest*>(&searchSessionsRequest);
            if (gameliftSearchSessionsRequest &&
                (!gameliftSearchSessionsRequest->m_aliasId.empty() || !gameliftSearchSessionsRequest->m_fleetId.empty()))
            {
                return true;
            }
            else
            {
                AZ_Error(AWSGameLiftSearchSessionsActivityName, false, AWSGameLiftSearchSessionsRequestInvalidErrorMessage);

                return false;
            }
        }
    }
} // namespace AWSGameLift
