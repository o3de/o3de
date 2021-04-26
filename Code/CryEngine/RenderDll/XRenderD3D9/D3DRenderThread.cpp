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

#include "D3DStereo.h"

#include "../Common/Textures/TextureManager.h"

//=======================================================================

bool CD3D9Renderer::RT_CreateDevice()
{
    LOADING_TIME_PROFILE_SECTION;
    
#if (defined(WIN32) || defined(WIN64)) && !defined(SUPPORT_DEVICE_INFO)
    if (!m_bShaderCacheGen && !SetWindow(m_width, m_height, m_bFullScreen, m_hWnd))
    {
        return false;
    }
#endif

    return SetRes();
}

void CD3D9Renderer::RT_ReleaseVBStream(void* pVB, [[maybe_unused]] int nStream)
{
    D3DBuffer* pBuf = (D3DBuffer*)pVB;
    SAFE_RELEASE(pBuf);
}
void CD3D9Renderer::RT_ReleaseCB(void* pVCB)
{
    AzRHI::ConstantBuffer* pCB = (AzRHI::ConstantBuffer*)pVCB;
    SAFE_RELEASE(pCB);
}

void CD3D9Renderer::RT_ClearTarget(ITexture* tex, const ColorF& color)
{
    CTexture* pTex = reinterpret_cast<CTexture*>(tex);
    if (pTex->GetFlags() & FT_USAGE_DEPTHSTENCIL)
    {
        D3DDepthSurface* pSurf = reinterpret_cast<D3DDepthSurface*>(pTex->GetDeviceDepthStencilSurf());
        if (!pSurf)
        {
            return;
        }

        // NOTE: normalized depth in color.r and unnormalized stencil in color.g
        GetDeviceContext().ClearDepthStencilView(pSurf, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, color.r, (uint8)color.g);
    }
    else
    {
        D3DSurface* pSurf = pTex->GetSurface(0, 0);
        if (!pSurf)
        {
            return;
        }

        GetDeviceContext().ClearRenderTargetView(pSurf, (float*)&color);
    }
}

void CD3D9Renderer::RT_DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, uint32 nVerts, uint32 nInds, const PublicRenderPrimitiveType nPrimType)
{
    FX_SetFPMode();

    if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        // Create the temp buffer after we try to set the vertex declaration otherwise
        // if that fails we won't call FX_DrawPrimitive that on a platform level
        // cleans up some of the memory stuff that the TempDynVB creates
        TempDynVB<SVF_P3F_C4B_T2F>::CreateFillAndBind(pBuf, nVerts, 0);
        if (pInds)
        {
            TempDynIB16::CreateFillAndBind(pInds, nInds);
            FX_DrawIndexedPrimitive(GetInternalPrimitiveType(nPrimType), 0, 0, nVerts, 0, nInds);
        }
        else
        {
            FX_DrawPrimitive(GetInternalPrimitiveType(nPrimType), 0, nVerts);
        }
    }
}

void CD3D9Renderer::RT_DrawDynVBUI(SVF_P2F_C4B_T2F_F4B* pBuf, uint16* pInds, uint32 nVerts, uint32 nInds, const PublicRenderPrimitiveType nPrimType)
{
    FX_SetUIMode();

    if (!FAILED(FX_SetVertexDeclaration(0, eVF_P2F_C4B_T2F_F4B)))
    {
        // Create the temp buffer after we try to set the vertex declaration otherwise
        // if that fails we won't call FX_DrawPrimitive that on a platform level
        // cleans up some of the memory stuff that the TempDynVB creates
        TempDynVB<SVF_P2F_C4B_T2F_F4B>::CreateFillAndBind(pBuf, nVerts, 0);
        if (pInds)
        {
            TempDynIB16::CreateFillAndBind(pInds, nInds);
            FX_DrawIndexedPrimitive(GetInternalPrimitiveType(nPrimType), 0, 0, nVerts, 0, nInds);
        }
        else
        {
            FX_DrawPrimitive(GetInternalPrimitiveType(nPrimType), 0, nVerts);
        }
    }
}

