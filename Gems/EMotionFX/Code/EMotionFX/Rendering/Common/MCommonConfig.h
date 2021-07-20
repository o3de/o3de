/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __MCOMMON_CONFIG_H
#define __MCOMMON_CONFIG_H

#include <MCore/Source/StandardHeaders.h>

// when we use DLL files, setup the MCOMMON_API macro
#if defined(MCOMMON_DLL_EXPORT)
    #define MCOMMON_API MCORE_EXPORT
#else
    #if defined(MCOMMON_DLL)
        #define MCOMMON_API MCORE_IMPORT
    #else
        #define MCOMMON_API
    #endif
#endif

// memory categories
enum
{
    MEMCATEGORY_MCOMMON = 992,
};

#endif
