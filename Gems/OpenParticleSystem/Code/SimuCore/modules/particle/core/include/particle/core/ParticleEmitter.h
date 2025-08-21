/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <atomic>
#include <vector>
#include "particle/core/Particle.h"
#include "particle/core/ParticleDataPool.h"
#include "particle/core/ParticleDelegate.h"
#include "particle/core/ParticleEffector.h"
#include "particle/core/ParticlePool.h"
#include "particle/core/ParticleCurve.h"
#include "particle/core/ParticleRandom.h"
#include "particle/core/ParticleRender.h"

namespace SimuCore::ParticleCore {
    struct EmitterCreateInfo {
        ParticleDataPool* dataPool;
        RandomStream* stream;
        ParticleEventPool* eventPool;
        uint32_t id;
    };

    template <typename T>
    struct ParticleEffectorInfo {
        T* effector;
        uint32_t offset;
        uint8_t* data;
    };

    class ParticleEmitter {
    public:
        struct Config {
            uint32_t maxSize = 500;
            float startTime = 0.f;
            float duration = 2.f;
            bool localSpace = false;
            SimulateType type = SimulateType::CPU;
            bool loop = true;
        };

        ParticleEmitter(const ParticleEmitter::Config& cfg, EmitterCreateInfo createInfo);

        ParticleEmitter(uint32_t data, EmitterCreateInfo createInfo);

        ~ParticleEmitter();

        template<typename Effector>
        ParticleEmitEffector* AddEmitterEffectorData(uint32_t data)
        {
            auto rst = new ParticleEmitEffector(DataArgs<Effector>{});
            ParticleEffectorInfo<ParticleEmitEffector> info;
            info.effector = rst;
            info.offset = data;
            info.data = dataPool->Data(rst->DataSize(), data);
            (void)emitEffectors.emplace_back(info);
            return rst;
        }

        template<typename Effector>
        ParticleEmitEffector* AddEmitterEffector(const typename Effector::DataType& data)
        {
            return AddEmitterEffectorData<Effector>(dataPool->AllocT(data));
        }

        template<typename Effector>
        ParticleSpawnEffector* AddSpawnEffectorData(uint32_t data)
        {
            auto rst = new ParticleSpawnEffector(DataArgs<Effector>{});
            ParticleEffectorInfo<ParticleSpawnEffector> info;
            info.effector = rst;
            info.offset = data;
            info.data = dataPool->Data(rst->DataSize(), data);
            (void)spawnEffectors.emplace_back(info);
            return rst;
        }

        template<typename Effector>
        ParticleSpawnEffector* AddSpawnEffector(const typename Effector::DataType& data)
        {
            return AddSpawnEffectorData<Effector>(dataPool->AllocT(data));
        }

        template<typename Effector>
        ParticleUpdateEffector* AddUpdateEffectorData(uint32_t data)
        {
            auto rst = new ParticleUpdateEffector(DataArgs<Effector>{});
            ParticleEffectorInfo<ParticleUpdateEffector> info;
            info.effector = rst;
            info.offset = data;
            info.data = dataPool->Data(rst->DataSize(), data);
            (void)updateEffectors.emplace_back(info);
            return rst;
        }

        template<typename Effector>
        ParticleUpdateEffector* AddUpdateEffector(const typename Effector::DataType& data)
        {
            return AddUpdateEffectorData<Effector>(dataPool->AllocT(data));
        }

        template<typename Effector>
        ParticleEventEffector* AddEventEffectorData(uint32_t data)
        {
            auto rst = new ParticleEventEffector(DataArgs<Effector>{});
            ParticleEffectorInfo<ParticleEventEffector> info;
            info.effector = rst;
            info.offset = data;
            info.data = dataPool->Data(rst->DataSize(), data);
            (void)eventEffectors.emplace_back(info);
            return rst;
        }

        template<typename Effector>
        ParticleEventEffector* AddEventEffector(const typename Effector::DataType& data)
        {
            return AddEventEffectorData<Effector>(dataPool->AllocT(data));
        }

        template<typename Config>
        ParticleRender* SetParticleRender(const Config& cfg)
        {
            ParticleRender* rst = AddParticleInternal(Config::RENDER_TYPE);
            if (rst != nullptr) {
                render = { rst, dataPool->AllocT(cfg) };
            }
            return rst;
        }

        template<typename Config>
        Config* GetRenderConfig()
        {
            if (render.first != nullptr) {
                return dataPool->Data<Config>(render.second);
            }
            return nullptr;
        }

