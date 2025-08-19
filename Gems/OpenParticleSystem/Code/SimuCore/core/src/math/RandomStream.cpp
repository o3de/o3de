/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cstdlib>
#include "core/math/RandomStream.h"

namespace SimuCore {
    RandomStream::RandomStream(int32_t seed) : randSeed(seed)
    {
    }

    float RandomStream::Rand()
    {
        Random::MutateSeed(randSeed);
        return Random::Rand();
    }

    float RandomStream::RandRange(float min, float max)
    {
        return min + (max - min) * Rand();
    }

    float RandomStream::UnitRandom()
    {
        return static_cast<float>(Rand()) / static_cast<float>(RAND_MAX);
    }

    float RandomStream::SymmetricRandom()
    {
        return 2.0f * UnitRandom() - 1.0f;
    }
}
