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

#include "RenderDll_precompiled.h"
#include "GPUTimer.h"
#include "DriverD3D.h"
#include <AzCore/Debug/EventTraceDrillerBus.h>

bool CD3DProfilingGPUTimer::s_bTimingEnabled = false;
bool CD3DProfilingGPUTimer::s_bTimingAllowed = true;

namespace EventTrace
{
    const AZStd::thread_id GpuThreadId{ (AZStd::native_thread_id_type)1 };
    const char* GpuThreadName = "GPU";
    const char* GpuCategory = "GPU";
}

void CD3DProfilingGPUTimer::EnableTiming()
{

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define GPUTIMER_CPP_SECTION_1 1
#define GPUTIMER_CPP_SECTION_2 2
#define GPUTIMER_CPP_SECTION_3 3
#define GPUTIMER_CPP_SECTION_4 4
#define GPUTIMER_CPP_SECTION_5 5
#endif

#ifdef ENABLE_PROFILING_GPU_TIMERS
    if (!s_bTimingEnabled && s_bTimingAllowed)
    {
        s_bTimingEnabled = true;
    }
#endif
}

void CD3DProfilingGPUTimer::DisableTiming()
{
#ifdef ENABLE_PROFILING_GPU_TIMERS
    if (s_bTimingEnabled)
    {
        s_bTimingEnabled = false;
    }
#endif
}

void CD3DProfilingGPUTimer::AllowTiming()
{
    s_bTimingAllowed = true;
}

void CD3DProfilingGPUTimer::DisallowTiming()
{
    s_bTimingAllowed = false;
    if (gcpRendD3D->m_pPipelineProfiler)
    {
        gcpRendD3D->m_pPipelineProfiler->ReleaseGPUTimers();
    }
    DisableTiming();
}

void CD3DProfilingGPUTimer::Start([[maybe_unused]] const char* name)
{
#ifdef ENABLE_PROFILING_GPU_TIMERS
    if (s_bTimingEnabled)
    {
        CD3DGPUTimer::Start(name);
    }
#endif
}

void CD3DProfilingGPUTimer::Stop()
{
#ifdef ENABLE_PROFILING_GPU_TIMERS
    if (s_bTimingEnabled)
    {
        CD3DGPUTimer::Stop();
    }
#endif
}

void CD3DProfilingGPUTimer::UpdateTime()
{
#ifdef ENABLE_PROFILING_GPU_TIMERS
    if (s_bTimingEnabled)
    {
        CD3DGPUTimer::UpdateTime();
    }
#endif
}

bool CD3DProfilingGPUTimer::Init()
{
#ifdef ENABLE_PROFILING_GPU_TIMERS
    return CD3DGPUTimer::Init();
#else
    return false;
#endif
}

CD3DGPUTimer::CD3DGPUTimer()
{
    m_time = 0.f;
    m_smoothedTime = 0.f;
    m_bInitialized = false;
    m_bWaiting = false;
    m_bStarted = false;

#if GPUTIMER_CPP_TRAIT_CSIMPLEGPUTIMER_SETQUERYSTART
    m_pQueryStart = m_pQueryStop = m_pQueryFreq = NULL;
#endif
}

CD3DGPUTimer::~CD3DGPUTimer()
{
    Release();
}

void CD3DGPUTimer::Start(const char* name)
{
#ifndef NULL_RENDERER
    if (!m_bWaiting && Init())
    {
        m_Name = name;

#if GPUTIMER_CPP_TRAIT_START_PRIMEQUERY
        // TODO: The d3d docs suggest that the disjoint query should be done once per frame at most
        gcpRendD3D->GetDeviceContext().Begin(m_pQueryFreq);
        gcpRendD3D->GetDeviceContext().End(m_pQueryStart);
        m_bStarted = true;
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GPUTIMER_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(GPUTimer_cpp)
#endif
    }
#endif
}

void CD3DGPUTimer::Stop()
{
#ifndef NULL_RENDERER
    if (m_bStarted && m_bInitialized)
    {
#if GPUTIMER_CPP_TRAIT_STOP_ENDQUERY
        gcpRendD3D->GetDeviceContext().End(m_pQueryStop);
        gcpRendD3D->GetDeviceContext().End(m_pQueryFreq);
        m_bStarted = false;
        m_bWaiting = true;
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GPUTIMER_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(GPUTimer_cpp)
#endif
    }
#endif
}

