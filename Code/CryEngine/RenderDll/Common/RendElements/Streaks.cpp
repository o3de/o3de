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
#include "Streaks.h"
#include "RootOpticsElement.h"
#include "../CryNameR.h"
#include "../../RenderDll/XRenderD3D9/DriverD3D.h"

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&Streaks::FUNC_NAME)
void  Streaks::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsElement::InitEditorParamGroups(groups);

    FuncVariableGroup streakGroup;
    streakGroup.SetName("Streaks", "Streaks");
    streakGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Enable gradient tex", "Enable gradient texture", this, MFPtr(SetEnableSpectrumTex), MFPtr(GetEnableSpectrumTex)));
    streakGroup.AddVariable(new OpticsMFPVariable(e_TEXTURE2D, "Gradient Tex", "Gradient Texture", this, MFPtr(SetSpectrumTex), MFPtr(GetSpectrumTex)));
    streakGroup.AddVariable(new OpticsMFPVariable(e_INT, "Noise seed", "Noise seed", this, MFPtr(SetNoiseSeed), MFPtr(GetNoiseSeed), -255.0f, 255.0f));
    streakGroup.AddVariable(new OpticsMFPVariable(e_INT, "Streak count", "Number of streaks to generate", this, MFPtr(SetStreakCount), MFPtr(GetStreakCount), 0, 1000.0f));
    streakGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Thickness", "Thickness of the shafts", this, MFPtr(SetThickness), MFPtr(GetThickness)));
    streakGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Thickness noise", "Noise strength of thickness variation", this, MFPtr(SetThicknessNoise), MFPtr(GetThicknessNoise)));
    streakGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Size noise", "Noise strength of shafts' sizes", this, MFPtr(SetSizeNoise), MFPtr(GetSizeNoise)));
    streakGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Spacing noise", "Noise strength of shafts' spacing", this, MFPtr(SetSpacingNoise), MFPtr(GetSpacingNoise)));
    groups.push_back(streakGroup);
}
#undef MFPtr
#endif

void Streaks::Load(IXmlNode* pNode)
{
    COpticsElement::Load(pNode);

    XmlNodeRef pStreakNode = pNode->findChild("Streaks");

    if (pStreakNode)
    {
        bool bUseSpectrumTex(m_bUseSpectrumTex);
        if (pStreakNode->getAttr("Enablegradienttex", bUseSpectrumTex))
        {
            SetEnableSpectrumTex(bUseSpectrumTex);
        }

        const char* gradientTexName = NULL;
        if (pStreakNode->getAttr("GradientTex", &gradientTexName))
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

        int nNoiseSeed(m_nNoiseSeed);
        if (pStreakNode->getAttr("Noiseseed", nNoiseSeed))
        {
            SetNoiseSeed(nNoiseSeed);
        }

        int nStreakCount(m_nStreakCount);
        if (pStreakNode->getAttr("Streakcount", nStreakCount))
        {
            SetStreakCount(nStreakCount);
        }

        float fThickness(m_fThickness);
        if (pStreakNode->getAttr("Thickness", fThickness))
        {
            SetThickness(fThickness);
        }

        float fThicknessNoiseStrength(m_fThicknessNoiseStrength);
        if (pStreakNode->getAttr("Thicknessnoise", fThicknessNoiseStrength))
        {
            SetThicknessNoise(fThicknessNoiseStrength);
        }

        float fSizeNoiseStrength(m_fSizeNoiseStrength);
        if (pStreakNode->getAttr("Sizenoise", fSizeNoiseStrength))
        {
            SetSizeNoise(fSizeNoiseStrength);
        }

        float fSpacingNoiseStrength(m_fSpacingNoiseStrength);
        if (pStreakNode->getAttr("Spacingnoise", fSpacingNoiseStrength))
        {
            SetSpacingNoise(fSpacingNoiseStrength);
        }
    }
}

