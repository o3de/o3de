/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <map>
#include <unordered_map>
#include "particle/core/Particle.h"
#include "particle/core/ParticleRender.h"

namespace SimuCore::ParticleCore {
    class ParticleRibbonRender : public ParticleRender {
    public:
        ParticleRibbonRender() = default;
        ~ParticleRibbonRender() override;

        void Render(const uint8_t* data, const BaseInfo& emitterInfo, uint8_t* driver, const ParticlePool& pool, const WorldInfo& world,
            DrawItem& item) override;

        RenderType GetType() const override
        {
            return RenderType::RIBBON;
        }

        uint32_t DataSize() const override;

    private:
        struct RibbonSegment {
            Vector3 tangent0 = { 0.f, 0.f, 0.f };
            Vector3 tangent1 = { 0.f, 0.f, 0.f };
            uint32_t head; // particleIndex
            uint32_t end;  // particleIndex
            uint32_t interpCount;
            float segmentLength;
            float tileV = 0.f;
            float distance = 0.f; // traveling distance to the first particle in ribbon
        };

        struct BufferInfo {
            Vector3 pos;
            AZ::Color color;
            Vector3 right;
            float width;
            uint32_t& vertexIdx;
            uint32_t& indexIdx;
            float& texV;
            std::vector<ParticleRibbonVertex>& vb;
            std::vector<uint32_t>& ib;
        };

        void SortParticles(const ParticlePool& pool, std::vector<Vector3>& positionBuffer, const BaseInfo& emitterInfo, const RibbonConfig& config);

        void CalculateParticlesInRibbons(const RibbonConfig& config);
        void CalculateParticlesInRibbon(const std::vector<std::pair<uint32_t, float>>& sortedIndices,
            std::vector<uint32_t>& indicesInRibbon, std::vector<RibbonSegment>& segments, float& totalDistance,
            const RibbonConfig& config);

        void Reset();

        void FillVertexAndIndexBuffer(const WorldInfo& world, const RibbonConfig& config,
            std::vector<ParticleRibbonVertex>& vb, std::vector<uint32_t>& ib);

        void FillVertex(BufferInfo& info) const;
        void FillHeadVertex(BufferInfo& info) const;
        void FillEndVertex(BufferInfo& info) const;

        void UpdateVertexBuffer(BufferView& bufferView, std::vector<ParticleRibbonVertex>& vb, bool reCreate) const;
        void UpdateIndexBuffer(BufferView& bufferView, std::vector<uint32_t>& ib, bool reCreate) const;

        void UpdateBuffer(const WorldInfo& world, const RibbonConfig& config);

        Vector3 CalRightVector(const WorldInfo& world, const RibbonConfig& config,
            const Vector3& up, const Vector3& position) const;

        const Particle* particle = nullptr;
        uint8_t* gDriver = nullptr;

        uint32_t segmentCount = 0;
        uint32_t ribbonCount = 0;
        uint32_t newVbSize = 0;
        uint32_t newIbSize = 0;

        std::map<uint64_t, std::vector<std::pair<uint32_t, float>>> sortedParticleIndices;
        std::unordered_map<uint64_t, std::vector<RibbonSegment>> ribbonSegments;
        std::unordered_map<uint64_t, float> ribbonDistances;

        std::unordered_map<uint64_t, std::vector<ParticleRibbonVertex>> vbs;
        std::unordered_map<uint64_t, BufferView> vertexBufferViews;
        std::unordered_map<uint64_t, std::vector<uint32_t>> ibs;
        std::unordered_map<uint64_t, BufferView> indexBufferViews;
    };

    constexpr uint32_t INDEX_COUNT_IN_ONE_SEGMENT = 6;
}

