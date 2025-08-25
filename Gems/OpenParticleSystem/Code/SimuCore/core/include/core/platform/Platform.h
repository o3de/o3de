/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#include <AzCore/PlatformDef.h>
#include <AzCore/Math/SimdMath.h>

#define SIMUCORE_SIMD_SSE41   (0x01 << 16)
#define SIMUCORE_SIMD_NEON    (0x02 << 16)

#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    #define SIMUCORE_SIMD SIMUCORE_SIMD_NEON
#elif AZ_TRAIT_USE_PLATFORM_SIMD_SSE
    #define SIMUCORE_SIMD SIMUCORE_SIMD_SSE41
#else
    #define SIMUCORE_SIMD 0
#endif

#define SIMUCORE_PLATFORM_INFO SIMUCORE_SIMD
