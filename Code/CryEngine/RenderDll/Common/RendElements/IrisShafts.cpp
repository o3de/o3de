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
#include "IrisShafts.h"
#include "RootOpticsElement.h"
#include "FlareSoftOcclusionQuery.h"
#include "../CryNameR.h"
#include "../../RenderDll/XRenderD3D9/DriverD3D.h"
#include "../Common/Textures/TextureManager.h"

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&IrisShafts::FUNC_NAME)
void IrisShafts::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsElement::InitEditorParamGroups(groups);
    FuncVariableGroup irisGroup;
    irisGroup.SetName("IrisShafts", "Iris Shafts");
    irisGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Enable gradient tex", "Enable gradient texture", this, MFPtr(SetEnableSpectrumTex), MFPtr(GetEnableSpectrumTex)));
    irisGroup.AddVariable(new OpticsMFPVariable(e_TEXTURE2D, "Base Tex", "Basic Texture", this, MFPtr(SetBaseTex), MFPtr(GetBaseTex)));
    irisGroup.AddVariable(new OpticsMFPVariable(e_TEXTURE2D, "Gradient Tex", "Gradient Texture", this, MFPtr(SetSpectrumTex), MFPtr(GetSpectrumTex)));
    irisGroup.AddVariable(new OpticsMFPVariable(e_INT, "Noise seed", "Noise seed", this, MFPtr(SetNoiseSeed), MFPtr(GetNoiseSeed), -255.0f, 255.0f));
    irisGroup.AddVariable(new OpticsMFPVariable(e_INT, "Complexity", "Complexity of shafts", this, MFPtr(SetComplexity), MFPtr(GetComplexity), 0, 1000.0f));
    irisGroup.AddVariable(new OpticsMFPVariable(e_INT, "Smoothness", "The level of smoothness", this, MFPtr(SetSmoothLevel), MFPtr(GetSmoothLevel), 0, 255.0f));
    irisGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Thickness", "Thickness of the shafts", this, MFPtr(SetThickness), MFPtr(GetThickness), 0, 255.0f));
    irisGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Thickness noise", "Noise strength of thickness variation", this, MFPtr(SetThicknessNoise), MFPtr(GetThicknessNoise)));
    irisGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Spread", "Spread of the shafts", this, MFPtr(SetSpread), MFPtr(GetSpread)));
    irisGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Spread noise", "Noise strength of spread variation", this, MFPtr(SetSpreadNoise), MFPtr(GetSpreadNoise)));
    irisGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Size noise", "Noise strength of shafts' sizes", this, MFPtr(SetSizeNoise), MFPtr(GetSizeNoise)));
    irisGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Spacing noise", "Noise strength of shafts' spacing", this, MFPtr(SetSpacingNoise), MFPtr(GetSpacingNoise)));
    groups.push_back(irisGroup);
}
#undef MFPtr
#endif

void IrisShafts::Load(IXmlNode* pNode)
{
    COpticsElement::Load(pNode);

    XmlNodeRef pIrisShaftsNode = pNode->findChild("IrisShafts");

    if (pIrisShaftsNode)
    {
        bool bUseSpectrumTex(m_bUseSpectrumTex);
        if (pIrisShaftsNode->getAttr("Enablegradienttex", bUseSpectrumTex))
        {
            SetEnableSpectrumTex(bUseSpectrumTex);
        }

        const char* baseTextureName = NULL;
        if (pIrisShaftsNode->getAttr("BaseTex", &baseTextureName))
        {
            if (baseTextureName && baseTextureName[0])
            {
                ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(baseTextureName);
                SetBaseTex((CTexture*)pTexture);
                //Release this reference because we're no longer going to reference the texture from this pointer
                if (pTexture)
                {
                    pTexture->Release();
                }
            }
        }

        const char* gradientTexName = NULL;
        if (pIrisShaftsNode->getAttr("GradientTex", &gradientTexName))
        {
            if (gradientTexName && gradientTexName[0])
            {
                ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(gradientTexName);
                SetSpectrumTex((CTexture*)pTexture);
                //Release this reference because we're no longer going to reference the texture from this pointer
                if (pTexture)
                {
                    pTexture->Release();
                }
            }
        }

        int nNoiseSeed(m_nNoiseSeed);
        if (pIrisShaftsNode->getAttr("Noiseseed", nNoiseSeed))
        {
            SetNoiseSeed(nNoiseSeed);
        }

        int nComplexity(m_nComplexity);
        if (pIrisShaftsNode->getAttr("Complexity", nComplexity))
        {
            SetComplexity(nComplexity);
        }

        int nSmoothLevel(m_nSmoothLevel);
        if (pIrisShaftsNode->getAttr("Smoothness", nSmoothLevel))
        {
            SetSmoothLevel(nSmoothLevel);
        }

        float fThickness(m_fThickness);
        if (pIrisShaftsNode->getAttr("Thickness", fThickness))
        {
            SetThickness(fThickness);
        }

        float fThicknessNoise(m_fThicknessNoiseStrength);
        if (pIrisShaftsNode->getAttr("Thicknessnoise", fThicknessNoise))
        {
            SetThicknessNoise(fThicknessNoise);
        }

        float fSpread(m_fSpread);
        if (pIrisShaftsNode->getAttr("Spread", fSpread))
        {
            SetSpread(fSpread);
        }

        float fThicknessNoiseStrength(m_fThicknessNoiseStrength);
        if (pIrisShaftsNode->getAttr("Spreadnoise", fThicknessNoiseStrength))
        {
            SetSpreadNoise(fThicknessNoiseStrength);
        }

        float fSizeNoiseStrength(m_fSizeNoiseStrength);
        if (pIrisShaftsNode->getAttr("Sizenoise", fSizeNoiseStrength))
        {
            SetSizeNoise(fSizeNoiseStrength);
        }

        float fSpacingNoiseStrength(m_fSpacingNoiseStrength);
        if (pIrisShaftsNode->getAttr("Spacingnoise", fSpacingNoiseStrength))
        {
            SetSpacingNoise(fSpacingNoiseStrength);
        }
    }
}

