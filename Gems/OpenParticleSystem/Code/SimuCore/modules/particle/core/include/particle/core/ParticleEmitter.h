/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/containers/vector.h>
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
        AZ::u32 id;
    };

    template <typename T>
    struct ParticleEffectorInfo {
        T* effector;
        AZ::u32 offset;
        AZ::u8* data;
    };

    class ParticleEmitter {
    public:
        struct Config {
            AZ::u32 maxSize = 500;
            float startTime = 0.f;
            float duration = 2.f;
            bool localSpace = false;
            SimulateType type = SimulateType::CPU;
            bool loop = true;
        };

        ParticleEmitter(const ParticleEmitter::Config& cfg, EmitterCreateInfo createInfo);

        ParticleEmitter(AZ::u32 data, EmitterCreateInfo createInfo);

        ~ParticleEmitter();

        template<typename Effector>
        ParticleEmitEffector* AddEmitterEffectorData(AZ::u32 data)
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
        ParticleSpawnEffector* AddSpawnEffectorData(AZ::u32 data)
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
        ParticleUpdateEffector* AddUpdateEffectorData(AZ::u32 data)
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
        ParticleEventEffector* AddEventEffectorData(AZ::u32 data)
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

        void SetSkeletonMesh(const AZ::Vector3* bone, AZ::u32 bCount, const AZ::Vector3* vertex, AZ::u32 vCount, const AZ::u32* indice, AZ::u32 iCount,
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

        ParticleRender* SetParticleRender(AZ::u32 data, RenderType type);

        RenderType GetRenderType() const;

        float Simulate(float delta);

        void Tick(float delta);

        void Render(AZ::u8* driver, const WorldInfo& world, DrawItem& item);

        float EndTime() const;

        const AZStd::vector<ParticleEffectorInfo<ParticleEmitEffector>>& GetEmitEffectors() const;

        const AZStd::vector<ParticleEffectorInfo<ParticleSpawnEffector>>& GetSpawnEffectors() const;

        const AZStd::vector<ParticleEffectorInfo<ParticleUpdateEffector>>& GetUpdateEffectors() const;

        const AZStd::vector<ParticleEffectorInfo<ParticleEventEffector>>& GetEventEffectors() const;

        const std::pair<ParticleRender*, AZ::u32>& GetRender() const;

        AZ::u32 GetConfig() const;

        ParticleEmitter::Config* GetEmitterConfig() const;

        void SetMoveDistance(float distance);

        void SetEmitterTransform(const AZ::Transform& transform);

        void GatherSimpleLight(AZStd::vector<LightParticle>& outParticleLights);

        const AZ::u32 GetEmitterId() const;

        const AZ::Transform& GetEmitterTransform() const;

        const AZ::u32 GetRenderSort() const;

        void SetMeshSampleType(MeshSampleType meshType);

        const MeshSampleType GetMeshSampleType() const;

        bool HasSkeletonModule() const;

        bool HasLightModule() const;

        void SetAabbExtends(const AZ::Vector3& max, const AZ::Vector3& min);

        void SetWorldFront(const AZ::Vector3& front);

        void UpdateDistPtr(const Distribution& distribution);

        void PrepareSimulation();

    private:
        EmitSpawnParam Emmit(float delta);
        AZ::u32 Spawn(const ParticleEventInfo* eventInfo, const InheritanceSpawn* inheritance, AZ::u32 newParticleNum);

        AZ::u32 Spawn(const AZStd::vector<ParticleEventInfo>& spawnEvents, const AZStd::vector<InheritanceSpawn*>& spawnInheritances, AZ::u32 newParticleNum);

        void HandleEvents(const ParticleEventInfo* eventInfo, const InheritanceSpawn* inheritance, Particle& particle);

        float UpdateParticle(float delta);

        void Update(float delta, AZ::u32 begin);

        void Update(const AZStd::vector<ParticleEventInfo>& spawnEvents, const AZStd::vector<InheritanceSpawn*>& spawnInheritances, AZ::u32 begin);

        void ResetRender();

        void ResetEffectors();

        void ResetEventPool(AZ::u32 emitterId, ParticleEventType type);

        ParticleRender* AddParticleInternal(RenderType type) const;

        AZ::u32 config;
        ParticleDataPool* dataPool;
        RandomStream* randomStream;
        ParticleEventPool* systemEventPool;
        AZ::u32 emitterID = UINT32_MAX;
        AZStd::atomic_uint64_t particleIdentity = 0;

        float currTime = 0.f;
        bool started = false;
        float moveDistance = 0.f;
        AZ::Transform emitterTransform;
        MeshSampleType meshSampleType = MeshSampleType::BONE;

        ParticlePool particlePool;
        const AZ::Vector3* boneStream = nullptr;
        AZ::u32 boneCount = 0;
        const AZ::Vector3* vertexStream = nullptr;
        AZ::u32 vertexCount = 0;
        const AZ::u32* indiceStream = nullptr;
        const double* areaStream = nullptr;
        AZ::u32 indiceCount = 0;
        AZ::Vector3 maxExtend = AZ::Vector3::CreateZero();
        AZ::Vector3 minExtend = AZ::Vector3::CreateZero();
        AZ::Vector3 worldFront = AZ::Vector3::CreateAxisZ();
        AZStd::unordered_map<AZ::u32, InheritanceSpawn> emitterInheritances;

        std::pair<ParticleRender*, AZ::u32> render;
        AZStd::vector<ParticleEffectorInfo<ParticleEmitEffector>> emitEffectors;
        AZStd::vector<ParticleEffectorInfo<ParticleSpawnEffector>> spawnEffectors;
        AZStd::vector<ParticleEffectorInfo<ParticleUpdateEffector>> updateEffectors;
        AZStd::vector<ParticleEffectorInfo<ParticleEventEffector>> eventEffectors;
    };
}
