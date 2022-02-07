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
    // Reference https://sdk.amazonaws.com/cpp/api/LATEST/_game_session_status_8h_source.html
    static const char* AWSGameLiftSessionStatusNames[6] = { "NotSet", "Active", "Activating", "Terminated", "Terminating", "Error"};

    // Reference https://sdk.amazonaws.com/cpp/api/LATEST/_game_session_status_reason_8h.html
    static const char* AWSGameLiftSessionStatusReasons[2] = { "NotSet", "Interrupted" };

    static constexpr const char AWSGameLiftErrorMessageTemplate[] = "Exception: %s, Message: %s";
    static constexpr const char AWSGameLiftClientMissingErrorMessage[] = "GameLift client is not configured yet.";
} // namespace AWSGameLift
