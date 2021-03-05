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
#include "PipelineProfiler.h"
#include "DriverD3D.h"
#include "../Common/Textures/TextureManager.h"


CRenderPipelineProfiler::CRenderPipelineProfiler()
{
    m_sectionsFrameIdx = 0;
    m_gpuSyncTime = 0;
    m_avgFrameTime = 0;
    m_enabled = false;
    m_recordData = false;
    m_waitForGPUTimers = false;

    m_stack.reserve(8);

    for (int i = 0; i < RT_COMMAND_BUF_COUNT; i++)
    {
        ResetBasicStats(m_basicStats[i], true);
    }
}


void CRenderPipelineProfiler::BeginFrame()
{
    AZ_TRACE_METHOD();
    m_recordData = IsEnabled();

    if (gEnv->IsEditor() && !gcpRendD3D->m_CurrContext->m_bMainViewport)
    {
        m_recordData = false;
    }

    if (m_recordData)
    {
        CD3DProfilingGPUTimer::EnableTiming();
    }

    uint32 nextSectionsFrameIdx = (m_sectionsFrameIdx + 1) % NumSectionsFrames;
    RPPSectionsFrame& nextSectionsFrame = m_sectionsFrames[nextSectionsFrameIdx];

    if (nextSectionsFrame.m_numSections > 0)
    {
        RPProfilerSection& lastSection = nextSectionsFrame.m_sections[nextSectionsFrame.m_numSections - 1];

        lastSection.gpuTimer.UpdateTime();
        if (lastSection.gpuTimer.HasPendingQueries())
        {
            // Don't record new data if there are still pending GPU time queries, otherwise results may be wrong when labels change between frames
            m_recordData = false;
            return;
        }
    }

    m_sectionsFrameIdx = nextSectionsFrameIdx;

    if (!WaitForTimers())
    {
        UpdateBasicStats();
    }

    nextSectionsFrame.m_numSections = 0;

    BeginSection("FRAME", 0);
    if (m_recordData)
    {
        nextSectionsFrame.m_sections[0].numDIPs = 0;
        nextSectionsFrame.m_sections[0].numPolys = 0;
    }
}


void CRenderPipelineProfiler::EndFrame()
{
    EndSection("FRAME");

    RPPSectionsFrame& sectionsFrame = m_sectionsFrames[m_sectionsFrameIdx];

    if (!m_stack.empty())
    {
        sectionsFrame.m_sections[0].recLevel = -1;
        m_stack.resize(0);
    }

    m_gpuSyncTime = 0;

    if (sectionsFrame.m_numSections > 0 && WaitForTimers())
    {
        CTimeValue startTime = gEnv->pTimer->GetAsyncTime();
        RPProfilerSection& lastSection = sectionsFrame.m_sections[sectionsFrame.m_numSections - 1];

        // This will lower the overall framerate but gives accurate GPU times
        do
        {
            lastSection.gpuTimer.UpdateTime();
        } while (lastSection.gpuTimer.HasPendingQueries());

        m_gpuSyncTime = gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(startTime) * 1000.0f;

        UpdateBasicStats();
    }

    UpdateThreadTimings();

    m_recordData = false;

    // Don't display stats on anything but the main editor viewport
    // Also don't display stats if the renderer hasn't finished loading some default resources
    // We can't display stats properly without loading the default White texture
    if ((!gEnv->IsEditor() || (gEnv->IsEditor() && gcpRendD3D->m_CurrContext->m_bMainViewport))  && gEnv->pRenderer->HasLoadedDefaultResources())
    {
        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_profiler == 1)
        {
            DisplayBasicStats();
        }
        else if IsCVarConstAccess(constexpr) (CRenderer::CV_r_profiler == 2)
        {
            DisplayAdvancedStats();
        }
    }
}


void CRenderPipelineProfiler::ReleaseGPUTimers()
{
    for (uint32 i = 0; i < NumSectionsFrames; ++i)
    {
        RPPSectionsFrame& sectionsFrame = m_sectionsFrames[i];
        for (uint32 j = 0; j < sectionsFrame.m_numSections; ++j)
        {
            RPProfilerSection& section = sectionsFrame.m_sections[j];
            section.gpuTimer.Stop();
            section.gpuTimer.Release();
        }
    }
}


