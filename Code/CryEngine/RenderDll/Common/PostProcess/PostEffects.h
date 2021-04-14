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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_POSTPROCESS_POSTEFFECTS_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_POSTPROCESS_POSTEFFECTS_H
#pragma once

#include "PostProcessUtils.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>

struct MotionBlurObjectParameters
{
    MotionBlurObjectParameters()
        : m_renderObject{}
        , m_updateFrameId{}
    {}

    MotionBlurObjectParameters(CRenderObject* renderObject, const Matrix34A& worldMatrix, AZ::u32 updateFrameId)
        : m_worldMatrix(worldMatrix)
        , m_renderObject(renderObject)
        , m_updateFrameId(updateFrameId) {}

    Matrix34 m_worldMatrix;
    CRenderObject* m_renderObject;
    AZ::u32 m_updateFrameId;
};

//////////////////////////////////////////////////////////////////////////

class CMotionBlur
    : public CPostEffect
{
public:
    CMotionBlur()
    {
        m_nRenderFlags = 0;
        m_nID = ePFX_eMotionBlur;

        // Register technique instance and it's parameters
        AddParamBool("MotionBlur_Active", m_pActive, 0);
        AddParamFloatNoTransition("FilterRadialBlurring_Amount", m_pRadBlurAmount, 0.0f);
        AddParamFloatNoTransition("FilterRadialBlurring_ScreenPosX", m_pRadBlurScreenPosX, 0.5f);
        AddParamFloatNoTransition("FilterRadialBlurring_ScreenPosY", m_pRadBlurScreenPosY, 0.5f);
        AddParamFloatNoTransition("FilterRadialBlurring_Radius", m_pRadBlurRadius, 1.0f);
        AddParamVec4("Global_DirectionalBlur_Vec", m_pDirectionalBlurVec, Vec4(0, 0, 0, 0));

        for (int idx = 0; idx < AZ_ARRAY_SIZE(m_Objects); ++idx)
        {
            m_Objects[idx] = AZStd::make_unique<ObjectMap>();
        }
    }

    virtual ~CMotionBlur()
    {
        Release();
    }

    virtual void Render();
    virtual bool Preprocess();

    virtual void Release()
    {
        m_Objects[0]->clear();
        m_Objects[1]->clear();
        m_Objects[2]->clear();
    }

    virtual void Reset([[maybe_unused]] bool bOnSpecChange = false)
    {
        m_pDirectionalBlurVec->ResetParamVec4(Vec4(0, 0, 0, 0));
    }

    void RenderObjectsVelocity();
    void CopyAndScaleDoFBuffer(CTexture* pSrcTex, CTexture* pDestTex);
    void OnBeginFrame();
    float ComputeMotionScale();

    static void SetupObject(CRenderObject* pObj, const SRenderingPassInfo& passInfo);
    static void GetPrevObjToWorldMat(CRenderObject* pObj, Matrix44A& res);
    static void InsertNewElements();
    static void FreeData();

    static const Matrix44A &GetPrevView()
    {
        return gRenDev->GetPreviousFrameMatrixSet().m_ViewMatrix;
    }

    virtual const char* GetName() const
    {
        return "MotionBlur";
    }

private:
    friend class CMotionBlurPass;

    CEffectParam* m_pRadBlurAmount, * m_pRadBlurScreenPosX, * m_pRadBlurScreenPosY, * m_pRadBlurRadius;
    CEffectParam* m_pDirectionalBlurVec;

    //! m_Objects contains motion blur parameters and is triple buffered
    //! t0: being written, t-1: current render frame, t-2: previous render frame
    static const uint32_t s_maxObjectBuffers = 3;
    typedef VectorMap<uintptr_t, MotionBlurObjectParameters > ObjectMap;
    static AZStd::unique_ptr<ObjectMap> m_Objects[s_maxObjectBuffers]; 

    //! Thread safe double buffered fill data used to populate the m_Objects buffer
    static TSRC_ALIGN CThreadSafeRendererContainer<ObjectMap::value_type> m_FillData[RT_COMMAND_BUF_COUNT];

    //! The threshold in frames at which we want to discard per-object motion data
    static const uint32_t s_discardThreshold = 60;
};

struct DepthOfFieldParameters
{
    DepthOfFieldParameters()
        : m_bEnabled{false}
    {}

    Vec4 m_FocusParams0;
    Vec4 m_FocusParams1;
    bool m_bEnabled;
};

