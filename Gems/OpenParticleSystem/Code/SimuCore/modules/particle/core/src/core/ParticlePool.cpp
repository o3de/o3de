/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/core/ParticlePool.h"

namespace SimuCore::ParticleCore {
    void ParticlePool::Resize(uint32_t size)
    {
        alive = 0;
        maxSize = size;
        particles.resize(maxSize);
    }

    void ParticlePool::Recycle(uint32_t beginPos)
    {
        for (uint32_t i = beginPos; i < alive; ++i) {
            Particle& particle = particles[i];
            if (particle.lifeTime < Math::EPSLON || particle.currentLife > particle.lifeTime) {
                Particle& endP = particles[alive - 1];
                particle = endP;
                --i;
                --alive;
            }
        }
    }

    void ParticlePool::Reset()
    {
        alive = 0;
    }

    uint32_t ParticlePool::Size() const
    {
        return maxSize;
    }

    uint32_t ParticlePool::Alive() const
    {
        return alive;
    }

    const std::vector<Particle>& ParticlePool::ParticleData() const
    {
        return particles;
    }
}