void CD3D9Renderer::RT_Draw2dImageInternal(C2dImage* images, uint32 numImages, bool stereoLeftEye)
{
    SetCullMode(R_CULL_DISABLE);

    float maxParallax = 0;
    float screenDist = 0;

    if (GetS3DRend().IsStereoEnabled())
    {
        maxParallax = GetS3DRend().GetMaxSeparationScene();
        screenDist = GetS3DRend().GetZeroParallaxPlaneDist();
    }

    // Flush the current viewports.
    // The GetViewport call below uses either m_MainRTViewport or m_NewViewport, then the image scaling code 
    // (ScaleCoordX/ScaleCoordY) uses m_CurViewport, so this could lead to different viewport settings being used unless we flush.
    FX_SetViewport();

    // Set orthographic projection
    Matrix44A origMatProj = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj;
    Matrix44A* m = &m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj;
    int vx, vy, vw, vh;
    GetViewport(&vx, &vy, &vw, &vh);
    mathMatrixOrthoOffCenterLH(m, (float)vx, (float)vw, (float)vh, (float)vy, 0.0f, 1.0f);
    Matrix44A origMatView = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;
    m = &m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView.SetIdentity();

    // Create dynamic geometry
    TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
    vb.Allocate(numImages * 4);
    SVF_P3F_C4B_T2F* vQuad = vb.Lock();

    for (uint32 i = 0; i < numImages; ++i)
    {
        C2dImage& img = images[i];
        uint32 baseIdx = i * 4;

        float parallax = 0;
        if (img.stereoDepth > 0)
        {
            parallax = 800 * maxParallax * (1 - screenDist / img.stereoDepth);
        }

        float xpos = (float)ScaleCoordX(img.xpos + parallax * (stereoLeftEye ? -1 : 1));
        float w = (float)ScaleCoordX(img.w);
        float ypos = (float)ScaleCoordY(img.ypos);
        float h = (float)ScaleCoordY(img.h);

        if (img.angle != 0)
        {
            float xsub = (float)(xpos + w / 2.0f);
            float ysub = (float)(ypos + h / 2.0f);

            float x, y, x1, y1;
            float mcos = cos_tpl(DEG2RAD(img.angle));
            float msin = sin_tpl(DEG2RAD(img.angle));

            x = xpos - xsub;
            y = ypos - ysub;
            x1 = x * mcos - y * msin;
            y1 = x * msin + y * mcos;
            x1 += xsub;
            y1 += ysub;
            vQuad[baseIdx].xyz.x = x1;
            vQuad[baseIdx].xyz.y = y1;

            x = xpos + w - xsub;
            y = ypos - ysub;
            x1 = x * mcos - y * msin;
            y1 = x * msin + y * mcos;
            x1 += xsub;
            y1 += ysub;
            vQuad[baseIdx + 1].xyz.x = x1;//xpos + fw;
            vQuad[baseIdx + 1].xyz.y = y1;// fy;

            x = xpos + w - xsub;
            y = ypos + h - ysub;
            x1 = x * mcos - y * msin;
            y1 = x * msin + y * mcos;
            x1 += xsub;
            y1 += ysub;
            vQuad[baseIdx + 3].xyz.x = x1;//xpos + fw;
            vQuad[baseIdx + 3].xyz.y = y1;//fy + fh;

            x = xpos - xsub;
            y = ypos + h - ysub;
            x1 = x * mcos - y * msin;
            y1 = x * msin + y * mcos;
            x1 += xsub;
            y1 += ysub;
            vQuad[baseIdx + 2].xyz.x = x1;//xpos;
            vQuad[baseIdx + 2].xyz.y = y1;//fy + fh;
        }
        else
        {
            vQuad[baseIdx].xyz.x = xpos;
            vQuad[baseIdx].xyz.y = ypos;

            vQuad[baseIdx + 1].xyz.x = xpos + w;
            vQuad[baseIdx + 1].xyz.y = ypos;

            vQuad[baseIdx + 2].xyz.x = xpos;
            vQuad[baseIdx + 2].xyz.y = ypos + h;

            vQuad[baseIdx + 3].xyz.x = xpos + w;
            vQuad[baseIdx + 3].xyz.y = ypos + h;
        }

        vQuad[baseIdx + 0].st = Vec2(img.s0, 1.0f - img.t0);
        vQuad[baseIdx + 1].st = Vec2(img.s1, 1.0f - img.t0);
        vQuad[baseIdx + 2].st = Vec2(img.s0, 1.0f - img.t1);
        vQuad[baseIdx + 3].st = Vec2(img.s1, 1.0f - img.t1);

        for (int j = 0; j < 4; ++j)
        {
            vQuad[baseIdx + j].color.dcolor = img.col;
            vQuad[baseIdx + j].xyz.z = img.z;
        }
    }

    vb.Unlock();
    vb.Bind(0);
    vb.Release();

    CTexture* prevTex = NULL;
    EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, (eCA_Diffuse | (eCA_Diffuse << 3)), (eCA_Diffuse | (eCA_Diffuse << 3)));
    EF_SetSrgbWrite(false);
    FX_SetFPMode();

    if (FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView = origMatView;
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj = origMatProj;
        return;
    }


    int nState = m_bDraw2dImageStretchMode ? CTexture::GetTexState(STexState(FILTER_TRILINEAR, true)) : CTexture::GetTexState(STexState(FILTER_POINT, true));

    // Draw quads
    for (uint32 i = 0; i < numImages; ++i)
    {
        C2dImage& img = images[i];

        if (img.pTex != prevTex)
        {
            prevTex = img.pTex;
            if (img.pTex)
            {
                img.pTex->Apply(0, nState);
                EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
                EF_SetSrgbWrite(false);
            }
            else
            {
                EF_SetColorOp(eCO_REPLACE, eCO_REPLACE, (eCA_Diffuse | (eCA_Diffuse << 3)), (eCA_Diffuse | (eCA_Diffuse << 3)));
                EF_SetSrgbWrite(false);
            }

            FX_SetFPMode();

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(D3DRenderThread_cpp)
#endif
        }

        FX_DrawPrimitive(eptTriangleStrip, i * 4, 4);
    }

    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView = origMatView;
    m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj = origMatProj;
}