//////////////////////////////////////////////////////////////////////////
// Deprecated: This class is used as a placeholder for parameters, but the 
// rendering logic now lives in DepthOfFieldPass.
//
class CDepthOfField
    : public CPostEffect
{
public:
    CDepthOfField()
    {
        m_nRenderFlags = 0;
        m_nID = ePFX_eDepthOfField;
        AddParamBool("Dof_Active", m_pActive, 0);
        AddParamFloatNoTransition("Dof_FocusDistance", m_pFocusDistance, 3.5f);
        AddParamFloatNoTransition("Dof_FocusRange", m_pFocusRange, 0.0f);
        AddParamFloatNoTransition("Dof_FocusMin", m_pFocusMin, 2.0f);
        AddParamFloatNoTransition("Dof_FocusMax", m_pFocusMax, 10.0f);
        AddParamFloatNoTransition("Dof_FocusLimit", m_pFocusLimit, 100.0f);
        AddParamFloatNoTransition("Dof_CenterWeight", m_pCenterWeight, 1.0f);
        AddParamFloatNoTransition("Dof_BlurAmount", m_pBlurAmount, 1.0f);
        AddParamBool("Dof_User_Active", m_pUserActive, 0);
        AddParamFloatNoTransition("Dof_User_FocusDistance", m_pUserFocusDistance, 3.5f);
        AddParamFloatNoTransition("Dof_User_FocusRange", m_pUserFocusRange, 5.0f);
        AddParamFloatNoTransition("Dof_User_BlurAmount", m_pUserBlurAmount, 1.0f);
        AddParamFloatNoTransition("Dof_Tod_FocusRange", m_pTimeOfDayFocusRange, 1000.0f);
        AddParamFloatNoTransition("Dof_Tod_BlurAmount", m_pTimeOfDayBlurAmount, 0.0f);
        AddParamFloatNoTransition("Dof_FocusMinZ", m_pFocusMinZ, 0.0f); // 0.4 is good default
        AddParamFloatNoTransition("Dof_FocusMinZScale", m_pFocusMinZScale, 0.0f); // 1.0 is good default

        m_fUserFocusRangeCurr = 0;
        m_fUserFocusDistanceCurr = 0;
        m_fUserBlurAmountCurr = 0;
        m_todFocusRange = 0.0f;
        m_todBlurAmount = 0.0f;
    }

    virtual void Reset([[maybe_unused]] bool bOnSpecChange = false)
    {
        m_pFocusDistance->ResetParam(3.5f);
        m_pFocusRange->ResetParam(0.0f);
        m_pCenterWeight->ResetParam(1.0f);
        m_pBlurAmount->ResetParam(1.0f);
        m_pFocusMin->ResetParam(2.0f);
        m_pFocusMax->ResetParam(10.0f);
        m_pFocusLimit->ResetParam(100.0f);
        m_pActive->ResetParam(0.0f);
        m_pUserActive->ResetParam(0.0f);
        m_pUserFocusDistance->ResetParam(3.5f);
        m_pUserFocusRange->ResetParam(5.0f);
        m_pUserBlurAmount->ResetParam(1.0f);
        m_pFocusMinZ->ResetParam(0.0f);
        m_pFocusMinZScale->ResetParam(0.0f);

        m_fUserFocusRangeCurr = 0;
        m_fUserFocusDistanceCurr = 0;
        m_fUserBlurAmountCurr = 0;
        m_todFocusRange = 0;
        m_todBlurAmount = 0;
    }

    virtual int CreateResources() { return true; }
    virtual void Release() {}
    virtual void Render() {}
    virtual bool Preprocess() { return false; }
    virtual const char* GetName() const { return "DepthOfField"; }

    void UpdateParameters();

    const DepthOfFieldParameters& GetParameters() const
    {
        return m_Parameters;
    }

private:
    CEffectParam* m_pFocusDistance, * m_pFocusRange, * m_pCenterWeight, * m_pBlurAmount;
    CEffectParam* m_pFocusMin, * m_pFocusMax;
    CEffectParam* m_pUserActive, * m_pUserFocusDistance, * m_pUserFocusRange, * m_pUserBlurAmount;
    CEffectParam* m_pTimeOfDayFocusRange, * m_pTimeOfDayBlurAmount;
    CEffectParam* m_pFocusMinZ, * m_pFocusMinZScale, *m_pFocusLimit;

    DepthOfFieldParameters m_Parameters;

    float m_fUserFocusRangeCurr;
    float m_fUserFocusDistanceCurr;
    float m_fUserBlurAmountCurr;
    float m_todFocusRange;
    float m_todBlurAmount;
};

