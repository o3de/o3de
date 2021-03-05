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

#include <AzCore/std/algorithm.h>


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

const char* CRainDrops::GetName() const
{
    return "RainDrops";
}

bool CRainDrops::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);
    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_RainDropsEffect == 0)
    {
        return false;
    }

    bool bRainActive = IsActiveRain();

    if (m_bFirstFrame)
    { //initialize with valid value on 1st frame
        m_pPrevView = PostProcessUtils().m_pView;
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_RainDropsEffect > 2)
    {
        return true;
    }

    return bRainActive;
}


bool CRainDrops::IsActiveRain()
{
    static float s_fLastSpawnTime = -1.0f;

    if (m_pAmount->GetParam() > 0.09f || m_nAliveDrops)
    {
        s_fLastSpawnTime = 0.0f;
        return true;
    }

    if (s_fLastSpawnTime == 0.0f)
    {
        s_fLastSpawnTime = PostProcessUtils().m_pTimer->GetCurrTime();
    }

    if (fabs(PostProcessUtils().m_pTimer->GetCurrTime() - s_fLastSpawnTime) < 1.0f)
    {
        return true;
    }
    return false;
}

void CRainDrops::SpawnParticle(SRainDrop*& pParticle, int iRTWidth, int iRTHeight)
{
    static SRainDrop pNewDrop;

    static float fLastSpawnTime = 0.0f;

    float fUserSize = 5.0f;//m_pSize->GetParam();
    float fUserSizeVar = 2.5f;//m_pSizeVar->GetParam();

    if (cry_random(0.0f, 1.0f) > 0.5f && fabs(PostProcessUtils().m_pTimer->GetCurrTime() - fLastSpawnTime) > m_pSpawnTimeDistance->GetParam())
    {
        pParticle->m_pPos.x = cry_random(0.0f, 1.0f);
        pParticle->m_pPos.y = cry_random(0.0f, 1.0f);

        pParticle->m_fLifeTime = pNewDrop.m_fLifeTime + pNewDrop.m_fLifeTimeVar * cry_random(-1.0f, 1.0f);
        //pParticle->m_fSize = (pNewDrop.m_fSize + pNewDrop.m_fSizeVar * cry_random(-1.0f, 1.0f)) * 2.5f;
        //pNewDrop.m_fSize + pNewDrop.m_fSizeVar
        pParticle->m_fSize = 1.0f / (10.0f * (fUserSize  + 0.5f * fUserSizeVar * cry_random(-1.0f, 1.0f)));

        pParticle->m_pPos.x -= pParticle->m_fSize  / (float)iRTWidth;
        pParticle->m_pPos.y -= pParticle->m_fSize  / (float)iRTHeight;

        pParticle->m_fSpawnTime = PostProcessUtils().m_pTimer->GetCurrTime();
        pParticle->m_fWeight = 0.0f; // default weight to force rain drop to be stopped for a while

        fLastSpawnTime = PostProcessUtils().m_pTimer->GetCurrTime();
    }
    else
    {
        pParticle->m_fSize = 0.0f;
    }
}

void CRainDrops::UpdateParticles(int iRTWidth, int iRTHeight)
{
    SRainDropsItor pItor, pItorEnd = m_pDropsLst.end();

    // Store camera parameters
    Vec3 vz = gcpRendD3D->GetViewParameters().vZ;  // front vec
    float fDot = vz.Dot(Vec3(0, 0, -1.0f));
    float fGravity = 1.0f - fabs(fDot);

    float fCurrFrameTime = 10.0f * gEnv->pTimer->GetFrameTime();

    bool bAllowSpawn(m_pAmount->GetParam() > 0.005f);

    m_nAliveDrops = 0;
    static int s_nPrevAliveDrops = 0;

    for (pItor = m_pDropsLst.begin(); pItor != pItorEnd; ++pItor)
    {
        SRainDrop* pCurr = (*pItor);

        float fCurrLifeTime = (PostProcessUtils().m_pTimer->GetCurrTime() - pCurr->m_fSpawnTime) / pCurr->m_fLifeTime;

        // particle died, spawn new
        if (fabs(fCurrLifeTime) > 1.0f || pCurr->m_fSize < 0.01f)
        {
            if (bAllowSpawn)
            {
                SpawnParticle(pCurr, iRTWidth, iRTHeight);
            }
            else
            {
                pCurr->m_fSize = 0.0f;
                continue;
            }
        }

        m_nAliveDrops++;

        // update position, etc

        // add gravity
        pCurr->m_pPos.y += m_pVelocityProj.y * cry_random(-0.2f, 1.0f);
        pCurr->m_pPos.y += fCurrFrameTime * fGravity * AZStd::GetMin(pCurr->m_fWeight, 0.5f * pCurr->m_fSize);
        // random horizontal movement and size + camera horizontal velocity
        pCurr->m_pPos.x += m_pVelocityProj.x * cry_random(-0.2f, 1.0f);
        pCurr->m_pPos.x += fCurrFrameTime * AZStd::GetMin(pCurr->m_fWeight, 0.25f * pCurr->m_fSize) * fGravity * cry_random(-1.0f, 1.0f);

        // Increase/decrease weight randomly
        pCurr->m_fWeight = clamp_tpl<float>(pCurr->m_fWeight + fCurrFrameTime * pCurr->m_fWeightVar * cry_random(-4.0f, 4.0f), 0.0f, 1.0f);
    }

    if (s_nPrevAliveDrops == 0  &&  m_nAliveDrops > 0)
    {
        m_bFirstFrame = true;
    }

    s_nPrevAliveDrops = m_nAliveDrops;
}

