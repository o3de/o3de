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

// Description : Direct3D specific FX shaders rendering pipeline.

#include "RenderDll_precompiled.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include <I3DEngine.h>
#include <IEntityRenderState.h>
#include "MultiLayerAlphaBlendPass.h"
#include <Common/ReverseDepth.h>
#include <Common/RenderCapabilities.h>
#include "GraphicsPipeline/FurPasses.h"

#include "../../Cry3DEngine/Environment/OceanEnvironmentBus.h"

#if defined(FEATURE_SVO_GI)
#include "D3D_SVO.h"
#endif

#include <AzCore/Statistics/StatisticalProfilerProxy.h>



long CD3D9Renderer::FX_SetVertexDeclaration(int StreamMask, const AZ::Vertex::Format& vertexFormat)
{
    FUNCTION_PROFILER_RENDER_FLAT
    HRESULT hr;

    bool bMorph = (StreamMask & VSM_MORPHBUDDY) != 0;
    bool bInstanced = (StreamMask & VSM_INSTANCED) != 0;

#if defined(FEATURE_PER_SHADER_INPUT_LAYOUT_CACHE)
    SOnDemandD3DVertexDeclarationCache pDeclCache[1] = {
        { NULL }
    };
    // (StreamMask & (0xfe | VSM_MORPHBUDDY)) is the value of StreamMask for most cases. There are a few exceptions:
    // 0xfe = 1111 1110 so the result is 0 in the case of VSM_GENERAL (1), or 0 if the mask bit is greater than 8 bits unless StreamMask happens to be VSM_MORPHBUDDY, in which case the result is again the value of StreamMask
    // At the time of this comment, that means the portion of the cacheID determined by StreamMask will be the same for VSM_GENERAL as it will be for VSM_INSTANCED, or anything that may come after VSM_INSTANCED
    uint64 cacheID = static_cast<uint64>(StreamMask & (0xfe | VSM_MORPHBUDDY)) ^ (static_cast<uint64>(vertexFormat.GetEnum()) << 32);
    if (CHWShader_D3D::s_pCurInstVS)
    {
        pDeclCache->m_pDeclaration = CHWShader_D3D::s_pCurInstVS->GetCachedInputLayout(cacheID);
    }
#else
    AZ::u32 declCacheKey = vertexFormat.GetEnum();
    if (CHWShader_D3D::s_pCurInstVS)
    {
        declCacheKey = CHWShader_D3D::s_pCurInstVS->GenerateVertexDeclarationCacheKey(vertexFormat);
    }

    SOnDemandD3DVertexDeclarationCache* pDeclCache = &m_RP.m_D3DVertexDeclarationCache[(StreamMask & 0xff) >> 1][bMorph || bInstanced][declCacheKey];

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(D3DFXPipeline_cpp)
#endif
#endif

    if (!pDeclCache->m_pDeclaration)
    {
        SOnDemandD3DVertexDeclaration Decl;

        EF_OnDemandVertexDeclaration(Decl, (StreamMask & 0xff) >> 1, vertexFormat, bMorph, bInstanced);
        if (!Decl.m_Declaration.size())
        {
            return S_FALSE;
        }

        if (!CHWShader_D3D::s_pCurInstVS || !CHWShader_D3D::s_pCurInstVS->m_pShaderData || CHWShader_D3D::s_pCurInstVS->m_bFallback)
        {
            return (HRESULT)-1;
        }
        int nSize = CHWShader_D3D::s_pCurInstVS->m_nDataSize;
        void* pVSData = CHWShader_D3D::s_pCurInstVS->m_pShaderData;
        if (FAILED(hr = GetDevice().CreateInputLayout(&Decl.m_Declaration[0], Decl.m_Declaration.size(), pVSData, nSize, &pDeclCache->m_pDeclaration)))
        {
#ifndef _RELEASE
            iLog->LogError("Failed to create an input layout for material \"%s\".\nThe shader and the vertex formats may be incompatible.\nVertex format: \"%d\".  Shader expects: \"%d\".\n\n", m_RP.m_pShaderResources->m_szMaterialName, (int)vertexFormat.GetEnum(), (int)CHWShader_D3D::s_pCurInstVS->m_vertexFormat.GetEnum());
#endif
            return hr;
        }
#if defined(FEATURE_PER_SHADER_INPUT_LAYOUT_CACHE)
        CHWShader_D3D::s_pCurInstVS->SetCachedInputLayout(pDeclCache->m_pDeclaration, cacheID);
#endif
    }
    D3DVertexDeclaration* pD3DDecl = pDeclCache->m_pDeclaration;
    if (!CHWShader_D3D::s_pCurInstVS || !CHWShader_D3D::s_pCurInstPS || (CHWShader_D3D::s_pCurInstVS->m_bFallback | CHWShader_D3D::s_pCurInstPS->m_bFallback))
    {
        FX_Commit();
        return E_FAIL;
    }

    if (m_pLastVDeclaration != pD3DDecl)
    {
        // Don't set input layout on fallback shader (crashes in DX11 NV driver)
        if (!CHWShader_D3D::s_pCurInstVS || CHWShader_D3D::s_pCurInstVS->m_bFallback)
        {
            return (HRESULT)-1;
        }
        m_pLastVDeclaration = pD3DDecl;
        m_DevMan.BindVtxDecl(pD3DDecl);
    }

    return S_OK;
}

void CD3D9Renderer::EF_ClearTargetsImmediately(uint32 nFlags)
{
    nFlags |= FRT_CLEAR_IMMEDIATE;

    EF_ClearTargetsLater(nFlags);

    if (nFlags & FRT_CLEAR_IMMEDIATE)
    {
        FX_SetActiveRenderTargets(true);
    }
}

void CD3D9Renderer::EF_ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors, float fDepth, uint8 nStencil)
{
    nFlags |= FRT_CLEAR_IMMEDIATE;

    EF_ClearTargetsLater(nFlags, Colors, fDepth, nStencil);

    if (nFlags & FRT_CLEAR_IMMEDIATE)
    {
        FX_SetActiveRenderTargets(true);
    }
}

void CD3D9Renderer::EF_ClearTargetsImmediately(uint32 nFlags, const ColorF& Colors)
{
    nFlags |= FRT_CLEAR_IMMEDIATE;

    EF_ClearTargetsLater(nFlags, Colors);

    if (nFlags & FRT_CLEAR_IMMEDIATE)
    {
        FX_SetActiveRenderTargets(true);
    }
}

void CD3D9Renderer::EF_ClearTargetsImmediately(uint32 nFlags, float fDepth, uint8 nStencil)
{
    nFlags |= FRT_CLEAR_IMMEDIATE;

    EF_ClearTargetsLater(nFlags, fDepth, nStencil);

    if (nFlags & FRT_CLEAR_IMMEDIATE)
    {
        FX_SetActiveRenderTargets(true);
    }
}

// Clear buffers (color, depth/stencil)
void CD3D9Renderer::EF_ClearTargetsLater(uint32 nFlags, const ColorF& Colors, float fDepth, uint8 nStencil)
{
    if (nFlags & FRT_CLEAR_FOGCOLOR)
    {
        for (int i = 0; i < RT_STACK_WIDTH; ++i)
        {
            if (m_pNewTarget[i])
            {
                m_pNewTarget[i]->m_ReqColor = m_cClearColor;
            }
        }
    }
    else if (nFlags & FRT_CLEAR_COLOR)
    {
        for (int i = 0; i < RT_STACK_WIDTH; ++i)
        {
            if (m_pNewTarget[i] && m_pNewTarget[i]->m_pTarget)
            {
                m_pNewTarget[i]->m_ReqColor = Colors;
            }
        }
    }

    if (nFlags & FRT_CLEAR_DEPTH)
    {
        m_pNewTarget[0]->m_fReqDepth = fDepth;
    }

    if (!(nFlags & FRT_CLEAR_IMMEDIATE))
    {
        m_pNewTarget[0]->m_ClearFlags = 0;
    }
    if ((nFlags & FRT_CLEAR_DEPTH) && m_pNewTarget[0]->m_pDepth)
    {
        m_pNewTarget[0]->m_ClearFlags |= CLEAR_ZBUFFER;
    }
    if (nFlags & FRT_CLEAR_COLOR)
    {
        m_pNewTarget[0]->m_ClearFlags |= CLEAR_RTARGET;
    }
    if (nFlags & FRT_CLEAR_COLORMASK)
    {
        m_pNewTarget[0]->m_ClearFlags |= FRT_CLEAR_COLORMASK;
    }

    if (m_sbpp && (nFlags & FRT_CLEAR_STENCIL) && m_pNewTarget[0]->m_pDepth)
    {
#ifdef SUPPORTS_MSAA
        if (gcpRendD3D->m_RP.m_MSAAData.Type)
        {
            m_RP.m_PersFlags2 |= RBPF2_MSAA_RESTORE_SAMPLE_MASK;
        }
#endif
        m_pNewTarget[0]->m_ClearFlags |= CLEAR_STENCIL;
        m_pNewTarget[0]->m_nReqStencil = nStencil;
    }
}

void CD3D9Renderer::EF_ClearTargetsLater(uint32 nFlags, float fDepth, uint8 nStencil)
{
    if (nFlags & FRT_CLEAR_FOGCOLOR)
    {
        for (int i = 0; i < RT_STACK_WIDTH; ++i)
        {
            if (m_pNewTarget[i])
            {
                m_pNewTarget[i]->m_ReqColor = m_cClearColor;
            }
        }
    }
    else if (nFlags & FRT_CLEAR_COLOR)
    {
        for (int i = 0; i < RT_STACK_WIDTH; ++i)
        {
            if (m_pNewTarget[i] && m_pNewTarget[i]->m_pTex)
            {
                m_pNewTarget[i]->m_ReqColor = m_pNewTarget[i]->m_pTex->GetClearColor();
            }
        }
    }

    if (nFlags & FRT_CLEAR_DEPTH)
    {
        m_pNewTarget[0]->m_fReqDepth = fDepth;
    }

    if (!(nFlags & FRT_CLEAR_IMMEDIATE))
    {
        m_pNewTarget[0]->m_ClearFlags = 0;
    }
    if ((nFlags & FRT_CLEAR_DEPTH) && m_pNewTarget[0]->m_pDepth)
    {
        m_pNewTarget[0]->m_ClearFlags |= CLEAR_ZBUFFER;
    }
    if (nFlags & FRT_CLEAR_COLOR)
    {
        m_pNewTarget[0]->m_ClearFlags |= CLEAR_RTARGET;
    }
    if (nFlags & FRT_CLEAR_COLORMASK)
    {
        m_pNewTarget[0]->m_ClearFlags |= FRT_CLEAR_COLORMASK;
    }

    if (m_sbpp && (nFlags & FRT_CLEAR_STENCIL) && m_pNewTarget[0]->m_pDepth)
    {
#ifdef SUPPORTS_MSAA
        if (gcpRendD3D->m_RP.m_MSAAData.Type)
        {
            m_RP.m_PersFlags2 |= RBPF2_MSAA_RESTORE_SAMPLE_MASK;
        }
#endif
        m_pNewTarget[0]->m_ClearFlags |= CLEAR_STENCIL;
        m_pNewTarget[0]->m_nReqStencil = nStencil;
    }
}

void CD3D9Renderer::EF_ClearTargetsLater(uint32 nFlags, const ColorF& Colors)
{
    EF_ClearTargetsLater(nFlags, Colors, Clr_FarPlane.r, 0);
    //      float(m_pNewTarget[0]->m_pSurfDepth->pTex->GetClearColor().r),
    //      uint8(m_pNewTarget[0]->m_pSurfDepth->pTex->GetClearColor().g));
}

void CD3D9Renderer::EF_ClearTargetsLater(uint32 nFlags)
{
    EF_ClearTargetsLater(nFlags, Clr_FarPlane.r, 0);
    //      float(m_pNewTarget[0]->m_pSurfDepth->pTex->GetClearColor().r),
    //      uint8(m_pNewTarget[0]->m_pSurfDepth->pTex->GetClearColor().g));
}


void CD3D9Renderer::FX_ClearTargetRegion(const uint32 nAdditionalStates /* = 0*/)
{
    assert(m_pRT->IsRenderThread());
    
    bool clearColor   = (m_pNewTarget[0]->m_ClearFlags & CLEAR_RTARGET) ? true : false;
    bool clearDepth   = (m_pNewTarget[0]->m_ClearFlags & CLEAR_ZBUFFER) ? true : false;
    bool clearStencil = (m_pNewTarget[0]->m_ClearFlags & CLEAR_STENCIL) ? true : false;
    
    ColorF colorValue = Clr_Empty;
    float  depthValue = 1.0f;
    uint8  stencilValue = 0;
    const char* clearTechnique = "Clear";
    
    if(clearColor)
    {
        colorValue = m_pNewTarget[0]->m_ReqColor;
        
        // Get number of render targets to clear
        int numRT = 0;
        for (int i = 0; i < RT_STACK_WIDTH; ++i)
        {
            if (m_pNewTarget[i] && m_pNewTarget[i]->m_pTarget)
            {
                numRT++;
                break;
            }
        }
        
        // Select the technique to clear the right amount of render targets
        switch(numRT)
        {
        case 0:
            AZ_Assert(false, "No color render target bound.");
            break;
        case 1:
            clearTechnique = "Clear";
            break;
        case 2:
            clearTechnique = "Clear2RT";
            break;
        case 3:
            clearTechnique = "Clear3RT";
            break;
        case 4:
            clearTechnique = "Clear4RT";
            break;
        default:
            AZ_Warning("Rendering", false, "More than 4 render targets bound. Only the first 4 will be cleared.");
            clearTechnique = "Clear4RT";
            break;
        }
    }
    
    if(clearDepth)
    {
        depthValue = ::FClamp(m_pNewTarget[0]->m_fReqDepth, 0.0f, 1.0f);
    }
    
    if(clearStencil)
    {
        stencilValue = m_pNewTarget[0]->m_nReqStencil;
    }

    CRenderObject* pObj = m_RP.m_pCurObject;
    CShader* pSHSave = m_RP.m_pShader;
    SShaderTechnique* pSHT = m_RP.m_pCurTechnique;
    SShaderPass* pPass = m_RP.m_pCurPass;
    CShaderResources* pShRes = m_RP.m_pShaderResources;

    gRenDev->m_cEF.mfRefreshSystemShader("Common", CShaderMan::s_ShaderCommon);

    m_RP.m_PersFlags1 |= RBPF1_IN_CLEAR;
    CShader* pSH = CShaderMan::s_ShaderCommon;
    uint32 nPasses = 0;
    pSH->FXSetTechnique(clearTechnique);
    pSH->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    pSH->FXBeginPass(0);
    
    int nState = 0;
    if (!clearColor)
    {
        nState |= GS_COLMASK_NONE;
    }
    
    if (clearDepth)
    {
        if (!clearColor && !clearStencil)
        {
            // If only clearing depth then we can optimize the draw by using not-equal comparison,
            // this way pixels with the same depth value as the clear value will be discarded.
            nState |= GS_DEPTHFUNC_NOTEQUAL;
        }
        else
        {
            nState |= GS_DEPTHFUNC_ALWAYS;
        }
        nState |= GS_DEPTHWRITE;
    }
    else
    {
        nState |= GS_NODEPTHTEST;
    }
    
    if (clearStencil)
    {
        int stencilState;
        if (!clearColor && !clearDepth)
        {
            // If only clearing stencil then we can optimize the draw by using not-equal comparison,
            // this way pixels with the same stencil value as the clear value will be discarded.
            stencilState =
                STENC_FUNC(FSS_STENCFUNC_NOTEQUAL) |
                STENCOP_FAIL(FSS_STENCOP_KEEP) |
                STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
                STENCOP_PASS(FSS_STENCOP_REPLACE);
        }
        else
        {
            stencilState =
                STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
                STENCOP_FAIL(FSS_STENCOP_REPLACE) |
                STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
                STENCOP_PASS(FSS_STENCOP_REPLACE);
        }
        
        const uint32 stencilMask = 0xFFFFFFFF;

        FX_SetStencilState(stencilState, stencilValue, stencilMask, stencilMask);
        nState |= GS_STENCIL;
    }

    m_pNewTarget[0]->m_ClearFlags = 0;

    nState |= nAdditionalStates;

    FX_SetState(nState, -1);
    D3DSetCull(eCULL_None);
    float fX = (float)m_CurViewport.nWidth;
    float fY = (float)m_CurViewport.nHeight;
    DrawQuad(-0.5f, -0.5f, fX - 0.5f, fY - 0.5f, colorValue, depthValue, fX, fY, fX, fY);
    m_RP.m_PersFlags1 &= ~RBPF1_IN_CLEAR;

    m_RP.m_pCurObject = pObj;
    m_RP.m_pShader = pSHSave;
    m_RP.m_pCurTechnique = pSHT;
    m_RP.m_pCurPass = pPass;
    m_RP.m_pShaderResources = pShRes;
}

void CD3D9Renderer::FX_SetActiveRenderTargets(bool /*bAllowDip*/)
{
    DETAILED_PROFILE_MARKER("FX_SetActiveRenderTargets");
    if (m_RP.m_PersFlags1 & RBPF1_IN_CLEAR)
    {
        return;
    }
    FUNCTION_PROFILER_RENDER_FLAT
    bool bDirty = false;
    if (m_nMaxRT2Commit >= 0)
    {
        for (int i = 0; i <= m_nMaxRT2Commit; i++)
        {
            if (!m_pNewTarget[i]->m_bWasSetRT)
            {
                m_pNewTarget[i]->m_bWasSetRT = true;
                if (m_pNewTarget[i]->m_pTex)
                {
                    m_pNewTarget[i]->m_pTex->SetResolved(false);
                }
                m_pCurTarget[i] = m_pNewTarget[i]->m_pTex;
                bDirty = true;
#ifndef _RELEASE
                if (m_logFileHandle != AZ::IO::InvalidHandle)
                {
                    Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " +++ Set RT");
                    if (m_pNewTarget[i]->m_pTex)
                    {
                        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " '%s'", m_pNewTarget[i]->m_pTex->GetName());
                        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " Format:%s", CTexture::NameForTextureFormat(m_pNewTarget[i]->m_pTex->m_eTFDst));
                        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " Type:%s", CTexture::NameForTextureType(m_pNewTarget[i]->m_pTex->m_eTT));
                        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " W/H:%d:%d\n",  m_pNewTarget[i]->m_pTex->GetWidth(), m_pNewTarget[i]->m_pTex->GetHeight());
                    }
                    else
                    {
                        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], " 'Unknown'\n");
                    }
                }
#endif
                CTexture* pRT = m_pNewTarget[i]->m_pTex;
                if (pRT && pRT->UseMultisampledRTV())
                {
                    pRT->Unbind();
                }
            }
        }
        if (!m_pNewTarget[0]->m_bWasSetD)
        {
            m_pNewTarget[0]->m_bWasSetD = true;
            bDirty = true;
        }
        //m_nMaxRT2Commit = -1;
    }

    if (bDirty)
    {
        CTexture* pRT = m_pNewTarget[0]->m_pTex;
        if (pRT && pRT->UseMultisampledRTV())
        {
            // Reset all texture slots which are used as RT currently
            D3DShaderResourceView* pRes = NULL;
            for (int i = 0; i < MAX_TMU; i++)
            {
                if (CTexture::s_TexStages[i].m_DevTexture == pRT->GetDevTexture())
                {
                    m_DevMan.BindSRV(eHWSC_Pixel, pRes, i);
                    CTexture::s_TexStages[i].m_DevTexture = NULL;
                }
            }
        }

        const uint32 nMaxRT2Commit = max(m_nMaxRT2Commit + 1, 0);

        ID3D11RenderTargetView* pRTV[RT_STACK_WIDTH] = { NULL };
        uint32 nNumViews = 0;

        for (uint32 v = 0; v < nMaxRT2Commit; ++v)
        {
            if (m_pNewTarget[v] && m_pNewTarget[v]->m_pTarget)
            {
                pRTV[v] = (ID3D11RenderTargetView*)m_pNewTarget[v]->m_pTarget;
                nNumViews = v + 1;
            }
        }

        GetDeviceContext().OMSetRenderTargets(m_pNewTarget[0]->m_pTarget == NULL ? 0 : nNumViews, pRTV, m_pNewTarget[0]->m_pDepth);
    }

    if (m_nMaxRT2Commit >= 0)
    {
        m_nMaxRT2Commit = -1;
    }

    FX_SetViewport();
    FX_ClearTargets();
}


void CD3D9Renderer::FX_SetViewport()
{
    // Set current viewport
    if (m_bViewportDirty)
    {
        m_bViewportDirty = false;
        if ((m_CurViewport != m_NewViewport))
        {
            m_CurViewport = m_NewViewport;
            D3DViewPort Port;
            Port.Width = (FLOAT)m_CurViewport.nWidth;
            Port.Height = (FLOAT)m_CurViewport.nHeight;
            Port.TopLeftX = (FLOAT)m_CurViewport.nX;
            Port.TopLeftY = (FLOAT)m_CurViewport.nY;
            Port.MinDepth = m_CurViewport.fMinZ;
            Port.MaxDepth = m_CurViewport.fMaxZ;

            if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
            {
                Port = ReverseDepthHelper::Convert(Port);
            }

            GetDeviceContext().RSSetViewports(1, &Port);
        }
    }
}

void CD3D9Renderer::FX_ClearTarget(D3DSurface* pView, const ColorF& cClear, const uint numRects, [[maybe_unused]] const RECT* pRects)
{
#if !defined(EXCLUDE_RARELY_USED_R_STATS) && defined(ENABLE_PROFILING_CODE)
    {
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCleared++;
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize += CDeviceTexture::TextureDataSize(pView, numRects, pRects);
    }
#endif

#if CRY_USE_DX12
    GetDeviceContext().ClearRectsRenderTargetView(
        pView,
        (const FLOAT*)&cClear,
        numRects,
        pRects);
#elif defined(DEVICE_SUPPORTS_D3D11_1) && D3DFXPIPELINE_CPP_TRAIT_CLEARVIEW
    GetDeviceContext().ClearView(
        pView,
        (const FLOAT*)&cClear,
        pRects,
        numRects);
#else
    if (!numRects)
    {
        GetDeviceContext().ClearRenderTargetView(
            pView,
            (const FLOAT*)&cClear);

        return;
    }

    // TODO: implement clears in compute for DX11, gives max performance (pipeline switch cost?)
    __debugbreak();
    abort();
#endif
}

void CD3D9Renderer::FX_ClearTarget(ITexture* tex, const ColorF& cClear, [[maybe_unused]] const uint numRects, const RECT* pRects, [[maybe_unused]] const bool bOptional)
{
    CTexture* pTex = reinterpret_cast<CTexture*>(tex);

    // TODO: should not happen, happens in the editor currently
    if (!pTex->GetDeviceRT())
    {
        pTex->GetSurface(-1, 0);
    }

#if CRY_USE_DX12
    FX_ClearTarget(
        pTex->GetDeviceRT(),
        cClear,
        numRects,
        pRects);
#else
    if (bOptional)
    {
        FX_ClearTarget(
            pTex->GetDeviceRT(),
            cClear,
            0U,
            nullptr);

        return;
    }

    // TODO: implement depth-clear as depth-only for DX11, gives max performance and probably just resets the depth-surface meta-data
    int ox, oy, ow, oh;
    FX_PushRenderTarget(0, pTex, nullptr);
    GetViewport(&ox, &oy, &ow, &oh);
    RT_SetViewport(pRects->left, pRects->top, pRects->right - pRects->left, pRects->bottom - pRects->top);
    FX_SetActiveRenderTargets();
    EF_ClearTargetsLater(FRT_CLEAR_COLOR, cClear);
    FX_ClearTargetRegion();
    FX_PopRenderTarget(0);
    SetViewport(ox, oy, ow, oh);
#endif
}

void CD3D9Renderer::FX_ClearTarget(ITexture* pTex, const ColorF& cClear)
{
    FX_ClearTarget(
        pTex,
        cClear,
        0U,
        nullptr,
        true);
}

void CD3D9Renderer::FX_ClearTarget(ITexture* pTex)
{
    FX_ClearTarget(
        pTex,
        pTex->GetClearColor());
}

//====================================================================================

void CD3D9Renderer::FX_ClearTarget(D3DDepthSurface* pView, const int nFlags, const float cDepth, const uint8 cStencil, const uint numRects, [[maybe_unused]] const RECT* pRects)
{
#if !defined(EXCLUDE_RARELY_USED_R_STATS) && defined(ENABLE_PROFILING_CODE)
    if (nFlags)
    {
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCleared++;
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize += CDeviceTexture::TextureDataSize(pView, numRects, pRects);
    }
#endif

    assert((
            (nFlags & CLEAR_ZBUFFER ? D3D11_CLEAR_DEPTH : 0) |
            (nFlags & CLEAR_STENCIL ? D3D11_CLEAR_STENCIL : 0)
            ) == nFlags);

#if CRY_USE_DX12
    GetDeviceContext().ClearRectsDepthStencilView(
        pView,
        nFlags,
        cDepth,
        cStencil,
        numRects,
        pRects);
#else
    if (!numRects)
    {
        GetDeviceContext().ClearDepthStencilView(
            pView,
            nFlags,
            cDepth,
            cStencil);

        return;
    }

    // TODO: implement clears in compute for DX11, gives max performance (pipeline switch cost?)
    __debugbreak();
    abort();
#endif
}

void CD3D9Renderer::FX_ClearTarget(SDepthTexture* pTex, const int nFlags, const float cDepth, const uint8 cStencil, [[maybe_unused]] const uint numRects, const RECT* pRects, [[maybe_unused]] const bool bOptional)
{
    assert((
            (nFlags & CLEAR_ZBUFFER ? D3D11_CLEAR_DEPTH : 0) |
            (nFlags & CLEAR_STENCIL ? D3D11_CLEAR_STENCIL : 0)
            ) == nFlags);

#if CRY_USE_DX12
    FX_ClearTarget(
        pTex->pSurf,
        nFlags,
        cDepth,
        cStencil,
        numRects,
        pRects);
#else
    if (bOptional)
    {
        FX_ClearTarget(
            static_cast<D3DDepthSurface*>(pTex->pSurf),
            nFlags,
            cDepth,
            cStencil,
            0U,
            nullptr);

        return;
    }

    // TODO: implement depth-clear as depth-only for DX11, gives max performance and probably just resets the depth-surface meta-data
    int ox, oy, ow, oh;
    FX_PushRenderTarget(0, (D3DSurface*)nullptr, pTex);
    GetViewport(&ox, &oy, &ow, &oh);
    RT_SetViewport(pRects->left, pRects->top, pRects->right - pRects->left, pRects->bottom - pRects->top);
    FX_SetActiveRenderTargets();
    EF_ClearTargetsLater(nFlags, Clr_Empty, cDepth, cStencil);
    FX_ClearTargetRegion();
    FX_PopRenderTarget(0);
    SetViewport(ox, oy, ow, oh);
#endif
}

void CD3D9Renderer::FX_ClearTarget(SDepthTexture* pTex, const int nFlags, const float cDepth, const uint8 cStencil)
{
    FX_ClearTarget(
        pTex,
        nFlags,
        cDepth,
        cStencil,
        0U,
        nullptr,
        true);
}

