/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleRibbonRender.h"
#include "particle/core/ParticlePool.h"
#include "particle/core/ParticleDriver.h"
#include "particle/core/ParticleHelper.h"

namespace SimuCore::ParticleCore {
    ParticleRibbonRender::~ParticleRibbonRender()
    {
        for (auto& view : vertexBufferViews) {
            ParticleDriver::bufferDestroyFn(gDriver, view.second.buffer);
        }
        for (auto& view : indexBufferViews) {
            ParticleDriver::bufferDestroyFn(gDriver, view.second.buffer);
        }
        particle = nullptr;
        gDriver = nullptr;
    }

    void ParticleRibbonRender::Render(const uint8_t* data, const BaseInfo& emitterInfo, uint8_t* driver, const ParticlePool& pool,
            const WorldInfo& world, DrawItem& item)
    {
        if (data == nullptr || pool.Alive() == 0) {
            return;
        }
        const RibbonConfig& config = *reinterpret_cast<const RibbonConfig*>(data);
        particle = pool.ParticleData().data();

        SortParticles(pool, item.positionBuffer, emitterInfo, config);
        CalculateParticlesInRibbons(config);
        if (segmentCount == 0 || ribbonCount == 0) {
            return;
        }
        gDriver = driver;
        UpdateBuffer(world, config);

        item.type = RenderType::RIBBON;
        item.drawArgs.type = DrawType::INDEXED;
        item.drawArgs.indexed.indexCount = segmentCount * INDEX_COUNT_IN_ONE_SEGMENT;
        item.drawArgs.indexed.instanceCount = ribbonCount;
        item.vertexBuffer = vertexBufferViews[world.viewKey.v];
        item.indexBuffer = indexBufferViews[world.viewKey.v];
    }

    void ParticleRibbonRender::SortParticles(const ParticlePool& pool, std::vector<Vector3>& positionBuffer, const BaseInfo& emitterInfo,
            const RibbonConfig& config)
    {
        sortedParticleIndices.clear();
        for (uint32_t current = 0; current < pool.Alive(); ++current) {
            Particle curParticleCopy = particle[current];
            if (curParticleCopy.hasLightEffect) {
                positionBuffer[current] = curParticleCopy.globalPosition;
            }
            float width = CalcDistributionTickValue(config.ribbonWidth, emitterInfo, curParticleCopy);
            if ((!config.inheritSize && config.mode == TrailMode::TRAIL) || config.mode == TrailMode::RIBBON) {
                const_cast<Particle&>(particle[current]).scale = Vector3(width);
            }

            if (config.mode == TrailMode::RIBBON && config.ribbonParam.ribbonCount > 0) {
                curParticleCopy.ribbonId = curParticleCopy.id % static_cast<uint64_t>(config.ribbonParam.ribbonCount);
            }

            const auto it = sortedParticleIndices.find(curParticleCopy.ribbonId);
            if (it == sortedParticleIndices.end()) {
                std::vector<std::pair<uint32_t, float>> sortedParticles;
                (void)sortedParticles.emplace_back(std::make_pair(current, curParticleCopy.currentLife));
                (void)sortedParticleIndices.emplace(curParticleCopy.ribbonId, sortedParticles);
            } else {
                (void)it->second.emplace_back(std::make_pair(current, curParticleCopy.currentLife));
            }
        }

        if (config.mode == TrailMode::TRAIL) {
            uint32_t trailCount = static_cast<uint32_t>(
                std::ceil(sortedParticleIndices.size() * config.trailParam.ratio));
            for (auto iter = sortedParticleIndices.begin(); iter != sortedParticleIndices.end();) {
                if (trailCount == 0) {
                    (void)sortedParticleIndices.erase(iter++);
                    continue;
                }

                std::sort(iter->second.begin(), iter->second.end(),
                    [](const std::pair<uint32_t, float>& o1, const std::pair<uint32_t, float>& o2) {
                    return o1.second < o2.second;
                });
                trailCount--;
                ++iter;
            }
        } else {
            for (auto iter = sortedParticleIndices.begin(); iter != sortedParticleIndices.end(); ++iter) {
                std::sort(iter->second.begin(), iter->second.end(),
                    [](const std::pair<uint32_t, float>& o1, const std::pair<uint32_t, float>& o2) {
                    return o1.second > o2.second;
                });
            }
        }
    }

