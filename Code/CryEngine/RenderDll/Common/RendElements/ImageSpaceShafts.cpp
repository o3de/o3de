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
#include "ImageSpaceShafts.h"
#include "FlareSoftOcclusionQuery.h"
#include "../CryNameR.h"
#include "Common/Textures/TextureManager.h"
#include "../../RenderDll/XRenderD3D9/DriverD3D.h"

CTexture* ImageSpaceShafts::m_pOccBuffer = NULL;
CTexture* ImageSpaceShafts::m_pDraftBuffer = NULL;

#if defined(FLARES_SUPPORT_EDITING)
#define MFPtr(FUNC_NAME) (Optics_MFPtr)(&ImageSpaceShafts::FUNC_NAME)
void ImageSpaceShafts::InitEditorParamGroups(AZStd::vector<FuncVariableGroup>& groups)
{
    COpticsElement::InitEditorParamGroups(groups);
    FuncVariableGroup isShaftsGroup;
    isShaftsGroup.SetName("ImageSpaceShafts", "Image Space Shafts");
    isShaftsGroup.AddVariable(new OpticsMFPVariable(e_BOOL, "High Quality", "Enable High quality mode", this, MFPtr(SetHighQualityMode), MFPtr(IsHighQualityMode)));
    isShaftsGroup.AddVariable(new OpticsMFPVariable(e_TEXTURE2D, "Gobo Tex", "Gobo texture", this, MFPtr(SetGoboTex), MFPtr(GetGoboTex)));
    groups.push_back(isShaftsGroup);
}
#undef MFPtr
#endif

void ImageSpaceShafts::Load(IXmlNode* pNode)
{
    COpticsElement::Load(pNode);

    XmlNodeRef pImageSpaceNode = pNode->findChild("ImageSpaceShafts");
    if (pImageSpaceNode)
    {
        bool bHighQualityMode(m_bHighQualityMode);
        if (pImageSpaceNode->getAttr("HighQuality", bHighQualityMode))
        {
            SetHighQualityMode(bHighQualityMode);
        }
        else
        {
            assert(0);
        }

        const char* pGoboTexName = NULL;
        if (pImageSpaceNode->getAttr("GoboTex", &pGoboTexName))
        {
            if (pGoboTexName && pGoboTexName[0])
            {
                ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(pGoboTexName);
                SetGoboTex((CTexture*)pTexture);
                //Release this reference because we're no longer going to reference the texture from this pointer
                if (pTexture)
                {
                    pTexture->Release();
                }
            }
        }
        else
        {
            assert(0);
        }
    }
    else
    {
        assert(0);
    }
}

CTexture* ImageSpaceShafts::GetGoboTex() 
{
    if (!m_pGoboTex)
    {
        m_pGoboTex = CTexture::ForName("EngineAssets/Textures/flares/iris_shaft.tif", FT_DONT_STREAM, eTF_Unknown);
    }
    return m_pGoboTex;
}

void ImageSpaceShafts::InitTextures()
{
    float occBufRatio, occDraftRatio, occFinalRatio;
    if (m_bHighQualityMode && CRenderer::CV_r_flareHqShafts)
    {
        occBufRatio = 0.5f;
        occDraftRatio = 0.5f;
        occFinalRatio = 0.5f;
    }
    else
    {
        occBufRatio = 0.25f;
        occDraftRatio = 0.4f;
        occFinalRatio = 0.45f;
    }

    int w = gcpRendD3D->GetWidth();
    int h = gcpRendD3D->GetHeight();
    int flag = FT_DONT_RELEASE | FT_DONT_STREAM;
    m_pOccBuffer = CTexture::CreateRenderTarget("$ImageSpaceShaftsOccBuffer", (int)(w * occBufRatio), (int)(h * occBufRatio), Clr_Empty, eTT_2D, flag, eTF_R8G8B8A8);

    ETEX_Format draftTexFormat(eTF_R16G16B16A16);

    m_pDraftBuffer = CTexture::CreateRenderTarget("$ImageSpaceShaftsDraftBuffer", (int)(w * occDraftRatio), (int)(h * occDraftRatio), Clr_Empty, eTT_2D, flag, draftTexFormat);

    m_bTexDirty = false;
}

