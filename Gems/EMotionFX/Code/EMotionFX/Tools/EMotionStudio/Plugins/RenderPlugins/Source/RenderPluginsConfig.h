/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
