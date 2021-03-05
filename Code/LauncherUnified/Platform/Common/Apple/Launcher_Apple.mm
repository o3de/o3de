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

#include "Launcher_Apple.h"

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include <Foundation/Foundation.h>


namespace LumberyardLauncher
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