void CD3DGPUTimer::UpdateTime()
{
#ifndef NULL_RENDERER
    if (m_bWaiting && m_bInitialized)
    {
#if GPUTIMER_CPP_TRAIT_UPDATETIME_GETDATA
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
        UINT64 timeStart, timeStop;

        ID3D11DeviceContext& context = gcpRendD3D->GetDeviceContext();
        if (context.GetData(m_pQueryFreq, &disjointData, m_pQueryFreq->GetDataSize(), 0) == S_OK &&
            context.GetData(m_pQueryStart, &timeStart, m_pQueryStart->GetDataSize(), 0) == S_OK &&
            context.GetData(m_pQueryStop, &timeStop, m_pQueryStop->GetDataSize(), 0) == S_OK)
        {
            if (!disjointData.Disjoint)
            {
                float time = (timeStop - timeStart) / (float)(disjointData.Frequency / 1000);
                if (time >= 0.f && time < 1000.f)
                {
                    m_time = time;                                 // Filter out wrong insane values that get reported sometimes

                    RecordSlice(timeStart, timeStop, disjointData.Frequency);
                }
            }
            m_bWaiting = false;
        }
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GPUTIMER_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(GPUTimer_cpp)
#endif

        if (!m_bWaiting)
        {
            m_smoothedTime = m_smoothedTime * 0.7f + m_time * 0.3f;
        }
    }
    else
    {
        // Reset timers when not used in frame
        m_time = 0.0f;
        m_smoothedTime = 0.0f;
    }
#endif
}

void CD3DGPUTimer::RecordSlice([[maybe_unused]] AZ::u64 timeStart, [[maybe_unused]] AZ::u64 timeStop, [[maybe_unused]] AZ::u64 frequency)
{
#if defined(CRY_USE_DX12)
    {
        AZ::u64 cpuTimeStart = gcpRendD3D->GetDeviceContext().MakeCpuTimestampMicroseconds(timeStart);
        AZ::u32 durationInMicroseconds = (AZ::u32)((1000000 * (timeStop - timeStart)) / frequency);

        // Cannot queue event because string is not deep copied (and lifetime is not guaranteed until next update).
        EBUS_EVENT(AZ::Debug::EventTraceDrillerBus, RecordSlice, m_Name.c_str(), EventTrace::GpuCategory, EventTrace::GpuThreadId, cpuTimeStart, durationInMicroseconds);
    }
#endif
}

void CD3DGPUTimer::Release()
{
#if GPUTIMER_CPP_TRAIT_RELEASE_RELEASEQUERY
    SAFE_RELEASE(m_pQueryStart);
    SAFE_RELEASE(m_pQueryStop);
    SAFE_RELEASE(m_pQueryFreq);
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GPUTIMER_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(GPUTimer_cpp)
#endif
    m_bInitialized = false;
    m_bWaiting = false;
    m_bStarted = false;
    m_smoothedTime = 0.f;
}

bool CD3DGPUTimer::Init()
{
#ifndef NULL_RENDERER
    if (!m_bInitialized)
    {
#if GPUTIMER_CPP_TRAIT_INIT_CREATEQUERY
        D3D11_QUERY_DESC stampDisjointDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
        D3D11_QUERY_DESC stampDesc = { D3D11_QUERY_TIMESTAMP, 0 };

        if (gcpRendD3D->GetDevice().CreateQuery(&stampDisjointDesc, &m_pQueryFreq) == S_OK &&
            gcpRendD3D->GetDevice().CreateQuery(&stampDesc, &m_pQueryStart) == S_OK &&
            gcpRendD3D->GetDevice().CreateQuery(&stampDesc, &m_pQueryStop) == S_OK)
        {
            m_bInitialized = true;
        }
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION GPUTIMER_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(GPUTimer_cpp)
#endif

        EBUS_EVENT(AZ::Debug::EventTraceDrillerSetupBus, SetThreadName, EventTrace::GpuThreadId, EventTrace::GpuThreadName);
    }
#endif

    return m_bInitialized;
}
