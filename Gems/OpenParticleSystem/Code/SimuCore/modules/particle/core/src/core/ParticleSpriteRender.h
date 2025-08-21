/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <unordered_map>
#include "particle/core/Particle.h"
#include "particle/core/ParticleRender.h"

namespace SimuCore::ParticleCore {
    class ParticleSpriteRender : public ParticleRender {
    public:
        ParticleSpriteRender() = default;
        ~ParticleSpriteRender() override;

        void Render(const uint8_t* data, const BaseInfo& emitterInfo, uint8_t* driver, const ParticlePool& pool, const WorldInfo& world,
            DrawItem& item) override;

        RenderType GetType() const override
        {
            return RenderType::SPRITE;
        }

        uint32_t DataSize() const override;

    private:
        void UpdateBuffer(const ParticlePool& pool, const SpriteConfig& config, const WorldInfo& world, DrawItem& item);

        uint8_t* gDriver = nullptr;

        uint32_t particleSize = 0;
        std::unordered_map<uint64_t, std::vector<ParticleSpriteVertex>> vbs;
        std::unordered_map<uint64_t, BufferView> bufferViews;
    };
}

