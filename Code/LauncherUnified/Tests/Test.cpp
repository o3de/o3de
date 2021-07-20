/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string_view.h>

namespace O3DELauncher
{
    bool WaitForAssetProcessorConnect()
    {
        return false;
    }

    bool IsDedicatedServer()
    {
        return false;
    }

    const char* GetLogFilename()
    {
        return "@log@/Game.log";
    }

    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return { LY_CMAKE_TARGET };
    }

    AZStd::string_view GetProjectName()
    {
        return { "Tests" };
    }

    AZStd::string_view GetProjectPath()
    {
        return { "Tests" };
    }
}
