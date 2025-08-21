/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "core/math/Random.h"

namespace SimuCore {
    class RandomStream {
    public:
        explicit RandomStream(int32_t seed = 0);
        ~RandomStream() = default;

        float Rand();

        float RandRange(float min, float max);

        float UnitRandom();

        float SymmetricRandom();

    private:
        int32_t randSeed;
    };
}
