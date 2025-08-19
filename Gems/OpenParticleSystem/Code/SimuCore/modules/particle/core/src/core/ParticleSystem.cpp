/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include "core/jobsystem/JobSystem.h"
#include "particle/core/ParticleSystem.h"

namespace SimuCore::ParticleCore {
    ParticleSystem::~ParticleSystem()
    {
        Reset();
    }

    ParticleEmitter* ParticleSystem::AddEmitter(const ParticleEmitter::Config& cfg)
    {
        auto emitter = new ParticleEmitter(
            cfg,
            EmitterCreateInfo{ &dataPool, &randomStream, &eventPool, emitterIdentifier++ }
        );
        (void)allEmitters.emplace(emitter->GetEmitterId(), emitter);
        return emitter;
    }

    ParticleEmitter* ParticleSystem::AddEmitter(uint32_t cfg)
    {
        auto emitter = new ParticleEmitter(
            cfg,
            EmitterCreateInfo{ &dataPool, &randomStream, &eventPool, emitterIdentifier++ }
        );
        (void)allEmitters.emplace(emitter->GetEmitterId(), emitter);
        return emitter;
    }

    void ParticleSystem::RemoveEmitter(ParticleEmitter* emitter)
    {
        if (emitter == nullptr) {
            return;
        }
        auto iter = allEmitters.find(emitter->GetEmitterId());
        if (iter != allEmitters.end() && iter->second == emitter) {
            allEmitters.erase(iter);
        }
        delete emitter;
    }

    void ParticleSystem::RemoveEmitter(uint32_t emitterId)
    {
        auto iter = allEmitters.find(emitterId);
        if (iter != allEmitters.end()) {
            delete iter->second;
            allEmitters.erase(iter);
        }
    }

    ParticleEmitter* ParticleSystem::GetEmitter(uint32_t emitterId)
    {
        auto iter = allEmitters.find(emitterId);
        if (iter == allEmitters.end()) {
            return nullptr;
        }
        return iter->second;
    }

    const std::unordered_map<uint32_t, ParticleEmitter*> ParticleSystem::GetAllEmitters() const
    {
        return allEmitters;
    }

    const std::vector<ParticleEmitter*> ParticleSystem::GetVisibleEmitters() const
    {
        return visibleEmitters;
    }

    void ParticleSystem::SetLODs(const std::vector<ParticleCore::LevelsOfDetail>& levelOfDetails)
    {
        lods = levelOfDetails;
    }

    ParticleCurve* ParticleSystem::AddCurve(const CurveExtrapMode& left, const CurveExtrapMode& right,
        float timeFactor, float valueFactor, const CurveTickMode& mode)
    {
        auto* curvePtr = new ParticleCurve();
        curvePtr->SetLeftExtrapMode(left);
        curvePtr->SetRightExtrapMode(right);
        curvePtr->SetTimeFactor(timeFactor);
        curvePtr->SetValueFactor(valueFactor);
        curvePtr->SetTickMode(mode);
        (void)distribution[DistributionType::CURVE].emplace_back(curvePtr);
        return curvePtr;
    }

    void ParticleSystem::AddRandom(float min, float max, const RandomTickMode& mode, uint32_t maxParticleNum)
    {
        ParticleDistribution* randomPtr = new ParticleRandom(min, max, mode, maxParticleNum);
        (void)distribution[DistributionType::RANDOM].emplace_back(randomPtr);
    }

    const ParticleDataPool& ParticleSystem::GetDataPool() const
    {
        return dataPool;
    }

    uint8_t* ParticleSystem::Data(uint32_t size, uint32_t index)
    {
        return dataPool.Data(size, index);
    }

    const uint8_t* ParticleSystem::Data(uint32_t size, uint32_t index) const
    {
        return dataPool.Data(size, index);
    }

    void ParticleSystem::EmplaceData(uint32_t stride, const uint8_t* data, uint32_t dataSize)
    {
        dataPool.EmplaceData(stride, data, dataSize);
    }

    void ParticleSystem::SetConfig(uint32_t data)
    {
        config = data;
        SimuCore::JobSystem& jobsystem = SimuCore::JobSystem::GetInstance();
        if (dataPool.Data<Config>(config)->parallel) {
            jobsystem.Initialize();
        } else {
            jobsystem.Initialize(1);
        }
    }

    std::vector<RenderType> ParticleSystem::GetRenderTypes() const
    {
        std::vector<RenderType> types;
        for (auto& emitter : allEmitters) {
            auto type = emitter.second->GetRenderType();
            if (type == RenderType::UNDEFINED) {
                continue;
            }
            types.emplace_back(type);
        }
        return types;
    }

    uint32_t ParticleSystem::GetConfig() const
    {
        return config;
    }

    ParticleSystem::Config* ParticleSystem::GetSystemConfig()
    {
        return dataPool.Data<Config>(config);
    }

    void ParticleSystem::Play()
    {
        status = Status::PLAYING;
    }

