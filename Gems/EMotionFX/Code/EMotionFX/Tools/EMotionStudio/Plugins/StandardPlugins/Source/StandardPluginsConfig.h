/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_STANDARDPLUGINSCONFIG_H
#define __EMSTUDIO_STANDARDPLUGINSCONFIG_H

// include the EMotion FX config and mem categories on default
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/MemoryCategories.h>
#include <EMotionFX_Traits_Platform.h>

// when we use DLL files, setup the EMFX_API macro
#if defined(STANDARDPLUGINS_DLL_EXPORT)
    #define STANDARDPLUGINS_API MCORE_EXPORT
#else
    #if defined(STANDARDPLUGINS_DLL)
        #define STANDARDPLUGINS_API MCORE_IMPORT
    #else
        #define STANDARDPLUGINS_API
    #endif
#endif

// memory categories
enum
{
    MEMCATEGORY_STANDARDPLUGINS                 = 1000,
    MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH       = 1001,
    MEMCATEGORY_STANDARDPLUGINS_RESEARCH        = 1002
};

#endif
