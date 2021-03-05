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

#include "MeshUtil.h"
#include "CameraOrbs.h"
#include "../Textures/Texture.h"
#include "../Textures/TextureManager.h"
#include "../../RenderDll/XRenderD3D9/DriverD3D.h"

class ScreenTile
    : public AbstractMeshElement
{
public:
    ScreenTile()
    {
        ValidateMesh();
    }
    void GenMesh()
    {
        const int rowCount = 15;
        const int colCount = 25;
        MeshUtil::GenScreenTile(-1, -1, 1, 1, ColorF(1, 1, 1, 1), rowCount, colCount,  m_vertBuf, m_idxBuf);
    }

    void Draw()
    {
        ApplyMesh();
        gcpRendD3D->FX_Commit();
        DrawMeshTriList();
    }
};

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&CameraOrbs::FUNC_NAME)
void  CameraOrbs::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsElement::InitEditorParamGroups(groups);

    FuncVariableGroup camGroup;
    camGroup.SetName("CameraOrbs", "Camera Orbs");
    camGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Illum range", "Illum range", this, MFPtr(SetIllumRange), MFPtr(GetIllumRange)));
    camGroup.AddVariable(new OpticsMFPVariable(e_TEXTURE2D, "Orb Texture", "The texture for orbs", this, MFPtr(SetOrbTex), MFPtr(GetOrbTex)));
    camGroup.AddVariable(new OpticsMFPVariable(e_TEXTURE2D, "Lens Texture", "The texture for lens", this, MFPtr(SetLensTex), MFPtr(GetLensTex)));
    camGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Enable lens texture", "Enable lens texture", this, MFPtr(SetUseLensTex), MFPtr(GetUseLensTex)));
    camGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Enable lens detail shading", "Enable lens detail shading", this, MFPtr(SetEnableLensDetailShading), MFPtr(GetEnableLensDetailShading)));
    camGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Lens texture strength", "Lens texture strength", this, MFPtr(SetLensTexStrength), MFPtr(GetLensTexStrength)));
    camGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Lens detail shading strength", "Lens detail shading strength", this, MFPtr(SetLensDetailShadingStrength), MFPtr(GetLensDetailShadingStrength)));
    camGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Lens detail bumpiness", "Lens detail bumpiness", this, MFPtr(SetLensDetailBumpiness), MFPtr(GetLensDetailBumpiness)));
    camGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Enable orb detail shading", "Enable orb detail shading", this, MFPtr(SetEnableOrbDetailShading), MFPtr(GetEnableOrbDetailShading)));
    groups.push_back(camGroup);

    FuncVariableGroup genGroup;
    genGroup.SetName("Generator");
    genGroup.AddVariable(new OpticsMFPVariable(e_INT, "Number of orbs", "Number of orbs", this, MFPtr(SetNumOrbs), MFPtr(GetNumOrbs), 0, 1000.0f));
    genGroup.AddVariable(new OpticsMFPVariable(e_INT, "Noise seed", "Noise seed", this, MFPtr(SetNoiseSeed), MFPtr(GetNoiseSeed), -255.0f, 255.0f));
    genGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Color variation", "Color variation", this, MFPtr(SetColorNoise), MFPtr(GetColorNoise)));
    genGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Size variation", "Size variation", this, MFPtr(SetSizeNoise), MFPtr(GetSizeNoise)));
    genGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Rotation variation", "Rotation variation", this, MFPtr(SetRotationNoise), MFPtr(GetRotationNoise)));
    groups.push_back(genGroup);

    FuncVariableGroup advShadingGroup;
    advShadingGroup.SetName("AdvancedShading", "Advanced Shading");
    advShadingGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "Enable adv shading", "Enable advanced shading mode", this, MFPtr(SetEnableAdvancdShading), MFPtr(GetEnableAdvancedShading)));
    advShadingGroup.AddVariable(new OpticsMFPVariable(e_COLOR, "Ambient Diffuse", "Ambient diffuse light (RGBK)", this, MFPtr(SetAmbientDiffuseRGBK), MFPtr(GetAmbientDiffuseRGBK)));
    advShadingGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Absorptance", "Absorptance of on-lens dirt", this, MFPtr(SetAbsorptance), MFPtr(GetAbsorptance)));
    advShadingGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Transparency", "Transparency of on-lens dirt", this, MFPtr(SetTransparency), MFPtr(GetTransparency)));
    advShadingGroup.AddVariable(new OpticsMFPVariable(e_FLOAT, "Scattering", "Subsurface Scattering of on-lens dirt", this, MFPtr(SetScatteringStrength), MFPtr(GetScatteringStrength)));
    groups.push_back(advShadingGroup);
}
#undef MFPtr
#endif

