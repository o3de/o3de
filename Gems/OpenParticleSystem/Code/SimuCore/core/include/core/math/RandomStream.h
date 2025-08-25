/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Random.h>

namespace SimuCore {
    class RandomStream {
    public:
        explicit RandomStream(AZ::u64 seed = 0);
        ~RandomStream() = default;

        float Rand();

        float RandRange(float min, float max);

        float UnitRandom();

        float SymmetricRandom();

    private:
        AZ::SimpleLcgRandom m_random;
    };
}
