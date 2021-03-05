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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_ROOTOPTICSELEMENT_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_ROOTOPTICSELEMENT_H
#pragma once

#include "OpticsGroup.h"
#include "Timeline.h"

class CRenderObject;
class CFlareSoftOcclusionQuery;

class RootOpticsElement
    : public COpticsGroup
{
public:
    struct SFlareLight
    {
        Vec3 m_vPos;
        ColorF m_cLdrClr;
        float m_fClrMultiplier;
        float m_fRadius;
        float m_fViewAngleFalloff;
        bool m_bAttachToSun;
        SOpticsInstanceParameters m_opticsParams;
    };

private:

    bool m_bChromaticAbrEnabled : 1;
    bool m_bLateralChromaticAbr : 1;
    float m_fChromaticAbrOffset;
    float m_fChromaticAbrDir;

    float m_fEffectiveSensorSize;
    bool m_bCustomSensorVariationMap : 1;

    bool m_bPostBlur : 1;
    float m_fPostBlurAmount;

    bool m_bOcclusionEnabled : 1;
    float m_fFlareVisibilityFactor;
    float m_fShaftVisibilityFactor;
    float m_fFlareTimelineDuration;
    float m_fShaftTimelineDuration;
    CFlareSoftOcclusionQuery* m_pOccQuery;

    bool m_bAffectedByLightColor : 1;
    bool m_bAffectedByLightRadius : 1;
    bool m_bAffectedByLightFOV : 1;
    SFlareLight m_flareLight;
    Vec2 m_OcclusionSize;
    bool m_bMultiplyColor : 1;

public:

    RootOpticsElement()
        : COpticsGroup("@root")
        , m_bChromaticAbrEnabled(false)
        , m_bLateralChromaticAbr(false)
        , m_fChromaticAbrOffset(0.002f)
        , m_fChromaticAbrDir(0.785f)
        , m_bOcclusionEnabled(true)
        , m_fFlareVisibilityFactor(1)
        , m_fShaftVisibilityFactor(1)
        , m_pOccQuery(NULL)
        , m_fEffectiveSensorSize(0.8f)
        , m_bCustomSensorVariationMap(false)
        , m_bAffectedByLightColor(false)
        , m_bAffectedByLightRadius(false)
        , m_bAffectedByLightFOV(true)
        , m_bPostBlur(false)
        , m_fPostBlurAmount(0)
        , m_bMultiplyColor(true)
    {
        SetFlareFadingDuration(0.2f);
        SetShaftFadingDuration(1.0f);
        m_OcclusionSize = Vec2(0.02f, 0.02f);
        m_fPerpectiveFactor = 1;
    }

    ~RootOpticsElement()
    {
    }

    EFlareType GetType() { return eFT_Root; }
    void validateGlobalVars(SAuxParams& aux);

#if defined(FLARES_SUPPORT_EDITING)
    void InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups);
#endif

    void Load(IXmlNode* pNode);

    using COpticsGroup::Render;
    void Render(SLensFlareRenderParam* pParam, const Vec3& vPos);
    bool ProcessAll(CShader* shader, SFlareLight& light, bool bForceRender = false, CCamera* pCamera = NULL);

    IOpticsElementBase* GetParent() const { return NULL; }

    float GetEffectiveSensorSize() const { return m_fEffectiveSensorSize; }
    void SetEffectiveSensorSize(float s) { m_fEffectiveSensorSize = s; }

    bool IsCustomSensorVariationMapEnabled() const { return m_bCustomSensorVariationMap; }
    void SetCustomSensorVariationMapEnabled(bool b) { m_bCustomSensorVariationMap = b; }

    bool IsVisible() const { return (m_fFlareVisibilityFactor > 0.0f) || (m_fShaftVisibilityFactor > 0.0f); }
    bool IsVisibleBasedOnLight(const SFlareLight& light, float distance) const
    {
        return (!m_bAffectedByLightRadius || distance < light.m_fRadius) &&
               (!m_bAffectedByLightColor || ((light.m_cLdrClr.r > LensOpConst::_LO_MIN || light.m_cLdrClr.g > LensOpConst::_LO_MIN || light.m_cLdrClr.b > LensOpConst::_LO_MIN) && light.m_fClrMultiplier > LensOpConst::_LO_MIN)) &&
               (!m_bAffectedByLightFOV || light.m_fViewAngleFalloff > LensOpConst::_LO_MIN);
    }
    bool IsOcclusionEnabled() const { return m_bOcclusionEnabled; }
    void SetOcclusionEnabled(bool b)
    {
        m_bOcclusionEnabled = b;
        if (!b)
        {
            SetVisibilityFactor(1.0f);
        }
    }

    void SetOcclusionQuery(CFlareSoftOcclusionQuery* query);
    CFlareSoftOcclusionQuery* GetOcclusionQuery() { return m_pOccQuery; }

    float GetFlareVisibilityFactor() const;
    float GetShaftVisibilityFactor() const;
    void SetVisibilityFactor(float f);
    Vec2 GetOccSize() const
    {
        return m_OcclusionSize;
    }
    void SetOccSize(Vec2 vSize)
    {
        m_OcclusionSize = vSize;
    }

    CTexture* GetOcclusionPattern();

    float GetFlareFadingDuration() const { return m_fFlareTimelineDuration / 1e3f; }
    void SetFlareFadingDuration(float d) { m_fFlareTimelineDuration = d * 1e3f; }

    float GetShaftFadingDuration() const { return m_fShaftTimelineDuration / 1e3f; }
    void SetShaftFadingDuration(float d) { m_fShaftTimelineDuration = d * 1e3f; }

    bool IsAffectedByLightColor() const { return m_bAffectedByLightColor; }
    void SetAffectedByLightColor(bool b)  { m_bAffectedByLightColor = b; }

    bool IsAffectedByLightRadius() const { return m_bAffectedByLightRadius; }
    void SetAffectedByLightRadius(bool b)  { m_bAffectedByLightRadius = b; }

    bool IsAffectedByLightFOV() const { return m_bAffectedByLightFOV; }
    void SetAffectedByLightFOV(bool b)  { m_bAffectedByLightFOV = b; }

    bool IsMultiplyColor() const    {   return m_bMultiplyColor;    }
    void SetMultiplyColor(bool b) { m_bMultiplyColor = b; }
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_ROOTOPTICSELEMENT_H
