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
        // Dedicated server does not depend on Asset Processor and assumes that assets are already prepared.
        return false;
    }

    bool IsDedicatedServer()
    {
        return true;
    }

    const char* GetLogFilename()
    {
        return "@log@/Server.log";
    }

    const char* GetLauncherTypeSpecialization()
    {
        return "server";
    }
}