void CD3D9Renderer::RT_DrawStringU(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx) const
{
    SetProfileMarker("DRAWSTRINGU", CRenderer::ESPM_PUSH);

    pFont->RenderCallback(x, y, z, pStr, asciiMultiLine, ctx);

    SetProfileMarker("DRAWSTRINGU", CRenderer::ESPM_POP);
}

void CD3D9Renderer::RT_DrawLines(Vec3 v[], int nump, ColorF& col, int flags, float fGround)
{
    if (m_bDeviceLost)
    {
        return;
    }

    int i;
    int st;

    EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, eCA_Texture | (eCA_Diffuse << 3), eCA_Texture | (eCA_Diffuse << 3));
    EF_SetSrgbWrite(false);

    st = GS_NODEPTHTEST;
    if (flags & 1)
    {
        st |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
    }
    FX_SetState(st);
    CTextureManager::Instance()->GetWhiteTexture()->Apply(0);

    DWORD c = D3DRGBA(col.r, col.g, col.b, col.a);

    if (fGround >= 0)
    {
        TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
        vb.Allocate(nump * 2);
        SVF_P3F_C4B_T2F* vQuad = vb.Lock();

        for (i = 0; i < nump; i++)
        {
            vQuad[i * 2 + 0].xyz.x = v[i][0];
            vQuad[i * 2 + 0].xyz.y = fGround;
            vQuad[i * 2 + 0].xyz.z = 0;
            vQuad[i * 2 + 0].color.dcolor = c;
            vQuad[i * 2 + 0].st = Vec2(0.0f, 0.0f);
            vQuad[i * 2 + 1].xyz = v[i];
            vQuad[i * 2 + 1].color.dcolor = c;
            vQuad[i * 2 + 1].st = Vec2(0.0f, 0.0f);
        }

        vb.Unlock();
        vb.Bind(0);
        vb.Release();

        FX_SetFPMode();
        if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
        {
            FX_DrawPrimitive(eptLineList, 0, nump * 2);
        }
    }
    else
    {
        TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
        vb.Allocate(nump);
        SVF_P3F_C4B_T2F* vQuad = vb.Lock();

        for (i = 0; i < nump; i++)
        {
            vQuad[i].xyz = v[i];
            vQuad[i].color.dcolor = c;
            vQuad[i].st = Vec2(0, 0);
        }

        vb.Unlock();
        vb.Bind(0);
        vb.Release();

        FX_SetFPMode();
        if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
        {
            FX_DrawPrimitive(eptLineStrip, 0, nump);
        }
    }
}

