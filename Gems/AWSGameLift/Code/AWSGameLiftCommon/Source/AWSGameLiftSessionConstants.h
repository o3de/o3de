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

namespace AWSGameLift
{
    // Reference https://sdk.amazonaws.com/cpp/api/LATEST/_game_session_status_8h_source.html
    static const char* AWSGameLiftSessionStatusNames[6] = { "NotSet", "Active", "Activating", "Terminated", "Terminating", "Error"};

    // Reference https://sdk.amazonaws.com/cpp/api/LATEST/_game_session_status_reason_8h.html
    static const char* AWSGameLiftSessionStatusReasons[2] = { "NotSet", "Interrupted" };

    static constexpr const char AWSGameLiftErrorMessageTemplate[] = "Exception: %s, Message: %s";
} // namespace AWSGameLift