Matrix44 CRainDrops::ComputeCurrentView(int iViewportWidth, int iViewportHeight)
{
    Matrix44 pCurrView(PostProcessUtils().m_pView);
    Matrix44 pCurrProj(PostProcessUtils().m_pProj);

    Matrix33 pLerpedView;
    pLerpedView.SetIdentity();

    float fCurrFrameTime = gEnv->pTimer->GetFrameTime();
    // scale down speed a bit
    float fAlpha = iszero (fCurrFrameTime) ? 0.0f : .0005f / (fCurrFrameTime);

    // Interpolate matrixes and position
    pLerpedView = Matrix33(pCurrView) * (1 - fAlpha) + Matrix33(m_pPrevView) * fAlpha;

    Vec3 pLerpedPos = Vec3::CreateLerp(pCurrView.GetRow(3), m_pPrevView.GetRow(3), fAlpha);

    // Compose final 'previous' viewProjection matrix
    Matrix44 pLView = pLerpedView;
    pLView.m30 = pLerpedPos.x;
    pLView.m31 = pLerpedPos.y;
    pLView.m32 = pLerpedPos.z;

    m_pViewProjPrev = pLView * pCurrProj;
    m_pViewProjPrev.Transpose();

    // Compute camera velocity vector
    Vec3 vz = gcpRendD3D->GetViewParameters().vZ;    // front vec
    Vec3 vMoveDir = gcpRendD3D->GetViewParameters().vOrigin - vz;
    Vec4 vCurrPos = Vec4(vMoveDir.x, vMoveDir.y, vMoveDir.z, 1.0f);

    Matrix44 pViewProjCurr (PostProcessUtils().m_pViewProj);

    Vec4 pProjCurrPos = pViewProjCurr * vCurrPos;

    pProjCurrPos.x = ((pProjCurrPos.x + pProjCurrPos.w) * 0.5f + (1.0f / (float) iViewportWidth) * pProjCurrPos.w) / pProjCurrPos.w;
    pProjCurrPos.y = ((pProjCurrPos.w - pProjCurrPos.y) * 0.5f + (1.0f / (float) iViewportHeight) * pProjCurrPos.w) / pProjCurrPos.w;

    Vec4 pProjPrevPos = m_pViewProjPrev * vCurrPos;

    pProjPrevPos.x = ((pProjPrevPos.x + pProjPrevPos.w) * 0.5f + (1.0f / (float) iViewportWidth) * pProjPrevPos.w) / pProjPrevPos.w;
    pProjPrevPos.y = ((pProjPrevPos.w - pProjPrevPos.y) * 0.5f + (1.0f / (float) iViewportHeight) * pProjPrevPos.w) / pProjPrevPos.w;

    m_pVelocityProj = Vec3(pProjCurrPos.x - pProjPrevPos.x, pProjCurrPos.y - pProjPrevPos.y, 0);

    return pCurrView;
}

