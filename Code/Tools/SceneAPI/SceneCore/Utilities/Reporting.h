#pragma once

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

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            // Window name used for reporting errors through AZ_TracePrintf.
            const char* const ErrorWindow = "Error";
            // Window name used for reporting warnings through AZ_TracePrintf.
            const char* const WarningWindow = "Warning";
            // Window name used for reporting success events through AZ_TracePrintf.
            const char* const SuccessWindow = "Success";
            // Window name used for logging through AZ_TracePrintf.
            const char* const LogWindow = "SceneAPI";

        } // Utilities
    } // SceneAPI
} // AZ
