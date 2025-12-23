/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "particle/core/ParticleEmitter.h"
#include "particle/core/ParticleHelper.h"
#include "ParticleMeshRender.h"
#include "ParticleSpriteRender.h"
#include "ParticleRibbonRender.h"

namespace SimuCore::ParticleCore {
    ParticleEmitter::ParticleEmitter(const ParticleEmitter::Config& cfg, EmitterCreateInfo createInfo)
        : config(createInfo.dataPool->AllocT(cfg)),
          dataPool(createInfo.dataPool),
          randomStream(createInfo.stream),
          systemEventPool(createInfo.eventPool),
          emitterID(createInfo.id)
    {
        particlePool.Resize(cfg.maxSize);
    }

    ParticleEmitter::ParticleEmitter(AZ::u32 data, EmitterCreateInfo createInfo)
        : config(data),
          dataPool(createInfo.dataPool),
          randomStream(createInfo.stream),
          systemEventPool(createInfo.eventPool),
          emitterID(createInfo.id)
    {
        auto& cfg = *dataPool->Data<Config>(config);
        particlePool.Resize(cfg.maxSize);
    }

    ParticleEmitter::~ParticleEmitter()
    {
        dataPool->Free(sizeof(Config), config);
        ResetRender();
        ResetEffectors();
        dataPool = nullptr;
        randomStream = nullptr;
        systemEventPool = nullptr;
        boneStream = nullptr;
        vertexStream = nullptr;
    }

    void ParticleEmitter::ResetRender()
    {
        if (render.first == nullptr) {
            return;
        }

        dataPool->Free(render.first->DataSize(), render.second);
        delete render.first;
        render.second = 0;
        render.first = nullptr;
    }

    template <typename T>
    void FreeEffector(ParticleDataPool& pool, AZStd::vector<ParticleEffectorInfo<T>>& effectors)
    {
        for (auto& e : effectors) {
            pool.Free(e.effector->DataSize(), e.offset);
            delete e.effector;
        }
        effectors.clear();
    }

    void ParticleEmitter::ResetEffectors()
    {
        FreeEffector(*dataPool, emitEffectors);
        FreeEffector(*dataPool, spawnEffectors);
        FreeEffector(*dataPool, updateEffectors);
    }

    ParticleRender* ParticleEmitter::SetParticleRender(AZ::u32 data, RenderType type)
    {
        ResetRender();
        ParticleRender* rst = AddParticleInternal(type);
        if (rst != nullptr) {
            render = { rst, data };
        }
        return rst;
    }

    RenderType ParticleEmitter::GetRenderType() const
    {
        if (render.first != nullptr) {
            return render.first->GetType();
        }
        return RenderType::UNDEFINED;
    }

    float ParticleEmitter::Simulate(float delta)
    {
        ResetEventPool(emitterID, ParticleEventType::UPDATE_LOCATION);
        ResetEventPool(emitterID, ParticleEventType::UPDATE_DEATH);
        ResetEventPool(emitterID, ParticleEventType::UPDATE_COLLISION);
        auto& cfg = *dataPool->Data<Config>(config);
        currTime += delta;
        if (cfg.loop) {
            bool next = (currTime - delta >= EndTime());
            currTime = next ? delta : currTime;
            started = !next;
        } else {
            if (currTime - delta >= EndTime()) {
                UpdateParticle(delta);
                return 0.0f;
            }
        }
        return UpdateParticle(delta);
    }