        void SetSkeletonMesh(const Vector3* bone, uint32_t bCount, const Vector3* vertex, uint32_t vCount, const uint32_t* indice, uint32_t iCount,
            const double* area)
        {
            boneStream = bone;
            boneCount = bCount;
            vertexStream = vertex;
            vertexCount = vCount;
            indiceStream = indice;
            indiceCount = iCount;
            areaStream = area;
        }

        ParticleRender* SetParticleRender(uint32_t data, RenderType type);

        RenderType GetRenderType() const;

        float Simulate(float delta);

        void Tick(float delta);

        void Render(uint8_t* driver, const WorldInfo& world, DrawItem& item);

        float EndTime() const;

        const std::vector<ParticleEffectorInfo<ParticleEmitEffector>>& GetEmitEffectors() const;

        const std::vector<ParticleEffectorInfo<ParticleSpawnEffector>>& GetSpawnEffectors() const;

        const std::vector<ParticleEffectorInfo<ParticleUpdateEffector>>& GetUpdateEffectors() const;

        const std::vector<ParticleEffectorInfo<ParticleEventEffector>>& GetEventEffectors() const;

        const std::pair<ParticleRender*, uint32_t>& GetRender() const;

        uint32_t GetConfig() const;

        ParticleEmitter::Config* GetEmitterConfig() const;

        void SetMoveDistance(float distance);

        void SetEmitterTransform(const SimuCore::Transform& transform);

        void GatherSimpleLight(std::vector<LightParticle>& outParticleLights);

        const uint32_t GetEmitterId() const;

        const Transform& GetEmitterTransform() const;

        const uint32_t GetRenderSort() const;

        void SetMeshSampleType(MeshSampleType meshType);

        const MeshSampleType GetMeshSampleType() const;

        bool HasSkeletonModule() const;

        bool HasLightModule() const;

        void SetAabbExtends(const Vector3& max, const Vector3& min);

        void SetWorldFront(const Vector3& front);

        void UpdateDistPtr(const Distribution& distribution);

        void PrepareSimulation();

    private:
        EmitSpawnParam Emmit(float delta);
        uint32_t Spawn(const ParticleEventInfo* eventInfo, const InheritanceSpawn* inheritance, uint32_t newParticleNum);

        uint32_t Spawn(const std::vector<ParticleEventInfo>& spawnEvents, const std::vector<InheritanceSpawn*>& spawnInheritances, uint32_t newParticleNum);

        void HandleEvents(const ParticleEventInfo* eventInfo, const InheritanceSpawn* inheritance, Particle& particle);

        float UpdateParticle(float delta);

        void Update(float delta, uint32_t begin);

        void Update(const std::vector<ParticleEventInfo>& spawnEvents, const std::vector<InheritanceSpawn*>& spawnInheritances, uint32_t begin);

        void ResetRender();

        void ResetEffectors();

        void ResetEventPool(uint32_t emitterId, ParticleEventType type);

        ParticleRender* AddParticleInternal(RenderType type) const;

        uint32_t config;
        ParticleDataPool* dataPool;
        RandomStream* randomStream;
        ParticleEventPool* systemEventPool;
        uint32_t emitterID = UINT32_MAX;
        std::atomic_uint64_t particleIdentity = 0;

        float currTime = 0.f;
        bool started = false;
        float moveDistance = 0.f;
        Transform emitterTransform;
        MeshSampleType meshSampleType = MeshSampleType::BONE;

        ParticlePool particlePool;
        const Vector3* boneStream = nullptr;
        uint32_t boneCount = 0;
        const Vector3* vertexStream = nullptr;
        uint32_t vertexCount = 0;
        const uint32_t* indiceStream = nullptr;
        const double* areaStream = nullptr;
        uint32_t indiceCount = 0;
        Vector3 maxExtend = VEC3_ZERO;
        Vector3 minExtend = VEC3_ZERO;
        Vector3 worldFront = VEC3_UNIT_Z;
        std::unordered_map<uint32_t, InheritanceSpawn> emitterInheritances;

        std::pair<ParticleRender*, uint32_t> render;
        std::vector<ParticleEffectorInfo<ParticleEmitEffector>> emitEffectors;
        std::vector<ParticleEffectorInfo<ParticleSpawnEffector>> spawnEffectors;
        std::vector<ParticleEffectorInfo<ParticleUpdateEffector>> updateEffectors;
        std::vector<ParticleEffectorInfo<ParticleEventEffector>> eventEffectors;
    };
}
