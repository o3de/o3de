/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace O3DELauncher
{
    bool WaitForAssetProcessorConnect()
    {
        return true;
    }

    bool IsDedicatedServer()
    {
        return false;
    }

    const char* GetLogFilename()
    {
        return "@log@/Unified.log";
    }

    const char* GetLauncherTypeSpecialization()
    {
        return "unified";
    }
}