    void ParticleEmitter::Tick(float delta)
    {
        auto emitParam = Emmit(delta);
        AZ::u32 beginPos = Spawn(nullptr, nullptr, emitParam.newParticleNum);
        Update(emitParam.realEmitTime, beginPos);
        particlePool.Recycle(beginPos);

        if (!emitParam.eventSpawn.empty())
        {
            beginPos = Spawn(emitParam.eventSpawn, {}, emitParam.eventSpawn.front().emitNum * static_cast<AZ::u32>(emitParam.eventSpawn.size()));
            Update(emitParam.eventSpawn, {}, beginPos);
            particlePool.Recycle(beginPos);
        }

        if (!emitParam.inheritanceSpawn.empty())
        {
            beginPos = Spawn({}, emitParam.inheritanceSpawn, emitParam.inheritanceSpawn.front()->emitNum * static_cast<AZ::u32>(emitParam.inheritanceSpawn.size()));
            Update({}, emitParam.inheritanceSpawn, beginPos);
            particlePool.Recycle(beginPos);
        }
        moveDistance = 0.f;
        started = true;
    }

    void ParticleEmitter::GatherSimpleLight(AZStd::vector<LightParticle>& outParticleLights)
    {
        const Particle* particle = particlePool.ParticleData().data();
        for (AZ::u32 i = 0; i < particlePool.Alive(); ++i) {
            const Particle& curr = particle[i];
            if (curr.hasLightEffect) {
                LightParticle lightParticle;
                lightParticle.lightColor = curr.lightColor;
                lightParticle.radianScale = curr.radianScale;
                outParticleLights.emplace_back(lightParticle);
            }
        }
    }

    const AZ::u32 ParticleEmitter::GetEmitterId() const
    {
        return emitterID;
    }

    const AZ::Transform& ParticleEmitter::GetEmitterTransform() const
    {
        return emitterTransform;
    }

    float ParticleEmitter::UpdateParticle(float delta)
    {
        auto& cfg = *dataPool->Data<Config>(config);
        float dt = std::min(currTime - cfg.startTime, delta);
        float tmp = dt < 0 ? delta : dt;
        Update(tmp, 0);
        particlePool.Recycle();
        return dt;
    }

    void ParticleEmitter::Render(AZ::u8* driver, const WorldInfo& world, DrawItem& item)
    {
        AZ_PROFILE_SCOPE(AzCore, "ParticleEmitter::Render");
        AZ::u8* data = dataPool->Data(render.first->DataSize(), render.second);
        item.positionBuffer.resize(particlePool.Alive());

        BaseInfo emitterInfo;
        auto& cfg = *dataPool->Data<Config>(config);
        emitterInfo.currentTime = std::min(currTime - cfg.startTime, cfg.duration);
        emitterInfo.duration = cfg.duration;
        emitterInfo.type = InfoType::UPDATE;

        render.first->Render(data, emitterInfo, driver, particlePool, world, item);
    }

    float ParticleEmitter::EndTime() const
    {
        auto& cfg = *dataPool->Data<Config>(config);
        return cfg.startTime + cfg.duration;
    }

    EmitSpawnParam ParticleEmitter::Emmit(float delta)
    {
        auto& cfg = *dataPool->Data<Config>(config);
        EmitInfo emitInfo = {};
        emitInfo.baseInfo.currentTime = std::min(currTime - cfg.startTime, cfg.duration);
        emitInfo.baseInfo.duration = cfg.duration;
        emitInfo.baseInfo.type = InfoType::EMIT;
        emitInfo.tickTime = delta;
        emitInfo.started = started;
        emitInfo.randomStream = randomStream;
        emitInfo.moveDistance = moveDistance;
        emitInfo.systemEventPool = systemEventPool;
        emitInfo.emitterInheritances = &emitterInheritances;
        EmitSpawnParam emitSpawnParam = {};

        for (const auto& ee : emitEffectors) {
            ee.effector->Execute(ee.data, emitInfo, emitSpawnParam);
        }

        if (emitSpawnParam.isProcessSpawnRate) {
            if (!emitSpawnParam.isIgnoreSpawnRate || (emitSpawnParam.isIgnoreSpawnRate && moveDistance <= 0.f)) {
                emitSpawnParam.newParticleNum += emitSpawnParam.spawnNum;
            }
        }

        if (emitSpawnParam.isProcessBurstList) {
            emitSpawnParam.newParticleNum += emitSpawnParam.burstNum;
        }
        emitSpawnParam.newParticleNum += emitSpawnParam.numOverMoving;
        ResetEventPool(emitterID, ParticleEventType::SPAWN_LOCATION);
        return emitSpawnParam;
    }

