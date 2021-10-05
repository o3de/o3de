/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AWSGameLift
{
    //! IAWSGameLiftMatchmakingInternalRequests
    //! GameLift Gem matchmaking internal interfaces which is used to communicate
    //! with client side ticket tracker to sync matchmaking ticket data and join
    //! player to the match
    class IAWSGameLiftMatchmakingInternalRequests
    {
    public:
        AZ_RTTI(IAWSGameLiftMatchmakingInternalRequests, "{C2DA440E-74E0-411E-813D-5880B50B0C9E}");

        IAWSGameLiftMatchmakingInternalRequests() = default;
        virtual ~IAWSGameLiftMatchmakingInternalRequests() = default;

        //! StartPolling
        //! Request to start process for polling matchmaking ticket based on given ticket id and player id
        //! @param ticketId The requested matchmaking ticket id
        //! @param playerId The requested matchmaking player id
        virtual void StartPolling(const AZStd::string& ticketId, const AZStd::string& playerId) = 0;

        //! StopPolling
        //! Request to stop process for polling matchmaking ticket
        virtual void StopPolling() = 0;
    };
} // namespace AWSGameLift