bool CRenderPipelineProfiler::FilterLabel(const char* name)
{
    return
        (strcmp(name, "SCREEN_STRETCH_RECT") == 0) ||
        (strcmp(name, "STRETCHRECT_EMU") == 0) ||
        (strcmp(name, "STENCIL_VOLUME") == 0) ||
        (strcmp(name, "DRAWSTRINGW") == 0);    // FIXME: Label stack error in DRAWSTRINGW
}


void CRenderPipelineProfiler::BeginSection(const char* name, [[maybe_unused]] uint32 eProfileLabelFlags)
{
    RPPSectionsFrame& sectionsFrame = m_sectionsFrames[m_sectionsFrameIdx];

    if (sectionsFrame.m_numSections >= RPPSectionsFrame::MaxNumSections)
    {
        m_recordData = false;
    }

    if (!m_recordData || FilterLabel(name))
    {
        return;
    }

    RPProfilerSection& section = sectionsFrame.m_sections[sectionsFrame.m_numSections++];

    cry_strcpy(section.name, name);

    section.recLevel = m_stack.size() + 1;
    section.numDIPs = gcpRendD3D->GetCurrentNumberOfDrawCalls();
    section.numPolys = gcpRendD3D->GetPolyCount();
    section.startTimeCPU = gEnv->pTimer->GetAsyncTime();
    section.gpuTimer.Start(name);

    m_stack.push_back(sectionsFrame.m_numSections - 1);

    string path = "";
    for (int i = 0; i < m_stack.size(); i++)
    {
        path += "\\";
        path += sectionsFrame.m_sections[m_stack[i]].name;
    }
    section.path = CCryNameTSCRC(path);
}


void CRenderPipelineProfiler::EndSection(const char* name)
{
    if (!m_recordData || FilterLabel(name))
    {
        return;
    }

    if (!m_stack.empty())
    {
        RPPSectionsFrame& sectionsFrame = m_sectionsFrames[m_sectionsFrameIdx];
        RPProfilerSection& section = sectionsFrame.m_sections[m_stack.back()];

        section.numDIPs = gcpRendD3D->GetCurrentNumberOfDrawCalls() - section.numDIPs;
        section.numPolys = gcpRendD3D->GetPolyCount() - section.numPolys;
        section.endTimeCPU = gEnv->pTimer->GetAsyncTime();
        section.gpuTimer.Stop();
        if (strncmp(section.name, name, 30) != 0)
        {
            section.recLevel = -section.recLevel;
        }

        m_stack.pop_back();
    }
}


void CRenderPipelineProfiler::UpdateThreadTimings()
{
    const float weight = 8.0f / 9.0f;
    const uint32 fillThreadID = (uint32)gcpRendD3D->m_RP.m_nFillThreadID;

    m_threadTimings.waitForMain = (gcpRendD3D->m_fTimeWaitForMain[fillThreadID] * (1.0f - weight) + m_threadTimings.waitForMain * weight);
    m_threadTimings.waitForRender = (gcpRendD3D->m_fTimeWaitForRender[fillThreadID] * (1.0f - weight) + m_threadTimings.waitForRender * weight);
    m_threadTimings.waitForGPU = (gcpRendD3D->m_fTimeWaitForGPU[fillThreadID] * (1.0f - weight) + m_threadTimings.waitForGPU * weight);
    m_threadTimings.gpuIdlePerc = (gcpRendD3D->m_fTimeGPUIdlePercent[fillThreadID] * (1.0f - weight) + m_threadTimings.gpuIdlePerc * weight);
    m_threadTimings.gpuFrameTime = (gcpRendD3D->m_fTimeProcessedGPU[fillThreadID] * (1.0f - weight) + m_threadTimings.gpuFrameTime * weight);
    m_threadTimings.frameTime = (iTimer->GetRealFrameTime() * (1.0f - weight) + m_threadTimings.frameTime * weight);
    m_threadTimings.renderTime = min((gcpRendD3D->m_fTimeProcessedRT[fillThreadID] * (1.0f - weight) + m_threadTimings.renderTime * weight), m_threadTimings.frameTime);
}