    AZ::u32 ParticleEmitter::Spawn(const AZStd::vector<ParticleEventInfo>& spawnEvents, const AZStd::vector<InheritanceSpawn*>& spawnInheritances,
            AZ::u32 newParticleNum)
    {
        auto& cfg = *dataPool->Data<Config>(config);
        AZ::u32 beginPos;
        bool useSpawnEventInfo = spawnEvents.empty() ? false : spawnEvents.front().useEventInfo;
        AZ::u32 numPerSpawnEventEmit = spawnEvents.empty() ? 0 : spawnEvents.front().emitNum;

        particlePool.ParallelSpawn(newParticleNum, beginPos,
                [this, &cfg, &spawnEvents, useSpawnEventInfo, numPerSpawnEventEmit, &beginPos, &spawnInheritances](Particle* particles,
                        AZ::u32 begin,
                        AZ::u32 end, AZ::u32 alive) {
                    for (AZ::u32 i = begin; i < end; ++i)
                    {
                        auto& particle = particles[i];
                        particle = {};
                        particle.id = particleIdentity.fetch_add(1, std::memory_order_relaxed);
                        const ParticleEventInfo* relatedSpawnEvent =
                                spawnEvents.empty() ? nullptr : &spawnEvents[(i - beginPos) / numPerSpawnEventEmit];
                        const InheritanceSpawn* relatedInheritanceEvent =
                                spawnInheritances.empty() ? nullptr : spawnInheritances[i - beginPos];
                        if (useSpawnEventInfo && relatedSpawnEvent)
                        {
                            particle.localPosition += emitterTransform.GetInverse().TransformPoint(relatedSpawnEvent->eventPosition);
                            particle.parentEventIdx = relatedSpawnEvent->locationEventIdx;
                        }
                        SpawnInfo spawnInfo;
                        spawnInfo.baseInfo.currentTime = std::min(currTime - cfg.startTime, cfg.duration);
                        spawnInfo.baseInfo.duration = cfg.duration;
                        spawnInfo.baseInfo.type = InfoType::SPAWN;
                        spawnInfo.randomStream = randomStream;
                        spawnInfo.vertexStream = vertexStream;
                        spawnInfo.vertexCount = vertexCount;
                        spawnInfo.indiceStream = indiceStream;
                        spawnInfo.indiceCount = indiceCount;
                        spawnInfo.areaStream = areaStream;
                        spawnInfo.boneStream = boneStream;
                        spawnInfo.boneCount = boneCount;
                        spawnInfo.systemEventPool = systemEventPool;
                        spawnInfo.currentEmitter = emitterID;
                        spawnInfo.emitterTrans = emitterTransform;
                        spawnInfo.front = worldFront;
                        for (const auto& se: spawnEffectors)
                        {
                            se.effector->Execute(se.data, spawnInfo, particle);
                        }
                        particle.spawnTrans = emitterTransform;
                        particle.globalPosition = emitterTransform.TransformPoint(particle.localPosition);
                        particle.rotateAroundPoint.SetW(i * 2.f * AZ::Constants::Pi / alive);
                        HandleEvents(relatedSpawnEvent, relatedInheritanceEvent, particle);
                    }
                });
        return beginPos;

    }

