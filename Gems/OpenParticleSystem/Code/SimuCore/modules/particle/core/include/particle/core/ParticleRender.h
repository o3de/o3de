/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SIMUPARTICLE_CORE_PARTICLE_RENDER_H
#define SIMUPARTICLE_CORE_PARTICLE_RENDER_H

#include "particle/core/Particle.h"
#include "particle/core/ParticleDelegate.h"
#include <AzCore/Debug/Profiler.h>

namespace SimuCore::ParticleCore {
    class ParticlePool;
    struct WorldInfo;

    class ParticleRender {
    public:
        ParticleRender() = default;
        virtual ~ParticleRender() = default;

        virtual void Render(
            const uint8_t* data, const BaseInfo& emitterInfo, uint8_t* driver, const ParticlePool& pool, const WorldInfo& world, DrawItem& item) = 0;

        virtual RenderType GetType() const
        {
            return RenderType::UNDEFINED;
        }

        virtual uint32_t DataSize() const
        {
            return 0;
        }
    };
}

#endif // SIMUPARTICLE_CORE_PARTICLE_RENDER_H
