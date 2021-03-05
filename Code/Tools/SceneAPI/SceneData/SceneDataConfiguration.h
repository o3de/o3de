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

#include <AzCore/PlatformDef.h>

#if defined(AZ_PLATFORM_WINDOWS)
    #define SCENE_DATA_CLASS
    #if defined(AZ_MONOLITHIC_BUILD)
        #define SCENE_DATA_API
    #else
        #if defined(SCENE_DATA_EXPORTS)
            #define SCENE_DATA_API AZ_DLL_EXPORT
        #else
            #define SCENE_DATA_API AZ_DLL_IMPORT
        #endif
    #endif
#else
    #if defined(AZ_MONOLITHIC_BUILD)
        #define SCENE_DATA_CLASS
        #define SCENE_DATA_API
    #else
        #if defined(SCENE_DATA_EXPORTS)
            #define SCENE_DATA_CLASS AZ_DLL_EXPORT
            #define SCENE_DATA_API AZ_DLL_EXPORT
        #else
            #define SCENE_DATA_CLASS AZ_DLL_IMPORT
            #define SCENE_DATA_API AZ_DLL_IMPORT
        #endif
    #endif
#endif