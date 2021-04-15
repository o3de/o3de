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
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"
#include "../Common/Textures/TextureManager.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Game/Hud specific post-processing
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_CustomRenderScene(bool bEnable)
{
    if (bEnable)
    {
        PostProcessUtils().Log(" +++ Begin custom render scene +++ \n");
        if IsCVarConstAccess(constexpr) ((CRenderer::CV_r_customvisions == 1) || (CRenderer::CV_r_customvisions == 3))
        {
            FX_ClearTarget(CTexture::s_ptexSceneNormalsMap, Clr_Transparent);
            FX_PushRenderTarget(0, CTexture::s_ptexSceneNormalsMap, &gcpRendD3D->m_DepthBufferOrig);
            RT_SetViewport(0, 0, GetWidth(), GetHeight());
        }

        m_RP.m_PersFlags2 |= RBPF2_CUSTOM_RENDER_PASS;
    }
    else
    {
        if IsCVarConstAccess(constexpr) ((CRenderer::CV_r_customvisions == 1) || (CRenderer::CV_r_customvisions == 3))
        {
            FX_PopRenderTarget(0);
        }

        FX_ResetPipe();

        PostProcessUtils().Log(" +++ End custom render scene +++ \n");

        RT_SetViewport(0, 0, GetWidth(), GetHeight());

        m_RP.m_PersFlags2 &= ~RBPF2_CUSTOM_RENDER_PASS;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CHudSilhouettes::Render()
{
    PROFILE_LABEL_SCOPE("HUD_SILHOUETTES");

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    float fBlendParam = clamp_tpl<float>(m_pAmount->GetParam(), 0.0f, 1.0f);
    float fType = m_pType->GetParam();

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Render highlighted geometry
    {
        PROFILE_LABEL_SCOPE("HUD_SILHOUETTES_ENTITIES_PASS");

        uint32 nPrevPers2 = gRenDev->m_RP.m_PersFlags2;
        gRenDev->m_RP.m_PersFlags2 &= ~RBPF2_NOPOSTAA;

        // render to texture all masks
        gcpRendD3D->FX_ProcessPostRenderLists(FB_CUSTOM_RENDER);

        gRenDev->m_RP.m_PersFlags2 = nPrevPers2;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Render silhouettes
    switch (static_cast<int>(CRenderer::CV_r_customvisions))
    {
    case 1:
    {
        RenderDeferredSilhouettes(fBlendParam, fType);
        break;
    }
    case 2:
    {
        // These are forward rendered so do nothing
        break;
    }
    case 3:
    {
        RenderDeferredSilhouettesOptimised(fBlendParam, fType);
    }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
void CHudSilhouettes::RenderDeferredSilhouettes(float fBlendParam, float fType)
{
    gcpRendD3D->RT_SetViewport(PostProcessUtils().m_pScreenRect.left, PostProcessUtils().m_pScreenRect.top, PostProcessUtils().m_pScreenRect.right, PostProcessUtils().m_pScreenRect.bottom);
    PostProcessUtils().m_pCurDepthSurface = &gcpRendD3D->m_DepthBufferOrig;

    CTexture* pMask = CTextureManager::Instance()->GetBlackTexture();
    CTexture* pMaskBlurred = CTextureManager::Instance()->GetBlackTexture();

    // skip processing, nothing was added to mask
    if ((SRendItem::BatchFlags(EFSLIST_GENERAL, gRenDev->m_RP.m_pRLD) | SRendItem::BatchFlags(EFSLIST_TRANSP, gRenDev->m_RP.m_pRLD)) & FB_CUSTOM_RENDER)
    {
        // could render directly to frame buffer ? save 1 resolve - no glow though
        {
            // store silhouettes/signature temporary render target, so that we can post process this afterwards
            gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexBackBufferScaled[0], NULL);
            gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexBackBufferScaled[0]->GetWidth(), CTexture::s_ptexBackBufferScaled[0]->GetHeight());

            static CCryNameTSCRC pTech1Name("BinocularView");
            PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            gRenDev->FX_SetState(GS_NODEPTHTEST);

            // Set VS params
            const float uvOffset = 2.0f;
            Vec4 vsParams = Vec4(uvOffset, 0.0f, 0.0f, 0.0f);
            CShaderMan::s_shPostEffectsGame->FXSetVSFloat(m_vsParamName, &vsParams, 1);

            // Set PS default params
            Vec4 pParams = Vec4(0, 0, 0, (!fType) ? 1.0f : 0.0f);
            CShaderMan::s_shPostEffectsGame->FXSetPSFloat(m_psParamName, &pParams, 1);

            PostProcessUtils().SetTexture(CTexture::s_ptexSceneNormalsMap, 0, FILTER_POINT);
            PostProcessUtils().SetTexture(CTexture::s_ptexZTarget, 1, FILTER_POINT);
            PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());

            PostProcessUtils().ShEndPass();
            gcpRendD3D->FX_PopRenderTarget(0);

            pMask = CTexture::s_ptexBackBufferScaled[0];
            pMaskBlurred = CTexture::s_ptexBackBufferScaled[1];
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // compute glow

        {
            GetUtils().StretchRect(CTexture::s_ptexBackBufferScaled[0], CTexture::s_ptexBackBufferScaled[1]);

            // blur - for glow
            //          GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[0], 1, 1.5f, 2.0f, false);
            GetUtils().TexBlurIterative(CTexture::s_ptexBackBufferScaled[1], 1, false);

            gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // finally add silhouettes to screen

        {
            static CCryNameTSCRC pTechSilhouettesName("BinocularViewSilhouettes");

            GetUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechSilhouettesName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            gRenDev->FX_SetState(GS_NODEPTHTEST  | GS_BLSRC_ONE | GS_BLDST_ONE);

            GetUtils().SetTexture(pMask, 0);
            GetUtils().SetTexture(pMaskBlurred, 1);

            // Set PS default params
            Vec4 pParams = Vec4(0, 0, 0, fBlendParam * 0.33f);
            static CCryNameR pParamName("psParams");
            CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &pParams, 1);

            GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0, &gcpRendD3D->m_FullResRect);

            GetUtils().ShEndPass();
        }
    }

    gcpRendD3D->RT_SetViewport(GetUtils().m_pScreenRect.left, GetUtils().m_pScreenRect.top, GetUtils().m_pScreenRect.right, GetUtils().m_pScreenRect.bottom);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
void CHudSilhouettes::RenderDeferredSilhouettesOptimised(float fBlendParam, float fType)
{
    const bool bHasSilhouettesToRender = ((SRendItem::BatchFlags(EFSLIST_GENERAL, gRenDev->m_RP.m_pRLD) |
                                           SRendItem::BatchFlags(EFSLIST_TRANSP, gRenDev->m_RP.m_pRLD))
                                          & FB_CUSTOM_RENDER) ? true : false;

    if (bHasSilhouettesToRender)
    {
        // Down Sample
        GetUtils().StretchRect(CTexture::s_ptexSceneNormalsMap, CTexture::s_ptexBackBufferScaled[0]);

        PROFILE_LABEL_SCOPE("HUD_SILHOUETTES_DEFERRED_PASS");

        // Draw silhouettes
        GetUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, m_deferredSilhouettesOptimisedTech, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        gRenDev->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE | GS_NOCOLMASK_A);

        GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 0, FILTER_LINEAR);

        // Set vs params
        const float uvOffset = 1.5f;
        Vec4 vsParams = Vec4(uvOffset, 0.0f, 0.0f, 0.0f);
        CShaderMan::s_shPostEffectsGame->FXSetVSFloat(m_vsParamName, &vsParams, 1);

        // Set ps params
        const float fillStrength = m_pFillStr->GetParam();
        const float silhouetteBoost = 1.7f;
        const float silhouetteBrightness = 1.333f;
        const float focusReduction = 0.33f;
        const float silhouetteAlpha = 0.8f;
        const bool bBinocularsActive = (!fType);
        const float silhouetteStrength = (bBinocularsActive ? 1.0f : (fBlendParam * focusReduction)) * silhouetteAlpha;

        Vec4 psParams;
        psParams.x = silhouetteStrength;
        psParams.y = silhouetteBoost;
        psParams.z = silhouetteBrightness;
        psParams.w = fillStrength;

        CShaderMan::s_shPostEffectsGame->FXSetPSFloat(m_psParamName, &psParams, 1);

        GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0, &gcpRendD3D->m_FullResRect);

        GetUtils().ShEndPass();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CAlienInterference::Render()
{
    PROFILE_LABEL_SCOPE("ALIEN_INTERFERENCE");

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    float fAmount = m_pAmount->GetParam();

    static CCryNameTSCRC pTechName("AlienInterference");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    Vec4 vParams = Vec4(1, 1, (float)PostProcessUtils().m_iFrameCounter, fAmount);
    static CCryNameR pParamName("psParams");
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

    static CCryNameR pParamAlienInterferenceName("AlienInterferenceTint");
    vParams = m_pTintColor->GetParamVec4();
    vParams.x *= 2.0f;
    vParams.y *= 2.0f;
    vParams.z *= 2.0f;
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamAlienInterferenceName, &vParams, 1);

    PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0.0f, &gcpRendD3D->m_FullResRect);



    PostProcessUtils().ShEndPass();
}