static STexState s_isPointTS(FILTER_POINT, true);
static STexState s_isBilinearTS(FILTER_BILINEAR, true);

void ImageSpaceShafts::Render(CShader* shader, Vec3 vSrcWorldPos, Vec3 vSrcProjPos, SAuxParams& aux)
{
    if (!IsVisible())
    {
        return;
    }

    CD3D9Renderer* rd = gcpRendD3D;

    if (!m_pOccBuffer || m_bTexDirty)
    {
        InitTextures();
    }

    PROFILE_LABEL_SCOPE("ImagesSpaceShafts");

    vSrcProjPos = computeOrbitPos(vSrcProjPos, m_globalOrbitAngle);

    static CCryNameTSCRC pImageSpaceShaftsTechName("ImageSpaceShafts");
    shader->FXSetTechnique(pImageSpaceShaftsTechName);
    uint nPass;
    shader->FXBegin(&nPass, FEF_DONTSETTEXTURES);
    static CCryNameR texSizeName("baseTexSize");

    Vec4 texSizeParam(1, 1, 0, 0);

    {
        PROFILE_LABEL_SCOPE("ImagesSpaceShafts-Occ");

        gRenDev->m_RP.m_FlagsShader_RT = 0;

        rd->FX_ClearTarget(m_pOccBuffer, Clr_Empty);
        rd->FX_PushRenderTarget(0, m_pOccBuffer, NULL, -1, false);

        ApplyGeneralFlags(shader);
        shader->FXBeginPass(0);

        CTexture* pGoboTex = ((CTexture*)m_pGoboTex) ? ((CTexture*)m_pGoboTex) : CTextureManager::Instance()->GetBlackTexture();
        pGoboTex->Apply(0, CTexture::GetTexState(s_isBilinearTS));
        CTexture::s_ptexZTargetScaled->Apply(1, CTexture::GetTexState(s_isBilinearTS));

        ApplyCommonVSParams(shader, vSrcWorldPos, vSrcProjPos);
        shader->FXSetVSFloat(texSizeName, &texSizeParam, 1);

        rd->DrawQuad(vSrcProjPos.x, vSrcProjPos.y, vSrcProjPos.x, vSrcProjPos.y, m_globalColor, aux.linearDepth);
        shader->FXEndPass();

        rd->FX_PopRenderTarget(0);
    }

    {
        PROFILE_LABEL_SCOPE("ImagesSpaceShafts-Gen");

        gRenDev->m_RP.m_FlagsShader_RT = 0;

        rd->FX_ClearTarget(m_pDraftBuffer, Clr_Empty);
        rd->FX_PushRenderTarget(0, m_pDraftBuffer, NULL, -1, false);

        ApplyGeneralFlags(shader);
        shader->FXBeginPass(1);

        ApplyCommonVSParams(shader, vSrcWorldPos, vSrcProjPos);
        shader->FXSetVSFloat(texSizeName, &texSizeParam, 1);
        m_pOccBuffer->Apply(1,  CTexture::GetTexState(s_isBilinearTS));

        rd->DrawQuad(vSrcProjPos.x, vSrcProjPos.y, vSrcProjPos.x, vSrcProjPos.y, m_globalColor, m_globalShaftBrightness);
        shader->FXEndPass();

        rd->FX_PopRenderTarget(0);
    }

    {
        PROFILE_LABEL_SCOPE("ImagesSpaceShafts-BLEND");

        gRenDev->m_RP.m_FlagsShader_RT = 0;

        m_pDraftBuffer->Apply(1,  CTexture::GetTexState(s_isBilinearTS));

        shader->FXBeginPass(2);
        rd->DrawQuad(0, 0, 0, 0, m_globalColor, m_globalShaftBrightness);
        shader->FXEndPass();
    }

    shader->FXEnd();
}
