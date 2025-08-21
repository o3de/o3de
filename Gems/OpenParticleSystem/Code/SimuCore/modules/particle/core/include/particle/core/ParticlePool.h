/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <vector>
#include <algorithm>
#include "particle/core/Particle.h"
#include "core/jobsystem/JobSystem.h"

namespace SimuCore::ParticleCore {

    // Regarding 64KB as min simulation group size to avoid too much thread context switch.
    constexpr uint32_t IDEAL_GROUP_COUNT{64 * 1024 / sizeof(Particle)};

    class ParticlePool {
    public:
        ParticlePool() : jobSystem(JobSystem::GetInstance()), maxGroup(jobSystem.GetThreadCount() + 1) {};
        ~ParticlePool() = default;

        void Resize(uint32_t size);

        void Recycle(uint32_t beginPos = 0);

        void Reset();

        template<typename Func>
        void ParallelSpawn(uint32_t num, uint32_t& begin, Func&& func)
        {
            uint32_t old = alive;
            begin = old;
            alive = std::min(alive + num, maxSize);

            auto count = alive - old;
            if (count == 0) {
                return;
            }
            JobSystem::Counter ctx;
            uint32_t groupNum;
            uint32_t countPerGroup;
            uint32_t lastGroupCount;
            std::tie(groupNum, countPerGroup, lastGroupCount) = CalcGroup(count);
            for (uint32_t group = 0; group < groupNum; group++) {
                jobSystem.AddJob(ctx, [this, old, func, group, countPerGroup]([[maybe_unused]]JobSystem::JobInfo jobInfo) {
                    uint32_t start = group * countPerGroup + old;
                    func(particles.data(), start, start + countPerGroup, alive);
                });
            }
            if (lastGroupCount > 0) {
                jobSystem.AddJob(ctx, [this, old, func, groupNum, countPerGroup, lastGroupCount]([[maybe_unused]]JobSystem::JobInfo jobInfo) {
                    uint32_t start = groupNum * countPerGroup + old;
                    func(particles.data(), start, start + lastGroupCount, alive);
                });
            }
            jobSystem.WaitCounter(ctx);
        }

        template<typename Func>
        void ParallelUpdate(uint32_t begin, Func&& func)
        {
            auto count = alive - begin;
            if (count == 0) {
                return;
            }
            JobSystem::Counter ctx;
            uint32_t groupNum;
            uint32_t countPerGroup;
            uint32_t lastGroupCount;
            std::tie(groupNum, countPerGroup, lastGroupCount) = CalcGroup(count);
            for (uint32_t group = 0; group < groupNum; group++) {
                jobSystem.AddJob(ctx, [this, begin, func, group, countPerGroup]([[maybe_unused]]JobSystem::JobInfo jobInfo) {
                    uint32_t start = group * countPerGroup + begin;
                    func(particles.data(), start, start + countPerGroup);
                });
            }
            if (lastGroupCount > 0) {
                jobSystem.AddJob(ctx, [this, begin, func, groupNum, countPerGroup, lastGroupCount]([[maybe_unused]]JobSystem::JobInfo jobInfo) {
                    uint32_t start = groupNum * countPerGroup + begin;
                    func(particles.data(), start, start + lastGroupCount);
                });
            }
            jobSystem.WaitCounter(ctx);
        }

        template<typename Func>
        void Event(uint32_t begin, Func&& func)
        {
            func(particles.data(), begin, alive);
        }

        template<typename Func>
        void RenderAll(Func&& func) const
        {
            if (alive == 0) {
                return;
            }

            const auto& particleToRender = ParticleData();
            JobSystem::Counter ctx;
            uint32_t groupNum;
            uint32_t countPerGroup;
            uint32_t lastGroupCount;
            std::tie(groupNum, countPerGroup, lastGroupCount) = CalcGroup(alive);
            for (uint32_t group = 0; group < groupNum; group++) {
                jobSystem.AddJob(ctx, [func, group, &particleToRender, countPerGroup]([[maybe_unused]]JobSystem::JobInfo jobInfo) {
                    uint32_t start = group * countPerGroup;
                    func(particleToRender.data(), start, start + countPerGroup);
                });
            }
            if (lastGroupCount > 0) {
                jobSystem.AddJob(ctx, [func, groupNum, lastGroupCount, countPerGroup, &particleToRender]([[maybe_unused]]JobSystem::JobInfo jobInfo) {
                    uint32_t start = groupNum * countPerGroup;
                    func(particleToRender.data(), start, start + lastGroupCount);
                });
            }
            jobSystem.WaitCounter(ctx);
        }

        [[nodiscard]]
        uint32_t Size() const;

        [[nodiscard]]
        uint32_t Alive() const;

        [[nodiscard]]
        const std::vector<Particle>& ParticleData() const;

    private:
        uint32_t alive = 0;
        uint32_t maxSize = 0;
        std::vector<Particle> particles;
        JobSystem& jobSystem;
        uint32_t maxGroup;

        [[nodiscard]]
        inline std::tuple<uint32_t, uint32_t, uint32_t> CalcGroup(uint32_t count) const
        {
            const uint32_t groupNum = std::min(count / std::min(count, IDEAL_GROUP_COUNT), maxGroup);
            const uint32_t countPerGroup = count / groupNum;
            const uint32_t lastGroupCount = count - groupNum * countPerGroup;
            return {groupNum, countPerGroup, lastGroupCount};
        }
    };
}