void CD3D9Renderer::FX_ClearTarget(SDepthTexture* pTex, const int nFlags)
{
    FX_ClearTarget(
        pTex,
        nFlags,
        (gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) ? 0.f : 1.f,
        0U);
}

void CD3D9Renderer::FX_ClearTarget(SDepthTexture* pTex)
{
    FX_ClearTarget(
        pTex,
        CLEAR_ZBUFFER |
        CLEAR_STENCIL);
}

//====================================================================================

void CD3D9Renderer::FX_ClearTargets()
{
    if (m_pNewTarget[0]->m_ClearFlags)
    {
        {
            const float fClearDepth = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) ? 1.0f - m_pNewTarget[0]->m_fReqDepth : m_pNewTarget[0]->m_fReqDepth;
            const uint8 nClearStencil = m_pNewTarget[0]->m_nReqStencil;
            const int nFlags = m_pNewTarget[0]->m_ClearFlags & ~CLEAR_RTARGET;

            // TODO: ClearFlags per render-target
            if ((m_pNewTarget[0]->m_pTarget != NULL) && (m_pNewTarget[0]->m_ClearFlags & CLEAR_RTARGET))
            {
                for (int i = 0; i < RT_STACK_WIDTH; ++i)
                {
                    if (m_pNewTarget[i]->m_pTarget)
                    {
                        // NOTE: optimal value is "m_pNewTarget[0]->m_pTex->GetClearColor()"
                        GetDeviceContext().ClearRenderTargetView(m_pNewTarget[i]->m_pTarget, &m_pNewTarget[i]->m_ReqColor[0]);
                    }
                }
            }

            assert((
                    (nFlags & FRT_CLEAR_DEPTH ? D3D11_CLEAR_DEPTH : 0) |
                    (nFlags & FRT_CLEAR_STENCIL ? D3D11_CLEAR_STENCIL : 0)
                    ) == nFlags);

            AZ_Warning("CD3D9Renderer", m_pNewTarget[0]->m_pDepth != nullptr, "FX_ClearTargets: Depth texture of target was nullptr. The depth target will not be cleared.");
            if (nFlags && m_pNewTarget[0]->m_pDepth != nullptr)
            {
                GetDeviceContext().ClearDepthStencilView(m_pNewTarget[0]->m_pDepth, nFlags, fClearDepth, nClearStencil);
            }
        }

        CTexture* pRT = m_pNewTarget[0]->m_pTex;
        if IsCVarConstAccess(constexpr) (CV_r_stats == 13)
        {
            EF_AddRTStat(pRT, m_pNewTarget[0]->m_ClearFlags, m_CurViewport.nWidth, m_CurViewport.nHeight);
        }

#if !defined(EXCLUDE_RARELY_USED_R_STATS) && defined(ENABLE_PROFILING_CODE)
        {
            if ((m_pNewTarget[0]->m_pTarget != NULL) && (m_pNewTarget[0]->m_ClearFlags & CLEAR_RTARGET))
            {
                for (int i = 0; i < RT_STACK_WIDTH; ++i)
                {
                    if (m_pNewTarget[i]->m_pTarget)
                    {
                        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCleared++;
                        m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize += CDeviceTexture::TextureDataSize(m_pNewTarget[i]->m_pTarget);
                    }
                }
            }

            if (m_pNewTarget[0]->m_ClearFlags & (~CLEAR_RTARGET) && m_pNewTarget[0]->m_pSurfDepth != nullptr)
            {
                m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTCleared++;
                m_RP.m_PS[m_RP.m_nProcessThreadID].m_RTClearedSize += CDeviceTexture::TextureDataSize(m_pNewTarget[0]->m_pSurfDepth->pSurf);
            }
        }
#endif

        m_pNewTarget[0]->m_ClearFlags = 0;
    }
}

void CD3D9Renderer::FX_Commit(bool bAllowDIP)
{
    DETAILED_PROFILE_MARKER("FX_Commit");
    // Commit all changed shader parameters
    if (m_RP.m_nCommitFlags & FC_GLOBAL_PARAMS)
    {
        CHWShader_D3D::mfCommitParamsGlobal();
        m_RP.m_nCommitFlags &= ~FC_GLOBAL_PARAMS;
    }
    if (m_RP.m_nCommitFlags & FC_MATERIAL_PARAMS)
    {
        CHWShader_D3D::UpdatePerMaterialConstantBuffer();
        m_RP.m_nCommitFlags &= ~FC_MATERIAL_PARAMS;
    }
    AzRHI::ConstantBufferCache::GetInstance().CommitAll();

    // Commit all changed RT's
    if (m_RP.m_nCommitFlags & FC_TARGETS)
    {
        FX_SetActiveRenderTargets(bAllowDIP);
        m_RP.m_nCommitFlags &= ~FC_TARGETS;
    }

    // Adapt viewport dimensions if changed
    FX_SetViewport();

    // Clear rendertargets if requested
    FX_ClearTargets();
}


// Set current geometry culling modes
void CD3D9Renderer::D3DSetCull(ECull eCull, bool bSkipMirrorCull)
{
    FUNCTION_PROFILER_RENDER_FLAT
    if (eCull != eCULL_None && !bSkipMirrorCull)
    {
        if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MIRRORCULL)
        {
            eCull = (eCull == eCULL_Back) ? eCULL_Front : eCULL_Back;
        }
    }

    if (eCull == m_RP.m_eCull)
    {
        return;
    }

    SStateRaster RS = m_StatesRS[m_nCurStateRS];

    RS.Desc.FrontCounterClockwise = true;

    if (eCull == eCULL_None)
    {
        RS.Desc.CullMode = D3D11_CULL_NONE;
    }
    else
    {
        if (eCull == eCULL_Back)
        {
            RS.Desc.CullMode = D3D11_CULL_BACK;
        }
        else
        {
            RS.Desc.CullMode = D3D11_CULL_FRONT;
        }
    }
    SetRasterState(&RS);
    m_RP.m_eCull = eCull;
}

uint8 g_StencilFuncLookup[8] =
{
    D3D11_COMPARISON_ALWAYS,                // FSS_STENCFUNC_ALWAYS   0x0
    D3D11_COMPARISON_NEVER,                 // FSS_STENCFUNC_NEVER    0x1
    D3D11_COMPARISON_LESS,                  // FSS_STENCFUNC_LESS     0x2
    D3D11_COMPARISON_LESS_EQUAL,        // FSS_STENCFUNC_LEQUAL   0x3
    D3D11_COMPARISON_GREATER,               // FSS_STENCFUNC_GREATER  0x4
    D3D11_COMPARISON_GREATER_EQUAL, // FSS_STENCFUNC_GEQUAL   0x5
    D3D11_COMPARISON_EQUAL,                 // FSS_STENCFUNC_EQUAL    0x6
    D3D11_COMPARISON_NOT_EQUAL          // FSS_STENCFUNC_NOTEQUAL 0x7
};

uint8 g_StencilOpLookup[8] =
{
    D3D11_STENCIL_OP_KEEP,                  // FSS_STENCOP_KEEP    0x0
    D3D11_STENCIL_OP_REPLACE,               // FSS_STENCOP_REPLACE 0x1
    D3D11_STENCIL_OP_INCR_SAT,          // FSS_STENCOP_INCR    0x2
    D3D11_STENCIL_OP_DECR_SAT,          // FSS_STENCOP_DECR    0x3
    D3D11_STENCIL_OP_ZERO,                  // FSS_STENCOP_ZERO    0x4
    D3D11_STENCIL_OP_INCR,                  // FSS_STENCOP_INCR_WRAP 0x5
    D3D11_STENCIL_OP_DECR,                  // FSS_STENCOP_DECR_WRAP 0x6
    D3D11_STENCIL_OP_INVERT                 // FSS_STENCOP_INVERT 0x7
};

void CRenderer::FX_SetStencilState(int st, uint32 nStencRef, uint32 nStencMask, uint32 nStencWriteMask, bool bForceFullReadMask)
{
    FUNCTION_PROFILER_RENDER_FLAT

        PrefetchLine(g_StencilFuncLookup, 0);

    const uint32 nPersFlags2 = m_RP.m_PersFlags2;
    if (!bForceFullReadMask && !(nPersFlags2 & RBPF2_READMASK_RESERVED_STENCIL_BIT))
    {
        nStencMask &= ~BIT_STENCIL_RESERVED;
    }

    if (nPersFlags2 & RBPF2_WRITEMASK_RESERVED_STENCIL_BIT)
    {
        nStencWriteMask &= ~BIT_STENCIL_RESERVED;
    }

    nStencRef |= m_RP.m_CurStencilRefAndMask;

    SStateDepth DS = gcpRendD3D->m_StatesDP[gcpRendD3D->m_nCurStateDP];
    DS.Desc.StencilReadMask = nStencMask;
    DS.Desc.StencilWriteMask = nStencWriteMask;

    int nCurFunc = st & FSS_STENCFUNC_MASK;
    DS.Desc.FrontFace.StencilFunc       = (D3D11_COMPARISON_FUNC)g_StencilFuncLookup[nCurFunc];

    int nCurOp = (st & FSS_STENCFAIL_MASK) >> FSS_STENCFAIL_SHIFT;
    DS.Desc.FrontFace.StencilFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[nCurOp];

    nCurOp = (st & FSS_STENCZFAIL_MASK) >> FSS_STENCZFAIL_SHIFT;
    DS.Desc.FrontFace.StencilDepthFailOp    = (D3D11_STENCIL_OP)g_StencilOpLookup[nCurOp];

    nCurOp = (st & FSS_STENCPASS_MASK) >> FSS_STENCPASS_SHIFT;
    DS.Desc.FrontFace.StencilPassOp = (D3D11_STENCIL_OP)g_StencilOpLookup[nCurOp];

    if (!(st & FSS_STENCIL_TWOSIDED))
    {
        DS.Desc.BackFace = DS.Desc.FrontFace;
    }
    else
    {
        nCurFunc = (st & (FSS_STENCFUNC_MASK << FSS_CCW_SHIFT)) >> FSS_CCW_SHIFT;
        DS.Desc.BackFace.StencilFunc        =   (D3D11_COMPARISON_FUNC)g_StencilFuncLookup[nCurFunc];

        nCurOp = (st & (FSS_STENCFAIL_MASK << FSS_CCW_SHIFT)) >> (FSS_STENCFAIL_SHIFT + FSS_CCW_SHIFT);
        DS.Desc.BackFace.StencilFailOp  = (D3D11_STENCIL_OP)g_StencilOpLookup[nCurOp];

        nCurOp = (st & (FSS_STENCZFAIL_MASK << FSS_CCW_SHIFT)) >> (FSS_STENCZFAIL_SHIFT + FSS_CCW_SHIFT);
        DS.Desc.BackFace.StencilDepthFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[nCurOp];

        nCurOp = (st & (FSS_STENCPASS_MASK << FSS_CCW_SHIFT)) >> (FSS_STENCPASS_SHIFT + FSS_CCW_SHIFT);
        DS.Desc.BackFace.StencilPassOp  = (D3D11_STENCIL_OP)g_StencilOpLookup[nCurOp];
    }

    m_RP.m_CurStencRef = nStencRef;
    m_RP.m_CurStencMask = nStencMask;
    m_RP.m_CurStencWriteMask = nStencWriteMask;

    gcpRendD3D->SetDepthState(&DS, nStencRef);

    m_RP.m_CurStencilState = st;
}

void CD3D9Renderer::EF_Scissor(bool bEnable, int sX, int sY, int sWdt, int sHgt)
{
    m_pRT->RC_SetScissor(bEnable, sX, sY, sWdt, sHgt);
}

void CD3D9Renderer::RT_SetScissor(bool bEnable, int sX, int sY, int sWdt, int sHgt)
{
    FUNCTION_PROFILER_RENDER_FLAT
    if (!CV_r_scissor || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
    {
        return;
    }
    D3D11_RECT scRect;
    if (bEnable)
    {
        if (sX != m_sPrevX || sY != m_sPrevY || sWdt != m_sPrevWdt || sHgt != m_sPrevHgt)
        {
            m_sPrevX = sX;
            m_sPrevY = sY;
            m_sPrevWdt = sWdt;
            m_sPrevHgt = sHgt;
            scRect.left = sX;
            scRect.top = sY;
            scRect.right = sX + sWdt;
            scRect.bottom = sY + sHgt;

            GetDeviceContext().RSSetScissorRects(1, &scRect);
        }
        if (bEnable != m_bsPrev)
        {
            m_bsPrev = bEnable;
            SStateRaster RS = m_StatesRS[m_nCurStateRS];
            RS.Desc.ScissorEnable = bEnable;
            SetRasterState(&RS);
        }
    }
    else
    {
        if (bEnable != m_bsPrev)
        {
            m_bsPrev = bEnable;
            m_sPrevWdt = 0;
            m_sPrevHgt = 0;
            SStateRaster RS = m_StatesRS[m_nCurStateRS];
            RS.Desc.ScissorEnable = bEnable;
            SetRasterState(&RS);
        }
    }
}

bool CD3D9Renderer::EF_GetScissorState(int& sX, int& sY, int& sWdt, int& sHgt)
{
    if (!CV_r_scissor || (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN))
    {
        return false;
    }

    sX = m_sPrevX;
    sY = m_sPrevY;
    sWdt = m_sPrevWdt;
    sHgt = m_sPrevHgt;
    return m_bsPrev;
}

void CD3D9Renderer::FX_FogCorrection()
{
    if (m_RP.m_nPassGroupID <= EFSLIST_DECAL)
    {
        uint32 uBlendFlags = m_RP.m_CurState & GS_BLEND_MASK;
        switch (uBlendFlags)
        {
        case GS_BLSRC_ONE | GS_BLDST_ONE:
            EF_SetFogColor(Col_Black);
            break;
        case GS_BLSRC_DSTALPHA | GS_BLDST_ONE:
            EF_SetFogColor(Col_Black);
            break;
        case GS_BLSRC_DSTCOL | GS_BLDST_SRCCOL:
        {
            static ColorF pColGrey = ColorF(0.5f, 0.5f, 0.5f, 1.0f);
            EF_SetFogColor(pColGrey);
            break;
        }
        case GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA:
            EF_SetFogColor(Col_Black);
            break;
        case GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCCOL:
            EF_SetFogColor(Col_Black);
            break;
        case GS_BLSRC_ZERO | GS_BLDST_ONEMINUSSRCCOL:
            EF_SetFogColor(Col_Black);
            break;
        case GS_BLSRC_SRCALPHA | GS_BLDST_ONE:
        case GS_BLSRC_SRCALPHA_A_ZERO | GS_BLDST_ONE:
            EF_SetFogColor(Col_Black);
            break;
        case GS_BLSRC_ZERO | GS_BLDST_ONE:
            EF_SetFogColor(Col_Black);
            break;
        case GS_BLSRC_DSTCOL | GS_BLDST_ZERO:
            EF_SetFogColor(Col_White);
            break;
        default:
            EF_SetFogColor(m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_FogColor);
            break;
        }
    }
    else
    {
        EF_SetFogColor(m_RP.m_TI[m_RP.m_nProcessThreadID].m_FS.m_FogColor);
    }
}

// Set current render states
void CD3D9Renderer::FX_SetState(int st, int AlphaRef, int RestoreState)
{
    FUNCTION_PROFILER_RENDER_FLAT
    int Changed;
    if IsCVarConstAccess(constexpr) (CV_r_measureoverdraw == 4 && (st & GS_DEPTHFUNC_MASK) == GS_DEPTHFUNC_HIZEQUAL)
    {
        // disable fine depth test
        st |= GS_NODEPTHTEST;
    }
    st |= m_RP.m_StateOr;
    st &= m_RP.m_StateAnd;
    Changed = st ^ m_RP.m_CurState;
    Changed |= RestoreState;

    // Due to the way reverse depth was implemented, we need to check if RBPF_REVERSE_DEPTH has changed and force flush the depth state if so.
    uint32 changedPersFlags = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags ^ m_RP.m_previousPersFlags;
    m_RP.m_previousPersFlags = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags;

    if (!Changed && !changedPersFlags && (AlphaRef == -1 || AlphaRef == m_RP.m_CurAlphaRef))
    {
        return;
    }

    //PROFILE_FRAME(State_RStates);

#ifndef _RELEASE
    m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumStateChanges++;
#endif
    if (m_StatesBL.size() == 0 || m_StatesDP.size() == 0 || m_StatesRS.size() == 0)
    {
        SetDefaultRenderStates();
    }
    SStateDepth DS = m_StatesDP[m_nCurStateDP];
    SStateBlend BS = m_StatesBL[m_nCurStateBL];
    SStateRaster RS = m_StatesRS[m_nCurStateRS];
    bool bDirtyDS = false;
    bool bDirtyBS = false;
    bool bDirtyRS = false;

    if ((Changed & GS_DEPTHFUNC_MASK) || (changedPersFlags & RBPF_REVERSE_DEPTH))
    {
        bDirtyDS = true;

        uint32 nDepthState = st;
        if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
        {
            nDepthState = ReverseDepthHelper::ConvertDepthFunc(st);
        }

        switch (nDepthState & GS_DEPTHFUNC_MASK)
        {
        case GS_DEPTHFUNC_HIZEQUAL:
        case GS_DEPTHFUNC_EQUAL:
            DS.Desc.DepthFunc = D3D11_COMPARISON_EQUAL;
            break;
        case GS_DEPTHFUNC_LEQUAL:
            DS.Desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
            break;
        case GS_DEPTHFUNC_GREAT:
            DS.Desc.DepthFunc = D3D11_COMPARISON_GREATER;
            break;
        case GS_DEPTHFUNC_LESS:
            DS.Desc.DepthFunc = D3D11_COMPARISON_LESS;
            break;
        case GS_DEPTHFUNC_NOTEQUAL:
            DS.Desc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;
            break;
        case GS_DEPTHFUNC_GEQUAL:
            DS.Desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
            break;
        case GS_DEPTHFUNC_ALWAYS:
            DS.Desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
            break;
        }
    }
    if (Changed & (GS_WIREFRAME))
    {
        bDirtyRS = true;

        if (st & GS_WIREFRAME)
        {
            RS.Desc.FillMode = D3D11_FILL_WIREFRAME;
        }
        else
        {
            RS.Desc.FillMode = D3D11_FILL_SOLID;
        }
    }

    if (Changed & GS_COLMASK_MASK)
    {
        bDirtyBS = true;
        uint32 nMask = 0xfffffff0 | ((st & GS_COLMASK_MASK) >> GS_COLMASK_SHIFT);
        nMask = (~nMask) & 0xf;
        for (size_t i = 0; i < RT_STACK_WIDTH; ++i)
        {
            BS.Desc.RenderTarget[i].RenderTargetWriteMask = nMask;
        }
    }

    if (Changed & GS_BLEND_MASK)
    {
        bDirtyBS = true;
        if (st & GS_BLEND_MASK)
        {
            if IsCVarConstAccess(constexpr) (CV_r_measureoverdraw && (m_RP.m_nRendFlags & SHDF_ALLOWHDR))
            {
                st = (st & ~GS_BLEND_MASK) | (GS_BLSRC_ONE | GS_BLDST_ONE);
                st &= ~GS_ALPHATEST_MASK;
            }

            // todo: add separate alpha blend support for mrt
            for (size_t i = 0; i < RT_STACK_WIDTH; ++i)
            {
                BS.Desc.RenderTarget[i].BlendEnable = TRUE;
            }

            // Source factor
            switch (st & GS_BLSRC_MASK)
            {
            case GS_BLSRC_ZERO:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
                break;
            case GS_BLSRC_ONE:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                break;
            case GS_BLSRC_DSTCOL:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
                break;
            case GS_BLSRC_ONEMINUSDSTCOL:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
                break;
            case GS_BLSRC_SRCALPHA:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
                break;
            case GS_BLSRC_ONEMINUSSRCALPHA:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_SRC_ALPHA;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                break;
            case GS_BLSRC_DSTALPHA:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_ALPHA;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
                break;
            case GS_BLSRC_ONEMINUSDSTALPHA:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_ALPHA;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
                break;
            case GS_BLSRC_ALPHASATURATE:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA_SAT;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA_SAT;
                break;
            case GS_BLSRC_SRCALPHA_A_ZERO:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
                break;
            case GS_BLSRC_SRC1ALPHA:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC1_ALPHA;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC1_ALPHA;
                break;
            default:
                BS.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                break;
            }

            //Destination factor
            switch (st & GS_BLDST_MASK)
            {
            case GS_BLDST_ZERO:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
                break;
            case GS_BLDST_ONE:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
                break;
            case GS_BLDST_SRCCOL:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
                break;
            case GS_BLDST_ONEMINUSSRCCOL:
                if (m_nHDRType == 1 && (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_HDR))
                {
                    BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
                    BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
                }
                else
                {
                    BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
                    BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                }
                break;
            case GS_BLDST_SRCALPHA:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
                break;
            case GS_BLDST_ONEMINUSSRCALPHA:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
                break;
            case GS_BLDST_DSTALPHA:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_ALPHA;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
                break;
            case GS_BLDST_ONEMINUSDSTALPHA:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
                break;
            case GS_BLDST_ONE_A_ZERO:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
                break;
            case GS_BLDST_ONEMINUSSRC1ALPHA:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC1_ALPHA;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC1_ALPHA;
                break;
            default:
                BS.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
                break;
            }

            //Blending operation
            D3D11_BLEND_OP blendOperation = D3D11_BLEND_OP_ADD;
            D3D11_BLEND_OP blendOperationAlpha = D3D11_BLEND_OP_ADD;
            switch (st & GS_BLEND_OP_MASK)
            {
            case GS_BLOP_MAX:
                blendOperation = D3D11_BLEND_OP_MAX;
                blendOperationAlpha = D3D11_BLEND_OP_MAX;
                break;
            case GS_BLOP_MIN:
                blendOperation = D3D11_BLEND_OP_MIN;
                blendOperationAlpha = D3D11_BLEND_OP_MIN;
                break;
            }

            //Separate blend modes for alpha
            switch (st & GS_BLALPHA_MASK)
            {
            case GS_BLALPHA_MIN:
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                blendOperationAlpha = D3D11_BLEND_OP_MIN;
                break;
            case GS_BLALPHA_MAX:
                BS.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
                BS.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                blendOperationAlpha = D3D11_BLEND_OP_MAX;
                break;
            }

            // todo: add separate alpha blend support for mrt
            for (size_t i = 0; i < RT_STACK_WIDTH; ++i)
            {
                BS.Desc.RenderTarget[i].BlendOp = blendOperation;
                BS.Desc.RenderTarget[i].BlendOpAlpha = blendOperationAlpha;
            }
        }
        else
        {
            // todo: add separate alpha blend support for mrt
            for (size_t i = 0; i < RT_STACK_WIDTH; ++i)
            {
                BS.Desc.RenderTarget[i].BlendEnable = FALSE;
            }
        }

        // Need to disable color write to MRTs for shadow map alpha blending (not supported by all hw)
        if ((m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN) && m_pNewTarget[1])
        {
            bDirtyBS = true;
            uint32 nMask = 0xfffffff0 | ((st & GS_COLMASK_MASK) >> GS_COLMASK_SHIFT);
            nMask = (~nMask) & 0xf;
            BS.Desc.RenderTarget[0].RenderTargetWriteMask = nMask;
            if (st & GS_BLEND_MASK)
            {
                BS.Desc.IndependentBlendEnable = TRUE;
                for (size_t i = 1; i < RT_STACK_WIDTH; ++i)
                {
                    BS.Desc.RenderTarget[i].RenderTargetWriteMask = 0;
                    BS.Desc.RenderTarget[i].BlendEnable = FALSE;
                }
            }
            else
            {
                BS.Desc.IndependentBlendEnable = FALSE;
                for (size_t i = 1; i < RT_STACK_WIDTH; ++i)
                {
                    BS.Desc.RenderTarget[i].RenderTargetWriteMask = nMask;
                    BS.Desc.RenderTarget[i].BlendEnable = TRUE;
                }
            }
        }
    }

    m_RP.m_depthWriteStateUsed |= (st & GS_DEPTHWRITE) != 0;
    if (Changed & GS_DEPTHWRITE)
    {
        bDirtyDS = true;
        if (st & GS_DEPTHWRITE)
        {
            DS.Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        }
        else
        {
            DS.Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        }
    }

    if (Changed & GS_NODEPTHTEST)
    {
        bDirtyDS = true;
        if (st & GS_NODEPTHTEST)
        {
            DS.Desc.DepthEnable = FALSE;
        }
        else
        {
            DS.Desc.DepthEnable = TRUE;
        }
    }

    if (Changed & GS_STENCIL)
    {
        bDirtyDS = true;
        if (st & GS_STENCIL)
        {
            DS.Desc.StencilEnable = TRUE;
        }
        else
        {
            DS.Desc.StencilEnable = FALSE;
        }
    }

    {
        // Alpha test must be handled in shader in D3D10 API
        if (((st ^ m_RP.m_CurState) & GS_ALPHATEST_MASK)   || ((st & GS_ALPHATEST_MASK) && (m_RP.m_CurAlphaRef != AlphaRef && AlphaRef != -1)))
        {
            if (st & GS_ALPHATEST_MASK)
            {
                m_RP.m_CurAlphaRef = AlphaRef;
                m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];
            }
            else
            {
                m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_ALPHATEST];
            }
            // When alpha test is turned on or off just changing m_RP.m_FlagsShader_RT doesn't work unless
            // an update is triggered. Setting this flag appears to cause the correct update.
            SThreadInfo* const pShaderThreadInfo = &(m_RP.m_TI[m_RP.m_nProcessThreadID]);
            pShaderThreadInfo->m_PersFlags |= RBPF_FP_DIRTY;
        }
    }

    if (bDirtyDS)
    {
        SetDepthState(&DS, m_nCurStencRef);
    }
    if (bDirtyRS)
    {
        SetRasterState(&RS);
    }
    if (bDirtyBS)
    {
        SetBlendState(&BS);
    }

    m_RP.m_CurState = st;
}

void CD3D9Renderer::FX_ZState(uint32& state)
{
    assert(m_RP.m_pRootTechnique);      // cannot be 0 here

    if ((m_RP.m_pRootTechnique->m_Flags & (FHF_WASZWRITE | FHF_POSITION_INVARIANT))
        && m_RP.m_nPassGroupID == EFSLIST_GENERAL
        && SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] == 0
        && CV_r_usezpass)
    {
        if ((m_RP.m_nBatchFilter & (FB_GENERAL | FB_MULTILAYERS)) && (m_RP.m_nRendFlags & (SHDF_ALLOWHDR | SHDF_ALLOWPOSTPROCESS)))
        {
            if (!(m_RP.m_pRootTechnique->m_Flags & FHF_POSITION_INVARIANT))
            {
                if IsCVarConstAccess(constexpr) (CRenderer::CV_r_measureoverdraw == 4)
                {
                    // Hi-Z test only, fine depth test is disabled at the top of FX_SetState()
                    state |= GS_DEPTHFUNC_HIZEQUAL;
                }
                else
                {
                    state |= GS_DEPTHFUNC_EQUAL;
                }
            }
            state &= ~(GS_DEPTHWRITE | GS_ALPHATEST_MASK);
        }
    }
}