void CRainDrops::Render()
{
    PROFILE_LABEL_SCOPE("RAIN_DROPS");

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    int iViewportX, iViewportY, iViewportWidth, iViewportHeight;
    gcpRendD3D->GetViewport(&iViewportX, &iViewportY, &iViewportWidth, &iViewportHeight);

    Matrix44 pCurrView = ComputeCurrentView(iViewportWidth, iViewportHeight);

    uint16    uPrevDytex  = (m_uCurrentDytex + 1) % 2;
    CTexture*&    rpPrevDytex = CTexture::s_ptexRainDropsRT[uPrevDytex];
    assert(rpPrevDytex);
    CTexture*&    rpCurrDytex = CTexture::s_ptexRainDropsRT[m_uCurrentDytex];
    assert(rpCurrDytex);
    int       iRTWidth    = rpCurrDytex->GetWidth();
    int       iRTHeight   = rpCurrDytex->GetHeight();

    UpdateParticles(iRTWidth, iRTHeight);

    gcpRendD3D->FX_PushRenderTarget(0, rpCurrDytex, NULL);
    gcpRendD3D->RT_SetViewport(0, 0, iRTWidth, iRTHeight);
    {
        ApplyExtinction(rpPrevDytex, iViewportWidth, iViewportHeight, iRTWidth, iRTHeight);
        DrawRaindrops(iViewportWidth, iViewportHeight, iRTWidth, iRTHeight);
    }

    gcpRendD3D->FX_PopRenderTarget(0);

    gcpRendD3D->RT_SetViewport(iViewportX, iViewportY, iViewportWidth, iViewportHeight);

    DrawFinal(rpCurrDytex);

    m_uCurrentDytex = (m_uCurrentDytex + 1) % 2;

    // store previous frame data
    m_pPrevView =  pCurrView;
    m_bFirstFrame = false;
}

void CRainDrops::DrawRaindrops([[maybe_unused]] int iViewportWidth, [[maybe_unused]] int iViewportHeight, int iRTWidth, int iRTHeight)
{
    PROFILE_LABEL_SCOPE("RAIN_DROPS_RAINDROPS");

    float fScreenW = iRTWidth;
    float fScreenH = iRTHeight;

    float fInvScreenW = 1.0f / fScreenW;
    float fInvScreenH = 1.0f / fScreenH;

    TransformationMatrices backupSceneMatrices;

    gcpRendD3D->Set2DMode(iRTWidth, iRTHeight, backupSceneMatrices);

    //clear and set render flags
    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]);

    //if( fDot >= - 0.25)
    {
        // render particles into effects rain map
        static CCryNameTSCRC pTech0Name("RainDropsGen");

        PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTech0Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        gcpRendD3D->FX_SetState(GS_BLSRC_ONE | GS_BLDST_ONE |  GS_NODEPTHTEST); //additive

        // override blend operation from additive to max (max(src,dst) gets written)
        SStateBlend bl = gcpRendD3D->m_StatesBL[gcpRendD3D->m_nCurStateBL];
        bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MAX;
        bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
        gcpRendD3D->SetBlendState(&bl);

        SRainDropsItor pItor, pItorEnd = m_pDropsLst.end();
        for (pItor = m_pDropsLst.begin(); pItor != pItorEnd; ++pItor)
        {
            SRainDrop* pCurr = (*pItor);

            if (pCurr->m_fSize < 0.01f)
            {
                continue;
            }

            // render a sprite
            float x0 = pCurr->m_pPos.x * fScreenW;
            float y0 = pCurr->m_pPos.y * fScreenH;

            float x1 = (pCurr->m_pPos.x + pCurr->m_fSize * (fScreenH / fScreenW)) * fScreenW;
            float y1 = (pCurr->m_pPos.y + pCurr->m_fSize) * fScreenH;

            float fCurrLifeTime = (PostProcessUtils().m_pTimer->GetCurrTime() - pCurr->m_fSpawnTime) / pCurr->m_fLifeTime;
            Vec4 vRainParams = Vec4(1.0f, 1.0f, 1.0f, 1.0f - fCurrLifeTime);

            static CCryNameR pParam0Name("vRainParams");
            CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParam0Name, &vRainParams, 1);
            PostProcessUtils().DrawScreenQuad(256, 256, x0, y0, x1, y1);
        }

        // restore previous blend operation
        bl.Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bl.Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        gcpRendD3D->SetBlendState(&bl);

        PostProcessUtils().ShEndPass();
    }

    gcpRendD3D->Unset2DMode(backupSceneMatrices);

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

