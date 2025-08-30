/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstdint>
#include "core/platform/Platform.h"

namespace SimuCore {
    constexpr uint32_t VECTOR_SIZE = 4;

    template <uint32_t D>
    struct VecValue {
        float value[D];

        float& operator[](size_t val)
        {
            return value[val];
        }

        const float& operator[](size_t val) const
        {
            return value[val];
        }
    };

#if (defined(SIMUCORE_PLATFORM_INFO) && defined(SIMUCORE_SIMD_SSE41) && ((SIMUCORE_PLATFORM_INFO & SIMUCORE_SIMD_SSE41) != 0))
    using VEC2_TYPE = __m128;
    using VEC3_TYPE = __m128;
    using VEC4_TYPE = __m128;
// TEMPORARILY - comment out the neon types.
/*#elif (defined(SIMUCORE_PLATFORM_INFO) && defined(SIMUCORE_SIMD_NEON) && ((SIMUCORE_PLATFORM_INFO & SIMUCORE_SIMD_NEON) != 0))
    using VEC2_TYPE = float32x4_t;
    using VEC3_TYPE = float32x4_t;
    using VEC4_TYPE = float32x4_t;
*/
#else
    using VEC2_TYPE = VecValue<VECTOR_SIZE>;
    using VEC3_TYPE = VecValue<VECTOR_SIZE>;
    using VEC4_TYPE = VecValue<VECTOR_SIZE>;
#endif
}