    void ParticleRibbonRender::CalculateParticlesInRibbons(const RibbonConfig& config)
    {
        Reset();
        for (auto iter = sortedParticleIndices.begin(); iter != sortedParticleIndices.end(); ++iter) {
            std::vector<uint32_t> indicesInRibbon;
            std::vector<RibbonSegment> segments;
            float totalDistance = 0.f;
            CalculateParticlesInRibbon(iter->second, indicesInRibbon, segments, totalDistance, config);
            if (indicesInRibbon.size() <= 1) {
                continue;
            }
            Vector3& fistTangent =  segments[0].tangent0;
            Vector3& nextToFirstTangent = segments[0].tangent1;
            fistTangent = (2.f * fistTangent.Dot(nextToFirstTangent)) * fistTangent - nextToFirstTangent;

            Vector3& lastTangent =  segments[segments.size() - 1].tangent1;
            Vector3& preToLastTangent = segments[segments.size() - 1].tangent0;
            lastTangent = (2.f * lastTangent.Dot(preToLastTangent)) * lastTangent - preToLastTangent;

            ribbonCount++;
            (void)ribbonSegments.emplace(iter->first, segments);
            (void)ribbonDistances.emplace(iter->first, totalDistance);
        }
    }

    void ParticleRibbonRender::CalculateParticlesInRibbon(const std::vector<std::pair<uint32_t, float>>& sortedIndices,
        std::vector<uint32_t>& indicesInRibbon, std::vector<RibbonSegment>& segments, float& totalDistance,
        const RibbonConfig& config)
    {
        for (uint32_t i = 0; i < sortedIndices.size(); i++) {
            if (i == 0) {
                (void)indicesInRibbon.emplace_back(sortedIndices[i].first);
                continue;
            }

            uint32_t lastIndex = indicesInRibbon.back();
            Particle lastParticle = particle[lastIndex];
            uint32_t currentIndex = sortedIndices[i].first;
            Particle currentParticle = particle[currentIndex];
            Vector3 curDir = currentParticle.globalPosition - lastParticle.globalPosition;
            float localDistance = curDir.Length();
            if (localDistance <= Math::EPSLON || (localDistance <
                config.minRibbonSegmentLength && i !=  sortedIndices.size() - 1)) {
                continue;
            }

            Vector3 lastDir = VEC3_ZERO;
            if (indicesInRibbon.size() > 1) {
                uint32_t lastLast = indicesInRibbon[indicesInRibbon.size() - 2];
                Particle lastLastParticle = particle[lastLast];
                lastDir = lastParticle.globalPosition - lastLastParticle.globalPosition;
                if (lastDir.Length() > Math::EPSLON) {
                    (void)lastDir.Normalize();
                }
            }

            Vector3 dir = curDir + lastDir;
            RibbonSegment segment;
            (void)indicesInRibbon.emplace_back(currentIndex);
            segment.head = lastIndex;
            segment.end = currentIndex;
            segment.segmentLength = localDistance;
            segment.tangent0 = segments.empty() ? curDir.Normalize() : segments[segments.size() - 1].tangent1;
            segment.tangent1 = (1.f - config.curveTension) * (dir.Normalize());
            segment.interpCount = static_cast<uint32_t>(std::ceil(localDistance / config.tesselationFactor));
            segmentCount += segment.interpCount;
            totalDistance += localDistance;
            segment.tileV = totalDistance / config.tilingDistance;
            segment.distance = totalDistance;
            segments.emplace_back(segment);
        }
    }

    void ParticleRibbonRender::Reset()
    {
        ribbonSegments.clear();
        ribbonDistances.clear();
        segmentCount = 0;
        ribbonCount = 0;
    }

    void ParticleRibbonRender::UpdateBuffer(const WorldInfo& world, const RibbonConfig& config)
    {
        auto& vertexBufferView = vertexBufferViews[world.viewKey.v];
        auto& vb = vbs[world.viewKey.v];

        auto& indexBufferView = indexBufferViews[world.viewKey.v];
        auto& ib = ibs[world.viewKey.v];

        bool reCreateVb = false;
        uint32_t vertexCount = (segmentCount + ribbonCount) * 2;
        if (vertexCount > vb.size()) {
            newVbSize = vertexCount;
            vb.resize(newVbSize);
            ParticleDriver::bufferDestroyFn(gDriver, vertexBufferView.buffer);
            reCreateVb = true;
        }

        bool reCreateIb = false;
        uint32_t indexCount = segmentCount * INDEX_COUNT_IN_ONE_SEGMENT;
        if (indexCount > ib.size()) {
            newIbSize = indexCount;
            ib.resize(newIbSize);
            ParticleDriver::bufferDestroyFn(gDriver, indexBufferView.buffer);
            reCreateIb = true;
        }

        FillVertexAndIndexBuffer(world, config, vb, ib);
        UpdateVertexBuffer(vertexBufferView, vb, reCreateVb);
        UpdateIndexBuffer(indexBufferView, ib, reCreateIb);
    }