ILINE void AddToStats(RPProfilerStats& outStats, RPProfilerSection& section)
{
    outStats.gpuTime += section.gpuTimer.GetTime();
    outStats.cpuTime += section.endTimeCPU.GetDifferenceInSeconds(section.startTimeCPU) * 1000.0f;
    outStats.numDIPs += section.numDIPs;
    outStats.numPolys += section.numPolys;
}

ILINE void SubtractFromStats(RPProfilerStats& outStats, RPProfilerSection& section)
{
    outStats.gpuTime -= section.gpuTimer.GetTime();
    outStats.cpuTime -= section.endTimeCPU.GetDifferenceInSeconds(section.startTimeCPU) * 1000.0f;
    outStats.numDIPs -= section.numDIPs;
    outStats.numPolys -= section.numPolys;
}



void CRenderPipelineProfiler::ResetBasicStats(RPProfilerStats* pBasicStats, bool bResetAveragedStats)
{
    for (uint32 i = 0; i < RPPSTATS_NUM; ++i)
    {
        RPProfilerStats& basicStat = pBasicStats[i];
        basicStat.gpuTime = 0.0f;
        basicStat.cpuTime = 0.0f;
        basicStat.numDIPs = 0;
        basicStat.numPolys = 0;
    }

    if (bResetAveragedStats)
    {
        for (uint32 i = 0; i < RPPSTATS_NUM; ++i)
        {
            RPProfilerStats& basicStat = pBasicStats[i];
            basicStat.gpuTimeSmoothed = 0.0f;
            basicStat.gpuTimeMax = 0.0f;
            basicStat._gpuTimeMaxNew = 0.0f;
        }
    }
}

void CRenderPipelineProfiler::ComputeAverageStats()
{
    static int s_frameCounter = 0;
    const int kUpdateFrequency = 60;

    const uint32 processThreadID = (uint32)gcpRendD3D->m_RP.m_nProcessThreadID;
    const uint32 fillThreadID = (uint32)gcpRendD3D->m_RP.m_nFillThreadID;

    // GPU times
    for (uint32 i = 0; i < RPPSTATS_NUM; ++i)
    {
        RPProfilerStats& basicStat = m_basicStats[processThreadID][i];
        basicStat.gpuTimeSmoothed = 0.9f * m_basicStats[fillThreadID][i].gpuTimeSmoothed + 0.1f * basicStat.gpuTime;
        float gpuTimeMax = std::max(basicStat._gpuTimeMaxNew, m_basicStats[fillThreadID][i]._gpuTimeMaxNew);
        basicStat._gpuTimeMaxNew = std::max(gpuTimeMax, basicStat.gpuTime);

        if (s_frameCounter % kUpdateFrequency == 0)
        {
            basicStat.gpuTimeMax = basicStat._gpuTimeMaxNew;
            basicStat._gpuTimeMaxNew = 0;
        }
    }

    s_frameCounter += 1;
}

