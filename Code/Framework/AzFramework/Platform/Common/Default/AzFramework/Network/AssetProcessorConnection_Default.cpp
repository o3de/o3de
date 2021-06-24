/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <stdio.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        namespace Platform
        {
            void DebugOutput(const char* message)
            {
                fputs("AssetProcessorConnection:", stdout);
                fputs(message, stdout);
            }
        }
    }
}
