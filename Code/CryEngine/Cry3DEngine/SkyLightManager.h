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

#ifndef CRYINCLUDE_CRY3DENGINE_SKYLIGHTMANAGER_H
#define CRYINCLUDE_CRY3DENGINE_SKYLIGHTMANAGER_H
#pragma once


#include <memory>
#include <vector>


class CSkyLightNishita;
struct ITimer;


class CSkyLightManager
    : public Cry3DEngineBase
{
public:
    struct SSkyDomeCondition
    {
        SSkyDomeCondition()
            : m_sunIntensity(20.0f, 20.0f, 20.0f)
            , m_Km(0.001f)
            , m_Kr(0.00025f)
            , m_g(-0.99f)
            , m_rgbWaveLengths(650.0f, 570.0f, 475.0f)
            , m_sunDirection(0.0f, 0.707106f, 0.707106f)
        {
        }

        Vec3 m_sunIntensity;
        float m_Km;
        float m_Kr;
        float m_g;
        Vec3 m_rgbWaveLengths;
        Vec3 m_sunDirection;
    } _ALIGN(16);

public:
    CSkyLightManager();
    ~CSkyLightManager();

    // sky dome condition
    void SetSkyDomeCondition(const SSkyDomeCondition& skyDomeCondition);
    void GetCurSkyDomeCondition(SSkyDomeCondition& skyCond) const;

    // controls updates
    void FullUpdate();
    void IncrementalUpdate(f32 updateRatioPerFrame, const SRenderingPassInfo& passInfo);
    void SetQuality(int32 quality);

    // rendering params
    const SSkyLightRenderParams* GetRenderParams() const;

    void GetMemoryUsage(ICrySizer* pSizer) const;

    void InitSkyDomeMesh();
    void ReleaseSkyDomeMesh();


    void UpdateInternal(int32 newFrameID, int32 numUpdates, int callerIsFullUpdate = 0);
private:
    typedef std::vector<CryHalf4> SkyDomeTextureData;

private:
    bool IsSkyDomeUpdateFinished() const;

    int GetFrontBuffer() const;
    int GetBackBuffer() const;
    void ToggleBuffer();
public:
    void UpdateRenderParams();
private:
    void PushUpdateParams();

private:
    SSkyDomeCondition m_curSkyDomeCondition;            //current sky dome conditions
    SSkyDomeCondition m_reqSkyDomeCondition[2];     //requested sky dome conditions, double buffered(engine writes async)
    SSkyDomeCondition m_updatingSkyDomeCondition; //sky dome conditions the update is currently processed with
    int m_updateRequested[2];                                       //true if an update is requested, double buffered(engine writes async)
    CSkyLightNishita* m_pSkyLightNishita;

    SkyDomeTextureData m_skyDomeTextureDataMie[ 2 ];
    SkyDomeTextureData m_skyDomeTextureDataRayleigh[ 2 ];
    int32 m_skyDomeTextureTimeStamp[ 2 ];

    bool m_bFlushFullUpdate;

    _smart_ptr<IRenderMesh> m_pSkyDomeMesh;

    int32 m_numSkyDomeColorsComputed;
    int32 m_curBackBuffer;

    int32 m_lastFrameID;
    int32 m_needRenderParamUpdate;

    Vec3 m_curSkyHemiColor[5];
    Vec3 m_curHazeColor;
    Vec3 m_curHazeColorMieNoPremul;
    Vec3 m_curHazeColorRayleighNoPremul;

    Vec3 m_skyHemiColorAccum[5];
    Vec3 m_hazeColorAccum;
    Vec3 m_hazeColorMieNoPremulAccum;
    Vec3 m_hazeColorRayleighNoPremulAccum;

    SSkyLightRenderParams m_renderParams _ALIGN(16);
} _ALIGN(128);


#endif // CRYINCLUDE_CRY3DENGINE_SKYLIGHTMANAGER_H
