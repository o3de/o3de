/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/core/ParticleRandom.h"

namespace SimuCore::ParticleCore {
    ParticleRandom::ParticleRandom(float min, float max, const RandomTickMode& mode, AZ::u32 maxParticleNum)
            : minRange(min), maxRange(max), tickMode(mode)
    {
        this->realCacheSize = std::min(MAX_CACHE_SIZE, maxParticleNum);
        switch (tickMode) {
            case RandomTickMode::ONCE:{
                valueCache.resize(1);
                break;
            }
            case RandomTickMode::PER_SPAWN: {
                valueCache.resize(this->realCacheSize, FLT_MIN);
                break;
            }
            default:
                valueCache.resize(0);
                break;
        }
    }


    float ParticleRandom::Tick(const BaseInfo& info)
    {
        (void)info;
        return RandomTick(0);
    }

    float ParticleRandom::Tick(const BaseInfo& info, const Particle& particle)
    {
        (void)info;
        return RandomTick(particle.id % realCacheSize);
    }

    float ParticleRandom::RandomTick(AZ::u64 index)
    {
        if (tickMode == RandomTickMode::ONCE) {
            if (sampler.needRegenerate) {
                valueCache[0] = Random::RandomRange(minRange, maxRange);
                sampler.needRegenerate = false;
            }
            return valueCache[0];
        }
        if (tickMode == RandomTickMode::PER_FRAME) {
            return Random::RandomRange(minRange, maxRange);
        }
        if (tickMode == RandomTickMode::PER_SPAWN) {
            if (valueCache[index] == FLT_MIN) {
                valueCache[index] = Random::RandomRange(minRange, maxRange);
            }
            return valueCache[index];
        }
        return 0.0f;
    }
} // SimuCore::ParticleCore
