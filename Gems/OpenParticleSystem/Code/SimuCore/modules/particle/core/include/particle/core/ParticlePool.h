/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "particle/core/Particle.h"

#include <AzCore/std/containers/vector.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>

namespace SimuCore::ParticleCore
{

    // Regarding 64KB as min simulation group size to avoid too much thread context switch.
    constexpr AZ::u32 IDEAL_GROUP_COUNT{ 64 * 1024 / sizeof(Particle) };

    class ParticlePool
    {
    public:
        ParticlePool();
        ~ParticlePool() = default;

        void Resize(AZ::u32 size);

        void Recycle(AZ::u32 beginPos = 0);

        void Reset();

        template<typename Func>
        void ParallelSpawn(AZ::u32 num, AZ::u32& begin, Func&& func)
        {
            AZ::u32 old = alive;
            begin = old;
            alive = AZStd::min(alive + num, maxSize);

            auto count = alive - old;
            if (count == 0)
            {
                return;
            }

            AZ::JobCompletion completion;
            AZ::u32 groupNum;
            AZ::u32 countPerGroup;
            AZ::u32 lastGroupCount;
            AZStd::tie(groupNum, countPerGroup, lastGroupCount) = CalcGroup(count);
            for (AZ::u32 group = 0; group < groupNum; group++)
            {
                AZ::Job* const job = AZ::CreateJobFunction(
                    [this, old, func, group, countPerGroup](AZ::Job& /*thisJob*/)
                    {
                        AZ::u32 start = group * countPerGroup + old;
                        func(particles.data(), start, start + countPerGroup, alive);
                    },
                    true);
                job->SetDependent(&completion);
                job->Start();
            }

            if (lastGroupCount > 0)
            {
                AZ::Job* const job = AZ::CreateJobFunction(
                    [this, old, func, groupNum, countPerGroup, lastGroupCount](AZ::Job& /*thisJob*/)
                    {
                        AZ::u32 start = groupNum * countPerGroup + old;
                        func(particles.data(), start, start + lastGroupCount, alive);
                    },
                    true);
                job->SetDependent(&completion);
                job->Start();
            }
            completion.StartAndWaitForCompletion();
        }

        template<typename Func>
        void ParallelUpdate(AZ::u32 begin, Func&& func)
        {
            auto count = alive - begin;
            if (count == 0)
            {
                return;
            }

            AZ::JobCompletion completion;
            AZ::u32 groupNum;
            AZ::u32 countPerGroup;
            AZ::u32 lastGroupCount;
            AZStd::tie(groupNum, countPerGroup, lastGroupCount) = CalcGroup(count);
            for (AZ::u32 group = 0; group < groupNum; group++)
            {
                AZ::Job* const job = AZ::CreateJobFunction(
                    [this, begin, func, group, countPerGroup](AZ::Job& /*thisJob*/)
                    {
                        AZ::u32 start = group * countPerGroup + begin;
                        func(particles.data(), start, start + countPerGroup);
                    },
                    true);
                job->SetDependent(&completion);
                job->Start();
            }

            if (lastGroupCount > 0)
            {
                AZ::Job* const job = AZ::CreateJobFunction(
                    [this, begin, func, groupNum, countPerGroup, lastGroupCount](AZ::Job& /*thisJob*/)
                    {
                        AZ::u32 start = groupNum * countPerGroup + begin;
                        func(particles.data(), start, start + lastGroupCount);
                    },
                    true);
                job->SetDependent(&completion);
                job->Start();
            }
            completion.StartAndWaitForCompletion();
        }

        template<typename Func>
        void Event(AZ::u32 begin, Func&& func)
        {
            func(particles.data(), begin, alive);
        }

        template<typename Func>
        void RenderAll(Func&& func) const
        {
            if (alive == 0)
            {
                return;
            }

            const auto& particleToRender = ParticleData();
            AZ::JobCompletion completion;
            AZ::u32 groupNum;
            AZ::u32 countPerGroup;
            AZ::u32 lastGroupCount;
            AZStd::tie(groupNum, countPerGroup, lastGroupCount) = CalcGroup(alive);
            for (AZ::u32 group = 0; group < groupNum; group++)
            {
                AZ::Job* const job = AZ::CreateJobFunction(
                    [func, group, &particleToRender, countPerGroup](AZ::Job& /*thisJob*/)
                    {
                        AZ::u32 start = group * countPerGroup;
                        func(particleToRender.data(), start, start + countPerGroup);
                    },
                    true);
                job->SetDependent(&completion);
                job->Start();
            }

            if (lastGroupCount > 0)
            {
                AZ::Job* const job = AZ::CreateJobFunction(
                    [func, groupNum, lastGroupCount, countPerGroup, &particleToRender](AZ::Job& /*thisJob*/)
                    {
                        AZ::u32 start = groupNum * countPerGroup;
                        func(particleToRender.data(), start, start + lastGroupCount);
                    },
                    true);
                job->SetDependent(&completion);
                job->Start();
            }
            completion.StartAndWaitForCompletion();
        }

        [[nodiscard]]
        AZ::u32 Size() const;

        [[nodiscard]]
        AZ::u32 Alive() const;

        [[nodiscard]]
        const AZStd::vector<Particle>& ParticleData() const;

    private:
        AZ::u32 alive = 0;
        AZ::u32 maxSize = 0;
        AZStd::vector<Particle> particles;
        AZ::u32 maxGroup;

        std::tuple<AZ::u32, AZ::u32, AZ::u32> CalcGroup(AZ::u32 count) const
        {
            const AZ::u32 groupNum = AZStd::min(count / AZStd::min(count, IDEAL_GROUP_COUNT), maxGroup);
            const AZ::u32 countPerGroup = count / groupNum;
            const AZ::u32 lastGroupCount = count - groupNum * countPerGroup;
            return { groupNum, countPerGroup, lastGroupCount };
        }
    };
}