    void ParticleRibbonRender::FillVertexAndIndexBuffer(const WorldInfo& world, const RibbonConfig& config,
        std::vector<ParticleRibbonVertex>& vb, std::vector<uint32_t>& ib)
    {
        bool bTileV = config.tilingDistance > 0.f;
        uint32_t vertexIdx = 0; // vertex index
        uint32_t indexIdx = 0; // indices index
        for (auto iter = ribbonSegments.begin(); iter != ribbonSegments.end(); ++iter) {
            float preTileV = 0.f;
            float travelingDistance = 0.f;
            auto totalDistance = ribbonDistances.at(iter->first);
            for (uint32_t segmentId = 0; segmentId < iter->second.size(); segmentId++) {
                RibbonSegment segment = iter->second[segmentId];
                Particle head = particle[segment.head];
                Particle end = particle[segment.end];

                float curTexV = bTileV ? preTileV : travelingDistance / totalDistance;
                Vector3 right = CalRightVector(world, config, segment.tangent0, head.globalPosition);
                BufferInfo bInfo{head.globalPosition, head.color,
                    right, head.scale.x, vertexIdx, indexIdx, curTexV, vb, ib};
                (segmentId == 0) ? FillHeadVertex(bInfo) : FillVertex(bInfo);

                for (uint32_t interpId = 1; interpId < segment.interpCount; interpId++) {
                    float step = interpId * 1.0f / segment.interpCount;
                    std::pair<Vector3, Vector3> pair0 = {head.globalPosition, segment.tangent0};
                    std::pair<Vector3, Vector3> pair1 = {end.globalPosition, segment.tangent1};
                    auto pos = Math::CubicInterp<Vector3>(pair0, pair1, step, segment.segmentLength);
                    AZ::Color color = head.color.Lerp(end.color, step);
                    auto width = Math::Lerp<float>(head.scale.x, end.scale.x, step);
                    curTexV = bTileV ? Math::Lerp<float>(preTileV, segment.tileV, step) :
                            (pos.Distance(head.globalPosition) + travelingDistance) / totalDistance;
                    Vector3 up = segment.tangent0.Lerp(segment.tangent1, step);
                    right = CalRightVector(world, config, up, head.globalPosition);
                    BufferInfo info{pos, color, right, width, vertexIdx, indexIdx, curTexV, vb, ib};
                    FillVertex(info);
                }

                if (segmentId == iter->second.size() - 1) {
                    right = CalRightVector(world, config, segment.tangent1, end.globalPosition).Normalize();
                    curTexV = bTileV ? segment.tileV : segment.distance / totalDistance;
                    BufferInfo info{end.globalPosition, end.color,
                        right, end.scale.x, vertexIdx, indexIdx, curTexV, vb, ib};
                    FillEndVertex(info);
                    continue;
                }
                preTileV = segment.tileV;
                travelingDistance = segment.distance;
            }
        }
    }

    void ParticleRibbonRender::FillHeadVertex(BufferInfo& info) const
    {
        ParticleRibbonVertex* prv = info.vb.data();
        uint32_t* idx = info.ib.data();

        prv[info.vertexIdx].position = Vector4(info.pos - info.right * info.width * 0.5f, 0.f);
        prv[info.vertexIdx].color = info.color;
        prv[info.vertexIdx].uv = AZ::Vector2(0.f, info.texV);
        idx[info.indexIdx++] = info.vertexIdx++;

        prv[info.vertexIdx].position = Vector4(info.pos + info.right * info.width * 0.5f, 0.f);
        prv[info.vertexIdx].color = info.color;
        prv[info.vertexIdx].uv = AZ::Vector2(1.f, info.texV);
        idx[info.indexIdx++] = info.vertexIdx++;
    }

    void ParticleRibbonRender::FillVertex(BufferInfo& info) const
    {
        ParticleRibbonVertex* prv = info.vb.data();
        uint32_t* idx = info.ib.data();
        prv[info.vertexIdx].position = Vector4(info.pos - info.right * info.width * 0.5f, 0.f);
        prv[info.vertexIdx].color = info.color;
        prv[info.vertexIdx].uv = AZ::Vector2(0.f, info.texV);
        idx[info.indexIdx++] = info.vertexIdx++;

        idx[info.indexIdx] = idx[info.indexIdx - 2];
        info.indexIdx++;
        prv[info.vertexIdx].position = Vector4(info.pos + info.right * info.width * 0.5f, 0.f);
        prv[info.vertexIdx].color = info.color;
        prv[info.vertexIdx].uv = AZ::Vector2(1.f, info.texV);
        idx[info.indexIdx++] = info.vertexIdx++;

        idx[info.indexIdx] = idx[info.indexIdx - 3];
        info.indexIdx++;
        idx[info.indexIdx] = idx[info.indexIdx - 1];
        info.indexIdx++;
        idx[info.indexIdx] = idx[info.indexIdx - 3];
        info.indexIdx++;
    }

