/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Matchmaking/MatchmakingRequests.h>

namespace AWSGameLift
{
    //! AWSGameLiftStopMatchmakingRequest
    //! GameLift stop matchmaking request which corresponds to Amazon GameLift
    //! Cancels a matchmaking ticket or match backfill ticket that is currently being processed. 
    //! StopMatchmakingRequest
    struct AWSGameLiftStopMatchmakingRequest
        : public AzFramework::StopMatchmakingRequest
    {
    public:
        AZ_RTTI(AWSGameLiftStopMatchmakingRequest, "{2766BC03-9F84-4346-A52B-49129BBAF38B}", AzFramework::StopMatchmakingRequest);
        static void Reflect(AZ::ReflectContext* context);

        AWSGameLiftStopMatchmakingRequest() = default;
        virtual ~AWSGameLiftStopMatchmakingRequest() = default;
    };
} // namespace AWSGameLift