void CD3D9Renderer::RT_Draw2dImageStretchMode(bool bStretch)
{
    m_bDraw2dImageStretchMode = bStretch;
}

void CD3D9Renderer::RT_Draw2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, DWORD col, float z)
{
    C2dImage img(xpos, ypos, w, h, pTexture, s0, t0, s1, t1, angle, col, z, 0);

    SetProfileMarker("DRAW2DIMAGE", CRenderer::ESPM_PUSH);

    if (GetS3DRend().IsStereoEnabled())
    {
        GetS3DRend().BeginRenderingTo(STEREO_EYE_LEFT);
        RT_Draw2dImageInternal(&img, 1, true);
        GetS3DRend().EndRenderingTo(STEREO_EYE_LEFT);

        GetS3DRend().BeginRenderingTo(STEREO_EYE_RIGHT);
        RT_Draw2dImageInternal(&img, 1, false);
        GetS3DRend().EndRenderingTo(STEREO_EYE_RIGHT);
    }
    else
    {
        RT_Draw2dImageInternal(&img, 1);
    }

    SetProfileMarker("DRAW2DIMAGE", CRenderer::ESPM_POP);
}

void CD3D9Renderer::RT_Push2dImage(float xpos, float ypos, float w, float h, CTexture* pTexture, float s0, float t0, float s1, float t1, float angle, DWORD col, float z, float stereoDepth)
{
    m_2dImages.Add(C2dImage(xpos, ypos, w, h, pTexture, s0, t0, s1, t1, angle, col, z, stereoDepth));
}

void CD3D9Renderer::RT_Draw2dImageList()
{
    if (m_2dImages.empty())
    {
        return;
    }

    SetProfileMarker("DRAW2DIMAGELIST", CRenderer::ESPM_PUSH);

    if (GetS3DRend().IsStereoEnabled())
    {
        GetS3DRend().BeginRenderingTo(STEREO_EYE_LEFT);
        RT_Draw2dImageInternal(m_2dImages.Data(), m_2dImages.size(), true);
        GetS3DRend().EndRenderingTo(STEREO_EYE_LEFT);

        GetS3DRend().BeginRenderingTo(STEREO_EYE_RIGHT);
        RT_Draw2dImageInternal(m_2dImages.Data(), m_2dImages.size(), false);
        GetS3DRend().EndRenderingTo(STEREO_EYE_RIGHT);
    }
    else
    {
        RT_Draw2dImageInternal(m_2dImages.Data(), m_2dImages.size());
    }

    SetProfileMarker("DRAW2DIMAGELIST", CRenderer::ESPM_POP);

    m_2dImages.resize(0);
}

void CD3D9Renderer::RT_PushRenderTarget(int nTarget, CTexture* pTex, SDepthTexture* pDepth, int nS)
{
    FX_PushRenderTarget(nTarget, pTex, pDepth, nS);
}

void CD3D9Renderer::RT_PopRenderTarget(int nTarget)
{
    FX_PopRenderTarget(nTarget);
}

void CD3D9Renderer::RT_Init()
{
    EF_Init();
}

void CD3D9Renderer::RT_CreateResource(SResourceAsync* pRes)
{
    if (pRes->eClassName == eRCN_Texture)
    {
        CTexture* pTex = NULL;

        if (pRes->nTexId)
        { // only create device texture
            pTex = CTexture::GetByID(pRes->nTexId);
            const byte* arrData[6] = { pRes->pData, 0, 0, 0, 0, 0 };
            pTex->CreateDeviceTexture(arrData);
        }
        else
        { // create full texture
            char* pName = pRes->Name;
            char szName[128];
            if (!pName)
            {
                sprintf_s(szName, "$AutoDownloadAsync_%d", m_TexGenID++);
                pName = szName;
            }
            pTex = CTexture::Create2DTexture(pName, pRes->nWidth, pRes->nHeight, pRes->nMips, pRes->nTexFlags, pRes->pData, (ETEX_Format)pRes->nFormat, (ETEX_Format)pRes->nFormat);
        }

        SAFE_DELETE_ARRAY(pRes->pData);
        pRes->pResource = pTex;
        pRes->nReady = (CTexture::IsTextureExist(pTex));
    }
    else
    {
        assert(0);
    }

    delete pRes;
}
void CD3D9Renderer::RT_ReleaseResource(SResourceAsync* pRes)
{
    if (pRes->eClassName == eRCN_Texture)
    {
        CTexture* pTex = (CTexture*)pRes->pResource;
        pTex->Release();
    }
    else
    {
        assert(0);
    }

    delete pRes;
}

