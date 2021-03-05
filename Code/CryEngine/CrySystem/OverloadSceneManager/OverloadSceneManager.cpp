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

// Description : Manages overload values (eg CPU,GPU etc)
//               1.0="everything is ok"  0.0="very bad frame rate"
//               various systems can use this information and control what is currently in the scene


#include "CrySystem_precompiled.h"
#include "OverloadSceneManager.h"

//--------------------------------------------------------------------------------------------------
// Name: COverloadSceneManager
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
COverloadSceneManager::COverloadSceneManager()
{
    InitialiseCVars();

    m_currentFrameStat = 0;

    ResetDefaultValues();
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Desc: static console callback functions
//--------------------------------------------------------------------------------------------------
static void OnChange_osm_enabled(ICVar* pCVar)
{
    if (pCVar->GetIVal() == 0)
    {
        gEnv->pOverloadSceneManager->Reset();
        //gEnv->pRenderer->EnablePipelineProfiler(false); // would need push/pop since something else might have enabled it
    }
    else if (pCVar->GetIVal() == 1)
    {
        gEnv->pRenderer->EnablePipelineProfiler(true);
    }
}

static void cmd_setFBScale(IConsoleCmdArgs* pParams)
{
    int argCount = pParams->GetArgCount();

    Vec2 newScale = Vec2(0.0f, 0.0f);
    gEnv->pRenderer->EF_Query(EFQ_GetViewportDownscaleFactor, newScale);

    if (argCount > 1)
    {
        newScale.x = clamp_tpl((float)atof(pParams->GetArg(1)), 0.0f, 1.0f);

        if (argCount > 2)
        {
            newScale.y = clamp_tpl((float)atof(pParams->GetArg(2)), 0.0f, 1.0f);
        }
        else
        {
            newScale.y = newScale.x;
        }

        newScale = gEnv->pRenderer->SetViewportDownscale(newScale.x, newScale.y);
    }

    int nWidth = gEnv->pRenderer->GetWidth();
    int nHeight = gEnv->pRenderer->GetHeight();

    gEnv->pLog->LogWithType(ILog::eInputResponse, "Current Viewport Resolution: %dx%d",
        int(nWidth * newScale.x), int(nHeight * newScale.y));
}

//--------------------------------------------------------------------------------------------------
// Name: InitialiseCVars
// Desc: Initialises CVars
//--------------------------------------------------------------------------------------------------
void COverloadSceneManager::InitialiseCVars()
{
    // depending on final requirements, these could be made into const cvars
    REGISTER_CVAR_CB(osm_enabled, 0, VF_NULL, "Enables/disables overload scene manager", &OnChange_osm_enabled);
    REGISTER_CVAR(osm_historyLength, 5, VF_NULL, "Overload scene manager number of frames to record stats for");
    REGISTER_CVAR(osm_targetFPS, 28.0f, VF_NULL, "Overload scene manager target frame rate");
    REGISTER_CVAR(osm_targetFPSTolerance, 1.0f, VF_NULL, "The overload scene manager will make adjustments if fps is outside targetFPS +/- this value");
    REGISTER_CVAR(osm_fbScaleDeltaDown, 5.0f, VF_NULL, "The speed multiplier for the overload scene manager frame buffer scaling down");
    REGISTER_CVAR(osm_fbScaleDeltaUp, 1.0f, VF_NULL, "The speed multiplier for the overload scene manager frame buffer scaling up");
    REGISTER_CVAR(osm_fbMinScale, 0.66f, VF_NULL, "The minimum scale factor the overload scene manager will drop to");

    REGISTER_COMMAND("osm_setFBScale", cmd_setFBScale, VF_NULL, "Sets the framebuffer scale to either a single scale on both X and Y, or independent scales.\n"
        "NOTE: Will be overridden immediately if Overload scene manager is still enabled - see osm_enabled");
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Reset
// Desc: Resets overload scene manager outputs to sensible defaults
//--------------------------------------------------------------------------------------------------
void COverloadSceneManager::Reset()
{
    ResetDefaultValues();

    gEnv->pRenderer->SetViewportDownscale(m_fbScale, m_fbScale);
}

void COverloadSceneManager::ResetDefaultValues()
{
    m_fbScale = 1.0f;
    m_scaleState = FBSCALE_AUTO;
    m_lerpOverride.m_start = m_lerpOverride.m_length = 1.0f;
    m_lerpOverride.m_reversed = false;
    m_lerpAuto.m_start = m_lerpAuto.m_length = 1.0f;
    m_lerpAuto.m_reversed = true;
    m_fbAutoScale = m_fbOverrideDestScale = m_fbOverrideCurScale = 1.0f;

    // completely reset history
    for (int i = 0; i < osm_historyLength; i++)
    {
        SScenePerformanceStats& stats = m_sceneStats[i];
        stats.frameRate = stats.gpuFrameRate = osm_targetFPS;
    }

    m_smoothedSceneStats.frameRate = m_smoothedSceneStats.gpuFrameRate = osm_targetFPS;
}
//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Update
// Desc: Updates overload scene manager
//--------------------------------------------------------------------------------------------------
void COverloadSceneManager::Update()
{
    if (!osm_enabled)
    {
        return;
    }

    UpdateStats();
    ResizeFB();
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: OverrideScale and ResetScale
// Desc: sets up appropriate members for lerping between automatic framebuffer scale and manual
//       overrides.
//--------------------------------------------------------------------------------------------------
void COverloadSceneManager::OverrideScale(float frameScale, float dt)
{
    dt = max(dt, 1.0f / 1000.0f);

    float curTime = gEnv->pTimer->GetCurrTime();

    frameScale = clamp_tpl(frameScale, osm_fbMinScale, 1.0f);

    m_fbOverrideDestScale = frameScale;

    if (m_scaleState == FBSCALE_AUTO)
    {
        // remove any override lerp - we want to lerp straight from auto to the given value.
        m_lerpOverride.m_start = m_lerpOverride.m_length = 1.0f;
        m_lerpOverride.m_reversed = false;
        m_fbOverrideCurScale = frameScale;

        m_lerpAuto.m_start = curTime;
        m_lerpAuto.m_length = dt;
        m_lerpAuto.m_reversed = false;
    }
    else if (m_scaleState == FBSCALE_OVERRIDE)
    {
        m_lerpOverride.m_start = curTime;
        m_lerpOverride.m_length = dt;
        m_lerpOverride.m_reversed = false;

        m_fbOverrideCurScale = m_fbScale;
    }

    m_scaleState = FBSCALE_OVERRIDE;
}

void COverloadSceneManager::ResetScale(float dt)
{
    dt = max(dt, 1.0f / 1000.0f);

    float curTime = gEnv->pTimer->GetCurrTime();

    // remove any override lerp - we want to lerp straight to auto
    m_lerpOverride.m_start = m_lerpOverride.m_length = 1.0f;
    m_lerpOverride.m_reversed = false;
    m_fbOverrideCurScale = m_fbOverrideDestScale = m_fbScale;

    m_lerpAuto.m_start = curTime;
    m_lerpAuto.m_length = dt;
    m_lerpAuto.m_reversed = true; // want to lerp back to auto mode

    m_scaleState = FBSCALE_AUTO;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: UpdateStats
// Desc: Updates overload stats
//--------------------------------------------------------------------------------------------------
void COverloadSceneManager::UpdateStats()
{
    m_currentFrameStat++;

    if (m_currentFrameStat >= osm_historyLength)
    {
        m_currentFrameStat = 0;
    }

    SScenePerformanceStats& currentStats = m_sceneStats[m_currentFrameStat];
    const float frameLength = gEnv->pTimer->GetRealFrameTime() * 1000.0f;
    const float gpuFrameLength = gEnv->pRenderer->GetGPUFrameTime() * 1000.0f;

    currentStats.frameRate = frameLength > 0.0f ? 1000.0f / frameLength : 0.0f;
    currentStats.gpuFrameRate = gpuFrameLength > 0.0f ? 1000.0f / gpuFrameLength : 0.0f;

    CalculateSmoothedStats();
}//-------------------------------------------------------------------------------------------------

void COverloadSceneManager::CalculateSmoothedStats()
{
    m_smoothedSceneStats.Reset();

    for (int i = 0; i < osm_historyLength; i++)
    {
        SScenePerformanceStats& stats = m_sceneStats[i];
        m_smoothedSceneStats.frameRate += stats.frameRate;
        m_smoothedSceneStats.gpuFrameRate += stats.gpuFrameRate;
    }

    m_smoothedSceneStats.frameRate /= osm_historyLength;
    m_smoothedSceneStats.gpuFrameRate /= osm_historyLength;
}

float COverloadSceneManager::CalcFBScale()
{
    float curTime = gEnv->pTimer->GetCurrTime();

    // first calculate delta of the override lerp
    float delta = clamp_tpl((curTime - m_lerpOverride.m_start) / m_lerpOverride.m_length, 0.0f, 1.0f);
    if (m_lerpOverride.m_reversed)
    {
        delta = 1.0f - delta;
    }

    // get the current target override
    float curOverrideScale = LERP(m_fbOverrideCurScale, m_fbOverrideDestScale, delta);

    // calculate the delta from (or two) automatic scaling
    delta = clamp_tpl((curTime - m_lerpAuto.m_start) / m_lerpAuto.m_length, 0.0f, 1.0f);
    if (m_lerpAuto.m_reversed)
    {
        delta = 1.0f - delta;
    }

    // final lerp from automatic to current override
    return LERP(m_fbAutoScale, curOverrideScale, delta);
}

void COverloadSceneManager::ResizeFB()
{
    // don't do anything for invalid framerates
    if (m_smoothedSceneStats.gpuFrameRate < 5.0f || m_smoothedSceneStats.gpuFrameRate > 100.0f)
    {
        return;
    }

    float fpsDiff = fabsf(m_smoothedSceneStats.gpuFrameRate - osm_targetFPS);

    if (m_smoothedSceneStats.gpuFrameRate < osm_targetFPS - osm_targetFPSTolerance)
    {
        m_fbAutoScale -= osm_fbScaleDeltaDown / 1000.0f * fpsDiff;
    }
    else if (m_smoothedSceneStats.gpuFrameRate > osm_targetFPS + osm_targetFPSTolerance)
    {
        m_fbAutoScale += osm_fbScaleDeltaUp / 1000.0f * fpsDiff;
    }

    m_fbAutoScale = clamp_tpl(m_fbAutoScale, osm_fbMinScale, 1.0f);

    m_fbScale = clamp_tpl(CalcFBScale(), osm_fbMinScale, 1.0f);

    gEnv->pRenderer->SetViewportDownscale(m_fbScale, m_fbScale);
}
