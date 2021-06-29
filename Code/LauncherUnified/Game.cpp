/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string_view.h>

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
        return "@log@/Game.log";
    }
}
