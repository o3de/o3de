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

#pragma once

// include the core standard headers
#include <MCore/Source/StandardHeaders.h>
#include "MemoryCategories.h"

/**
 * Forward declare an EMotion FX class.
 */
#define EMFX_FORWARD_DECLARE(className) \
    namespace EMotionFX { class className; }


// when we use DLL files, setup the EMFX_API macro
#if defined(EMFX_DLL_EXPORT)
    #define EMFX_API MCORE_EXPORT
#else
    #if defined(EMFX_DLL)
        #define EMFX_API MCORE_IMPORT
    #else
        #define EMFX_API
    #endif
#endif


// disable scaling support throughout the full API when desired
//#define EMFX_SCALE_DISABLED


// include given code or not, based on if scaling is enabled or disabled
#ifndef EMFX_SCALE_DISABLED
    #define EMFX_SCALECODE(x) x
#else
    #define EMFX_SCALECODE(x)
#endif

// Invalid index values, used in things like Find functions that return indices.
// When something cannot be found it will return one of these.
constexpr AZ::u64 InvalidIndex64 = 0xffffffffffffffff;
constexpr AZ::u32 InvalidIndex32 = 0xffffffff;
constexpr AZ::u16 InvalidIndex16 = 0xffff;
constexpr AZ::u8 InvalidIndex8 = 0xff;
constexpr size_t InvalidIndex = 0xffffffffffffffff;