void CRenderPipelineProfiler::UpdateBasicStats()
{
    RPProfilerStats* pBasicStats = m_basicStats[gcpRendD3D->m_RP.m_nProcessThreadID];

    ResetBasicStats(pBasicStats, false);

    bool bRecursivePass = false;
    RPPSectionsFrame& sectionsFrame = m_sectionsFrames[m_sectionsFrameIdx];

    for (uint32 i = 0; i < sectionsFrame.m_numSections; ++i)
    {
        RPProfilerSection& section = sectionsFrame.m_sections[i];

        section.gpuTimer.UpdateTime();

        if (strcmp(section.name, "SCENE_REC") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_Recursion], section);
            bRecursivePass = true;
        }
        else if (strcmp(section.name, "SCENE") == 0)
        {
            bRecursivePass = false;
        }

        if (bRecursivePass)
        {
            continue;
        }

        // Scene
        if (strcmp(section.name, "GBUFFER") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_SceneOverall], section);
        }
        else if (strcmp(section.name, "DECALS") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_SceneDecals], section);
        }
        else if (strcmp(section.name, "DEFERRED_DECALS") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_SceneOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_SceneDecals], section);
        }
        else if (strcmp(section.name, "OPAQUE_PASSES") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_SceneOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_SceneForward], section);
        }
        else if (strcmp(section.name, "WATER") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_SceneOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_SceneWater], section);
        }

        // Shadows
        else if (strcmp(section.name, "SHADOWMAPS SUN") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_ShadowsOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_ShadowsSun], section);
        }
        else if (strcmp(section.name, "CUSTOM MAPS") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_ShadowsSunCustom], section);
        }
        else if (strcmp(section.name, "SHADOWMAP_POOL") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_ShadowsOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_ShadowsLocal], section);
        }


        // Lighting
        else if (strcmp(section.name, "TILED_SHADING") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_LightingOverall], section);
        }
        else if (strcmp(section.name, "DEFERRED_SHADING") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_LightingOverall], section);
        }
        else if (strcmp(section.name, "DEFERRED_CUBEMAPS") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_LightingOverall], section);
        }
        else if (strcmp(section.name, "DEFERRED_LIGHTS") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_LightingOverall], section);
        }
        else if (strcmp(section.name, "AMBIENT_PASS") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_LightingOverall], section);
        }
        else if (strcmp(section.name, "SVOGI") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_LightingOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_LightingGI], section);
        }

        // VFX
        else if (strcmp(section.name, "TRANSPARENT_BW") == 0 || strcmp(section.name, "TRANSPARENT_AW") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_VfxOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_VfxTransparent], section);
        }
        else if (strcmp(section.name, "FOG_GLOBAL") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_VfxOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_VfxFog], section);
        }
        else if (strcmp(section.name, "VOLUMETRIC FOG") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_VfxOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_VfxFog], section);
        }
        else if (strcmp(section.name, "DEFERRED_RAIN") == 0 || strcmp(section.name, "RAIN") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_VfxOverall], section);
        }
        else if (strcmp(section.name, "LENS_OPTICS") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_VfxOverall], section);
            AddToStats(pBasicStats[eRPPSTATS_VfxFlares], section);
        }
        else if (strcmp(section.name, "OCEAN CAUSTICS") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_VfxOverall], section);
        }
        else if (strcmp(section.name, "WATERVOLUME_CAUSTICS") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_VfxOverall], section);
        }

        // Extra TI states
        else if (strcmp(section.name, "TI_INJECT_CLEAR") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_INJECT_CLEAR], section);
        }
        else if (strcmp(section.name, "TI_VOXELIZE") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_VOXELIZE], section);
        }
        else if (strcmp(section.name, "TI_INJECT_LIGHT") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_INJECT_LIGHT], section);
        }
        else if (strcmp(section.name, "TI_INJECT_AIR") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_INJECT_AIR], section);
        }
        else if (strcmp(section.name, "TI_INJECT_REFL0") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_INJECT_REFL0], section);
        }
        else if (strcmp(section.name, "TI_INJECT_REFL1") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_INJECT_REFL1], section);
        }
        else if (strcmp(section.name, "TI_INJECT_DYNL") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_INJECT_DYNL], section);
        }
        else if (strcmp(section.name, "TI_NID_DIFF") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_NID_DIFF], section);
        }
        else if (strcmp(section.name, "TI_GEN_DIFF") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_GEN_DIFF], section);
        }
        else if (strcmp(section.name, "TI_GEN_SPEC") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_GEN_SPEC], section);
        }
        else if (strcmp(section.name, "TI_GEN_AIR") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_GEN_AIR], section);
        }
        else if (strcmp(section.name, "TI_UPSCALE_DIFF") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_UPSCALE_DIFF], section);
        }
        else if (strcmp(section.name, "TI_UPSCALE_SPEC") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_UPSCALE_SPEC], section);
        }
        else if (strcmp(section.name, "TI_DEMOSAIC_DIFF") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_DEMOSAIC_DIFF], section);
        }
        else if (strcmp(section.name, "TI_DEMOSAIC_SPEC") == 0)
        {
            AddToStats(pBasicStats[eRPPSTATS_TI_DEMOSAIC_SPEC], section);
        }
    }

    AddToStats(pBasicStats[eRPPSTATS_OverallFrame], sectionsFrame.m_sections[0]);

    ComputeAverageStats();
}