void CGhostVision::Render()
{
    PROFILE_LABEL_SCOPE("GHOST_VISION");

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    float fUserValue1 = m_pUserValue1->GetParam();
    float fUserValue2 = m_pUserValue2->GetParam();
    float fUserValue3 = m_pUserValue3->GetParam();

    static CCryNameTSCRC pTechName("GhostVision");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);

    //gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    Vec4 vParams = Vec4((float)PostProcessUtils().m_iFrameCounter, fUserValue1, fUserValue2, fUserValue3);
    static CCryNameR pParamName("psParams");
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

    static CCryNameR pParamGhostVisionName("GhostVisionTint");
    vParams = m_pTintColor->GetParamVec4();
    vParams.x *= 2.0f;
    vParams.y *= 2.0f;
    vParams.z *= 2.0f;
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamGhostVisionName, &vParams, 1);

    PostProcessUtils().SetTexture(m_pUserTex1, 0, FILTER_LINEAR, TADDR_CLAMP);
    PostProcessUtils().SetTexture(m_pUserTex1, 1, FILTER_LINEAR, TADDR_CLAMP);

    PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0.0f, &gcpRendD3D->m_FullResRect);
    PostProcessUtils().ShEndPass();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CScreenFrost::Render()
{
    float fAmount = m_pAmount->GetParam();

    if (fAmount <= 0.02f)
    {
        m_fRandOffset = cry_random(0.0f, 1.0f);
        return;
    }

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    float fCenterAmount = m_pCenterAmount->GetParam();

    PostProcessUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[1]);

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // display frost
    static CCryNameTSCRC pTechName("ScreenFrost");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    static CCryNameR pParam0Name("screenFrostParamsVS");
    static CCryNameR pParam1Name("screenFrostParamsPS");

    PostProcessUtils().ShSetParamVS(pParam0Name, Vec4(1, 1, 1, m_fRandOffset));
    PostProcessUtils().ShSetParamPS(pParam1Name, Vec4(1, 1, fCenterAmount, fAmount));

    PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

    PostProcessUtils().ShEndPass();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CFlashBang::Preprocess()
{
    float fActive = m_pActive->GetParam();
    if (fActive || m_fSpawnTime)
    {
        if (fActive)
        {
            m_fSpawnTime = 0.0f;
        }

        m_pActive->SetParam(0.0f);

        return true;
    }


    return false;
}