void CameraOrbs::Load(IXmlNode* pNode)
{
    COpticsElement::Load(pNode);

    XmlNodeRef pCameraOrbsNode = pNode->findChild("CameraOrbs");
    if (pCameraOrbsNode)
    {
        float fIllumRadius(m_fIllumRadius);
        if (pCameraOrbsNode->getAttr("Illumrange", fIllumRadius))
        {
            SetIllumRange(fIllumRadius);
        }

        const char* orbTextureName(NULL);
        if (pCameraOrbsNode->getAttr("OrbTexture", &orbTextureName))
        {
            if (orbTextureName && orbTextureName[0])
            {
                ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(orbTextureName);
                SetOrbTex((CTexture*)pTexture);
                if (pTexture)
                {
                    pTexture->Release();
                }
            }
        }

        const char* lensTextureName(NULL);
        if (pCameraOrbsNode->getAttr("LensTexture", &lensTextureName))
        {
            if (lensTextureName && lensTextureName[0])
            {
                ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(lensTextureName);
                SetLensTex((CTexture*)pTexture);
                if (pTexture)
                {
                    pTexture->Release();
                }
            }
        }

        bool bUseLensTex(m_bUseLensTex);
        if (pCameraOrbsNode->getAttr("Enablelenstexture", bUseLensTex))
        {
            SetUseLensTex(bUseLensTex);
        }

        bool bLensDetailShading(m_bLensDetailShading);
        if (pCameraOrbsNode->getAttr("Enablelensdetailshading", bLensDetailShading))
        {
            SetEnableLensDetailShading(bLensDetailShading);
        }

        float fLensTexStrength(m_fLensTexStrength);
        if (pCameraOrbsNode->getAttr("Lenstexturestrength", fLensTexStrength))
        {
            SetLensTexStrength(fLensTexStrength);
        }

        float fLensDetailShadingStrength(m_fLensDetailShadingStrength);
        if (pCameraOrbsNode->getAttr("Lensdetailshadingstrength", fLensDetailShadingStrength))
        {
            SetLensDetailShadingStrength(fLensDetailShadingStrength);
        }

        float fLensDetailBumpiness(m_fLensDetailBumpiness);
        if (pCameraOrbsNode->getAttr("Lensdetailbumpiness", fLensDetailBumpiness))
        {
            SetLensDetailBumpiness(fLensDetailBumpiness);
        }

        bool bOrbDetailShading(m_bOrbDetailShading);
        if (pCameraOrbsNode->getAttr("Enableorbdetailshading", bOrbDetailShading))
        {
            SetEnableOrbDetailShading(bOrbDetailShading);
        }
    }

    XmlNodeRef pGeneratorNode = pNode->findChild("Generator");
    if (pGeneratorNode)
    {
        int numOfOrbs(m_nNumOrbs);
        if (pGeneratorNode->getAttr("Numberoforbs", numOfOrbs))
        {
            SetNumOrbs(numOfOrbs);
        }

        int nNoiseSeed(m_iNoiseSeed);
        if (pGeneratorNode->getAttr("Noiseseed", nNoiseSeed))
        {
            SetNoiseSeed(nNoiseSeed);
        }

        float fColorNoise(m_fClrNoise);
        if (pGeneratorNode->getAttr("Colorvariation", fColorNoise))
        {
            SetColorNoise(fColorNoise);
        }

        float fSizeNoise(m_fSizeNoise);
        if (pGeneratorNode->getAttr("Sizevariation", fSizeNoise))
        {
            SetSizeNoise(fSizeNoise);
        }

        float fRotNoise(m_fRotNoise);
        if (pGeneratorNode->getAttr("Rotationvariation", fRotNoise))
        {
            SetRotationNoise(fRotNoise);
        }
    }

    XmlNodeRef pAdvancedShading = pNode->findChild("AdvancedShading");
    if (pAdvancedShading)
    {
        bool bAdvancedShading(m_bAdvancedShading);
        if (pAdvancedShading->getAttr("Enableadvshading", bAdvancedShading))
        {
            SetEnableAdvancdShading(bAdvancedShading);
        }

        Vec3 vColor(m_cAmbientDiffuse.r, m_cAmbientDiffuse.g, m_cAmbientDiffuse.b);
        int nAlpha((int)(m_cAmbientDiffuse.a * 255.0f));
        if (pAdvancedShading->getAttr("AmbientDiffuse", vColor) && pAdvancedShading->getAttr("AmbientDiffuse.alpha", nAlpha))
        {
            SetAmbientDiffuseRGBK(ColorF(vColor.x, vColor.y, vColor.z, (float)nAlpha / 255.0f));
        }

        float fAbsorptance(m_fAbsorptance);
        if (pAdvancedShading->getAttr("Absorptance", fAbsorptance))
        {
            SetAbsorptance(fAbsorptance);
        }

        float fTransparency(m_fTransparency);
        if (pAdvancedShading->getAttr("Transparency", fTransparency))
        {
            SetTransparency(fTransparency);
        }

        float fScatteringStrength(m_fScatteringStrength);
        if (pAdvancedShading->getAttr("Scattering", fScatteringStrength))
        {
            SetScatteringStrength(fScatteringStrength);
        }
    }
}

