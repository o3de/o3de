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
        ParticleRandom(float min, float max, const RandomTickMode& mode, uint32_t maxParticleNum);
        ~ParticleRandom() = default;

        float Tick(const BaseInfo& info) override;
        float Tick(const BaseInfo& info, const Particle& particle) override;
    private:
        const static uint32_t MAX_CACHE_SIZE{100000};
        float RandomTick(uint64_t index);

        float minRange = 0.0f;
        float maxRange = 1.0f;
        // particle & random value
        uint32_t realCacheSize{0};
        std::vector<float> valueCache;
        RandomSamplerInfo sampler;
        RandomTickMode tickMode = RandomTickMode::ONCE;
    };
} // SimuCore::ParticleCore
