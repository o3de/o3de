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
#include "../CryFont/FBitmap.h"
//=========================================================================================

bool CD3D9Renderer::FontUpdateTexture(int nTexId, int nX, int nY, int USize, int VSize, byte* pData)
{
    CTexture* tp = CTexture::GetByID(nTexId);
    assert (tp);

    if (tp)
    {
        tp->UpdateTextureRegion(pData, nX, nY, 0, USize, VSize, 0, eTF_A8);

        return true;
    }
    return false;
}

bool CD3D9Renderer::FontUploadTexture(class CFBitmap* pBmp, ETEX_Format eTF)
{
    if (!pBmp)
    {
        return false;
    }

    unsigned int* pData = new unsigned int[pBmp->GetWidth() * pBmp->GetHeight()];

    if (!pData)
    {
        return false;
    }

    pBmp->Get32Bpp(&pData);

    char szName[128];
    azsprintf(szName, "$AutoFont_%d", m_TexGenID++);

    // FT_DONT_RELEASE was previously set here and caused the VRAM from font textures to never be released
    int iFlags = FT_TEX_FONT | FT_DONT_STREAM;
    CTexture* tp = CTexture::Create2DTexture(szName, pBmp->GetWidth(), pBmp->GetHeight(), 1, iFlags, (unsigned char*)pData, eTF, eTF);

    SAFE_DELETE_ARRAY(pData);

    pBmp->SetRenderData((void*)tp);

    return true;
}

int CD3D9Renderer::FontCreateTexture(int Width, int Height, byte* pData, ETEX_Format eTF, bool genMips, const char* textureName)
{
    if (!pData)
    {
        return -1;
    }

    char szName[128];
    if (textureName)
    {
        sprintf_s(szName, "$AutoFont_%d_%s", m_TexGenID++, textureName);
    }
    else
    {
        sprintf_s(szName, "$AutoFont_%d", m_TexGenID++);
    }
    
    // FT_DONT_RELEASE was previously set here and caused the VRAM from font textures to never be released
    int iFlags = FT_TEX_FONT | FT_DONT_STREAM;
    if (genMips)
    {
        iFlags |= FT_FORCE_MIPS;
    }
    CTexture* tp = CTexture::Create2DTexture(szName, Width, Height, 1, iFlags, (unsigned char*)pData, eTF, eTF);

    return tp->GetID();
}

void CD3D9Renderer::FontReleaseTexture(class CFBitmap* pBmp)
{
    if (!pBmp)
    {
        return;
    }

    CTexture* tp = (CTexture*)pBmp->GetRenderData();

    SAFE_RELEASE(tp);
}

void CD3D9Renderer::FontSetTexture(class CFBitmap* pBmp, int nFilterMode)
{
    if (pBmp)
    {
        CTexture* tp = (CTexture*)pBmp->GetRenderData();
        if (tp)
        {
            tp->SetFilterMode(nFilterMode);
            tp->Apply(0);
        }
    }
}

void CD3D9Renderer::FontSetTexture(int nTexId, int nFilterMode)
{
    if (nTexId <= 0)
    {
        return;
    }
    CTexture* tp = CTexture::GetByID(nTexId);
    assert (tp);
    if (!tp)
    {
        return;
    }

    tp->SetFilterMode(nFilterMode);
    tp->Apply(0);
    //CTexture::m_Text_NoTexture->Apply(0);
}

int s_InFontState = 0;