void CameraOrbs::GenMesh()
{
    ScatterOrbs();
    int iTempX, iTempY, iWidth, iHeight;
    gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
    MeshUtil::GenSprites(m_OrbsList, iWidth / (float)iHeight, true, m_vertBuf, m_idxBuf);
    MeshUtil::TrianglizeQuadIndices(m_OrbsList.size(), m_idxBuf);
}

void CameraOrbs::ScatterOrbs()
{
    stable_rand::setSeed(m_iNoiseSeed);
    for (uint32 i = 0; i < m_OrbsList.size(); i++)
    {
        SpritePoint& sprite = m_OrbsList[i];

        Vec2& p = sprite.pos;
        p.x = stable_rand::randUnit();
        p.y = stable_rand::randUnit();

        sprite.rotation = m_fRotation * stable_rand::randBias(GetRotationNoise()) * 2 * PI;
        sprite.size = m_globalSize * stable_rand::randBias(GetSizeNoise());
        sprite.brightness = m_globalFlareBrightness * stable_rand::randBias(GetBrightnessNoise());

        ColorF& clr = sprite.color;
        ColorF variation(stable_rand::randBias(m_fClrNoise), stable_rand::randBias(m_fClrNoise), stable_rand::randBias(m_fClrNoise));
        clr = variation;

        float clrMax = clr.Max();
        clr /= clrMax;
        clr.a = m_globalColor.a;
    }
}

CTexture* CameraOrbs::GetOrbTex()
{
    if (!m_pOrbTex)
    {
        m_pOrbTex = CTexture::ForName("EngineAssets/Textures/flares/orb_01.tif",  FT_DONT_STREAM, eTF_Unknown);
    }
    return m_pOrbTex;
}

CTexture* CameraOrbs::GetLensTex()
{
    if (!m_pLensTex)
    {
        m_pLensTex = CTexture::ForName("EngineAssets/Textures/flares/lens_dirtyglass.tif", FT_DONT_STREAM, eTF_Unknown);
    }

    return m_pLensTex;
}

void CameraOrbs::ApplyOrbFlags([[maybe_unused]] CShader* shader, bool detailShading) const
{
    if (detailShading)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
    }
}

void CameraOrbs::ApplyLensDetailParams(CShader* shader, float texStength, float detailStrength, float bumpiness) const
{
    static CCryNameR lensDetailName("lensDetailParams");
    const Vec4 lensDetailParam(texStength, detailStrength, bumpiness, 0);
    shader->FXSetPSFloat(lensDetailName, &lensDetailParam, 1);
}

void CameraOrbs::ApplyAdvancedShadingFlag([[maybe_unused]] CShader* shader) const
{
    if (m_bAdvancedShading)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    }
}

