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
#include "Glow.h"
#include "../Textures/Texture.h"
#include "../CryNameR.h"
#include "../../RenderDll/XRenderD3D9/DriverD3D.h"

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&Glow::FUNC_NAME)
void Glow::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsElement::InitEditorParamGroups(groups);

    FuncVariableGroup glowGroup;
    glowGroup.SetName("Glow");
    glowGroup.AddVariable(new OpticsMFPVariable(e_INT, "Polygon factor", "Polygons factor", this, MFPtr(SetPolygonFactor), MFPtr(GetPolygonFactor), 0, 128.0f));
    glowGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Focus factor", "Focus factor", this, MFPtr(SetFocusFactor), MFPtr(GetFocusFactor)));
    glowGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Gamma", "Gamma", this, MFPtr(SetGamma), MFPtr(GetGamma)));
    groups.push_back(glowGroup);
}
#undef MFPtr
#endif

void Glow::Load(IXmlNode* pNode)
{
    COpticsElement::Load(pNode);

    XmlNodeRef pGlowNode = pNode->findChild("Glow");
    if (pGlowNode)
    {
        int nPolygonFactor(0);
        if (pGlowNode->getAttr("Polygonfactor", nPolygonFactor))
        {
            SetPolygonFactor(nPolygonFactor);
        }

        float fFocusFactor(m_fFocusFactor);
        if (pGlowNode->getAttr("Focusfactor", fFocusFactor))
        {
            SetFocusFactor(fFocusFactor);
        }

        float fGamma(m_fGamma);
        if (pGlowNode->getAttr("Gamma", fGamma))
        {
            SetGamma(fGamma);
        }
    }
}

void Glow::GenMesh()
{
    float ringPos = 1;
    MeshUtil::GenDisk(m_fSize, (int)m_fPolyonFactor, 1, true, m_globalColor, &ringPos, m_vertBuf, m_idxBuf);
}

void Glow::ApplyDistributionParamsPS(CShader* shader)
{
    static CCryNameR lumaParamsName("lumaParams");
    Vec4 lumaParams(m_fFocusFactor, m_fGamma, 0, 0);
    shader->FXSetPSFloat(lumaParamsName, &lumaParams, 1);
}

void Glow::DrawMesh()
{
    gcpRendD3D->FX_Commit();
    DrawMeshTriList();
}

void Glow::Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, [[maybe_unused]] SAuxParams& aux)
{
    if (!IsVisible())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("Glow");

    gRenDev->m_RP.m_FlagsShader_RT = 0;

    static CCryNameTSCRC pGlowTechName("Glow");
    shader->FXSetTechnique(pGlowTechName);
    uint nPass;

    shader->FXBegin(&nPass, FEF_DONTSETTEXTURES);

    ApplyGeneralFlags(shader);
    shader->FXBeginPass(0);

    ApplyCommonVSParams(shader, vSrcWorldPos, vSrcProjPos);
    ApplyExternTintAndBrightnessVS(shader, m_globalColor, m_globalFlareBrightness);

    static CCryNameR meshCenterName("meshCenterAndBrt");
    float x = computeMovementLocationX(vSrcProjPos);
    float y = computeMovementLocationY(vSrcProjPos);
    const Vec4 meshCenterParam(x, y, vSrcProjPos.z, 1);
    shader->FXSetVSFloat(meshCenterName, &meshCenterParam, 1);

    ApplyDistributionParamsPS(shader);

    ValidateMesh();
    ApplyMesh();
    DrawMesh();

    shader->FXEndPass();

    shader->FXEnd();
}