void CD3D9Renderer::RT_UnbindTMUs()
{
    D3DShaderResourceView* pTex[MAX_TMU] = {NULL};
    for (uint32 i = 0; i < MAX_TMU; ++i)
    {
        CTexture::s_TexStages[i].m_DevTexture = NULL;
    }
    m_DevMan.BindSRV(eHWSC_Vertex, pTex, 0, MAX_TMU);
    m_DevMan.BindSRV(eHWSC_Geometry, pTex, 0, MAX_TMU);
    m_DevMan.BindSRV(eHWSC_Domain, pTex, 0, MAX_TMU);
    m_DevMan.BindSRV(eHWSC_Hull, pTex, 0, MAX_TMU);
    m_DevMan.BindSRV(eHWSC_Compute, pTex, 0, MAX_TMU);
    m_DevMan.BindSRV(eHWSC_Pixel, pTex, 0, MAX_TMU);

    m_DevMan.CommitDeviceStates();
}

void CD3D9Renderer::RT_UnbindResources()
{
    for (AZ::u32 shaderClass = 0; shaderClass < eHWSC_Num; ++shaderClass)
    {
        for (AZ::u32 shaderSlot = 0; shaderSlot < eConstantBufferShaderSlot_Count; ++shaderSlot)
        {
            m_DevMan.BindConstantBuffer(EHWShaderClass(shaderClass), nullptr, shaderSlot);
        }
    }

    D3DBuffer* pBuffers[16] = { 0 };
    UINT StrideOffset[16] = { 0 };

    m_DevMan.BindIB(NULL, 0, DXGI_FORMAT_R16_UINT);
    m_RP.m_pIndexStream = NULL;

    m_DevMan.BindVB(0, 16, pBuffers, StrideOffset, StrideOffset);
    m_RP.m_VertexStreams[0].pStream = NULL;

    m_DevMan.BindVtxDecl(NULL);
    m_pLastVDeclaration = NULL;

    m_DevMan.BindShader(eHWSC_Pixel, NULL);
    m_DevMan.BindShader(eHWSC_Vertex, NULL);
    m_DevMan.BindShader(eHWSC_Geometry, NULL);
    m_DevMan.BindShader(eHWSC_Domain, NULL);
    m_DevMan.BindShader(eHWSC_Hull, NULL);
    m_DevMan.BindShader(eHWSC_Compute, NULL);

    CHWShader::s_pCurPS = nullptr;
    CHWShader::s_pCurVS = nullptr;
    CHWShader::s_pCurGS = nullptr;
    CHWShader::s_pCurDS = nullptr;
    CHWShader::s_pCurHS = nullptr;
    CHWShader::s_pCurCS = nullptr;

    m_DevMan.CommitDeviceStates();
}

void CD3D9Renderer::RT_ReleaseRenderResources()
{
    GetGraphicsPipeline().Shutdown();

    m_cEF.mfReleasePreactivatedShaderData();
    m_cEF.m_Bin.InvalidateCache();
    //m_cEF.m_Bin.m_BinPaths.clear();
    ForceFlushRTCommands();

    for (uint i = 0; i < CLightStyle::s_LStyles.Num(); i++)
    {
        delete CLightStyle::s_LStyles[i];
    }
    CLightStyle::s_LStyles.Free();

    FX_PipelineShutdown();
    ID3D11RenderTargetView* pRTV[RT_STACK_WIDTH] = { NULL };
    GetDeviceContext().OMSetRenderTargets(RT_STACK_WIDTH, pRTV, NULL);
    m_nMaxRT2Commit = -1;
}
void CD3D9Renderer::RT_CreateRenderResources()
{
    EF_Init();

    if (m_pPostProcessMgr)
    {
        m_pPostProcessMgr->CreateResources();
    }

    GetGraphicsPipeline().Init();
}