void CRenderPipelineProfiler::DisplayAdvancedStats()
{
#ifndef _RELEASE
    CD3D9Renderer* rd = gcpRendD3D;

    RPPSectionsFrame& sectionsFrame = m_sectionsFrames[m_sectionsFrameIdx];
    uint32 elemsPerColumn = (gcpRendD3D->GetHeight() - 60) / 16;

    ColorF color = sectionsFrame.m_numSections >= RPPSectionsFrame::MaxNumSections ? Col_Red : ColorF(1.0f, 1.0f, 0.2f, 1);
    m_avgFrameTime = 0.8f * gEnv->pTimer->GetRealFrameTime() + 0.2f * m_avgFrameTime; // exponential moving average for frame time

    rd->Draw2dLabel(20, 10, 1.7f, &color.r, false, "FPS %.1f  GPU Sync %.1fms", 1.0f / m_avgFrameTime, m_gpuSyncTime);
    color.r = color.g = color.b = 0.35f;
    rd->Draw2dLabel(320, 10, 1.5f, &color.r, false, "GPU");
    rd->Draw2dLabel(400, 10, 1.5f, &color.r, false, "CPU");
    rd->Draw2dLabel(470, 10, 1.5f, &color.r, false, "DIPs");
    rd->Draw2dLabel(520, 10, 1.5f, &color.r, false, "Polys");

    if (sectionsFrame.m_numSections > elemsPerColumn)
    {
        rd->Draw2dLabel(320 + 600, 10, 1.5f, &color.r, false, "GPU");
        rd->Draw2dLabel(400 + 600, 10, 1.5f, &color.r, false, "CPU");
        rd->Draw2dLabel(470 + 600, 10, 1.5f, &color.r, false, "DIPs");
        rd->Draw2dLabel(520 + 600, 10, 1.5f, &color.r, false, "Polys");
    }

    // refresh the list every 3 seconds to clear out old data and reduce gaps
    CTimeValue currentTime = gEnv->pTimer->GetAsyncTime();
    static CTimeValue lastClearTime = currentTime;

    if (currentTime.GetDifferenceInSeconds(lastClearTime) > 3.0f)
    {
        lastClearTime = currentTime;
        m_staticNameList.clear();
    }

    // reset the used flag
    for (std::multimap<CCryNameTSCRC, SStaticElementInfo>::iterator it = m_staticNameList.begin(); it != m_staticNameList.end(); ++it)
    {
        it->second.bUsed = false;
    }

    static float gpuTimeAverage[2] = { 10, 10 };
    gpuTimeAverage[1] = gpuTimeAverage[0];

    for (uint32 i = 0; i < sectionsFrame.m_numSections; ++i)
    {
        RPProfilerSection& section = sectionsFrame.m_sections[i];

        // find the slot we should render to according to the static list - if not found, insert
        std::multimap<CCryNameTSCRC, SStaticElementInfo>::iterator it = m_staticNameList.lower_bound(section.path);
        while (it != m_staticNameList.end() && it->second.bUsed)
        {
            ++it;
        }

        // not found: either a new element or a duplicate - insert at correct position and render there
        if (it == m_staticNameList.end() || it->first != section.path)
        {
            it = m_staticNameList.insert(std::make_pair(section.path, i));
        }

        it->second.bUsed = true;

        float gpuTime = section.gpuTimer.GetTime();
        float cpuTime = section.endTimeCPU.GetDifferenceInSeconds(section.startTimeCPU);
        float ypos = 30.0f + (it->second.nPos % elemsPerColumn) * 16.0f;
        float xpos = 20.0f + ((int)(it->second.nPos / elemsPerColumn)) * 600.0f;
        color.r = color.g = color.b = min(0.4f + gpuTime * 0.4f, 0.9f);

        if (section.recLevel < 0)   // Label stack error
        {
            color.r = 1;
            color.g = color.b = 0;
        }
        else if (i == 0)   // Special case for FRAME section
        {
            color.r = 1;
            color.g = 1;
            color.b = 0.2f;
        }
        else if (max(gpuTime, cpuTime) > gpuTimeAverage[1] * 0.75f)
        {
            color.r = color.g = color.b = 1; // highlight heavy elements
        }
        rd->Draw2dLabel(xpos + max((int)(abs(section.recLevel) - 2), 0) * 15.0f, ypos, 1.5f, &color.r, false, section.name);
        rd->Draw2dLabel(xpos + 300, ypos, 1.5f, &color.r, false, "%.2fms", gpuTime);
        rd->Draw2dLabel(xpos + 380, ypos, 1.5f, &color.r, false, "%.2fms", cpuTime * 1000.0f);
        rd->Draw2dLabel(xpos + 450, ypos, 1.5f, &color.r, false, "%i", section.numDIPs);
        rd->Draw2dLabel(xpos + 500, ypos, 1.5f, &color.r, false, "%i", section.numPolys);

        if (i != 0)
        {
            gpuTimeAverage[0] += (max(gpuTime, cpuTime) - gpuTimeAverage[0]) * 0.05f;
        }
    }

    rd->RT_RenderTextMessages();
#endif
}


