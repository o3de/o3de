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
    //! AWSGameLiftAcceptMatchRequest
    //! GameLift accept match request which corresponds to Amazon GameLift
    //! Registers a player's acceptance or rejection of a proposed FlexMatch match. 
    //! AcceptMatchRequest
    struct AWSGameLiftAcceptMatchRequest
        : public AzFramework::AcceptMatchRequest
    {
    public:
        AZ_RTTI(AWSGameLiftAcceptMatchRequest, "{8372B297-88E8-4C13-B31D-BE87236CA416}", AzFramework::AcceptMatchRequest);
        static void Reflect(AZ::ReflectContext* context);

        AWSGameLiftAcceptMatchRequest() = default;
        virtual ~AWSGameLiftAcceptMatchRequest() = default;
    };
} // namespace AWSGameLift