void CD3D9Renderer::FX_HairState(uint32& nState, const SShaderPass* pPass)
{
    if ((m_RP.m_nPassGroupID == EFSLIST_GENERAL || m_RP.m_nPassGroupID == EFSLIST_TRANSP) && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_ZPASS))
        && !(m_RP.m_PersFlags2 & (RBPF2_MOTIONBLURPASS)))
    {
        // reset quality settings. BEWARE: these are used by shadows as well
        m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_TILED_SHADING] | g_HWSR_MaskBit[HWSR_QUALITY1]);
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY];

        // force per object fog
        m_RP.m_FlagsShader_RT |= (g_HWSR_MaskBit[HWSR_FOG] | g_HWSR_MaskBit[HWSR_ALPHABLEND]);

        if (CV_r_DeferredShadingTiled && CV_r_DeferredShadingTiledHairQuality > 0)
        {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_TILED_SHADING];

            if (CV_r_DeferredShadingTiledHairQuality > 1)
            {
                m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_QUALITY1];
            }
        }

        if ((pPass->m_RenderState & GS_DEPTHFUNC_MASK) == GS_DEPTHFUNC_LESS)
        {
            nState = (nState & ~(GS_BLEND_MASK | GS_DEPTHFUNC_MASK));
            nState |= GS_DEPTHFUNC_LESS;

            if ((m_RP.m_nPassGroupID == EFSLIST_TRANSP) && (m_RP.m_pShader->m_Flags2 & EF2_DEPTH_FIXUP) && RenderCapabilities::SupportsDualSourceBlending())
            {
                nState |= GS_BLSRC_SRC1ALPHA | GS_BLDST_ONEMINUSSRC1ALPHA | GS_BLALPHA_MIN;
            }
            else
            {
                nState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
            }

            if (pPass->m_RenderState & GS_DEPTHWRITE)
            {
                nState |= GS_DEPTHWRITE;
            }
            else
            {
                nState &= ~GS_DEPTHWRITE;
            }
        }
        else
        {
            nState = (nState & ~(GS_BLEND_MASK | GS_DEPTHFUNC_MASK));
            nState |= GS_DEPTHFUNC_EQUAL /*| GS_DEPTHWRITE*/;
        }
    }
}

void CD3D9Renderer::FX_CommitStates(const SShaderTechnique* pTech, const SShaderPass* pPass, bool bUseMaterialState)
{
    FUNCTION_PROFILER_RENDER_FLAT
    uint32 State = 0;
    int AlphaRef = pPass->m_AlphaRef == 0xff ? -1 : pPass->m_AlphaRef;
    SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    if (rRP.m_pCurObject->m_RState)
    {
        switch (rRP.m_pCurObject->m_RState & OS_TRANSPARENT)
        {
        case OS_ALPHA_BLEND:
            State = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
            break;
        case OS_ADD_BLEND:
            // In HDR mode, this is equivalent to pure additive GS_BLSRC_ONE | GS_BLDST_ONE.
            State = GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCCOL;
            break;
        case OS_MULTIPLY_BLEND:
            State = GS_BLSRC_DSTCOL | GS_BLDST_SRCCOL;
            break;
        }

        if (rRP.m_pCurObject->m_RState & OS_NODEPTH_TEST)
        {
            State |= GS_NODEPTHTEST;
        }
        AlphaRef = 0;
    }
    else
    {
        State = pPass->m_RenderState;
    }

    if (bUseMaterialState && rRP.m_MaterialStateOr != 0)
    {
        if (rRP.m_MaterialStateOr & GS_ALPHATEST_MASK)
        {
            AlphaRef = rRP.m_MaterialAlphaRef;
        }
        State &= ~rRP.m_MaterialStateAnd;
        State |=  rRP.m_MaterialStateOr;
    }

    //This has higher priority than material states as for alphatested material
    //it is forced to use depth writing (FX_SetResourcesState)
    if (rRP.m_pCurObject->m_RState & OS_TRANSPARENT)
    {
        State &= ~GS_DEPTHWRITE;
    }

    if (!(pTech->m_Flags & FHF_POSITION_INVARIANT) && !(pPass->m_PassFlags & SHPF_FORCEZFUNC))
    {
        FX_ZState(State);
    }

    if (bUseMaterialState && (rRP.m_pCurObject->m_fAlpha < 1.0f) && !rRP.m_bIgnoreObjectAlpha)
    {
        if (pTech && pTech->m_NameCRC == m_techShadowGen)
        {
            // If rendering to a shadow map:
            State = (State | GS_DEPTHWRITE);
        }
        else
        {
            // If not rendering to a shadow map:
            State = (State & ~(GS_BLEND_MASK | GS_DEPTHWRITE)) | (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
        }
    }

    State &= ~rRP.m_ForceStateAnd;
    State |=  rRP.m_ForceStateOr;

    if (rRP.m_pShader->m_Flags2 & EF2_HAIR)
    {
        FX_HairState(State, pPass);
    }
    else if ((m_RP.m_nPassGroupID == EFSLIST_TRANSP) && (m_RP.m_nSortGroupID == 1) &&
             !(rRP.m_PersFlags2 & RBPF2_MOTIONBLURPASS) && !(m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN | RBPF_ZPASS)))
    {
        State &= ~GS_BLALPHA_MASK;

        // Depth fixup for transparent geometry
        if ((m_RP.m_pShader->m_Flags2 & EF2_DEPTH_FIXUP) && RenderCapabilities::SupportsDualSourceBlending())
        {
            if (rRP.m_pCurObject->m_RState & OS_ALPHA_BLEND || ((State & (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA)) == (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA)))
            {
                State &= ~(GS_NOCOLMASK_A | GS_BLSRC_MASK | GS_BLDST_MASK);
                State |= GS_BLSRC_SRC1ALPHA | GS_BLDST_ONEMINUSSRC1ALPHA;
                rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHABLEND] | g_HWSR_MaskBit[HWSR_DEPTHFIXUP];

                // min blending on depth values (alpha channel)
                State |= GS_BLALPHA_MIN;
            }
        }
    }

    if ((rRP.m_PersFlags2 & RBPF2_ALLOW_DEFERREDSHADING) && (rRP.m_pShader->m_Flags & EF_SUPPORTSDEFERREDSHADING))
    {
        if (rTI.m_PersFlags & RBPF_ZPASS)
        {
            if ((rRP.m_pShader->m_Flags & EF_DECAL))
            {
                State = (State & ~(GS_BLEND_MASK | GS_DEPTHWRITE | GS_DEPTHFUNC_MASK));
                State |= GS_DEPTHFUNC_LEQUAL | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
                rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHABLEND];
            }

            // Disable alpha writes - for alpha blend case we use default alpha value as a default power factor
            if (State & GS_BLEND_MASK)
            {
                State |= GS_COLMASK_RGB;
            }

            // Disable alpha testing/depth writes if geometry had a z-prepass
            if (!(rRP.m_PersFlags2 & RBPF2_ZPREPASS) && (rRP.m_RIs[0][0]->nBatchFlags & FB_ZPREPASS))
            {
                State &= ~(GS_DEPTHWRITE | GS_DEPTHFUNC_MASK | GS_ALPHATEST_MASK);
                State |= GS_DEPTHFUNC_EQUAL;
                rRP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_ALPHATEST];
            }
        }
    }

    {
        const AZ::u32 VelocityMask = FOB_MOTION_BLUR | FOB_VERTEX_VELOCITY | FOB_SKINNED;
        const AZ::u32 SoftwareSkinned = FOB_MOTION_BLUR | FOB_VERTEX_VELOCITY;
        if ((rRP.m_ObjFlags & VelocityMask) == SoftwareSkinned &&
            (rRP.m_PersFlags2 & RBPF2_MOTIONBLURPASS) != 0)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VERTEX_VELOCITY];
        }
    }

    if (rRP.m_PersFlags2 & RBPF2_CUSTOM_RENDER_PASS)
    {
        rRP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_customvisions == 2)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
            State |= GS_BLSRC_ONE | GS_BLDST_ONE;
        }
        else if IsCVarConstAccess(constexpr) (CRenderer::CV_r_customvisions == 3)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];

            // Ignore depth thresholding in Post3DRender
            if (rRP.m_PersFlags2 & RBPF2_POST_3D_RENDERER_PASS)
            {
                rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
            }
        }
    }

    if (m_NewViewport.fMaxZ <= 0.01f)
    {
        State &= ~GS_DEPTHWRITE;
    }

    // Intermediate solution to disable depth testing in 3D HUD
    if (rRP.m_pCurObject->m_ObjFlags & FOB_RENDER_AFTER_POSTPROCESSING)
    {
        State &= ~GS_DEPTHFUNC_MASK;
        State |= GS_NODEPTHTEST;
    }

    if (rRP.m_PersFlags2 & RBPF2_DISABLECOLORWRITES)
    {
        State &= ~GS_COLMASK_MASK;
        State |= GS_COLMASK_NONE;
    }

    FX_SetState(State, AlphaRef);

    if (State & GS_ALPHATEST_MASK)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];
    }

    int nBlend = rRP.m_CurState & (GS_BLEND_MASK & ~GS_BLALPHA_MASK);
    if (nBlend)
    {
        //set alpha blend shader flag when the blend mode for color is set to alpha blend.
        if (nBlend == (GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA)
            || nBlend == (GS_BLSRC_SRCALPHA | GS_BLDST_ONE)
            || nBlend == (GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA)
            || nBlend == (GS_BLSRC_SRCALPHA_A_ZERO | GS_BLDST_ONEMINUSSRCALPHA))
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHABLEND];
        }
    }

    // Enable position invariant flag to disable fast math on certain vertex shader operations that affect position calculations.
    // This fixes issues with geometry that renders in both z-prepass and any other pass from having precision
    // issues when executing different vertex shaders and expecting the same position output results.
    if (rRP.m_RIs[0][0]->nBatchFlags & FB_ZPREPASS)
    {
        rRP.m_FlagsShader_MDV |= MDV_POSITION_INVARIANT;
    }
}

//=====================================================================================

bool CD3D9Renderer::FX_GetTargetSurfaces(CTexture* pTarget, D3DSurface*& pTargSurf, [[maybe_unused]] SRTStack* pCur, int nCMSide, [[maybe_unused]] int nTarget, [[maybe_unused]] uint32 nTileCount)
{
    if (pTarget)
    {
        if (!CTexture::IsTextureExist(pTarget) && !pTarget->m_bNoDevTexture)
        {
            pTarget->CreateRenderTarget(eTF_Unknown, pTarget->GetClearColor());
        }

        if (!CTexture::IsTextureExist(pTarget))
        {
            return false;
        }
        pTargSurf = pTarget->GetSurface(nCMSide, 0);
    }
    else
    {
        pTargSurf = NULL;
    }
    return true;
}

bool CD3D9Renderer::FX_SetRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, [[maybe_unused]] uint32 nTileCount)
{
    if (nTarget >= RT_STACK_WIDTH || m_nRTStackLevel[nTarget] >= MAX_RT_STACK)
    {
        return false;
    }
    HRESULT hr = 0;
    SRTStack* pCur = &m_RTStack[nTarget][m_nRTStackLevel[nTarget]];
    pCur->m_pTarget = static_cast<D3DSurface*>(pTargetSurf);
    pCur->m_pSurfDepth = pDepthTarget;
    pCur->m_pDepth = pDepthTarget ? (D3DDepthSurface*)pDepthTarget->pSurf : NULL;
    pCur->m_pTex = NULL;

#ifdef _DEBUG
    if (m_nRTStackLevel[nTarget] == 0 && nTarget == 0)
    {
        assert(pCur->m_pTarget == m_pBackBuffer && (pDepthTarget == nullptr || pCur->m_pDepth == m_pNativeZBuffer));
    }
#endif

    pCur->m_bNeedReleaseRT = false;
    pCur->m_bWasSetRT = false;
    pCur->m_bWasSetD = false;
    m_pNewTarget[nTarget] = pCur;
    if (nTarget == 0)
    {
        m_RP.m_StateOr &= ~GS_COLMASK_NONE;
    }
    m_nMaxRT2Commit = max(m_nMaxRT2Commit, nTarget);
    m_RP.m_nCommitFlags |= FC_TARGETS;
    return (hr == S_OK);
}
bool CD3D9Renderer::FX_PushRenderTarget(int nTarget, void* pTargetSurf, SDepthTexture* pDepthTarget, uint32 nTileCount)
{
    assert(m_pRT->IsRenderThread());
    if (nTarget >= RT_STACK_WIDTH || m_nRTStackLevel[nTarget] >= MAX_RT_STACK)
    {
        return false;
    }
    m_nRTStackLevel[nTarget]++;
    return FX_SetRenderTarget(nTarget, pTargetSurf, pDepthTarget, nTileCount);
}

bool CD3D9Renderer::FX_SetRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, bool bPush, int nCMSide, bool bScreenVP, uint32 nTileCount)
{
    assert(!nTarget || !pDepthTarget);
    assert((unsigned int) nTarget < RT_STACK_WIDTH);

    if (pTarget && !(pTarget->GetFlags() & FT_USAGE_RENDERTARGET))
    {
        CryFatalError("Attempt to bind a non-render-target texture as a render-target");
    }

    if (pTarget && pDepthTarget)
    {
        if (pTarget->GetWidth() > pDepthTarget->nWidth || pTarget->GetHeight() > pDepthTarget->nHeight)
        {
            iLog->LogError("Error: RenderTarget '%s' size:%i x %i DepthSurface size:%i x %i \n", pTarget->GetName(), pTarget->GetWidth(), pTarget->GetHeight(), pDepthTarget->nWidth, pDepthTarget->nHeight);
        }
        assert(pTarget->GetWidth() <= pDepthTarget->nWidth);
        assert(pTarget->GetHeight() <= pDepthTarget->nHeight);
    }

    if (nTarget >= RT_STACK_WIDTH || m_nRTStackLevel[nTarget] >= MAX_RT_STACK)
    {
        return false;
    }

    SRTStack* pCur = &m_RTStack[nTarget][m_nRTStackLevel[nTarget]];
    D3DSurface* pTargSurf;

    if (pCur->m_pTex)
    {
        if (pCur->m_bNeedReleaseRT)
        {
            pCur->m_bNeedReleaseRT = false;
        }
        m_pNewTarget[0]->m_bWasSetRT = false;
        m_pNewTarget[0]->m_pTarget = NULL;

        pCur->m_pTex->DecrementRenderTargetUseCount();
    }

    if (!pTarget)
    {
        pTargSurf = NULL;
    }
    else
    {
        if (!FX_GetTargetSurfaces(pTarget, pTargSurf, pCur, nCMSide, nTarget, nTileCount))
        {
            return false;
        }
    }

    if (pTarget)
    {
        int nFrameID = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;
        if (pTarget && pTarget->m_nUpdateFrameID != nFrameID)
        {
            pTarget->m_nUpdateFrameID = nFrameID;
        }
    }

    if (!bPush && pDepthTarget && pDepthTarget->pSurf != pCur->m_pDepth)
    {
        //assert(pCur->m_pDepth == m_pCurDepth);
        //assert(pCur->m_pDepth != m_pZBuffer);   // Attempt to override default Z-buffer surface
        if (pCur->m_pSurfDepth)
        {
            pCur->m_pSurfDepth->bBusy = false;
        }
    }
    pCur->m_pDepth = pDepthTarget ? (D3DDepthSurface*)pDepthTarget->pSurf : NULL;
    pCur->m_ClearFlags = 0;
    pCur->m_pTarget = pTargSurf;
    pCur->m_bNeedReleaseRT = true;
    pCur->m_bWasSetRT = false;
    pCur->m_bWasSetD = false;
    pCur->m_bScreenVP = bScreenVP;

    if (pDepthTarget)
    {
        pDepthTarget->bBusy = true;
        pDepthTarget->nFrameAccess = m_RP.m_TI[m_RP.m_nProcessThreadID].m_nFrameUpdateID;
    }

    if (pTarget)
    {
        pCur->m_pTex = pTarget;
    }
    else if (pDepthTarget)
    {
        pCur->m_pTex = (CTexture*)pDepthTarget->pTex;
    }
    else
    {
        pCur->m_pTex = NULL;
    }

    if (pCur->m_pTex)
    {
        pCur->m_pTex->IncrementRenderTargetUseCount();
    }

    pCur->m_pSurfDepth = pDepthTarget;

    if (pTarget)
    {
        pCur->m_Width = pTarget->GetWidth();
        pCur->m_Height = pTarget->GetHeight();
    }
    else
    if (pDepthTarget)
    {
        pCur->m_Width = pDepthTarget->nWidth;
        pCur->m_Height = pDepthTarget->nHeight;
    }
    if (!nTarget)
    {
        if (bScreenVP)
        {
            RT_SetViewport(m_MainViewport.nX, m_MainViewport.nY, m_MainViewport.nWidth, m_MainViewport.nHeight);
        }
        else
        {
            RT_SetViewport(0, 0, pCur->m_Width, pCur->m_Height);
        }
    }
    m_pNewTarget[nTarget] = pCur;
    m_nMaxRT2Commit = max(m_nMaxRT2Commit, nTarget);
    m_RP.m_nCommitFlags |= FC_TARGETS;
    return true;
}

CTexture* CD3D9Renderer::FX_GetCurrentRenderTarget(int target)
{
    return m_RTStack[target][gcpRendD3D->m_nRTStackLevel[target]].m_pTex;
}

D3DSurface* CD3D9Renderer::FX_GetCurrentRenderTargetSurface(int target) const
{
    return m_RTStack[target][gcpRendD3D->m_nRTStackLevel[target]].m_pTarget;
}

void CD3D9Renderer::FX_SetColorDontCareActions(int const nTarget,
    [[maybe_unused]] bool const loadDontCare,
    [[maybe_unused]] bool const storeDontCare)
{
    assert((unsigned int) nTarget < RT_STACK_WIDTH);

    SRTStack* srt = m_pNewTarget[nTarget];
    assert(srt);

    if (srt->m_pTarget)
    {
        // Call appropriate extension depending on rendering platform
#ifdef CRY_USE_METAL
        DXMETALSetColorDontCareActions(srt->m_pTarget, loadDontCare, storeDontCare);
#endif
#if defined(ANDROID)
        DXGLSetColorDontCareActions(srt->m_pTarget, loadDontCare, storeDontCare);
#endif
    }
}

void CD3D9Renderer::FX_SetDepthDontCareActions(int const nTarget,
    [[maybe_unused]] bool const loadDontCare,
    [[maybe_unused]] bool const storeDontCare)
{
    assert((unsigned int) nTarget < RT_STACK_WIDTH);

    SRTStack* srt = m_pNewTarget[nTarget];
    assert(srt);

    if (srt->m_pDepth)
    {
        // Call appropriate extension depending on rendering platform
#ifdef CRY_USE_METAL
        DXMETALSetDepthDontCareActions(srt->m_pDepth, loadDontCare, storeDontCare);
#endif
#if defined(ANDROID)
        DXGLSetDepthDontCareActions(srt->m_pDepth, loadDontCare, storeDontCare);
#endif
    }
}

void CD3D9Renderer::FX_SetStencilDontCareActions(int const nTarget,
    [[maybe_unused]] bool const loadDontCare,
    [[maybe_unused]] bool const storeDontCare)
{
    assert((unsigned int) nTarget < RT_STACK_WIDTH);

    SRTStack* srt = m_pNewTarget[nTarget];
    assert(srt);

    if (srt->m_pDepth)
    {
        // Call appropriate extension depending on rendering platform
#ifdef CRY_USE_METAL
        DXMETALSetStencilDontCareActions(srt->m_pDepth, loadDontCare, storeDontCare);
#endif
#if defined(ANDROID)
        DXGLSetStencilDontCareActions(srt->m_pDepth, loadDontCare, storeDontCare);
#endif
    }
}

void CD3D9Renderer::FX_TogglePLS([[maybe_unused]] bool const enable)
{
#if defined(OPENGL_ES) && !defined(DESKTOP_GLES)
    DXGLTogglePLS(&GetDeviceContext(), enable);
#endif
}

bool CD3D9Renderer::FX_PushRenderTarget(int nTarget, CTexture* pTarget, SDepthTexture* pDepthTarget, int nCMSide, bool bScreenVP, uint32 nTileCount)
{
    assert(m_pRT->IsRenderThread());

    if (nTarget >= RT_STACK_WIDTH || m_nRTStackLevel[nTarget] == MAX_RT_STACK)
    {
        assert(0);
        return false;
    }
    m_nRTStackLevel[nTarget]++;
    return FX_SetRenderTarget(nTarget, pTarget, pDepthTarget, true, nCMSide, bScreenVP, nTileCount);
}

bool CD3D9Renderer::FX_RestoreRenderTarget(int nTarget)
{
    if (nTarget >= RT_STACK_WIDTH || m_nRTStackLevel[nTarget] < 0)
    {
        return false;
    }

    SRTStack* pCur = &m_RTStack[nTarget][m_nRTStackLevel[nTarget]];

    SRTStack* pPrev = &m_RTStack[nTarget][m_nRTStackLevel[nTarget] + 1];

    if (pPrev->m_bNeedReleaseRT)
    {
        pPrev->m_bNeedReleaseRT = false;
        if (pPrev->m_pTarget && pPrev->m_pTarget == m_pNewTarget[nTarget]->m_pTarget)
        {
            m_pNewTarget[nTarget]->m_bWasSetRT = false;

            pPrev->m_pTarget = NULL;
            m_pNewTarget[nTarget]->m_pTarget = NULL;
        }
    }

    if (nTarget == 0)
    {
        if (pPrev->m_pSurfDepth)
        {
            pPrev->m_pSurfDepth->bBusy = false;
            pPrev->m_pSurfDepth = NULL;
        }
    }
    if (pPrev->m_pTex)
    {
        pPrev->m_pTex->DecrementRenderTargetUseCount();
        pPrev->m_pTex = NULL;
    }
    if (!nTarget)
    {
        if (pCur->m_bScreenVP)
        {
            RT_SetViewport(m_MainViewport.nX, m_MainViewport.nY, m_MainViewport.nWidth, m_MainViewport.nHeight);
        }
        else
        if (!m_nRTStackLevel[nTarget])
        {
            RT_SetViewport(0, 0, m_backbufferWidth, m_backbufferHeight);
        }
        else
        {
            RT_SetViewport(0, 0, pCur->m_Width, pCur->m_Height);
        }
    }
    pCur->m_bWasSetD = false;
    pCur->m_bWasSetRT = false;
    m_pNewTarget[nTarget] = pCur;
    m_nMaxRT2Commit = max(m_nMaxRT2Commit, nTarget);

    m_RP.m_nCommitFlags |= FC_TARGETS;
    return true;
}
bool CD3D9Renderer::FX_PopRenderTarget(int nTarget)
{
    assert(m_pRT->IsRenderThread());
    if (m_nRTStackLevel[nTarget] <= 0)
    {
        assert(0);
        return false;
    }
    m_nRTStackLevel[nTarget]--;
    return FX_RestoreRenderTarget(nTarget);
}

//////////////////////////////////////////////////////////////////////////
// REFACTOR BEGIN: (bethelz) Move scratch depth pool into its own class.

SDepthTexture* CD3D9Renderer::FX_GetDepthSurface(int nWidth, int nHeight, [[maybe_unused]] bool bAA, bool shaderResourceView)
{
    assert(m_pRT->IsRenderThread());

    SDepthTexture* pSrf = NULL;
    D3D11_TEXTURE2D_DESC desc;
    uint32 i;
    int nBestX = -1;
    int nBestY = -1;
    for (i = 0; i < m_TempDepths.Num(); i++)
    {
        pSrf = m_TempDepths[i];
        if (!pSrf->bBusy && pSrf->pSurf)
        {
            // verify that this texture supports binding as a shader resource if requested
            pSrf->pTarget->GetDesc(&desc);
            if (shaderResourceView && !(desc.BindFlags & D3D11_BIND_SHADER_RESOURCE))
            {
                continue;
            }
            if (pSrf->nWidth == nWidth && pSrf->nHeight == nHeight)
            {
                nBestX = i;
                break;
            }
            if (nBestX < 0 && pSrf->nWidth == nWidth && pSrf->nHeight >= nHeight)
            {
                nBestX = i;
            }
            else
            if (nBestY < 0 && pSrf->nWidth >= nWidth && pSrf->nHeight == nHeight)
            {
                nBestY = i;
            }
        }
    }
    if (nBestX >= 0)
    {
        return m_TempDepths[nBestX];
    }
    if (nBestY >= 0)
    {
        return m_TempDepths[nBestY];
    }


    bool allowUsingLargerRT = true;

#if defined(CRY_OPENGL_DO_NOT_ALLOW_LARGER_RT)
    allowUsingLargerRT = false;
#elif defined(SUPPORT_D3D_DEBUG_RUNTIME)
    if (CV_d3d11_debugruntime)
    {
        allowUsingLargerRT = false;
    }
#endif

    if (allowUsingLargerRT)
    {
        for (i = 0; i < m_TempDepths.Num(); i++)
        {
            pSrf = m_TempDepths[i];
            // verify that this texture supports binding as a shader resource if requested
            pSrf->pTarget->GetDesc(&desc);
            if (shaderResourceView && !(desc.BindFlags & D3D11_BIND_SHADER_RESOURCE))
            {
                continue;
            }
            if (pSrf->nWidth >= nWidth && pSrf->nHeight >= nHeight && !pSrf->bBusy)
            {
                break;
            }
        }
    }
    else
    {
        i = m_TempDepths.Num();
    }

    if (i == m_TempDepths.Num())
    {
        pSrf = CreateDepthSurface(nWidth, nHeight, shaderResourceView);
        if (pSrf != nullptr)
        {
            if (pSrf->pSurf != nullptr)
            {
                m_TempDepths.AddElem(pSrf);
            }
            else
            {
                DestroyDepthSurface(pSrf);
                pSrf = nullptr;
            }
        }
    }

    return pSrf;
}

