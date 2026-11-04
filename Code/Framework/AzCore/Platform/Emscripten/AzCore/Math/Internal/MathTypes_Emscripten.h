/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if AZ_TRAIT_USE_PLATFORM_SIMD_SSE
    #include <xmmintrin.h>
    #include <pmmintrin.h>
    #include <emmintrin.h>
    #include <smmintrin.h>
#elif AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    #include <arm_neon.h>
#endif