    void ParticleSystem::Pause()
    {
        status = Status::PAUSEING;
    }

    void ParticleSystem::Stop()
    {
        status = Status::STOPED;
        time = 0;
    }

    void ParticleSystem::Reset()
    {
        Stop();
        for (auto emitter : allEmitters) {
            delete emitter.second;
        }
        for (auto& dist : distribution) {
            for (auto ptr : dist.second) {
                delete ptr;
            }
        }
        distribution.clear();
        allEmitters.clear();
        dataPool.Reset();
    }

    void ParticleSystem::Simulate(float delta)
    {
        if (status != Status::PLAYING) {
            return;
        }
        CalculateVisibleEmitters();

        if (eventPool.inheritances.size() != emitterIdentifier) {
            eventPool.inheritances.clear();
            eventPool.inheritances.resize(emitterIdentifier, std::unordered_map<uint64_t, InheritanceInfo>());
        }
        for (uint32_t i = 0; i < emitterIdentifier; i++) {
            eventPool.inheritances[i].clear();
        }

        std::vector<float> dts;
        for (auto emitter : visibleEmitters) {
            dts.emplace_back(emitter->Simulate(delta));
        }

        for (size_t i = 0; i < visibleEmitters.size(); i++) {
            if (dts[i] > 0.0f) {
                visibleEmitters[i]->Tick(dts[i]);
            }
        }
    }

    void ParticleSystem::UpdateWorldInfo(const Transform& cameraTrans,
        const Transform& systemTrans, const Vector3& worldFront)
    {
        currentDistance = (cameraTrans.translation - systemTrans.translation).Length();
        for (auto& emitter : allEmitters) {
            emitter.second->SetEmitterTransform(systemTrans);
            emitter.second->SetWorldFront(worldFront);
        }
    }

    void ParticleSystem::CalculateVisibleEmitters()
    {
        visibleEmitters.clear();
        std::vector<std::pair<ParticleEmitter*, uint32_t>> renderSortInfo;
        for (auto& lod : lods) {
            if (currentDistance > lod.distance) {
                continue;
            }
            for (auto& idx : lod.emitterIndexes) {
                auto iter = allEmitters.find(idx);
                if (iter != allEmitters.end()) {
                    (void)renderSortInfo.emplace_back(iter->second, iter->second->GetRenderSort());
                }
            }
            break;
        }

        if (lods.size() == 0) {
            for (auto emitter : allEmitters) {
                (void)renderSortInfo.emplace_back(emitter.second, emitter.second->GetRenderSort());
            }
        }

        std::sort(renderSortInfo.begin(), renderSortInfo.end(),
            [](const std::pair<ParticleEmitter*, uint32_t>& sortInfo1,
                const std::pair<ParticleEmitter*, uint32_t>& sortInfo2) {
            return sortInfo1.second < sortInfo2.second;
        });

        for (auto it = renderSortInfo.begin(); it != renderSortInfo.end(); ++it) {
            (void)visibleEmitters.emplace_back(it->first);
        }
    }

    void ParticleSystem::SetPreWarm(const PreWarm& warmUp)
    {
        preWarm = dataPool.AllocT(warmUp);
    }

    uint32_t ParticleSystem::GetPreWarm() const
    {
        return preWarm;
    }

    void ParticleSystem::SetPreWarm(uint32_t warmUp)
    {
        preWarm = warmUp;
    }

    void ParticleSystem::WarmUp()
    {
        for (auto& emitter : allEmitters) {
            emitter.second->PrepareSimulation();
        }

        auto warmUp = dataPool.Data<PreWarm>(preWarm);
        if (warmUp == nullptr) {
            return;
        }
        if (warmUp->tickCount == 0 || warmUp->tickDelta <= Math::EPSLON) {
            return;
        }
        for (uint32_t tickIndex = 0; tickIndex < warmUp->tickCount; tickIndex++) {
            std::unordered_map<uint32_t, float> dts;
            for (auto emitter : allEmitters) {
                (void)dts.emplace(emitter.first, emitter.second->Simulate(warmUp->tickDelta));
            }

            for (auto& emitter : allEmitters) {
                if (dts[emitter.first] > 0.0f) {
                    emitter.second->Tick(dts[emitter.first]);
                }
            }
        }
    }

    void ParticleSystem::UpdateDistribution()
    {
        for (auto emitter : allEmitters) {
            emitter.second->UpdateDistPtr(distribution);
        }
    }

    void ParticleSystem::UseGlobalSpace()
    {
        for (auto emitter : allEmitters) {
            emitter.second->GetEmitterConfig()->localSpace = false;
        }
    }

    uint32_t ParticleSystem::GetMaxParticleNum()
    {
        uint32_t maxNum{0};
        for (const auto& emitter : allEmitters) {
            maxNum = std::max(emitter.second->GetEmitterConfig()->maxSize, maxNum);
        }
        return maxNum;
    }
}