    AZ::u32 ParticleEmitter::Spawn(
            const ParticleEventInfo* eventInfo, const InheritanceSpawn* inheritance, AZ::u32 newParticleNum)
    {
        auto& cfg = *dataPool->Data<Config>(config);
        AZ::u32 beginPos;
        particlePool.ParallelSpawn(newParticleNum, beginPos,
                [this, &cfg, &eventInfo, &inheritance](Particle* particles, AZ::u32 begin, AZ::u32 end, AZ::u32 alive) {
                    for (AZ::u32 i = begin; i < end; ++i) {
                        auto& particle = particles[i];
                        particle = {};
                        particle.id = particleIdentity.fetch_add(1, std::memory_order_relaxed);
                        if (eventInfo != nullptr && eventInfo->useEventInfo) {
                            particle.localPosition += emitterTransform.GetInverse().TransformPoint(eventInfo->eventPosition);
                            particle.parentEventIdx = eventInfo->locationEventIdx;
                        }
                        SpawnInfo spawnInfo;
                        spawnInfo.baseInfo.currentTime = std::min(currTime - cfg.startTime, cfg.duration);
                        spawnInfo.baseInfo.duration = cfg.duration;
                        spawnInfo.baseInfo.type = InfoType::SPAWN;
                        spawnInfo.randomStream = randomStream;
                        spawnInfo.vertexStream = vertexStream;
                        spawnInfo.vertexCount = vertexCount;
                        spawnInfo.indiceStream = indiceStream;
                        spawnInfo.indiceCount = indiceCount;
                        spawnInfo.areaStream = areaStream;
                        spawnInfo.boneStream = boneStream;
                        spawnInfo.boneCount = boneCount;
                        spawnInfo.systemEventPool = systemEventPool;
                        spawnInfo.currentEmitter = emitterID;
                        spawnInfo.emitterTrans = emitterTransform;
                        spawnInfo.front = worldFront;
                        for (const auto& se: spawnEffectors) {
                            se.effector->Execute(se.data, spawnInfo, particle);
                        }
                        particle.spawnTrans = emitterTransform;
                        particle.globalPosition = emitterTransform.TransformPoint(particle.localPosition);
                        particle.rotateAroundPoint.SetW(i * 2.f * AZ::Constants::Pi / alive);
                        HandleEvents(eventInfo, inheritance, particle);
                    }
                });
        return beginPos;
    }

    void ParticleEmitter::HandleEvents(const ParticleEventInfo* eventInfo,
        const InheritanceSpawn* inheritance, Particle& particle)
    {
        AZ::u8* data = dataPool->Data(render.first->DataSize(), render.second);
        if (eventInfo != nullptr && data != nullptr && render.first->GetType() == RenderType::RIBBON) {
            const auto* ribbonConfig = reinterpret_cast<const RibbonConfig*>(data);
            if (ribbonConfig->mode == TrailMode::TRAIL) {
                particle.color = eventInfo->color;
                particle.ribbonId = eventInfo->particleId;
                particle.scale = 2.0f * eventInfo->size;
                particle.lifeTime = ribbonConfig->trailParam.inheritLifetime ?
                    eventInfo->lifeTime : ribbonConfig->trailParam.lifetime;
                if (ribbonConfig->trailParam.dieWithParticles &&
                    eventInfo->lifeTime - eventInfo->currentLife < particle.lifeTime) {
                    particle.lifeTime = eventInfo->lifeTime - eventInfo->currentLife;
                }
            }
        }

        if (inheritance != nullptr) {
            particle.ribbonId = inheritance->ribbonId;
            if (inheritance->applyPosition) {
                particle.globalPosition = inheritance->position;
                particle.localPosition = particle.spawnTrans.GetInverse().TransformPoint(particle.globalPosition);
            }
            if (inheritance->applyVelocity) {
                if (inheritance->overwriteVelocity) {
                    particle.velocity = inheritance->velocity;
                } else {
                    particle.velocity += inheritance->velocity;
                }
            }
            if (inheritance->applySize) {
                particle.scale = inheritance->size;
            }
            if (inheritance->applyColorRGB) {
                particle.color.SetR(inheritance->color.GetR());
                particle.color.SetG(inheritance->color.GetG());
                particle.color.SetB(inheritance->color.GetB());
            }
            if (inheritance->applyColorAlpha) {
                particle.color.SetA(inheritance->color.GetA());
            }
            if (inheritance->applyAge) {
                particle.currentLife = inheritance->currentLife;
            }
            if (inheritance->applyLifetime) {
                particle.lifeTime = inheritance->lifetime;
            }
        }
    }

