/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <map>
#include <AzCore/std/containers/unordered_map.h>
#include "particle/core/Particle.h"
#include "particle/core/ParticleRender.h"

namespace SimuCore::ParticleCore {
    class ParticleRibbonRender : public ParticleRender {
    public:
        ParticleRibbonRender() = default;
        ~ParticleRibbonRender() override;

        void Render(const AZ::u8* data, const BaseInfo& emitterInfo, AZ::u8* driver, const ParticlePool& pool, const WorldInfo& world,
            DrawItem& item) override;

        RenderType GetType() const override
        {
            return RenderType::RIBBON;
        }

        AZ::u32 DataSize() const override;

    private:
        struct RibbonSegment {
            AZ::Vector3 tangent0 = { 0.f, 0.f, 0.f };
            AZ::Vector3 tangent1 = { 0.f, 0.f, 0.f };
            AZ::u32 head; // particleIndex
            AZ::u32 end;  // particleIndex
            AZ::u32 interpCount;
            float segmentLength;
            float tileV = 0.f;
            float distance = 0.f; // traveling distance to the first particle in ribbon
        };

        struct BufferInfo {
            AZ::Vector3 pos;
            AZ::Color color;
            AZ::Vector3 right;
            float width;
            AZ::u32& vertexIdx;
            AZ::u32& indexIdx;
            float& texV;
            AZStd::vector<ParticleRibbonVertex>& vb;
            AZStd::vector<AZ::u32>& ib;
        };

        void SortParticles(const ParticlePool& pool, AZStd::vector<AZ::Vector3>& positionBuffer, const BaseInfo& emitterInfo, const RibbonConfig& config);

        void CalculateParticlesInRibbons(const RibbonConfig& config);
        void CalculateParticlesInRibbon(const AZStd::vector<std::pair<AZ::u32, float>>& sortedIndices,
            AZStd::vector<AZ::u32>& indicesInRibbon, AZStd::vector<RibbonSegment>& segments, float& totalDistance,
            const RibbonConfig& config);

        void Reset();

        void FillVertexAndIndexBuffer(const WorldInfo& world, const RibbonConfig& config,
            AZStd::vector<ParticleRibbonVertex>& vb, AZStd::vector<AZ::u32>& ib);

        void FillVertex(BufferInfo& info) const;
        void FillHeadVertex(BufferInfo& info) const;
        void FillEndVertex(BufferInfo& info) const;

        void UpdateVertexBuffer(BufferView& bufferView, AZStd::vector<ParticleRibbonVertex>& vb, bool reCreate) const;
        void UpdateIndexBuffer(BufferView& bufferView, AZStd::vector<AZ::u32>& ib, bool reCreate) const;

        void UpdateBuffer(const WorldInfo& world, const RibbonConfig& config);

        AZ::Vector3 CalRightVector(const WorldInfo& world, const RibbonConfig& config,
            const AZ::Vector3& up, const AZ::Vector3& position) const;

        const Particle* particle = nullptr;
        AZ::u8* gDriver = nullptr;

        AZ::u32 segmentCount = 0;
        AZ::u32 ribbonCount = 0;
        AZ::u32 newVbSize = 0;
        AZ::u32 newIbSize = 0;

        std::map<AZ::u64, AZStd::vector<std::pair<AZ::u32, float>>> sortedParticleIndices;
        AZStd::unordered_map<AZ::u64, AZStd::vector<RibbonSegment>> ribbonSegments;
        AZStd::unordered_map<AZ::u64, float> ribbonDistances;

        AZStd::unordered_map<AZ::u64, AZStd::vector<ParticleRibbonVertex>> vbs;
        AZStd::unordered_map<AZ::u64, BufferView> vertexBufferViews;
        AZStd::unordered_map<AZ::u64, AZStd::vector<AZ::u32>> ibs;
        AZStd::unordered_map<AZ::u64, BufferView> indexBufferViews;
    };

    constexpr AZ::u32 INDEX_COUNT_IN_ONE_SEGMENT = 6;
}

