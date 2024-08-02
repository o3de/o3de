/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// CARBONATED specific classes to track GPU data, used in Device.h
#pragma once

#if !(defined(AZ_PLATFORM_IOS) || defined(AZ_PLATFORM_MAC))
#include <AzCore/std/chrono/chrono.h>
#endif

namespace AZ::RHI
{

    struct TimeInterval
    {
        double m_begin;
        double m_end;
        
        TimeInterval() {}
        TimeInterval(double begin, double end) : m_begin(begin), m_end(end) {}
        
        bool IsOverlap(const TimeInterval& t)
        {
            return (m_begin >= t.m_begin) ?  m_begin <= t.m_end : t.m_begin <= m_end;
        }
        
        void Merge(const TimeInterval& t)
        {
            AZ_Assert(IsOverlap(t), "Cannot combine non-overlapping intervals");
            if (m_begin > t.m_begin)
            {
                m_begin = t.m_begin;
            }
            if (t.m_end > m_end)
            {
                m_end = t.m_end;
            }
        }
        
        AZ_FORCE_INLINE static double GetTimeSec()
        {
#if defined(AZ_PLATFORM_IOS) || defined(AZ_PLATFORM_MAC)
            return double(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1000000000.0;
#else
            const auto current_time = AZStd::chrono::system_clock::now();
            return AZStd::chrono::duration<double>(current_time.time_since_epoch()).count();
#endif
        }
    };

    struct FrameCommands
    {
        struct CommandBuffer
        {
            const void* m_buffer;
            double m_commitTime;
            
            CommandBuffer() : m_buffer(nullptr), m_commitTime(0.0) {}
            CommandBuffer(const void* buffer) : m_buffer(buffer), m_commitTime(0.0) {}
        };
        
        unsigned int m_frameNumber;
        double m_sumTime;
        double m_waitTime;
        double m_endMaxTime;
        unsigned int m_numBuffers;
        AZStd::vector<CommandBuffer> m_commands;
        AZStd::vector<TimeInterval> m_intervals;
        AZStd::vector<TimeInterval> m_rawIntervals;

        FrameCommands() :
            m_frameNumber(0),
            m_sumTime(0.0),
            m_waitTime(0.0),
            m_endMaxTime(0.0),
            m_numBuffers(0)
        {
            m_commands.reserve(6);
            m_intervals.reserve(4);
            m_rawIntervals.reserve(6);
        }
        
        void RegisterCommandBuffer(const void* commandBuffer)
        {
            CommandBuffer cb(commandBuffer);
            m_commands.push_back(cb);
        }
        void RegisterInterval(double commit, double begin, double end)
        {
            m_sumTime += end - begin;
            if (commit < 0.00001)
            {
                AZ_Error("GPUtime", false, "zero commit time %f", commit);
            }
            else
            {
                if (begin < commit)
                {
                    AZ_Error("GPUtime", false, "bad commit time %f, begin %f", commit, begin);
                }
                else
                {
                    m_waitTime += begin - commit;
                    m_numBuffers++;
                    
                    const double overall = end - commit;
                    if (overall > m_endMaxTime)
                    {
                        m_endMaxTime = overall;
                    }
                }
            }
            bool consumed = false;
            for (auto it = m_intervals.begin(); it != m_intervals.end(); it++)
            {
                if (end < it->m_begin)
                {
                    m_intervals.insert(it, TimeInterval(begin, end));
                    consumed = true;
                    break;
                }
                else if (begin > it->m_end)
                {
                    continue;
                }
                it->Merge(TimeInterval(begin, end));
                consumed = true;
                for (;;)
                {
                    auto itNext = it + 1;
                    if (itNext == m_intervals.end())
                    {
                        break;
                    }
                    if (!it->IsOverlap(*itNext))
                    {
                        break;
                    }
                    it->Merge(*itNext);
                    m_intervals.erase(itNext);
                }
                break;
            }
            if (!consumed)
            {
                m_intervals.push_back(TimeInterval(begin, end));
            }
            m_rawIntervals.push_back(TimeInterval(begin, end));
            
            ValidateIntervals();
        }
        void LogIntervals()
        {
            int i = 0;
            for (auto it = m_intervals.begin(); it != m_intervals.end(); it++, i++)
            {
                AZ_Info("GPUtime", "%d %f %f", i, it->m_begin, it->m_end);
            }
        }
        void ValidateIntervals()
        {
            for (auto it = m_intervals.begin(); it != m_intervals.end(); it++)
            {
                auto itNext = it + 1;
                if (it->m_begin >= it->m_end)
                {
                    LogIntervals();
                    AZ_Error("GPUtime", false, "Bad interval %f %f", it->m_begin, it->m_end);
                }
                if (itNext != m_intervals.end())
                {
                    if (it->m_end > itNext->m_begin)
                    {
                        LogIntervals();
                        AZ_Error("GPUtime", false, "Unordered intervals %f..%f  %f..%f", it->m_begin, it->m_end, itNext->m_begin, itNext->m_end);
                    }
                }
            }
        }
        
        void Init(int frameNumber)
        {
            m_frameNumber = frameNumber;
            m_sumTime = 0.0;
            m_waitTime = 0.0;
            m_endMaxTime = 0.0;
            m_numBuffers = 0;
            m_commands.clear();
            m_intervals.clear();
            m_rawIntervals.clear();
        }
        
        double CalculateTime()
        {
            double result = 0.0;
            for (const TimeInterval& ti : m_intervals)
            {
                result += ti.m_end - ti.m_begin;
            }
            return result;
        }
    };

    struct FrameCommandMetrics
    {
        struct FrameInterval
        {
            double m_begin;
            double m_end;
        };
        AZStd::vector<FrameInterval> m_intervals;
        AZStd::vector<FrameInterval> m_rawIntervals;
        void Init()
        {
            m_intervals.clear();
            m_rawIntervals.clear();
        }
    };
}