    void ParticleRibbonRender::FillEndVertex(BufferInfo& info) const
    {
        ParticleRibbonVertex* prv = info.vb.data();
        uint32_t* idx = info.ib.data();
        prv[info.vertexIdx].position = Vector4(info.pos - info.right * info.width * 0.5f, 0.f);
        prv[info.vertexIdx].color = info.color;
        prv[info.vertexIdx].uv = AZ::Vector2(0.f, info.texV);
        idx[info.indexIdx++] = info.vertexIdx++;

        idx[info.indexIdx] = idx[info.indexIdx - 2];
        info.indexIdx++;
        prv[info.vertexIdx].position = Vector4(info.pos + info.right * info.width * 0.5f, 0.f);
        prv[info.vertexIdx].color = info.color;
        prv[info.vertexIdx].uv = AZ::Vector2(1.f, info.texV);
        idx[info.indexIdx++] = info.vertexIdx++;
        idx[info.indexIdx] = idx[info.indexIdx - 3];
        info.indexIdx++;
    }

    Vector3 ParticleRibbonRender::CalRightVector(const WorldInfo& world, const RibbonConfig& config,
        const Vector3& up, const Vector3& position) const
    {
        Vector3 facing;
        switch (config.facing) {
            case RibbonFacing::CAMERA:
                facing = world.cameraPosition - position;
                break;
            case RibbonFacing::SCREEN:
            default:
                facing = world.axisZ;
                break;
        }
        return facing.Cross(up).Normalize();
    }

    void ParticleRibbonRender::UpdateVertexBuffer(
        BufferView& bufferView, std::vector<ParticleRibbonVertex>& vb, bool reCreate) const
    {
        if (reCreate || bufferView.buffer.data.ptr == nullptr) {
            BufferCreate info = {};
            info.size = newVbSize * static_cast<uint32_t>(sizeof(ParticleRibbonVertex));
            info.data = reinterpret_cast<const uint8_t*>(vb.data());
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            ParticleDriver::bufferCreateFn(gDriver, info, bufferView.buffer);
            bufferView.offset = 0;
            bufferView.size = newVbSize * static_cast<uint32_t>(sizeof(ParticleRibbonVertex));
            bufferView.stride = sizeof(ParticleRibbonVertex);
        } else {
            BufferUpdate info = {};
            info.usage = BufferUsage::VERTEX;
            info.memory = MemoryType::DYNAMIC;
            info.size = newVbSize * static_cast<uint32_t>(sizeof(ParticleRibbonVertex));
            info.data = reinterpret_cast<const uint8_t*>(vb.data());
            ParticleDriver::bufferUpdateFn(gDriver, info, bufferView.buffer);
        }
    }

    void ParticleRibbonRender::UpdateIndexBuffer(
        BufferView& bufferView, std::vector<uint32_t>& ib, bool reCreate) const
    {
        if (reCreate || bufferView.buffer.data.ptr == nullptr) {
            BufferCreate info = {};
            info.size = newIbSize * static_cast<uint32_t>(sizeof(uint32_t));
            info.data = reinterpret_cast<const uint8_t*>(ib.data());
            info.usage = BufferUsage::INDEX;
            info.memory = MemoryType::DYNAMIC;
            ParticleDriver::bufferCreateFn(gDriver, info, bufferView.buffer);
            bufferView.offset = 0;
            bufferView.size = newIbSize * static_cast<uint32_t>(sizeof(uint32_t));
            bufferView.stride = sizeof(uint32_t);
        } else {
            BufferUpdate info = {};
            info.usage = BufferUsage::INDEX;
            info.memory = MemoryType::DYNAMIC;
            info.size = newIbSize * static_cast<uint32_t>(sizeof(uint32_t));
            info.data = reinterpret_cast<const uint8_t*>(ib.data());
            ParticleDriver::bufferUpdateFn(gDriver, info, bufferView.buffer);
        }
    }

    uint32_t ParticleRibbonRender::DataSize() const
    {
        return sizeof(RibbonConfig);
    }
}