// Commit changes states to the hardware before drawing
bool CD3D9Renderer::FX_CommitStreams(SShaderPass* sl, bool bSetVertexDecl)
{
    FUNCTION_PROFILER_RENDER_FLAT

    //PROFILE_FRAME(Draw_Predraw);
    SRenderPipeline& RESTRICT_REFERENCE rp(m_RP);

#if ENABLE_NORMALSTREAM_SUPPORT
    if (CHWShader_D3D::s_pCurInstHS)
    {
        rp.m_FlagsStreams_Stream |= (1 << VSF_NORMALS);
        rp.m_FlagsStreams_Decl |= (1 << VSF_NORMALS);
    }
#endif

    HRESULT hr;
    if (bSetVertexDecl)
    {
        if ((m_RP.m_ObjFlags & FOB_POINT_SPRITE) && !CHWShader_D3D::s_pCurInstHS)
        {
            rp.m_FlagsStreams_Stream |= VSM_INSTANCED;
            rp.m_FlagsStreams_Decl |= VSM_INSTANCED;
        }
        hr = FX_SetVertexDeclaration(rp.m_FlagsStreams_Decl, rp.m_CurVFormat);
        if (FAILED(hr))
        {
            return false;
        }
    }

    if (rp.m_pRE)
    {
        bool bRet = rp.m_pRE->mfPreDraw(sl);
        return bRet;
    }
    else if (rp.m_RendNumVerts && rp.m_RendNumIndices)
    {
        if (rp.m_FlagsPerFlush & RBSI_EXTERN_VMEM_BUFFERS)
        {
            assert(rp.m_pExternalVertexBuffer);
            assert(rp.m_pExternalIndexBuffer);

            // bind out external vertex/index buffer to use those directly, the client code has to set them up correctly
            rp.m_pExternalVertexBuffer->Bind(0, 0, rp.m_StreamStride);
            rp.m_pExternalIndexBuffer->Bind(0);

            // adjust the first index to render from as well as
            // other renderer stats
            rp.m_FirstIndex = rp.m_nExternalVertexBufferFirstIndex;
            rp.m_FirstVertex = rp.m_nExternalVertexBufferFirstVertex;

            rp.m_PS[rp.m_nProcessThreadID].m_DynMeshUpdateBytes += rp.m_StreamStride * rp.m_RendNumVerts;
            rp.m_PS[rp.m_nProcessThreadID].m_DynMeshUpdateBytes += rp.m_RendNumIndices * sizeof(short);

            // clear external video memory buffer flag
            rp.m_FlagsPerFlush &= ~RBSI_EXTERN_VMEM_BUFFERS;
            rp.m_nExternalVertexBufferFirstIndex = 0;
            rp.m_nExternalVertexBufferFirstVertex = 0;
            rp.m_pExternalVertexBuffer = NULL;
            rp.m_pExternalIndexBuffer = NULL;
        }
        else
        {
            /* NOTE:
             * It is extremely important that transient dynamic VBs are filled in binding order.
             * In the following case, rp.m_StreamPtr.Ptr verts data should be filled PRIOR the tangents.
             * This is due to underlying restrictions of certain rendering layers such as METAL.
             * The METAL renderer uses a ring buffer for transient data mapped to dynamic VBs.
             * To calculate the proper offsets when binding the buffers, it assumes the map/unmap
             * order is following an increasing VB slots binding.
             *
             * If the order is switched (tangents are filled before positions), the tangent data
             * will be used in the slot before the position which will result in a mismatch with
             * the expected IA layout. This will either cause artifacts or nothing to be rendered.
             */

            {
                TempDynVBAny::CreateFillAndBind(rp.m_StreamPtr.Ptr, rp.m_RendNumVerts, 0, rp.m_StreamStride);
                rp.m_FirstVertex = 0;
                rp.m_PS[rp.m_nProcessThreadID].m_DynMeshUpdateBytes += rp.m_RendNumVerts * rp.m_StreamStride;
            }

            if (rp.m_FlagsStreams_Stream & VSM_TANGENTS)
            {
                TempDynVB<SPipTangents>::CreateFillAndBind((const SPipTangents*) rp.m_StreamPtrTang.Ptr, rp.m_RendNumVerts, VSF_TANGENTS);
                rp.m_PersFlags1 |= RBPF1_USESTREAM << VSF_TANGENTS;
                rp.m_PS[rp.m_nProcessThreadID].m_DynMeshUpdateBytes += rp.m_RendNumVerts * sizeof(SPipTangents);
            }
            else
            if (rp.m_PersFlags1 & (RBPF1_USESTREAM << (VSF_TANGENTS | VSF_QTANGENTS)))
            {
                rp.m_PersFlags1 &= ~(RBPF1_USESTREAM << (VSF_TANGENTS | VSF_QTANGENTS));
                FX_SetVStream(1, NULL, 0, 0);
            }

            {
                TempDynIB16::CreateFillAndBind(rp.m_SysRendIndices, rp.m_RendNumIndices);
                rp.m_FirstIndex = 0;
                rp.m_PS[rp.m_nProcessThreadID].m_DynMeshUpdateBytes += rp.m_RendNumIndices * sizeof(short);
            }
        }
    }

    return true;
}

// Draw current indexed mesh
void CD3D9Renderer::FX_DrawIndexedMesh (const eRenderPrimitiveType nPrimType)
{
    DETAILED_PROFILE_MARKER("FX_DrawIndexedMesh");
    FX_Commit();

    // Don't render fallback in DX11
    if (!CHWShader_D3D::s_pCurInstVS || !CHWShader_D3D::s_pCurInstPS || CHWShader_D3D::s_pCurInstVS->m_bFallback || CHWShader_D3D::s_pCurInstPS->m_bFallback)
    {
        return;
    }
    if (CHWShader_D3D::s_pCurInstGS && CHWShader_D3D::s_pCurInstGS->m_bFallback)
    {
        return;
    }

    PROFILE_FRAME(Draw_DrawCall);

    if (nPrimType != eptHWSkinGroups)
    {
        eRenderPrimitiveType eType = nPrimType;
        int nFirstI = m_RP.m_FirstIndex;
        int nNumI = m_RP.m_RendNumIndices;
#ifdef TESSELLATION_RENDERER
        if (CHWShader_D3D::s_pCurInstHS)
        {
            FX_SetAdjacencyOffsetBuffer();
            eType = ept3ControlPointPatchList;
        }
#endif
        FX_DrawIndexedPrimitive(eType, 0, 0, m_RP.m_RendNumVerts, nFirstI, nNumI);

#if defined(ENABLE_PROFILING_CODE)
#   ifdef TESSELLATION_RENDERER
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygonsByTypes[m_RP.m_nPassGroupDIP][EVCT_STATIC][m_RP.m_nBatchFilter == FB_Z] += (nPrimType == eptTriangleList || nPrimType == ept3ControlPointPatchList ? nNumI / 3 : nNumI - 2);
#   else
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygonsByTypes[m_RP.m_nPassGroupDIP][EVCT_STATIC][m_RP.m_nBatchFilter == FB_Z] += (nPrimType == eptTriangleList ? nNumI / 3 : nNumI - 2);
#   endif
#endif
    }
    else
    {
        CRenderChunk* pChunk = m_RP.m_pRE->mfGetMatInfo();
        if (pChunk)
        {
            int nNumVerts = pChunk->nNumVerts;

            int nFirstIndexId = pChunk->nFirstIndexId;
            int nNumIndices = pChunk->nNumIndices;

            if (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN) && (gRenDev->m_RP.m_PersFlags2 & RBPF2_DISABLECOLORWRITES))
            {
                _smart_ptr<IMaterial> pMaterial = (m_RP.m_pCurObject) ? (m_RP.m_pCurObject->m_pCurrMaterial) : NULL;
                ((CREMeshImpl*)m_RP.m_pRE)->m_pRenderMesh->AddShadowPassMergedChunkIndicesAndVertices(pChunk, pMaterial, nNumVerts, nNumIndices);
            }

            eRenderPrimitiveType eType = eptTriangleList;

        #ifdef TESSELLATION_RENDERER
            if (CHWShader_D3D::s_pCurInstHS)
            {
                FX_SetAdjacencyOffsetBuffer();
                eType = ept3ControlPointPatchList;
            }
        #endif
            FX_DrawIndexedPrimitive(eType, 0, 0, nNumVerts, nFirstIndexId, nNumIndices);

        #if defined(ENABLE_PROFILING_CODE)
            m_RP.m_PS[m_RP.m_nProcessThreadID].m_nPolygonsByTypes[m_RP.m_nPassGroupDIP][EVCT_SKINNED][m_RP.m_nBatchFilter == FB_Z] += (pChunk->nNumIndices / 3);
        #endif
        }
    }
}

//====================================================================================


TArray<CRenderObject*> CD3D9Renderer::s_tempObjects[2];
TArray<SRendItem*> CD3D9Renderer::s_tempRIs;

// Actual drawing of instances
void CD3D9Renderer::FX_DrawInstances([[maybe_unused]] CShader* ef, SShaderPass* slw, [[maybe_unused]] int nRE, uint32 nStartInst, uint32 nLastInst, uint32 nUsedAttr, [[maybe_unused]] byte* InstanceData, int nInstAttrMask, byte Attributes[], [[maybe_unused]] short dwCBufSlot)
{
    DETAILED_PROFILE_MARKER("FX_DrawInstances");
    uint32 i;


    SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;


    if (!CHWShader_D3D::s_pCurInstVS || !CHWShader_D3D::s_pCurInstPS || CHWShader_D3D::s_pCurInstVS->m_bFallback || CHWShader_D3D::s_pCurInstPS->m_bFallback)
    {
        return;
    }

    PREFAST_SUPPRESS_WARNING(6326)
    if (!nStartInst)
    {
        // Set the stream 3 to be per instance data and iterate once per instance
        rRP.m_PersFlags1 &= ~(RBPF1_USESTREAM << 3);
        if (!FX_CommitStreams(slw, false))
        {
            return;
        }
        int StreamMask = rRP.m_FlagsStreams_Decl >> 1;
        SVertexDeclaration* vd = 0;
        // See if the desired vertex declaration already exists in m_CustomVD
        for (i = 0; i < rRP.m_CustomVD.Num(); i++)
        {
            vd = rRP.m_CustomVD[i];
            if (vd->StreamMask == StreamMask && rRP.m_CurVFormat == vd->VertexFormat && vd->InstAttrMask == nInstAttrMask && vd->m_vertexShader == CHWShader_D3D::s_pCurInstVS)
            {
                break;
            }
        }
        // If the vertex declaration was not found, create it
        if (i == rRP.m_CustomVD.Num())
        {
            vd = new SVertexDeclaration;
            rRP.m_CustomVD.AddElem(vd);
            vd->StreamMask = StreamMask;
            vd->VertexFormat = rRP.m_CurVFormat;
            vd->InstAttrMask = nInstAttrMask;
            vd->m_pDeclaration = NULL;
            vd->m_vertexShader = CHWShader_D3D::s_pCurInstVS;

            // Copy the base vertex format declaration
            SOnDemandD3DVertexDeclaration Decl;
            EF_OnDemandVertexDeclaration(Decl, StreamMask, rRP.m_CurVFormat, false, false);

            int nElementsToCopy = Decl.m_Declaration.size();
            for (i = 0; i < (uint32)nElementsToCopy; i++)
            {
                vd->m_Declaration.push_back(Decl.m_Declaration[i]);
            }

            // Add additional D3D11_INPUT_ELEMENT_DESCs with the TEXCOORD semantic to the end of the vertex declaration to handle the per instance data
            uint32 texCoordSemanticIndexOffset = rRP.m_CurVFormat.GetAttributeUsageCount(AZ::Vertex::AttributeUsage::TexCoord);
            D3D11_INPUT_ELEMENT_DESC elemTC = {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1}; // texture
            for (i = 0; i < nUsedAttr; i++)
            {
                elemTC.AlignedByteOffset = i * INST_PARAM_SIZE;
                elemTC.SemanticIndex = Attributes[i] + texCoordSemanticIndexOffset;
                vd->m_Declaration.push_back(elemTC);
            }
        }
        if (!vd->m_pDeclaration)
        {
            HRESULT hr = S_OK;
            assert (CHWShader_D3D::s_pCurInstVS && CHWShader_D3D::s_pCurInstVS->m_pShaderData);
            if (FAILED(hr = GetDevice().CreateInputLayout(&vd->m_Declaration[0], vd->m_Declaration.size(), CHWShader_D3D::s_pCurInstVS->m_pShaderData, CHWShader_D3D::s_pCurInstVS->m_nDataSize, &vd->m_pDeclaration)))
            {
                return;
            }
        }
        if (m_pLastVDeclaration != vd->m_pDeclaration)
        {
            m_pLastVDeclaration = vd->m_pDeclaration;
            m_DevMan.BindVtxDecl(vd->m_pDeclaration);
        }
    }

    int nInsts = nLastInst - nStartInst + 1;
    {
        //PROFILE_FRAME(Draw_ShaderIndexMesh);
#ifndef _RELEASE
        char instanceLabel[64];
        if (CV_r_geominstancingdebug)
        {
            snprintf(instanceLabel, 63, "Instances: %d", nInsts);
            PROFILE_LABEL_PUSH(instanceLabel);
        }
#endif

        assert (rRP.m_pRE && rRP.m_pRE->mfGetType() == eDATA_Mesh);
        FX_Commit();
        D3D11_PRIMITIVE_TOPOLOGY eTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
#ifdef TESSELLATION_RENDERER
        if (CHWShader_D3D::s_pCurInstHS)
        {
            FX_SetAdjacencyOffsetBuffer();
            eTopology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
        }
#endif
        SetPrimitiveTopology(eTopology);
        m_DevMan.DrawIndexedInstanced(rRP.m_RendNumIndices, nInsts, ApplyIndexBufferBindOffset(rRP.m_FirstIndex), 0, 0);

#ifndef _RELEASE
        if (CV_r_geominstancingdebug)
        {
            PROFILE_LABEL_POP(instanceLabel);
        }
#endif

#if defined(ENABLE_PROFILING_CODE)
        int nPolysPerInst = rRP.m_RendNumIndices / 3;
        int nPolysAll = nPolysPerInst * nInsts;
        rRP.m_PS[rRP.m_nProcessThreadID].m_nPolygons[rRP.m_nPassGroupDIP] += rRP.m_RendNumIndices / 3;
        rRP.m_PS[rRP.m_nProcessThreadID].m_nDIPs[rRP.m_nPassGroupDIP] += nInsts;
        rRP.m_PS[rRP.m_nProcessThreadID].m_nPolygons[rRP.m_nPassGroupDIP] += nPolysAll;
        rRP.m_PS[rRP.m_nProcessThreadID].m_nInsts += nInsts;
        rRP.m_PS[rRP.m_nProcessThreadID].m_nInstCalls++;
#endif
    }
}

#define MAX_HWINST_PARAMS_CONST (240 - VSCONST_INSTDATA)

// Draw geometry instances in single DIP using HW geom. instancing (StreamSourceFreq)
void CD3D9Renderer::FX_DrawShader_InstancedHW(CShader* ef, SShaderPass* slw)
{
#if defined(HW_INSTANCING_ENABLED)
    PROFILE_FRAME(DrawShader_Instanced);

    SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    // Set culling mode
    if (!(rRP.m_FlagsPerFlush & RBSI_LOCKCULL))
    {
        if (slw->m_eCull != -1)
        {
            D3DSetCull((ECull)slw->m_eCull);
        }
    }

    bool bProcessedAll = true;

    uint32 i;

    SCGBind bind;
    byte Attributes[32];

    rRP.m_FlagsPerFlush |= RBSI_INSTANCED;

    TempDynInstVB vb(gcpRendD3D);

    uint32 nCurInst;
    byte* data = NULL;
    CShaderResources* pCurRes = rRP.m_pShaderResources;
    CShaderResources* pSaveRes = pCurRes;

    uint64 nRTFlags = rRP.m_FlagsShader_RT;
    uint64 nSaveRTFlags = nRTFlags;

    // batch further and send everything as if it's rotated (full 3x4 matrix), even if we could
    // just send position
    uint64* __restrict maskBit = g_HWSR_MaskBit;

    nRTFlags |= maskBit[HWSR_INSTANCING_ATTR];

    if IsCVarConstAccess(constexpr) (CV_r_geominstancingdebug > 1)
    {
        // !DEBUG0 && !DEBUG1 && DEBUG2 && DEBUG3
        nRTFlags &= ~(maskBit[HWSR_DEBUG0] | maskBit[HWSR_DEBUG1]);
        nRTFlags |= (maskBit[HWSR_DEBUG2] | maskBit[HWSR_DEBUG3]);
    }

    rRP.m_FlagsShader_RT = nRTFlags;

    if (CRenderer::CV_r_SlimGBuffer)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    short dwCBufSlot = 0;

    CHWShader_D3D* vp = (CHWShader_D3D*)slw->m_VShader;
    CHWShader_D3D* ps = (CHWShader_D3D*)slw->m_PShader;

    // Set Pixel shader and all associated textures
    // Note: Need to set pixel shader first to properly set up modifiers for vertex shader (see ShaderCore.cpp & ModificatorTC.cfi)
    if (!ps->mfSet(HWSF_SETTEXTURES))
    {
        rRP.m_FlagsShader_RT = nSaveRTFlags;

        rRP.m_pShaderResources = pSaveRes;
        rRP.m_PersFlags1 |= RBPF1_USESTREAM << 3;
        return;
    }

    // Set Vertex shader
    if (!vp->mfSet(HWSF_INSTANCED | HWSF_SETTEXTURES))
    {
        rRP.m_FlagsShader_RT = nSaveRTFlags;

        rRP.m_pShaderResources = pSaveRes;
        rRP.m_PersFlags1 |= RBPF1_USESTREAM << 3;
        return;
    }

    CHWShader_D3D::SHWSInstance* pVPInst = vp->m_pCurInst;
    if (!pVPInst || pVPInst->m_bFallback || (ps->m_pCurInst && ps->m_pCurInst->m_bFallback))
    {
        return;
    }

    CHWShader_D3D* curGS = (CHWShader_D3D*)slw->m_GShader;
    if (curGS)
    {
        curGS->mfSet(0);
        curGS->UpdatePerInstanceConstantBuffer();
    }
    else
    {
        CHWShader_D3D::mfBindGS(nullptr, nullptr);
    }

    CHWShader_D3D* pCurHS, * pCurDS;
    bool bTessEnabled = FX_SetTessellationShaders(pCurHS, pCurDS, slw);

    vp->UpdatePerInstanceConstantBuffer();
    ps->UpdatePerInstanceConstantBuffer();

    #ifdef TESSELLATION_RENDERER
    CHWShader_D3D* curCS = (CHWShader_D3D*)slw->m_CShader;
    if (curCS)
    {
        curCS->mfSetCS(0);
    }
    else
    {
        CHWShader_D3D::mfBindCS(nullptr, nullptr);
    }

    if (pCurDS)
    {
        pCurDS->UpdatePerInstanceConstantBuffer();
    }
    if (pCurHS)
    {
        pCurHS->UpdatePerInstanceConstantBuffer();
    }
    #endif

    // VertexDeclaration of MeshInstance always starts with InstMatrix which has 3 vector4, that's why nUsedAttr is 3.
    int32 nUsedAttr = 3, nInstAttrMask = 0;
    pVPInst->GetInstancingAttribInfo(Attributes, nUsedAttr, nInstAttrMask);

    IRenderElement* pRE = NULL;
    CRenderMesh* pRenderMesh = NULL;

    const int nLastRE = rRP.m_nLastRE;
    for (int nRE = 0; nRE <= nLastRE; nRE++)
    {
        uint32 nRIs = rRP.m_RIs[nRE].size();
        SRendItem** rRIs = &(rRP.m_RIs[nRE][0]);

        // don't process REs that don't make the cut for instancing.
        // these were batched with an instance-ready RE, so leave this to drop through into DrawBatch
        if (nRIs <= (uint32)CRenderer::m_iGeomInstancingThreshold)
        {
            bProcessedAll = false;
            continue;
        }

        CShaderResources* pRes = SRendItem::mfGetRes(rRIs[0]->SortVal);
        pRE = rRP.m_pRE = rRIs[0]->pElem;
        rRP.m_pCurObject = rRIs[0]->pObj;

        CREMeshImpl* __restrict pMesh = (CREMeshImpl*) pRE;

        pRE->mfPrepare(false);
        {
            if (pCurRes != pRes)
            {
                rRP.m_pShaderResources = pRes;
                CHWShader_D3D::UpdatePerMaterialConstantBuffer();

                vp->UpdatePerBatchConstantBuffer();
                if (vp->m_pCurInst)
                {
                    vp->mfSetSamplers(vp->m_pCurInst->m_pSamplers, eHWSC_Vertex);
                }
                ps->UpdatePerBatchConstantBuffer();
                if (ps->m_pCurInst)
                {
                    ps->mfSetSamplers(ps->m_pCurInst->m_pSamplers, eHWSC_Pixel);
                }
#ifdef TESSELLATION_RENDERER
                if (pCurDS && pCurDS->m_pCurInst)
                {
                    pCurDS->mfSetSamplers(pCurDS->m_pCurInst->m_pSamplers, eHWSC_Domain);
                }
#endif
                pCurRes = pRes;
            }

            if (pMesh->m_pRenderMesh != pRenderMesh)
            {
                // Create/Update video mesh (VB management)
                if (!pRE->mfCheckUpdate(rRP.m_FlagsStreams_Stream, rTI.m_nFrameUpdateID, bTessEnabled))
                {
                    rRP.m_FlagsShader_RT = nSaveRTFlags;

                    rRP.m_pShaderResources = pSaveRes;
                    rRP.m_PersFlags1 |= RBPF1_USESTREAM << 3;
                    return;
                }

                pRenderMesh = pMesh->m_pRenderMesh;
            }

            {
                nCurInst = 0;

                // Detects possibility of using attributes based instancing
                // If number of used attributes exceed 16 we can't use attributes based instancing (switch to constant based)
                int nStreamMask = rRP.m_FlagsStreams_Stream >> 1;
                int nVFormat = rRP.m_CurVFormat.GetEnum();
                uint32 nCO = 0;
                uint32 dwDeclarationSize = 0;
                if (dwDeclarationSize + nUsedAttr - 1 > 16)
                {
                    iLog->LogWarning("WARNING: Attributes based instancing cannot exceed 16 attributes (%s uses %d attr. + %d vertex decl.attr.)[VF: %d, SM: 0x%x]", vp->GetName(), nUsedAttr, dwDeclarationSize - 1, nVFormat, nStreamMask);
                }
                else
                {
                    while ((int)nCurInst < nRIs)
                    {
                        uint32 nLastInst = nRIs - 1;

                        {
                            uint32 nParamsPerInstAllowed = MAX_HWINST_PARAMS;
                            if ((nLastInst - nCurInst + 1) * nUsedAttr >= nParamsPerInstAllowed)
                            {
                                nLastInst = nCurInst + (nParamsPerInstAllowed / nUsedAttr) - 1;
                            }
                        }
                        {
                            vb.Allocate(nLastInst - nCurInst + 1, nUsedAttr * INST_PARAM_SIZE);
                            data = (byte*) vb.Lock();
                        }
                        CRenderObject* curObj = rRP.m_pCurObject;

                        // 3 float4 = inst Matrix
                        const AZ::u32 perInstanceStride = nUsedAttr * sizeof(float[4]);

                        // Fill the stream 3 for per-instance data
                        byte* pWalkData = data;
                        for (i = nCurInst; i <= nLastInst; i++)
                        {
                            CRenderObject* renderObject = rRIs[nCO++]->pObj;
                            rRP.m_pCurObject = renderObject;
                            AzRHI::SIMDCopy(pWalkData, renderObject->m_II.m_Matrix.GetData(), 3);

                            if (pVPInst->m_nParams_Inst >= 0)
                            {
                                SCGParamsGroup& Group = CGParamManager::s_Groups[pVPInst->m_nParams_Inst];
                                vp->UpdatePerInstanceConstants(eHWSC_Vertex, Group.pParams, Group.nParams, pWalkData);
                            }

                            pWalkData += perInstanceStride;
                        }
                        rRP.m_pCurObject = curObj;

                        vb.Unlock();

                        // Set the first stream to be the indexed data and render N instances
                        vb.Bind(3, nUsedAttr * INST_PARAM_SIZE);

                        vb.Release();

                        FX_DrawInstances(ef, slw, nRE, nCurInst, nLastInst, nUsedAttr, data, nInstAttrMask, Attributes, dwCBufSlot);

                        nCurInst = nLastInst + 1;
                    }
                }
            }
        }
    }
#ifdef TESSELLATION_RENDERER
    if (bTessEnabled)
    {
        CHWShader_D3D::mfBindDS(NULL, NULL);
        CHWShader_D3D::mfBindHS(NULL, NULL);
    }
#endif

    rRP.m_PersFlags1 |= RBPF1_USESTREAM << 3;
    rRP.m_pShaderResources = pSaveRes;
    rRP.m_nCommitFlags = FC_ALL;
    rRP.m_FlagsShader_RT = nSaveRTFlags;
    rRP.m_nNumRendPasses++;
    if (!bProcessedAll)
    {
        FX_DrawBatches(ef, slw);
    }
#else
    CryFatalError("HW Instancing not supported on this platform");
#endif
}
//#endif

//====================================================================================

byte CD3D9Renderer::FX_StartQuery(SRendItem* pRI)
{
    if (!CV_r_ConditionalRendering || !(m_RP.m_nBatchFilter & (FB_Z | FB_GENERAL)))
    {
        return 0;
    }
#if !defined(NULL_RENDERER)
    if (m_RP.m_nBatchFilter & FB_Z)
    {
        if (m_OcclQueriesUsed >= MAX_OCCL_QUERIES)
        {
            return 0;
        }

        assert(pRI->nOcclQuery > MAX_OCCL_QUERIES);
        uint32 nQuery = m_OcclQueriesUsed;
        ++m_OcclQueriesUsed;
        COcclusionQuery* pQ = &m_OcclQueries[nQuery];
        if (!pQ->IsCreated())
        {
            pQ->Create();
        }
        pQ->BeginQuery();
        pRI->nOcclQuery = nQuery;
#ifndef _RELEASE
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQIssued++;
#endif
        return 1;
    }
    else
    {
        if (pRI->nOcclQuery >= MAX_OCCL_QUERIES || pRI->nOcclQuery < 0)
        {
            return 0;
        }

        COcclusionQuery* pQ = &m_OcclQueries[pRI->nOcclQuery];
#ifndef _RELEASE
        CTimeValue Time = iTimer->GetAsyncTime();
#endif
        uint32 nPixels = pQ->GetVisibleSamples(CV_r_ConditionalRendering == 2 ? false : true);
#ifndef _RELEASE
        m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQStallTime += static_cast<int>(iTimer->GetAsyncTime().GetMilliSeconds() - Time.GetMilliSeconds());
#endif
        bool bReady = pQ->IsReady();
        if (!bReady)
        {
#ifndef _RELEASE
            m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQNotReady++;
#endif
            return 0;
        }
        if (nPixels == 0)
        {
#ifndef _RELEASE
            m_RP.m_PS[m_RP.m_nProcessThreadID].m_NumQOccluded++;
#endif
            return 2;
        }
        return 0;
    }
#endif
    return 0;
}

void CD3D9Renderer::FX_EndQuery(SRendItem* pRI, byte bStartQ)
{
    if (!bStartQ)
    {
        return;
    }

    assert(pRI->nOcclQuery < MAX_OCCL_QUERIES);
    COcclusionQuery* pQ = &m_OcclQueries[pRI->nOcclQuery];
    pQ->EndQuery();
}