void CameraOrbs::ApplyAdvancedShadingParams(CShader* shader, const ColorF& ambDiffuseRGBK, float absorptance, float transparency, float scattering) const
{
    static CCryNameR ambDiffuseRGBKName("ambientDiffuseRGBK");
    static CCryNameR advShadingName("advShadingParams");

    static STexState pointTS(FILTER_POINT, true);
    CTexture* pAmbTex = CTexture::s_ptexSceneTarget;
    pAmbTex->Apply(1, CTexture::GetTexState(pointTS));

    const Vec4 ambDiffuseRGBKParam(ambDiffuseRGBK.r, ambDiffuseRGBK.g, ambDiffuseRGBK.b, ambDiffuseRGBK.a);
    const Vec4 advShadingParam(absorptance, transparency, scattering, 0);
    shader->FXSetPSFloat(ambDiffuseRGBKName, &ambDiffuseRGBKParam, 1);
    shader->FXSetPSFloat(advShadingName, &advShadingParam, 1);
}

void CameraOrbs::Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux)
{
    static ScreenTile screenTile;

    if (!IsVisible())
    {
        return;
    }

    PROFILE_LABEL_SCOPE("CameraOrbs");

    gRenDev->m_RP.m_FlagsShader_RT = 0;

    static CCryNameTSCRC pCameraOrbsTechName("CameraOrbs");
    static CCryNameR lightColorName("lightColorInfo");

    static STexState bilinearTS(FILTER_LINEAR, true);
    bilinearTS.SetBorderColor(0);
    bilinearTS.SetClampMode(TADDR_BORDER, TADDR_BORDER, TADDR_BORDER);

    vSrcProjPos = computeOrbitPos(vSrcProjPos, m_globalOrbitAngle);

    shader->FXSetTechnique(pCameraOrbsTechName);

    uint nPass;
    shader->FXBegin(&nPass, FEF_DONTSETTEXTURES);

    ApplyGeneralFlags(shader);
    ApplyAdvancedShadingFlag(shader);
    ApplyOcclusionBokehFlag(shader);
    ApplyOrbFlags(shader, m_bOrbDetailShading);

    shader->FXBeginPass(0);

    const float x = computeMovementLocationX(vSrcProjPos);
    const float y = computeMovementLocationY(vSrcProjPos);
    ApplyCommonVSParams(shader, vSrcWorldPos, vSrcProjPos);
    ApplyVSParam_LightProjPos(shader, Vec3(x, y, aux.linearDepth));

    const ColorF lightColor = m_globalFlareBrightness * m_globalColor * m_globalColor.a;
    const Vec4 lightColorParam(lightColor.r, lightColor.g, lightColor.b, m_fIllumRadius);
    shader->FXSetVSFloat(lightColorName, &lightColorParam, 1);

    ApplyLensDetailParams(shader,  1, 1, GetLensDetailBumpiness());
    if (m_globalOcclusionBokeh)
    {
        ApplyOcclusionPattern(shader);
    }
    else
    {
       CTextureManager::Instance()->GetBlackTexture()->Apply(5, CTexture::GetTexState(bilinearTS));
    }

    if (m_bAdvancedShading)
    {
        ApplyAdvancedShadingParams(shader, GetAmbientDiffuseRGBK(), GetAbsorptance(), GetTransparency(), GetScatteringStrength());
    }

    CTexture* pOrbTex = GetOrbTex() ? GetOrbTex() : CTextureManager::Instance()->GetBlackTexture();
    pOrbTex->Apply(0, CTexture::GetTexState(bilinearTS));
    ValidateMesh();
    ApplyMesh();
    DrawMeshTriList();
    shader->FXEndPass();

    if (m_bUseLensTex)
    {
        gRenDev->m_RP.m_FlagsShader_RT = 0;
        ApplyOrbFlags(shader, m_bLensDetailShading);
        shader->FXBeginPass(1);
        if (m_bAdvancedShading)
        {
            ApplyAdvancedShadingParams(shader, GetAmbientDiffuseRGBK(), GetAbsorptance(), GetTransparency(), GetScatteringStrength());
        }
        ApplyLensDetailParams(shader,  GetLensTexStrength(), GetLensDetailShadingStrength(), GetLensDetailBumpiness());
        CTexture* pLensTex = GetLensTex() ? GetLensTex() : CTextureManager::Instance()->GetBlackTexture();
        pLensTex->Apply(2, CTexture::GetTexState(bilinearTS));
        screenTile.Draw();
        shader->FXEndPass();
    }

    shader->FXEnd();
}
