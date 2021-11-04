/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Session/SessionRequests.h>

namespace AWSGameLift
{
    //! AWSGameLiftCreateSessionOnQueueRequest
    //! GameLift create session on queue request which corresponds to Amazon GameLift
    //! StartGameSessionPlacement
    struct AWSGameLiftCreateSessionOnQueueRequest
        : public AzFramework::CreateSessionRequest
    {
    public:
        AZ_RTTI(AWSGameLiftCreateSessionOnQueueRequest, "{2B99E594-CE81-4EB0-8888-74EF4242B59F}", AzFramework::CreateSessionRequest);
        static void Reflect(AZ::ReflectContext* context);

        AWSGameLiftCreateSessionOnQueueRequest() = default;
        virtual ~AWSGameLiftCreateSessionOnQueueRequest() = default;

        //! Name of the queue to use to place the new game session. You can use either the queue name or ARN value. 
        AZStd::string m_queueName;

        //! A unique identifier to assign to the new game session placement. This value is developer-defined.
        //! The value must be unique across all Regions and cannot be reused unless you are resubmitting a canceled or timed-out placement request.
        AZStd::string m_placementId;
    };
} // namespace AWSGameLift
