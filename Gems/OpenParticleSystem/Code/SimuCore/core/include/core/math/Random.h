/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SIMU_CORE_MATH_RANDOM_H
#define SIMU_CORE_MATH_RANDOM_H

#include <cstdint>
#include <AzCore/Math/Color.h>

namespace SimuCore {
    class Random {
    public:
        static bool GenRandom(void* data, uint32_t size);

        template <typename T>
        static bool GenRandom(T& val)
        {
            return GenRandom(&val, sizeof(T));
        }

        static void RandSeed(int32_t seed);

        static float Rand();

        static float Rand(int32_t seed);

        static void MutateSeed(int32_t& seed);

        static float RandomRange(float min, float max);

        static uint32_t RandomRange(uint32_t min, uint32_t max);

        static AZ::Color RandomRange(const AZ::Color& min, const AZ::Color& max);

        static int32_t randSeed;
        static int32_t initSeed;
    };
}

#endif // SIMU_CORE_MATH_RANDOM_H
