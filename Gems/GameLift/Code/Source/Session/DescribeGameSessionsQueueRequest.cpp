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

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)

#include <GameLift/Session/DescribeGameSessionsQueueRequest.h>
// To avoid the warning below
// Semaphore.h(50): warning C4251: 'Aws::Utils::Threading::Semaphore::m_mutex': class 'std::mutex' needs to have dll-interface to be used by clients of class 'Aws::Utils::Threading::Semaphore'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <aws/gamelift/model/DescribeGameSessionQueuesRequest.h>
AZ_POP_DISABLE_WARNING

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/GameLiftErrors.h>
#include <aws/gamelift/model/DescribeGameSessionQueuesResult.h>


namespace GridMate
{
    const AZStd::string GameLiftFleetIdPrefix = "fleet/";

    const AZStd::string ExtractFleetIdFromFleetArn(AZStd::string fleetArn)
    {
        return fleetArn.substr(fleetArn.rfind(GameLiftFleetIdPrefix) + GameLiftFleetIdPrefix.size());
    }

    DescribeGameSessionsQueueRequest::DescribeGameSessionsQueueRequest(GameLiftClientService* clientService
        , const std::shared_ptr<GameLiftRequestInterfaceContext>& context)
        : GameLiftRequestInterface(context)
    {
    }

    bool DescribeGameSessionsQueueRequest::Initialize()
    {
        Aws::GameLift::Model::DescribeGameSessionQueuesRequest request;
        request.AddNames(m_context->queueName);

        m_context->client.lock()->DescribeGameSessionQueuesAsync(request,
             [this](const Aws::GameLift::GameLiftClient* client,
                 const Aws::GameLift::Model::DescribeGameSessionQueuesRequest& request,
                 const Aws::GameLift::Model::DescribeGameSessionQueuesOutcome& outcome,
                 const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
         {
             this->DescribeGameSessionQueuesHandler(client, request, outcome, context);
         }
         );
        return true;
    }

    void DescribeGameSessionsQueueRequest::DescribeGameSessionQueuesHandler(const Aws::GameLift::GameLiftClient* client,
        const Aws::GameLift::Model::DescribeGameSessionQueuesRequest& request,
        const Aws::GameLift::Model::DescribeGameSessionQueuesOutcome& outcome,
        const std::shared_ptr<const Aws::Client::AsyncCallerContext>&)
    {
        if (!outcome.IsSuccess())
        {
            m_context->errorHandler(outcome.GetError().GetMessage());
            return;
        }

        Aws::String queueName = m_context->queueName;

        if (outcome.GetResult().GetGameSessionQueues().size() == 0)
        {
            Aws::String errorMessage = "No Queue found for queue name: %s";
            Aws::Utils::StringUtils::Replace(errorMessage, "%s", queueName.c_str());
            m_context->errorHandler(errorMessage);
            return;
        }
        AZStd::string fleetId;
        for (auto const& gameSessionQueue : outcome.GetResult().GetGameSessionQueues())
        {
            if (gameSessionQueue.GetName() == queueName && gameSessionQueue.GetDestinations().size() > 0)
            {
                // Default to first fleet in queue. FleetId is used to test connectivity later.
                AZStd::string fleetArn = gameSessionQueue.GetDestinations().size() > 0 ? gameSessionQueue.GetDestinations()[0].GetDestinationArn().c_str() : "";
                fleetId = ExtractFleetIdFromFleetArn(fleetArn);
                break;
            }
        }

        // This case is very unlikely as this means GameLift queue has no destinations.
        if (fleetId.empty())
        {
            Aws::String errorMessage = "No Destination fleet found %s";
            Aws::Utils::StringUtils::Replace(errorMessage, "%s", queueName.c_str());
            m_context->errorHandler(errorMessage);
            return;
        }

        m_context->successHandler(Aws::String(fleetId.c_str()));
    }

} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