namespace DebugUI
{
    const float columnHeight = 0.027f;

    void DrawText(float x, float y, float size, ColorF color, const char* text)
    {
        CD3D9Renderer* rd = gcpRendD3D;
        float aspect = (float)rd->GetOverlayWidth() / (float)rd->GetOverlayHeight();
        float sx = VIRTUAL_SCREEN_WIDTH / aspect;
        float sy = VIRTUAL_SCREEN_HEIGHT;

        SDrawTextInfo ti;
        ti.xscale = size * 1.55f / aspect;
        ti.yscale = size * 1.1f;
        ti.color[0] = color.r;
        ti.color[1] = color.r;
        ti.color[2] = color.b;
        ti.color[3] = color.a;
        ti.flags = eDrawText_800x600 | eDrawText_2D;
        rd->Draw2dText(x * sx, y * sy, text, ti);
    }

    void DrawText(float x, float y, float size, ColorF color, const char* format, va_list args)
    {
        char buffer[512];
        if (azvsnprintf(buffer, sizeof(buffer), format, args) == -1)
        {
            buffer[sizeof(buffer) - 1] = 0;
        }

        DrawText(x, y, size, color, buffer);
    }

    void DrawBox(float x, float y, float width, float height, ColorF color)
    {
        //s_ptexWhite is not supposed to be nullptr here
        //This can happen if the profiler is asked to draw before the level loads
        AZ_Assert(CTextureManager::Instance()->GetWhiteTexture() != nullptr, "Pipeline Profiler tried to draw a box but White textue was nullptr! Have the default texture resources been loaded yet?");

        CD3D9Renderer* rd = gcpRendD3D;
        float aspect = (float)rd->GetOverlayWidth() / (float)rd->GetOverlayHeight();
        float sx = VIRTUAL_SCREEN_WIDTH / aspect;
        float sy = VIRTUAL_SCREEN_HEIGHT;
        const Vec2 overscanOffset = Vec2(rd->s_overscanBorders.x * VIRTUAL_SCREEN_WIDTH, rd->s_overscanBorders.y * VIRTUAL_SCREEN_HEIGHT);
        rd->Draw2dImage(x * sx + overscanOffset.x, y * sy + overscanOffset.y, width * sx, height * sy, CTextureManager::Instance()->GetWhiteTexture()->GetID(), 0, 0, 1, 1, 0, color);
    }


    void DrawTable(float x, float y, float width, int numColumns, const char* title)
    {
        DrawBox(x, y, width, columnHeight, ColorF(0.45f, 0.45f, 0.55f, 0.6f));
        DrawBox(x, y + columnHeight, width, columnHeight * (float)numColumns + 0.007f, ColorF(0.05f, 0.05f, 0.05f, 0.6f));
        DrawText(x + 0.006f, y + 0.004f, 1, Col_Salmon, title);
    }

    void DrawTableColumn(float tableX, float tableY, int columnIndex, const char* columnText, ...)
    {
        va_list args;
        va_start(args, columnText);
        DrawText(tableX + 0.02f, tableY + (float)(columnIndex + 1) * columnHeight + 0.005f, 1, Col_White, columnText, args);
        va_end(args);
    }

    void DrawTableBar(float x, float tableY, int columnIndex, float percentage, ColorF color)
    {
        const float barHeight = 0.02f;
        const float barWidth = 0.15f;

        DrawBox(x, tableY + (float)(columnIndex + 1) * columnHeight + (columnHeight - barHeight) * 0.5f + 0.005f,
            barWidth, barHeight, ColorF(1, 1, 1, 0.2f));

        DrawBox(x, tableY + (float)(columnIndex + 1) * columnHeight + (columnHeight - barHeight) * 0.5f + 0.005f,
            percentage * barWidth, barHeight, ColorF(color.r, color.g, color.b, 0.7f));
    }
}


