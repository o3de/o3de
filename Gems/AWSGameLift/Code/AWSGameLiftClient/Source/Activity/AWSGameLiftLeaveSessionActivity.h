/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AWSGameLift
{
    namespace LeaveSessionActivity
    {
        static constexpr const char AWSGameLiftLeaveSessionActivityName[] = "AWSGameLiftLeaveSessionActivity";
        static constexpr const char AWSGameLiftLeaveSessionMissingRequestHandlerErrorMessage[] =
            "Missing GameLift LeaveSession request handler, please make sure Multiplayer Gem is enabled and registered as handler.";

        // Request to leave the current session
        void LeaveSession();
    } // namespace LeaveSessionActivity
} // namespace AWSGameLift

