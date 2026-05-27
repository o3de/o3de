/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <memory>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include "particle/core/Particle.h"
#include "particle/core/ParticleEmitter.h"
#include "particle/core/ParticleCurve.h"
#include "particle/core/ParticleRandom.h"

namespace SimuCore::ParticleCore {
    struct LevelsOfDetail {
        float distance;
        AZStd::vector<AZ::u32> emitterIndexes;
    };

    class ParticleSystem {
    public:
        struct PreWarm {
            float warmupTime = 0.f;
            AZ::u32 tickCount = 0;
            float tickDelta = 0.f;
        };

        struct Config {
            bool loop = true;
            bool parallel = true;
        };

        ParticleSystem() {}

        ~ParticleSystem();

        using DrawItemMap = AZStd::unordered_map<ParticleEmitter*, AZStd::vector<DrawItem>>;

        ParticleEmitter* AddEmitter(const ParticleEmitter::Config& cfg);

        ParticleEmitter* AddEmitter(AZ::u32 cfg);

        void RemoveEmitter(ParticleEmitter* emitter);

        void RemoveEmitter(AZ::u32 emitterId);

        ParticleEmitter* GetEmitter(AZ::u32 emitterId);

        const AZStd::unordered_map<AZ::u32, ParticleEmitter*> GetAllEmitters() const;

        const AZStd::vector<ParticleEmitter*> GetVisibleEmitters() const;

        void SetLODs(const AZStd::vector<ParticleCore::LevelsOfDetail>& levelOfDetails);

        ParticleCurve* AddCurve(const CurveExtrapMode& left, const CurveExtrapMode& right,
            float timeFactor, float valueFactor, const CurveTickMode& mode);

        void AddRandom(float min, float max, const RandomTickMode& mode, AZ::u32 maxParticleNum);

        void UpdateDistribution();

        void SetConfig(AZ::u32 data);

        const ParticleDataPool& GetDataPool() const;

        AZ::u8* Data(AZ::u32 size, AZ::u32 index);

        const AZ::u8* Data(AZ::u32 size, AZ::u32 index) const;

        void EmplaceData(AZ::u32 stride, const AZ::u8* data, AZ::u32 dataSize);

        AZStd::vector<RenderType> GetRenderTypes() const;

        AZ::u32 GetConfig() const;

        ParticleSystem::Config* GetSystemConfig();

        void Play();

        void Pause();

        void Stop();

        void Reset();

        void Simulate(float delta);

        void UpdateWorldInfo(const AZ::Transform& cameraTrans, const AZ::Transform& systemTrans, const AZ::Vector3& worldFront);

        template<typename Func>
        void ForEachEmitter(Func&& f)
        {
            for (auto& emitter : allEmitters) {
                f(emitter.second);
            }
        }

        void SetPreWarm(const PreWarm& warmUp);

        void SetPreWarm(AZ::u32 warmUp);

        AZ::u32 GetPreWarm() const;

        void WarmUp();

        void UseGlobalSpace();

        AZ::u32 GetMaxParticleNum();

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
        AZStd::vector<ParticleEmitter*> visibleEmitters;

        AZ::u32 emitterIdentifier = 0; // mark emitterId, unique identifier
        AZ::u32 config = UINT32_MAX;
        AZ::u32 preWarm = UINT32_MAX;
        AZStd::unordered_map<AZ::u32, ParticleEmitter*> allEmitters; // id & ptr
        AZStd::vector<ParticleCore::LevelsOfDetail> lods;
        Distribution distribution;
        ParticleDataPool dataPool;
        RandomStream randomStream;
        ParticleEventPool eventPool;
    };
}
