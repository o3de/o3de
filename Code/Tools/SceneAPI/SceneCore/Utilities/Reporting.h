#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            // A key to indicate if the scene builder is in debug mode
            inline constexpr const char* Key_AssetProcessorInDebugOutput = "/O3DE/AssetProcessor/InDebug";
        } // Utilities
    } // SceneAPI
} // AZ