float IrisShafts::ComputeSpreadParameters(const float spread)
{
    return spread * 75;
}

int IrisShafts::ComputeDynamicSmoothLevel(int maxLevel, float spanAngle, float threshold)
{
    return min(maxLevel, (int)(spanAngle / threshold));
}

void IrisShafts::GenMesh()
{
    stable_rand::setSeed((int)(m_nNoiseSeed * m_fConcentrationBoost));

    float dirDelta = 1.0f / (float)m_nComplexity;
    float halfAngleRange = m_fAngleRange / 2;
    ColorF color(1, 1, 1, 1);

    m_vertBuf.clear();
    m_idxBuf.clear();

    std::vector<int> randomTable;
    randomTable.resize(m_nComplexity);
    for (int i = 0; i < m_nComplexity; ++i)
    {
        randomTable[i] = i;
    }
    for (int i = 0; i < m_nComplexity; ++i)
    {
        std::swap(randomTable[(int)(stable_rand::randPositive() * m_nComplexity)], randomTable[(int)(stable_rand::randPositive() * m_nComplexity)]);
    }

    for (int i = 0; i < m_nComplexity; i++)
    {
        float spacingNoise = 1 + stable_rand::randUnit() * m_fSpacingNoiseStrength;

        float dirDiff;
        if (m_fConcentrationBoost > 1.f && randomTable[i] == m_nComplexity / 2)
        {
            dirDiff = dirDelta * spacingNoise * 0.05f;
        }
        else
        {
            dirDiff = m_fAngleRange * fmod((randomTable[i] * dirDelta + spacingNoise), 1.0f) - halfAngleRange;
        }
        float dirUnit = m_fPrimaryDir + dirDiff;
        float dir = 360 * dirUnit;

        float dirDiffRatio = fabs(dirDiff) / m_fAngleRange;
        float sizeBoost = 1 + (m_fConcentrationBoost - 1) * (-1.75f * dirDiffRatio + 2.f);  // From center to edge: 2->0.25
        float brightnessBoost = 1 + (m_fConcentrationBoost - 1) * (1 / (15 * (dirDiffRatio + 0.02f)) - 1);  // from center to edge: 2.333->-0.994

        color.a = brightnessBoost;
        float size = sizeBoost * (1 + stable_rand::randUnit() * m_fSizeNoiseStrength);
        float thickness = m_fThickness * (1 + stable_rand::randUnit() * m_fThicknessNoiseStrength);
        float spread = m_fSpread * (1 + stable_rand::randUnit() * m_fSpreadNoiseStrength);
        float halfAngle = ComputeSpreadParameters(spread);
        int dynSmoothLevel = ComputeDynamicSmoothLevel(m_nSmoothLevel, halfAngle * 2, 1);
        if (dynSmoothLevel <= 1)
        {
            continue;
        }

        std::vector<SVF_P3F_C4B_T2F> vertices;
        std::vector<uint16> indices;
        MeshUtil::GenShaft(size, thickness, dynSmoothLevel, dir - halfAngle, dir + halfAngle, color, vertices, indices);

        int generatedPolyonNum = (int)(indices.size() + m_idxBuf.size()) / 3;
        if (CRenderer::CV_r_FlaresIrisShaftMaxPolyNum != 0 && generatedPolyonNum > CRenderer::CV_r_FlaresIrisShaftMaxPolyNum)
        {
            break;
        }

        int indexOffset = m_vertBuf.size();
        m_vertBuf.insert(m_vertBuf.end(), vertices.begin(), vertices.end());

        for (int k = 0, iIndexSize(indices.size()); k < iIndexSize; ++k)
        {
            m_idxBuf.push_back(indices[k] + indexOffset);
        }
    }
}

