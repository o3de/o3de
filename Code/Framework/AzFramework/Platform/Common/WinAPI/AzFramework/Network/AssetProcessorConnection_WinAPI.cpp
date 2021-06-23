/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/base.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        namespace Platform
        {
            void DebugOutput(const char* message)
            {
                OutputDebugString("AssetProcessorConnection:");
                OutputDebugString(message);
            }
        }
    }
}
