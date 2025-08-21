/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "particle/core/Particle.h"
#include "particle/core/ParticleEmitter.h"
#include "particle/core/ParticleCurve.h"
#include "particle/core/ParticleRandom.h"

namespace SimuCore::ParticleCore {
    struct LevelsOfDetail {
        float distance;
        std::vector<uint32_t> emitterIndexes;
    };

    class ParticleSystem {
    public:
        struct PreWarm {
            float warmupTime = 0.f;
            uint32_t tickCount = 0;
            float tickDelta = 0.f;
        };

        struct Config {
            bool loop = true;
            bool parallel = true;
        };

        ParticleSystem() {}

        ~ParticleSystem();

        using DrawItemMap = std::unordered_map<ParticleEmitter*, std::vector<DrawItem>>;

        ParticleEmitter* AddEmitter(const ParticleEmitter::Config& cfg);

        ParticleEmitter* AddEmitter(uint32_t cfg);

        void RemoveEmitter(ParticleEmitter* emitter);

        void RemoveEmitter(uint32_t emitterId);

        ParticleEmitter* GetEmitter(uint32_t emitterId);

        const std::unordered_map<uint32_t, ParticleEmitter*> GetAllEmitters() const;

        const std::vector<ParticleEmitter*> GetVisibleEmitters() const;

        void SetLODs(const std::vector<ParticleCore::LevelsOfDetail>& levelOfDetails);

        ParticleCurve* AddCurve(const CurveExtrapMode& left, const CurveExtrapMode& right,
            float timeFactor, float valueFactor, const CurveTickMode& mode);

        void AddRandom(float min, float max, const RandomTickMode& mode, uint32_t maxParticleNum);

        void UpdateDistribution();

        void SetConfig(uint32_t data);

        const ParticleDataPool& GetDataPool() const;

        uint8_t* Data(uint32_t size, uint32_t index);

        const uint8_t* Data(uint32_t size, uint32_t index) const;

        void EmplaceData(uint32_t stride, const uint8_t* data, uint32_t dataSize);

        std::vector<RenderType> GetRenderTypes() const;

        uint32_t GetConfig() const;

        ParticleSystem::Config* GetSystemConfig();

        void Play();

        void Pause();

        void Stop();

        void Reset();

        void Simulate(float delta);

        void UpdateWorldInfo(const Transform& cameraTrans, const Transform& systemTrans, const Vector3& worldFront);

        template<typename Func>
        void ForEachEmitter(Func&& f)
        {
            for (auto& emitter : allEmitters) {
                f(emitter.second);
            }
        }

        void SetPreWarm(const PreWarm& warmUp);

        void SetPreWarm(uint32_t warmUp);

        uint32_t GetPreWarm() const;

        void WarmUp();

        void UseGlobalSpace();

        uint32_t GetMaxParticleNum();

    private:
        enum class Status {
            PLAYING,
            STOPED,
            PAUSEING
        };

        void CalculateVisibleEmitters();

        Status status = Status::STOPED;
        float time = 0.f;
        float currentDistance = 0.0f;
        std::vector<ParticleEmitter*> visibleEmitters;

        uint32_t emitterIdentifier = 0; // mark emitterId, unique identifier
        uint32_t config = UINT32_MAX;
        uint32_t preWarm = UINT32_MAX;
        std::unordered_map<uint32_t, ParticleEmitter*> allEmitters; // id & ptr
        std::vector<ParticleCore::LevelsOfDetail> lods;
        Distribution distribution;
        ParticleDataPool dataPool;
        RandomStream randomStream;
        ParticleEventPool eventPool;
    };
}
