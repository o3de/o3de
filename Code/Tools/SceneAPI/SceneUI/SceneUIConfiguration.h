#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

#if defined(_LIB)
    #define SCENE_UI_API
#else
    #ifdef SCENE_UI_EXPORTS
        #define SCENE_UI_API AZ_DLL_EXPORT
    #else
        #define SCENE_UI_API AZ_DLL_IMPORT
    #endif
#endif
