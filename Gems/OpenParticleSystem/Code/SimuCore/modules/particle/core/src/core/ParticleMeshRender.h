/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include "particle/core/Particle.h"
#include "particle/core/ParticleRender.h"

namespace SimuCore::ParticleCore {
    class ParticleMeshRender : public ParticleRender {
    public:
        ParticleMeshRender() = default;
        ~ParticleMeshRender() override;

        void Render(const AZ::u8* data, const BaseInfo& emitterInfo, AZ::u8* driver, const ParticlePool& pool, const WorldInfo& world,
            DrawItem& item) override;

        RenderType GetType() const override
        {
            return RenderType::MESH;
        }

        AZ::u32 DataSize() const override;

    private:
        void UpdateBuffer(const ParticlePool& pool, const WorldInfo& world, AZStd::vector<AZ::Vector3>& positionBuffer);

        AZ::u8* gDriver = nullptr;
        AZ::u32 particleSize = 0;
        AZStd::unordered_map<AZ::u64, AZStd::vector<ParticleMeshVertex>> vbs;
        AZStd::unordered_map<AZ::u64, BufferView> bufferViews;
    };
}

