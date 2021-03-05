/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#pragma once

#include "DX12QueryHeap.hpp"

#include <AzCore/std/containers/vector.h>

#define DX12_GPU_PROFILE_MODE_OFF       0       // Turn off profiling
#define DX12_GPU_PROFILE_MODE_BASIC     1       // Profiles command list lifetime
#define DX12_GPU_PROFILE_MODE_DETAIL    2       // Profiles draw call state changes
#define DX12_GPU_PROFILE_MODE DX12_GPU_PROFILE_MODE_OFF

namespace DX12
{
    struct Timer
    {
        const char* m_Name;
        AZ::u64 m_Timestamp;
        AZ::u32 m_Duration;
    };

    class CommandList;

    using TimerHandle = AZ::u32;

    class TimerHeap
    {
    public:
        TimerHeap(Device& device);

        void Init(AZ::u32 timerCountMax);
        void Shutdown();

        void Begin();
        void End(CommandList& commandList);

        TimerHandle BeginTimer(CommandList& commandList, const char* name);
        void EndTimer(CommandList& commandList, TimerHandle handle);

        void ReadbackTimers();

        const AZStd::vector<Timer>& GetTimers() const
        {
            return m_Timers;
        }

    private:
        QueryHeap m_TimestampHeap;
        SmartPtr<ID3D12Resource> m_TimestampDownloadBuffer;
        AZStd::vector<Timer> m_Timers;
        AZ::u32 m_TimerCountMax;
    };

    class ScopedTimer
    {
    public:
        ScopedTimer(TimerHeap& timers, CommandList& commandList, const char* name)
            : m_Timers(timers)
            , m_CommandList(commandList)
        {
            m_Handle = timers.BeginTimer(commandList, name);
        }

        ~ScopedTimer()
        {
            m_Timers.EndTimer(m_CommandList, m_Handle);
        }

    private:
        TimerHeap& m_Timers;
        CommandList& m_CommandList;
        TimerHandle m_Handle;
    };
}

    // Conditionally disable timing at compile-time based on profile policy
#if DX12_GPU_PROFILE_MODE == DX12_GPU_PROFILE_MODE_DETAIL
#define DX12_COMMANDLIST_TIMER(name) DX12::ScopedTimer timer__(m_Timers, *this, name);
#define DX12_COMMANDLIST_TIMER_DETAIL(name) DX12_COMMANDLIST_TIMER(name)
#elif DX12_GPU_PROFILE_MODE == DX12_GPU_PROFILE_MODE_BASIC
#define DX12_COMMANDLIST_TIMER(name) DX12::ScopedTimer timer__(m_Timers, *this, name);
#define DX12_COMMANDLIST_TIMER_DETAIL(name)
#else
#define DX12_COMMANDLIST_TIMER(name)
#define DX12_COMMANDLIST_TIMER_DETAIL(name)
#endif
