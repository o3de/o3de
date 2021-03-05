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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_PIPELINEPROFILER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_PIPELINEPROFILER_H
#pragma once

#include "GPUTimer.h"


struct SStaticElementInfo
{
    SStaticElementInfo(uint _nPos)
        : nPos(_nPos)
        , bUsed(false) {}
    uint nPos;
    bool bUsed;
};

struct RPProfilerSection
{
    char             name[31];
    CCryNameTSCRC    path;
    int8             recLevel;  // Negative value means error in stack
    uint32           numDIPs, numPolys;
    CTimeValue       startTimeCPU, endTimeCPU;
    CD3DProfilingGPUTimer  gpuTimer;
};

struct RPPSectionsFrame
{
    RPPSectionsFrame()
        : m_numSections(0) {}

    static const uint32 MaxNumSections = 256;
    RPProfilerSection   m_sections[MaxNumSections];
    uint32              m_numSections;
};

struct RPThreadTimings
{
    float waitForMain;
    float waitForRender;
    float waitForGPU;
    float gpuIdlePerc;
    float gpuFrameTime;
    float frameTime;
    float renderTime;

    RPThreadTimings()
        : waitForMain(0)
        , waitForRender(0)
        , waitForGPU(0)
        , gpuIdlePerc(0)
        , gpuFrameTime(33.0f)
        , frameTime(33.0f)
        , renderTime(0)
    {
    }
};

class CRenderPipelineProfiler
{
public:
    CRenderPipelineProfiler();

    void BeginFrame();
    void EndFrame();
    void BeginSection(const char* name, uint32 eProfileLabelFlags = 0);
    void EndSection(const char* name);

    bool IsEnabled();
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    void SetWaitForGPUTimers(bool wait) { m_waitForGPUTimers = wait; }

    const RPProfilerStats& GetBasicStats(ERenderPipelineProfilerStats stat, int nThreadID) { assert((uint32)stat < RPPSTATS_NUM); return m_basicStats[nThreadID][stat]; }
    const RPProfilerStats* GetBasicStatsArray(int nThreadID) { return m_basicStats[nThreadID]; }

    void ReleaseGPUTimers();

protected:
    bool FilterLabel(const char* name);
    void UpdateThreadTimings();
    void ResetBasicStats(RPProfilerStats* pBasicStats, bool bResetAveragedStats);
    void ComputeAverageStats();
    void UpdateBasicStats();

    void DisplayAdvancedStats();
    void DisplayBasicStats();

    bool WaitForTimers() { return m_waitForGPUTimers; }

protected:
#if OPENGL
    static const uint32 NumSectionsFrames = 4;
#else
    static const uint32 NumSectionsFrames = 2;
#endif

    std::vector< uint32 >    m_stack;
    RPPSectionsFrame         m_sectionsFrames[NumSectionsFrames];
    uint32                   m_sectionsFrameIdx;
    float                    m_gpuSyncTime;
    float                    m_avgFrameTime;
    bool                     m_enabled;
    bool                     m_recordData;
    bool                     m_waitForGPUTimers;

    RPProfilerStats          m_basicStats[RT_COMMAND_BUF_COUNT][RPPSTATS_NUM];
    RPThreadTimings          m_threadTimings;

    // we take a snapshot every now and then and store it in here to prevent the text from jumping too much
    std::multimap<CCryNameTSCRC, SStaticElementInfo> m_staticNameList;
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_PIPELINEPROFILER_H