void CD3D9Renderer::FX_DrawBatchesSkinned(CShader* pSh, SShaderPass* pPass, SSkinningData* pSkinningData)
{
    DETAILED_PROFILE_MARKER("FX_DrawBatchesSkinned");
    PROFILE_FRAME(DrawShader_BatchSkinned);

    SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    CHWShader_D3D* pCurVS = (CHWShader_D3D*)pPass->m_VShader;
    CHWShader_D3D* pCurPS = (CHWShader_D3D*)pPass->m_PShader;

    const int nThreadID = m_RP.m_nProcessThreadID;
    const int bRenderLog  = CRenderer::CV_r_log;
    CREMeshImpl* pRE = (CREMeshImpl*) rRP.m_pRE;
    CRenderObject* const pSaveObj = rRP.m_pCurObject;

    CHWShader_D3D* pCurGS = (CHWShader_D3D*)pPass->m_GShader;


    CRenderMesh* pRenderMesh = pRE->m_pRenderMesh;

    rRP.m_nNumRendPasses++;
    rRP.m_RendNumGroup = 0;
    rRP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_VERTEX_VELOCITY];

    if (pSkinningData->nHWSkinningFlags & eHWS_Skinning_Matrix)
    {
        rRP.m_FlagsShader_RT |= (g_HWSR_MaskBit[HWSR_SKINNING_MATRIX]);
    }
    else if (pSkinningData->nHWSkinningFlags & eHWS_Skinning_DQ_Linear)
    {
        rRP.m_FlagsShader_RT |= (g_HWSR_MaskBit[HWSR_SKINNING_DQ_LINEAR]);
    }
    else
    {
        rRP.m_FlagsShader_RT |= (g_HWSR_MaskBit[HWSR_SKINNING_DUAL_QUAT]);
    }

    bool bRes = pCurPS->mfSetPS(HWSF_SETTEXTURES);
    bRes &= pCurVS->mfSetVS(0);

    CHWShader_D3D* pCurHS, *pCurDS;
    bool bTessEnabled = FX_SetTessellationShaders(pCurHS, pCurDS, pPass);

    if (pCurGS)
    {
        bRes &= pCurGS->mfSetGS(0);
    }
    else
    {
        CHWShader_D3D::mfBindGS(NULL, NULL);
    }

    const uint numObjects = rRP.m_RIs[0].Num();

    if (!bRes)
    {
        goto done;
    }

    if (CHWShader_D3D::s_pCurInstVS && CHWShader_D3D::s_pCurInstVS->m_bFallback)
    {
        goto done;
    }

    // Create/Update video mesh (VB management)
    if (!pRE->mfCheckUpdate(m_RP.m_FlagsStreams_Stream, rTI.m_nFrameUpdateID, bTessEnabled))
    {
        goto done;
    }

    if (ShouldApplyFogCorrection())
    {
        FX_FogCorrection();
    }

    // Unlock all VB (if needed) and set current streams
    if (!FX_CommitStreams(pPass))
    {
        goto done;
    }

    for (uint nObj = 0; nObj < numObjects; ++nObj)
    {
        CRenderObject* pObject = rRP.m_RIs[0][nObj]->pObj;
        rRP.m_pCurObject = pObject;

#ifdef DO_RENDERSTATS
        if (FX_ShouldTrackStats())
        {
            FX_TrackStats(pObject, pRE->m_pRenderMesh);
        }
#endif

#ifdef DO_RENDERLOG
        if (bRenderLog >= 3)
        {
            Vec3 vPos = pObject->GetTranslation();
            Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "+++ HWSkin Group Pass %d (Obj: %d [%.3f, %.3f, %.3f])\n", m_RP.m_nNumRendPasses, pObject->m_Id, vPos[0], vPos[1], vPos[2]);
        }
#endif

        pCurVS->UpdatePerInstanceConstantBuffer();
        pCurPS->UpdatePerInstanceConstantBuffer();

        if (pCurGS)
        {
            pCurGS->UpdatePerInstanceConstantBuffer();
        }
        else
        {
            CHWShader_D3D::mfBindGS(NULL, NULL);
        }
#ifdef TESSELLATION_RENDERER
        if (pCurDS)
        {
            pCurDS->UpdatePerInstanceConstantBuffer();
        }
        else
        {
            CHWShader_D3D::mfBindDS(NULL, NULL);
        }
        if (pCurHS)
        {
            pCurHS->UpdatePerInstanceConstantBuffer();
        }
        else
        {
            CHWShader_D3D::mfBindHS(NULL, NULL);
        }
#endif

        AzRHI::ConstantBuffer* pBuffer[2] = { NULL };
        SRenderObjData* pOD = pObject->GetObjData();
        assert(pOD);
        if (pOD)
        {
            SSkinningData* const skinningData = pOD->m_pSkinningData;
            if (!pRE->BindRemappedSkinningData(skinningData->remapGUID))
            {
                continue;
            }

            pBuffer[0] = alias_cast<SCharInstCB*>(skinningData->pCharInstCB)->m_buffer;

#ifdef CRY_USE_METAL
            // Buffer is sometimes null... binding a null skinned VB will fail on METAL
            if (!pBuffer[0])
            {
                continue;
            }
#endif

            // get previous data for motion blur if available
            if (skinningData->pPreviousSkinningRenderData)
            {
                pBuffer[1] = alias_cast<SCharInstCB*>(skinningData->pPreviousSkinningRenderData->pCharInstCB)->m_buffer;
            }
        }
        else
        {
            continue;
        }

#ifndef _RELEASE
        rRP.m_PS[nThreadID].m_NumRendSkinnedObjects++;
#endif

        m_PerInstanceConstantBufferPool.SetConstantBuffer(rRP.m_RIs[0][nObj]);

        m_DevMan.BindConstantBuffer(eHWSC_Vertex, pBuffer[0], eConstantBufferShaderSlot_SkinQuat);
        m_DevMan.BindConstantBuffer(eHWSC_Vertex, pBuffer[1], eConstantBufferShaderSlot_SkinQuatPrev);
        {
            DETAILED_PROFILE_MARKER("DrawSkinned");
            if (rRP.m_pRE)
            {
                rRP.m_pRE->mfDraw(pSh, pPass);
            }
            else
            {
                FX_DrawIndexedMesh(pRenderMesh ? pRenderMesh->GetPrimitiveType() : eptTriangleList);
            }
        }
    }

done:

    m_DevMan.BindConstantBuffer(eHWSC_Vertex, nullptr, eConstantBufferShaderSlot_SkinQuat);
    m_DevMan.BindConstantBuffer(eHWSC_Vertex, nullptr, eConstantBufferShaderSlot_SkinQuatPrev);

    rRP.m_FlagsShader_MD &= ~HWMD_TEXCOORD_FLAG_MASK;
    rRP.m_pCurObject = pSaveObj;

#ifdef TESSELLATION_RENDERER
    if (bTessEnabled)
    {
        CHWShader_D3D::mfBindDS(NULL, NULL);
        CHWShader_D3D::mfBindHS(NULL, NULL);
    }
#endif
    rRP.m_RendNumGroup = -1;
}

#if defined(DO_RENDERSTATS)
void CD3D9Renderer::FX_TrackStats([[maybe_unused]] CRenderObject* pObj, [[maybe_unused]] IRenderMesh* pRenderMesh)
{
#if !defined(_RELEASE)
    SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
    if (pObj)
    {
        if (IRenderNode* pRenderNode = (IRenderNode*)pObj->m_pRenderNode)
        {
            //Add to per node map for r_stats 6
            if (CV_r_stats == 6 || m_pDebugRenderNode || m_bCollectDrawCallsInfoPerNode)
            {
                IRenderer::RNDrawcallsMapNode& drawCallsInfoPerNode = rRP.m_pRNDrawCallsInfoPerNode[m_RP.m_nProcessThreadID];

                IRenderer::RNDrawcallsMapNodeItor pItor = drawCallsInfoPerNode.find(pRenderNode);

                if (pItor != drawCallsInfoPerNode.end())
                {
                    SDrawCallCountInfo& pInfoDP = pItor->second;
                    pInfoDP.Update(pObj, pRenderMesh);
                }
                else
                {
                    SDrawCallCountInfo pInfoDP;
                    pInfoDP.Update(pObj, pRenderMesh);
                    drawCallsInfoPerNode.insert(IRenderer::RNDrawcallsMapNodeItor::value_type(pRenderNode, pInfoDP));
                }
            }

            //Add to per mesh map for perfHUD
            if (m_bCollectDrawCallsInfo)
            {
                IRenderer::RNDrawcallsMapMesh& drawCallsInfoPerMesh = rRP.m_pRNDrawCallsInfoPerMesh[m_RP.m_nProcessThreadID];

                IRenderer::RNDrawcallsMapMeshItor pItor = drawCallsInfoPerMesh.find(pRenderMesh);

                if (pItor != drawCallsInfoPerMesh.end())
                {
                    SDrawCallCountInfo& pInfoDP = pItor->second;
                    pInfoDP.Update(pObj, pRenderMesh);
                }
                else
                {
                    SDrawCallCountInfo pInfoDP;
                    pInfoDP.Update(pObj, pRenderMesh);
                    drawCallsInfoPerMesh.insert(IRenderer::RNDrawcallsMapMeshItor::value_type(pRenderMesh, pInfoDP));
                }
            }
        }
    }
#endif
}
#endif



bool CD3D9Renderer::FX_SetTessellationShaders(CHWShader_D3D*& pCurHS, CHWShader_D3D*& pCurDS, const SShaderPass* pPass)
{
    SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;

#ifdef TESSELLATION_RENDERER
    pCurHS = (CHWShader_D3D*)pPass->m_HShader;
    pCurDS = (CHWShader_D3D*)pPass->m_DShader;

    bool bTessEnabled = (pCurHS != NULL) && (pCurDS != NULL) && !(rRP.m_pCurObject->m_ObjFlags & FOB_NEAREST) && (rRP.m_pCurObject->m_ObjFlags & FOB_ALLOW_TESSELLATION);

#ifndef MOTIONBLUR_TESSELLATION
    bTessEnabled &= !(rRP.m_PersFlags2 & RBPF2_MOTIONBLURPASS);
#endif

    if (bTessEnabled && pCurHS->mfSetHS(0) && pCurDS->mfSetDS(HWSF_SETTEXTURES))
    {
        if (CV_r_tessellationdebug == 1)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG1];
        }

        return true;
    }

    CHWShader_D3D::mfBindHS(NULL, NULL);
    CHWShader_D3D::mfBindDS(NULL, NULL);
#endif

    rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NO_TESSELLATION];

    pCurHS = NULL;
    pCurDS = NULL;

    return false;
}

#ifdef TESSELLATION_RENDERER

void CD3D9Renderer::FX_SetAdjacencyOffsetBuffer()
{
#ifdef MESH_TESSELLATION_RENDERER
    if (m_RP.m_pRE && m_RP.m_pRE->mfGetType() == eDATA_Mesh)
    {
        CREMeshImpl* pMesh = (CREMeshImpl*) m_RP.m_pRE;
        // this buffer contains offset HS has to apply to SV_PrimitiveID it gets from HW. we need this because
        // sometimes we do not start rendering from the beginning of index buffer
        // AI AndreyK: probably texture buffer has to be replayed by per-instance constant
        m_DevMan.BindSRV(eHWSC_Hull, pMesh->m_tessCB.GetShaderResourceView(), 15);
    }
    else
    {
        m_DevMan.BindSRV(eHWSC_Hull, NULL, 15);
    }
#endif
}

#endif //#ifdef TESSELLATION_RENDERER

void CD3D9Renderer::FX_DrawBatches(CShader* pSh, SShaderPass* pPass)
{
    DETAILED_PROFILE_MARKER("FX_DrawBatches");
    FUNCTION_PROFILER_RENDER_FLAT

        SRenderPipeline& RESTRICT_REFERENCE rRP = m_RP;
    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];

    // Set culling mode
    if (!((rRP.m_FlagsPerFlush & RBSI_LOCKCULL) || (m_RP.m_PersFlags2 & RBPF2_LIGHTSHAFTS)))
    {
        if (pPass->m_eCull != -1)
        {
            D3DSetCull((ECull)pPass->m_eCull);
        }
    }

    bool bHWSkinning = FX_SetStreamFlags(pPass);
    SSkinningData* pSkinningData = NULL;
    if (bHWSkinning)
    {
        SRenderObjData* pOD = rRP.m_pCurObject->GetObjData();
        if (!pOD || !pOD->m_pSkinningData)
        {
            pSkinningData = pOD->m_pSkinningData;
            bHWSkinning = false;
            Warning("Warning: Skinned geometry used without character instance");
        }
    }
    IF(bHWSkinning && (rRP.m_pCurObject->m_ObjFlags & FOB_SKINNED) && !CV_r_character_nodeform, 0)
    {
        FX_DrawBatchesSkinned(pSh, pPass, pSkinningData);
    }
    else
    {
        DETAILED_PROFILE_MARKER("FX_DrawBatchesStatic");

        // Set shaders
        bool bRes = true;
        const int rStats = CV_r_stats;
        const int rLog = CV_r_log;

        CHWShader_D3D* pCurGS = (CHWShader_D3D*)pPass->m_GShader;

        if (pCurGS)
        {
            bRes &= pCurGS->mfSetGS(0);
        }
        else
        {
            CHWShader_D3D::mfBindGS(NULL, NULL);
        }

        CHWShader_D3D* pCurVS = (CHWShader_D3D*)pPass->m_VShader;
        CHWShader_D3D* pCurPS = (CHWShader_D3D*)pPass->m_PShader;
        bRes &= pCurPS->mfSetPS(HWSF_SETTEXTURES);
        bRes &= pCurVS->mfSetVS(HWSF_SETTEXTURES);

        CHWShader_D3D* pCurHS, * pCurDS;
        bool bTessEnabled = FX_SetTessellationShaders(pCurHS, pCurDS, pPass);

        if (bRes)
        {
            if (ShouldApplyFogCorrection())
            {
                FX_FogCorrection();
            }

            assert(rRP.m_pRE || !rRP.m_nLastRE);
            IRenderElement* pRE = rRP.m_pRE;
            IRenderElement* pRESave = pRE;
            CRenderObject* pSaveObj = rRP.m_pCurObject;
            CShaderResources* pCurRes = rRP.m_pShaderResources;
            CShaderResources* pSaveRes = pCurRes;
            for (int nRE = 0; nRE <= rRP.m_nLastRE; nRE++)
            {
                TArray<SRendItem*>& rRIs = rRP.m_RIs[nRE];
                if (!(rRP.m_FlagsPerFlush & RBSI_INSTANCED) || rRIs.size() <= (unsigned)CRenderer::m_iGeomInstancingThreshold)
                {
                    if (pRE)
                    {
                        // Check the material for this object and make sure that it is actually supposed to be casting a shadow.
                        const bool isShadowPass = m_RP.m_nPassGroupID == EFSLIST_SHADOW_GEN;
                        int objectMaterialID = rRIs[0]->pElem->mfGetMatId();
                        if (isShadowPass && (objectMaterialID != -1))
                        {
                            if (rRIs[0]->pObj->m_pCurrMaterial && (rRIs[0]->pObj->m_pCurrMaterial->GetSafeSubMtl(objectMaterialID)->GetFlags() & MTL_FLAG_NOSHADOW))
                            {
                                continue;
                            }
                        }

                        pRE = rRP.m_pRE = rRIs[0]->pElem;
                        rRP.m_pCurObject = rRIs[0]->pObj;
                        CShaderResources* pRes = (rRP.m_PersFlags2 & RBPF2_MATERIALLAYERPASS) ? rRP.m_pShaderResources : SRendItem::mfGetRes(rRIs[0]->SortVal);
                        uint32 nFrameID = rTI.m_nFrameUpdateID;
                        if (!pRE->mfCheckUpdate(rRP.m_FlagsStreams_Stream | 0x80000000, nFrameID, bTessEnabled))
                        {
                            continue;
                        }
                        if (nRE || rRP.m_nNumRendPasses || pCurRes != pRes) // Only static meshes (CREMeshImpl) can use geom batching
                        {
                            rRP.m_pShaderResources = pRes;
                            CHWShader_D3D::UpdatePerMaterialConstantBuffer();

                            pRE->mfPrepare(false);
                            CREMeshImpl* pM = (CREMeshImpl*)pRE;
                            if (pM->m_CustomData || pCurRes != pRes) // Custom data can indicate some shader parameters are from mesh
                            {
                                pCurVS->UpdatePerBatchConstantBuffer();
                                pCurPS->UpdatePerBatchConstantBuffer();
                                if (pCurPS->m_pCurInst)
                                {
                                    pCurPS->mfSetSamplers(pCurPS->m_pCurInst->m_pSamplers, eHWSC_Pixel);
                                }
                                if (pCurVS->m_pCurInst)
                                {
                                    pCurVS->mfSetSamplers(pCurVS->m_pCurInst->m_pSamplers, eHWSC_Vertex);
                                }
#ifdef TESSELLATION_RENDERER
                                if (pCurDS && pCurDS->m_pCurInst)
                                {
                                    pCurDS->mfSetSamplers(pCurDS->m_pCurInst->m_pSamplers, eHWSC_Domain);
                                }
#endif
                                pCurRes = pRes;
                            }
                        }
                    }

                    rRP.m_nNumRendPasses++;
                    // Unlock all VBs (if needed) and bind current streams
                    if (FX_CommitStreams(pPass))
                    {
                        uint32 nO;
                        const uint32 nNumRI = rRIs.Num();
                        CRenderObject* pObj = NULL;
                        CRenderObject::SInstanceInfo* pI;
#ifdef DO_RENDERSTATS
                        if ((CV_r_stats == 6 || m_pDebugRenderNode || m_bCollectDrawCallsInfo))
                        {
                            for (nO = 0; nO < nNumRI; nO++)
                            {
                                pObj = rRIs[nO]->pObj;

                                IRenderElement* pElemBase = rRIs[nO]->pElem;

                                if (pElemBase->mfGetType() == eDATA_Mesh)
                                {
                                    CREMeshImpl* pMesh = (CREMeshImpl*)pElemBase;
                                    IRenderMesh* pRenderMesh = pMesh ? pMesh->m_pRenderMesh : NULL;

                                    FX_TrackStats(pObj, pRenderMesh);
                                }
                            }
                        }
#endif

                        for (nO = 0; nO < nNumRI; ++nO)
                        {
                            pObj = rRIs[nO]->pObj;
                            rRP.m_pCurObject = pObj;
                            pI = &pObj->m_II;
                            byte bStartQ = FX_StartQuery(rRIs[nO]);
                            if (bStartQ == 2)
                            {
                                continue;
                            }

#ifdef DO_RENDERLOG
                            if (rLog >= 3)
                            {
                                Vec3 vPos = pObj->GetTranslation();
                                Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "+++ General Pass %d (Obj: %d [%.3f, %.3f, %.3f], %.3f)\n", m_RP.m_nNumRendPasses, pObj->m_Id, vPos[0], vPos[1], vPos[2], pObj->m_fDistance);
                            }
#endif

                            pCurVS->UpdatePerInstanceConstantBuffer();
                            pCurPS->UpdatePerInstanceConstantBuffer();

                            if (pCurGS)
                            {
                                pCurGS->UpdatePerInstanceConstantBuffer();
                            }
                            else
                            {
                                CHWShader_D3D::mfBindGS(NULL, NULL);
                            }
#ifdef TESSELLATION_RENDERER
                            if (pCurDS)
                            {
                                pCurDS->UpdatePerInstanceConstantBuffer();
                            }
                            if (pCurHS)
                            {
                                pCurHS->UpdatePerInstanceConstantBuffer();
                            }
#endif

                            AZ_Assert(rRIs[nO], "current render item is null");
                            m_PerInstanceConstantBufferPool.SetConstantBuffer(rRIs[nO]);

                            {
                                if (pRE)
                                {
                                    pRE->mfDraw(pSh, pPass);
                                }
                                else
                                {
                                    FX_DrawIndexedMesh(eptTriangleList);
                                }
                            }

                            m_RP.m_nCommitFlags &= ~(FC_TARGETS | FC_GLOBAL_PARAMS);
                            FX_EndQuery(rRIs[nO], bStartQ);
                        }

                        rRP.m_FlagsShader_MD &= ~(HWMD_TEXCOORD_FLAG_MASK);
                        if (pRE)
                        {
                            pRE->mfClearFlags(FCEF_PRE_DRAW_DONE);
                        }
                    }
                    rRP.m_pCurObject = pSaveObj;
                    rRP.m_pRE = pRESave;
                    rRP.m_pShaderResources = pSaveRes;
                }
            }
        }
#ifdef TESSELLATION_RENDERER
        if (bTessEnabled)
        {
            CHWShader_D3D::mfBindHS(NULL, NULL);
            CHWShader_D3D::mfBindDS(NULL, NULL);
        }
#endif
    }
    m_RP.m_nCommitFlags = FC_ALL;
}

//============================================================================================

void CD3D9Renderer::FX_DrawShader_General(CShader* ef, SShaderTechnique* pTech)
{
    SShaderPass* slw;
    int32 i;

    PROFILE_FRAME(DrawShader_Generic);


    EF_Scissor(false, 0, 0, 0, 0);

    if (pTech->m_Passes.Num())
    {
        slw = &pTech->m_Passes[0];
        const int nCount = pTech->m_Passes.Num();
        uint32 curPassBit = 1;
        for (i = 0; i < nCount; i++, slw++, (curPassBit <<= 1))
        {
            m_RP.m_pCurPass = slw;

            // Set all textures and HW TexGen modes for the current pass (ShadeLayer)
            assert (slw->m_VShader && slw->m_PShader);
            if (!slw->m_VShader || !slw->m_PShader || (curPassBit & m_RP.m_CurPassBitMask))
            {
                continue;
            }

            FX_CommitStates(pTech, slw, (slw->m_PassFlags & SHPF_NOMATSTATE) == 0);

            bool bSkinned = (m_RP.m_pCurObject->m_ObjFlags & FOB_SKINNED) && !CV_r_character_nodeform;

            bSkinned |= FX_SetStreamFlags(slw);

            if (m_RP.m_FlagsPerFlush & RBSI_INSTANCED && !bSkinned)
            {
                // Using HW geometry instancing approach
                FX_DrawShader_InstancedHW(ef, slw);
            }
            else
            {
                FX_DrawBatches(ef, slw);
            }
        }
    }
}

void CD3D9Renderer::FX_DrawShader_Fur(CShader* ef, SShaderTechnique* pTech)
{
    static CCryNameTSCRC techFurZPost("FurZPost");
    FurPasses& furPasses = FurPasses::GetInstance();
    bool isFurZPost = (pTech->m_NameCRC == techFurZPost);
    furPasses.SetFurShellPassPercent(isFurZPost ? 1.0f : 0.0f);

    // Fur should be rendered with an object containing a render node.
    // Example of objects without render node are various effects such as light beams
    // light arc that their material was set to Fur by mistake - in such case we gracefully don't render ;)
    // Adding a trace warning is an option but it'll slow down the render frame quite noticeably. 
    if (!m_RP.m_pCurObject || !m_RP.m_pCurObject->m_pRenderNode)
        return;

    static CCryNameTSCRC techFurShell("General");
    if (pTech->m_NameCRC == techFurShell)
    {
        PROFILE_FRAME(DrawShader_Fur);


        EF_Scissor(false, 0, 0, 0, 0);

        int recurseLevel = SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID]; // Skip fur shells for recursive passes
        FurPasses::RenderMode furRenderMode = furPasses.GetFurRenderingMode();
        if (pTech->m_Passes.Num() && furRenderMode != FurPasses::RenderMode::None && CV_r_FurShellPassCount > 0 && recurseLevel == 0)
        {
            uint64 nSavedFlags = m_RP.m_FlagsShader_RT;
            uint32 nSavedStateAnd = m_RP.m_ForceStateAnd;

            furPasses.ApplyFurDebugFlags();

            if (furRenderMode == FurPasses::RenderMode::AlphaTested)
            {
                m_RP.m_ForceStateAnd |= GS_BLALPHA_MASK | GS_BLEND_MASK;
                m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_ADDITIVE_BLENDING];

                // Ensure that alpha testing is set up for alpha tested fur shells, even if not specified in the
                // material, by forcing a minimum alpha test of 0.01. This allows fur materials that do not
                // specify alpha testing to appear similar to alpha blended fur, but materials that control
                // alpha testing still benefit from their settings.
                FX_SetAlphaTestState(max(0.01f, m_RP.m_pShaderResources->GetAlphaRef()));
            }
            else if (furRenderMode == FurPasses::RenderMode::AlphaBlended)
            {
                // Even if the material specifies alpha testing, don't write depth for alpha blended fur shells
                m_RP.m_ForceStateAnd |= GS_DEPTHWRITE;
            }

            // OIT permutation flag set
            MultiLayerAlphaBlendPass::GetInstance().ConfigureShaderFlags(m_RP.m_FlagsShader_RT);

            assert(pTech->m_Passes.Num() == 1);

            SShaderPass* slw = &pTech->m_Passes[0];
            m_RP.m_pCurPass = slw;

            // Set all textures and HW TexGen modes for the current pass (ShadeLayer)
            assert (slw->m_VShader && slw->m_PShader);
            if (slw->m_VShader && slw->m_PShader)
            {
                FX_CommitStates(pTech, slw, (slw->m_PassFlags & SHPF_NOMATSTATE) == 0);

                bool bSkinned = (m_RP.m_pCurObject->m_ObjFlags & FOB_SKINNED) && !CV_r_character_nodeform;

                bSkinned |= FX_SetStreamFlags(slw);

                int startShell = 1;
                int endShell = CV_r_FurShellPassCount;
                int numShellPasses = CV_r_FurShellPassCount;

                if ((m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_HDR_MODE]) == 0)
                {
                    // For aux views such as the material editor, draw the base surface, as it is not captured by Z pass there
                    startShell = 0;
                }

                if (CV_r_FurDebugOneShell > 0 && CV_r_FurDebugOneShell <= CV_r_FurShellPassCount)
                {
                    startShell = CV_r_FurDebugOneShell;
                    endShell = startShell;
                }
                else if (IRenderNode* pRenderNode = m_RP.m_pCurObject->m_pRenderNode)
                {
                    // Scale number of shell passes by object's distance to camera and LOD ratio
                    float lodRatio = pRenderNode->GetLodRatioNormalized();
                    if (lodRatio > 0.0f)
                    {
                        static ICVar* pTargetSize = gEnv->pConsole->GetCVar("e_LodFaceAreaTargetSize");
                        if (pTargetSize)
                        {
                            lodRatio *= pTargetSize->GetFVal();
                        }

                        // Not using pRenderNode->GetMaxViewDist() because we want to be able to LOD out the fur while still being able to see the object at distance
                        float   maxDistance = CV_r_FurMaxViewDist * pRenderNode->GetViewDistanceMultiplier();
                        float   firstLodDistance = pRenderNode->GetFirstLodDistance();
                        float   lodDistance = AZ::GetClamp(firstLodDistance / lodRatio, 0.0f, maxDistance - 0.001f);

                        // Distance before first LOD change (factoring in LOD ratio) uses full number of shells
                        // Beyond that distance, number of shells linearly decreases to 0 as distance approaches max view distance.
                        float distance = m_RP.m_pCurObject->m_fDistance;
                        float distanceRatio = (maxDistance - distance) / (maxDistance - lodDistance);
                        float clampedDistanceRatio = AZ::GetClamp(distanceRatio, 0.0f, 1.0f);
                        endShell = aznumeric_cast<int>(endShell * clampedDistanceRatio);
                        numShellPasses = endShell;
                    }
                }

                numShellPasses = AZ::GetMax(numShellPasses, 1);
                for (int i = startShell; i <= endShell; ++i)
                {
                    // Set shell distance from base surface in fur params
                    furPasses.SetFurShellPassPercent(static_cast<float>(i) / numShellPasses);

                    if (m_RP.m_FlagsPerFlush & RBSI_INSTANCED && !bSkinned)
                    {
                        // Using HW geometry instancing approach
                        FX_DrawShader_InstancedHW(ef, slw);
                    }
                    else
                    {
                        FX_DrawBatches(ef, slw);
                    }
                }
            }

            m_RP.m_ForceStateAnd = nSavedStateAnd;
            m_RP.m_FlagsShader_RT = nSavedFlags;
        }
    }
    else
    {
        uint64 nSavedFlags = m_RP.m_FlagsShader_RT;
        static CCryNameTSCRC techFurShadow("FurShadowGen");
        if (pTech->m_NameCRC == techFurShadow)
        {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_TURBULENCE]; // Indicates fin pass
            if (CV_r_FurFinShadowPass == 0 || furPasses.GetFurRenderingMode() == FurPasses::RenderMode::None)
            {
                m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHADOW_PASS]; // Indicates fins should be skipped in shadow pass
            }
        }

        // All other techniques use the normal path
        FX_DrawShader_General(ef, pTech);
        m_RP.m_FlagsShader_RT = nSavedFlags;
    }
}