class CPostAA
    : public CPostEffect
{
public:
    CPostAA()
    {
        m_nRenderFlags = PSP_UPDATE_SCENE_SPECULAR;
        m_nID = ePFX_PostAA;
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset([[maybe_unused]] bool bOnSpecChange = false) {}

    virtual const char* GetName() const
    {
        return "PostAA";
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CSunShafts
    : public CPostEffect
{
public:
    CSunShafts()
    {
        m_nID = ePFX_SunShafts;
        m_pOcclQuery = 0;

        AddParamBool("SunShafts_Active", m_pActive, 0);
        AddParamInt("SunShafts_Type", m_pShaftsType, 0); // default shafts type - highest quality
        AddParamFloatNoTransition("SunShafts_Amount", m_pShaftsAmount, 0.25f); // shafts visibility
        AddParamFloatNoTransition("SunShafts_RaysAmount", m_pRaysAmount, 0.25f); // rays visibility
        AddParamFloatNoTransition("SunShafts_RaysAttenuation", m_pRaysAttenuation, 5.0f); // rays attenuation
        AddParamFloatNoTransition("SunShafts_RaysSunColInfluence", m_pRaysSunColInfluence, 1.0f); // sun color influence
        AddParamVec4NoTransition("SunShafts_RaysCustomColor", m_pRaysCustomCol, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        AddParamFloat("Scratches_Strength", m_pScratchStrength, 0.0f);
        AddParamFloat("Scratches_Threshold", m_pScratchThreshold, 0.0f);
        AddParamFloat("Scratches_Intensity", m_pScratchIntensity, 0.7f);
        m_bShaftsEnabled = false;
        m_nVisSampleCount = 0;
    }


    virtual int Initialize();
    virtual void Release();
    virtual void OnLostDevice();
    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    bool IsVisible();
    bool SunShaftsGen(CTexture* pSunShafts, CTexture* pPingPongRT = 0);
    bool MergedSceneDownsampleAndSunShaftsMaskGen(CTexture* pSceneSrc, CTexture* pSceneDst, CTexture* pSunShaftsMaskDst);
    void GetSunShaftsParams(Vec4 pParams[2])
    {
        pParams[0] = m_pRaysCustomCol->GetParamVec4();
        pParams[1] = Vec4(0, 0, m_pRaysAmount->GetParam(), m_pRaysSunColInfluence->GetParam());
    }

    virtual const char* GetName() const
    {
        return "MergedSunShaftsEdgeAAColorCorrection";
    }

private:

    bool m_bShaftsEnabled;
    uint32 m_nVisSampleCount;

    // int, float, float, float, vec4
    CEffectParam* m_pShaftsType, * m_pShaftsAmount, * m_pRaysAmount, * m_pRaysAttenuation, * m_pRaysSunColInfluence, * m_pRaysCustomCol;
    CEffectParam* m_pScratchStrength, * m_pScratchThreshold, * m_pScratchIntensity;
    COcclusionQuery* m_pOcclQuery;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFilterSharpening
    : public CPostEffect
{
public:
    CFilterSharpening()
    {
        m_nID = ePFX_FilterSharpening;

        AddParamInt("FilterSharpening_Type", m_pType, 0);
        AddParamFloat("FilterSharpening_Amount", m_pAmount, 1.0f);
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "FilterSharpening";
    }

private:

    // float, int
    CEffectParam* m_pAmount, * m_pType;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFilterBlurring
    : public CPostEffect
{
public:
    CFilterBlurring()
    {
        m_nID = ePFX_FilterBlurring;

        AddParamInt("FilterBlurring_Type", m_pType, 0);
        AddParamFloat("FilterBlurring_Amount", m_pAmount, 0.0f);
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "FilterBlurring";
    }

private:

    // float, int
    CEffectParam* m_pAmount, * m_pType;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CUberGamePostProcess
    : public CPostEffect
{
public:

    // Bitmaks used to enable only certain effects or combinations of most expensive effects
    enum EPostsProcessMask
    {
        ePE_SyncArtifacts = (1 << 0),
        ePE_RadialBlur = (1 << 1),
        ePE_ChromaShift = (1 << 2),
    };

    CUberGamePostProcess()
    {
        m_nID = ePFX_UberGamePostProcess;
        m_nCurrPostEffectsMask = 0;

        AddParamTex("tex_VisualArtifacts_Mask", m_pMask, 0);

        AddParamVec4("clr_VisualArtifacts_ColorTint", m_pColorTint, Vec4(1.0f, 1.0f, 1.0f, 1.0f));

        AddParamFloat("VisualArtifacts_Vsync", m_pVSyncAmount, 0.0f);
        AddParamFloat("VisualArtifacts_VsyncFreq", m_pVSyncFreq, 1.0f);

        AddParamFloat("VisualArtifacts_Interlacing", m_pInterlationAmount, 0.0f);
        AddParamFloat("VisualArtifacts_InterlacingTile", m_pInterlationTiling, 1.0f);
        AddParamFloat("VisualArtifacts_InterlacingRot", m_pInterlationRotation, 0.0f);

        AddParamFloat("VisualArtifacts_Pixelation", m_pPixelationScale, 0.0f);
        AddParamFloat("VisualArtifacts_Noise", m_pNoise, 0.0f);

        AddParamFloat("VisualArtifacts_SyncWaveFreq", m_pSyncWaveFreq, 0.0f);
        AddParamFloat("VisualArtifacts_SyncWavePhase", m_pSyncWavePhase, 0.0f);
        AddParamFloat("VisualArtifacts_SyncWaveAmplitude", m_pSyncWaveAmplitude, 0.0f);

        AddParamFloat("FilterChromaShift_User_Amount", m_pFilterChromaShiftAmount, 0.0f); // Kept for backward - compatibility
        AddParamFloat("FilterArtifacts_ChromaShift", m_pChromaShiftAmount, 0.0f);

        AddParamFloat("FilterGrain_Amount", m_pFilterGrainAmount, 0.0f);// Kept for backward - compatibility
        AddParamFloat("FilterArtifacts_Grain", m_pGrainAmount, 0.0f);
        AddParamFloatNoTransition("FilterArtifacts_GrainTile", m_pGrainTile, 1.0f);
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "UberGamePostProcess";
    }

private:
    CEffectParam* m_pVSyncAmount;
    CEffectParam* m_pVSyncFreq;

    CEffectParam* m_pColorTint;

    CEffectParam* m_pInterlationAmount;
    CEffectParam* m_pInterlationTiling;
    CEffectParam* m_pInterlationRotation;

    CEffectParam* m_pPixelationScale;
    CEffectParam* m_pNoise;

    CEffectParam* m_pSyncWaveFreq;
    CEffectParam* m_pSyncWavePhase;
    CEffectParam* m_pSyncWaveAmplitude;

    CEffectParam* m_pFilterChromaShiftAmount;
    CEffectParam* m_pChromaShiftAmount;

    CEffectParam* m_pGrainAmount;
    CEffectParam* m_pFilterGrainAmount; // todo: add support
    CEffectParam* m_pGrainTile;

    CEffectParam* m_pMask;

    uint8 m_nCurrPostEffectsMask;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

struct SColorGradingMergeParams;

class CColorGrading
    : public CPostEffect
{
public:
    CColorGrading()
    {
        m_nID = ePFX_ColorGrading;

        // levels adjustment
        AddParamFloatNoTransition("ColorGrading_minInput", m_pMinInput, 0.0f);
        AddParamFloatNoTransition("ColorGrading_gammaInput", m_pGammaInput, 1.0f);
        AddParamFloatNoTransition("ColorGrading_maxInput", m_pMaxInput, 255.0f);
        AddParamFloatNoTransition("ColorGrading_minOutput", m_pMinOutput, 0.0f);
        AddParamFloatNoTransition("ColorGrading_maxOutput", m_pMaxOutput, 255.0f);

        // generic color adjustment
        AddParamFloatNoTransition("ColorGrading_Brightness", m_pBrightness, 1.0f);
        AddParamFloatNoTransition("ColorGrading_Contrast", m_pContrast, 1.0f);
        AddParamFloatNoTransition("ColorGrading_Saturation", m_pSaturation, 1.0f);

        // filter color
        m_pDefaultPhotoFilterColor = Vec4(0.952f, 0.517f, 0.09f, 1.0f);
        AddParamVec4NoTransition("clr_ColorGrading_PhotoFilterColor", m_pPhotoFilterColor,  m_pDefaultPhotoFilterColor);  // use photoshop default orange
        AddParamFloatNoTransition("ColorGrading_PhotoFilterColorDensity", m_pPhotoFilterColorDensity, 0.0f);

        // selective color
        AddParamVec4NoTransition("clr_ColorGrading_SelectiveColor", m_pSelectiveColor, Vec4(0.0f, 0.0f, 0.0f, 0.0f)),
        AddParamFloatNoTransition("ColorGrading_SelectiveColorCyans", m_pSelectiveColorCyans, 0.0f),
        AddParamFloatNoTransition("ColorGrading_SelectiveColorMagentas", m_pSelectiveColorMagentas, 0.0f),
        AddParamFloatNoTransition("ColorGrading_SelectiveColorYellows", m_pSelectiveColorYellows, 0.0f),
        AddParamFloatNoTransition("ColorGrading_SelectiveColorBlacks", m_pSelectiveColorBlacks, 0.0f),

        // mist adjustment
        AddParamFloatNoTransition("ColorGrading_GrainAmount", m_pGrainAmount, 0.0f);
        AddParamFloatNoTransition("ColorGrading_SharpenAmount", m_pSharpenAmount, 1.0f);

        // user params
        AddParamFloatNoTransition("ColorGrading_Saturation_Offset", m_pSaturationOffset, 0.0f);
        AddParamVec4NoTransition("ColorGrading_PhotoFilterColor_Offset", m_pPhotoFilterColorOffset, Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        AddParamFloatNoTransition("ColorGrading_PhotoFilterColorDensity_Offset", m_pPhotoFilterColorDensityOffset, 0.0f);
        AddParamFloatNoTransition("ColorGrading_GrainAmount_Offset", m_pGrainAmountOffset, 0.0f);
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);
    bool UpdateParams(SColorGradingMergeParams& pMergeParams);

    virtual const char* GetName() const
    {
        return "ColorGrading";
    }

private:

    // levels adjustment
    CEffectParam* m_pMinInput;
    CEffectParam* m_pGammaInput;
    CEffectParam* m_pMaxInput;
    CEffectParam* m_pMinOutput;
    CEffectParam* m_pMaxOutput;

    // generic color adjustment
    CEffectParam* m_pBrightness;
    CEffectParam* m_pContrast;
    CEffectParam* m_pSaturation;
    CEffectParam* m_pSaturationOffset;

    // filter color
    CEffectParam* m_pPhotoFilterColor;
    CEffectParam* m_pPhotoFilterColorDensity;
    CEffectParam* m_pPhotoFilterColorOffset;
    CEffectParam* m_pPhotoFilterColorDensityOffset;
    Vec4 m_pDefaultPhotoFilterColor;

    // selective color
    CEffectParam* m_pSelectiveColor;
    CEffectParam* m_pSelectiveColorCyans;
    CEffectParam* m_pSelectiveColorMagentas;
    CEffectParam* m_pSelectiveColorYellows;
    CEffectParam* m_pSelectiveColorBlacks;

    // misc adjustments
    CEffectParam* m_pGrainAmount;
    CEffectParam* m_pGrainAmountOffset;

    CEffectParam* m_pSharpenAmount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CUnderwaterGodRays
    : public CPostEffect
{
public:
    CUnderwaterGodRays()
    {
        m_nID = ePFX_eUnderwaterGodRays;

        AddParamFloat("UnderwaterGodRays_Amount", m_pAmount, 1.0f);
        AddParamInt("UnderwaterGodRays_Quality", m_pQuality, 1); // 0 = low, 1 = med, 2= high, 3= ultra-high, 4= crazy high, and so on
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "UnderwaterGodRays";
    }

private:

    // float, int
    CEffectParam* m_pAmount, * m_pQuality;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CVolumetricScattering
    : public CPostEffect
{
public:
    CVolumetricScattering()
    {
        m_nID = ePFX_eVolumetricScattering;

        AddParamFloat("VolumetricScattering_Amount", m_pAmount, 0.0f);
        AddParamFloat("VolumetricScattering_Tilling", m_pTiling, 1.0f);
        AddParamFloat("VolumetricScattering_Speed", m_pSpeed, 1.0f);
        AddParamVec4("clr_VolumetricScattering_Color", m_pColor, Vec4(0.5f, 0.75f, 1.0f, 1.0f));

        AddParamInt("VolumetricScattering_Type", m_pType, 0); // 0 = alien environment, 1 = ?, 2 = ?? ???
        AddParamInt("VolumetricScattering_Quality", m_pQuality, 1); // 0 = low, 1 = med, 2= high, 3= ultra-high, 4= crazy high, and so on
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "VolumetricScattering";
    }

private:

    // float, int, int
    CEffectParam* m_pAmount, * m_pTiling, * m_pSpeed, * m_pColor, * m_pType, * m_pQuality;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Game/Hud specific post-effects
////////////////////////////////////////////////////////////////////////////////////////////////////

class CAlienInterference
    : public CPostEffect
{
public:
    CAlienInterference()
    {
        m_nID = ePFX_eAlienInterference;

        AddParamFloat("AlienInterference_Amount", m_pAmount, 0);
        AddParamVec4NoTransition("clr_AlienInterference_Color", m_pTintColor, Vec4(Vec3(0.85f, 0.95f, 1.25f) * 0.5f, 1.0f));
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "AlienInterference";
    }

private:

    // float
    CEffectParam* m_pAmount;
    // vec4
    CEffectParam* m_pTintColor;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
class CGhostVision
    : public CPostEffect
{
public:
    CGhostVision()
    {
        m_nID = ePFX_eGhostVision;

        m_pUserTex1 = 0;
        m_pUserTex2 = 0;

        AddParamBool("GhostVision_Bool1", m_pUserBool1, 0);
        AddParamBool("GhostVision_Bool2", m_pUserBool2, 0);
        AddParamBool("GhostVision_Bool3", m_pUserBool3, 0);
        AddParamFloat("GhostVision_Amount1", m_pUserValue1, 0);
        AddParamFloat("GhostVision_Amount2", m_pUserValue2, 0);
        AddParamFloat("GhostVision_Amount3", m_pUserValue3, 0);
        AddParamVec4NoTransition("clr_GhostVision_Color", m_pTintColor, Vec4(Vec3(0.55f, 0.55f, 0.55f) * 0.5f, 1.0f));
    }

    virtual int CreateResources();
    virtual bool Preprocess();
    virtual void Render();
    virtual void Release();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "GhostVision";
    }

private:
    // texture
    CTexture* m_pUserTex1;
    CTexture* m_pUserTex2;

    // bool
    CEffectParam* m_pUserBool1;
    CEffectParam* m_pUserBool2;
    CEffectParam* m_pUserBool3;

    // float
    CEffectParam* m_pUserValue1;
    CEffectParam* m_pUserValue2;
    CEffectParam* m_pUserValue3;

    // vec4
    CEffectParam* m_pTintColor;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
class CWaterDroplets
    : public CPostEffect
{
public:
    CWaterDroplets()
    {
        m_nID = ePFX_eWaterDroplets;

        AddParamFloat("WaterDroplets_Amount", m_pAmount, 0.0f);
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "WaterDroplets";
    }

private:

    // float
    CEffectParam* m_pAmount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CWaterFlow
    : public CPostEffect
{
public:
    CWaterFlow()
    {
        m_nID = ePFX_eWaterFlow;

        AddParamFloat("WaterFlow_Amount", m_pAmount, 0.0f);
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "WaterFlow";
    }

private:

    // float
    CEffectParam* m_pAmount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CWaterRipples
    : public CPostEffect
{
    struct SWaterHit
    {
        SWaterHit()
            : worldPos(0.0f, 0.0f)
            , scale(1.0f)
            , strength(1.0f)
        {
        }

        SWaterHit(const Vec3& hitWorldPos, const float hitScale, const float hitStrength)
            : worldPos(hitWorldPos.x, hitWorldPos.y)
            , scale(hitScale)
            , strength(hitStrength)
        {
        }

        Vec2  worldPos;
        float scale;
        float strength;
    };

public:
    CWaterRipples()
        : m_bSnapToCenter(false)
        , m_bInitializeSim(true)
    {
        m_nRenderFlags = 0;
        m_nID = ePFX_WaterRipples;

        AddParamFloatNoTransition("WaterRipples_Amount", m_pAmount, 0.0f);

        m_pRipplesGenTechName = "WaterRipplesGen";
        m_pRipplesHitTechName = "WaterRipplesHit";
        m_pRipplesParamName = "WaterRipplesParams";

        m_fLastSpawnTime = 0.0f;
        m_fSimGridSize = 25.0f;
        m_fSimGridSnapRange = 5.0f;

        for (uint32 i = 0; i < RT_COMMAND_BUF_COUNT; i++)
        {
            s_pWaterHits[i].reserve(16);
        }

        s_nUpdateMask = 0;
    }

    static void CreatePhysCallbacks();
    static void ReleasePhysCallbacks();
    virtual bool Preprocess();

    // Enabled/Disabled if no hits on list to process - call from render thread
    bool RT_SimulationStatus();

    // Add hits to list - called from main thread
    static void AddHit(const Vec3& vPos, const float scale, const float strength);

    virtual void Release()
    {
        Reset();
    }

    void RenderHits();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "WaterRipples";
    }

    Vec4 GetLookupParams() { return s_vLookupParams; }

    void DEBUG_DrawWaterHits();

private:

    static int OnEventPhysCollision(const struct EventPhys* pEvent);

private:

    enum
    {
        MAX_HITS = 128
    };

    struct SWaterHitRecord
    {
        SWaterHit mHit;
        float fHeight;
        int nCounter;
    };

    CCryNameTSCRC m_pRipplesGenTechName;
    CCryNameTSCRC m_pRipplesHitTechName;
    CCryNameR m_pRipplesParamName;

    // float
    CEffectParam* m_pAmount;
    float m_fLastSpawnTime;
    float m_fLastUpdateTime;

    float m_fSimGridSize;
    float m_fSimGridSnapRange;

    static AZStd::vector< CWaterRipples::SWaterHit, AZ::StdLegacyAllocator > s_pWaterHits[RT_COMMAND_BUF_COUNT];
    static AZStd::vector< CWaterRipples::SWaterHit, AZ::StdLegacyAllocator > s_pWaterHitsMGPU;
    static AZStd::vector< CWaterRipples::SWaterHitRecord, AZ::StdLegacyAllocator > m_DebugWaterHits;
    static Vec3 s_CameraPos;
    static Vec2 s_SimOrigin;

    static int s_nUpdateMask;
    static Vec4 s_vParams;
    static Vec4 s_vLookupParams;

    static bool s_bInitializeSim;
    bool m_bSnapToCenter;
    bool m_bInitializeSim;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CWaterVolume
    : public CPostEffect
{
public:
    CWaterVolume()
    {
        m_nRenderFlags = 0;
        m_nID = ePFX_WaterVolume;

        AddParamFloatNoTransition("WaterVolume_Amount", m_pAmount, 0.0f);
        m_nCurrSimID = 0;
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);
    int GetCurrentPuddle()
    {
        return m_nCurrSimID;
    }

    virtual const char* GetName() const
    {
        return "WaterVolume";
    }

private:

    // float
    CEffectParam* m_pAmount;
    int m_nCurrSimID;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CScreenFrost
    : public CPostEffect
{
public:
    CScreenFrost()
    {
        m_nID = ePFX_eScreenFrost;

        AddParamFloat("ScreenFrost_Amount", m_pAmount, 0.0f); // amount of visible frost
        AddParamFloat("ScreenFrost_CenterAmount", m_pCenterAmount, 1.0f); // amount of visible frost in center

        m_fRandOffset = 0;
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "ScreenFrost";
    }

private:

    // float, float
    CEffectParam* m_pAmount, * m_pCenterAmount;
    float m_fRandOffset;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CRainDrops
    : public CPostEffect
{
public:
    CRainDrops()
    {
        m_nID = ePFX_eRainDrops;

        AddParamFloat("RainDrops_Amount", m_pAmount, 0.0f); // amount of visible droplets
        AddParamFloat("RainDrops_SpawnTimeDistance", m_pSpawnTimeDistance, 0.35f); // amount of visible droplets
        AddParamFloat("RainDrops_Size", m_pSize, 5.0f); // drop size
        AddParamFloat("RainDrops_SizeVariation", m_pSizeVar, 2.5f); // drop size variation

        m_uCurrentDytex = 0;
        m_pVelocityProj = Vec3(0, 0, 0);

        m_nAliveDrops = 0;

        m_pPrevView.SetIdentity();
        m_pViewProjPrev.SetIdentity();

        m_bFirstFrame = true;
    }

    virtual ~CRainDrops()
    {
        Release();
    }

    virtual int CreateResources();
    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);
    virtual void Release();

    virtual const char* GetName() const;

private:

    // Rain particle properties
    struct SRainDrop
    {
        // set default data
        SRainDrop()
            : m_pPos(0, 0, 0)
            , m_fSize(5.0f)
            , m_fSizeVar(2.5f)
            , m_fSpawnTime(0.0f)
            , m_fLifeTime(2.0f)
            , m_fLifeTimeVar(1.0f)
            , m_fWeight(1.0f)
            , m_fWeightVar(0.25f)
        {
        }

        // Screen position
        Vec3 m_pPos;
        // Size and variation (bigger also means more weight)
        float m_fSize, m_fSizeVar;
        // Spawn time
        float m_fSpawnTime;
        // Life time and variation
        float m_fLifeTime, m_fLifeTimeVar;
        // Weight and variation
        float m_fWeight, m_fWeightVar;
    };

    //in Preprocess(), check if effect is active
    bool IsActiveRain();

    // Compute current interpolated view matrix
    Matrix44 ComputeCurrentView(int iViewportWidth, int iViewportHeight);

    // Spawn a particle
    void SpawnParticle(SRainDrop*& pParticle, int iRTWidth, int iRTHeight);
    // Update all particles
    void UpdateParticles(int iRTWidth, int iRTHeight);
    // Generate rain drops map
    void RainDropsMapGen();
    // Draw the raindrops
    void DrawRaindrops(int iViewportWidth, int iViewportHeight, int iRTWidth, int iRTHeight);
    // Apply the blur and movement to it
    void ApplyExtinction(CTexture*& rptexPrevRT, int iViewportWidth, int iViewportHeight, int iRTWidth, int iRTHeight);

    // Final draw pass, merge with backbuffer
    void DrawFinal(CTexture*& rptexCurrRT);

    // float
    CEffectParam* m_pAmount;
    CEffectParam* m_pSpawnTimeDistance;
    CEffectParam* m_pSize;
    CEffectParam* m_pSizeVar;

    uint16        m_uCurrentDytex;
    bool          m_bFirstFrame;

    //todo: use generic screen particles
    typedef std::vector<SRainDrop*> SRainDropsVec;
    typedef SRainDropsVec::iterator SRainDropsItor;
    SRainDropsVec m_pDropsLst;

    Vec3 m_pVelocityProj;
    Matrix44 m_pPrevView;
    Matrix44 m_pViewProjPrev;

    int m_nAliveDrops;
    static const int m_nMaxDropsCount = 100;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CHudSilhouettes
    : public CPostEffect
{
public:
    CHudSilhouettes()
    {
        m_nRenderFlags = 0;
        m_nID = ePFX_HUDSilhouettes;

        m_deferredSilhouettesOptimisedTech = "DeferredSilhouettesOptimised";
        m_psParamName = "psParams";
        m_vsParamName = "vsParams";

        m_bSilhouettesOptimisedTechAvailable = false;
        m_pSilhouetesRT = NULL;

        AddParamBool("HudSilhouettes_Active", m_pActive, 0);
        AddParamFloatNoTransition("HudSilhouettes_Amount", m_pAmount, 1.0f); //0.0f gives funky blending result ? investigate
        AddParamFloatNoTransition("HudSilhouettes_FillStr", m_pFillStr, 0.15f);
        AddParamInt("HudSilhouettes_Type", m_pType, 1);

        FindIfSilhouettesOptimisedTechAvailable();
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "HUDSilhouettes";
    }

private:

    void    RenderDeferredSilhouettes(float fBlendParam, float fType);
    void  RenderDeferredSilhouettesOptimised(float fBlendParam, float fType);

    void    FindIfSilhouettesOptimisedTechAvailable()
    {
#ifndef NULL_RENDERER
        if (CShaderMan::s_shPostEffectsGame)
        {
            m_bSilhouettesOptimisedTechAvailable = (CShaderMan::s_shPostEffectsGame->mfFindTechnique(m_deferredSilhouettesOptimisedTech)) ? true : false;
        }
#endif
    }


    CCryNameTSCRC m_deferredSilhouettesOptimisedTech;
    CCryNameR           m_vsParamName;
    CCryNameR           m_psParamName;

    // float
    CEffectParam* m_pAmount, * m_pFillStr, * m_pType;
    CTexture* m_pSilhouetesRT;

    bool m_bSilhouettesOptimisedTechAvailable;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFlashBang
    : public CPostEffect
{
public:
    CFlashBang()
    {
        m_nID = ePFX_eFlashBang;

        AddParamBool("FlashBang_Active", m_pActive, 0);
        AddParamFloat("FlashBang_DifractionAmount", m_pDifractionAmount, 1.0f);
        AddParamFloat("FlashBang_Time", m_pTime, 2.0f); // flashbang time duration in seconds
        AddParamFloat("FlashBang_BlindAmount", m_pBlindAmount, 0.5f); // flashbang blind time (fraction of frashbang time)

        m_pGhostImage = 0;
        m_fBlindAmount = 1.0f;
        m_fSpawnTime = 0.0f;
    }

    virtual ~CFlashBang()
    {
        Release();
    }

    virtual bool Preprocess();
    virtual void Release();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "FlashBang";
    }

private:

    SDynTexture* m_pGhostImage;

    float m_fBlindAmount;
    float m_fSpawnTime;

    // float, float
    CEffectParam* m_pTime, * m_pDifractionAmount, * m_pBlindAmount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CSceneRain
    : public CPostEffect
{
public:
    CSceneRain()
    {
        m_nRenderFlags = 0;
        m_nID = ePFX_SceneRain;

        m_pConeVB = 0;
        m_nConeVBSize = 0;
        m_updateFrameCount = 0;
        m_bReinit = true;

        AddParamBool("SceneRain_Active", m_pActive, 0);
    }

    virtual int CreateResources();
    virtual void Release();
    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);
    virtual void OnLostDevice();

    virtual const char* GetName() const;

    // Rain volume parameters (filled during rain layer/occ generation pass)
    SRainParams m_RainVolParams;

private:
    bool m_bReinit;
    void* m_pConeVB;
    uint16 m_nConeVBSize;
    uint32 m_updateFrameCount;
    void CreateBuffers(uint16 nVerts, void*& pINVB, SVF_P3F_C4B_T2F* pVtxList);
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CSceneSnow
    : public CPostEffect
{
public:
    _smart_ptr<IRenderMesh> m_pSnowFlakeMesh;
    CSceneSnow()
    {
        m_nID = ePFX_SceneSnow;

        AddParamBool("SceneSnow_Active", m_pActive, 0);

        m_nAliveClusters = 0;
        m_pSnowFlakeMesh = NULL;
    }

    virtual ~CSceneSnow()
    {
        Release();
    }

    bool IsActiveSnow();

    virtual int CreateResources();
    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);
    virtual void Release();

    virtual const char* GetName() const;

    // Rain volume parameters (filled during rain layer/occ generation pass)
    // Needed for occlusion.
    SRainParams m_RainVolParams;
    SSnowParams m_SnowVolParams;

private:

    // Snow particle properties
    struct SSnowCluster
    {
        // set default data
        SSnowCluster()
            : m_pPos(0, 0, 0)
            , m_pPosPrev(0, 0, 0)
            , m_fSpawnTime(0.0f)
            , m_fLifeTime(4.0f)
            , m_fLifeTimeVar(2.5f)
            , m_fWeight(0.3f)
            , m_fWeightVar(0.1f)
        {
        }

        // World position
        Vec3 m_pPos, m_pPosPrev;
        // Spawn time
        float m_fSpawnTime;
        // Life time and variation
        float m_fLifeTime, m_fLifeTimeVar;
        // Weight and variation
        float m_fWeight, m_fWeightVar;
    };

    // Generate particle cluster mesh
    bool GenerateClusterMesh();
    // Spawn a cluster
    void SpawnCluster(SSnowCluster*& pCluster);
    // Update all clusters
    void UpdateClusters();
    // Draw clusters
    void DrawClusters();
    // Half resolution composite.
    void HalfResComposite();

    // float
    CEffectParam* m_pActive;

    typedef std::vector<SSnowCluster*> SSnowClusterVec;
    typedef SSnowClusterVec::iterator SSnowClusterItor;
    SSnowClusterVec m_pClusterList;

    int m_nSnowFlakeVertCount;

    int m_nAliveClusters;
    int m_nNumClusters;
    int m_nFlakesPerCluster;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CSoftAlphaTest
    : public CPostEffect
{
public:
    CSoftAlphaTest()
    {
        m_nID = ePFX_eSoftAlphaTest;
        m_nRenderFlags = 0;
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);
    virtual const char* GetName() const
    {
        return "SoftAlphaTest";
    }

private:
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CImageGhosting
    : public CPostEffect
{
public:
    CImageGhosting()
    {
        m_nRenderFlags = 0;
        m_nID = ePFX_ImageGhosting;
        AddParamFloat("ImageGhosting_Amount", m_pAmount, 0);
        m_bInit = true;
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);
    virtual const char* GetName() const
    {
        return "ImageGhosting";
    }

private:

    CEffectParam* m_pAmount;
    bool m_bInit;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CFilterKillCamera
    : public CPostEffect
{
public:

    CFilterKillCamera()
    {
        m_nID = ePFX_FilterKillCamera;

        AddParamBool("FilterKillCamera_Active", m_pActive, 0);
        AddParamInt("FilterKillCamera_Mode", m_pMode, 0);
        AddParamFloat("FilterKillCamera_GrainStrength", m_pGrainStrength, 0.0f);
        AddParamVec4("FilterKillCamera_ChromaShift", m_pChromaShift, Vec4(1.0f, 0.5f, 0.1f, 1.0f)); // xyz = offset, w = strength
        AddParamVec4("FilterKillCamera_Vignette", m_pVignette, Vec4(1.0f, 1.0f, 0.5f, 1.4f)); // xy = screen scale, z = radius, w = blind noise vignette scale
        AddParamVec4("FilterKillCamera_ColorScale", m_pColorScale, Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        AddParamVec4("FilterKillCamera_Blindness", m_pBlindness, Vec4(0.5f, 0.5f, 1.0f, 0.7f)); // x = blind duration, y = blind fade out duration, z = blindness grey scale, w = blind noise min scale

        m_blindTimer = 0.0f;
        m_lastMode = 0;
    }

    virtual int Initialize();
    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "FilterKillCamera";
    }

private:

    CCryNameTSCRC m_techName;
    CCryNameR m_paramName;

    CEffectParam* m_pGrainStrength;
    CEffectParam*   m_pChromaShift;
    CEffectParam* m_pVignette;
    CEffectParam* m_pColorScale;
    CEffectParam* m_pBlindness;
    CEffectParam* m_pMode;
    float                   m_blindTimer;
    int                     m_lastMode;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CScreenBlood
    : public CPostEffect
{
public:
    CScreenBlood()
    {
        m_nRenderFlags = 0;
        m_nID = ePFX_eScreenBlood;
        AddParamFloat("ScreenBlood_Amount", m_pAmount, 0.0f);  // damage amount
        AddParamVec4("ScreenBlood_Border", m_pBorder, Vec4(0.0f, 0.0f, 2.0f, 1.0f));  // Border: x=xOffset y=yOffset z=range w=alpha
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);
    virtual const char* GetName() const
    {
        return "ScreenBlood";
    }

private:

    CEffectParam* m_pAmount;
    CEffectParam* m_pBorder;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class ScreenFader
    : public CPostEffect
{
public:
    ScreenFader()
    {
        m_nID = ePFX_ScreenFader;

        AddParamBool("ScreenFader_Enable", m_enable, false);
        AddParamFloatNoTransition("ScreenFader_FadeInTime", m_fadeInTime, 0.0f);
        AddParamFloatNoTransition("ScreenFader_FadeOutTime", m_fadeOutTime, 0.0f);
        AddParamVec4NoTransition("ScreenFader_ScreenCoordinates", m_screenCoordinates, Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        AddParamVec4NoTransition("ScreenFader_FadeColor", m_fadeColor, Vec4(0.0f, 0.0f, 0.0f, 0.0f));
        AddParamTex("ScreenFader_TextureName", m_fadeTextureParam, 0);
    }

    ~ScreenFader()
    {
        // Clean up our ScenePasses
        ScreenPassList::iterator passIter = m_screenPasses.begin();
        while ( passIter != m_screenPasses.end() )
        {
            ScreenFaderPass* pass = (*passIter);
            delete pass;
            passIter = m_screenPasses.erase(passIter);
        }
    }

    bool Preprocess() override;
    virtual void Render() override;
    virtual void Reset(bool bOnSpecChange = false) override;

    const char* GetName() const override
    {
        return "ScreenFader";
    }

private:
    
    struct ScreenFaderPass
    {
        ScreenFaderPass()
        {
            m_currentColor = ColorF(0.0f, 0.0f, 0.0f, 1.0f);    
            m_screenCoordinates = Vec4(0.0f, 0.0f, 1.0f, 1.0f);
        }
        ~ScreenFaderPass()
        {
            if ( m_fadeTexture )
            {
                m_fadeTexture->Release();
            }
        }

        IPostEffectGroup* m_group = nullptr;
        CTexture* m_fadeTexture = nullptr;
        ColorF m_currentColor;

        // Specified as a 0 -> 1 percentage value for screen coordinates.  0,0,1,1 == fullscreen.
        Vec4 m_screenCoordinates;
        bool m_fadingIn = false;
        bool m_fadingOut = false;
        float m_currentFadeTime = 0.0f;
        float m_fadeInTime = 0.0f;
        float m_fadeOutTime = 0.0f;
        float m_fadeDuration = 0.0f; // Helper variable that will be set to the duration of fadeInTime or FadeOutTime
        float m_fadeDirection = 0.0f; // Multiplier -1 or +1
    };
    
    static bool SortFaderPasses(ScreenFaderPass* pass1, ScreenFaderPass* pass2);

    ScreenFader(const ScreenFader& other) = delete;
    ScreenFader(ScreenFader&&) = delete;

    // Screen fader passes will render on top of each other, rather than blend between the previous
    // screen fader passes that were set in previous PostEffectGroup files.
    // This list will be sorted based on the priority of its PostEffectGroup
    typedef AZStd::list<ScreenFaderPass*> ScreenPassList;
    ScreenPassList m_screenPasses;

    // Unlike other CPostEffect objects, we do NOT want to read the values that are auto-populated into these variables.
    // They will input into ::PreProcess with the blended values between the last two PostEffectGroup layers.
    // This system requires the individual values from each active group, not the blended values.
    // These variables are re-used as teporary variables when reading variables from the groups.
    CEffectParam* m_enable = nullptr;
    CEffectParam* m_fadeInTime = nullptr;
    CEffectParam* m_fadeOutTime = nullptr;
    CEffectParam* m_fadeColor = nullptr;
    CEffectParam* m_screenCoordinates = nullptr;
    CEffectParam* m_fadeTextureParam = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

class CPost3DRenderer
    : public CPostEffect
{
public:

    enum ERenderMeshMode
    {
        eRMM_Default = 0,
        eRMM_Custom,
        eRMM_DepthOnly
    };

    enum EPost3DRendererFlags
    {
        eP3DR_HasSilhouettes                                                = (1 << 0),
        eP3DR_DirtyFlashRT                                                  =   (1 << 1),
        eP3DR_ClearOnResolveTempRT                                  = (1 << 2),
        eP3DR_ClearOnResolveFlashRT                                 = (1 << 3),
        eP3DR_ClearOnResolvePrevBackBufferCopyRT        = (1 << 4)
    };

    CPost3DRenderer()
    {
        m_nID = ePFX_Post3DRenderer;

        AddParamBool("Post3DRenderer_Active", m_pActive, 0);
        AddParamFloat("Post3DRenderer_FOVScale", m_pFOVScale, 0.5f);
        AddParamFloat("Post3DRenderer_SilhouetteStrength", m_pSilhouetteStrength, 0.3f);
        AddParamFloat("Post3DRenderer_EdgeFadeScale", m_pEdgeFadeScale, 0.2f); // Between 0 and 1.0
        AddParamFloat("Post3DRenderer_PixelAspectRatio", m_pPixelAspectRatio, 1.0f);
        AddParamVec4("Post3DRenderer_Ambient", m_pAmbient, Vec4(0.0f, 0.0f, 0.0f, 0.2f));

        m_gammaCorrectionTechName = "Post3DRendererGammaCorrection";
        m_alphaCorrectionTechName = "Post3DRendererAlphaCorrection";
        m_texToTexTechName = "TextureToTexture";
        m_customRenderTechName = "CustomRenderPass";
        m_combineSilhouettesTechName = "Post3DRendererSilhouttes";
        m_silhouetteTechName = "BinocularView";

        m_psParamName = "psParams";
        m_vsParamName = "vsParams";

        m_nRenderFlags = 0;

        m_pFlashRT = NULL;
        m_pTempRT = NULL;

        m_edgeFadeScale = 0.0f;
        m_alpha = 1.0f;
        m_groupCount = 0;
        m_post3DRendererflags = 0;
        m_deferDisableFrameCountDown = 0;
    }

    virtual bool Preprocess();
    virtual void Render();
    virtual void Reset(bool bOnSpecChange = false);

    virtual const char* GetName() const
    {
        return "Post3DRenderer";
    }

private:

    ILINE bool HasModelsToRender() const
    {
        SRenderListDesc* pRenderListDesc = gRenDev->m_RP.m_pRLD;
        const uint32 batchMask = SRendItem::BatchFlags(EFSLIST_GENERAL, pRenderListDesc)
            | SRendItem::BatchFlags(EFSLIST_SKIN, pRenderListDesc)
            | SRendItem::BatchFlags(EFSLIST_DECAL, pRenderListDesc)
            | SRendItem::BatchFlags(EFSLIST_TRANSP, pRenderListDesc);
        return (batchMask & FB_POST_3D_RENDER) ? true : false;
    }

    void    ClearFlashRT();

    void  RenderGroup(uint8 groupId);
    void    RenderMeshes(uint8 groupId, float screenRect[4], ERenderMeshMode renderMeshMode = eRMM_Default);
    void    RenderDepth(uint8 groupId, float screenRect[4], bool bCustomRender = false);
    void    AlphaCorrection();
    void    GammaCorrection(float screenRect[4]);
    void    RenderSilhouettes(uint8 groupId, float screenRect[4]);
    void    SilhouetteOutlines(CTexture* pOutlineTex, CTexture* pGlowTex);
    void    SilhouetteGlow(CTexture* pOutlineTex, CTexture* pGlowTex);
    void    SilhouetteCombineBlurAndOutline(CTexture* pOutlineTex, CTexture* pGlowTex);
    void    ApplyShaderQuality(EShaderType shaderType = eST_General);

    void    ProcessRenderList(int list, uint32 batchFilter, uint8 groupId, float screenRect[4], bool bCustomRender = false);
    void  ProcessBatchesList(int listStart, int listEnd, uint32 batchFilter, uint8 groupId, float screenRect[4], bool bCustomRender = false);

    CCryNameTSCRC       m_gammaCorrectionTechName;
    CCryNameTSCRC       m_alphaCorrectionTechName;
    CCryNameTSCRC     m_texToTexTechName;
    CCryNameTSCRC       m_customRenderTechName;
    CCryNameTSCRC       m_combineSilhouettesTechName;
    CCryNameTSCRC       m_silhouetteTechName;

    CCryNameR               m_psParamName;
    CCryNameR               m_vsParamName;

    CEffectParam*       m_pFOVScale;
    CEffectParam*       m_pAmbient;
    CEffectParam*       m_pSilhouetteStrength;
    CEffectParam*       m_pEdgeFadeScale;
    CEffectParam*       m_pPixelAspectRatio;

    CTexture*               m_pFlashRT;
    CTexture*               m_pTempRT;

    float                       m_alpha;
    float                       m_edgeFadeScale;

    uint8                       m_groupCount;
    uint8                       m_post3DRendererflags;
    uint8                       m_deferDisableFrameCountDown;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_POSTPROCESS_POSTEFFECTS_H
