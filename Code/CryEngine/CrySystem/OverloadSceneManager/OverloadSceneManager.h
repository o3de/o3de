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

#ifndef CRYINCLUDE_CRYSYSTEM_OVERLOADSCENEMANAGER_OVERLOADSCENEMANAGER_H
#define CRYINCLUDE_CRYSYSTEM_OVERLOADSCENEMANAGER_OVERLOADSCENEMANAGER_H
#pragma once


// Includes
#include "IOverloadSceneManager.h"

#define SCENE_PERFORMANCE_FRAME_HISTORY 64

//==================================================================================================
// Name: SOverloadSceneStats
// Desc: Overload scene stats
// Author: James Chilvers
//==================================================================================================
struct SScenePerformanceStats
{
    SScenePerformanceStats()
    {
        Reset();
    }

    void Reset()
    {
        frameRate = 0.0f;
        gpuFrameRate = 0.0f;
    }

    float   frameRate;
    float   gpuFrameRate;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: COverloadSceneManager
// Desc: Manages overload values (eg CPU,GPU etc)
//           1.0="everything is ok"  0.0="very bad frame rate"
//           various systems can use this information and control what is currently in the scene
// Author: James Chilvers
//==================================================================================================
class COverloadSceneManager
    : public IOverloadSceneManager
{
    friend class COverloadDG;

public:

    COverloadSceneManager();
    ~COverloadSceneManager() override = default;

    virtual void  Reset();

    virtual void Update();

    virtual void OverrideScale(float frameScale, float dt);
    virtual void ResetScale(float dt);

private:

    void  ResetDefaultValues();
    void    InitialiseCVars();
    void    UpdateStats();
    void    CalculateSmoothedStats();
    void    ResizeFB();

    float CalcFBScale();     // performs all lerping and returns final framebuffer scale

    // cvars
    int                                             osm_enabled;
    int                                             osm_historyLength;
    float                                           osm_targetFPS;
    float                                           osm_targetFPSTolerance;
    float                                           osm_fbScaleDeltaUp, osm_fbScaleDeltaDown;
    float                     osm_fbMinScale;

    SScenePerformanceStats      m_smoothedSceneStats;
    SScenePerformanceStats      m_sceneStats[SCENE_PERFORMANCE_FRAME_HISTORY];
    int                                             m_currentFrameStat;

    // current output scale, set to the renderer
    float                                           m_fbScale;

    // Lerping behaviour is to lerp from autoscale to (lerp between cur/dest override)
    //
    //                                        m_fbOverrideCurScale
    //                                                |
    //                                                |
    //    m_fbAutoScale  ---- m_lerpAuto --->  m_lerpOverride
    //                                                |
    //                                                v
    //                                        m_fbOverrideDestScale

    // framebuffer scales to support lerping & overriding of calculated scale
    float                                           m_fbAutoScale, m_fbOverrideCurScale, m_fbOverrideDestScale;

    struct ScaleLerp
    {
        bool m_reversed;  // Normally lerp is 0 -> 1. If reversed it's 1 -> 0
        float m_start;    // time in seconds
        float m_length;   // time in seconds
    };

    // lerpAuto is the lerp between auto scale and whatever override is.
    // lerpOverride is the lerp between m_fbOverrideCurScale and m_fbOverrideDestScale
    ScaleLerp                 m_lerpAuto, m_lerpOverride;

    // State is the current destination of any lerps.
    enum ScaleState
    {
        FBSCALE_AUTO,
        FBSCALE_OVERRIDE,
    } m_scaleState;
};//------------------------------------------------------------------------------------------------

#endif // CRYINCLUDE_CRYSYSTEM_OVERLOADSCENEMANAGER_OVERLOADSCENEMANAGER_H
