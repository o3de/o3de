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

#include "RootOpticsElement.h"
#include "FlareSoftOcclusionQuery.h"
#include "../Textures/Texture.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "stdarg.h"

enum EVisFader
{
    VISFADER_FLARE = 0,
    VISFADER_SHAFT,
    VISFADER_NUM
};

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&RootOpticsElement::FUNC_NAME)
void  RootOpticsElement::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsGroup::InitEditorParamGroups(groups);

    FuncVariableGroup rootGroup;
    rootGroup.SetName("GlobalSettings", "Global Settings");
    rootGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Enable Occlusion", "Enable Occlusion", this, MFPtr(SetOcclusionEnabled), MFPtr(IsOcclusionEnabled)));
    rootGroup.AddVariable(new OpticsMFPVariable(e_VEC2, "Occlusion Size", "The size for occlusion plane", this, MFPtr(SetOccSize), MFPtr(GetOccSize)));
    rootGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Flare fade time", "The duration of flare afterimage fading in seconds", this, MFPtr(SetFlareFadingDuration), MFPtr(GetFlareFadingDuration)));
    rootGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Shaft fade time", "The duration of shaft afterimage fading in seconds", this, MFPtr(SetShaftFadingDuration), MFPtr(GetShaftFadingDuration)));
    rootGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Affected by light color", "light color can affect flare color", this, MFPtr(SetAffectedByLightColor), MFPtr(IsAffectedByLightColor)));
    rootGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Affected by light radius", "light radius can affect flare fading", this, MFPtr(SetAffectedByLightRadius), MFPtr(IsAffectedByLightRadius)));
    rootGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Affected by light FOV", "light projection FOV can affect flare fading", this, MFPtr(SetAffectedByLightFOV), MFPtr(IsAffectedByLightFOV)));
    rootGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Multiply Color", "Select one of between Multiply and Addition about color calculation. If true, Multiply will be chosen.", this, MFPtr(SetMultiplyColor), MFPtr(IsMultiplyColor)));
    groups.push_back(rootGroup);

    FuncVariableGroup sensorGroup;
    sensorGroup.SetName("Sensor");
    sensorGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Custom Sensor Variation Map", "Enable Custom Sensor Variation Map", this, MFPtr(SetCustomSensorVariationMapEnabled), MFPtr(IsCustomSensorVariationMapEnabled)));
    sensorGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Effective Sensor Size", "The size of image-able part of the sensor", this, MFPtr(SetEffectiveSensorSize), MFPtr(GetEffectiveSensorSize)));
    groups.push_back(sensorGroup);
}
#undef MFPtr
#endif

void RootOpticsElement::Load(IXmlNode* pNode)
{
    COpticsElement::Load(pNode);

    XmlNodeRef pGloabalSettingsNode = pNode->findChild("GlobalSettings");
    if (pGloabalSettingsNode)
    {
        bool bOcclusionEnabled(m_bOcclusionEnabled);
        if (pGloabalSettingsNode->getAttr("EnableOcclusion", bOcclusionEnabled))
        {
            SetOcclusionEnabled(bOcclusionEnabled);
        }

        Vec2 occlusionSize(m_OcclusionSize);
        if (pGloabalSettingsNode->getAttr("OcclusionSize", occlusionSize))
        {
            SetOccSize(occlusionSize);
        }

        float fFlareTimelineDuration(m_fFlareTimelineDuration);
        if (pGloabalSettingsNode->getAttr("Flarefadetime", fFlareTimelineDuration))
        {
            SetFlareFadingDuration(fFlareTimelineDuration);
        }

        float fShaftTimelineDuration(m_fShaftTimelineDuration);
        if (pGloabalSettingsNode->getAttr("Flarefadetime", fShaftTimelineDuration))
        {
            SetShaftFadingDuration(fShaftTimelineDuration);
        }

        bool bAffectedByLightColor(m_bAffectedByLightColor);
        if (pGloabalSettingsNode->getAttr("Affectedbylightcolor", bAffectedByLightColor))
        {
            SetAffectedByLightColor(bAffectedByLightColor);
        }

        bool bAffectedByLightRadius(m_bAffectedByLightRadius);
        if (pGloabalSettingsNode->getAttr("Affectedbylightradius", bAffectedByLightRadius))
        {
            SetAffectedByLightRadius(bAffectedByLightRadius);
        }

        bool bAffectedByLightFOV(m_bAffectedByLightFOV);
        if (pGloabalSettingsNode->getAttr("AffectedbylightFOV", bAffectedByLightFOV))
        {
            SetAffectedByLightFOV(bAffectedByLightFOV);
        }

        bool bMultiplyColor(m_bMultiplyColor);
        if (pGloabalSettingsNode->getAttr("MultiplyColor", bMultiplyColor))
        {
            SetMultiplyColor(bMultiplyColor);
        }
    }

    XmlNodeRef pSensorNode = pNode->findChild("Sensor");
    if (pSensorNode)
    {
        bool bCustomSensorVariationMap(m_bCustomSensorVariationMap);
        if (pSensorNode->getAttr("CustomSensorVariationMap", bCustomSensorVariationMap))
        {
            SetCustomSensorVariationMapEnabled(bCustomSensorVariationMap);
        }

        float fEffectiveSensorSize(m_fEffectiveSensorSize);
        if (pSensorNode->getAttr("EffectiveSensorSize", fEffectiveSensorSize))
        {
            SetEffectiveSensorSize(fEffectiveSensorSize);
        }
    }
}