void CFlashBang::Render()
{
    float fTimeDuration = m_pTime->GetParam();
    float fDifractionAmount = m_pDifractionAmount->GetParam();
    float fBlindTime = m_pBlindAmount->GetParam();

    if (!m_fSpawnTime)
    {
        m_fSpawnTime = PostProcessUtils().m_pTimer->GetCurrTime();

        // Create temporary ghost image and capture screen
        SAFE_DELETE(m_pGhostImage);

        m_pGhostImage = new SDynTexture(CTexture::s_ptexBackBuffer->GetWidth() >> 1, CTexture::s_ptexBackBuffer->GetHeight() >> 1, eTF_R8G8B8A8, eTT_2D,  FT_STATE_CLAMP, "GhostImageTempRT");
        m_pGhostImage->Update(CTexture::s_ptexBackBuffer->GetWidth() >> 1, CTexture::s_ptexBackBuffer->GetHeight() >> 1);

        if (m_pGhostImage && m_pGhostImage->m_pTexture)
        {
            PostProcessUtils().StretchRect(CTexture::s_ptexBackBuffer, m_pGhostImage->m_pTexture);
        }
    }

    // Update current time
    float fCurrTime = (PostProcessUtils().m_pTimer->GetCurrTime() - m_fSpawnTime) / fTimeDuration;

    // Effect finished
    if (fCurrTime > 1.0f)
    {
        m_fSpawnTime = 0.0f;
        m_pActive->SetParam(0.0f);

        SAFE_DELETE(m_pGhostImage);

        return;
    }

    // make sure to update dynamic texture if required
    if (m_pGhostImage && !m_pGhostImage->m_pTexture)
    {
        m_pGhostImage->Update(CTexture::s_ptexBackBuffer->GetWidth() >> 1, CTexture::s_ptexBackBuffer->GetHeight() >> 1);
    }

    if (!m_pGhostImage || !m_pGhostImage->m_pTexture)
    {
        return;
    }

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    //////////////////////////////////////////////////////////////////////////////////////////////////
    static CCryNameTSCRC pTechName("FlashBang");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    float fLuminance = 1.0f  - fCurrTime;//PostProcessUtils().InterpolateCubic(0.0f, 1.0f, 0.0f, 1.0f, fCurrTime);

    // opt: some pre-computed constants
    Vec4 vParams = Vec4(fLuminance, fLuminance * fDifractionAmount, 3.0f * fLuminance * fBlindTime, fLuminance);
    static CCryNameR pParamName("vFlashBangParams");
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamName, &vParams, 1);

    PostProcessUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);
    PostProcessUtils().SetTexture(m_pGhostImage->m_pTexture, 1, FILTER_LINEAR);

    PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

    PostProcessUtils().ShEndPass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CFilterKillCamera::Render()
{
    PROFILE_LABEL_SCOPE("KILL_CAMERA");
    PROFILE_SHADER_SCOPE;

    // Update time
    float frameTime = PostProcessUtils().m_pTimer->GetFrameTime();
    m_blindTimer += frameTime;

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    float grainStrength = m_pGrainStrength->GetParam();
    Vec4 chromaShift = m_pChromaShift->GetParamVec4(); // xyz = offset, w = strength
    Vec4 vignette = m_pVignette->GetParamVec4(); // xy = screen scale, z = radius, w = blind noise vignette scale
    Vec4 colorScale = m_pColorScale->GetParamVec4();

    // Scale vignette with overscan borders
    Vec2 overscanBorders = Vec2(0.0f, 0.0f);
    gRenDev->EF_Query(EFQ_OverscanBorders, overscanBorders);
    const float vignetteOverscanMaxValue = 4.0f;
    const Vec2 vignetteOverscanScalar = Vec2(1.0f, 1.0f) + (overscanBorders * vignetteOverscanMaxValue);
    vignette.x *= vignetteOverscanScalar.x;
    vignette.y *= vignetteOverscanScalar.y;

    float inverseVignetteRadius = 1.0f / clamp_tpl<float>(vignette.z * 2.0f, 0.001f, 2.0f);
    Vec2 vignetteScreenScale(max(vignette.x, 0.0f), max(vignette.y, 0.0f));

    // Blindness
    Vec4 blindness = m_pBlindness->GetParamVec4(); // x = blind duration, y = blind fade out duration, z = blindness grey scale, w = blind noise min scale
    float blindDuration = max(blindness.x, 0.0f);
    float blindFadeOutDuration = max(blindness.y, 0.0f);
    float blindGreyScale = clamp_tpl<float>(blindness.z, 0.0f, 1.0f);
    float blindNoiseMinScale = clamp_tpl<float>(blindness.w, 0.0f, 10.0f);
    float blindNoiseVignetteScale = clamp_tpl<float>(vignette.w, 0.0f, 10.0f);

    float blindAmount = 0.0f;
    if (m_blindTimer < blindDuration)
    {
        blindAmount = 1.0f;
    }
    else
    {
        float blindFadeOutTimer = m_blindTimer - blindDuration;
        if (blindFadeOutTimer < blindFadeOutDuration)
        {
            blindAmount = 1.0f - (blindFadeOutTimer / blindFadeOutDuration);
        }
    }

    // Rendering
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, m_techName, FEF_DONTSETSTATES);

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    gcpRendD3D->GetViewport(&x, &y, &width, &height);

    // Set PS default params
    const int PARAM_COUNT = 4;
    Vec4 pParams[PARAM_COUNT];

    // psParams[0] - xy = Rand lookup, zw = vignetteScreenScale * invRadius
    pParams[0].x = cry_random(0, 1023) / (float)width;
    pParams[0].y = cry_random(0, 1023) / (float)height;
    pParams[0].z = vignetteScreenScale.x * inverseVignetteRadius;
    pParams[0].w = vignetteScreenScale.y * inverseVignetteRadius;

    // psParams[1] - xyz = color scale, w = grain strength
    pParams[1].x = colorScale.x;
    pParams[1].y = colorScale.y;
    pParams[1].z = colorScale.z;
    pParams[1].w = grainStrength;

    // psParams[2] - xyz = chroma shift, w = chroma shift color strength
    pParams[2] = chromaShift;

    // psParams[3] - x = blindAmount, y = blind grey scale, z = blindNoiseVignetteScale, w = blindNoiseMinScale
    pParams[3].x = blindAmount;
    pParams[3].y = blindGreyScale;
    pParams[3].z = blindNoiseVignetteScale;
    pParams[3].w = blindNoiseMinScale;

    CShaderMan::s_shPostEffects->FXSetPSFloat(m_paramName, pParams, PARAM_COUNT);

    PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

    PostProcessUtils().ShEndPass();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CScreenBlood::Render()
{
    PROFILE_LABEL_SCOPE("SCREEN BLOOD");

    static CCryNameTSCRC pTechName("ScreenBlood");
    GetUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETSTATES);
    gcpRendD3D->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_DSTCOL | GS_BLDST_SRCALPHA);

    // Border params
    const Vec4 borderParams = m_pBorder->GetParamVec4();
    const float borderRange = borderParams.z;
    const Vec2 borderOffset = Vec2(borderParams.x, borderParams.y);
    const float alpha = borderParams.w;

    // Use overscan borders to scale blood thickness around screen
    const float overscanScalar = 3.0f;
    Vec2 overscanBorders = Vec2(0.0f, 0.0f);
    gcpRendD3D->EF_Query(EFQ_OverscanBorders, overscanBorders);
    overscanBorders =  Vec2(1.0f, 1.0f) + ((overscanBorders + borderOffset) * overscanScalar);

    const float borderScale = max(0.2f, borderRange - (m_pAmount->GetParam() * borderRange));
    Vec4 pParams = Vec4(overscanBorders.x, overscanBorders.y, alpha, borderScale);
    static CCryNameR pParamName("psParams");
    CShaderMan::s_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);
    GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());

    GetUtils().ShEndPass();

    //m_nRenderFlags = PSP_UPDATE_BACKBUFFER;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void ScreenFader::Render()
{
    // CD3D9Renderer::Draw2dImage() is based off of the "virtual screen" dimensions, not the viewport, rendertarget or renderer dimensions.
    const unsigned int renderWidth = static_cast<decltype(renderWidth)>(VIRTUAL_SCREEN_WIDTH);
    const unsigned int renderHeight = static_cast<decltype(renderHeight)>(VIRTUAL_SCREEN_HEIGHT);

    // Render all of our Screen Fader passes in order.
    for ( auto passIter = m_screenPasses.begin(); passIter != m_screenPasses.end(); ++passIter )
    {
        ScreenFaderPass* pass = (*passIter);

        // Fading in goes 0 -> Duration, fading out goes Duration (or current time) -> 0
        if ( pass->m_fadingIn || pass->m_fadingOut )
        {
            pass->m_currentFadeTime += gEnv->pTimer->GetFrameTime() * pass->m_fadeDirection;
        }

        float currentAlpha = 1.0f;
        if ( pass->m_fadeDuration > 0.0f )
        {
            currentAlpha = pass->m_currentFadeTime / pass->m_fadeDuration;
            currentAlpha = CLAMP(currentAlpha, 0.0f, 1.0f) * pass->m_currentColor.a;
        }

        if ( currentAlpha > 0.001f )
        { 
            const unsigned int screenLeft = pass->m_screenCoordinates.x * renderWidth;
            const unsigned int screenTop = pass->m_screenCoordinates.y * renderHeight;
            const unsigned int screenWidth = (pass->m_screenCoordinates.z - pass->m_screenCoordinates.x) * renderWidth;
            const unsigned int screenHeight = (pass->m_screenCoordinates.w - pass->m_screenCoordinates.y) * renderHeight;

            int texId = (pass->m_fadeTexture == nullptr) ? -1 : pass->m_fadeTexture->GetTextureID();
            gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
            gEnv->pRenderer->Draw2dImage(screenLeft, screenTop, screenWidth, screenHeight,
                                        texId,
                                        0.0f, 1.0f, 1.0f, 0.0f, // tex coords
                                        0.0f, // angle
                                        pass->m_currentColor.r, pass->m_currentColor.g, pass->m_currentColor.b, currentAlpha,
                                        0.0f);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
