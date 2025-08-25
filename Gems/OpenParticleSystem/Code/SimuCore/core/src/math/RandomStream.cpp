/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "core/math/RandomStream.h"

namespace SimuCore {
    RandomStream::RandomStream(AZ::u64 seed)
    {
        if (seed)
        {
            m_random.SetSeed(static_cast<AZ::u64>(seed));
        }
        else
        {
            // use a cryptographically secure random to seed our fast random stream
            AZ::BetterPseudoRandom random;
            random.GetRandom(&seed, sizeof(seed));
            m_random.SetSeed(static_cast<AZ::u64>(seed));
        }
    }

    float RandomStream::Rand()
    {
        return m_random.GetRandomFloat();
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
