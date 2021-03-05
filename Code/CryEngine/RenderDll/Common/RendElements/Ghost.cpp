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
#include "Ghost.h"
#include "MeshUtil.h"
#include "../Textures/TextureManager.h"

#include "../../RenderDll/XRenderD3D9/DriverD3D.h"

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&CLensGhost::FUNC_NAME)
void CLensGhost::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsElement::InitEditorParamGroups(groups);
    FuncVariableGroup ghostGroup;
    ghostGroup.SetName("LensGhost", "Lens Ghost");
    ghostGroup.AddVariable(new OpticsMFPVariable(e_TEXTURE2D, "Texture", "The texture for lens ghosts", this, MFPtr(SetTexture), MFPtr(GetTexture)));
    groups.push_back(ghostGroup);
}
#undef MFPtr
#endif

void CLensGhost::Load(IXmlNode* pNode)
{
    COpticsElement::Load(pNode);

    XmlNodeRef pLensGhost = pNode->findChild("LensGhost");
    if (pLensGhost)
    {
        const char* textureName(NULL);
        if (pLensGhost->getAttr("Texture", &textureName))
        {
            if (textureName && textureName[0])
            {
                ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(textureName);
                SetTexture((CTexture*)pTexture);
                //Release this reference because we're no longer going to reference the texture from this pointer
                if (pTexture)
                {
                    pTexture->Release();
                }
            }
        }
    }
}

CTexture* CLensGhost::GetTexture()
{
    if (!m_pTex)
    {
        m_pTex = CTexture::ForName("EngineAssets/Textures/flares/ghost_grey.tif",  FT_DONT_RELEASE | FT_DONT_STREAM, eTF_Unknown);
    }
    return m_pTex;
}

void CLensGhost::Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, [[maybe_unused]] SAuxParams& aux)
{
    if (!IsVisible())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("Ghost");

    gRenDev->m_RP.m_FlagsShader_RT = 0;

    vSrcProjPos = computeOrbitPos(vSrcProjPos, m_globalOrbitAngle);
    CD3D9Renderer* rd = gcpRendD3D;

    static CCryNameTSCRC pGhostTechName("Ghost");
    static CCryNameR texSizeName("baseTexSize");
    static CCryNameR tileInfoName("ghostTileInfo");

    static STexState bilinearTS(FILTER_LINEAR, true);
    bilinearTS.SetBorderColor(0);
    bilinearTS.SetClampMode(TADDR_BORDER, TADDR_BORDER, TADDR_BORDER);

    shader->FXSetTechnique(pGhostTechName);

    uint nPass;
    shader->FXBegin(&nPass, FEF_DONTSETTEXTURES);

    ApplyGeneralFlags(shader);
    ApplyOcclusionBokehFlag(shader);
    shader->FXBeginPass(0);

    CTexture* tex = GetTexture() ? GetTexture() : CTextureManager::Instance()->GetBlackTexture();
    tex->Apply(0, CTexture::GetTexState(bilinearTS));

    const Vec4 texSizeParam((float)tex->GetWidth(), (float)tex->GetHeight(), 0, 0);
    shader->FXSetVSFloat(texSizeName, &texSizeParam, 1);

    if (m_globalOcclusionBokeh)
    {
        ApplyOcclusionPattern(shader);
    }
    else
    {
        CTextureManager::Instance()->GetBlackTexture()->Apply(5, CTexture::GetTexState(bilinearTS));
    }

    shader->FXSetVSFloat(tileInfoName, &m_vTileDefinition, 1);
    ApplyCommonVSParams(shader, vSrcWorldPos, vSrcProjPos);

    float x = computeMovementLocationX(vSrcProjPos);
    float y = computeMovementLocationY(vSrcProjPos);

    rd->DrawQuad(x, y, x, y, m_globalColor, m_globalFlareBrightness);
    shader->FXEndPass();

    shader->FXEnd();
}


const int CMultipleGhost::MAX_COUNT = 1000;

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&CMultipleGhost::FUNC_NAME)
void  CMultipleGhost::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsElement::InitEditorParamGroups(groups);
    FuncVariableGroup ghostGroup;
    ghostGroup.SetName("MultiGhost", "Multi Ghost");
    ghostGroup.AddVariable(new OpticsMFPVariable(e_TEXTURE2D, "Texture", "The texture for lens ghosts", this, MFPtr(SetTexture), MFPtr(GetTexture)));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_INT, "Count", "The number of ghosts", this, MFPtr(SetCount), MFPtr(GetCount), 0, static_cast<float>(MAX_COUNT)));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_INT, "Random Seed", "The Seed of random generator", this, MFPtr(SetRandSeed), MFPtr(GetRandSeed), -255.0f, 255.0f));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_VEC2, "Scattering range", "The scattering range for sub ghosts", this, MFPtr(SetRange), MFPtr(GetRange)));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_VEC2, "position factor", "multiplier of position", this, MFPtr(SetPositionFactor), MFPtr(GetPositionFactor)));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_VEC2, "position offset", "offset of position", this, MFPtr(SetPositionOffset), MFPtr(GetPositionOffset)));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "X-axis noise", "the noise of scattering on x-axis", this, MFPtr(SetXOffsetNoise), MFPtr(GetXOffsetNoise)));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Y-axis noise", "the noise of scattering on y-axis", this, MFPtr(SetYOffsetNoise), MFPtr(GetYOffsetNoise)));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Size Variation", "The strength for size variation", this, MFPtr(SetSizeNoise), MFPtr(GetSizeNoise)));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Brightness Variation", "The strength for brightness variation", this, MFPtr(SetBrightnessNoise), MFPtr(GetBrightnessNoise)));
    ghostGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Color Variation", "The strength for color variation", this, MFPtr(SetColorNoise), MFPtr(GetColorNoise)));
    groups.push_back(ghostGroup);
}
#undef MFPtr
#endif

