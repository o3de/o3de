#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
