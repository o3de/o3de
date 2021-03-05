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
#pragma once

#if !defined(BUILD_GAMELIFT_SERVER) && defined(BUILD_GAMELIFT_CLIENT)

#include <GameLift/Session/GameLiftRequestInterface.h>


namespace GridMate
{
    /*
    * Responsible for making GameLift DescribeGameSessionQueues and extracting fleetId from the destination arn.
    * Calls clientService callback with the fleetId
    */
    class DescribeGameSessionsQueueRequest
        : public GameLiftRequestInterface
    {
    public:
        GM_CLASS_ALLOCATOR(DescribeGameSessionsQueueRequest);
        DescribeGameSessionsQueueRequest(GameLiftClientService* client, const std::shared_ptr<GameLiftRequestInterfaceContext>& context);
        bool Initialize() override;
    private:
        void DescribeGameSessionQueuesHandler(const Aws::GameLift::GameLiftClient* client,
            const Aws::GameLift::Model::DescribeGameSessionQueuesRequest& request,
            const Aws::GameLift::Model::DescribeGameSessionQueuesOutcome& outcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context);
    };
} // namespace GridMate

#endif
