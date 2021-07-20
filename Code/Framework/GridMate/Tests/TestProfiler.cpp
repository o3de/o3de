/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Tests.h"
#include "TestProfiler.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Crc.h>

#include <GridMate/Containers/set.h>
#include <GridMate/Containers/unordered_set.h>

using namespace GridMate;

typedef set<const AZ::Debug::ProfilerRegister*> ProfilerSet;

static bool CollectPerformanceCounters(const AZ::Debug::ProfilerRegister& reg, const AZStd::thread_id&, ProfilerSet& profilers, const char* systemId)
{
    if (reg.m_type != AZ::Debug::ProfilerRegister::PRT_TIME)
    {
        return true;
    }
    if (reg.m_systemId != AZ::Crc32(systemId))
    {
        return true;
    }

    const AZ::Debug::ProfilerRegister* profReg = &reg;
    profilers.insert(profReg);
    return true;
}

static string FormatString(const string& pre, const string& name, const string& post, AZ::u64 time, AZ::u64 calls)
{
    string units = "us";
    if (AZ::u64 divtime = time / 1000)
    {
        time = divtime;
        units = "ms";
    }
    return string::format("%s%s %s %10llu%s (%llu calls)\n", pre.c_str(), name.c_str(), post.c_str(), time, units.c_str(), calls);
}

struct TotalSortContainer
{
    TotalSortContainer(const AZ::Debug::ProfilerRegister* self = nullptr)
    {
        m_self = self;
    }

    void Print(AZ::s32 level, const char* systemId)
    {
        if (m_self && level >= 0)
        {
            string levelIndent;
            for (AZ::s32 i = 0; i < level; i++)
            {
                levelIndent += (i == level - 1) ? "+---" : "|   ";
            }
            string name = m_self->m_name ? m_self->m_name : m_self->m_function;
            string outputTotal = FormatString(levelIndent, name, " Total:", m_self->m_timeData.m_time, m_self->m_timeData.m_calls);
            AZ_Printf(systemId, outputTotal.c_str());

            if (m_self->m_timeData.m_childrenTime || m_self->m_timeData.m_childrenCalls)
            {
                string childIndent = levelIndent;
                for (auto i = name.begin(); i != name.end(); ++i)
                {
                    childIndent += " ";
                }
                childIndent[level * 4] = '|';

                string outputChild = FormatString(childIndent, "", "Child:", m_self->m_timeData.m_childrenTime, m_self->m_timeData.m_childrenCalls);
                AZ_Printf(systemId, outputChild.c_str());

                string outputSelf = FormatString(childIndent, "", "Self :", m_self->m_timeData.m_time - m_self->m_timeData.m_childrenTime, m_self->m_timeData.m_calls);
                AZ_Printf(systemId, outputSelf.c_str());
            }
        }

        for (auto i = m_children.begin(); i != m_children.end(); ++i)
        {
            i->Print(level + 1, systemId);
        }
    }

    TotalSortContainer* Find(const AZ::Debug::ProfilerRegister* obj)
    {
        if (m_self == obj)
        {
            return this;
        }

        for (TotalSortContainer& child : m_children)
        {
            TotalSortContainer* found = child.Find(obj);
            if (found)
            {
                return found;
            }
        }

        return nullptr;
    }

    struct TotalSorter
    {
        bool operator()(const TotalSortContainer& a, const TotalSortContainer& b) const
        {
            if (a.m_self->m_timeData.m_time == b.m_self->m_timeData.m_time)
            {
                return a.m_self > b.m_self;
            }
            return a.m_self->m_timeData.m_time > b.m_self->m_timeData.m_time;
        }
    };
    set<TotalSortContainer, TotalSorter> m_children;
    const AZ::Debug::ProfilerRegister* m_self;
};

void TestProfiler::StartProfiling()
{
    StopProfiling();

    AZ::Debug::Profiler::Create();
}

void TestProfiler::StopProfiling()
{
    if (AZ::Debug::Profiler::IsReady())
    {
        AZ::Debug::Profiler::Destroy();
    }
}

void TestProfiler::PrintProfilingTotal(const char* systemId)
{
    if (!AZ::Debug::Profiler::IsReady())
    {
        return;
    }

    ProfilerSet profilers;
    AZ::Debug::Profiler::Instance().ReadRegisterValues(AZStd::bind(&CollectPerformanceCounters, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::ref(profilers), systemId));

    // Validate we wont get stuck in an infinite loop
    TotalSortContainer root;
    for (auto i = profilers.begin(); i != profilers.end(); )
    {
        const AZ::Debug::ProfilerRegister* profile = *i;
        if (profile->m_timeData.m_lastParent)
        {
            auto parent = profilers.find(profile->m_timeData.m_lastParent);
            if (parent == profilers.end())
            {
                // Error, just ignore this entry
                i = profilers.erase(i);
                continue;
            }
        }
        ++i;
    }

    // Put all root nodes into the final list
    for (auto i = profilers.begin(); i != profilers.end(); )
    {
        const AZ::Debug::ProfilerRegister* profile = *i;
        if (!profile->m_timeData.m_lastParent)
        {
            root.m_children.insert(profile);
            i = profilers.erase(i);
        }
        else
        {
            ++i;
        }
    }

    // Put all non-root nodes into the final list
    while (!profilers.empty())
    {
        for (auto i = profilers.begin(); i != profilers.end(); )
        {
            const AZ::Debug::ProfilerRegister* profile = *i;
            TotalSortContainer* found = root.Find(profile->m_timeData.m_lastParent);
            if (found)
            {
                found->m_children.insert(profile);
                i = profilers.erase(i);
            }
            else
            {
                ++i;
            }
        }
    }

    AZ_Printf(systemId, "Profiling timers by total execution time:\n");
    root.Print(-1, systemId);
}

void TestProfiler::PrintProfilingSelf(const char* systemId)
{
    if (!AZ::Debug::Profiler::IsReady())
    {
        return;
    }

    ProfilerSet profilers;
    AZ::Debug::Profiler::Instance().ReadRegisterValues(AZStd::bind(&CollectPerformanceCounters, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::ref(profilers), systemId));

    struct SelfSorter
    {
        bool operator()(const AZ::Debug::ProfilerRegister* a, const AZ::Debug::ProfilerRegister* b) const
        {
            auto aTime = a->m_timeData.m_time - a->m_timeData.m_childrenTime;
            auto bTime = b->m_timeData.m_time - b->m_timeData.m_childrenTime;

            if (aTime == bTime)
            {
                return a > b;
            }
            return aTime > bTime;
        }
    };

    set<const AZ::Debug::ProfilerRegister*, SelfSorter> selfSorted;
    for (auto& profiler : profilers)
    {
        selfSorted.insert(profiler);
    }

    AZ_Printf(systemId, "Profiling timers by exclusive execution time:\n");
    for (auto profiler : selfSorted)
    {
        string str = FormatString("", profiler->m_name ? profiler->m_name : profiler->m_function, "Self Time:",
                profiler->m_timeData.m_time - profiler->m_timeData.m_childrenTime, profiler->m_timeData.m_calls);
        AZ_Printf(systemId, str.c_str());
    }
}
