/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "core/math/Random.h"
#include "core/math/Math.h"
#include <AzCore/Math/Random.h>

namespace SimuCore_Random_Internal
{
    // note, this class does not make any heap allocations.
    AZ::SimpleLcgRandom s_random;
    // note, if we want cryptographic quality random numbers, we can use AZ::BetterPseudoRandom which
    // is already PAL-ified to use the best available system API like wincrypt on windows.
    // But I don't think that particles need to be cryptographically secure, and those APIS are always more
    // costly to call.
} // namespace SimuCore_Random_Internal

namespace SimuCore {
    using namespace SimuCore_Random_Internal;

    float Random::Rand()
    {
        return s_random.GetRandomFloat();
    }

    float Random::RandomRange(float min, float max)
    {
        return min + (max - min) * Rand();
    }

    AZ::u32 Random::RandomRange(AZ::u32 min, AZ::u32 max)
    {
        return min + static_cast<AZ::u32>(static_cast<float>(max - min) * Rand());
    }

    AZ::Color Random::RandomRange(const AZ::Color& min, const AZ::Color& max)
    {
        return AZ::Color(min.GetR() + Rand() * (max.GetR() - min.GetR()),
                         min.GetG() + Rand() * (max.GetG() - min.GetG()),
                         min.GetB() + Rand() * (max.GetB() - min.GetB()),
                         min.GetA() + Rand() * (max.GetA() - min.GetA()));
    }
}
