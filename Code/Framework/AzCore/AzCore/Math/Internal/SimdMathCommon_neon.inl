/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace Simd
    {
        namespace Neon
        {
            constexpr uint32x4_t LastLane{ 0, 0, 0, 1 };

            AZ_MATH_INLINE bool AreAllLanesTrue(uint32x2_t value)
            {
                return vminv_u32(value) != 0;
            }

            AZ_MATH_INLINE bool AreAllLanesTrue(uint32x4_t value)
            {
                return vminvq_u32(value) != 0;
            }

            AZ_MATH_INLINE bool AreFirstThreeLanesTrue(uint32x4_t value)
            {
                return AreAllLanesTrue(vorrq_u32(value, LastLane));
            }
        }
    }
}
