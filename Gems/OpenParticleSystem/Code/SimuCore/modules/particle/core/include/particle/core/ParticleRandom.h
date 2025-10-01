/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "particle/core/ParticleDistribution.h"
#include "core/math/Random.h"

namespace SimuCore::ParticleCore {

    class ParticleRandom : public ParticleDistribution  {
    public:
        ParticleRandom() = default;
        ParticleRandom(float min, float max, const RandomTickMode& mode, AZ::u32 maxParticleNum);
        ~ParticleRandom() = default;

        float Tick(const BaseInfo& info) override;
        float Tick(const BaseInfo& info, const Particle& particle) override;
    private:
        static constexpr AZ::u32 MAX_CACHE_SIZE{100000};
        float RandomTick(AZ::u64 index);

        float minRange = 0.0f;
        float maxRange = 1.0f;
        // particle & random value
        AZ::u32 realCacheSize{0};
        AZStd::vector<float> valueCache;
        RandomSamplerInfo sampler;
        RandomTickMode tickMode = RandomTickMode::ONCE;
    };
} // SimuCore::ParticleCore