void CD3D9Renderer::FontSetRenderingState(bool overrideViewProjMatrices, TransformationMatrices& backupMatrices)
{
    assert(!s_InFontState);
    assert(m_pRT->IsRenderThread());

    // setup various d3d things that we need
    FontSetState(false, GS_DEPTHFUNC_LEQUAL);

    s_InFontState++;

    if (overrideViewProjMatrices)
    {
        Matrix44A *m;
        backupMatrices.m_projectMatrix = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj;
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj.SetIdentity();
        m = &m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj;
    
        if (m_NewViewport.nWidth != 0 && m_NewViewport.nHeight != 0)
        {
            const bool bReverseDepth = (m_RP.m_TI[m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
            const float zn = bReverseDepth ?  1.0f : -1.0f;
            const float zf = bReverseDepth ? -1.0f :  1.0f;

            mathMatrixOrthoOffCenter(m, 0.0f, (float)m_NewViewport.nWidth, (float)m_NewViewport.nHeight, 0.0f, zn, zf);
        }

        backupMatrices.m_viewMatrix = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView.SetIdentity();
    }
}

void CD3D9Renderer::FontRestoreRenderingState(bool overrideViewProjMatrices, const TransformationMatrices& restoringMatrices)
{
    assert(m_pRT->IsRenderThread());
    assert(s_InFontState == 1);
    s_InFontState--;

    if (overrideViewProjMatrices)
    {
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView = restoringMatrices.m_viewMatrix;
        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj = restoringMatrices.m_projectMatrix;
    }

    FontSetState(true, GS_DEPTHFUNC_LEQUAL);
}

void CD3D9Renderer::FontSetBlending(int blendSrc, int blendDest, int baseState)
{
    assert(m_pRT->IsRenderThread());
    m_fontBlendMode = blendSrc | blendDest;

    int additionalFontStateFlags = GS_DEPTHFUNC_LEQUAL;
    int alphaReference = -1;
    if (eTF_R16G16F != CTexture::s_eTFZ && CTexture::s_eTFZ != eTF_R32F)
    {
        additionalFontStateFlags = GS_DEPTHFUNC_LEQUAL | GS_ALPHATEST_GREATER;
        alphaReference = 0;
    }

    FX_SetState(m_fontBlendMode | ((baseState == -1) ? additionalFontStateFlags : baseState), alphaReference);
}

void CD3D9Renderer::FontSetState(bool bRestore, int baseState)
{
    static DWORD wireframeMode;
#if !defined(OPENGL)
    static D3DCOLORVALUE color;
#endif //!defined(OPENGL)
    static bool bMatColor;
    static uint32 nState, nForceState;

    assert(m_pRT->IsRenderThread());

    // grab the modes that we might need to change
    if (!bRestore)
    {
        D3DSetCull(eCULL_None);

        nState = m_RP.m_CurState;
        nForceState = m_RP.m_StateOr;
        wireframeMode = m_wireframe_mode;

        EF_SetVertColor();

        if (wireframeMode > R_SOLID_MODE)
        {
            SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
            RS.Desc.FillMode = D3D11_FILL_SOLID;
            gcpRendD3D->SetRasterState(&RS);
        }

        m_RP.m_FlagsPerFlush = 0;
        int additionalFontStateFlags = GS_DEPTHFUNC_LEQUAL;
        int alphaReference = -1;
        if (eTF_R16G16F != CTexture::s_eTFZ && CTexture::s_eTFZ != eTF_R32F)
        {
            additionalFontStateFlags = GS_DEPTHFUNC_LEQUAL | GS_ALPHATEST_GREATER;
            alphaReference = 0;
        }

        FX_SetState(m_fontBlendMode | ((baseState == -1) ? additionalFontStateFlags : baseState), alphaReference);
        EF_SetColorOp(eCO_REPLACE, eCO_MODULATE, eCA_Diffuse | (eCA_Diffuse << 3), DEF_TEXARG0);

    }
    else
    {
        if (wireframeMode == R_WIREFRAME_MODE)
        {
            SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
            RS.Desc.FillMode = D3D11_FILL_WIREFRAME;
            gcpRendD3D->SetRasterState(&RS);
        }
        m_RP.m_StateOr = nForceState;

        //FX_SetState(State);
    }
}

void CD3D9Renderer::DrawDynVB(SVF_P3F_C4B_T2F* pBuf, uint16* pInds, int nVerts, int nInds, const PublicRenderPrimitiveType nPrimType)
{
    PROFILE_FRAME(Draw_IndexMesh_Dyn);

    if (!pBuf)
    {
        return;
    }
    if (m_bDeviceLost)
    {
        return;
    }

    //if (CV_d3d9_forcesoftware)
    //  return;
    if (!nVerts || (pInds && !nInds))
    {
        return;
    }

    m_pRT->RC_DrawDynVB(pBuf, pInds, nVerts, nInds, nPrimType);
}

void CD3D9Renderer::DrawDynUiPrimitiveList(DynUiPrimitiveList& primitives, int totalNumVertices, int totalNumIndices)
{
    PROFILE_FRAME(Draw_IndexMesh_Dyn);

    if (m_bDeviceLost)
    {
        return;
    }

    m_pRT->RC_DrawDynUiPrimitiveList(primitives, totalNumVertices, totalNumIndices);
}
