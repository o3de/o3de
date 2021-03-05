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
