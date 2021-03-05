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
#include "ChromaticRing.h"
#include "../Textures/Texture.h"
#include "../CryNameR.h"
#include "../../RenderDll/XRenderD3D9/DriverD3D.h"

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&ChromaticRing::FUNC_NAME)
void  ChromaticRing::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsElement::InitEditorParamGroups(groups);

    FuncVariableGroup crGroup;
    crGroup.SetName("ChromaticRing", "Chromatic Ring");
    crGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Lock to light", "Lock to light", this, MFPtr(SetLockMovement), MFPtr(IsLockMovement)));
    crGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Thickness", "Thickness", this, MFPtr(SetWidth), MFPtr(GetWidth)));

    crGroup.AddVariable(new OpticsMFPVariable(e_INT, "Polygon complexity", "Polygon complexity", this, MFPtr(SetPolyComplexity), MFPtr(GetPolyComplexity), 0, 1024.0f));
    crGroup.AddVariable(new OpticsMFPVariable(e_TEXTURE2D, "Gradient Texture", "Gradient Texture", this, MFPtr(SetSpectrumTex), MFPtr(GetSpectrumTex)));
    crGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Enable Gradient Texture", "Enable Gradient Texture", this, MFPtr(SetUsingSpectrumTex), MFPtr(IsUsingSpectrumTex)));

    crGroup.AddVariable(new OpticsMFPVariable(e_INT, "Noise seed", "Noise seed", this, MFPtr(SetNoiseSeed), MFPtr(GetNoiseSeed), -255.0f, 255.0f));
    crGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Noise strength", "Noise strength", this, MFPtr(SetNoiseStrength), MFPtr(GetNoiseStrength)));
    crGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Completion fading", "the fading ratio at the ends of this arc", this, MFPtr(SetCompletionFading), MFPtr(GetCompletionFading)));
    crGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Completion span angle", "The span of this arc in degree", this, MFPtr(SetCompletionSpanAngle), MFPtr(GetCompletionSpanAngle)));
    crGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Completion rotation", "The rotation of this arc", this, MFPtr(SetCompletionRotation), MFPtr(GetCompletionRotation)));

    groups.push_back(crGroup);
}
#undef MFPtr
#endif

void ChromaticRing::Load(IXmlNode* pNode)
{
    COpticsElement::Load(pNode);

    XmlNodeRef pCameraOrbsNode = pNode->findChild("ChromaticRing");
    if (pCameraOrbsNode)
    {
        bool bLockMovement(m_bLockMovement);
        if (pCameraOrbsNode->getAttr("Locktolight", bLockMovement))
        {
            SetLockMovement(bLockMovement);
        }

        float fWidth(m_fWidth);
        if (pCameraOrbsNode->getAttr("Thickness", fWidth))
        {
            SetWidth(fWidth);
        }

        int nPolyComplexity(m_nPolyComplexity);
        if (pCameraOrbsNode->getAttr("Polygoncomplexity", nPolyComplexity))
        {
            SetPolyComplexity(nPolyComplexity);
        }

        const char* gradientTexName = NULL;
        if (pCameraOrbsNode->getAttr("GradientTexture", &gradientTexName))
        {
            if (gradientTexName && gradientTexName[0])
            {
                ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(gradientTexName);
                SetSpectrumTex((CTexture*)pTexture);
                if (pTexture)
                {
                    pTexture->Release();
                }
            }
        }

        bool bUseSpectrumTex(m_bUseSpectrumTex);
        if (pCameraOrbsNode->getAttr("EnableGradientTexture", bUseSpectrumTex))
        {
            SetUsingSpectrumTex(bUseSpectrumTex);
        }

        int nNoiseSeed(m_nNoiseSeed);
        if (pCameraOrbsNode->getAttr("Noiseseed", nNoiseSeed))
        {
            SetNoiseSeed(nNoiseSeed);
        }

        float fNoiseStrength(m_fNoiseStrength);
        if (pCameraOrbsNode->getAttr("Noisestrength", fNoiseStrength))
        {
            SetNoiseStrength(fNoiseStrength);
        }

        float fCompletionFading(m_fCompletionFading);
        if (pCameraOrbsNode->getAttr("Completionfading", fCompletionFading))
        {
            SetCompletionFading(fCompletionFading);
        }

        float fTotalAngle(0);
        if (pCameraOrbsNode->getAttr("Completionspanangle", fTotalAngle))
        {
            SetCompletionSpanAngle(fTotalAngle);
        }

        float fCompletionRotation(0);
        if (pCameraOrbsNode->getAttr("Completionrotation", fCompletionRotation))
        {
            SetCompletionRotation(fCompletionRotation);
        }
    }
}

void ChromaticRing::DrawMesh()
{
    gcpRendD3D->FX_Commit();
    DrawMeshTriList();
    DrawMeshWireframe();
}

float ChromaticRing::computeDynamicSize(const Vec3& vSrcProjPos, const float maxSize)
{
    Vec2 dir(vSrcProjPos.x - 0.5f, vSrcProjPos.y - 0.5f);
    float len = dir.GetLength();
    const float hoopDistFactor = 2.3f;
    return len * hoopDistFactor * maxSize;
}

void ChromaticRing::Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, [[maybe_unused]] SAuxParams& aux)
{
    if (!IsVisible())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("ChromaticRing");

    gRenDev->m_RP.m_FlagsShader_RT = 0;

    vSrcProjPos = computeOrbitPos(vSrcProjPos, m_globalOrbitAngle);

    static CCryNameTSCRC pChromaticRingTechName("ChromaticRing");
    shader->FXSetTechnique(pChromaticRingTechName);
    uint nPass;
    shader->FXBegin(&nPass, FEF_DONTSETTEXTURES);

    ApplyGeneralFlags(shader);
    ApplySpectrumTexFlag(shader, m_bUseSpectrumTex);
    shader->FXBeginPass(0);

    float newSize = computeDynamicSize(vSrcProjPos, m_globalSize);

    float oldSize = m_globalSize;
    m_globalSize = newSize;
    ApplyCommonVSParams(shader, vSrcWorldPos, vSrcProjPos);
    m_globalSize = oldSize;
    ApplyExternTintAndBrightnessVS(shader, m_globalColor, m_globalFlareBrightness);

    static CCryNameR meshCenterName("meshCenterAndBrt");
    float x, y;
    if (m_bLockMovement)
    {
        x = vSrcProjPos.x;
        y = vSrcProjPos.y;
    }
    else
    {
        x = computeMovementLocationX(vSrcProjPos);
        y = computeMovementLocationY(vSrcProjPos);
    }
    const Vec4 meshCenterParam(x, y, vSrcProjPos.z, m_globalFlareBrightness);
    shader->FXSetVSFloat(meshCenterName, &meshCenterParam, 1);

    static STexState bilinearTS(FILTER_LINEAR, true);
    bilinearTS.SetBorderColor(0);
    bilinearTS.SetClampMode(TADDR_BORDER, TADDR_BORDER, TADDR_BORDER);
    if (m_pSpectrumTex == NULL)
    {
        m_pSpectrumTex = CTexture::ForName("EngineAssets/Textures/flares/spectrum_full.tif",  FT_DONT_STREAM, eTF_Unknown);
    }
    m_pSpectrumTex->Apply(0, CTexture::GetTexState(bilinearTS));

    ValidateMesh();
    ApplyMesh();
    DrawMesh();
    shader->FXEndPass();

    shader->FXEnd();
}