    void ParticleEmitter::Update(const AZStd::vector<ParticleEventInfo>& spawnEvents, const AZStd::vector<InheritanceSpawn*>& spawnInheritances, AZ::u32 beginPos)
    {
        if (particlePool.Alive() == 0) {
            return;
        }
        auto& cfg = *dataPool->Data<Config>(config);
        UpdateInfo updateInfo;
        updateInfo.baseInfo.currentTime = std::min(currTime - cfg.startTime, cfg.duration);
        updateInfo.baseInfo.duration = cfg.duration;
        updateInfo.baseInfo.type = InfoType::UPDATE;
        updateInfo.randomStream = randomStream;
        updateInfo.localSpace = cfg.localSpace;
        updateInfo.emitterTrans = emitterTransform;
        updateInfo.front = worldFront;
        updateInfo.maxExtend = maxExtend;
        updateInfo.minExtend = minExtend;

        AZ::u32 numPerSpawnEventEmit = spawnEvents.empty() ? 0 : spawnEvents.front().emitNum;

        particlePool.ParallelUpdate(beginPos, [&spawnEvents, numPerSpawnEventEmit, &spawnInheritances, &cfg, &updateInfo, beginPos, this](Particle* particles, AZ::u32 begin, AZ::u32 end) {
            for (AZ::u32 i = begin; i < end; ++i) {
                auto& particle = particles[i];

                const ParticleEventInfo* relatedSpawnEvent = spawnEvents.empty() ? nullptr : &spawnEvents[(i - beginPos) / numPerSpawnEventEmit];
                const InheritanceSpawn* relatedInheritanceEvent = spawnInheritances.empty() ? nullptr : spawnInheritances[i - beginPos];
                auto delta = relatedSpawnEvent != nullptr ? relatedSpawnEvent->eventTimeBeforeTick
                                                          : relatedInheritanceEvent != nullptr ? relatedInheritanceEvent->emitTime
                                                                                               : 0;
                particle.currentLife += delta;

                for (const auto& ue: updateEffectors) {
                    ue.effector->Execute(ue.data, updateInfo, particle);
                }
                particle.localPosition += particle.velocity * delta;
                particle.globalPosition = cfg.localSpace ? emitterTransform.TransformPoint(particle.localPosition)
                                                         : particle.spawnTrans.TransformPoint(particle.localPosition);
                particle.rotationVector.SetW(particle.rotationVector.GetW() + particle.angularVel * delta);
            }
        });

        particlePool.Event(beginPos,
                [&cfg, this](Particle* particles, AZ::u32 begin, AZ::u32 alive) {
                    for (const auto& ee : eventEffectors) {
                        EventInfo eventInfo;
                        eventInfo.emitterTrans = emitterTransform;
                        eventInfo.particleBuffer = particles;
                        eventInfo.systemEventPool = systemEventPool;
                        eventInfo.begin = begin;
                        eventInfo.alive = alive;
                        eventInfo.currentEmitter = emitterID;
                        eventInfo.localSpace = cfg.localSpace;
                        ee.effector->Execute(ee.data, eventInfo);
                    }
                });
    }