void CD3D9Renderer::RT_PrecacheDefaultShaders()
{
    SShaderCombination cmb;
    m_cEF.s_ShaderStereo->mfPrecache(cmb, true, true, NULL);

    cmb.m_RTMask |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    cmb.m_RTMask |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    cmb.m_RTMask |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    m_cEF.s_ShaderVideo->mfPrecache(cmb, true, true, nullptr);
}

void CD3D9Renderer::RT_ResetGlass()
{
}

void CD3D9Renderer::SetRendererCVar(ICVar* pCVar, const char* pArgText, const bool bSilentMode)
{
    m_pRT->RC_SetRendererCVar(pCVar, pArgText, bSilentMode);
}

void CD3D9Renderer::RT_SetRendererCVar(ICVar* pCVar, const char* pArgText, const bool bSilentMode)
{
    if (pCVar)
    {
        pCVar->Set(pArgText);

        if (!bSilentMode)
        {
            if (gEnv->IsEditor())
            {
                gEnv->pLog->LogWithType(ILog::eInputResponse, "%s = %s (Renderer CVar)", pCVar->GetName(), pCVar->GetString());
            }
            else
            {
                gEnv->pLog->LogWithType(ILog::eInputResponse, "    $3%s = $6%s $5(Renderer CVar)", pCVar->GetName(), pCVar->GetString());
            }
        }
    }
}

void CD3D9Renderer::RT_PostLevelLoading()
{
    CRenderer::RT_PostLevelLoading();

    // Clear our the shadow mask texture in case the level we are loading does not
    // have any shadow casters. If we don't clear out the mask then whatever was
    // previous in the mask, including a previously loaded level, will be used and
    // incorrect shadows will be drawn.
    FX_ClearShadowMaskTexture();
}

void CD3D9Renderer::StartLoadtimePlayback(ILoadtimeCallback* pCallback)
{
    // make sure we can't access loading mode twice!
    if (m_pRT->m_pLoadtimeCallback)
    {
        return;
    }
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

    if (pCallback)
    {
        FlushRTCommands(true, true, true);

        m_pRT->m_pLoadtimeCallback = pCallback;
        SetViewport(0, 0, GetOverlayWidth(), GetOverlayHeight());
        m_pRT->RC_StartVideoThread();

        if (m_pRT->IsMultithreaded())
        {
            // wait until render thread has fully processed the start of the video
            // to reduce the congestion on the IO reading (make sure nothing else
            // beats the video to actually start reading something from the DVD)
            while (m_pRT->m_eVideoThreadMode != SRenderThread::eVTM_Active)
            {
                m_pRT->FlushAndWait();
                AZStd::this_thread::yield();
            }
        }
    }
}

void CD3D9Renderer::StopLoadtimePlayback()
{
    if (m_pRT->m_pLoadtimeCallback)
    {
        LOADING_TIME_PROFILE_SECTION;

        FlushRTCommands(true, true, true);

        m_pRT->RC_StopVideoThread();

        if (m_pRT->IsMultithreaded())
        {
            // wait until render thread has fully processed the shutdown of the loading thread
            while (m_pRT->m_eVideoThreadMode != SRenderThread::eVTM_Disabled)
            {
                m_pRT->FlushAndWait();
                AZStd::this_thread::yield();
            }
        }

        m_pRT->m_pLoadtimeCallback = nullptr;

        m_pRT->RC_BeginFrame();

#if !defined(STRIP_RENDER_THREAD)
        // Blit the accumulated commands from the renderloading thread into the current fill command queue
        // : Currently hacked into the RC_UpdateMaterialConstants command
        if (m_pRT->m_CommandsLoading.size())
        {
            void* buf = m_pRT->m_Commands[m_pRT->m_nCurThreadFill].Grow(m_pRT->m_CommandsLoading.size());
            memcpy(buf, &m_pRT->m_CommandsLoading[0], m_pRT->m_CommandsLoading.size());
            m_pRT->m_CommandsLoading.Free();
        }
#endif // !defined(STRIP_RENDER_THREAD)
    }
}

//////////////////////////////////////////////////////////////////////////