void IrisShafts::Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, [[maybe_unused]] SAuxParams& aux)
{
    if (!IsVisible())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("IrisShafts");

    gRenDev->m_RP.m_FlagsShader_RT = 0;

    vSrcProjPos = computeOrbitPos(vSrcProjPos, m_globalOrbitAngle);
    static CCryNameTSCRC pIrisShaftsTechName("IrisShafts");
    shader->FXSetTechnique(pIrisShaftsTechName);
    uint nPass;
    shader->FXBegin(&nPass, FEF_DONTSETTEXTURES);

    ApplyGeneralFlags(shader);
    ApplySpectrumTexFlag(shader, m_bUseSpectrumTex);
    shader->FXBeginPass(0);

    if (m_globalOcclusionBokeh)
    {
        RootOpticsElement* root = GetRoot();
        CFlareSoftOcclusionQuery* occQuery = root->GetOcclusionQuery();
        float occ = occQuery->GetOccResult();
        float interpOcc = root->GetShaftVisibilityFactor();

        if (occ >= 0.05f && fabs(m_fPrevOcc - occ) > 0.01f)
        {
            m_fAngleRange = 0.65f * occ * occ + 0.35f;
            m_fPrimaryDir = occQuery->GetDirResult() / (2 * PI);
            m_fConcentrationBoost = 2.0f - occ;
            m_fBrightnessBoost = aznumeric_cast<float>(1 + 2 * pow(occ - 1, 6));

            m_meshDirty = true;
            m_fPrevOcc = occ;
        }
        else if (occ < 0.05f)
        {
            m_fAngleRange = 0.33f;
            m_fBrightnessBoost = aznumeric_cast<float>(1 + 2 * pow(interpOcc - 1, 6));
            m_fPrevOcc = 0;
            m_meshDirty = true;
        }
    }
    else
    {
        if (m_fAngleRange < 0.999f)
        {
            m_meshDirty = true;
        }
        m_fPrimaryDir = 0;
        m_fAngleRange = 1;
        m_fConcentrationBoost = 1;
        m_fBrightnessBoost = 1;
    }

    ApplyCommonVSParams(shader, vSrcWorldPos, vSrcProjPos);
    ApplyExternTintAndBrightnessVS(shader, m_globalColor, m_globalFlareBrightness);

    static CCryNameR meshCenterName("meshCenterAndBrt");
    const float x = computeMovementLocationX(vSrcProjPos);
    const float y = computeMovementLocationY(vSrcProjPos);
    const Vec4 meshCenterParam(x, y, vSrcProjPos.z, 1);
    shader->FXSetVSFloat(meshCenterName, &meshCenterParam, 1);

    static STexState bilinearTS(FILTER_LINEAR, true);
    bilinearTS.SetBorderColor(0);
    bilinearTS.SetClampMode(TADDR_BORDER, TADDR_BORDER, TADDR_BORDER);

    CTexture* pBaseTex = ((CTexture*)m_pBaseTex) ? ((CTexture*)m_pBaseTex) : CTextureManager::Instance()->GetBlackTexture();
    pBaseTex->Apply(1, CTexture::GetTexState(bilinearTS));

    CTexture* pSpectrumTex = (m_bUseSpectrumTex && ((CTexture*)m_pSpectrumTex)) ? ((CTexture*)m_pSpectrumTex) : CTextureManager::Instance()->GetBlackTexture();
    pSpectrumTex->Apply(0, CTexture::GetTexState(bilinearTS));

    if (m_MaxNumberOfPolygon != CRenderer::CV_r_FlaresIrisShaftMaxPolyNum)
    {
        m_meshDirty = true;
        m_MaxNumberOfPolygon = CRenderer::CV_r_FlaresIrisShaftMaxPolyNum;
    }

    ValidateMesh();

    ApplyMesh();
    DrawMeshTriList();

    shader->FXEndPass();

    shader->FXEnd();
}