    void ParticleEmitter::Update(float delta, AZ::u32 begin)
    {
        if (particlePool.Alive() == 0) {
            return;
        }
        auto& cfg = *dataPool->Data<Config>(config);
        UpdateInfo updateInfo;
        updateInfo.baseInfo.currentTime = std::min(currTime - cfg.startTime, cfg.duration);
        updateInfo.baseInfo.duration = cfg.duration;
        updateInfo.baseInfo.type = InfoType::UPDATE;
        updateInfo.tickTime = delta;
        updateInfo.randomStream = randomStream;
        updateInfo.localSpace = cfg.localSpace;
        updateInfo.emitterTrans = emitterTransform;
        updateInfo.front = worldFront;
        updateInfo.maxExtend = maxExtend;
        updateInfo.minExtend = minExtend;

        particlePool.ParallelUpdate(begin, [&delta, &cfg, &updateInfo, this](Particle* particles, AZ::u32 begin, AZ::u32 end) {
            for (AZ::u32 i = begin; i < end; ++i) {
                auto& particle = particles[i];
                particle.currentLife += delta;

                for (const auto& ue: updateEffectors) {
                    ue.effector->Execute(ue.data, updateInfo, particle);
                }
                particle.localPosition += particle.velocity * delta;
                particle.globalPosition = cfg.localSpace ? emitterTransform.TransformPoint(particle.localPosition)
                                                         : particle.spawnTrans.TransformPoint(particle.localPosition);
                particle.rotationVector.SetW(particle.rotationVector.GetW() + particle.angularVel * delta);
            }
        });

        particlePool.Event(begin,
            [&delta, &cfg, this](Particle* particles, AZ::u32 begin, AZ::u32 alive) {
            for (const auto& ee : eventEffectors) {
                EventInfo eventInfo;
                eventInfo.emitterTrans = emitterTransform;
                eventInfo.particleBuffer = particles;
                eventInfo.systemEventPool = systemEventPool;
                eventInfo.begin = begin;
                eventInfo.alive = alive;
                eventInfo.currentEmitter = emitterID;
                eventInfo.tickTime = delta;
                eventInfo.localSpace = cfg.localSpace;
                ee.effector->Execute(ee.data, eventInfo);
            }
        });
    }

    ParticleRender* ParticleEmitter::AddParticleInternal(RenderType type) const
    {
        switch (type) {
            case RenderType::SPRITE:
                return new ParticleSpriteRender();
            case RenderType::MESH:
                return new ParticleMeshRender();
            case RenderType::RIBBON:
                return new ParticleRibbonRender();
            case RenderType::UNDEFINED:
            default:
                return nullptr;
        }
    }

    const AZStd::vector<ParticleEffectorInfo<ParticleEmitEffector>>& ParticleEmitter::GetEmitEffectors() const
    {
        return emitEffectors;
    }

    const AZStd::vector<ParticleEffectorInfo<ParticleSpawnEffector>>& ParticleEmitter::GetSpawnEffectors() const
    {
        return spawnEffectors;
    }

    const AZStd::vector<ParticleEffectorInfo<ParticleUpdateEffector>>& ParticleEmitter::GetUpdateEffectors() const
    {
        return updateEffectors;
    }

    const AZStd::vector<ParticleEffectorInfo<ParticleEventEffector>>& ParticleEmitter::GetEventEffectors() const
    {
        return eventEffectors;
    }

    const std::pair<ParticleRender*, AZ::u32>& ParticleEmitter::GetRender() const
    {
        return render;
    }

    AZ::u32 ParticleEmitter::GetConfig() const
    {
        return config;
    }

    ParticleEmitter::Config* ParticleEmitter::GetEmitterConfig() const
    {
        return dataPool->Data<Config>(config);
    }

    void ParticleEmitter::SetMoveDistance(float distance)
    {
        moveDistance = distance;
    }

    void ParticleEmitter::SetEmitterTransform(const AZ::Transform& transform)
    {
        emitterTransform = transform;
    }

    void ParticleEmitter::ResetEventPool(AZ::u32 emitterId, ParticleEventType type)
    {
        AZ::u64 key = (static_cast<AZ::u64>(emitterId) << 32) +
            static_cast<AZ::u64>(type);
        auto iter = systemEventPool->events.find(key);
        if (iter == systemEventPool->events.end()) {
            systemEventPool->events[key] = AZStd::vector<ParticleEventInfo>();
        }
        systemEventPool->events[key].clear();
    }