void CRainDrops::ApplyExtinction(CTexture*& rptexPrevRT, int iViewportWidth, int iViewportHeight, int iRTWidth, int iRTHeight)
{
    //restore black texture to backbuffer for cleaning
    CTexture*   pTex = CTextureManager::Instance()->GetBlackTexture();
    PostProcessUtils().CopyTextureToScreen(pTex);

    PROFILE_LABEL_SCOPE("RAIN_DROPS_EXTINCTION");
    if (!m_bFirstFrame)
    {
        // Store camera parameters
        Vec3 vx = gcpRendD3D->GetViewParameters().vX;    // up vec
        Vec3 vy = gcpRendD3D->GetViewParameters().vY;    // right vec
        Vec3 vz = gcpRendD3D->GetViewParameters().vZ;    // front vec
        float fDot = vz.Dot(Vec3(0, 0, -1.0f));
        float fGravity = (1.0f - fabs(fDot));
        float fFrameScale = 4.0f * gEnv->pTimer->GetFrameTime();
        Vec4 vRainNormalMapParams = Vec4(m_pVelocityProj.x * ((float) iViewportWidth), 0, fFrameScale * fGravity, fFrameScale * 1.0f + m_pVelocityProj.y * ((float) iViewportHeight)) * 0.25f;

        //texture to restore
        CTexture*   pPrevTexture = rptexPrevRT;

        // apply extinction
        static CCryNameTSCRC pTech1Name("RainDropsExtinction");
        PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTech1Name, FEF_DONTSETSTATES);   //FEF_DONTSETTEXTURES |

        gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

        static CCryNameR pParam1Name("vRainNormalMapParams");
        PostProcessUtils().ShSetParamVS(pParam1Name, vRainNormalMapParams);

        vRainNormalMapParams.w = fFrameScale;
        static CCryNameR pParam0Name("vRainParams");
        PostProcessUtils().ShSetParamPS(pParam0Name, vRainNormalMapParams);

        PostProcessUtils().SetTexture(pPrevTexture, 0, FILTER_LINEAR);

        PostProcessUtils().DrawFullScreenTri(iRTWidth, iRTHeight);

        PostProcessUtils().ShEndPass();
    }
}


void CRainDrops::DrawFinal(CTexture*& rptexCurrRT)
{
    PROFILE_LABEL_SCOPE("RAIN_DROPS_FINAL");

    //clear and set render flags
    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]);

    // display rain
    static CCryNameTSCRC pTechName("RainDropsFinal");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    static CCryNameR pParam0Name("vRainNormalMapParams");
    PostProcessUtils().ShSetParamVS(pParam0Name, Vec4(1.0f, 1.0f, 1.0f, -1.0f));
    static CCryNameR pParam1Name("vRainParams");
    PostProcessUtils().ShSetParamPS(pParam1Name, Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    PostProcessUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_LINEAR);
    PostProcessUtils().SetTexture(rptexCurrRT, 1, FILTER_LINEAR, TADDR_MIRROR);

    PostProcessUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0.0f, &gcpRendD3D->m_FullResRect);

    PostProcessUtils().ShEndPass();

    //reset render flags
    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CSceneRain::CreateBuffers(uint16 nVerts, void*& pINVB, SVF_P3F_C4B_T2F* pVtxList)
{
    D3DBuffer* pVB = 0;

    HRESULT hr = S_OK;

    // Create VB/IB
    D3D11_BUFFER_DESC BufDesc;
    ZeroStruct(BufDesc);
    BufDesc.ByteWidth = nVerts * sizeof(SVF_P3F_C4B_T2F);
    BufDesc.Usage = D3D11_USAGE_IMMUTABLE;
    BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufDesc.CPUAccessFlags = 0;
    BufDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA SubResData;
    ZeroStruct(SubResData);
    SubResData.pSysMem = pVtxList;
    SubResData.SysMemPitch = 0;
    SubResData.SysMemSlicePitch = 0;

    hr = gcpRendD3D->GetDevice().CreateBuffer(&BufDesc, &SubResData, (ID3D11Buffer**)&pVB);
    assert(SUCCEEDED(hr));

    pINVB = pVB;
}

