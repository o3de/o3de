/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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
            const AZ::u8* data, const BaseInfo& emitterInfo, AZ::u8* driver, const ParticlePool& pool, const WorldInfo& world, DrawItem& item) = 0;

        virtual RenderType GetType() const
        {
            return RenderType::UNDEFINED;
        }

        virtual AZ::u32 DataSize() const
        {
            return 0;
        }
    };
}