    const AZ::u32 ParticleEmitter::GetRenderSort() const
    {
        if (render.first == nullptr) {
            return 0;
        }

        AZ::u8* data = dataPool->Data(render.first->DataSize(), render.second);
        if (data == nullptr) {
            return 0;
        }
        if (render.first->GetType() == RenderType::SPRITE) {
            SpriteConfig& spriteConfig = *reinterpret_cast<SpriteConfig*>(data);
            return spriteConfig.sortId;
        }

        if (render.first->GetType() == RenderType::MESH) {
            MeshConfig& meshConfig = *reinterpret_cast<MeshConfig*>(data);
            return meshConfig.sortId;
        }

        if (render.first->GetType() == RenderType::RIBBON) {
            RibbonConfig& ribbonConfig = *reinterpret_cast<RibbonConfig*>(data);
            return ribbonConfig.sortId;
        }
        return 0;
    }

    void ParticleEmitter::SetMeshSampleType(MeshSampleType meshType)
    {
        meshSampleType = meshType;
    }

    const MeshSampleType ParticleEmitter::GetMeshSampleType() const
    {
        return meshSampleType;
    }

    bool ParticleEmitter::HasSkeletonModule() const
    {
        AZStd::string skeletonModuleName = "SpawnLocSkeleton";
        for (const auto& se : spawnEffectors) {
            if (se.effector->Name().find(skeletonModuleName) != AZStd::string::npos) {
                return true;
            }
        }
        return false;
    }

    bool ParticleEmitter::HasLightModule() const
    {
        AZStd::string lightModuleName = "SpawnLightEffect";
        for (const auto& se : spawnEffectors) {
            if (se.effector->Name().find(lightModuleName) != AZStd::string::npos) {
                return true;
            }
        }
        return false;
    }

    void ParticleEmitter::SetAabbExtends(const AZ::Vector3& max, const AZ::Vector3& min)
    {
        maxExtend = max;
        minExtend = min;
    }

    void ParticleEmitter::SetWorldFront(const AZ::Vector3& front)
    {
        worldFront = front;
    }

    void ParticleEmitter::UpdateDistPtr(const Distribution& distribution)
    {
        for (const auto& ee : emitEffectors) {
            ee.effector->Update(ee.data, distribution);
        }
        for (const auto& se : spawnEffectors) {
            se.effector->Update(se.data, distribution);
        }
        for (const auto& ue : updateEffectors) {
            ue.effector->Update(ue.data, distribution);
        }
        if (render.first->GetType() == RenderType::RIBBON) {
            AZ::u8* renderData = dataPool->Data(render.first->DataSize(), render.second);
            if (renderData == nullptr) {
                return;
            }
            auto* ribbonConfig = reinterpret_cast<RibbonConfig*>(renderData);
            UpdateDistributionPtr(ribbonConfig->ribbonWidth, distribution);
        }
    }

    void ParticleEmitter::PrepareSimulation()
    {
        emitEffectors.erase(
                std::remove_if(emitEffectors.begin(), emitEffectors.end(), [](auto& effector) { return effector.data == nullptr; }),
                emitEffectors.end());
        spawnEffectors.erase(
                std::remove_if(spawnEffectors.begin(), spawnEffectors.end(), [](auto& effector) { return effector.data == nullptr; }),
                spawnEffectors.end());
        updateEffectors.erase(
                std::remove_if(updateEffectors.begin(), updateEffectors.end(), [](auto& effector) { return effector.data == nullptr; }),
                updateEffectors.end());
        eventEffectors.erase(
                std::remove_if(eventEffectors.begin(), eventEffectors.end(), [](auto& effector) { return effector.data == nullptr; }),
                eventEffectors.end());
    }
}
