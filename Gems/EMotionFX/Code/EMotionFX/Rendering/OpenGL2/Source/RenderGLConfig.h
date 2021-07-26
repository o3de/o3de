/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_CONFIG_H
#define __RENDERGL_CONFIG_H

#include <MCore/Source/StandardHeaders.h>


// when we use DLL files, setup the RENDERGL_API macro
#if defined(RENDERGL_DLL_EXPORT)
    #define RENDERGL_API MCORE_EXPORT
#else
    #if defined(RENDERGL_DLL)
        #define RENDERGL_API MCORE_IMPORT
    #else
        #define RENDERGL_API
    #endif
#endif

// memory categories
enum
{
    MEMCATEGORY_RENDERING = 997
};


#endif