void CMultipleGhost::Load(IXmlNode* pNode)
{
    COpticsElement::Load(pNode);

    XmlNodeRef pMultiGhostNode = pNode->findChild("MultiGhost");

    if (pMultiGhostNode)
    {
        const char* textureName(NULL);
        if (pMultiGhostNode->getAttr("Texture", &textureName))
        {
            if (textureName && textureName[0])
            {
                ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(textureName);
                SetTexture((CTexture*)pTexture);
                if (pTexture)
                {
                    pTexture->Release();
                }
            }
        }

        int nCount(0);
        if (pMultiGhostNode->getAttr("Count", nCount))
        {
            SetCount(nCount);
        }

        int nRandSeed(0);
        if (pMultiGhostNode->getAttr("RandomSeed", nRandSeed))
        {
            SetRandSeed(nRandSeed);
        }

        Vec2 vRange(m_vRange);
        if (pMultiGhostNode->getAttr("Scatteringrange", vRange))
        {
            SetRange(vRange);
        }

        Vec2 vPositionFactor(m_vPositionFactor);
        if (pMultiGhostNode->getAttr("positionfactor", vPositionFactor))
        {
            SetPositionFactor(vPositionFactor);
        }

        Vec2 vPositionOffset(m_vPositionOffset);
        if (pMultiGhostNode->getAttr("positionoffset", vPositionOffset))
        {
            SetPositionOffset(vPositionOffset);
        }

        float xOffsetNoise(m_fXOffsetNoise);
        if (pMultiGhostNode->getAttr("X-axisnoise", xOffsetNoise))
        {
            SetXOffsetNoise(xOffsetNoise);
        }

        float yOffsetNoise(m_fYOffsetNoise);
        if (pMultiGhostNode->getAttr("Y-axisnoise", yOffsetNoise))
        {
            SetYOffsetNoise(yOffsetNoise);
        }

        float fSizeNoise(m_fSizeNoise);
        if (pMultiGhostNode->getAttr("SizeVariation", fSizeNoise))
        {
            SetSizeNoise(fSizeNoise);
        }

        float fBrightnessNoise(m_fBrightnessNoise);
        if (pMultiGhostNode->getAttr("BrightnessVariation", fBrightnessNoise))
        {
            SetBrightnessNoise(fBrightnessNoise);
        }

        float fColorNoise(m_fColorNoise);
        if (pMultiGhostNode->getAttr("ColorVariation", fColorNoise))
        {
            SetColorNoise(fColorNoise);
        }
    }
}

void CMultipleGhost::SetCount(int count)
{
    if (count < 0)
    {
        return;
    }
    m_nCount = min(count, MAX_COUNT);
    RemoveAll();
    for (int i = 0; i < m_nCount; i++)
    {
        CLensGhost* ghost = new CLensGhost("SubGhost");
        ghost->SetAutoRotation(true);
        ghost->SetAspectRatioCorrection(true);
        ghost->SetOccBokehEnabled(true);
        ghost->SetSensorSizeFactor(1);
        ghost->SetSensorBrightnessFactor(1);
        Add(ghost);
    }
    m_bContentDirty = true;
}

void CMultipleGhost::GenGhosts(SAuxParams& aux)
{
    stable_rand::setSeed(GetRandSeed());

    float rangeStart = m_vRange.x;
    float rangeEnd = m_vRange.y;
    float span = rangeEnd - rangeStart;
    float halfXNoiseRange = span / 2 * m_fXOffsetNoise;
    float halfYNoiseRange = span / 2 * m_fYOffsetNoise;
    if ((uint)GetCount() != children.size())
    {
        SetCount(GetCount());
    }
    for (uint i = 0; i < children.size(); i++)
    {
        CLensGhost* ghost = (CLensGhost*)&*(children[i]);
        ghost->SetTexture(GetTexture());
        Vec2 pos;
        float axisPos = stable_rand::randPositive() * span + rangeStart;
        pos.x = stable_rand::randUnit() * halfXNoiseRange + axisPos * m_vPositionFactor.x + m_vPositionOffset.x;
        pos.y = stable_rand::randUnit() * halfYNoiseRange + axisPos * m_vPositionFactor.y + m_vPositionOffset.y;
        ghost->SetMovement(pos);
        ghost->SetSize(stable_rand::randBias(GetSizeNoise()));
        ghost->SetBrightness(stable_rand::randBias(GetBrightnessNoise()));
        ColorF variation(stable_rand::randBias(GetColorNoise()), stable_rand::randBias(GetColorNoise()), stable_rand::randBias(GetColorNoise()));
        ghost->SetColor(variation);

        ghost->SetDynamicsEnabled(GetDynamicsEnabled());
        ghost->SetDynamicsInvert(GetDynamicsInvert());
        ghost->SetDynamicsOffset(GetDynamicsOffset());
        ghost->SetDynamicsRange(GetDynamicsRange());
    }
    validateChildrenGlobalVars(aux);
    m_bContentDirty = false;
}

void CMultipleGhost::Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux)
{
    if (!IsVisible())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("MultiGhost");

    if (m_bContentDirty)
    {
        GenGhosts(aux);
    }

    COpticsGroup::Render(shader, vSrcWorldPos, vSrcProjPos, aux);
}