void Streaks::DrawMesh()
{
    gcpRendD3D->FX_Commit();
    DrawMeshTriList();
}

void Streaks::GenMesh()
{
    stable_rand::setSeed((int)m_nNoiseSeed);
    float dirDelta = 1.0f / (float)(m_separatedMeshList.size());

    ColorF tint(1, 1, 1, 1);
    for (uint32 i = 0; i < m_separatedMeshList.size(); i++)
    {
        float spacingNoise = 1 + stable_rand::randUnit() * m_fSpacingNoiseStrength;

        float dirUnit = (i * dirDelta + spacingNoise);
        float dir = dirUnit;

        float randRadius = (1 + stable_rand::randUnit() * m_fSizeNoiseStrength);
        float thickness = m_fThickness * (1 + stable_rand::randUnit() * m_fThicknessNoiseStrength);

        std::vector<SVF_P3F_C4B_T2F>& vBuf =  (m_separatedMeshList[i]);
        MeshUtil::GenStreak(dir, randRadius, thickness, tint, vBuf, m_separatedMeshIndices);
    }
}

void Streaks::ApplySingleMesh(int n)
{
    m_vertBuf = m_separatedMeshList[n];
    m_idxBuf = m_separatedMeshIndices;
    ApplyMesh();
}

void Streaks::Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, [[maybe_unused]] SAuxParams& aux)
{
    if (!IsVisible())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("DRAW_Streaks");

    gRenDev->m_RP.m_FlagsShader_RT = 0;

    vSrcProjPos = computeOrbitPos(vSrcProjPos, m_globalOrbitAngle);
    shader->FXSetTechnique("Streaks");
    uint nPass;
    shader->FXBegin(&nPass, FEF_DONTSETTEXTURES);

    ApplyGeneralFlags(shader);
    ApplySpectrumTexFlag(shader, m_bUseSpectrumTex);
    shader->FXBeginPass(0);

    // common params
    ApplyCommonVSParams(shader, vSrcWorldPos, vSrcProjPos);
    ApplyExternTintAndBrightnessVS(shader, m_globalColor, m_globalFlareBrightness);

    // meshCenter and brightness params
    static CCryNameR meshCenterName("meshCenterAndBrt");
    float x = computeMovementLocationX(vSrcProjPos);
    float y = computeMovementLocationY(vSrcProjPos);
    Vec4 meshCenterParam(x, y, vSrcProjPos.z, 1);
    shader->FXSetVSFloat(meshCenterName, &meshCenterParam, 1);

    // spectrum texture:
    static STexState bilinearTS(FILTER_LINEAR, true);
    bilinearTS.SetBorderColor(0);
    bilinearTS.SetClampMode(TADDR_BORDER, TADDR_BORDER, TADDR_BORDER);

    if (m_bUseSpectrumTex)
    {
        if (m_pSpectrumTex == NULL)
        {
            m_pSpectrumTex = CTexture::ForName("EngineAssets/Textures/flares/spectrum_full.tif",  FT_DONT_STREAM, eTF_Unknown);
            if (m_pSpectrumTex)
            {
                m_pSpectrumTex->Release();
            }
        }
        m_pSpectrumTex->Apply(0, CTexture::GetTexState(bilinearTS));
    }

    ValidateMesh();
    for (uint32 i = 0; i < m_separatedMeshList.size(); i++)
    {
        ApplySingleMesh(i);
        DrawMesh();
    }
    shader->FXEndPass();
    shader->FXEnd();
}

void Streaks::GetMemoryUsage(ICrySizer* pSizer) const
{
    int nVertexSize(0);
    for (int i = 0, iSize(m_separatedMeshList.size()); i < iSize; ++i)
    {
        nVertexSize += m_separatedMeshList[i].size() * sizeof(SVF_P3F_C4B_T2F);
    }
    pSizer->AddObject(this, sizeof(*this) + GetMeshDataSize() + nVertexSize + m_separatedMeshIndices.size() * sizeof(uint16));
}
