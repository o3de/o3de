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

#ifndef __EMSTUDIO_RENDERPLUGINSCONFIG_H
#define __EMSTUDIO_RENDERPLUGINSCONFIG_H

#include <MCore/Source/StandardHeaders.h>

// when we use DLL files, setup the EMFX_API macro
#if defined(RENDERPLUGINS_DLL_EXPORT)
    #define RENDERPLUGINS_API MCORE_EXPORT
#else
    #if defined(RENDERPLUGINS_DLL)
        #define RENDERPLUGINS_API MCORE_IMPORT
    #else
        #define RENDERPLUGINS_API
    #endif
#endif

// memory categories
enum
{
    MEMCATEGORY_RENDERPLUGIN = 993
};

#endif