int CSceneRain::CreateResources()
{
    Release();

    static const int nSlices = 12;
    const float nSliceStep(DEG2RAD(360.0f / (float) nSlices));

    std::vector<SVF_P3F_C4B_T2F> pVB;
    SVF_P3F_C4B_T2F vVertex;
    vVertex.color.dcolor = ~0;
    vVertex.st = Vec2(0.0f, 0.0f);

    // Generate top cone vertices
    for (int h = 0; h < nSlices + 1; ++h)
    {
        float x = cosf(((float)h) * nSliceStep);
        float y = sinf(((float)h) * nSliceStep);

        vVertex.xyz = Vec3(x * 0.01f, y * 0.01f, 1);
        pVB.push_back(vVertex);

        vVertex.xyz = Vec3(x, y, 0.33f);
        pVB.push_back(vVertex);
    }

    // Generate cylinder vertices
    for (int h = 0; h < nSlices + 1; ++h)
    {
        float x = cosf(((float)h) * nSliceStep);
        float y = sinf(((float)h) * nSliceStep);

        vVertex.xyz = Vec3(x, y, 0.33f);
        pVB.push_back(vVertex);

        vVertex.xyz = Vec3(x, y, -0.33f);
        pVB.push_back(vVertex);
    }

    // Generate bottom cone vertices
    for (int h = 0; h < nSlices + 1; ++h)
    {
        float x = cosf(((float)h) * nSliceStep);
        float y = sinf(((float)h) * nSliceStep);

        vVertex.xyz = Vec3(x, y, -0.33f);
        pVB.push_back(vVertex);

        vVertex.xyz = Vec3(x * 0.01f, y * 0.01f, -1);
        pVB.push_back(vVertex);
    }

    m_nConeVBSize = pVB.size();
    CreateBuffers(m_nConeVBSize, m_pConeVB, &pVB[0]);

    return 1;
}

void CSceneRain::Release()
{
    D3DBuffer* pVB = (D3DBuffer*)m_pConeVB;
    SAFE_RELEASE(pVB);
    m_pConeVB = 0;
    m_bReinit = true;
}