void RootOpticsElement::SetOcclusionQuery(CFlareSoftOcclusionQuery* query)
{
    m_pOccQuery = query;
}

float RootOpticsElement::GetFlareVisibilityFactor() const
{
    CSoftOcclusionVisiblityFader* pFader = m_pOccQuery ? m_pOccQuery->GetVisibilityFader(VISFADER_FLARE) : NULL;
    return pFader ? pFader->m_fVisibilityFactor : 0.0f;
}

float RootOpticsElement::GetShaftVisibilityFactor() const
{
    CSoftOcclusionVisiblityFader* pFader = m_pOccQuery ? m_pOccQuery->GetVisibilityFader(VISFADER_SHAFT) : NULL;
    return pFader ? pFader->m_fVisibilityFactor : 0.0f;
}

void RootOpticsElement::SetVisibilityFactor(float f)
{
    f = clamp_tpl(f, 0.f, 1.f);

    if (m_pOccQuery)
    {
        for (uint i = 0; i < VISFADER_NUM; ++i)
        {
            if (CSoftOcclusionVisiblityFader* pFader = m_pOccQuery->GetVisibilityFader(i))
            {
                pFader->m_fVisibilityFactor = f;
            }
        }
    }
}

CTexture* RootOpticsElement::GetOcclusionPattern()
{
    return m_pOccQuery->GetGatherTexture();
}

void RootOpticsElement::validateGlobalVars(SAuxParams& aux)
{
    m_globalPerpectiveFactor = m_fPerpectiveFactor;
    m_globalDistanceFadingFactor = m_flareLight.m_bAttachToSun ? 0.0f : m_fDistanceFadingFactor;
    m_globalSensorBrightnessFactor = m_fSensorBrightnessFactor;
    m_globalSensorSizeFactor = m_fSensorSizeFactor;
    m_globalSize = m_fSize;
    m_globalFlareBrightness = GetBrightness();
    m_globalMovement = m_vMovement;

    if (m_bAffectedByLightColor)
    {
        if (aux.bMultiplyColor)
        {
            m_globalColor = ColorF(m_Color.r * m_flareLight.m_cLdrClr.r, m_Color.g * m_flareLight.m_cLdrClr.g, m_Color.b * m_flareLight.m_cLdrClr.b, m_Color.a);
        }
        else
        {
            m_globalColor = ColorF(m_Color.r + m_flareLight.m_cLdrClr.r, m_Color.g + m_flareLight.m_cLdrClr.g, m_Color.b + m_flareLight.m_cLdrClr.b, m_Color.a);
        }

        m_globalFlareBrightness *= m_flareLight.m_fClrMultiplier;
    }
    else
    {
        m_globalColor = m_Color;
    }

    if (m_bAffectedByLightRadius)
    {
        m_globalFlareBrightness *= clamp_tpl(1 - aux.distance / m_flareLight.m_fRadius, 0.0f, 1.0f);
    }

    if (m_bAffectedByLightFOV)
    {
        m_globalFlareBrightness *= aux.viewAngleFalloff;
    }

    float fShaftVisibilityFactor = aux.bForceRender ? 1.0f : GetShaftVisibilityFactor();
    float fFlareVisibilityFactor = aux.bForceRender ? 1.0f : GetFlareVisibilityFactor();

    m_globalShaftBrightness = m_globalFlareBrightness * fShaftVisibilityFactor;
    m_globalFlareBrightness *= fFlareVisibilityFactor;

    m_globalOcclusionBokeh = m_bOcclusionBokeh & IsOcclusionEnabled();
    m_globalCorrectAspectRatio = m_globalCorrectAspectRatio;
    m_globalAutoRotation = m_globalAutoRotation;
    m_globalOrbitAngle = m_fOrbitAngle;
    m_globalTransform = m_mxTransform;

    COpticsGroup::validateChildrenGlobalVars(aux);
}

void RootOpticsElement::Render(SLensFlareRenderParam* pParam, const Vec3& vPos)
{
    if (pParam == NULL)
    {
        return;
    }

    if (!pParam->IsValid())
    {
        return;
    }

    SFlareLight light;
    light.m_vPos = vPos;
    light.m_fRadius = 10000.0f;
    light.m_bAttachToSun = false;
    light.m_cLdrClr = ColorF(1, 1, 1, 1);
    light.m_fClrMultiplier = 1;
    light.m_fViewAngleFalloff = 1;

    ProcessAll((CShader*)pParam->pShader, light, true, pParam->pCamera);
}

