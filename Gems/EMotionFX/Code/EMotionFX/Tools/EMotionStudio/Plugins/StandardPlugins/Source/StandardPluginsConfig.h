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