void CD3D9Renderer::FX_DrawDebugPasses()
{
    if (!m_RP.m_pRootTechnique || m_RP.m_pRootTechnique->m_nTechnique[TTYPE_DEBUG] < 0)
    {
        return;
    }

    CShader* sh = m_RP.m_pShader;
    SShaderTechnique* pTech = m_RP.m_pShader->m_HWTechniques[m_RP.m_pRootTechnique->m_nTechnique[TTYPE_DEBUG]];

    PROFILE_FRAME(DrawShader_DebugPasses);

    PROFILE_LABEL_SCOPE("DEBUG_PASS");

    int nLastRE = m_RP.m_nLastRE;
    m_RP.m_nLastRE = 0;
    for (int nRE = 0; nRE <= nLastRE; nRE++)
    {
        s_tempRIs.SetUse(0);

        m_RP.m_pRE = m_RP.m_RIs[nRE][0]->pElem;

        if (!m_RP.m_pRE)
        {
            continue;
        }

        for (uint32 i = 0; i < m_RP.m_RIs[nRE].Num(); i++)
        {
            s_tempRIs.AddElem(m_RP.m_RIs[nRE][i]);
        }

        if (!s_tempRIs.Num())
        {
            continue;
        }

        m_RP.m_pRE->mfPrepare(false);
        uint32 nSaveMD = m_RP.m_FlagsShader_MD;

        TArray<SRendItem*> saveArr;
        saveArr.Assign(m_RP.m_RIs[0]);
        m_RP.m_RIs[0].Assign(s_tempRIs);

        CRenderObject* pSaveObject = m_RP.m_pCurObject;
        m_RP.m_pCurObject = m_RP.m_RIs[0][0]->pObj;
        m_RP.m_FlagsShader_MD &= ~HWMD_TEXCOORD_FLAG_MASK;
        int32 nMaterialStatePrevOr = m_RP.m_MaterialStateOr;
        int32 nMaterialStatePrevAnd = m_RP.m_MaterialStateAnd;
        m_RP.m_MaterialStateAnd = GS_BLEND_MASK;
        m_RP.m_MaterialStateOr = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

        FX_DrawTechnique(sh, pTech);

        m_RP.m_RIs[0].Assign(saveArr);
        saveArr.ClearArr();

        m_RP.m_pCurObject = pSaveObject;
        m_RP.m_pPrevObject = NULL;
        m_RP.m_FlagsShader_MD = nSaveMD;
        m_RP.m_MaterialStateOr = nMaterialStatePrevOr;
        m_RP.m_MaterialStateAnd = nMaterialStatePrevAnd;
    }
    m_RP.m_nLastRE = nLastRE;
}

// deprecated (cannot remove at this stage) - maybe can batch into FX_DrawEffectLayerPasses (?)
void CD3D9Renderer::FX_DrawMultiLayers()
{
    // Verify if current mesh has valid data for layers
    CREMeshImpl* pRE = (CREMeshImpl*) m_RP.m_pRE;
    if (!m_RP.m_pShader || !m_RP.m_pShaderResources || !m_RP.m_pCurObject->m_nMaterialLayers)
    {
        return;
    }

    _smart_ptr<IMaterial> pObjMat = m_RP.m_pCurObject->m_pCurrMaterial;
    if ((SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID] > 0) || !m_RP.m_pShaderResources || !pObjMat)
    {
        return;
    }

    if (m_RP.m_PersFlags2 & (RBPF2_CUSTOM_RENDER_PASS | RBPF2_MOTIONBLURPASS))
    {
        return;
    }

    CRenderChunk* pChunk = pRE->m_pChunk;
    if (!pChunk)
    {
        assert(pChunk);
        return;
    }

    // Check if chunk material has layers at all
    _smart_ptr<IMaterial> pDefaultMtl = gEnv->p3DEngine->GetMaterialManager()->GetDefaultLayersMaterial();
    _smart_ptr<IMaterial> pCurrMtl = pObjMat->GetSubMtlCount() ? pObjMat->GetSubMtl(pChunk->m_nMatID) : pObjMat;
    if (!pCurrMtl || !pDefaultMtl || (pCurrMtl->GetFlags() & MTL_FLAG_NODRAW))
    {
        return;
    }

    uint32 nLayerCount = pDefaultMtl->GetLayerCount();
    if (!nLayerCount)
    {
        return;
    }

    // Start multi-layers processing
    PROFILE_FRAME(DrawShader_MultiLayers);

    if (m_logFileHandle != AZ::IO::InvalidHandle)
    {
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** Start Multilayers processing ***\n");
    }

    for (int nRE = 0; nRE <= m_RP.m_nLastRE; nRE++)
    {
        m_RP.m_pRE = m_RP.m_RIs[nRE][0]->pElem;

        // Render all layers
        for (uint32 nCurrLayer(0); nCurrLayer < nLayerCount; ++nCurrLayer)
        {
            IMaterialLayer* pLayer = const_cast< IMaterialLayer* >(pCurrMtl->GetLayer(nCurrLayer));
            IMaterialLayer* pDefaultLayer =  const_cast< IMaterialLayer* >(pDefaultMtl->GetLayer(nCurrLayer));
            bool bDefaultLayer = false;
            if (!pLayer)
            {
                // Replace with default layer
                pLayer =  pDefaultLayer;
                bDefaultLayer = true;

                if (!pLayer)
                {
                    continue;
                }
            }

            if (!pLayer->IsEnabled() || pLayer->DoesFadeOut())
            {
                continue;
            }

            // Set/verify layer shader technique
            SShaderItem& pCurrShaderItem = pLayer->GetShaderItem();
            CShader* pSH = static_cast<CShader*>(pCurrShaderItem.m_pShader);
            if (!pSH || pSH->m_HWTechniques.empty())
            {
                continue;
            }

            SShaderTechnique* pTech = pSH->m_HWTechniques[0];
            if (!pTech)
            {
                continue;
            }

            // Re-create render object list, based on layer properties
            {
                s_tempRIs.SetUse(0);

                CRenderObject* pObj = m_RP.m_pCurObject;
                uint32 nObj = 0;

                for (nObj = 0; nObj < m_RP.m_RIs[nRE].Num(); nObj++)
                {
                    pObj = m_RP.m_RIs[nRE][nObj]->pObj;
                    uint8 nMaterialLayers = 0;
                    nMaterialLayers |= ((pObj->m_nMaterialLayers & MTL_LAYER_BLEND_DYNAMICFROZEN)) ? MTL_LAYER_FROZEN : 0;
                    if (nMaterialLayers & (1 << nCurrLayer))
                    {
                        s_tempRIs.AddElem(m_RP.m_RIs[nRE][nObj]);
                    }
                }

                // nothing in render list
                if (!s_tempRIs.Num())
                {
                    continue;
                }
            }

            int nSaveLastRE = m_RP.m_nLastRE;
            m_RP.m_nLastRE = 0;

            TexturesResourcesMap        pPrevLayerResourceTex;     // A map of texture used by the shader
            if (bDefaultLayer)
            {   // Keep layer resources and replace with resources from base shader
                pPrevLayerResourceTex = ((CShaderResources*)pCurrShaderItem.m_pShaderResources)->m_TexturesResourcesMap; 
                ((CShaderResources*)pCurrShaderItem.m_pShaderResources)->m_TexturesResourcesMap = m_RP.m_pShaderResources->m_TexturesResourcesMap;
            }

            m_RP.m_pRE->mfPrepare(false);

            // Store current rendering data
            TArray<SRendItem*> pPrevRenderObjLst;
            pPrevRenderObjLst.Assign(m_RP.m_RIs[0]);
            CRenderObject* pPrevObject = m_RP.m_pCurObject;
            CShaderResources* pPrevShaderResources = m_RP.m_pShaderResources;
            CShader* pPrevSH = m_RP.m_pShader;
            uint32 nPrevNumRendPasses = m_RP.m_nNumRendPasses;
            uint64 nFlagsShaderRTprev = m_RP.m_FlagsShader_RT;

            SShaderTechnique* pPrevRootTech = m_RP.m_pRootTechnique;
            m_RP.m_pRootTechnique = pTech;

            int nMaterialStatePrevOr = m_RP.m_MaterialStateOr;
            int nMaterialStatePrevAnd = m_RP.m_MaterialStateAnd;
            uint32 nFlagsShaderLTprev = m_RP.m_FlagsShader_LT;

            int nPersFlagsPrev = m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags;
            int nPersFlags2Prev = m_RP.m_PersFlags2;
            int nMaterialAlphaRefPrev = m_RP.m_MaterialAlphaRef;
            bool bIgnoreObjectAlpha = m_RP.m_bIgnoreObjectAlpha;
            m_RP.m_bIgnoreObjectAlpha = true;

            m_RP.m_pShader = pSH;
            m_RP.m_RIs[0].Assign(s_tempRIs);
            m_RP.m_pCurObject = m_RP.m_RIs[0][0]->pObj;
            m_RP.m_pPrevObject = NULL;
            m_RP.m_pShaderResources = (CShaderResources*)pCurrShaderItem.m_pShaderResources;

            // Reset light passes (need ambient)
            m_RP.m_nNumRendPasses = 0;
            m_RP.m_PersFlags2 |= RBPF2_MATERIALLAYERPASS;

            if ((1 << nCurrLayer) & MTL_LAYER_FROZEN)
            {
                m_RP.m_MaterialStateAnd = GS_BLEND_MASK | GS_ALPHATEST_MASK;
                m_RP.m_MaterialStateOr  = GS_BLSRC_ONE | GS_BLDST_ONE;
                m_RP.m_MaterialAlphaRef = 0xff;
            }

            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

            FX_DrawTechnique(pSH, pTech);

            // Restore previous rendering data
            m_RP.m_RIs[0].Assign(pPrevRenderObjLst);
            pPrevRenderObjLst.ClearArr();
            m_RP.m_pShader = pPrevSH;
            m_RP.m_pShaderResources = pPrevShaderResources;
            m_RP.m_pCurObject = pPrevObject;
            m_RP.m_pPrevObject = NULL;
            m_RP.m_PersFlags2 = nPersFlags2Prev;
            m_RP.m_nLastRE = nSaveLastRE;


            m_RP.m_nNumRendPasses = nPrevNumRendPasses;

            m_RP.m_FlagsShader_LT = nFlagsShaderLTprev;
            m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags = nPersFlagsPrev;
            m_RP.m_FlagsShader_RT = nFlagsShaderRTprev;

            m_RP.m_nNumRendPasses = 0;

            m_RP.m_pRootTechnique = pPrevRootTech;
            m_RP.m_bIgnoreObjectAlpha = bIgnoreObjectAlpha;
            m_RP.m_MaterialStateOr = nMaterialStatePrevOr;
            m_RP.m_MaterialStateAnd = nMaterialStatePrevAnd;
            m_RP.m_MaterialAlphaRef = nMaterialAlphaRefPrev;

            if (bDefaultLayer)
            {   // restore from the base layer
                ((CShaderResources*)pCurrShaderItem.m_pShaderResources)->m_TexturesResourcesMap = pPrevLayerResourceTex;
            }
        }
    }

    m_RP.m_pRE = pRE;

    if (m_logFileHandle != AZ::IO::InvalidHandle)
    {
        Logv(SRendItem::m_RecurseLevel[m_RP.m_nProcessThreadID], "*** End Multilayers processing ***\n");
    }
}

void CD3D9Renderer::FX_SelectTechnique(CShader* pShader, SShaderTechnique* pTech)
{
    SShaderTechniqueStat Stat;
    Stat.pTech = pTech;
    Stat.pShader = pShader;
    if (pTech->m_Passes.Num())
    {
        SShaderPass* pPass = &pTech->m_Passes[0];
        if (pPass->m_PShader && pPass->m_VShader)
        {
            Stat.pVS = (CHWShader_D3D*)pPass->m_VShader;
            Stat.pPS = (CHWShader_D3D*)pPass->m_PShader;
            Stat.pVSInst = Stat.pVS->m_pCurInst;
            Stat.pPSInst = Stat.pPS->m_pCurInst;
            g_SelectedTechs.push_back(Stat);
        }
    }
}

void CD3D9Renderer::FX_DrawTechnique(CShader* ef, SShaderTechnique* pTech)
{
    FUNCTION_PROFILER_RENDER_FLAT
    switch (ef->m_eSHDType)
    {
    case eSHDT_General:
        FX_DrawShader_General(ef, pTech);
        break;
    case eSHDT_Light:
        FX_DrawShader_General(ef, pTech);
        break;
    case eSHDT_Terrain:
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::LegacyTerrain, "FX_DrawTechnique");
        FX_DrawShader_General(ef, pTech);
    }
        break;
    case eSHDT_Fur:
        FX_DrawShader_Fur(ef, pTech);
        break;
    case eSHDT_CustomDraw:
    case eSHDT_Sky:
        if (m_RP.m_pRE)
        {
            EF_Scissor(false, 0, 0, 0, 0);
            if (pTech && pTech->m_Passes.Num())
            {
                m_RP.m_pRE->mfDraw(ef, &pTech->m_Passes[0]);
            }
            else
            {
                m_RP.m_pRE->mfDraw(ef, NULL);
            }
        }
        break;
    default:
        assert(0);
    }
    if (m_RP.m_ObjFlags & FOB_SELECTED)
    {
        FX_SelectTechnique(ef, pTech);
    }
}

#if defined(HW_INSTANCING_ENABLED)
static void sDetectInstancing(CShader* pShader, [[maybe_unused]] CRenderObject* pObj)
{
    CRenderer* rd = gRenDev;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    if (!CRenderer::CV_r_geominstancing || rd->m_bUseGPUFriendlyBatching[rRP.m_nProcessThreadID] || !(pShader->m_Flags & EF_SUPPORTSINSTANCING) || CRenderer::CV_r_measureoverdraw ||
        // don't instance in motion blur pass or post 3d render
        rRP.m_PersFlags2 & RBPF2_POST_3D_RENDERER_PASS ||     //rRP.m_PersFlags2 & RBPF2_MOTIONBLURPASS ||
        // only instance meshes
        !rRP.m_pRE || rRP.m_pRE->mfGetType() != eDATA_Mesh
        )
    {
        rRP.m_FlagsPerFlush &= ~RBSI_INSTANCED;
        return;
    }

    int i = 0, nLastRE = rRP.m_nLastRE;
    for (; i <= nLastRE; i++)
    {
        int nRIs = rRP.m_RIs[i].Num();

        // instance even with conditional rendering - && RIs[0]->nOcclQuery<0
        if (nRIs > CRenderer::m_iGeomInstancingThreshold || (rRP.m_FlagsPerFlush & RBSI_INSTANCED))
        {
            rRP.m_FlagsPerFlush |= RBSI_INSTANCED;
            break;
        }
    }
    if (i > rRP.m_nLastRE)
    {
        rRP.m_FlagsPerFlush &= ~RBSI_INSTANCED;
    }
}
#endif

void CD3D9Renderer::FX_SetAlphaTestState(float alphaRef)
{
    if (!(m_RP.m_PersFlags2 & RBPF2_NOALPHATEST))
    {
        const int nAlphaRef = int(alphaRef * 255.0f);

        m_RP.m_MaterialAlphaRef = nAlphaRef;
        m_RP.m_MaterialStateOr = GS_ALPHATEST_GEQUAL | GS_DEPTHWRITE;
        m_RP.m_MaterialStateAnd = GS_ALPHATEST_MASK;
    }
    else
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ALPHATEST];
    }
}

// Set/Restore shader resources overrided states
bool CD3D9Renderer::FX_SetResourcesState()
{
    FUNCTION_PROFILER_RENDER_FLAT
    if (!m_RP.m_pShader)
    {
        return false;
    }
    m_RP.m_MaterialStateOr = 0;
    m_RP.m_MaterialStateAnd = 0;
    if (!m_RP.m_pShaderResources)
    {
        return true;
    }

    PrefetchLine(m_RP.m_pShaderResources, 0);       //Shader Resources fit in a cache line, but they're not 128-byte aligned! We are likely going
    PrefetchLine(m_RP.m_pShaderResources, 124); //  to cache miss on access to m_ResFlags but will hopefully avoid later ones

    if (m_RP.m_pShader->m_Flags2 & EF2_IGNORERESOURCESTATES)
    {
        return true;
    }

    m_RP.m_ShaderTexResources[EFTT_DECAL_OVERLAY] = NULL;

    const CShaderResources* pRes = m_RP.m_pShaderResources;
    const uint32 uResFlags = pRes->m_ResFlags;
    if (uResFlags & MTL_FLAG_NOTINSTANCED)
    {
        m_RP.m_FlagsPerFlush &= ~RBSI_INSTANCED;
    }

    if (uResFlags & MTL_FLAG_2SIDED)
    {
        D3DSetCull(eCULL_None);
        m_RP.m_FlagsPerFlush |= RBSI_LOCKCULL;
    }

    if (pRes->IsAlphaTested())
    {
        FX_SetAlphaTestState(pRes->GetAlphaRef());
    }

    if (pRes->IsTransparent())
    {
        if (!(m_RP.m_PersFlags2 & RBPF2_NOALPHABLEND))
        {
            const float fOpacity = pRes->GetStrengthValue(EFTT_OPACITY);

            m_RP.m_MaterialStateAnd |= GS_DEPTHWRITE | GS_BLEND_MASK;
            m_RP.m_MaterialStateOr &= ~GS_DEPTHWRITE;
            if (uResFlags & MTL_FLAG_ADDITIVE)
            {
                m_RP.m_MaterialStateOr |= GS_BLSRC_ONE | GS_BLDST_ONE;
                m_RP.m_CurGlobalColor[0] = fOpacity;
                m_RP.m_CurGlobalColor[1] = fOpacity;
                m_RP.m_CurGlobalColor[2] = fOpacity;
                m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ADDITIVE_BLENDING];
            }
            else
            {
                m_RP.m_MaterialStateOr |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
                m_RP.m_CurGlobalColor[3] = fOpacity;
            }
            m_RP.m_fCurOpacity = fOpacity;
        }
    }
    {
        if (pRes->m_pDeformInfo)
        {
            m_RP.m_FlagsShader_MDV |= pRes->m_pDeformInfo->m_eType;
        }
        m_RP.m_FlagsShader_MDV |= m_RP.m_pCurObject->m_nMDV | m_RP.m_pShader->m_nMDV;
        if (m_RP.m_ObjFlags & FOB_OWNER_GEOMETRY)
        {
            m_RP.m_FlagsShader_MDV &= ~MDV_DEPTH_OFFSET;
        }
    }

    return true;
}

//===================================================================================================

#if !defined(_RELEASE)
static void sBatchStats([[maybe_unused]] SRenderPipeline& rRP)
{
#if defined(ENABLE_PROFILING_CODE)
    SPipeStat& rPS = rRP.m_PS[rRP.m_nProcessThreadID];
    rPS.m_NumRendMaterialBatches++;
    rPS.m_NumRendGeomBatches += rRP.m_nLastRE + 1;
    for (int i = 0; i <= rRP.m_nLastRE; i++)
    {
        rPS.m_NumRendInstances += rRP.m_RIs[i].Num();
    }
#endif
}
#endif

static void sLogFlush(const char* str, CShader* pSH, SShaderTechnique* pTech)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    if (rd->m_logFileHandle != AZ::IO::InvalidHandle)
    {
        SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "%s: '%s.%s', Id:%d, ResId:%d, VF:%d\n", str, pSH->GetName(), pTech ? pTech->m_NameStr.c_str() : "Unknown", pSH->GetID(), rRP.m_pShaderResources ? rRP.m_pShaderResources->m_Id : -1, (int)rRP.m_CurVFormat.GetEnum());

        if (rRP.m_ObjFlags & FOB_SELECTED)
        {
            if (rRP.m_MaterialStateOr & GS_ALPHATEST_MASK)
            {
                rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), (AT) (Selected)\n", rRP.m_pCurObject->m_II.m_Matrix(0, 3), rRP.m_pCurObject->m_II.m_Matrix(1, 3), rRP.m_pCurObject->m_II.m_Matrix(2, 3), rRP.m_pCurObject->m_ObjFlags);
            }
            else
            if (rRP.m_MaterialStateOr & GS_BLEND_MASK)
            {
                rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB) (Dist: %.3f) (Selected)\n", rRP.m_pCurObject->m_II.m_Matrix(0, 3), rRP.m_pCurObject->m_II.m_Matrix(1, 3), rRP.m_pCurObject->m_II.m_Matrix(2, 3), rRP.m_pCurObject->m_ObjFlags, rRP.m_pCurObject->m_fDistance);
            }
            else
            {
                rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), RE: 0x%x (Selected)\n", rRP.m_pCurObject->m_II.m_Matrix(0, 3), rRP.m_pCurObject->m_II.m_Matrix(1, 3), rRP.m_pCurObject->m_II.m_Matrix(2, 3), rRP.m_pCurObject->m_ObjFlags, rRP.m_pRE);
            }
        }
        else
        {
            if (rRP.m_MaterialStateOr & GS_ALPHATEST_MASK)
            {
                rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AT), Inst: %d, RE: 0x%x (Dist: %.3f)\n", rRP.m_pCurObject->m_II.m_Matrix(0, 3), rRP.m_pCurObject->m_II.m_Matrix(1, 3), rRP.m_pCurObject->m_II.m_Matrix(2, 3), rRP.m_pCurObject->m_ObjFlags, rRP.m_RIs[0].Num(), rRP.m_pRE, rRP.m_pCurObject->m_fDistance);
            }
            else
            if (rRP.m_MaterialStateOr & GS_BLEND_MASK)
            {
                rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x) (AB), Inst: %d, RE: 0x%x (Dist: %.3f)\n", rRP.m_pCurObject->m_II.m_Matrix(0, 3), rRP.m_pCurObject->m_II.m_Matrix(1, 3), rRP.m_pCurObject->m_II.m_Matrix(2, 3), rRP.m_pCurObject->m_ObjFlags, rRP.m_RIs[0].Num(), rRP.m_pRE, rRP.m_pCurObject->m_fDistance);
            }
            else
            {
                rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  %.3f, %.3f, %.3f (0x%x), Inst: %d, RE: 0x%x\n", rRP.m_pCurObject->m_II.m_Matrix(0, 3), rRP.m_pCurObject->m_II.m_Matrix(1, 3), rRP.m_pCurObject->m_II.m_Matrix(2, 3), rRP.m_pCurObject->m_ObjFlags, rRP.m_RIs[0].Num(), rRP.m_pRE);
            }
        }
        if (rRP.m_pRE && rRP.m_pRE->mfGetType() == eDATA_Mesh)
        {
            CREMeshImpl* pRE = (CREMeshImpl*) rRP.m_pRE;
            CRenderMesh* pRM = pRE->m_pRenderMesh;
            if (pRM && pRM->m_Chunks.size() && pRM->m_sSource)
            {
                int nChunk = -1;
                for (int i = 0; i < pRM->m_Chunks.size(); i++)
                {
                    CRenderChunk* pCH = &pRM->m_Chunks[i];
                    if (pCH->pRE == pRE)
                    {
                        nChunk = i;
                        break;
                    }
                }
                rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "  Mesh: %s (Chunk: %d)\n", pRM->m_sSource.c_str(), nChunk);
            }
        }
    }
}