void CSceneRain::Render()
{
    if (!m_pConeVB)
    {
        CreateResources();
    }

    PROFILE_LABEL_SCOPE("RAIN");
    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    // Shader constants
    CCryNameTSCRC pSceneRainTechName("SceneRain");
    CCryNameTSCRC pSceneRainOccAccTechName("SceneRainOccAccumulate");
    CCryNameTSCRC pSceneRainOccBlurTechName("SceneRainOccBlur");
    static CCryNameR pRainParamName0("sceneRainParams0");
    static CCryNameR pRainParamName1("sceneRainParams1");
    static CCryNameR pUnscaledFactorName("unscaledFactor");
    static CCryNameR pRainOccParamName("sceneRainOccMtx");
    static CCryNameR pRainMtxParamName("sceneRainMtx");
    static CCryNameR pRainOccBlurParamName("gRainOccBlurScale");
    static CCryNameR pRainBloomParamName("gRainBloomParams");

    // Generate screen-space rain occlusion
    CTexture* pDistRainOccTex = NULL;

    if (m_RainVolParams.bApplyOcclusion)
    {
        pDistRainOccTex = CTexture::s_ptexRainSSOcclusion[0];

        if (!CTexture::IsTextureExist(pDistRainOccTex))
        {
            // Render targets not generated yet
            // - Better to skip and have no rain than it render over everything
            return;
        }

        PROFILE_LABEL_SCOPE("RAIN_DISTANT_OCCLUSION");
        {
            PROFILE_LABEL_SCOPE("ACCUMULATE");
            gcpRendD3D->FX_PushRenderTarget(0, pDistRainOccTex, NULL);
            gcpRendD3D->RT_SetViewport(0, 0, pDistRainOccTex->GetWidth(), pDistRainOccTex->GetHeight());

            PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pSceneRainOccAccTechName, FEF_DONTSETSTATES);

            CShaderMan::s_shPostEffects->FXSetVSFloat(pRainOccParamName, (Vec4*)m_RainVolParams.matOccTransRender.GetData(), 4);
            CShaderMan::s_shPostEffects->FXSetPSFloat(pRainOccParamName, (Vec4*)m_RainVolParams.matOccTransRender.GetData(), 4);
            gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

            PostProcessUtils().DrawFullScreenTriWPOS(pDistRainOccTex->GetWidth(), pDistRainOccTex->GetHeight());

            PostProcessUtils().ShEndPass();

            gcpRendD3D->FX_PopRenderTarget(0);
        }

        {
            PROFILE_LABEL_SCOPE("BLUR");
            pDistRainOccTex = CTexture::s_ptexRainSSOcclusion[0];

            const float fDist = 8.0f;
            PostProcessUtils().TexBlurGaussian(pDistRainOccTex, 0, 1.f, fDist, false, NULL, false, CTexture::s_ptexRainSSOcclusion[1]);
        }
    }

    Vec2 unscaledFactor(1.0f, 1.0f);

    D3DBuffer* pVB = (D3DBuffer*)m_pConeVB;
    uint16 nVerts = m_nConeVBSize;

    gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexHDRTarget, NULL);
    gcpRendD3D->RT_SetViewport(0, 0, CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());

    const float fAmount = m_RainVolParams.fCurrentAmount * CRenderer::CV_r_rainamount;
    const int nLayers = clamp_tpl(int(fAmount + 0.0001f), 1, 3);
    const int nPasses = nLayers;

    // Get lightning color - will just use for overbright rain
    Vec3 v;
    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_HIGHLIGHT_COLOR, v);
    const float fHighlight = 2.f * v.len();
    Vec4 pParams = Vec4(0.f, 0.f, 0.f, 0.f);
    const float fSizeMult = max(CRenderer::CV_r_rainDistMultiplier, 1e-3f) * 0.5f;

    if (m_RainVolParams.bApplyOcclusion)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }
    else
    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    for (int pass = 0; pass < nPasses; ++pass)
    {
        int nCurLayer = pass;

        PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pSceneRainTechName, FEF_DONTSETSTATES);

        gcpRendD3D->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
        gcpRendD3D->D3DSetCull(eCULL_None);

        pParams.x = m_RainVolParams.fRainDropsSpeed;
        pParams.z = float(nCurLayer) / max(nLayers - 1, 1);
        pParams.y = fHighlight * (1.f - pParams.z);
        pParams.w = (nCurLayer + 1) * fSizeMult;
        pParams.w = powf(pParams.w, 1.5f);
        CShaderMan::s_shPostEffects->FXSetVSFloat(pRainParamName0, &pParams, 1);

        pParams = Vec4(unscaledFactor.x, unscaledFactor.y, 1.0f, 1.0f);
        CShaderMan::s_shPostEffects->FXSetVSFloat(pUnscaledFactorName, &pParams, 1);

        pParams.x = fAmount * m_RainVolParams.fRainDropsAmount;
        pParams.w = 1.f;
        CShaderMan::s_shPostEffects->FXSetPSFloat(pRainParamName0, &pParams, 1);

        pParams = Vec4(m_RainVolParams.fRainDropsLighting, 0.f, 0.f, 0.f);
        CShaderMan::s_shPostEffects->FXSetPSFloat(pRainParamName1, &pParams, 1);

        if (m_RainVolParams.bApplyOcclusion)
        {
            CShaderMan::s_shPostEffects->FXSetVSFloat(pRainOccParamName, (Vec4*)m_RainVolParams.matOccTransRender.GetData(), 4);

            STexState sTexState = STexState(FILTER_LINEAR, true);
            pDistRainOccTex->Apply(2, CTexture::GetTexState(sTexState));
        }

        Matrix44 rainRot = Matrix44(Matrix33(m_RainVolParams.qRainRotation));
        CShaderMan::s_shPostEffects->FXSetVSFloat(pRainMtxParamName, (Vec4*)rainRot.GetData(), 4);

        gcpRendD3D->FX_SetVStream(0, pVB, 0, sizeof(SVF_P3F_C4B_T2F));

        // Bind average luminance
        GetUtils().SetTexture(CTexture::s_ptexHDRToneMaps[0], 0, FILTER_LINEAR);

        if (!FAILED(gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
        {
            gcpRendD3D->FX_Commit();
            gcpRendD3D->FX_DrawPrimitive(eptTriangleStrip, 0, nVerts);
        }

        PostProcessUtils().ShEndPass();
    }

    gcpRendD3D->FX_PopRenderTarget(0);

    gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];

    // (re-set back-buffer): if the platform does lazy RT updates/setting there's strong possibility we run into problems when we try to resolve with no RT set
    gcpRendD3D->FX_SetActiveRenderTargets();
}


bool CSceneRain::Preprocess()
{
    bool bIsActive = IsActive()
        && !(m_RainVolParams.bApplyOcclusion && m_RainVolParams.areaAABB.IsReset())
        && m_RainVolParams.fRainDropsAmount > 0.01f;

    if (!bIsActive)
    {
        Release();
    }
    else
    {
        ++m_updateFrameCount;
    }

    return false;  // Rain is rendered in HDR
}


void CSceneRain::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pActive->ResetParam(0);
    m_RainVolParams.areaAABB.Reset();
    m_updateFrameCount = 0;
}

void CSceneRain::OnLostDevice()
{
    Release();
}

const char* CSceneRain::GetName() const
{
    return "SceneRain";
}