bool RootOpticsElement::ProcessAll(CShader* shader, SFlareLight& light, bool bForceRender, CCamera* pCamera)
{
    CD3D9Renderer* pRD = gcpRendD3D;

    Vec3 vSrcWorldPos = light.m_vPos;
    Vec3 vSrcProjPos;

    m_flareLight = light;
    float linearDepth = 0;
    float distance = 0;

    if (pCamera)
    {
        Matrix44A mProj, mView;
        mathMatrixPerspectiveFov(&mProj, pCamera->GetFov(), pCamera->GetProjRatio(), pCamera->GetNearPlane(), pCamera->GetFarPlane());
        mathMatrixLookAt(&mView, pCamera->GetPosition(), pCamera->GetPosition() + pCamera->GetViewdir(), Vec3(0, 0, 1));
        if (!CFlareSoftOcclusionQuery::ComputeProjPos(vSrcWorldPos, mView, mProj, vSrcProjPos))
        {
            return false;
        }
        linearDepth = clamp_tpl(CFlareSoftOcclusionQuery::ComputeLinearDepth(vSrcWorldPos, mView, pCamera->GetNearPlane(), pCamera->GetFarPlane()), -1.0f, 0.99f);
        distance = mView.GetTranslation().GetDistance(vSrcWorldPos);
    }
    else
    {
        if (!CFlareSoftOcclusionQuery::ComputeProjPos(vSrcWorldPos, pRD->m_ViewMatrix, pRD->m_ProjMatrix, vSrcProjPos))
        {
            return false;
        }

        if (pRD->m_RP.m_TI[pRD->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
        {
            vSrcProjPos.z = 1.0f - vSrcProjPos.z;
        }

        const CameraViewParameters& rc = gRenDev->GetViewParameters();
        linearDepth = clamp_tpl(CFlareSoftOcclusionQuery::ComputeLinearDepth(vSrcWorldPos, pRD->m_CameraMatrix, rc.fNear, rc.fFar), -1.0f, 0.99f);
        distance = pRD->GetViewParameters().vOrigin.GetDistance(vSrcWorldPos);
    }

    if (GetElementCount() <= 0 || !IsEnabled())
    {
        return false;
    }

    if (!bForceRender && (linearDepth <= 0 || !IsVisibleBasedOnLight(light, distance)))
    {
        return false;
    }

    bool bVisible(!IsOcclusionEnabled());

    if (!bVisible && !bForceRender)
    {
        float curTargetVisibility = 0.0f;
        m_fFlareVisibilityFactor = m_fShaftVisibilityFactor = 0.0f;

        if (m_pOccQuery)
        {
            curTargetVisibility = m_pOccQuery->GetVisibility();

            if (CSoftOcclusionVisiblityFader* pFader = m_pOccQuery->GetVisibilityFader(VISFADER_FLARE))
            {
                m_fFlareVisibilityFactor = pFader->UpdateVisibility(curTargetVisibility, m_fFlareTimelineDuration);
            }

            if (CSoftOcclusionVisiblityFader* pFader = m_pOccQuery->GetVisibilityFader(VISFADER_SHAFT))
            {
                m_fShaftVisibilityFactor = pFader->UpdateVisibility(curTargetVisibility, m_fShaftTimelineDuration);
            }
        }

        bVisible = IsVisible();
    }

    if (bVisible || bForceRender)
    {
        SAuxParams aux;
        aux.linearDepth = linearDepth;
        aux.distance = distance;
        float x = vSrcProjPos.x * 2 - 1;
        float y = vSrcProjPos.y * 2 - 1;
        float unitLenSq = (x * x + y * y);
        aux.sensorVariationValue = clamp_tpl((1 - powf(unitLenSq, 0.25f)) * 2 - 1, -1.0f, 1.0f);
        aux.perspectiveShortening = clamp_tpl(10.f * (1.f - vSrcProjPos.z), 0.0f, 2.0f);
        aux.viewAngleFalloff = light.m_fViewAngleFalloff;
        aux.attachToSun = light.m_bAttachToSun;
        aux.bMultiplyColor = IsMultiplyColor();
        aux.bForceRender = bForceRender;

        // The legacy entity system does not allow for overriding the color, brightness and size of lens flares.
        // The new component systems does allow for overriding these values so we apply them only when valid
        if (light.m_opticsParams.m_isValid)
        {
            SetColor(light.m_opticsParams.m_color);
            SetBrightness(light.m_opticsParams.m_brightness);
            SetSize(light.m_opticsParams.m_size);
        }
        validateGlobalVars(aux);
        if (bForceRender || m_globalFlareBrightness > 0.001f || m_globalShaftBrightness > 0.001f)
        {
            gcpRendD3D->m_RP.m_PersFlags2 |= RBPF2_LENS_OPTICS_COMPOSITE;
            COpticsGroup::Render(shader, vSrcWorldPos, vSrcProjPos, aux);
        }
    }

    return true;
}