void CD3D9Renderer::FX_RefractionPartialResolve()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

    {
        SRenderObjData* ObjData = rRP.m_pCurObject->GetObjData();

        if (ObjData)
        {
            uint8 screenBounds[4];

            screenBounds[0] = ObjData->m_screenBounds[0];
            screenBounds[1] = ObjData->m_screenBounds[1];
            screenBounds[2] = ObjData->m_screenBounds[2];
            screenBounds[3] = ObjData->m_screenBounds[3];

            float boundsI2F[] = {
                (float)(screenBounds[0] << 4), (float)(screenBounds[1] << 4),
                (float)(min(screenBounds[2] << 4, GetWidth())), (float)(min(screenBounds[3] << 4, GetHeight()))
            };

            if (((screenBounds[2] - screenBounds[0]) && (screenBounds[3] - screenBounds[1])) &&
                !((rRP.m_nCurrResolveBounds[0] == screenBounds[0])
                  && (rRP.m_nCurrResolveBounds[1] == screenBounds[1])
                  && (rRP.m_nCurrResolveBounds[2] == screenBounds[2])
                  && (rRP.m_nCurrResolveBounds[3] == screenBounds[3])))
            {
                rRP.m_nCurrResolveBounds[0] = screenBounds[0];
                rRP.m_nCurrResolveBounds[1] = screenBounds[1];
                rRP.m_nCurrResolveBounds[2] = screenBounds[2];
                rRP.m_nCurrResolveBounds[3] = screenBounds[3];

                int boundsF2I[] = {
                    int(boundsI2F[0] * m_RP.m_CurDownscaleFactor.x),
                    int(boundsI2F[1] * m_RP.m_CurDownscaleFactor.y),
                    int(boundsI2F[2] * m_RP.m_CurDownscaleFactor.x),
                    int(boundsI2F[3] * m_RP.m_CurDownscaleFactor.y)
                };

                int currScissorX, currScissorY, currScissorW, currScissorH;
                CTexture* pTarget =    CTexture::s_ptexCurrSceneTarget;

                //cache RP states  - Probably a bit excessive, but want to be safe
                CShaderResources* currRes = rd->m_RP.m_pShaderResources;
                CShader* currShader  = rd->m_RP.m_pShader;
                int currShaderTechnique = rd->m_RP.m_nShaderTechnique;
                SShaderTechnique* currTechnique = rd->m_RP.m_pCurTechnique;
                uint32 currCommitFlags = rd->m_RP.m_nCommitFlags;
                uint32 currFlagsShaderBegin = rd->m_RP.m_nFlagsShaderBegin;
                ECull currCull = m_RP.m_eCull;

                float currVPMinZ = rd->m_NewViewport.fMinZ; // Todo: Add to GetViewport / SetViewport
                float currVPMaxZ = rd->m_NewViewport.fMaxZ;

                D3DSetCull(eCULL_None);

                bool bScissored = EF_GetScissorState(currScissorX, currScissorY, currScissorW, currScissorH);

                int newScissorX = int(boundsF2I[0]);
                int newScissorY = int(boundsF2I[1]);
                int newScissorW = max(0, min(int(boundsF2I[2]), GetWidth()) - newScissorX);
                int newScissorH = max(0, min(int(boundsF2I[3]), GetHeight()) - newScissorY);

                EF_Scissor(true, newScissorX, newScissorY, newScissorW, newScissorH);

                FX_ScreenStretchRect(pTarget);

                EF_Scissor(bScissored, currScissorX, currScissorY, currScissorW, currScissorH);

                D3DSetCull(currCull);

                //restore RP states
                rd->m_RP.m_pShaderResources = currRes;
                rd->m_RP.m_pShader = currShader;
                rd->m_RP.m_nShaderTechnique = currShaderTechnique;
                rd->m_RP.m_pCurTechnique = currTechnique;
                rd->m_RP.m_nCommitFlags = currCommitFlags | FC_MATERIAL_PARAMS;
                rd->m_RP.m_nFlagsShaderBegin = currFlagsShaderBegin;
                rd->m_NewViewport.fMinZ = currVPMinZ;
                rd->m_NewViewport.fMaxZ = currVPMaxZ;

#if REFRACTION_PARTIAL_RESOLVE_STATS
                {
                    const int x1 = (screenBounds[0] << 4);
                    const int y1 = (screenBounds[1] << 4);
                    const int x2 = (screenBounds[2] << 4);
                    const int y2 = (screenBounds[3] << 4);

                    const int resolveWidth = x2 - x1;
                    const int resolveHeight = y2 - y1;
                    const int resolvePixelCount = resolveWidth * resolveHeight;

                    // Update stats
                    SPipeStat& pipeStat = rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID];
                    pipeStat.m_refractionPartialResolveCount++;
                    pipeStat.m_refractionPartialResolvePixelCount += resolvePixelCount;

                    const float resolveCostConversion = 18620398.0f;

                    pipeStat.m_fRefractionPartialResolveEstimatedCost += ((float)resolvePixelCount / resolveCostConversion);

#if REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS
                    if (CRenderer::CV_r_RefractionPartialResolvesDebug == eRPR_DEBUG_VIEW_2D_AREA || CRenderer::CV_r_RefractionPartialResolvesDebug == eRPR_DEBUG_VIEW_2D_AREA_OVERLAY)
                    {
                        // Render 2d areas additively on screen
                        IRenderAuxGeom* pAuxRenderer = gEnv->pRenderer->GetIRenderAuxGeom();
                        if (pAuxRenderer)
                        {
                            SAuxGeomRenderFlags oldRenderFlags = pAuxRenderer->GetRenderFlags();

                            SAuxGeomRenderFlags newRenderFlags;
                            newRenderFlags.SetDepthTestFlag(e_DepthTestOff);
                            newRenderFlags.SetAlphaBlendMode(e_AlphaAdditive);
                            newRenderFlags.SetMode2D3DFlag(e_Mode2D);
                            pAuxRenderer->SetRenderFlags(newRenderFlags);

                            const float screenWidth = (float)GetWidth();
                            const float screenHeight = (float)GetHeight();

                            // Calc resolve area
                            const float left = x1 / screenWidth;
                            const float top = y1 / screenHeight;
                            const float right = x2 / screenWidth;
                            const float bottom = y2 / screenHeight;

                            // Render resolve area
                            ColorB areaColor(20, 0, 0, 255);

                            if (CRenderer::CV_r_RefractionPartialResolvesDebug == eRPR_DEBUG_VIEW_2D_AREA_OVERLAY)
                            {
                                int val = (pipeStat.m_refractionPartialResolveCount) % 3;
                                areaColor = ColorB((val == 0) ? 0 : 128, (val == 1)  ? 0 : 128, (val == 2)  ? 0 : 128, 255);
                            }

                            const uint vertexCount = 6;
                            const Vec3 vert[vertexCount] = {
                                Vec3(left, top, 0.0f),
                                Vec3(left, bottom, 0.0f),
                                Vec3(right, top, 0.0f),
                                Vec3(left, bottom, 0.0f),
                                Vec3(right, bottom, 0.0f),
                                Vec3(right, top, 0.0f)
                            };
                            pAuxRenderer->DrawTriangles(vert, vertexCount, areaColor);

                            // Set previous Aux render flags back again
                            pAuxRenderer->SetRenderFlags(oldRenderFlags);
                        }
                    }
#endif  // REFRACTION_PARTIAL_RESOLVE_DEBUG_VIEWS
                }
#endif  // REFRACTION_PARTIAL_RESOLVE_STATS
            }
        }
    }
}

// Flush current render item
void CD3D9Renderer::FX_FlushShader_General()
{
    FUNCTION_PROFILER_RENDER_FLAT
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    if (!rRP.m_pRE && !rRP.m_RendNumVerts)
    {
        return;
    }

    CShader* ef = rRP.m_pShader;
    if (!ef)
    {
        return;
    }

    const CShaderResources* rsr = rRP.m_pShaderResources;
    if ((ef->m_Flags & EF_SUPPORTSDEFERREDSHADING_FULL) && (rRP.m_PersFlags2 & RBPF2_FORWARD_SHADING_PASS) && (!rsr->IsEmissive()))
    {
        return;
    }

    SThreadInfo& RESTRICT_REFERENCE rTI = rRP.m_TI[rRP.m_nProcessThreadID];
    assert(!(rTI.m_PersFlags & RBPF_SHADOWGEN));
    assert(!(rRP.m_nBatchFilter & FB_Z));

    if (!rRP.m_sExcludeShader.empty())
    {
        char nm[1024];
        azstrcpy(nm, AZ_ARRAY_SIZE(nm), ef->GetName());
        azstrlwr(nm, AZ_ARRAY_SIZE(nm));
        if (strstr(rRP.m_sExcludeShader.c_str(), nm))
        {
            return;
        }
    }
#ifdef DO_RENDERLOG
    if (rd->m_logFileHandle != AZ::IO::InvalidHandle && CV_r_log == 3)
    {
        rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n\n.. Start %s flush: '%s' ..\n", "General", ef->GetName());
    }
#endif

#ifndef _RELEASE
    sBatchStats(rRP);
#endif
    CRenderObject* pObj = rRP.m_pCurObject;

    PROFILE_SHADER_SCOPE;

    if (rd->m_RP.m_pRE)
    {
        rd->m_RP.m_pRE = rd->m_RP.m_RIs[0][0]->pElem;
    }

#if defined(HW_INSTANCING_ENABLED)
    sDetectInstancing(ef, pObj);
#endif

    // Techniques draw cycle...
    SShaderTechnique* __restrict pTech = ef->mfGetStartTechnique(rRP.m_nShaderTechnique);

    if (pTech)
    {
        uint32 flags = (FB_CUSTOM_RENDER | FB_MOTIONBLUR | FB_SOFTALPHATEST | FB_WATER_REFL | FB_WATER_CAUSTIC);

        if (rRP.m_pShaderResources && !(rRP.m_nBatchFilter & flags))
        {
            uint32 i;
            // Update render targets if necessary
            if (!(rTI.m_PersFlags & RBPF_DRAWTOTEXTURE))
            {
                uint32 targetNum = rRP.m_pShaderResources->m_RTargets.Num();
                const CShaderResources* const __restrict pShaderResources = rRP.m_pShaderResources;
                for (i = 0; i < targetNum; ++i)
                {
                    SHRenderTarget* pTarg = pShaderResources->m_RTargets[i];
                    if (pTarg->m_eOrder == eRO_PreDraw)
                    {
                        rd->FX_DrawToRenderTarget(ef, rRP.m_pShaderResources, pObj, pTech, pTarg, 0, rRP.m_pRE);
                    }
                }
                targetNum = pTech->m_RTargets.Num();
                for (i = 0; i < targetNum; ++i)
                {
                    SHRenderTarget* pTarg = pTech->m_RTargets[i];
                    if (pTarg->m_eOrder == eRO_PreDraw)
                    {
                        rd->FX_DrawToRenderTarget(ef, rRP.m_pShaderResources, pObj, pTech, pTarg, 0, rRP.m_pRE);
                    }
                }
            }
        }
        rRP.m_pRootTechnique = pTech;

        flags = (FB_MOTIONBLUR | FB_CUSTOM_RENDER | FB_SOFTALPHATEST | FB_DEBUG | FB_WATER_REFL | FB_WATER_CAUSTIC  | FB_PARTICLES_THICKNESS);

        if (rRP.m_nBatchFilter & flags)
        {
            int nTech = -1;
            if (rRP.m_nBatchFilter & FB_MOTIONBLUR)
            {
                nTech = TTYPE_MOTIONBLURPASS;
            }
            else if (rRP.m_nBatchFilter & FB_CUSTOM_RENDER)
            {
                nTech = TTYPE_CUSTOMRENDERPASS;
            }
            else if (rRP.m_nBatchFilter & FB_SOFTALPHATEST)
            {
                nTech = TTYPE_SOFTALPHATESTPASS;
            }
            else if (rRP.m_nBatchFilter & FB_WATER_REFL)
            {
                nTech = TTYPE_WATERREFLPASS;
            }
            else if (rRP.m_nBatchFilter & FB_WATER_CAUSTIC)
            {
                nTech = TTYPE_WATERCAUSTICPASS;
            }
            else if (rRP.m_nBatchFilter & FB_PARTICLES_THICKNESS)
            {
                nTech = TTYPE_PARTICLESTHICKNESSPASS;
            }
            else if (rRP.m_nBatchFilter & FB_DEBUG)
            {
                nTech = TTYPE_DEBUG;
            }

            if (nTech >= 0 && pTech->m_nTechnique[nTech] > 0)
            {
                assert(pTech->m_nTechnique[nTech] < (int)ef->m_HWTechniques.Num());
                pTech = ef->m_HWTechniques[pTech->m_nTechnique[nTech]];
            }
            rRP.m_nShaderTechniqueType = nTech;
        }
#ifndef _RELEASE
        if (CV_r_debugrendermode)
        {
            if (CV_r_debugrendermode & 1)
            {
                rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
            }
            if (CV_r_debugrendermode & 2)
            {
                rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG1];
            }
            if (CV_r_debugrendermode & 4)
            {
                rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG2];
            }
            if (CV_r_debugrendermode & 8)
            {
                rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG3];
            }
        }
#endif

        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingLBuffersFmt == 2)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION];
        }
        
        if (CRenderer::CV_r_SlimGBuffer)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
        }
     
        // If the object is transparent and if the object has the UAV bound.
        bool multilayerUAVBound = (rRP.m_ObjFlags & FOB_AFTER_WATER) != 0;
        if (rRP.m_pShaderResources && rRP.m_pShaderResources->IsTransparent() && multilayerUAVBound)
        {
            MultiLayerAlphaBlendPass::GetInstance().ConfigureShaderFlags(rRP.m_FlagsShader_RT);
        }

        if (!rd->FX_SetResourcesState())
        {
            return;
        }

        // Handle emissive materials
        CShaderResources* pCurRes = rRP.m_pShaderResources;
        if (pCurRes && pCurRes->IsEmissive() && !pCurRes->IsTransparent() && (rRP.m_PersFlags2 & RBPF2_HDR_FP16))
        {
            rRP.m_MaterialStateAnd |= GS_BLEND_MASK;
            rRP.m_MaterialStateOr = (rRP.m_MaterialStateOr & ~GS_BLEND_MASK) | (GS_BLSRC_ONE | GS_BLDST_ONE);
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ADDITIVE_BLENDING];
        }

        else if (rRP.m_ObjFlags & FOB_BENDED)
        {
            rRP.m_FlagsShader_MDV |= MDV_BENDING;
        }
        rRP.m_FlagsShader_RT |= pObj->m_nRTMask;
#ifdef TESSELLATION_RENDERER
        if ((pObj->m_ObjFlags & FOB_NEAREST) || !(pObj->m_ObjFlags & FOB_ALLOW_TESSELLATION))
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NO_TESSELLATION];
        }
#endif
        if (!(rRP.m_PersFlags2 & RBPF2_NOSHADERFOG) && rTI.m_FS.m_bEnable && !(rRP.m_ObjFlags & FOB_NO_FOG) || !(rRP.m_PersFlags2 & RBPF2_ALLOW_DEFERREDSHADING))
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FOG];
            if (CRenderer::CV_r_VolumetricFog != 0)
            {
                rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_VOLUMETRIC_FOG];
            }
        }
        rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_FOG_VOLUME_HIGH_QUALITY_SHADER]);
        static ICVar* pCVarFogVolumeShadingQuality = gEnv->pConsole->GetCVar("e_FogVolumeShadingQuality");
        if (pCVarFogVolumeShadingQuality->GetIVal() > 0)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_FOG_VOLUME_HIGH_QUALITY_SHADER];
        }
        
        
        const uint64 objFlags = rRP.m_ObjFlags;
        if (objFlags & FOB_NEAREST)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NEAREST];
        }
        if (SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID] == 0 && CV_r_ParticlesSoftIsec)
        {
            //Enable soft particle shader flag for soft particles or particles have half resolution enabled
            //Note: the half res render pass is relying on the soft particle flag to test z buffer. 
            //I am not sure why they did this instead of just have enable z buffer test for that pass.
            if ((objFlags & FOB_SOFT_PARTICLE) || (rRP.m_PersFlags2 & RBPF2_HALFRES_PARTICLES))
            {
                rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SOFT_PARTICLE];
            }
        }

        if (ef->m_Flags2 & EF2_ALPHABLENDSHADOWS)
        {
            rd->FX_SetupShadowsForTransp();
        }

        if (rRP.m_pCurObject->m_RState & OS_ENVIRONMENT_CUBEMAP)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ENVIRONMENT_CUBEMAP];
        }

        if (rRP.m_pCurObject->m_RState & OS_ANIM_BLEND)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ANIM_BLEND];
        }
        if (objFlags & FOB_POINT_SPRITE)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SPRITE];
        }

        // Only enable for resources not using zpass
        if (!(rRP.m_pRLD->m_nBatchFlags[rRP.m_nSortGroupID][rRP.m_nPassGroupID] & FB_Z) || (ef->m_Flags & EF_DECAL))
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NOZPASS];
        }

        rRP.m_pCurTechnique = pTech;

        if ((rRP.m_nBatchFilter & (FB_MULTILAYERS | FB_DEBUG)) && !rRP.m_pReplacementShader)
        {
            if (rRP.m_nBatchFilter & FB_MULTILAYERS)
            {
                rd->FX_DrawMultiLayers();
            }

            if (rRP.m_nBatchFilter & FB_DEBUG)
            {
                rd->FX_DrawDebugPasses();
            }
        }
        else
        {
            rd->FX_DrawTechnique(ef, pTech);
        }
    }//pTech
    else
    if (ef->m_eSHDType == eSHDT_CustomDraw)
    {
        rd->FX_DrawTechnique(ef, 0);
    }

#ifdef DO_RENDERLOG
    sLogFlush("Flush General", ef, pTech);
#endif
}

void CD3D9Renderer::FX_FlushShader_ShadowGen()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    if (!rRP.m_pRE && !rRP.m_RendNumVerts)
    {
        return;
    }

    CShader* ef = rRP.m_pShader;
    if (!ef)
    {
        return;
    }

    if (!rRP.m_sExcludeShader.empty())
    {
        char nm[1024];
        azstrcpy(nm, AZ_ARRAY_SIZE(nm), ef->GetName());
        azstrlwr(nm, AZ_ARRAY_SIZE(nm));
        if (strstr(rRP.m_sExcludeShader.c_str(), nm))
        {
            return;
        }
    }

    assert(rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN);

    CRenderObject* pObj = rRP.m_pCurObject;

#ifdef DO_RENDERLOG
    if (rd->m_logFileHandle != AZ::IO::InvalidHandle)
    {
        if (CV_r_log == 3)
        {
            rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n\n.. Start %s flush: '%s' ..\n", "ShadowGen", ef->GetName());
        }
        if (CV_r_log >= 3)
        {
            rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n");
        }
    }
#endif

#ifndef _RELEASE
    sBatchStats(rRP);
#endif

    PROFILE_SHADER_SCOPE;

#if defined(HW_INSTANCING_ENABLED)
    sDetectInstancing(ef, pObj);
#endif

    SShaderTechnique* __restrict pTech = ef->mfGetStartTechnique(rRP.m_nShaderTechnique);
    assert(pTech);
    if (!pTech || pTech->m_nTechnique[TTYPE_SHADOWGEN] < 0)
    {
        return;
    }

    rRP.m_nShaderTechniqueType = TTYPE_SHADOWGEN;

    if (rd->m_RP.m_pRE)
    {
        rd->m_RP.m_pRE = rd->m_RP.m_RIs[0][0]->pElem;
    }

    rRP.m_pRootTechnique = pTech;

    pTech = ef->m_HWTechniques[pTech->m_nTechnique[TTYPE_SHADOWGEN]];


    const SRenderPipeline::ShadowInfo& shadowInfo = rd->m_RP.m_ShadowInfo;

    if (ef->m_eSHDType == eSHDT_Terrain)
    {
        if (shadowInfo.m_pCurShadowFrustum->m_Flags & DLF_DIRECTIONAL)
        {
            rd->D3DSetCull(eCULL_None);
            rd->m_RP.m_FlagsPerFlush |= RBSI_LOCKCULL;
        }
        else
        {
            rd->D3DSetCull(eCULL_Front);
            rd->m_RP.m_FlagsPerFlush |= RBSI_LOCKCULL;
        }
    }

    // RSMs
#if defined(FEATURE_SVO_GI)
    if (CSvoRenderer::GetRsmColorMap(*shadowInfo.m_pCurShadowFrustum))
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

        if (!(shadowInfo.m_pCurShadowFrustum->m_Flags & DLF_DIRECTIONAL))
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0] | g_HWSR_MaskBit[HWSR_HW_PCF_COMPARE];
        }

        rd->D3DSetCull(eCULL_Back);

        const uint64_t objFlags = rRP.m_ObjFlags;
        if (objFlags & FOB_DECAL_TEXGEN_2D)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DECAL_TEXGEN_2D];
        }
    }
    else
#endif
    if (rRP.m_PersFlags2 & (RBPF2_DRAWTOCUBE | RBPF2_DISABLECOLORWRITES))
    {
        if (rRP.m_PersFlags2 & RBPF2_DISABLECOLORWRITES)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
        }
        if (rRP.m_PersFlags2 & RBPF2_DRAWTOCUBE)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
        }
    }
#ifdef TESSELLATION_RENDERER
    if ((pObj->m_ObjFlags & FOB_NEAREST) || !(pObj->m_ObjFlags & FOB_ALLOW_TESSELLATION))
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NO_TESSELLATION];
    }
#endif

    if (!rd->FX_SetResourcesState())
    {
        return;
    }

    if ((rRP.m_ObjFlags & FOB_BENDED))
    {
        rRP.m_FlagsShader_MDV |= MDV_BENDING;
    }
    rRP.m_FlagsShader_RT |= rRP.m_pCurObject->m_nRTMask;

    if (rRP.m_ObjFlags & FOB_NEAREST)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NEAREST];
    }

    if (rRP.m_ObjFlags & FOB_DISSOLVE)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DISSOLVE];
    }

    rRP.m_pCurTechnique = pTech;
    rd->FX_DrawTechnique(ef, pTech);

#ifdef DO_RENDERLOG
    sLogFlush("Flush ShadowGen", ef, pTech);
#endif
}

void CD3D9Renderer::FX_FlushShader_ZPass()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;
    if (!rRP.m_pRE && !rRP.m_RendNumVerts)
    {
        return;
    }

    CShader* ef = rRP.m_pShader;
    if (!ef)
    {
        return;
    }

    if (!rRP.m_sExcludeShader.empty())
    {
        char nm[1024];
        azstrcpy(nm, AZ_ARRAY_SIZE(nm), ef->GetName());
        azstrlwr(nm, AZ_ARRAY_SIZE(nm));
        if (strstr(rRP.m_sExcludeShader.c_str(), nm))
        {
            return;
        }
    }

    assert(!(rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_SHADOWGEN));
    assert(rRP.m_nBatchFilter & (FB_Z | FB_ZPREPASS | FB_POST_3D_RENDER));

#ifdef DO_RENDERLOG
    if (rd->m_logFileHandle != AZ::IO::InvalidHandle)
    {
        if (CV_r_log == 3)
        {
            rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n\n.. Start %s flush: '%s' ..\n", "ZPass", ef->GetName());
        }
        else
        if (CV_r_log >= 3)
        {
            rd->Logv(SRendItem::m_RecurseLevel[rRP.m_nProcessThreadID], "\n");
        }
    }
#endif

    if (rd->m_RP.m_pRE)
    {
        rd->m_RP.m_pRE = rd->m_RP.m_RIs[0][0]->pElem;
    }

#ifndef _RELEASE
    sBatchStats(rRP);
#endif
    PROFILE_SHADER_SCOPE;

#if defined(HW_INSTANCING_ENABLED)
    sDetectInstancing(ef, rRP.m_pCurObject);
#endif

    // Techniques draw cycle...
    SShaderTechnique* __restrict pTech = ef->mfGetStartTechnique(rRP.m_nShaderTechnique);
    const uint32 nTechniqueID = (rRP.m_nBatchFilter & FB_Z) ? TTYPE_Z : TTYPE_ZPREPASS;
    if (!pTech || pTech->m_nTechnique[nTechniqueID] < 0)
    {
        return;
    }

    rRP.m_nShaderTechniqueType = nTechniqueID;
    rRP.m_pRootTechnique = pTech;

    // Skip z-pass if appropriate technique does not exist
    assert(pTech->m_nTechnique[nTechniqueID] < (int)ef->m_HWTechniques.Num());
    pTech = ef->m_HWTechniques[pTech->m_nTechnique[nTechniqueID]];

    if (!rd->FX_SetResourcesState())
    {
        return;
    }

    rRP.m_FlagsShader_RT |= rRP.m_pCurObject->m_nRTMask;
    if (rRP.m_ObjFlags & FOB_BENDED)
    {
        rRP.m_FlagsShader_MDV |= MDV_BENDING;
    }

    if (rRP.m_PersFlags2 & RBPF2_MOTIONBLURPASS)
    {
        if ((rRP.m_pCurObject->m_ObjFlags & (FOB_MOTION_BLUR | FOB_HAS_PREVMATRIX)) && (rRP.m_PersFlags2 & RBPF2_NOALPHABLEND))
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_MOTION_BLUR];
        }
        else
        {
            rRP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_MOTION_BLUR];
        }
    }

#ifdef TESSELLATION_RENDERER
    if ((rRP.m_pCurObject->m_ObjFlags & FOB_NEAREST) || !(rRP.m_pCurObject->m_ObjFlags & FOB_ALLOW_TESSELLATION))
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NO_TESSELLATION];
    }
#endif

    // Set VisArea and Dynamic objects Stencil Ref
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingStencilPrepass)
    {
        if (rRP.m_nPassGroupID != EFSLIST_DECAL && !(rRP.m_nBatchFilter & FB_ZPREPASS))
        {
            rRP.m_ForceStateOr |=   GS_STENCIL;

            uint32 nStencilRef = CRenderer::CV_r_VisAreaClipLightsPerPixel ? 0 : (rd->m_RP.m_RIs[0][0]->nStencRef | BIT_STENCIL_INSIDE_CLIPVOLUME);
            
            // Here we check if an object can receive decals.
            bool bObjectAcceptsDecals = !(rRP.m_pCurObject->m_NoDecalReceiver);
            if (bObjectAcceptsDecals)
            {
                nStencilRef |= (!(rRP.m_pCurObject->m_ObjFlags & FOB_DYNAMIC_OBJECT) || CV_r_deferredDecalsOnDynamicObjects ? BIT_STENCIL_RESERVED : 0);
            }
            const int32 stencilState = STENC_FUNC(FSS_STENCFUNC_ALWAYS) | STENCOP_FAIL(FSS_STENCOP_KEEP) | STENCOP_ZFAIL(FSS_STENCOP_KEEP) | STENCOP_PASS(FSS_STENCOP_REPLACE);
            rd->FX_SetStencilState(stencilState, nStencilRef, 0xFF, 0xFF);
        }
        else
        {
            rRP.m_ForceStateOr &=   ~GS_STENCIL;
        }
    }

    rRP.m_pCurTechnique = pTech;
    rd->FX_DrawTechnique(ef, pTech);

    rRP.m_ForceStateOr &=   ~GS_STENCIL;
    //reset stencil AND mask always
    rRP.m_CurStencilRefAndMask = 0;

#ifdef DO_RENDERLOG
    sLogFlush("Flush ZPass", ef, pTech);
#endif
}

//===================================================================================================

static int sTexLimitRes(uint32 nSrcsize, uint32 nDstSize)
{
    while (true)
    {
        if (nSrcsize > nDstSize)
        {
            nSrcsize >>= 1;
        }
        else
        {
            break;
        }
    }
    return nSrcsize;
}

static Matrix34 sMatrixLookAt(const Vec3& dir, const Vec3& up, float rollAngle = 0)
{
    Matrix34 M;
    // LookAt transform.
    Vec3 xAxis, yAxis, zAxis;
    Vec3 upVector = up;

    yAxis = -dir.GetNormalized();

    //if (zAxis.x == 0.0 && zAxis.z == 0) up.Set( -zAxis.y,0,0 ); else up.Set( 0,1.0f,0 );

    xAxis = upVector.Cross(yAxis).GetNormalized();
    zAxis = xAxis.Cross(yAxis).GetNormalized();

    // OpenGL kind of matrix.
    M(0, 0) = xAxis.x;
    M(0, 1) = yAxis.x;
    M(0, 2) = zAxis.x;
    M(0, 3) = 0;

    M(1, 0) = xAxis.y;
    M(1, 1) = yAxis.y;
    M(1, 2) = zAxis.y;
    M(1, 3) = 0;

    M(2, 0) = xAxis.z;
    M(2, 1) = yAxis.z;
    M(2, 2) = zAxis.z;
    M(2, 3) = 0;

    if (rollAngle != 0)
    {
        Matrix34 RollMtx;
        RollMtx.SetIdentity();

        float cossin[2];
        //   sincos_tpl(rollAngle, cossin);
        sincos_tpl(rollAngle, &cossin[1], &cossin[0]);

        RollMtx(0, 0) = cossin[0];
        RollMtx(0, 2) = -cossin[1];
        RollMtx(2, 0) = cossin[1];
        RollMtx(2, 2) = cossin[0];

        // Matrix multiply.
        M = RollMtx * M;
    }

    return M;
}

