/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Launcher_Apple.h"

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include <Foundation/Foundation.h>


namespace O3DELauncher
{
    const char* GetAppResourcePath()
    {
        static char pathToAssets[AZ_MAX_PATH_LEN] = { 0 };

        if (pathToAssets[0] == 0)
        {
            const char* pathToResources = [[[NSBundle mainBundle] resourcePath] UTF8String];
            azsnprintf(pathToAssets, AZ_MAX_PATH_LEN, "%s/%s", pathToResources, "assets");
        }

        return pathToAssets;
    }
}