void CRenderPipelineProfiler::DisplayBasicStats()
{
#ifndef _RELEASE
    if (gEnv->pConsole->IsOpened())
    {
        return;
    }

    CD3D9Renderer* rd = gcpRendD3D;

    struct StatsGroup
    {
        char                          name[32];
        ERenderPipelineProfilerStats  statIndex;
    };

    StatsGroup statsGroups[] = {
        { "Frame", eRPPSTATS_OverallFrame },
        { "  Ocean Reflections", eRPPSTATS_Recursion },
        { "  Scene", eRPPSTATS_SceneOverall },
        { "    Decals", eRPPSTATS_SceneDecals },
        { "    Forward", eRPPSTATS_SceneForward },
        { "    Water", eRPPSTATS_SceneWater },
        { "  Shadows", eRPPSTATS_ShadowsOverall },
        { "    Sun", eRPPSTATS_ShadowsSun },
        { "    Per-Object", eRPPSTATS_ShadowsSunCustom },
        { "    Local", eRPPSTATS_ShadowsLocal },
        { "  Lighting", eRPPSTATS_LightingOverall },
        { "    Voxel GI", eRPPSTATS_LightingGI },
        { "  VFX", eRPPSTATS_VfxOverall },
        { "    Particles/Glass", eRPPSTATS_VfxTransparent },
        { "    Fog", eRPPSTATS_VfxFog },
        { "    Flares", eRPPSTATS_VfxFlares },
    };

    rd->SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);

    // Threading info
    {
        DebugUI::DrawTable(0.05f, 0.1f, 0.45f, 4, "Overview");

        float frameTime = m_threadTimings.frameTime;
        float mainThreadTime = max(m_threadTimings.frameTime - m_threadTimings.waitForRender, 0.0f);
        float renderThreadTime = max(m_threadTimings.renderTime - m_threadTimings.waitForGPU, 0.0f);
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(PipelineProfiler_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        float gpuTime = max(m_threadTimings.gpuFrameTime, 0.0f);
#endif
        float waitForGPU = max(m_threadTimings.waitForGPU, 0.0f);

        DebugUI::DrawTableBar(0.335f, 0.1f, 0, mainThreadTime / frameTime, Col_Yellow);
        DebugUI::DrawTableBar(0.335f, 0.1f, 1, renderThreadTime / frameTime, Col_Green);
        DebugUI::DrawTableBar(0.335f, 0.1f, 2, gpuTime / frameTime, Col_Cyan);
        DebugUI::DrawTableBar(0.335f, 0.1f, 3, waitForGPU / frameTime, Col_Red);

        DebugUI::DrawTableColumn(0.05f, 0.1f, 0, "Main Thread             %6.2f ms", mainThreadTime * 1000.0f);
        DebugUI::DrawTableColumn(0.05f, 0.1f, 1, "Render Thread           %6.2f ms", renderThreadTime * 1000.0f);
        DebugUI::DrawTableColumn(0.05f, 0.1f, 2, "GPU                     %6.2f ms", gpuTime * 1000.0f);
        DebugUI::DrawTableColumn(0.05f, 0.1f, 3, "CPU waits for GPU       %6.2f ms", waitForGPU * 1000.0f);
    }

    // GPU times
    {
        const float targetFrameTime = 1000.0f / CRenderer::CV_r_profilerTargetFPS;

        int numColumns = sizeof(statsGroups) / sizeof(StatsGroup);
        DebugUI::DrawTable(0.05f, 0.27f, 0.45f, numColumns, "GPU Time");

        RPProfilerStats* pBasicStats = m_basicStats[gcpRendD3D->m_RP.m_nProcessThreadID];
        for (uint32 i = 0; i < numColumns; ++i)
        {
            const RPProfilerStats& stats = pBasicStats[statsGroups[i].statIndex];
            DebugUI::DrawTableColumn(0.05f, 0.27f, i, "%-20s  %4.1f ms  %2.0f %%", statsGroups[i].name, stats.gpuTimeSmoothed, stats.gpuTimeSmoothed / targetFrameTime * 100.0f);
        }
    }

    rd->RT_RenderTextMessages();
#endif
}

bool CRenderPipelineProfiler::IsEnabled()
{
    return m_enabled || CRenderer::CV_r_profiler;
}