void TexBlurAnisotropicVertical(CTexture* pTex, int nAmount, float fScale, [[maybe_unused]] float fDistribution, [[maybe_unused]] bool bAlphaOnly)
{
    if (!pTex)
    {
        return;
    }

    SDynTexture* tpBlurTemp = new SDynTexture(pTex->GetWidth(), pTex->GetHeight(), pTex->GetDstFormat(), eTT_2D,  FT_STATE_CLAMP, "TempBlurAnisoVertRT");
    if (!tpBlurTemp)
    {
        return;
    }

    tpBlurTemp->Update(pTex->GetWidth(), pTex->GetHeight());

    if (!tpBlurTemp->m_pTexture)
    {
        SAFE_DELETE(tpBlurTemp);
        return;
    }

    PROFILE_SHADER_SCOPE;

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
    gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

    Vec4 vWhite(1.0f, 1.0f, 1.0f, 1.0f);

    static CCryNameTSCRC pTechName("AnisotropicVertical");
    CShader* m_pCurrShader = CShaderMan::s_shPostEffects;

    uint32 nPasses;
    m_pCurrShader->FXSetTechnique(pTechName);
    m_pCurrShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    m_pCurrShader->FXBeginPass(0);

    gRenDev->FX_SetState(GS_NODEPTHTEST);

    // setup texture offsets, for texture sampling
    float t1 = 1.0f / (float) pTex->GetHeight();

    Vec4 pWeightsPS;
    pWeightsPS.x = 0.25f * t1;
    pWeightsPS.y = 0.5f * t1;
    pWeightsPS.z = 0.75f * t1;
    pWeightsPS.w = 1.0f * t1;


    pWeightsPS *= -fScale;


    STexState sTexState = STexState(FILTER_LINEAR, true);
    static CCryNameR pParam0Name("blurParams0");

    for (int p(1); p <= nAmount; ++p)
    {
        //Horizontal

        CShaderMan::s_shPostEffects->FXSetVSFloat(pParam0Name, &pWeightsPS, 1);
        gcpRendD3D->FX_PushRenderTarget(0, tpBlurTemp->m_pTexture, NULL);
        gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

        pTex->Apply(0, CTexture::GetTexState(sTexState));
        PostProcessUtils().DrawFullScreenTri(pTex->GetWidth(), pTex->GetHeight());

        gcpRendD3D->FX_PopRenderTarget(0);

        //Vertical

        pWeightsPS *= 2.0f;

        gcpRendD3D->FX_PushRenderTarget(0, pTex, NULL);
        gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

        CShaderMan::s_shPostEffects->FXSetVSFloat(pParam0Name, &pWeightsPS, 1);
        tpBlurTemp->m_pTexture->Apply(0, CTexture::GetTexState(sTexState));
        PostProcessUtils().DrawFullScreenTri(pTex->GetWidth(), pTex->GetHeight());

        gcpRendD3D->FX_PopRenderTarget(0);
    }

    m_pCurrShader->FXEndPass();
    m_pCurrShader->FXEnd();

    // Restore previous viewport
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    //release dyntexture
    SAFE_DELETE(tpBlurTemp);
}

bool CD3D9Renderer::FX_DrawToRenderTarget(CShader* pShader, CShaderResources* pRes, CRenderObject* pObj, SShaderTechnique* pTech, SHRenderTarget* pRT, int nPreprType, IRenderElement* pRE)
{
    if (!pRT)
    {
        return false;
    }

    int nThreadList = m_pRT->GetThreadList();

    uint32 nPrFlags = pRT->m_nFlags;
    if (nPrFlags & FRT_RENDTYPE_CURSCENE)
    {
        return false;
    }

    CRenderObject* pPrevIgn = m_RP.m_TI[nThreadList].m_pIgnoreObject;
    CTexture* Tex = pRT->m_pTarget[0];
    SEnvTexture* pEnvTex = NULL;

    if (nPreprType == SPRID_SCANTEX)
    {
        nPrFlags |= FRT_CAMERA_REFLECTED_PLANE;
        pRT->m_nFlags = nPrFlags;
    }

    if (nPrFlags & FRT_RENDTYPE_CURSCENE)
    {
        return false;
    }

    AZ_TRACE_METHOD();

    uint32 nWidth = pRT->m_nWidth;
    uint32 nHeight = pRT->m_nHeight;

    if (pRT->m_nIDInPool >= 0)
    {
        assert((int)CTexture::s_CustomRT_2D->Num() > pRT->m_nIDInPool);
        if ((int)CTexture::s_CustomRT_2D->Num() <= pRT->m_nIDInPool)
        {
            return false;
        }
        pEnvTex = &(*CTexture::s_CustomRT_2D)[pRT->m_nIDInPool];

        if (nWidth == -1)
        {
            nWidth = GetWidth();
        }
        if (nHeight == -1)
        {
            nHeight = GetHeight();
        }

        ETEX_Format eTF = pRT->m_eTF;
        // $HDR
        if (eTF == eTF_R8G8B8A8 && IsHDRModeEnabled() && m_nHDRType <= 1)
        {
            eTF = eTF_R16G16B16A16F;
        }

        // Very hi specs render reflections at half res - lower specs (and consoles) at quarter res
        bool bMakeEnvironmentTexture = false;
        if (OceanToggle::IsActive())
        {
            float fSizeScale = 0.5f;
            AZ::OceanEnvironmentBus::BroadcastResult(fSizeScale, &AZ::OceanEnvironmentBus::Events::GetReflectResolutionScale);
            fSizeScale = clamp_tpl(fSizeScale, 0.0f, 1.0f);

            nWidth = sTexLimitRes(nWidth, uint32(GetWidth() * fSizeScale));
            nHeight = sTexLimitRes(nHeight, uint32(GetHeight() * fSizeScale));

            bMakeEnvironmentTexture = (!pEnvTex->m_pTex || pEnvTex->m_pTex->GetFormat() != eTF || pEnvTex->m_pTex->GetWidth() != nWidth || pEnvTex->m_pTex->GetHeight() != nHeight);
        }
        else
        {
            float fSizeScale = (CV_r_waterreflections_quality == 5) ? 0.5f : 0.25f;

            nWidth = sTexLimitRes(nWidth, uint32(GetWidth() * fSizeScale));
            nHeight = sTexLimitRes(nHeight, uint32(GetHeight() * fSizeScale));

            bMakeEnvironmentTexture = (!pEnvTex->m_pTex || pEnvTex->m_pTex->GetFormat() != eTF);
        }

        // clamping to a reasonable texture size
        if (nWidth < 32)
        {
            nWidth = 32;
        }
        if (nHeight < 32)
        {
            nHeight = 32;
        }

        if (bMakeEnvironmentTexture)
        {
            char name[128];
            sprintf_s(name, "$RT_2D_%d", m_TexGenID++);
            int flags = FT_NOMIPS | FT_STATE_CLAMP | FT_DONT_STREAM;
            pEnvTex->m_pTex = new SDynTexture(nWidth, nHeight, eTF, eTT_2D, flags, name);
        }
        assert(nWidth > 0 && nWidth <= m_d3dsdBackBuffer.Width);
        assert(nHeight > 0 && nHeight <= m_d3dsdBackBuffer.Height);
        Tex = pEnvTex->m_pTex->m_pTexture;
    }
    else
    if (Tex)
    {
        if (Tex->GetCustomID() == TO_RT_2D)
        {
            bool bReflect = false;
            if (nPrFlags & (FRT_CAMERA_REFLECTED_PLANE | FRT_CAMERA_REFLECTED_WATERPLANE))
            {
                bReflect = true;
            }
            Matrix33 orientation = Matrix33(GetCamera().GetMatrix());
            Ang3 Angs = CCamera::CreateAnglesYPR(orientation);
            Vec3 Pos = GetCamera().GetPosition();
            bool bNeedUpdate = false;
            pEnvTex = CTexture::FindSuitableEnvTex(Pos, Angs, false, -1, false, pShader, pRes, pObj, bReflect, pRE, &bNeedUpdate);

            if (!bNeedUpdate)
            {
                if (!pEnvTex)
                {
                    return false;
                }
                if (pEnvTex->m_pTex && pEnvTex->m_pTex->m_pTexture)
                {
                    return true;
                }
            }
            m_RP.m_TI[nThreadList].m_pIgnoreObject = pObj;
            switch (CRenderer::CV_r_envtexresolution)
            {
            case 0:
                nWidth = 64;
                break;
            case 1:
                nWidth = 128;
                break;
            case 2:
            default:
                nWidth = 256;
                break;
            case 3:
                nWidth = 512;
                break;
            }
            nHeight = nWidth;
            if (!pEnvTex || !pEnvTex->m_pTex)
            {
                return false;
            }
            if (!pEnvTex->m_pTex->m_pTexture)
            {
                pEnvTex->m_pTex->Update(nWidth, nHeight);
            }
            Tex = pEnvTex->m_pTex->m_pTexture;
        }
    }
    if (m_pRT->IsRenderThread() && Tex && Tex->IsActiveRenderTarget())
    {
        return true;
    }

    // always allow for non-mgpu
    bool bMGPUAllowNextUpdate = gRenDev->GetActiveGPUCount() == 1;

    ETEX_Format eTF = pRT->m_eTF;
    // $HDR
    if (eTF == eTF_R8G8B8A8 && IsHDRModeEnabled() && m_nHDRType <= 1)
    {
        eTF = eTF_R16G16B16A16F;
    }
    if (pEnvTex && (!pEnvTex->m_pTex || pEnvTex->m_pTex->GetFormat() != eTF))
    {
        SAFE_DELETE(pEnvTex->m_pTex);
        char name[128];
        sprintf_s(name, "$RT_2D_%d", m_TexGenID++);
        int flags = FT_NOMIPS | FT_STATE_CLAMP | FT_DONT_STREAM;
        pEnvTex->m_pTex = new SDynTexture(nWidth, nHeight, eTF, eTT_2D, flags, name);
        assert(nWidth > 0 && nWidth <= m_d3dsdBackBuffer.Width);
        assert(nHeight > 0 && nHeight <= m_d3dsdBackBuffer.Height);
        pEnvTex->m_pTex->Update(nWidth, nHeight);
    }

    bool bEnableAnisotropicBlur = true;
    switch (pRT->m_eUpdateType)
    {
    case eRTUpdate_WaterReflect:
    {
        if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_waterreflections)
        {
            assert(pEnvTex != NULL);
            if (pEnvTex && pEnvTex->m_pTex && pEnvTex->m_pTex->m_pTexture)
            {
                m_pRT->RC_ClearTarget(pEnvTex->m_pTex->m_pTexture, Clr_Empty);
            }
            return true;
        }

        if (m_RP.m_nLastWaterFrameID == GetFrameID())
        {
            // water reflection already created this frame, share it
            return true;
        }

        I3DEngine* eng = (I3DEngine*)gEnv->p3DEngine;
        int nVisibleWaterPixelsCount = eng->GetOceanVisiblePixelsCount() / 2;     // bug in occlusion query returns 2x more
        int nPixRatioThreshold = (int)(GetWidth() * GetHeight() * CRenderer::CV_r_waterreflections_min_visible_pixels_update);

        static int nVisWaterPixCountPrev = nVisibleWaterPixelsCount;

        float fUpdateFactorMul = 1.0f;
        float fUpdateDistanceMul = 1.0f;
        if (nVisWaterPixCountPrev < nPixRatioThreshold / 4)
        {
            bEnableAnisotropicBlur = false;
            fUpdateFactorMul = CV_r_waterreflections_minvis_updatefactormul * 10.0f;
            fUpdateDistanceMul = CV_r_waterreflections_minvis_updatedistancemul * 5.0f;
        }
        else
        if (nVisWaterPixCountPrev < nPixRatioThreshold)
        {
            fUpdateFactorMul = CV_r_waterreflections_minvis_updatefactormul;
            fUpdateDistanceMul = CV_r_waterreflections_minvis_updatedistancemul;
        }

        float fWaterUpdateFactor = CV_r_waterupdateFactor * fUpdateFactorMul;
        float fWaterUpdateDistance = CV_r_waterupdateDistance * fUpdateDistanceMul;

        float fTimeUpd = min(0.3f, eng->GetDistanceToSectorWithWater());
        fTimeUpd *= fWaterUpdateFactor;
        //if (fTimeUpd > 1.0f)
        //fTimeUpd = 1.0f;
        Vec3 camView = m_RP.m_TI[nThreadList].m_cam.m_viewParameters.ViewDir();
        Vec3 camUp = m_RP.m_TI[nThreadList].m_cam.m_viewParameters.vY;

        m_RP.m_nLastWaterFrameID = GetFrameID();

        Vec3 camPos = GetCamera().GetPosition();
        float fDistCam = (camPos - m_RP.m_LastWaterPosUpdate).GetLength();
        float fDotView = camView * m_RP.m_LastWaterViewdirUpdate;
        float fFOV = GetCamera().GetFov();
        if (m_RP.m_fLastWaterUpdate - 1.0f > m_RP.m_TI[nThreadList].m_RealTime)
        {
            m_RP.m_fLastWaterUpdate = m_RP.m_TI[nThreadList].m_RealTime;
        }

        const float fMaxFovDiff = 0.1f;         // no exact test to prevent slowly changing fov causing per frame water reflection updates

        static bool bUpdateReflection = true;
        if (bMGPUAllowNextUpdate)
        {
            bUpdateReflection = m_RP.m_TI[nThreadList].m_RealTime - m_RP.m_fLastWaterUpdate >= fTimeUpd || fDistCam > fWaterUpdateDistance;
            bUpdateReflection = bUpdateReflection || fDotView<0.9f || fabs(fFOV - m_RP.m_fLastWaterFOVUpdate)>fMaxFovDiff;
        }

        if (bUpdateReflection && bMGPUAllowNextUpdate)
        {
            m_RP.m_fLastWaterUpdate = m_RP.m_TI[nThreadList].m_RealTime;
            m_RP.m_LastWaterViewdirUpdate = camView;
            m_RP.m_LastWaterUpdirUpdate = camUp;
            m_RP.m_fLastWaterFOVUpdate = fFOV;
            m_RP.m_LastWaterPosUpdate = camPos;
            assert(pEnvTex != NULL);
            pEnvTex->m_pTex->ResetUpdateMask();
        }
        else
        if (!bUpdateReflection)
        {
            assert(pEnvTex != NULL);
            PREFAST_ASSUME(pEnvTex != NULL);
            if (pEnvTex &&  pEnvTex->m_pTex && pEnvTex->m_pTex->IsValid())
            {
                return true;
            }
        }

        assert(pEnvTex != NULL);
        PREFAST_ASSUME(pEnvTex != NULL);
        pEnvTex->m_pTex->SetUpdateMask();
    }
    break;
    }

    // Just copy current BB to the render target and exit
    if (nPrFlags & FRT_RENDTYPE_COPYSCENE)
    {
        // Get current render target from the RT stack
        if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_debugrefraction)
        {
            FX_ScreenStretchRect(Tex);   // should encode hdr format
        }
        else
        {
            assert(Tex != NULL);
            m_pRT->RC_ClearTarget(Tex, Clr_Debug);
        }

        return true;
    }

    I3DEngine* eng = (I3DEngine*)gEnv->p3DEngine;
    Matrix44A matProj, matView;

    float plane[4];
    bool bUseClipPlane = false;
    bool bChangedCamera = false;

    int nPersFlags = m_RP.m_TI[nThreadList].m_PersFlags;
    //int nPersFlags2 = m_RP.m_TI[nThreadList].m_PersFlags2;

    static CCamera tmp_cam_mgpu = GetCamera();
    CCamera tmp_cam = GetCamera();
    CCamera prevCamera = tmp_cam;
    bool bMirror = false;
    bool bOceanRefl = false;

    // Set the camera
    if (nPrFlags & FRT_CAMERA_REFLECTED_WATERPLANE)
    {
        bOceanRefl = true;

        m_RP.m_TI[nThreadList].m_pIgnoreObject = pObj;
        float fMinDist = min(SKY_BOX_SIZE * 0.5f, eng->GetDistanceToSectorWithWater()); // 16 is half of skybox size
        float fMaxDist = eng->GetMaxViewDistance();

        Vec3 vPrevPos = tmp_cam.GetPosition();

        Plane Pl;
        Pl.n = Vec3(0, 0, 1);
        Pl.d = OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : eng->GetWaterLevel();
        if ((vPrevPos | Pl.n) - Pl.d < 0)
        {
            Pl.d = -Pl.d;
            Pl.n = -Pl.n;
        }

        plane[0] = Pl.n[0];
        plane[1] = Pl.n[1];
        plane[2] = Pl.n[2];
        plane[3] = -Pl.d;

        Matrix44 camMat;
        GetModelViewMatrix(camMat.GetData());
        Vec3 vPrevDir = Vec3(-camMat(0, 2), -camMat(1, 2), -camMat(2, 2));
        Vec3 vPrevUp = Vec3(camMat(0, 1), camMat(1, 1), camMat(2, 1));
        Vec3 vNewDir = Pl.MirrorVector(vPrevDir);
        Vec3 vNewUp = Pl.MirrorVector(vPrevUp);
        float fDot = vPrevPos.Dot(Pl.n) - Pl.d;
        Vec3 vNewPos = vPrevPos - Pl.n * 2.0f * fDot;
        Matrix34 m = sMatrixLookAt(vNewDir, vNewUp, tmp_cam.GetAngles()[2]);

        // New position + offset along view direction - minimizes projection artefacts
        m.SetTranslation(vNewPos + Vec3(vNewDir.x, vNewDir.y, 0));

        tmp_cam.SetMatrix(m);

        assert(pEnvTex);
        PREFAST_ASSUME(pEnvTex);
        tmp_cam.SetFrustum((int)(pEnvTex->m_pTex->GetWidth() * tmp_cam.GetProjRatio()), pEnvTex->m_pTex->GetHeight(), tmp_cam.GetFov(), fMinDist, fMaxDist); //tmp_cam.GetFarPlane());

        // Allow camera update
        if (bMGPUAllowNextUpdate)
        {
            tmp_cam_mgpu = tmp_cam;
        }

        SetCamera(tmp_cam_mgpu);
        bChangedCamera = true;
        bUseClipPlane = true;
        bMirror = true;
        //m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_MIRRORCULL;
    }
    else
    if (nPrFlags & FRT_CAMERA_REFLECTED_PLANE)
    {   // Mirror case
        m_RP.m_TI[nThreadList].m_pIgnoreObject = pObj;
        float fMinDist = 0.25f;
        float fMaxDist = eng->GetMaxViewDistance();

        Vec3 vPrevPos = tmp_cam.GetPosition();

        Plane Pl;
        pRE->mfGetPlane(Pl);
        //Pl.d = -Pl.d;
        if (pObj)
        {
            Matrix44 mat = pObj->m_II.m_Matrix.GetTransposed();
            Pl = TransformPlane(mat, Pl);
        }
        if ((vPrevPos | Pl.n) - Pl.d < 0)
        {
            Pl.d = -Pl.d;
            Pl.n = -Pl.n;
        }

        plane[0] = Pl.n[0];
        plane[1] = Pl.n[1];
        plane[2] = Pl.n[2];
        plane[3] = -Pl.d;

        //this is the new code to calculate the reflection matrix

        Matrix44A camMat;
        GetModelViewMatrix(camMat.GetData());
        Vec3 vPrevDir = Vec3(-camMat(0, 2), -camMat(1, 2), -camMat(2, 2));
        Vec3 vPrevUp = Vec3(camMat(0, 1), camMat(1, 1), camMat(2, 1));
        Vec3 vNewDir = Pl.MirrorVector(vPrevDir);
        Vec3 vNewUp = Pl.MirrorVector(vPrevUp);
        float fDot = vPrevPos.Dot(Pl.n) - Pl.d;
        Vec3 vNewPos = vPrevPos - Pl.n * 2.0f * fDot;
        Matrix34A m = sMatrixLookAt(vNewDir, vNewUp, tmp_cam.GetAngles()[2]);
        m.SetTranslation(vNewPos);
        tmp_cam.SetMatrix(m);

        assert(Tex);
        tmp_cam.SetFrustum((int)(Tex->GetWidth() * tmp_cam.GetProjRatio()), Tex->GetHeight(), tmp_cam.GetFov(), fMinDist, fMaxDist); //tmp_cam.GetFarPlane());
        bMirror = true;
        bUseClipPlane = true;

        SetCamera(tmp_cam);
        bChangedCamera = true;
    }
    else
    if (((nPrFlags & FRT_CAMERA_CURRENT) || (nPrFlags & FRT_RENDTYPE_CURSCENE)) && pRT->m_eOrder == eRO_PreDraw && !(nPrFlags & FRT_RENDTYPE_CUROBJECT))
    {
        // Always restore stuff after explicitly changing...

        // get texture surface
        // Get current render target from the RT stack
        if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_debugrefraction)
        {
            FX_ScreenStretchRect(Tex);   // should encode hdr format
        }
        else
        {
            m_pRT->RC_ClearTarget(Tex, Clr_Debug);
        }

        m_RP.m_TI[nThreadList].m_pIgnoreObject = pPrevIgn;
        return true;
    }

    bool bRes = true;

    m_pRT->RC_PushVP();
    m_pRT->RC_PushFog();
    m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_DRAWTOTEXTURE | RBPF_ENCODE_HDR;

    if (m_logFileHandle != AZ::IO::InvalidHandle)
    {
        Logv(SRendItem::m_RecurseLevel[nThreadList], "*** Set RT for Water reflections ***\n");
    }

    assert(pEnvTex);
    PREFAST_ASSUME(pEnvTex);
    m_pRT->RC_SetEnvTexRT(pEnvTex, pRT->m_bTempDepth ? pEnvTex->m_pTex->GetWidth() : -1, pRT->m_bTempDepth ? pEnvTex->m_pTex->GetHeight() : -1, true);
    m_pRT->RC_ClearTargetsImmediately(1, pRT->m_nFlags, pRT->m_ClearColor, pRT->m_fClearDepth);

    float fAnisoScale = 1.0f;
    if (pRT->m_nFlags & FRT_RENDTYPE_CUROBJECT)
    {
        CCryNameR& nameTech = pTech->m_NameStr;
        char newTech[128];
        sprintf_s(newTech, "%s_RT", nameTech.c_str());
        SShaderTechnique* pT = pShader->mfFindTechnique(newTech);
        if (!pT)
        {
            iLog->Log("Error: CD3D9Renderer::FX_DrawToRenderTarget: Couldn't find technique '%s' in shader '%s'\n", newTech, pShader->GetName());
        }
        else
        {
            FX_ObjectChange(pShader, pRes, pObj, pRE);
            FX_Start(pShader, -1, pRes, pRE);
            pRE->mfPrepare(false);
            FX_DrawShader_General(pShader, pT);
        }
    }
    else
    {
        if (bMirror)
        {
            if (bOceanRefl)
            {
                SetCamera(tmp_cam);
            }

            m_pRT->RC_SetEnvTexMatrix(pEnvTex);

            if (bOceanRefl)
            {
                SetCamera(tmp_cam_mgpu);
            }
        }

        m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_OBLIQUE_FRUSTUM_CLIPPING | RBPF_MIRRORCAMERA;  // | RBPF_MIRRORCULL; ??

        Plane p;
        p.n[0] = plane[0];
        p.n[1] = plane[1];
        p.n[2] = plane[2];
        p.d = plane[3]; // +0.25f;
        fAnisoScale = plane[3];
        fAnisoScale = fabs(fabs(fAnisoScale) - GetCamera().GetPosition().z);
        m_RP.m_TI[nThreadList].m_bObliqueClipPlane = true;

        // put clipplane in clipspace..
        Matrix44A mView, mProj, mCamProj, mInvCamProj;
        GetModelViewMatrix(&mView(0, 0));
        GetProjectionMatrix(&mProj(0, 0));
        mCamProj = mView * mProj;
        mInvCamProj = mCamProj.GetInverted();
        m_RP.m_TI[nThreadList].m_pObliqueClipPlane = TransformPlane2(mInvCamProj, p);

        int nRenderPassFlags = 0; 

        if (bOceanRefl && OceanToggle::IsActive())
        {
            AZ::OceanEnvironmentBus::Broadcast(&AZ::OceanEnvironmentBus::Events::ApplyReflectRenderFlags, nRenderPassFlags);
        }
        else
        {
            int nReflQuality = (bOceanRefl) ? (int)CV_r_waterreflections_quality : (int)CV_r_reflections_quality;

            // set reflection quality setting
            switch (nReflQuality)
            {
            case 1:
            case 2:
                nRenderPassFlags |= SRenderingPassInfo::ENTITIES;
                break;
            case 3:
            case 4:
            case 5:
                nRenderPassFlags |= SRenderingPassInfo::STATIC_OBJECTS | SRenderingPassInfo::ENTITIES;
                break;
            }
        }

        int nRFlags = SHDF_ALLOWHDR | SHDF_NO_DRAWNEAR;

        eng->RenderSceneReflection(nRFlags, SRenderingPassInfo::CreateRecursivePassRenderingInfo(bOceanRefl ? tmp_cam_mgpu : tmp_cam, nRenderPassFlags));

        m_RP.m_TI[nThreadList].m_bObliqueClipPlane = false;
        m_RP.m_TI[nThreadList].m_PersFlags &= ~RBPF_OBLIQUE_FRUSTUM_CLIPPING;
    }
    m_pRT->RC_PopRT(0);

    bool bUseVeryHiSpecAnisotropicReflections = false;
    if (OceanToggle::IsActive())
    {
        bool bAnisotropicReflections = false;
        if (bOceanRefl)
        {
            AZ::OceanEnvironmentBus::BroadcastResult(bAnisotropicReflections, &AZ::OceanEnvironmentBus::Events::GetReflectionAnisotropic);
        }
        else
        {
            bAnisotropicReflections = (int)CV_r_reflections_quality >= 4;
        }
        bUseVeryHiSpecAnisotropicReflections = (bAnisotropicReflections && bEnableAnisotropicBlur && Tex && Tex->GetDevTexture());
    }
    else
    {
        int nReflQuality = (bOceanRefl) ? (int)CV_r_waterreflections_quality : (int)CV_r_reflections_quality;
        bUseVeryHiSpecAnisotropicReflections = (nReflQuality >= 4 && bEnableAnisotropicBlur && Tex && Tex->GetDevTexture());
    }

    // Very Hi specs get anisotropic reflections?
    if (bUseVeryHiSpecAnisotropicReflections)
    {
        m_pRT->RC_TexBlurAnisotropicVertical(Tex, fAnisoScale);
    }

    if (m_logFileHandle != AZ::IO::InvalidHandle)
    {
        Logv(SRendItem::m_RecurseLevel[nThreadList], "*** End RT for Water reflections ***\n");
    }

    // todo: encode hdr format

    m_RP.m_TI[nThreadList].m_PersFlags = nPersFlags;
    //m_RP.m_TI[nThreadList].m_PersFlags2 = nPersFlags2;

    if (bChangedCamera)
    {
        SetCamera(prevCamera);
    }

    m_pRT->RC_PopVP();
    m_pRT->RC_PopFog();

    // increase frame id to support multiple recursive draws
    m_RP.m_TI[nThreadList].m_nFrameID++;
    m_RP.m_TI[nThreadList].m_pIgnoreObject = pPrevIgn;

    return bRes;
}
