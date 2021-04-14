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
#include "../Common/Textures/TextureHelpers.h"
#include "../Common/RendElements/FlareSoftOcclusionQuery.h"
#include "../Common/ReverseDepth.h"
#include "../Common/RenderCapabilities.h"
#include "D3DTiledShading.h"

#include <AzCore/std/sort.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DDEFERREDSHADING_CPP_SECTION_1 1
#define D3DDEFERREDSHADING_CPP_SECTION_2 2
#define D3DDEFERREDSHADING_CPP_SECTION_3 3
#define D3DDEFERREDSHADING_CPP_SECTION_4 4
#define D3DDEFERREDSHADING_CPP_SECTION_5 5
#define D3DDEFERREDSHADING_CPP_SECTION_6 6
#endif

#if defined(FEATURE_SVO_GI)
#include "D3D_SVO.h"
#endif
#include "GraphicsPipeline/FurPasses.h"

#define MAX_VIS_AREAS 32

// MSAA potential optimizations todo:
//  - long term: port all functionality to compute, including all extra effects passes.

// About MSAA:
// - Please be careful when accessing or rendering into msaa'ed targets. When adding new techniques please make sure to test
// - For post process technique to be MSAA friendly, do either:
//    - Use compute. Single pass and as efficient as gets. Context switches might be problematic, until all lighting pipeline done like this.
//    - For non compute, require 2 passes. One at pixel frequency, other at sub sample frequency.
//               - Reuse existing sample frequency regions on stencil via stencilread/write mask:
//                      - If not possible, tag pixel frequency regions using stencil + m_pMSAAMaskRT
//                      - Alternative poor man version, do clip in shader.

CDeferredShading* CDeferredShading::m_pInstance = NULL;

#define RT_LIGHTSMASK g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5] | g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ] | g_HWSR_MaskBit[HWSR_CUBEMAP0] | g_HWSR_MaskBit[HWSR_APPLY_SSDO] | g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION]
#define RT_LIGHTPASS_RESETMASK g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE5] | g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ] | g_HWSR_MaskBit[HWSR_CUBEMAP0] | g_HWSR_MaskBit[HWSR_APPLY_SSDO] | g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION]
#define RT_DEBUGMASK g_HWSR_MaskBit[HWSR_DEBUG0] | g_HWSR_MaskBit[HWSR_DEBUG1] | g_HWSR_MaskBit[HWSR_DEBUG2] | g_HWSR_MaskBit[HWSR_DEBUG3]
#define RT_TEX_PROJECT g_HWSR_MaskBit[HWSR_SAMPLE0]
#define RT_GLOBAL_CUBEMAP g_HWSR_MaskBit[HWSR_SAMPLE0]
#define RT_SPECULAR_CUBEMAP g_HWSR_MaskBit[HWSR_SAMPLE1]
#define RT_AMBIENT_LIGHT g_HWSR_MaskBit[HWSR_SAMPLE5]
#define RT_GLOBAL_CUBEMAP_IGNORE_VISAREAS g_HWSR_MaskBit[HWSR_SAMPLE4]
#define RT_AREALIGHT g_HWSR_MaskBit[HWSR_SAMPLE1]
#define RT_OVERDRAW_DEBUG g_HWSR_MaskBit[HWSR_DEBUG0]
#define RT_BOX_PROJECTION g_HWSR_MaskBit[HWSR_SAMPLE3]
#define RT_CLIPVOLUME_ID g_HWSR_MaskBit[HWSR_LIGHTVOLUME0]

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CTexPoolAtlas::Init(int nSize)
{
    m_nSize = nSize;
    Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CTexPoolAtlas::Clear()
{
    memset(&m_arrAllocatedBlocks[0], 0, MAX_BLOCKS * sizeof(m_arrAllocatedBlocks[0]));
#ifdef _DEBUG
    m_nTotalWaste = 0;
    m_arrDebugBlocks.clear();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CTexPoolAtlas::FreeMemory()
{
#ifdef _DEBUG
    stl::free_container(m_arrDebugBlocks);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CTexPoolAtlas::AllocateGroup(int32* pOffsetX, int32* pOffsetY, int nSizeX, int nSizeY)
{
    int nBorder = 2;
    nSizeX += nBorder << 1;
    nSizeY += nBorder << 1;

    if (nSizeX > m_nSize || nSizeY > m_nSize)
    {
        return false;
    }

    uint16 nBestX = 0;
    uint16 nBestY = 0;
    uint16 nBestID = 0;
    uint32 nBestWaste = ~0;

    // outer loop over all relevant allocated blocks (Y dimension)
    int nCurrY = 0;
    for (int nCurrBlockID = 0;
         m_arrAllocatedBlocks[max(nCurrBlockID - 1, 0)] > 0 && nCurrY <= m_nSize - nSizeY && nBestWaste > 0;
         ++nCurrBlockID)
    {
        const uint32 nCurrBlock = m_arrAllocatedBlocks[nCurrBlockID];
        const uint16 nCurrBlockWidth = LOWORD(nCurrBlock);
        const uint16 nCurrBlockHeight = HIWORD(nCurrBlock);

        // get max X for intersected blocks
        int nCurrX = nCurrBlockWidth;
        int nNextY = nCurrBlockHeight;
        for (uint16 nNextBlockID = nCurrBlockID + 1;
             m_arrAllocatedBlocks[nNextBlockID] > 0 && nNextY < nSizeY;
             ++nNextBlockID)
        {
            const uint32 nNextBlock = m_arrAllocatedBlocks[nNextBlockID];
            const uint16 nNextBlockWidth = LOWORD(nNextBlock);
            const uint16 nNextBlockHeight = HIWORD(nNextBlock);
            nCurrX = max(nCurrX, int(nNextBlockWidth));
            nNextY += nNextBlockHeight;
        }

        // can fit next to them?
        if (nSizeX <= m_nSize - nCurrX)
        {
            // compute waste
            uint32 nWaste = 0;
            nNextY = nCurrY;
            for (uint16 nNextBlockID = nCurrBlockID;
                 m_arrAllocatedBlocks[nNextBlockID] > 0 && nNextY < nCurrY + nSizeY;
                 ++nNextBlockID)
            {
                const uint32 nNextBlock = m_arrAllocatedBlocks[nNextBlockID];
                const uint16 nNextBlockWidth = LOWORD(nNextBlock);
                const uint16 nNextBlockHeight = HIWORD(nNextBlock);
                nWaste += (nCurrX - nNextBlockWidth) * (min(nCurrY + nSizeY, nNextY + nNextBlockHeight) - max(nCurrY, nNextY));
                nNextY += nNextBlockHeight;
            }
            nWaste += max(nCurrY + nSizeY - nNextY, 0) * nCurrX;

            // right spot?
            if (nWaste < nBestWaste)
            {
                nBestX = nCurrX;
                nBestY = nCurrY;
                nBestID = nCurrBlockID;
                nBestWaste = nWaste;
            }
        }

        nCurrY += nCurrBlockHeight;
    }

    if ((nBestX | nBestY) || nCurrY <= m_nSize - nSizeY)
    {
        assert(nBestID < MAX_BLOCKS - 1);
        if (nBestID >= MAX_BLOCKS - 1)
        {
            return false;
        }

        *pOffsetX = nBestX + nBorder;
        *pOffsetY = nBestY + nBorder;

        // block to be added, update block info
        uint32 nBlockData = m_arrAllocatedBlocks[nBestID];
        const uint16 nReplacedHeight = HIWORD(nBlockData);
        if (nSizeY < nReplacedHeight)
        {
            nBlockData = MAKELONG(nBlockData, nReplacedHeight - nSizeY);
            // shift by 1
            for (uint16 nID = nBestID + 1; nBlockData > 0; ++nID)
            {
                std::swap(m_arrAllocatedBlocks[nID], nBlockData);
            }
        }
        else if (nSizeY > nReplacedHeight)
        {
            uint16 nCoveredHeight = nReplacedHeight;
            uint16 nBlocksToSkip = 0;
            nBlockData = m_arrAllocatedBlocks[nBestID + 1];
            for (uint16 nID = nBestID + 1; nBlockData > 0; nBlockData = m_arrAllocatedBlocks[++nID])
            {
                const uint16 nCurrHeight = HIWORD(nBlockData);
                nCoveredHeight += nCurrHeight;
                if (nSizeY >= nCoveredHeight)
                {
                    nBlocksToSkip++;
                }
                else
                {
                    m_arrAllocatedBlocks[nID] = MAKELONG(nBlockData, nCoveredHeight - nSizeY);
                    break;
                }
            }
            // shift by nBlocksToSkip
            uint16 nID = nBestID + nBlocksToSkip + 1;
            nBlockData = m_arrAllocatedBlocks[nID];
            for (; nBlockData > 0; nBlockData = m_arrAllocatedBlocks[++nID])
            {
                m_arrAllocatedBlocks[nID - nBlocksToSkip] = nBlockData;
            }
        }
        m_arrAllocatedBlocks[nBestID] = MAKELONG(nBestX + nSizeX, nSizeY);

#ifdef _DEBUG
        if (m_arrDebugBlocks.size())
        {
            m_nTotalWaste += nBestWaste;
        }
        _AddDebugBlock(nBestX, nBestY, nSizeX, nSizeY);
#endif

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CTexPoolAtlas::_AddDebugBlock(int x, int y, int sizeX, int sizeY)
{
    SShadowMapBlock block;
    block.m_nX1 = x;
    block.m_nX2 = x + sizeX;
    block.m_nY1 = y;
    block.m_nY2 = y + sizeY;
    assert(block.m_nX2 <= m_nSize && block.m_nY2 <= m_nSize);
    for (std::vector<SShadowMapBlock>::const_iterator it = m_arrDebugBlocks.begin();
         it != m_arrDebugBlocks.end(); it++)
    {
        assert(!block.Intersects(*it));
    }
    m_arrDebugBlocks.push_back(block);
}

float CTexPoolAtlas::_GetDebugUsage() const
{
    uint32 nUsed = 0;
    for (std::vector<SShadowMapBlock>::const_iterator it = m_arrDebugBlocks.begin();
         it != m_arrDebugBlocks.end(); it++)
    {
        nUsed += (it->m_nX2 - it->m_nX1) * (it->m_nY2 - it->m_nY1);
    }

    return 100.f * nUsed / (m_nSize * m_nSize);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SRenderLight::CalculateScissorRect()
{
    Vec3 cameraPos = gcpRendD3D->GetCamera().GetPosition();
    Vec3 vViewVec = m_Origin - cameraPos;
    float fDistToLS =  vViewVec.GetLength();

    // Use max of width/height for area lights.
    float fMaxRadius = m_fRadius;
    
    if (m_Flags & DLF_AREA_LIGHT) // Use max for area lights.
    {
        fMaxRadius += max(m_fAreaWidth, m_fAreaHeight);
    }
    else if (m_Flags & DLF_DEFERRED_CUBEMAPS)
    {
        fMaxRadius = m_ProbeExtents.len();  // This is not optimal for a box
    }
    ITexture* pLightTexture = m_pLightImage ? m_pLightImage : NULL;
    bool bProjectiveLight = (m_Flags & DLF_PROJECT) && pLightTexture && !(pLightTexture->GetFlags() & FT_REPLICATE_TO_ALL_SIDES);
    bool bInsideLightVolume = fDistToLS <= fMaxRadius;
    if (bInsideLightVolume && !bProjectiveLight)
    {
        //optimization when we are inside light frustum
        m_sX = 0;
        m_sY = 0;
        m_sWidth  = gcpRendD3D->GetWidth();
        m_sHeight = gcpRendD3D->GetHeight();

        return;
    }

    // e_ScissorDebug will modify the view matrix here, so take a local copy
    const Matrix44& mView = gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_matView;
    const Matrix44& mProj = gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_matProj;

    Vec3 vCenter = m_Origin;
    float fRadius = fMaxRadius;

    const int nMaxVertsToProject = 10;
    int nVertsToProject = 4;
    Vec3 pBRectVertices[nMaxVertsToProject];

    Vec4 vCenterVS = Vec4(vCenter, 1) * mView;

    if (!bInsideLightVolume)
    {
        // Compute tangent planes
        float r = fRadius;
        float sq_r = r * r;

        Vec3 vLPosVS = Vec3(vCenterVS.x, vCenterVS.y, vCenterVS.z);
        float lx = vLPosVS.x;
        float ly = vLPosVS.y;
        float lz = vLPosVS.z;
        float sq_lx = lx * lx;
        float sq_ly = ly * ly;
        float sq_lz = lz * lz;

        // Compute left and right tangent planes to light sphere
        float sqrt_d = sqrt_tpl(max(sq_r * sq_lx  - (sq_lx + sq_lz) * (sq_r - sq_lz), 0.0f));
        float nx = iszero(sq_lx + sq_lz) ? 1.0f : (r * lx + sqrt_d) / (sq_lx + sq_lz);
        float nz = iszero(lz) ? 1.0f : (r - nx * lx) / lz;

        Vec3 vTanLeft = Vec3(nx, 0, nz).normalized();

        nx = iszero(sq_lx + sq_lz) ? 1.0f : (r * lx - sqrt_d) / (sq_lx + sq_lz);
        nz = iszero(lz) ? 1.0f : (r - nx * lx) / lz;
        Vec3 vTanRight = Vec3(nx, 0, nz).normalized();

        pBRectVertices[0] = vLPosVS - r * vTanLeft;
        pBRectVertices[1] = vLPosVS - r * vTanRight;

        // Compute top and bottom tangent planes to light sphere
        sqrt_d = sqrt_tpl(max(sq_r * sq_ly  - (sq_ly + sq_lz) * (sq_r - sq_lz), 0.0f));
        float ny = iszero(sq_ly + sq_lz) ? 1.0f : (r * ly - sqrt_d) / (sq_ly + sq_lz);
        nz = iszero(lz) ? 1.0f : (r - ny * ly) / lz;
        Vec3 vTanBottom = Vec3(0, ny, nz).normalized();

        ny = iszero(sq_ly + sq_lz) ? 1.0f :  (r * ly + sqrt_d) / (sq_ly + sq_lz);
        nz = iszero(lz) ? 1.0f : (r - ny * ly) / lz;
        Vec3 vTanTop = Vec3(0, ny, nz).normalized();

        pBRectVertices[2] = vLPosVS - r * vTanTop;
        pBRectVertices[3] = vLPosVS - r * vTanBottom;
    }

    if (bProjectiveLight)
    {
        // todo: improve/simplify projective case

        Vec3 vRight  = m_ObjMatrix.GetColumn2();
        Vec3 vUp      = -m_ObjMatrix.GetColumn1();
        Vec3 pDirFront = m_ObjMatrix.GetColumn0();
        pDirFront.NormalizeFast();

        // Cone radius
        float fConeAngleThreshold = 0.0f;
        float fConeRadiusScale = /*min(1.0f,*/ tan_tpl((m_fLightFrustumAngle + fConeAngleThreshold) * (gf_PI / 180.0f));  //);
        float fConeRadius = fRadius * fConeRadiusScale;

        Vec3 pDiagA = (vUp + vRight);
        float fDiagLen = 1.0f / pDiagA.GetLengthFast();
        pDiagA *= fDiagLen;

        Vec3 pDiagB = (vUp - vRight);
        pDiagB *= fDiagLen;

        float fPyramidBase =  sqrt_tpl(fConeRadius * fConeRadius * 2.0f);
        pDirFront *= fRadius;

        Vec3 pEdgeA  = (pDirFront + pDiagA * fPyramidBase);
        Vec3 pEdgeA2 = (pDirFront - pDiagA * fPyramidBase);
        Vec3 pEdgeB  = (pDirFront + pDiagB * fPyramidBase);
        Vec3 pEdgeB2 = (pDirFront - pDiagB * fPyramidBase);

        uint32 nOffset = 4;

        // Check whether the camera is inside the extended bounding sphere that contains pyramid

        // we are inside light frustum
        // Put all pyramid vertices in view space
        Vec4 pPosVS = Vec4(vCenter, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
        pPosVS = Vec4(vCenter + pEdgeA, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
        pPosVS = Vec4(vCenter + pEdgeB, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
        pPosVS = Vec4(vCenter + pEdgeA2, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);
        pPosVS = Vec4(vCenter + pEdgeB2, 1) * mView;
        pBRectVertices[nOffset++] = Vec3(pPosVS.x, pPosVS.y, pPosVS.z);

        nVertsToProject = nOffset;
    }

    Vec3 vPMin = Vec3(1, 1, 999999.0f);
    Vec2 vPMax = Vec2(0, 0);
    Vec2 vMin = Vec2(1, 1);
    Vec2 vMax = Vec2(0, 0);

    int nStart = 0;

    if (bInsideLightVolume)
    {
        nStart = 4;
        vMin = Vec2(0, 0);
        vMax = Vec2(1, 1);
    }

    const ICVar* scissorDebugCVar = iConsole->GetCVar("e_ScissorDebug");
    int scissorDebugEnabled = scissorDebugCVar ? scissorDebugCVar->GetIVal() : 0;
    Matrix44 invertedView;
    if (scissorDebugEnabled)
    {
        invertedView = mView.GetInverted();
    }

    // Project all vertices
    for (int i = nStart; i < nVertsToProject; i++)
    {
        if (scissorDebugEnabled)
        {
            if (gcpRendD3D->GetIRenderAuxGeom() != NULL)
            {
                Vec4 pVertWS = Vec4(pBRectVertices[i], 1) * invertedView;
                Vec3 v = Vec3(pVertWS.x, pVertWS.y, pVertWS.z);
                gcpRendD3D->GetIRenderAuxGeom()->DrawPoint(v, RGBA8(0xff, 0xff, 0xff, 0xff), 10);

                int32 nPrevVert = (i - 1) < 0 ? nVertsToProject - 1 : (i - 1);
                pVertWS = Vec4(pBRectVertices[nPrevVert], 1) * invertedView;
                Vec3 v2 = Vec3(pVertWS.x, pVertWS.y, pVertWS.z);
                gcpRendD3D->GetIRenderAuxGeom()->DrawLine(v, RGBA8(0xff, 0xff, 0x0, 0xff), v2, RGBA8(0xff, 0xff, 0x0, 0xff), 3.0f);
            }
        }

        Vec4 vScreenPoint = Vec4(pBRectVertices[i], 1.0) * mProj;

        //projection space clamping
        vScreenPoint.w = max(vScreenPoint.w, 0.00000000000001f);
        vScreenPoint.x = max(vScreenPoint.x, -(vScreenPoint.w));
        vScreenPoint.x = min(vScreenPoint.x, vScreenPoint.w);
        vScreenPoint.y = max(vScreenPoint.y, -(vScreenPoint.w));
        vScreenPoint.y = min(vScreenPoint.y, vScreenPoint.w);

        //NDC
        vScreenPoint /= vScreenPoint.w;

        //output coords
        //generate viewport (x=0,y=0,height=1,width=1)
        Vec2 vWin;
        vWin.x = (1.0f + vScreenPoint.x) *  0.5f;
        vWin.y = (1.0f + vScreenPoint.y) *  0.5f;  //flip coords for y axis

        // clamp to [0.0, 1.0]
        vWin.x = clamp_tpl<float>(vWin.x, 0.0f, 1.0f);
        vWin.y = clamp_tpl<float>(vWin.y, 0.0f, 1.0f);

        assert(vWin.x >= 0.0f && vWin.x <= 1.0f);
        assert(vWin.y >= 0.0f && vWin.y <= 1.0f);

        if (bProjectiveLight && i >= 4)
        {
            // Get light pyramid screen bounds
            vPMin.x = min(vPMin.x, vWin.x);
            vPMin.y = min(vPMin.y, vWin.y);
            vPMax.x = max(vPMax.x, vWin.x);
            vPMax.y = max(vPMax.y, vWin.y);

            vPMin.z = min(vPMin.z, vScreenPoint.z); // if pyramid intersects the nearplane, the test is unreliable. (requires proper clipping)
        }
        else
        {
            // Get light sphere screen bounds
            vMin.x = min(vMin.x, vWin.x);
            vMin.y = min(vMin.y, vWin.y);
            vMax.x = max(vMax.x, vWin.x);
            vMax.y = max(vMax.y, vWin.y);
        }
    }

    int iWidth = gcpRendD3D->GetWidth();
    int iHeight = gcpRendD3D->GetHeight();
    float fWidth = (float)iWidth;
    float fHeight = (float)iHeight;

    if (bProjectiveLight)
    {
        vPMin.x = (float)fsel(vPMin.z, vPMin.x, vMin.x); // Use sphere bounds if pyramid bounds are unreliable
        vPMin.y = (float)fsel(vPMin.z, vPMin.y, vMin.y);
        vPMax.x = (float)fsel(vPMin.z, vPMax.x, vMax.x);
        vPMax.y = (float)fsel(vPMin.z, vPMax.y, vMax.y);

        // Clamp light pyramid bounds to light sphere screen bounds
        vMin.x = clamp_tpl<float>(vPMin.x, vMin.x, vMax.x);
        vMin.y = clamp_tpl<float>(vPMin.y, vMin.y, vMax.y);
        vMax.x = clamp_tpl<float>(vPMax.x, vMin.x, vMax.x);
        vMax.y = clamp_tpl<float>(vPMax.y, vMin.y, vMax.y);
    }

    m_sX = (short)(vMin.x * fWidth);
    m_sY = (short)((1.0f - vMax.y) * fHeight);
    m_sWidth = (short)ceilf((vMax.x - vMin.x) * fWidth);
    m_sHeight = (short)ceilf((vMax.y - vMin.y) * fHeight);

    // make sure we don't create a scissor rect out of bound (D3DError)
    m_sWidth = (m_sX + m_sWidth) > iWidth ? iWidth - m_sX : m_sWidth;
    m_sHeight = (m_sY + m_sHeight) > iHeight ? iHeight - m_sY : m_sHeight;

#if !defined(RELEASE)
    if (scissorDebugEnabled)
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

            const float screenWidth = (float)gcpRendD3D->GetWidth();
            const float screenHeight = (float)gcpRendD3D->GetHeight();

            // Calc resolve area
            const float left = m_sX / screenWidth;
            const float top = m_sY / screenHeight;
            const float right = (m_sX + m_sWidth) / screenWidth;
            const float bottom = (m_sY + m_sHeight) / screenHeight;

            // Render resolve area
            ColorB areaColor(50, 0, 50, 255);

            if (vPMin.z < 0.0f)
            {
                areaColor = ColorB(0, 100, 0, 255);
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
#endif
}

uint32 CDeferredShading::AddLight(const CDLight& pDL, float fMult, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    const uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
    const int32 nRecurseLevel = passInfo.GetRecursiveLevel();

    eDeferredLightType LightType = eDLT_DeferredLight;
    if (pDL.m_Flags & DLF_DEFERRED_CUBEMAPS)
    {
        LightType = eDLT_DeferredCubemap;
    }
    else if (pDL.m_Flags & DLF_AMBIENT)
    {
        LightType = eDLT_DeferredAmbientLight;
    }

    if (pDL.GetLensOpticsElement() && !pDL.m_pSoftOccQuery)
    {
        CDLight* pLight = const_cast<CDLight*>(&pDL);

        const uint8 numVisibilityFaders = 2; // For each flare type
        pLight->m_pSoftOccQuery = new CFlareSoftOcclusionQuery(numVisibilityFaders);
    }

    TArray<SRenderLight>& rArray = m_pLights[LightType][nThreadID][nRecurseLevel];

    const int32 lightsNum = (int32)rArray.Num();
    int idx = -1;

    rArray.AddElem(pDL);
    idx = rArray.Num() - 1;
    rArray[idx].m_lightId = idx;
    rArray[idx].AcquireResources();

    IF_LIKELY (eDLT_DeferredLight == LightType)
    {
        rArray[idx].m_Color *= fMult;
        rArray[idx].m_SpecMult *= fMult;
    }
    else if (eDLT_DeferredAmbientLight == LightType)
    {
        ColorF origCol(rArray[idx].m_Color);
        rArray[idx].m_Color.lerpFloat(Col_White, origCol, fMult);
    }
    else if (eDLT_DeferredCubemap == LightType)
    {
        rArray[idx].m_fProbeAttenuation *= fMult;
    }
    else
    {
        AZ_Assert(false, "Unhandled eDeferredLightType %d", LightType);
    }

    gcpRendD3D->EF_CheckLightMaterial(const_cast<CDLight*>(&pDL), idx, passInfo, rendItemSorter);

    return idx;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

TArray<SRenderLight>& CDeferredShading::GetLights(int nThreadID, int nCurRecLevel, const eDeferredLightType eType)
{
    return m_pLights[eType][nThreadID][nCurRecLevel];
}

SRenderLight* CDeferredShading::GetLightByID(const uint16 nLightID, const eDeferredLightType eType)
{
    const uint32 nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
    assert(SRendItem::m_RecurseLevel[nThreadID] >= 0);
    const int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];

    TArray<SRenderLight>& pLightsList = GetLights(nThreadID, nRecurseLevel,  eType);
    SRenderLight* foundIter = AZStd::find_if(pLightsList.begin(), pLightsList.end(), [&](const SRenderLight& light) {return light.m_lightId == nLightID; });
    if (foundIter != pLightsList.end())
    {
        return foundIter;
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeferredShading::GetClipVolumeParams(const Vec4*& pParams, uint32& nCount)
{
    pParams = m_vClipVolumeParams;
    nCount = m_nClipVolumesCount[m_nThreadID][m_nRecurseLevel];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CDeferredShading::GetLightsNum(const eDeferredLightType eType)
{
    uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
    int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];

    assert(nRecurseLevel >= 0);

    return m_pLights[eType][nThreadID][nRecurseLevel].size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::ResetLights()
{
    uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
    int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nRecurseLevel >= 0);

    for (uint32 iLightType = 0; iLightType < eDLT_NumLightTypes; ++iLightType)
    {
        TArray<SRenderLight>& pLightList = m_pLights[iLightType][nThreadID][nRecurseLevel];

        const uint32 nNumLights = pLightList.size();
        for (uint32 l = 0; l < nNumLights; ++l)
        {
            SRenderLight& pLight = pLightList[l];
            pLight.DropResources();
        }

        pLightList.SetUse(0);
    }
    m_vecGIClipVolumes[nThreadID][nRecurseLevel].resize(0);

    gcpRendD3D->GetVolumetricFog().Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::ResetAllLights()
{
    for (size_t i = 0; i < eDLT_NumLightTypes; ++i)
    {
        for (size_t j = 0; j < RT_COMMAND_BUF_COUNT; ++j)
        {
            for (size_t k = 0; k < MAX_REND_RECURSION_LEVELS; ++k)
            {
                TArray<SRenderLight>& pLightList = m_pLights[i][j][k];

                const uint32 nNumLights = pLightList.size();
                for (uint32 l = 0; l < nNumLights; ++l)
                {
                    SRenderLight& pLight = pLightList[l];
                    pLight.DropResources();
                }

                pLightList.Free();
            }
        }
    }

    gcpRendD3D->GetTiledShading().Clear();

    gcpRendD3D->GetVolumetricFog().ClearAll();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::ReleaseData()
{
    // When the engine shutsdown this method gets called twice: once for when the level
    // is unloaded (main thread) and once when the renderer is shutdown (render thread).
    // Because m_nShadowPoolSize gets set to zero only in this method, we can use it as
    // flag to indicate that we have already released the data and there is no reason
    // to do so again. This avoids the assert a few lines below when the renderer is
    // shutdown...
    if (m_nShadowPoolSize == 0)
    {
        return;
    }


    ResetAllLights();
    for (uint32 iThread = 0; iThread < 2; ++iThread)
    {
        for (uint32 nRecurseLevel = 0; nRecurseLevel < MAX_REND_RECURSION_LEVELS; ++nRecurseLevel)
        {
            m_vecGIClipVolumes[iThread][nRecurseLevel].clear();
        }
    }

    m_shadowPoolAlloc.SetUse(0);
    stl::free_container(m_shadowPoolAlloc);

    m_blockPack->FreeContainers();

    m_nShadowPoolSize = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
inline uint8 CDeferredShading::AddClipVolume(const IClipVolume* pClipVolume)
{
    uint32 nThreadID = gRenDev->m_RP.m_nFillThreadID;
    // Note: vis area and clip volume code is processed before EF_StartEf() in 3DEngine side - so recurse level still at -1 at beginning
    int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] + 1;

    SClipVolumeData pClipVolumeData;
    pClipVolumeData.nStencilRef = m_nClipVolumesCount[nThreadID][nRecurseLevel] + 1; // the first clip volume ID is reserved for outdoors
    pClipVolumeData.nFlags = pClipVolume->GetClipVolumeFlags();
    pClipVolumeData.mAABB = pClipVolume->GetClipVolumeBBox();
    pClipVolume->GetClipVolumeMesh(pClipVolumeData.m_pRenderMesh, pClipVolumeData.mWorldTM);

    m_pClipVolumes[nThreadID][nRecurseLevel].push_back(pClipVolumeData);
    m_nClipVolumesCount[nThreadID][nRecurseLevel]++;

    return pClipVolumeData.nStencilRef;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CDeferredShading::SetClipVolumeBlendData(const IClipVolume* pClipVolume, const SClipVolumeBlendInfo& blendInfo)
{
    uint32 nThreadID = gRenDev->m_RP.m_nFillThreadID;
    // Note: vis area and clip volume code is processed before EF_StartEf() in 3DEngine side - so recurse level still at -1 at beginning
    int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] + 1;

    size_t nClipVolumeIndex = pClipVolume->GetStencilRef() - 1; // 0 is reserved for outdoor
    assert(m_pClipVolumes[nThreadID][nRecurseLevel].size() > nClipVolumeIndex &&
        m_pClipVolumes[nThreadID][nRecurseLevel][nClipVolumeIndex].nStencilRef == pClipVolume->GetStencilRef());

    SClipVolumeData& clipVolumeData = m_pClipVolumes[nThreadID][nRecurseLevel][nClipVolumeIndex];
    for (int i = 0; i < SClipVolumeBlendInfo::BlendPlaneCount; ++i)
    {
        clipVolumeData.m_BlendData.blendPlanes[i] = Vec4(blendInfo.blendPlanes[i].n, blendInfo.blendPlanes[i].d);
        clipVolumeData.m_BlendData.nBlendIDs[i] = blendInfo.blendVolumes[i] ? blendInfo.blendVolumes[i]->GetStencilRef() : 0;
    }

    clipVolumeData.nFlags |= IClipVolume::eClipVolumeBlend;
    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::ResetClipVolumes()
{
    uint32 nThreadID = gcpRendD3D->m_RP.m_nFillThreadID;
    // Note: vis area and clip volume code is processed before EF_StartEf() in 3DEngine side - so recurse level still at -1 at beginning
    uint32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID] + 1;

    if (nRecurseLevel < MAX_REND_RECURSION_LEVELS)
    {
        m_pClipVolumes[nThreadID][nRecurseLevel].clear();
        m_nClipVolumesCount[nThreadID][nRecurseLevel] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::ResetAllClipVolumes()
{
    for (size_t i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        for (size_t j = 0; j < MAX_REND_RECURSION_LEVELS; ++j)
        {
            m_pClipVolumes[i][j].clear();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CDeferredShading::SpecularAccEnableMRT(bool bEnable)
{
    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        assert(m_pLBufferSpecularRT);

        CD3D9Renderer* const __restrict rd = gcpRendD3D;

        if (bEnable && !m_bSpecularState)
        {
            rd->FX_PushRenderTarget(1, m_pLBufferSpecularRT, NULL, -1, false, 1);
            m_bSpecularState = true;
            return true;
        }
        else if (!bEnable && m_bSpecularState)
        {
            m_pLBufferSpecularRT->SetResolved(true);
            rd->FX_PopRenderTarget(1);
            m_bSpecularState = false;
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::SetupPasses()
{
    AZ_TRACE_METHOD();
    CreateDeferredMaps();

    m_nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
    m_nRecurseLevel = SRendItem::m_RecurseLevel[m_nThreadID];
    assert(m_nRecurseLevel >= 0);

    m_nBindResourceMsaa = gcpRendD3D->m_RP.m_MSAAData.Type ? SResourceView::DefaultViewMS : SResourceView::DefaultView;

    gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK | RT_DEBUGMASK);

    m_pLBufferDiffuseRT = CTexture::s_ptexCurrentSceneDiffuseAccMap;
    m_pLBufferSpecularRT = CTexture::s_ptexSceneSpecularAccMap;
    m_pNormalsRT = CTexture::s_ptexSceneNormalsMap;

    if (FurPasses::GetInstance().IsRenderingFur())
    {
        m_pDepthRT = CTexture::s_ptexFurZTarget;
    }
    else
    {
        m_pDepthRT = CTexture::s_ptexZTarget;
    }

    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        m_pResolvedStencilRT = CTexture::s_ptexGmemStenLinDepth;
        m_pDepthRT = CTexture::s_ptexGmemStenLinDepth;
    }
    else
    {
        m_pResolvedStencilRT = CTexture::s_ptexVelocity;
    }
    m_pDiffuseRT = CTexture::s_ptexSceneDiffuse;
    m_pSpecularRT = CTexture::s_ptexSceneSpecular;
    m_pMSAAMaskRT = CTexture::s_ptexBackBuffer;

    m_nTexStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
    m_nTexStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));

    CameraViewParameters* viewParameters = &gcpRendD3D->m_RP.m_TI[m_nThreadID].m_cam.m_viewParameters;
    m_pCamFront = viewParameters->vZ;
    m_pCamFront.Normalize();
    m_pCamPos = viewParameters->vOrigin;

    m_fCamFar = viewParameters->fFar;
    m_fCamNear = viewParameters->fNear;

    m_fRatioWidth = (float)m_pLBufferDiffuseRT->GetWidth() / (float)CTexture::s_ptexSceneTarget->GetWidth();
    m_fRatioHeight = (float)m_pLBufferDiffuseRT->GetHeight() / (float)CTexture::s_ptexSceneTarget->GetHeight();

    m_pView = gcpRendD3D->m_CameraMatrix;
    m_pView.Transpose();

    m_mViewProj = gcpRendD3D->m_ViewProjMatrix;
    m_mViewProj.Transpose();

    m_pViewProjI = gcpRendD3D->m_ViewProjMatrix.GetInverted();

    gRenDev->m_cEF.mfRefreshSystemShader("DeferredShading", CShaderMan::s_shDeferredShading);
    m_pShader = CShaderMan::s_shDeferredShading;

    gcpRendD3D->SetCullMode(R_CULL_BACK);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest <= 1)
    {
        m_nRenderState |= GS_NODEPTHTEST;
    }
    else
    {
        m_nRenderState &= ~GS_NODEPTHTEST;
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDebug == 2)
    {
        gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_OVERDRAW_DEBUG;
    }

    m_nCurTargetWidth = m_pLBufferDiffuseRT->GetWidth();
    m_nCurTargetHeight = m_pLBufferDiffuseRT->GetHeight();

    // Verify if sun present in non-deferred light list (usually fairly small list)
    gcpRendD3D->m_RP.m_pSunLight = NULL;
    for (uint32 i = 0; i < gcpRendD3D->m_RP.m_DLights[m_nThreadID][m_nRecurseLevel].Num(); i++)
    {
        SRenderLight* dl = &gcpRendD3D->m_RP.m_DLights[m_nThreadID][m_nRecurseLevel][i];
        if (dl->m_Flags & DLF_SUN)
        {
            gcpRendD3D->m_RP.m_pSunLight = dl;
            break;
        }
    }

    SetupGlobalConsts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::SetupGlobalConsts()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    //set world basis
    float maskRTWidth = float(m_nCurTargetWidth);
    float maskRTHeight = float(m_nCurTargetHeight);
    Vec4r vWBasisX, vWBasisY, vWBasisZ, vCamPos;
    Vec4 vParamValue, vMag;
    CShadowUtils::ProjectScreenToWorldExpansionBasis(rd->m_IdentityMatrix, rd->GetCamera(), Vec2(rd->m_TemporalJitterClipSpace.x, rd->m_TemporalJitterClipSpace.y), maskRTWidth, maskRTHeight, vWBasisX, vWBasisY, vWBasisZ, vCamPos, true, NULL);

    vWorldBasisX = vWBasisX / rd->m_RP.m_CurDownscaleFactor.x;
    vWorldBasisY = vWBasisY / rd->m_RP.m_CurDownscaleFactor.y;
    vWorldBasisZ = vWBasisZ;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::FilterGBuffer()
{
    if (!CRenderer::CV_r_DeferredShadingFilterGBuffer)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DDEFERREDSHADING_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DDeferredShading_cpp)
#endif
        return;
    }

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    PROFILE_LABEL_SCOPE("GBUFFER_FILTER");

    static CCryNameTSCRC tech("FilterGBuffer");

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DDEFERREDSHADING_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DDeferredShading_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    PostProcessUtils().StretchRect(CTexture::s_ptexSceneSpecular, CTexture::s_ptexStereoR);
    CTexture* pSceneSpecular = CTexture::s_ptexStereoR;
#endif

    if (CRenderer::CV_r_SlimGBuffer)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    rd->FX_PushRenderTarget(0, CTexture::s_ptexSceneSpecular, NULL);
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, tech, FEF_DONTSETSTATES);
    rd->FX_SetState(GS_NODEPTHTEST);
    SPostEffectsUtils::SetTexture(CTexture::s_ptexSceneNormalsMap, 0, FILTER_POINT, 0);
    SPostEffectsUtils::SetTexture(pSceneSpecular, 1, FILTER_POINT, 0);
    SPostEffectsUtils::SetTexture(m_pDepthRT, 2, FILTER_POINT, 0);

    // Bind sampler directly so that it works with DX11 style texture objects
    ID3D11SamplerState* pSamplers[1] = { (ID3D11SamplerState*)CTexture::s_TexStates[m_nTexStatePoint].m_pDeviceState };
    rd->m_DevMan.BindSampler(eHWSC_Pixel, pSamplers, 15, 1);

    SD3DPostEffectsUtils::DrawFullScreenTri(CTexture::s_ptexSceneSpecular->GetWidth(), CTexture::s_ptexSceneSpecular->GetHeight());
    SD3DPostEffectsUtils::ShEndPass();

    ID3D11SamplerState* pSampNull[1] = { NULL };
    rd->m_DevMan.BindSampler(eHWSC_Pixel, pSampNull, 15, 1);
    rd->FX_Commit();

    rd->FX_PopRenderTarget(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeferredShading::DrawLightVolume(EShapeMeshType meshType, const Matrix44& mUnitVolumeToWorld, const Vec4& vSphereAdjust)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    float maskRTWidth = (float) m_nCurTargetWidth;
    float maskRTHeight = (float) m_nCurTargetHeight;

    Vec4 vScreenScale(1.0f / maskRTWidth, 1.0f / maskRTHeight,
        0.0f, 0.0f);

    {
        static CCryNameR paramName("g_ScreenScale");
        m_pShader->FXSetPSFloat(paramName, &vScreenScale, 1);
    }

    {
        static CCryNameR paramName("vWBasisX");
        m_pShader->FXSetPSFloat(paramName, &vWorldBasisX, 1);
    }

    {
        static CCryNameR paramName("vWBasisY");
        m_pShader->FXSetPSFloat(paramName, &vWorldBasisY, 1);
    }

    {
        static CCryNameR paramName("vWBasisZ");
        m_pShader->FXSetPSFloat(paramName, &vWorldBasisZ, 1);
    }

    {
        static CCryNameR paramNameVolumeToWorld("g_mUnitLightVolumeToWorld");
        m_pShader->FXSetVSFloat(paramNameVolumeToWorld, (Vec4*) mUnitVolumeToWorld.GetData(), 4);
    }

    {
        static CCryNameR paramNameSphereAdjust("g_vLightVolumeSphereAdjust");
        m_pShader->FXSetVSFloat(paramNameSphereAdjust, &vSphereAdjust, 1);
    }

    {
        static CCryNameR paramName("g_mViewProj");
        Matrix44 mViewProjMatrix =  rd->m_ViewMatrix * rd->m_ProjMatrix;
        m_pShader->FXSetVSFloat(paramName, (Vec4*) mViewProjMatrix.GetData(), 4);
    }

    // Vertex/index buffer
    rd->FX_SetVStream(0, rd->m_pUnitFrustumVB[meshType], 0, sizeof(SVF_P3F_C4B_T2F));
    rd->FX_SetIStream(rd->m_pUnitFrustumIB[meshType], 0, (rd->kUnitObjectIndexSizeof == 2 ? Index16 : Index32));

    rd->D3DSetCull(eCULL_Back);
    if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        rd->FX_Commit(false);
        rd->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, rd->m_UnitFrustVBSize[meshType], 0, rd->m_UnitFrustIBSize[meshType]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DrawDecalVolume([[maybe_unused]] const SDeferredDecal& rDecal, Matrix44A& mDecalLightProj, ECull volumeCull)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    float maskRTWidth = float(m_nCurTargetWidth);
    float maskRTHeight = float(m_nCurTargetHeight);

    Vec4 vScreenScale(1.0f / maskRTWidth, 1.0f / maskRTHeight,
        0.0f, 0.0f);

    {
        static CCryNameR paramName("g_ScreenScale");
        m_pShader->FXSetPSFloat(paramName, &vScreenScale, 1);
    }

    {
        static CCryNameR paramName("vWBasisX");
        m_pShader->FXSetPSFloat(paramName, &vWorldBasisX, 1);
    }

    {
        static CCryNameR paramName("vWBasisY");
        m_pShader->FXSetPSFloat(paramName, &vWorldBasisY, 1);
    }

    {
        static CCryNameR paramName("vWBasisZ");
        m_pShader->FXSetPSFloat(paramName, &vWorldBasisZ, 1);
    }

    //////////////// light sphere processing /////////////////////////////////
    {
        Matrix44 mInvDecalLightProj = mDecalLightProj.GetInverted();
        static CCryNameR paramName("g_mInvLightProj");
        m_pShader->FXSetVSFloat(paramName, alias_cast<Vec4*>(&mInvDecalLightProj), 4);
    }

    {
        static CCryNameR paramName("g_mViewProj");
        m_pShader->FXSetVSFloat(paramName, alias_cast<Vec4*>(&m_mViewProj), 4);
    }

    rd->FX_SetVStream(0, rd->m_pUnitFrustumVB[SHAPE_BOX], 0, sizeof(SVF_P3F_C4B_T2F));
    rd->FX_SetIStream(rd->m_pUnitFrustumIB[SHAPE_BOX], 0, (rd->kUnitObjectIndexSizeof == 2 ? Index16 : Index32));

    rd->D3DSetCull(volumeCull);
    if (!FAILED(rd->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        rd->FX_Commit(false);
        rd->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, rd->m_UnitFrustVBSize[SHAPE_BOX], 0, rd->m_UnitFrustIBSize[SHAPE_BOX]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// Calculates matrix that projects WS position into decal volume for texture coordinates
Matrix44A GetDecalLightProjMatrix(const SDeferredDecal& rDecal)
{
    static const float fZNear = -0.3f;
    static const float fZFar = 0.5f;

    static const Matrix44A mTextureAndDepth(
        0.5f, 0.0f, 0.0f, 0.5f,
        0.0f, -0.5f, 0.0f, 0.5f,
        0.0f, 0.0f, 1.0f / (fZNear - fZFar), fZNear / (fZNear - fZFar),
        0.0f, 0.0f, 0.0f, 1.0f);

    // transform world coords to decal texture coords
    Matrix44A mDecalLightProj = mTextureAndDepth * rDecal.projMatrix.GetInverted();
    return mDecalLightProj;
}

// Calculates tangent space to world matrix
Matrix44A CalculateTSMatrix(const Vec3 vBasisX, const Vec3 vBasisY, const Vec3 vBasisZ)
{
    const Vec3 vNormX = vBasisX.GetNormalized();
    const Vec3 vNormY = vBasisY.GetNormalized();
    const Vec3 vNormZ = vBasisZ.GetNormalized();

    // decal normal map to world transform
    Matrix44A mDecalTS(
        vNormX.x, vNormX.y, vNormX.z, 0,
        -vNormY.x, -vNormY.y, -vNormY.z, 0,
        vNormZ.x, vNormZ.y, vNormZ.z, 0,
        0, 0, 0, 1);

    return mDecalTS;
}

// Shared function to get dynamic parameters for deferred decals
void GetDynamicDecalParams(AZStd::vector<SShaderParam>& shaderParams, float& decalAlphaMult, float& decalFalloff, float& decalDiffuseOpacity, float& emittanceMapGamma)
{
    decalAlphaMult = 1.0f;
    decalFalloff = 1.0f;
    emittanceMapGamma = 1.0f;
    decalDiffuseOpacity = 1.0f;

    for (uint32 i = 0, si = shaderParams.size(); i < si; ++i)
    {
        const char* name = shaderParams[i].m_Name.c_str();
        if (azstricmp(name, "DecalAlphaMult") == 0)
        {
            decalAlphaMult = shaderParams[i].m_Value.m_Float;
        }
        else if (azstricmp(name, "DecalFalloff") == 0)
        {
            decalFalloff = shaderParams[i].m_Value.m_Float;
        }
        else if (azstricmp(name, "EmittanceMapGamma") == 0)
        {
            emittanceMapGamma = shaderParams[i].m_Value.m_Float;
        }
        else if (azstricmp(name, "DecalDiffuseOpacity") == 0)
        {
            decalDiffuseOpacity = shaderParams[i].m_Value.m_Float;
        }
    }
}

bool CDeferredShading::DeferredDecalPass(const SDeferredDecal& rDecal, uint32 indDecal)
{
    // __________________________________________________________________________________________
    // Early out if no emissive material

    _smart_ptr<IMaterial> pDecalMaterial = rDecal.pMaterial;
    if (pDecalMaterial == NULL)
    {
        AZ_WarningOnce("CDeferredShading", pDecalMaterial == NULL, "Decal missing material.");
        return false;
    }

    SShaderItem& sItem = pDecalMaterial->GetShaderItem(0);
    if (sItem.m_pShaderResources == NULL)
    {
        assert(0);
        return false;
    }

    // __________________________________________________________________________________________
    // Begin

    PROFILE_FRAME(CDeferredShading_DecalPass);
    PROFILE_SHADER_SCOPE;

    gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK | g_HWSR_MaskBit[HWSR_SAMPLE4]);

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    rd->m_RP.m_nDeferredPrimitiveID = SHAPE_PROJECTOR;

    bool bStencilMask = false;
    bool bUseLightVolumes = true;

    rd->EF_Scissor(false, 0, 0, 1, 1);
    rd->SetDepthBoundTest(0.0f, 1.0f, false); // stencil pre-passes are rop bound, using depth bounds increases even more rop cost

    // coord systems conversion (from orientation to shader matrix)
    const Vec3 vBasisX = rDecal.projMatrix.GetColumn0();
    const Vec3 vBasisY = rDecal.projMatrix.GetColumn1();
    const Vec3 vBasisZ = rDecal.projMatrix.GetColumn2();

    // __________________________________________________________________________________________
    // Textures

    CTexture* pCurTarget = CTexture::s_ptexSceneNormalsMap;
    m_nCurTargetWidth = pCurTarget->GetWidth();
    m_nCurTargetHeight = pCurTarget->GetHeight();

    const float decalSize = max(vBasisX.GetLength() * 2.0f, vBasisY.GetLength() * 2.0f);

    // We will use mipLevelFactor from diffuse texture for other textures
    float mipLevelFactor = 0.0;

    ITexture* pDiffuseTex = SetTexture(sItem, EFTT_DIFFUSE, 2, rDecal.rectTexture, decalSize, mipLevelFactor, ESetTexture_Transform | ESetTexture_bSRGBLookup);
    assert(pDiffuseTex != nullptr);

    int setTextureFlags = ESetTexture_HWSR | ESetTexture_AllowDefault | ESetTexture_MipFactorProvided;
    SetTexture(sItem, EFTT_NORMALS,    3, rDecal.rectTexture, decalSize, mipLevelFactor, setTextureFlags);
    SetTexture(sItem, EFTT_SMOOTHNESS, 4, rDecal.rectTexture, decalSize, mipLevelFactor, setTextureFlags);
    SetTexture(sItem, EFTT_OPACITY,    5, rDecal.rectTexture, decalSize, mipLevelFactor, setTextureFlags);

    SD3DPostEffectsUtils::SetTexture((CTexture*)CTexture::s_ptexBackBuffer, 6, FILTER_POINT, 0);   //contains copy of normals buffer

    // Need to set the depth texture if is not available as a RT
    const bool needDepthTexture = !rd->FX_GetEnabledGmemPath(nullptr) || rd->FX_GmemGetDepthStencilMode() == CD3D9Renderer::eGDSM_Texture;
    if (needDepthTexture)
    {
        m_pDepthRT->Apply(0, m_nTexStatePoint);
    }

    // __________________________________________________________________________________________
    // Stencil

    rd->m_RP.m_PersFlags2 |= RBPF2_READMASK_RESERVED_STENCIL_BIT;

    //apply stencil dynamic masking
    rd->FX_SetStencilState(
        STENC_FUNC(FSS_STENCFUNC_EQUAL) |
        STENCOP_FAIL(FSS_STENCOP_KEEP) |
        STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
        STENCOP_PASS(FSS_STENCOP_KEEP),
        BIT_STENCIL_RESERVED, BIT_STENCIL_RESERVED, 0xFFFFFFFF
    );

    rd->m_RP.m_PersFlags2 &= ~RBPF2_READMASK_RESERVED_STENCIL_BIT;

    if (bStencilMask)
    {
        rd->FX_StencilTestCurRef(true, false);
    }

    // __________________________________________________________________________________________
    // Shader technique

    if (rDecal.fGrowAlphaRef > 0.0f)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_deferredDecalsDebug == 1)
    {
        rd->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0]; // disable alpha grow feature
        rd->m_RP.m_FlagsShader_RT |=  g_HWSR_MaskBit[HWSR_SAMPLE2]; // debug output
        rd->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE3]; // disable normals
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingLBuffersFmt == 2)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION];
    }
    
    if (bUseLightVolumes)
    {
        //enable light volumes rendering
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
        static CCryNameTSCRC techName("DeferredDecalVolume");
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    }
    else
    {
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pDeferredDecalTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    }

    // __________________________________________________________________________________________
    // Shader Params

    // Texture transforms
    m_pShader->FXSetPSFloat(m_pParamTexTransforms, m_vTextureTransforms[0], 2 * EMaxTextureSlots);

    // decal normal map to world transform
    Matrix44A mDecalTS = CalculateTSMatrix(vBasisX, vBasisY, vBasisZ);
    m_pShader->FXSetPSFloat(m_pParamDecalTS, (Vec4*)mDecalTS.GetData(), 4);

    // transform world coords to decal texture coords
    Matrix44A mDecalLightProj = GetDecalLightProjMatrix(rDecal);
    m_pShader->FXSetPSFloat(m_pParamLightProjMatrix, (Vec4*)mDecalLightProj.GetData(), 4);

    // Diffuse
    Vec4 vDiff = sItem.m_pShaderResources->GetColorValue(EFTT_DIFFUSE).toVec4();
    vDiff.w = sItem.m_pShaderResources->GetStrengthValue(EFTT_OPACITY) * rDecal.fAlpha;
    m_pShader->FXSetPSFloat(m_pParamDecalDiffuse, &vDiff, 1);

    // Angle Attenuation
    Vec4 angleAttenuation = Vec4(rDecal.angleAttenuation,0,0,0);
    m_pShader->FXSetPSFloat(m_pParamDecalAngleAttenuation, &angleAttenuation, 1);
    
    // Specular
    Vec4 vSpec = sItem.m_pShaderResources->GetColorValue(EFTT_SPECULAR).toVec4();
    vSpec.w = sItem.m_pShaderResources->GetStrengthValue(EFTT_SMOOTHNESS);
    m_pShader->FXSetPSFloat(m_pParamDecalSpecular, &vSpec, 1);

    // Dynamic params
    float decalAlphaMult, decalFalloff, decalDiffuseOpacity, emittanceMapGamma;
    auto& shaderParams = sItem.m_pShaderResources->GetParameters();
    GetDynamicDecalParams(shaderParams, decalAlphaMult, decalFalloff, decalDiffuseOpacity, emittanceMapGamma);

    float fGrowAlphaRef = rDecal.fGrowAlphaRef;

    // Debug shader params
    if (pDiffuseTex && CRenderer::CV_r_deferredDecalsDebug == 1)
    {
        indDecal = pDiffuseTex->GetTextureID() % 3;

        decalAlphaMult      = indDecal == 0 ? 1.0f : 0.0;
        decalFalloff        = indDecal == 1 ? 1.0f : 0.0;
        decalDiffuseOpacity = indDecal == 2 ? 1.0f : 0.0;
        fGrowAlphaRef       = 0.94f;   // Crytek magic value
    }

    Vec4 decalParams(decalAlphaMult, decalFalloff, decalDiffuseOpacity, rDecal.fGrowAlphaRef);
    m_pShader->FXSetPSFloat(m_pGeneralParams, &decalParams, 1);


    // __________________________________________________________________________________________
    // State

    int nStates = m_nRenderState;

    int disableFlags = GS_BLEND_MASK | GS_COLMASK_NONE | GS_NODEPTHTEST | GS_DEPTHFUNC_MASK;
    int enableFlags = GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_DEPTHFUNC_LEQUAL | GS_COLMASK_RGB | GS_STENCIL;

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_deferredDecalsDebug == 2)
    {
        enableFlags |= GS_DEPTHWRITE | GS_WIREFRAME;
    }

    nStates &= ~disableFlags;
    nStates |= enableFlags;

    // __________________________________________________________________________________________
    // Culling

    ECull volumeCull = eCULL_Back;

    rd->EF_Scissor(false, 0, 0, 1, 1);

    const float r = fabs(vBasisX.dot(m_pCamFront)) + fabs(vBasisY.dot(m_pCamFront)) + fabs(vBasisZ.dot(m_pCamFront));
    const float s = m_pCamFront.dot(rDecal.projMatrix.GetTranslation() - m_pCamPos);
    if (fabs(s) - m_fCamNear <= r) // OBB-Plane via separating axis test, to check if camera near plane intersects decal volume
    {
        nStates &= ~(GS_NODEPTHTEST | GS_DEPTHFUNC_MASK);
        nStates |= GS_DEPTHFUNC_GREAT;
        volumeCull = eCULL_Front;
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_deferredDecalsDebug == 2)
    {
        volumeCull = eCULL_Back;
    }

    // __________________________________________________________________________________________
    // Render

    rd->FX_SetState(nStates);

    if (bUseLightVolumes)
    {
        DrawDecalVolume(rDecal, mDecalLightProj, volumeCull);
    }
    else
    {
        SD3DPostEffectsUtils::DrawFullScreenTriWPOS(m_nCurTargetWidth, m_nCurTargetHeight);
    }

    SD3DPostEffectsUtils::ShEndPass();

    if (bStencilMask)
    {
        rd->FX_StencilTestCurRef(false);
    }

    return true;
}

// This renders the emissive part of a single deferred decal.
// Only the emissive part of the light is output as the rest of the lighting has been calculated in the deferred resolve.
// Blends using SRC_ONE and DST_ONE
// Called by CD3D9Renderer::FX_DeferredDecalsEmissive
// Uses pixel shader DecalEmissivePassPS in DeferredShading.cfx
void CDeferredShading::DeferredDecalEmissivePass(const SDeferredDecal& rDecal, [[maybe_unused]] uint32 indDecal)
{
    // __________________________________________________________________________________________
    // Early out if no emissive material

    _smart_ptr<IMaterial> pDecalMaterial = rDecal.pMaterial;
    if (pDecalMaterial == NULL)
    {
        AZ_WarningOnce("CDeferredShading", pDecalMaterial == NULL, "Decal missing material.");
        return;
    }

    SShaderItem& sItem = pDecalMaterial->GetShaderItem(0);
    if (sItem.m_pShaderResources == NULL)
    {
        assert(0);
        return;
    }

    if (!sItem.m_pShaderResources->IsEmissive())
    {
        return;
    }

    // __________________________________________________________________________________________
    // Begin

    PROFILE_FRAME(CDeferredShading_DecalEmissivePass);
    PROFILE_SHADER_SCOPE;

    gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK | g_HWSR_MaskBit[HWSR_SAMPLE4]);

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    rd->m_RP.m_nDeferredPrimitiveID = SHAPE_PROJECTOR;

    bool bUseLightVolumes = true;

    rd->EF_Scissor(false, 0, 0, 1, 1);
    rd->SetDepthBoundTest(0.0f, 1.0f, false);

    // coord systems conversion (from orientation to shader matrix)
    const Vec3 vBasisX = rDecal.projMatrix.GetColumn0();
    const Vec3 vBasisY = rDecal.projMatrix.GetColumn1();
    const Vec3 vBasisZ = rDecal.projMatrix.GetColumn2();

    // __________________________________________________________________________________________
    // Textures

    CTexture* pCurTarget = CTexture::s_ptexHDRTarget;
    m_nCurTargetWidth = pCurTarget->GetWidth();
    m_nCurTargetHeight = pCurTarget->GetHeight();

    // Particles use the $Detail slot for emittance
    EEfResTextures emittanceTextureIdx = EFTT_EMITTANCE;
    const char* shaderName = sItem.m_pShader->GetName();
    if (strcmp(shaderName, "Particles") == 0)
        emittanceTextureIdx = EFTT_DETAIL_OVERLAY;

    // Each texture will calculate it's own mip level factor
    float dummyMipLevelFactor = 0;
    const float decalSize = max(vBasisX.GetLength() * 2.0f, vBasisY.GetLength() * 2.0f);

    int setTextureFlags = ESetTexture_HWSR | ESetTexture_AllowDefault | ESetTexture_Transform;
    SetTexture(sItem, emittanceTextureIdx,  3, rDecal.rectTexture, decalSize, dummyMipLevelFactor, setTextureFlags);
    SetTexture(sItem, EFTT_DECAL_OVERLAY,   4, rDecal.rectTexture, decalSize, dummyMipLevelFactor, setTextureFlags);
    SetTexture(sItem, EFTT_OPACITY,         5, rDecal.rectTexture, decalSize, dummyMipLevelFactor, setTextureFlags);

    SD3DPostEffectsUtils::SetTexture((CTexture*)CTexture::s_ptexZTarget,    0, FILTER_POINT, 0);    // depth
    SD3DPostEffectsUtils::SetTexture((CTexture*)CTexture::s_ptexBackBuffer, 6, FILTER_POINT, 0);    // copy of normals

    // Need to set the depth texture if is not available as a RT
    const bool needDepthTexture = !rd->FX_GetEnabledGmemPath(nullptr) || rd->FX_GmemGetDepthStencilMode() == CD3D9Renderer::eGDSM_Texture;
    if (needDepthTexture)
    {
        m_pDepthRT->Apply(0, m_nTexStatePoint);
    }

    // __________________________________________________________________________________________
    // Shader technique

    if (rDecal.fGrowAlphaRef > 0.0f)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    if (bUseLightVolumes)
    {
        //enable light volumes rendering
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
        static CCryNameTSCRC techName("DeferredDecalEmissiveVolume");
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    }
    else
    {
        static CCryNameTSCRC techName("DeferredDecalEmissive");
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingLBuffersFmt == 2)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION];
    }
    // __________________________________________________________________________________________
    // Shader Params

    // Dynamic Params
    float decalAlphaMult, decalFalloff, decalDiffuseOpacity, emittanceMapGamma;
    auto& shaderParams = sItem.m_pShaderResources->GetParameters();
    GetDynamicDecalParams(shaderParams, decalAlphaMult, decalFalloff, decalDiffuseOpacity, emittanceMapGamma);

    Vec4 decalParams(decalAlphaMult, decalFalloff, emittanceMapGamma, rDecal.fGrowAlphaRef);
    m_pShader->FXSetPSFloat(m_pGeneralParams, &decalParams, 1);

    // Texture transforms
    m_pShader->FXSetPSFloat(m_pParamTexTransforms, m_vTextureTransforms[0], 2 * EMaxTextureSlots);

    // transform world coords to decal texture coords
    Matrix44A mDecalLightProj = GetDecalLightProjMatrix(rDecal);
    m_pShader->FXSetPSFloat(m_pParamLightProjMatrix, (Vec4*)mDecalLightProj.GetData(), 4);

    // decal normal map to world transform
    Matrix44A mDecalTS = CalculateTSMatrix(vBasisX, vBasisY, vBasisZ);
    m_pShader->FXSetPSFloat(m_pParamDecalTS, (Vec4*)mDecalTS.GetData(), 4);

    // Emissive color + intensity
    Vec4 vEmissive = sItem.m_pShaderResources->GetColorValue(EFTT_EMITTANCE).toVec4();
    vEmissive.w = sItem.m_pShaderResources->GetStrengthValue(EFTT_EMITTANCE);
    m_pShader->FXSetPSFloat(m_pParamDecalEmissive, &vEmissive, 1);

    // __________________________________________________________________________________________
    // State

    int nStates = m_nRenderState;

    int disableFlags = GS_NODEPTHTEST | GS_STENCIL | GS_DEPTHFUNC_MASK | GS_BLEND_MASK | GS_COLMASK_NONE;
    int enableFlags = GS_DEPTHFUNC_LEQUAL | GS_COLMASK_RGB | GS_BLSRC_ONE | GS_BLDST_ONE;

    nStates &= ~disableFlags;
    nStates |= enableFlags;

    // __________________________________________________________________________________________
    // Culling

    ECull volumeCull = eCULL_Back;

    rd->EF_Scissor(false, 0, 0, 1, 1);

    const float r = fabs(vBasisX.dot(m_pCamFront)) + fabs(vBasisY.dot(m_pCamFront)) + fabs(vBasisZ.dot(m_pCamFront));
    const float s = m_pCamFront.dot(rDecal.projMatrix.GetTranslation() - m_pCamPos);
    if (fabs(s) - m_fCamNear <= r) // OBB-Plane via separating axis test, to check if camera near plane intersects decal volume
    {
        nStates &= ~(GS_NODEPTHTEST | GS_DEPTHFUNC_MASK);
        nStates |= GS_DEPTHFUNC_GREAT;
        volumeCull = eCULL_Front;
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_deferredDecalsDebug == 2)
    {
        volumeCull = eCULL_Back;
    }

    // __________________________________________________________________________________________
    // Render

    rd->FX_SetState(nStates);

    if (bUseLightVolumes)
    {
        DrawDecalVolume(rDecal, mDecalLightProj, volumeCull);
    }
    else
    {
        SD3DPostEffectsUtils::DrawFullScreenTriWPOS(m_nCurTargetWidth, m_nCurTargetHeight);
    }

    SD3DPostEffectsUtils::ShEndPass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::GetLightRenderSettings(const SRenderLight* const __restrict pDL, bool& bStencilMask, bool& bUseLightVolumes, EShapeMeshType& meshType)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

    const bool bAreaLight = (pDL->m_Flags & DLF_AREA_LIGHT) && pDL->m_fAreaWidth && pDL->m_fAreaHeight && pDL->m_fLightFrustumAngle;

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_deferredshadingLightVolumes)
    {
        if (bAreaLight)
        {
            // area lights use non-uniform box volume
            // need to do more complex box intersection test
            float fExpensionRadius = pDL->m_fRadius * 1.08f;
            Vec3 vScale(fExpensionRadius, fExpensionRadius, fExpensionRadius);

            Matrix34 mObjInv = CShadowUtils::GetAreaLightMatrix(pDL, vScale);
            mObjInv.Invert();

            // check if volumes bounding box intersects the near clipping plane
            const Plane* pNearPlane(rd->GetCamera().GetFrustumPlane(FR_PLANE_NEAR));
            Vec3 pntOnNearPlane(rd->GetCamera().GetPosition() - pNearPlane->DistFromPlane(rd->GetCamera().GetPosition()) * pNearPlane->n);
            Vec3 pntOnNearPlaneOS(mObjInv.TransformPoint(pntOnNearPlane));

            Vec3 nearPlaneOS_n(mObjInv.TransformVector(pNearPlane->n));
            f32 nearPlaneOS_d(-nearPlaneOS_n.Dot(pntOnNearPlaneOS));

            // get extreme lengths
            float t(fabsf(nearPlaneOS_n.x) + fabsf(nearPlaneOS_n.y) + fabsf(nearPlaneOS_n.z));

            float t0 = t + nearPlaneOS_d;
            float t1 = -t + nearPlaneOS_d;

            if (t0 * t1 > 0.0f)
            {
                bUseLightVolumes = true;
            }
            else
            {
                bStencilMask = true;
            }
        }
        else
        {
            const float kDLRadius = pDL->m_fRadius;
            const float fSmallLightBias = 0.5f;
            // the light mesh tessellation and near clipping plane require some bias when testing if inside sphere
            // higher bias for low radius lights
            float fSqLightRadius = kDLRadius * max(-0.1f * kDLRadius + 1.5f, 1.22f);
            fSqLightRadius = max(kDLRadius + fSmallLightBias, fSqLightRadius); //always add on a minimum bias, for very small light's sake
            fSqLightRadius *= fSqLightRadius;
            if (fSqLightRadius < pDL->m_Origin.GetSquaredDistance(m_pCamPos))
            {
                bUseLightVolumes = true;
            }
            else
            {
                bStencilMask = true;
            }
        }
    }

    Vec4 pLightRect  = Vec4(pDL->m_sX, pDL->m_sY, pDL->m_sWidth, pDL->m_sHeight);
    Vec4 scaledLightRect = pLightRect * Vec4(
            rRP.m_CurDownscaleFactor.x, rRP.m_CurDownscaleFactor.y,
            rRP.m_CurDownscaleFactor.x, rRP.m_CurDownscaleFactor.y
            );

    float fCurTargetWidth = (float)(m_nCurTargetWidth);
    float fCurTargetHeight = (float)(m_nCurTargetHeight);

    if (!iszero(CRenderer::CV_r_DeferredShadingLightLodRatio))
    {
        if (CRenderer::CV_r_DeferredShadingLightStencilRatio > 0.01f)
        {
            const float fLightLodRatioScale = CRenderer::CV_r_DeferredShadingLightLodRatio;
            float fLightArea = pLightRect.z * pLightRect.w;
            float fScreenArea = fCurTargetHeight * fCurTargetWidth;
            float fLightRatio = fLightLodRatioScale * (fLightArea / fScreenArea);

            const float fDrawVolumeThres = 0.005f;
            if (fLightRatio < fDrawVolumeThres)
            {
                bUseLightVolumes = false;
            }

            if (fLightRatio > 4 * CRenderer::CV_r_DeferredShadingLightStencilRatio)
            {
                meshType = SHAPE_PROJECTOR2;
            }
            else
            if (fLightRatio > 2 * CRenderer::CV_r_DeferredShadingLightStencilRatio)
            {
                meshType = SHAPE_PROJECTOR1;
            }
        }
        else
        {
            const float fLightLodRatioScale = CRenderer::CV_r_DeferredShadingLightLodRatio;
            float fLightArea = pLightRect.z * pLightRect.w;
            float fScreenArea = fCurTargetHeight * fCurTargetWidth;
            float fLightRatio = fLightLodRatioScale * (fLightArea / fScreenArea);

            const float fDrawVolumeThres = 0.005f;
            if (fLightRatio < fDrawVolumeThres)
            {
                bUseLightVolumes = false;
            }
        }
    }
}

void CDeferredShading::LightPass(const SRenderLight* const __restrict pDL, bool bForceStencilDisable /*= false*/)
{
    PROFILE_FRAME(CDeferredShading_LightPass);
    PROFILE_SHADER_SCOPE;

    PROFILE_LABEL(pDL->m_sName);

    PrefetchLine(&pDL->m_Color, 0);
    PrefetchLine(&pDL->m_sWidth, 0);

    // Skip non-ambient area light if support is disabled
    if ((pDL->m_Flags & DLF_AREA_LIGHT) && !(pDL->m_Flags & DLF_AMBIENT) && !CRenderer::CV_r_DeferredShadingAreaLights)
    {
        return;
    }

    gcpRendD3D->m_RP.m_FlagsShader_RT &= ~(RT_LIGHTPASS_RESETMASK | RT_CLIPVOLUME_ID);

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

    CD3D9Renderer::EGmemPath gmemPath = gcpRendD3D->FX_GetEnabledGmemPath(nullptr);
    bool castShadowMaps = (pDL->m_Flags & DLF_CASTSHADOW_MAPS) != 0;
    bool isGmemEnabled = gmemPath != CD3D9Renderer::eGT_REGULAR_PATH;
    const ITexture* pLightTex = pDL->m_pLightImage ? pDL->m_pLightImage : NULL;
    const bool bProj2D = (pDL->m_Flags & DLF_PROJECT) && pLightTex && !(pLightTex->GetFlags() & FT_REPLICATE_TO_ALL_SIDES);
    const bool bAreaLight = (pDL->m_Flags & DLF_AREA_LIGHT) && pDL->m_fAreaWidth && pDL->m_fAreaHeight && pDL->m_fLightFrustumAngle;

    // Store light properties (color/radius, position relative to camera, rect, zbounds)
    Vec4 pLightDiffuse = Vec4(pDL->m_Color.r, pDL->m_Color.g, pDL->m_Color.b, pDL->m_SpecMult);

    float fInvRadius = (pDL->m_fRadius <= 0) ? 1.0f : 1.0f / pDL->m_fRadius;
    Vec4 pLightPosCS = Vec4(pDL->m_Origin - m_pCamPos, fInvRadius);
    Vec4 pDepthBounds = GetLightDepthBounds(pDL, (rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0);

    Vec4 scaledLightRect = Vec4(
            pDL->m_sX * rRP.m_CurDownscaleFactor.x,
            pDL->m_sY * rRP.m_CurDownscaleFactor.y,
            pDL->m_sWidth * rRP.m_CurDownscaleFactor.x,
            pDL->m_sHeight * rRP.m_CurDownscaleFactor.y
            );

    bool bUseLightVolumes = false;
    bool bStencilMask = (CRenderer::CV_r_DeferredShadingStencilPrepass && (bProj2D || bAreaLight)) || CRenderer::CV_r_DebugLightVolumes || (pDL->m_fProjectorNearPlane < 0);
    rRP.m_nDeferredPrimitiveID = SHAPE_PROJECTOR;

    GetLightRenderSettings(pDL, bStencilMask, bUseLightVolumes, rRP.m_nDeferredPrimitiveID);

    //reset stencil mask
    if (bForceStencilDisable)
    {
        bStencilMask = false;
    }

    if (pDL->m_Flags & DLF_AMBIENT)
    {
        rRP.m_FlagsShader_RT |= RT_AMBIENT_LIGHT;
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingAreaLights)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
    }

    float fAttenuationBulbSize = pDL->m_fAttenuationBulbSize;

    if (bAreaLight)
    {
        fAttenuationBulbSize = (pDL->m_fAreaWidth + pDL->m_fAreaHeight) * 0.25;
    }

    // Adjust light intensity so that the intended brightness is reached 1 meter from the light's surface
    if (!(pDL->m_Flags & DLF_AMBIENT))
    {
        fAttenuationBulbSize = max(fAttenuationBulbSize, 0.001f);

        // Solve I * 1 / (1 + d/lightsize)^2 = 1
        float intensityMul = 1.0f + 1.0f / fAttenuationBulbSize;
        intensityMul *= intensityMul;
        pLightDiffuse.x *= intensityMul;
        pLightDiffuse.y *= intensityMul;
        pLightDiffuse.z *= intensityMul;
    }

    // Enable light pass flags
    if ((pDL->m_Flags & DLF_PROJECT))
    {
        assert(!(pDL->GetDiffuseCubemap() && pDL->GetSpecularCubemap()));
        rRP.m_FlagsShader_RT |= RT_TEX_PROJECT;
        if (bProj2D && !bAreaLight)
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ];
        }
    }

    if (bAreaLight)
    {
        rRP.m_FlagsShader_RT |= RT_AREALIGHT;
    }

    uint16 scaledX = (uint16)(scaledLightRect.x);
    uint16 scaledY = (uint16)(scaledLightRect.y);
    uint16 scaledWidth = (uint16)(scaledLightRect.z) + 1;
    uint16 scaledHeight = (uint16)(scaledLightRect.w) + 1;

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingScissor)
    {
        SetupScissors(true, scaledX, scaledY, scaledWidth, scaledHeight);
    }

    if (bStencilMask)
    {
        PROFILE_LABEL_SCOPE("STENCIL_VOLUME");

#if !defined(CRY_USE_METAL) && !defined(ANDROID)
        SpecularAccEnableMRT(false);
#endif

        rd->SetDepthBoundTest(0.0f, 1.0f, false); // stencil pre-passes are rop bound, using depth bounds increases even more rop cost
        rd->FX_StencilFrustumCull(castShadowMaps ? -4 : -1, pDL, NULL, 0);
    }
    else
    if (rd->m_bDeviceSupports_NVDBT && CRenderer::CV_r_DeferredShadingDepthBoundsTest == 1)
    {
        rd->SetDepthBoundTest(pDepthBounds.x, pDepthBounds.z, true);
    }

    // todo: try out on consoles if DBT helps on light pass (on light stencil prepass is actually slower)
    if (rd->m_bDeviceSupports_NVDBT && bStencilMask && CRenderer::CV_r_DeferredShadingDepthBoundsTest && CRenderer::CV_r_deferredshadingDBTstencil)
    {
        rd->SetDepthBoundTest(pDepthBounds.x, pDepthBounds.z, true);
    }

#if !defined(CRY_USE_METAL) && !defined(ANDROID)
    if (bStencilMask)
    {
        SpecularAccEnableMRT(true);
    }
#endif

    uint nNumClipVolumes = m_nClipVolumesCount[m_nThreadID][m_nRecurseLevel];
    if (nNumClipVolumes > 0)
    {
        rRP.m_FlagsShader_RT |= RT_CLIPVOLUME_ID;
    }
    
    // Directional occlusion
    if (CRenderer::CV_r_ssdo)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_APPLY_SSDO];
    }


    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingLBuffersFmt == 2)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION];
    }

    if (CRenderer::CV_r_SlimGBuffer)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }
    
    uint64 currentSample2MaskBit = rRP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_SAMPLE2];
    if (isGmemEnabled)
    {
        // Signal the shader if we support independent blending
        if (RenderCapabilities::SupportsIndependentBlending())
        {
            rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
        }
        else
        {
            rRP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE2];
        }
    }

    if (bUseLightVolumes)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pLightVolumeTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    }
    else
    {
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    }

    int nStates = m_nRenderState;
    nStates &= ~(GS_BLEND_MASK);

    nStates &= ~(GS_NODEPTHTEST | GS_DEPTHFUNC_MASK);           // Ensure zcull used.
    nStates |= GS_DEPTHFUNC_LEQUAL;

    // For PLS we do programmable blending in the fragment shader since we need to write to the PLS struct
    if (!(isGmemEnabled && RenderCapabilities::SupportsPLSExtension()))
    {
        if (!(pDL->m_Flags & DLF_AMBIENT))
        {
            nStates |= (GS_BLSRC_ONE | GS_BLDST_ONE);
        }
        else
        {
            nStates |= (GS_BLSRC_DSTCOL | GS_BLDST_ZERO);
        }

        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDebug == 2)
        {
            nStates &= ~GS_BLEND_MASK;
            nStates |= (GS_BLSRC_ONE | GS_BLDST_ONE);
        }
    }

    rd->FX_SetState(nStates);

    SStateBlend currentBlendState = gcpRendD3D->m_StatesBL[gcpRendD3D->m_nCurStateBL];
    if (CD3D9Renderer::eGT_256bpp_PATH == gmemPath && RenderCapabilities::SupportsIndependentBlending())
    {
        // For GMEM 256 we have 6 RTs so we need to disable blending and writing for all the non lighting RTs
        SStateBlend newBlendState = currentBlendState;
        newBlendState.Desc.IndependentBlendEnable = TRUE;
        for (int i = 0; i < AZ_ARRAY_SIZE(newBlendState.Desc.RenderTarget); ++i)
        {
            newBlendState.Desc.RenderTarget[i].BlendEnable = FALSE;
            newBlendState.Desc.RenderTarget[i].RenderTargetWriteMask = 0;
        }

        // Enable blending for specular and diffuse light buffers
        // Copy all state info from slot 0 since the engine only update that slot when calling FX_State.
        newBlendState.Desc.RenderTarget[CD3D9Renderer::s_gmemRendertargetSlots[gmemPath][CD3D9Renderer::eGT_SpecularLight]] = currentBlendState.Desc.RenderTarget[0];
        newBlendState.Desc.RenderTarget[CD3D9Renderer::s_gmemRendertargetSlots[gmemPath][CD3D9Renderer::eGT_DiffuseLight]] = currentBlendState.Desc.RenderTarget[0];
        gcpRendD3D->SetBlendState(&newBlendState);
    }

    if (bStencilMask)
    {
        rd->FX_StencilTestCurRef(true, false);
    }


    if ((pDL->m_Flags & DLF_PROJECT))
    {
        Matrix44A ProjMatrixT;

        if (bProj2D)
        {
            CShadowUtils::GetProjectiveTexGen(pDL, 0, &ProjMatrixT);
        }
        else
        {
            ProjMatrixT = pDL->m_ProjMatrix;
        }

        // translate into camera space
        ProjMatrixT.Transpose();
        const Vec4 vEye(gRenDev->GetViewParameters().vOrigin, 0.f);
        Vec4 vecTranslation(vEye.Dot((Vec4&)ProjMatrixT.m00), vEye.Dot((Vec4&)ProjMatrixT.m10), vEye.Dot((Vec4&)ProjMatrixT.m20), vEye.Dot((Vec4&)ProjMatrixT.m30));
        ProjMatrixT.m03 += vecTranslation.x;
        ProjMatrixT.m13 += vecTranslation.y;
        ProjMatrixT.m23 += vecTranslation.z;
        ProjMatrixT.m33 += vecTranslation.w;
        m_pShader->FXSetPSFloat(m_pParamLightProjMatrix, (Vec4*) ProjMatrixT.GetData(), 4);
    }

    {
        Vec2 vLightSize = Vec2(pDL->m_fAreaWidth * 0.5f, pDL->m_fAreaHeight * 0.5f);

        float fAreaFov = pDL->m_fLightFrustumAngle * 2.0f;
        if (castShadowMaps && bAreaLight)
        {
            fAreaFov = min(fAreaFov, 135.0f);     // Shadow can only cover ~135 degree FOV without looking bad, so we clamp the FOV to hide shadow clipping.
        }
        const float fCosAngle = cosf(fAreaFov * (gf_PI / 360.0f));     // pre-compute on CPU.

        static CCryNameR arealightMatrixName("g_AreaLightMatrix");
        Matrix44 mAreaLightMatrix;
        mAreaLightMatrix.SetRow4(0, Vec4(pDL->m_ObjMatrix.GetColumn0().GetNormalized(), 1.0f));
        mAreaLightMatrix.SetRow4(1, Vec4(pDL->m_ObjMatrix.GetColumn1().GetNormalized(), 1.0f));
        mAreaLightMatrix.SetRow4(2, Vec4(pDL->m_ObjMatrix.GetColumn2().GetNormalized(), 1.0f));
        mAreaLightMatrix.SetRow4(3, Vec4(vLightSize.x, vLightSize.y, 0, fCosAngle));
        m_pShader->FXSetPSFloat(arealightMatrixName, alias_cast<Vec4*>(&mAreaLightMatrix), 4);
    }

    m_pShader->FXSetPSFloat(m_pParamLightPos, &pLightPosCS, 1);
    m_pShader->FXSetPSFloat(m_pParamLightDiffuse, &pLightDiffuse, 1);


    uint32 stencilID = (pDL->m_nStencilRef[1] + 1) << 16 | pDL->m_nStencilRef[0] + 1;
    Vec4 vAttenParams(fAttenuationBulbSize, *alias_cast<float*>(&stencilID), 0, 0);
    m_pShader->FXSetPSFloat(m_pAttenParams, &vAttenParams, 1);

    // Directional occlusion
    const int ssdoTexSlot = 8;
    SetSSDOParameters(ssdoTexSlot);
    
    if (castShadowMaps)
    {
        static ICVar* pVar = iConsole->GetCVar("e_ShadowsPoolSize");
        int nShadowAtlasRes = pVar->GetIVal();

        const ShadowMapFrustum& firstFrustum = CShadowUtils::GetFirstFrustum(m_nCurLighID);
        //LRad
        float kernelSize = firstFrustum.bOmniDirectionalShadow ? 2.5f : 1.5f;
        const Vec4 vShadowParams(kernelSize * (float(firstFrustum.nTexSize) / nShadowAtlasRes), 0.0f, 0.0f, firstFrustum.fDepthConstBias);
        m_pShader->FXSetPSFloat(m_pGeneralParams, &vShadowParams, 1);

        // set up shadow matrix
        static CCryNameR paramName("g_mLightShadowProj");
        Matrix44A shadowMat = gRenDev->m_TempMatrices[0][0];
        const Vec4 vEye(gRenDev->GetViewParameters().vOrigin, 0.f);
        Vec4 vecTranslation(vEye.Dot((Vec4&)shadowMat.m00), vEye.Dot((Vec4&)shadowMat.m10), vEye.Dot((Vec4&)shadowMat.m20), vEye.Dot((Vec4&)shadowMat.m30));
        shadowMat.m03 += vecTranslation.x;
        shadowMat.m13 += vecTranslation.y;
        shadowMat.m23 += vecTranslation.z;
        shadowMat.m33 += vecTranslation.w;

        // pre-multiply by 1/frustrum_far_plane
        (Vec4&)shadowMat.m20 *= gRenDev->m_cEF.m_TempVecs[2].x;

        //camera matrix
        m_pShader->FXSetPSFloat(paramName, alias_cast<Vec4*>(&shadowMat), 4);
    }

    CTexture* pTexLightImage = (CTexture*)pLightTex;

    if (CD3D9Renderer::eGT_256bpp_PATH != gmemPath)
    {
        // Note: Shadows use slot 3 and slot 7 for shadow map and jitter map
#if defined(ANDROID)
        m_pDepthRT->Apply(0, m_nTexStatePoint, EFTT_UNKNOWN, -2, SResourceView::DefaultView);
#else
        m_pDepthRT->Apply(0, m_nTexStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);
#endif
        m_pNormalsRT->Apply(1, m_nTexStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);
        m_pDiffuseRT->Apply(2, m_nTexStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);
        m_pSpecularRT->Apply(4, m_nTexStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);
    }

    if (!isGmemEnabled)
    {
        m_pMSAAMaskRT->Apply(9, m_nTexStatePoint);

        if (nNumClipVolumes > 0)
        {
            m_pResolvedStencilRT->Apply(11, m_nTexStatePoint);
            m_pShader->FXSetPSFloat(m_pClipVolumeParams, m_vClipVolumeParams, min((uint32)MAX_DEFERRED_CLIP_VOLUMES, nNumClipVolumes + VIS_AREAS_OUTDOOR_STENCIL_OFFSET));
        }
    }

    if ((pDL->m_Flags & DLF_PROJECT) && pTexLightImage)
    {
        SD3DPostEffectsUtils::SetTexture(pTexLightImage, 5, FILTER_TRILINEAR, bProj2D ? 1 : 0);
    }

    if (isGmemEnabled && nNumClipVolumes > 0)
    {
        m_pShader->FXSetPSFloat(m_pClipVolumeParams, m_vClipVolumeParams, min((uint32)MAX_DEFERRED_CLIP_VOLUMES, nNumClipVolumes + VIS_AREAS_OUTDOOR_STENCIL_OFFSET));

        // Global blend weight
        static CCryNameR clipVolGlobalBendWeight("g_fGlobalClipVolumeBlendWeight");
        Vec4 blendWeight(CRenderer::CV_r_GMEMVisAreasBlendWeight, 0, 0, 0);
        m_pShader->FXSetPSFloat(clipVolGlobalBendWeight, &blendWeight, 1);
    }

    if (bUseLightVolumes)
    {
        gcpRendD3D->D3DSetCull(eCULL_Back);

        const Vec3 scale(pDL->m_fRadius * 1.08f);
        Matrix34 mUnitVolumeToWorld = bAreaLight ? CShadowUtils::GetAreaLightMatrix(pDL, scale) : Matrix34::CreateScale(scale, pDL->m_Origin);

        DrawLightVolume(bAreaLight ? SHAPE_BOX : SHAPE_SPHERE, mUnitVolumeToWorld.GetTransposed());
    }
    else
    {
        gcpRendD3D->D3DSetCull(eCULL_Back, true);     //fs quads should not revert test..
        SD3DPostEffectsUtils::DrawFullScreenTriWPOS(m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight(), pDepthBounds.x);
    }

    SD3DPostEffectsUtils::ShEndPass();

    rd->SetDepthBoundTest(0.0f, 1.0f, false);

    if (bStencilMask)
    {
        rd->FX_StencilTestCurRef(false);
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingScissor)
    {
        rd->EF_Scissor(false, 0, 0, 0, 0);
    }

    // Restore blend state
    if (CD3D9Renderer::eGT_256bpp_PATH == gmemPath)
    {
        gcpRendD3D->SetBlendState(&currentBlendState);
        rRP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE2];
        rRP.m_FlagsShader_RT |= currentSample2MaskBit;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeferredShading::RenderClipVolumesToStencil(int nClipAreaReservedStencilBit)
{
    std::vector<SClipVolumeData>& pClipVolumes = m_pClipVolumes[m_nThreadID][m_nRecurseLevel];

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    const bool bRenderVisAreas = CRenderer::CV_r_VisAreaClipLightsPerPixel > 0;

    for (int nCurrVolume = pClipVolumes.size() - 1; nCurrVolume >= 0; --nCurrVolume)
    {
        const SClipVolumeData& pVolumeData = pClipVolumes[nCurrVolume];
        if (pVolumeData.m_pRenderMesh && pVolumeData.nStencilRef < MAX_DEFERRED_CLIP_VOLUMES)
        {
            if ((pVolumeData.nFlags & IClipVolume::eClipVolumeIsVisArea) != 0 && !bRenderVisAreas)
            {
                continue;
            }

            assert(((pVolumeData.nStencilRef + 1) & (BIT_STENCIL_RESERVED | nClipAreaReservedStencilBit)) == 0);
            const int nStencilRef = ~(pVolumeData.nStencilRef + 1) & ~(BIT_STENCIL_RESERVED | nClipAreaReservedStencilBit);

            rd->FX_StencilCullNonConvex(nStencilRef, pVolumeData.m_pRenderMesh.get(), pVolumeData.mWorldTM);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeferredShading::RenderPortalBlendValues(int nClipAreaReservedStencilBit)
{
    std::vector<SClipVolumeData>& pClipVolumes = m_pClipVolumes[m_nThreadID][m_nRecurseLevel];
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    uint nPrevState =  rd->m_RP.m_CurState;
    ECull ePrevCullMode = rd->m_RP.m_eCull;

    uint nNewState = nPrevState;
    nNewState &= ~(GS_COLMASK_NONE | GS_DEPTHWRITE);
    nNewState |= GS_NODEPTHTEST | GS_NOCOLMASK_R | GS_NOCOLMASK_B | GS_NOCOLMASK_A;
    rd->FX_SetState(nNewState);

    static CCryNameTSCRC TechName0 = "PortalBlendVal";
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, TechName0, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    SD3DPostEffectsUtils::SetTexture(m_pDepthRT, 3, FILTER_POINT);

    for (int nCurrVolume = pClipVolumes.size() - 1; nCurrVolume >= 0; --nCurrVolume)
    {
        const SClipVolumeData& pClipVolumeData = pClipVolumes[nCurrVolume];
        if (pClipVolumeData.nStencilRef < MAX_DEFERRED_CLIP_VOLUMES && pClipVolumeData.nFlags & IClipVolume::eClipVolumeBlend)
        {
            const bool bRenderMesh = pClipVolumeData.m_pRenderMesh && CRenderer::CV_r_VisAreaClipLightsPerPixel > 0;

            static CCryNameR blendPlane0Param("BlendPlane0");
            Vec4 plane0ClipSpace = rd->m_ViewProjInverseMatrix * pClipVolumeData.m_BlendData.blendPlanes[0];
            m_pShader->FXSetPSFloat(blendPlane0Param, &plane0ClipSpace, 1);

            static CCryNameR blendPlane1Param("BlendPlane1");
            Vec4 plane1ClipSpace = rd->m_ViewProjInverseMatrix * pClipVolumeData.m_BlendData.blendPlanes[1];
            m_pShader->FXSetPSFloat(blendPlane1Param, &plane1ClipSpace, 1);

            static CCryNameR screenScaleParam("g_ScreenScale");
            Vec4 screenScale = Vec4(1.0f / m_pLBufferDiffuseRT->GetWidth(), 1.0f / m_pLBufferDiffuseRT->GetHeight(), 0, 0);
            m_pShader->FXSetPSFloat(screenScaleParam, &screenScale, 1);

            rd->m_nStencilMaskRef = nClipAreaReservedStencilBit + pClipVolumeData.nStencilRef + 1;
            rd->FX_StencilTestCurRef(true);

            static CCryNameR paramNameVolumeToWorld("g_mUnitLightVolumeToWorld");
            Matrix44 matIdentity(IDENTITY);
            m_pShader->FXSetVSFloat(paramNameVolumeToWorld, (Vec4*) matIdentity.GetData(), 4);

            static CCryNameR paramNameSphereAdjust("g_vLightVolumeSphereAdjust");
            Vec4 vZero(ZERO);
            m_pShader->FXSetVSFloat(paramNameSphereAdjust, &vZero, 1);

            if (bRenderMesh)
            {
                CRenderMesh* pRenderMesh = static_cast<CRenderMesh*>(pClipVolumeData.m_pRenderMesh.get());
                pRenderMesh->CheckUpdate(0);

                const buffer_handle_t hVertexStream = pRenderMesh->GetVBStream(VSF_GENERAL);
                const buffer_handle_t hIndexStream = pRenderMesh->GetIBStream();

                if (hVertexStream != ~0u && hIndexStream != ~0u)
                {
                    size_t nOffsI = 0;
                    size_t nOffsV = 0;

                    D3DBuffer* pVB = gRenDev->m_DevBufMan.GetD3D(hVertexStream, &nOffsV);
                    D3DBuffer* pIB = gRenDev->m_DevBufMan.GetD3D(hIndexStream, &nOffsI);

                    rd->FX_SetVStream(0, pVB, nOffsV, pRenderMesh->GetStreamStride(VSF_GENERAL));
                    rd->FX_SetIStream(pIB, nOffsI, (sizeof(vtx_idx) == 2 ? Index16 : Index32));

                    if (!FAILED(rd->FX_SetVertexDeclaration(0, pRenderMesh->_GetVertexFormat())))
                    {
                        static CCryNameR viewProjParam("g_mViewProj");
                        m_pShader->FXSetVSFloat(viewProjParam, (Vec4*) rd->m_ViewProjMatrix.GetData(), 4);

                        rd->D3DSetCull(eCULL_Front);
                        rd->FX_Commit();
                        rd->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, pRenderMesh->GetNumVerts(), 0, pRenderMesh->GetNumInds());
                    }
                }
            }
            else
            {
                Matrix44 matQuadToClip(IDENTITY);
                matQuadToClip.m00 =  2;
                matQuadToClip.m30 = -1;
                matQuadToClip.m11 = -2;
                matQuadToClip.m31 =  1;

                static CCryNameR viewProjParam("g_mViewProj");
                m_pShader->FXSetVSFloat(viewProjParam, (Vec4*) matQuadToClip.GetData(), 4);

                rd->D3DSetCull(eCULL_Back);
                rd->FX_Commit();
                SD3DPostEffectsUtils::DrawFullScreenTri(m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight());
            }
        }
    }

    SD3DPostEffectsUtils::ShEndPass();

    rd->D3DSetCull(ePrevCullMode);
    rd->FX_SetState(nPrevState);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeferredShading::PrepareClipVolumeData(bool& bOutdoorVisible)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    const bool bMSAA = false;
    const bool isGmemEnabled = gcpRendD3D->FX_GetEnabledGmemPath(nullptr) != CD3D9Renderer::eGT_REGULAR_PATH;
    const CD3D9Renderer::EGmemDepthStencilMode gmemStencilMode = gcpRendD3D->FX_GmemGetDepthStencilMode();

    // Reserved outdoor fragments
    std::vector<SClipVolumeData>& pClipVolumes = m_pClipVolumes[m_nThreadID][m_nRecurseLevel];
    for (uint i = 0; i < VIS_AREAS_OUTDOOR_STENCIL_OFFSET; ++i)
    {
        uint32 nFlags = IClipVolume::eClipVolumeConnectedToOutdoor | IClipVolume::eClipVolumeAffectedBySun;
        m_vClipVolumeParams[i] = Vec4(0, 0, 0, *alias_cast<float*>(&nFlags));
    }

    for (uint i = 0; i < pClipVolumes.size(); ++i)
    {
        const SClipVolumeData& pClipVolumeData = pClipVolumes[i];
        if (pClipVolumeData.nStencilRef + 1 < MAX_DEFERRED_CLIP_VOLUMES)
        {
            uint32 nData = (pClipVolumeData.m_BlendData.nBlendIDs[1] + 1) << 24 | (pClipVolumeData.m_BlendData.nBlendIDs[0] + 1) << 16 | pClipVolumeData.nFlags;
            m_vClipVolumeParams[pClipVolumeData.nStencilRef + 1] = Vec4(0, 0, 0, *alias_cast<float*>(&nData));

            bOutdoorVisible |= pClipVolumeData.nFlags & IClipVolume::eClipVolumeConnectedToOutdoor ? true : false;
        }
    }


    if (CRenderer::CV_r_VolumetricFog != 0)
    {
        if (isGmemEnabled)
        {
            CRY_ASSERT(0); // TODO: implement volumetric fog to work with GMEM
        }
        rd->GetVolumetricFog().ClearVolumeStencil();
    }

    const int nClipVolumeReservedStencilBit = BIT_STENCIL_INSIDE_CLIPVOLUME;

    // Render Clip areas to stencil
    if (!pClipVolumes.empty())
    {
        rd->FX_ResetPipe();

        const uint32 nPersFlags2 = rd->m_RP.m_PersFlags2;
        rd->m_RP.m_PersFlags2 |= RBPF2_WRITEMASK_RESERVED_STENCIL_BIT;

        //ClipVolumes
        {
            PROFILE_LABEL_SCOPE("CLIPVOLUMES TO STENCIL");
            if (!isGmemEnabled)
            {
                if (!RenderCapabilities::SupportsStencilTextures())
                {
                    // Because there's no support for stencil textures we can't resolve the stencil to a texture.
                    // So we draw the ClipVolumes directly to the texture in the "resolve" during the PS.
                    rd->FX_PushRenderTarget(0, m_pResolvedStencilRT, &rd->m_DepthBufferOrigMSAA);
                    const ColorF clearColor((1.0f / 255.0f), 0.0f, 0.0f, 0.0f);
                    rd->EF_ClearTargetsImmediately(FRT_CLEAR_COLOR, clearColor);
                }
                else
                {
                    rd->FX_PushRenderTarget(0, (CTexture*)NULL, &rd->m_DepthBufferOrigMSAA);
                }

                rd->RT_SetViewport(0, 0, m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight());
            }
            RenderClipVolumesToStencil(nClipVolumeReservedStencilBit);
            if (!isGmemEnabled)
            {
                rd->FX_PopRenderTarget(0);
            }
        }

        // Portals blending and volumetric fog are not supported in GMEM path.
        // Use "r_GMEMVisAreasBlendWeight" for global blending between portals.
        if (!isGmemEnabled)
        {
            // Portal blend factors
            static ICVar* pPortalsBlendCVar =  iConsole->GetCVar("e_PortalsBlend");
            if (pPortalsBlendCVar->GetIVal() > 0)
            {
                rd->FX_PushRenderTarget(0, m_pResolvedStencilRT, &rd->m_DepthBufferOrigMSAA);
                rd->RT_SetViewport(0, 0, m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight());
                RenderPortalBlendValues(nClipVolumeReservedStencilBit);
                rd->FX_PopRenderTarget(0);
            }

            if (CRenderer::CV_r_VolumetricFog != 0)
            {
                rd->GetVolumetricFog().RenderClipVolumeToVolumeStencil(nClipVolumeReservedStencilBit);
            }
        }

        rd->m_RP.m_PersFlags2 = nPersFlags2;
    }

    rd->m_nStencilMaskRef = nClipVolumeReservedStencilBit + m_nClipVolumesCount[m_nThreadID][m_nRecurseLevel] + 1;

    if (isGmemEnabled)
    {
        switch (gmemStencilMode)
        {
        case CD3D9Renderer::eGDSM_RenderTarget:
        {
            if (!RenderCapabilities::SupportsPLSExtension())
            {
                PROFILE_LABEL_SCOPE("RESOLVE STENCIL");
                static CCryNameTSCRC pszResolveStencil("ResolveStencil");
                PostProcessUtils().ShBeginPass(CShaderMan::s_shDeferredShading, pszResolveStencil, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
                gcpRendD3D->FX_SetState(GS_NODEPTHTEST | GS_NOCOLMASK_R | GS_NOCOLMASK_B | GS_NOCOLMASK_A);
                GetUtils().DrawQuadFS(CShaderMan::s_shDeferredShading, false, CTexture::s_ptexGmemStenLinDepth->GetWidth(), CTexture::s_ptexGmemStenLinDepth->GetHeight());
                GetUtils().ShEndPass();
            }
            return;
        }
        case CD3D9Renderer::eGDSM_DepthStencilBuffer:
            // We resolve the stencil during the depth linearization 
            return;
        default:
            // Stencil is resolved using the non gmem path.
            break;
        }
    }

    // Need to resolve stencil because light volumes and shadow mask overwrite stencil
    // If there's no support for stencil textures, then we already clipped the stencil volumes straight
    // to the resolved target during the 'CLIPVOLUMES TO STENCIL' pass.
    if (RenderCapabilities::SupportsStencilTextures())
    {
        PROFILE_LABEL_SCOPE("RESOLVE STENCIL");
#if defined(CRY_USE_METAL) || defined(ANDROID)
        bool renderTargetWasPopped = SpecularAccEnableMRT(false);
#endif
        rd->FX_PushRenderTarget(0, m_pResolvedStencilRT, NULL, -1, false, 1);

        const bool isGmemResolve = isGmemEnabled && gmemStencilMode == CD3D9Renderer::eGDSM_Texture;
        // Metal Load/Store Actions
        rd->FX_SetColorDontCareActions(0, isGmemResolve ? false : true, false); // For GMEM we need to preserve the Red channel because it contains the linearized depth.
        rd->FX_SetDepthDontCareActions(0, true, true);
        rd->FX_SetStencilDontCareActions(0, true, true);
        rd->FX_SetColorDontCareActions(1, true, false);
        rd->FX_SetDepthDontCareActions(1, true, true);
        rd->FX_SetStencilDontCareActions(1, true, true);

        // color mask
        static CCryNameTSCRC pszResolveStencil("ResolveStencil");
        PostProcessUtils().ShBeginPass(CShaderMan::s_shDeferredShading, pszResolveStencil, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        int states = GS_NODEPTHTEST | GS_NOCOLMASK_B | GS_NOCOLMASK_A;

        // For Gmem we write into the green channel.
        states |= isGmemResolve ? GS_NOCOLMASK_R : GS_NOCOLMASK_G;
        rd->FX_SetState(states);

        CTexture::SetSamplerState(4, m_nTexStatePoint, eHWSC_Pixel);
        rd->m_DevMan.BindSRV(eHWSC_Pixel, gcpRendD3D->m_pZBufferStencilReadOnlySRV, 4);

        GetUtils().DrawQuadFS(CShaderMan::s_shDeferredShading, false, m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight());
        GetUtils().ShEndPass();
        rd->FX_PopRenderTarget(0);

#if defined(CRY_USE_METAL) || defined(ANDROID)
        // Do not try to re-push a render target if one was not popped above.
        if (renderTargetWasPopped)
        {
            SpecularAccEnableMRT(true);
        }
#endif
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CDeferredShading::AmbientPass(SRenderLight* pGlobalCubemap, bool& bOutdoorVisible)
{
    PROFILE_SHADER_SCOPE;
    PROFILE_FRAME(CDeferredShading_AmbientPass);
    PROFILE_LABEL_SCOPE("AMBIENT_PASS");

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

    rd->m_RP.m_nDeferredPrimitiveID = SHAPE_PROJECTOR;
    rd->D3DSetCull(eCULL_Back, true); //fs quads should not revert test..

    const bool bMSAA = gcpRendD3D->m_RP.m_MSAAData.Type ? true : false;

    const uint64 nFlagsShaderRT = rRP.m_FlagsShader_RT;
    rRP.m_FlagsShader_RT &= ~(RT_LIGHTSMASK);

    SpecularAccEnableMRT(false);
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.0f, DBT_SKY_CULL_DEPTH, false); // Disable depth bounds for ambient lookup.
    }
    const uint32 nNumClipVolumes = m_nClipVolumesCount[m_nThreadID][m_nRecurseLevel];
    if (nNumClipVolumes)
    {
        rRP.m_FlagsShader_RT |= RT_CLIPVOLUME_ID;
    }
    else
    {
        bOutdoorVisible = true;
    }

    // Store global cubemap color
    Vec4 pLightDiffuse;
    if (pGlobalCubemap)
    {
        pLightDiffuse = Vec4(Vec3(pGlobalCubemap->m_Color.r, pGlobalCubemap->m_Color.g, pGlobalCubemap->m_Color.b), pGlobalCubemap->m_SpecMult);

        const float fLuminance = pGlobalCubemap->m_Color.Luminance();
        if (fLuminance > 0.001f)    // too dull => skip
        {
            rRP.m_FlagsShader_RT |= RT_GLOBAL_CUBEMAP;
            // ignore specular if it's too dull
            if (fLuminance * pLightDiffuse.w >= 0.005f)
            {
                rRP.m_FlagsShader_RT |= RT_SPECULAR_CUBEMAP;
            }

            rRP.m_FlagsShader_RT |= (pGlobalCubemap->m_Flags & DLF_IGNORES_VISAREAS) ? RT_GLOBAL_CUBEMAP_IGNORE_VISAREAS : 0;
        }
        else
        {
            pGlobalCubemap = NULL;
        }
    }

    // Patch z-target for all platforms, we need stencil access.
    CTexture* pDepthBufferRT = m_pDepthRT;
    ID3D11DepthStencilView* pZBufferOrigDSV = (ID3D11DepthStencilView*) rd->m_DepthBufferOrigMSAA.pSurf;    // Override depthstencil shader/depthstencil views
    rd->m_DepthBufferOrigMSAA.pSurf = rd->m_pZBufferReadOnlyDSV;
    ID3D11ShaderResourceView* pZTargetOrigSRV = (ID3D11ShaderResourceView*) pDepthBufferRT->GetShaderResourceView(bMSAA ? SResourceView::DefaultViewMS : SResourceView::DefaultView);

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        rd->FX_PushRenderTarget(0, m_pLBufferDiffuseRT, &rd->m_DepthBufferOrigMSAA, -1, false, 1);

        SpecularAccEnableMRT(true);

        // CONFETTI BEGIN: David Srour
        // Metal Load/Store Actions
        // Though the ambient pass doesn't explicitly need to use MTLLoadActionLoad for the color buffer,
        // the following passes after Ambient do use rasterization blending. Only one draw call usually
        // occurs for Ambient pass and many more for the following passes... hence, just set the load/store actions
        // only once here.
        rd->FX_SetDepthDontCareActions(0, false, true);
        rd->FX_SetDepthDontCareActions(1, false, true);

        // The following can only be set if r_DeferredShadingLightVolumes==0 && r_DeferredShadingStencilPrepass == 0, otherwise stencil might need to be written to during light pass.
        if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_deferredshadingLightVolumes &&
            !CRenderer::CV_r_DeferredShadingStencilPrepass)
        {
            rd->FX_SetStencilDontCareActions(0, false, true);
            rd->FX_SetStencilDontCareActions(1, false, true);
        }
        // CONFETTI END
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.0f, DBT_SKY_CULL_DEPTH, true); // Enable depth bounds - discard sky
    }
    Vec3 vE3DParam;
    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_AMBIENT_GROUND_COLOR, vE3DParam);

    const Vec4 cAmbGroundColor = Vec4(vE3DParam, 0);

    Vec4 cAmbHeightParams = Vec4(gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_AMBIENT_MIN_HEIGHT), gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_AMBIENT_MAX_HEIGHT), 0, 0);
    cAmbHeightParams.z = 1.0 / max(0.0001f, cAmbHeightParams.y);
    
    if (pGlobalCubemap && CRenderer::CV_r_ssdo)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_APPLY_SSDO];
    }
        
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pAmbientOutdoorTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    rd->FX_SetState(GS_NODEPTHTEST);

    SD3DPostEffectsUtils::ShSetParamPS(m_pParamAmbient, Vec4(0, 0, 0, 0));
    SD3DPostEffectsUtils::ShSetParamPS(m_pParamAmbientGround, cAmbGroundColor);
    SD3DPostEffectsUtils::ShSetParamPS(m_pParamAmbientHeight, cAmbHeightParams);

    if (nNumClipVolumes)
    {
        m_pShader->FXSetPSFloat(m_pClipVolumeParams, m_vClipVolumeParams, min((uint32)MAX_DEFERRED_CLIP_VOLUMES, nNumClipVolumes + VIS_AREAS_OUTDOOR_STENCIL_OFFSET));

        if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            // Global blend weight
            static CCryNameR clipVolGlobalBendWeight("g_fGlobalClipVolumeBlendWeight");
            Vec4 blendWeight(CRenderer::CV_r_GMEMVisAreasBlendWeight, 0, 0, 0);
            m_pShader->FXSetPSFloat(clipVolGlobalBendWeight, &blendWeight, 1);
        }
    }

    if (pGlobalCubemap)
    {
        CTexture* const texDiffuse = (CTexture*)pGlobalCubemap->GetDiffuseCubemap();
        CTexture* const texSpecular = (CTexture*)pGlobalCubemap->GetSpecularCubemap();
        CTexture* const texNoTextureCM = CTextureManager::Instance()->GetNoTextureCM();

        SD3DPostEffectsUtils::SetTexture(texDiffuse->GetTextureType() < eTT_Cube ? texNoTextureCM : texDiffuse, 1, FILTER_BILINEAR, 1, texDiffuse->IsSRGB());
        SD3DPostEffectsUtils::SetTexture(texSpecular->GetTextureType() < eTT_Cube ? texNoTextureCM : texSpecular, 2, FILTER_TRILINEAR, 1, texSpecular->IsSRGB());

        SD3DPostEffectsUtils::ShSetParamPS(m_pParamLightDiffuse, pLightDiffuse);
        const Vec4 vCubemapParams(IntegerLog2((uint32)texSpecular->GetWidthNonVirtual()) - 2, 0, 0, 0);     // Use 4x4 mip for lowest gloss values
        m_pShader->FXSetPSFloat(m_pGeneralParams, &vCubemapParams, 1);
        
        // Directional occlusion
        const int ssdoTexSlot = 8;
        SetSSDOParameters(ssdoTexSlot);
    }

    if (CD3D9Renderer::eGT_256bpp_PATH != gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        m_pNormalsRT->Apply(0, m_nTexStatePoint, EFTT_UNKNOWN, -1, m_nBindResourceMsaa);
        m_pSpecularRT->Apply(7, m_nTexStatePoint);
        m_pDiffuseRT->Apply(11, m_nTexStatePoint);
    }

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        pDepthBufferRT->SetShaderResourceView(rd->m_pZBufferDepthReadOnlySRV, bMSAA);        // DX11 requires explicitly bind depth then stencil to have access to both depth and stencil read from shader. Formats also must match
#ifndef ANDROID
        pDepthBufferRT->Apply(3, m_nTexStatePoint, EFTT_UNKNOWN, -1, m_nBindResourceMsaa);
        pDepthBufferRT->SetShaderResourceView(rd->m_pZBufferStencilReadOnlySRV, bMSAA);
        pDepthBufferRT->Apply(4, m_nTexStatePoint, EFTT_UNKNOWN, -1, m_nBindResourceMsaa);
#endif // !ANDROID

        m_pMSAAMaskRT->Apply(5, m_nTexStatePoint);
    }

    CTextureManager::Instance()->GetDefaultTexture("EnvironmentBRDF")->Apply(10, m_nTexStateLinear);

    //  this is expected by Mali drivers
    //  this "workaround" was suggested by the Mali team as we were getting incorrect stencil/depth tests behavior due to driver bug
    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr) && (gRenDev->GetFeatures() & RFT_HW_ARM_MALI))
    {
        int nPrevState = rRP.m_CurState;
        int newState = nPrevState;

        newState &= ~(GS_BLEND_MASK | GS_NODEPTHTEST | GS_DEPTHFUNC_MASK | GS_COLMASK_NONE);
        newState |= GS_COLMASK_NONE;
        newState |= GS_DEPTHFUNC_GREAT;
        newState |= GS_DEPTHWRITE;
        rd->FX_SetState(newState);

        rd->FX_PushVP();
        rd->m_NewViewport.nX = 0;
        rd->m_NewViewport.nY = 0;
        rd->m_NewViewport.nWidth = 1;
        rd->m_NewViewport.nHeight = 1;
        rd->m_NewViewport.fMinZ = 1.0f;
        rd->m_NewViewport.fMaxZ = 1.0f;
        rd->m_bViewportDirty = true;

        SD3DPostEffectsUtils::DrawFullScreenTriWPOS(m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight(), 0, &gcpRendD3D->m_FullResRect);

        rd->FX_PopVP();
        rd->FX_SetState(nPrevState);
    }

    SD3DPostEffectsUtils::DrawFullScreenTriWPOS(m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight(), 0, &gcpRendD3D->m_FullResRect);
    SD3DPostEffectsUtils::ShEndPass();

    rd->m_DepthBufferOrigMSAA.pSurf = pZBufferOrigDSV;          // Restore DSV/SRV
    pDepthBufferRT->SetShaderResourceView(pZTargetOrigSRV, bMSAA);

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        rd->FX_PopRenderTarget(0);
    }

    CTexture* const     pBlack = CTextureManager::Instance()->GetBlackTexture();
    if (pBlack)
    {
        pBlack->Apply(3, m_nTexStatePoint);
        pBlack->Apply(4, m_nTexStatePoint);
    }

#if defined(CRY_USE_METAL) || defined(ANDROID)
    //  we don't want to swich RT's too often for metal
    //  We want to keep all light RTs bound regardless of
    //  specular RT usage.
    //  This trick re-enables specular RT
    SpecularAccEnableMRT(false);
    SpecularAccEnableMRT(true);
#endif

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        rd->FX_SetActiveRenderTargets();
    }

    // Follow up with NVidia. Seems driver/pixel quads synchronization? Wrong behavior when reading from native stencil/depth.
    // Luckily can clear stencil since vis areas/decals tag not needed from here on
    if (CRenderer::CV_r_DeferredShadingAmbientSClear)
    {
        rd->EF_ClearTargetsImmediately(FRT_CLEAR_STENCIL, Clr_Unused.r, 1);
    }

    rRP.m_FlagsShader_RT = nFlagsShaderRT;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DeferredCubemaps(const TArray<SRenderLight>& rCubemaps, const uint32 nStartIndex /* = 0 */)
{
    if (nStartIndex < rCubemaps.Num() && CRenderer::CV_r_DeferredShadingEnvProbes)
    {
        // apply deferred cubemaps first
        PROFILE_LABEL_SCOPE("DEFERRED_CUBEMAPS");

        for (uint32 nCurrentCubemap = nStartIndex; nCurrentCubemap < rCubemaps.Num(); ++nCurrentCubemap)
        {
            const SRenderLight& pDL = rCubemaps[nCurrentCubemap];
            if (pDL.m_Flags & (DLF_FAKE | DLF_VOLUMETRIC_FOG_ONLY))
            {
                continue;
            }
            DeferredCubemapPass(&pDL);
            m_nLightsProcessedCount++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DeferredCubemapPass(const SRenderLight* const __restrict pDL)
{
    PROFILE_SHADER_SCOPE;
    PROFILE_FRAME(CDeferredShading_CubemapPass);
    PROFILE_LABEL(pDL->m_sName);

    float scissorInt2Float[] = { (float)(pDL->m_sX), (float)(pDL->m_sY), (float)(pDL->m_sWidth), (float)(pDL->m_sHeight) };

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    SRenderPipeline& RESTRICT_REFERENCE rRP = rd->m_RP;

    rRP.m_nDeferredPrimitiveID = SHAPE_PROJECTOR;

    AZ_PUSH_DISABLE_WARNING(, "-Wconstant-logical-operand")
    bool bStencilMask =  CRenderer::CV_r_DeferredShadingStencilPrepass || CRenderer::CV_r_DebugLightVolumes;
    AZ_POP_DISABLE_WARNING
    bool bUseLightVolumes = false;
    bool bHasSpecular = false;

    const uint64 nOldFlags = rRP.m_FlagsShader_RT;

    rRP.m_FlagsShader_RT &= ~(RT_CLIPVOLUME_ID | RT_LIGHTSMASK | RT_GLOBAL_CUBEMAP | RT_SPECULAR_CUBEMAP | RT_BOX_PROJECTION);

    uint nNumClipVolumes = m_nClipVolumesCount[m_nThreadID][m_nRecurseLevel];
    rd->m_RP.m_FlagsShader_RT |= (pDL->m_Flags & DLF_BOX_PROJECTED_CM) ? RT_BOX_PROJECTION : 0;
    rd->m_RP.m_FlagsShader_RT |= nNumClipVolumes > 0 ? RT_CLIPVOLUME_ID : 0;

    // Store light properties (color/radius, position relative to camera, rect, z bounds)
    Vec4 pLightDiffuse = Vec4(pDL->m_Color.r, pDL->m_Color.g, pDL->m_Color.b, pDL->m_SpecMult);

    const float fInvRadius = (pDL->m_fRadius <= 0) ? 1.0f : 1.0f / pDL->m_fRadius;
    const Vec4 pLightPosCS = Vec4(pDL->m_Origin - m_pCamPos, fInvRadius);
    const float fAttenFalloffMax = max(pDL->GetFalloffMax(), 1e-3f);

    const bool bReverseDepth = (rRP.m_TI[rRP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;
    Vec4 pDepthBounds = GetLightDepthBounds(pDL, bReverseDepth);

    // avoiding LHS, comment out if we ever use different resolution for light accumulation target
    Vec4 pLightRect  = Vec4(scissorInt2Float[0], scissorInt2Float[1], scissorInt2Float[2], scissorInt2Float[3]);
    Vec4 scaledLightRect = Vec4(pLightRect.x * rRP.m_CurDownscaleFactor.x, pLightRect.y * rRP.m_CurDownscaleFactor.y,
            pLightRect.z * rRP.m_CurDownscaleFactor.x, pLightRect.w * rRP.m_CurDownscaleFactor.y);

    assert(!(pDL->m_Flags & DLF_PROJECT));

    if (CRenderer::CV_r_DeferredShadingLightLodRatio)
    {
        float fLightArea = pLightRect.z * pLightRect.w;
        float fScreenArea = (float) m_nCurTargetWidth * m_nCurTargetHeight;
        float fLightRatio = fLightArea / fScreenArea;

        const float fMinScreenAreaRatioThreshold = 0.01f;   // 1% of screen by default

        const float fDrawVolumeThres = 0.01f;
        if ((fLightRatio* CRenderer::CV_r_DeferredShadingLightLodRatio) < fDrawVolumeThres)
        {
            //scissor + depthbound test only
            bStencilMask = false;
        }
    }

    uint16 scaledX = (uint16)(scaledLightRect.x);
    uint16 scaledY = (uint16)(scaledLightRect.y);
    uint16 scaledWidth = (uint16)(scaledLightRect.z) + 1;
    uint16 scaledHeight = (uint16)(scaledLightRect.w) + 1;

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingScissor)
    {
        SetupScissors(true, scaledX, scaledY, scaledWidth, scaledHeight);
    }

    if (bStencilMask)
    {
#if !defined(CRY_USE_METAL) && !defined(ANDROID)
        SpecularAccEnableMRT(false);
#endif
        rd->SetDepthBoundTest(0.0f, 1.0f, false);
        rd->FX_StencilFrustumCull(-1, pDL, NULL, 0);
    }
    else
    if (rd->m_bDeviceSupports_NVDBT && CRenderer::CV_r_DeferredShadingDepthBoundsTest == 1)
    {
        rd->SetDepthBoundTest(pDepthBounds.x, pDepthBounds.z, true);
    }

    // todo: try out on consoles if DBT helps on light pass (on light stencil prepass is actually slower)
    if (rd->m_bDeviceSupports_NVDBT && bStencilMask && CRenderer::CV_r_DeferredShadingDepthBoundsTest && CRenderer::CV_r_deferredshadingDBTstencil)
    {
        rd->SetDepthBoundTest(pDepthBounds.x, pDepthBounds.z, true);
    }

    const float fFadeout = pDL->m_fProbeAttenuation;
    const float fLuminance = pDL->m_Color.Luminance() * fFadeout;

    if (fLuminance * pLightDiffuse.w >= 0.03f)  // if specular intensity is too low, skip it
    {
        rRP.m_FlagsShader_RT |= RT_SPECULAR_CUBEMAP;
        bHasSpecular = true;
    }

    if (CRenderer::CV_r_SlimGBuffer)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingLBuffersFmt == 2)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION];
    }

#if !defined(CRY_USE_METAL) && !defined(ANDROID)
    SpecularAccEnableMRT(bHasSpecular);
#endif

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_deferredshadingLightVolumes)
    {
        const Plane* pNearPlane(rd->GetCamera().GetFrustumPlane(FR_PLANE_NEAR));
        Vec3 n = pNearPlane->n;
        Vec3 e = pDL->m_ProbeExtents;
        Vec3 u0 = pDL->m_ObjMatrix.GetColumn0().GetNormalized();
        Vec3 u1 = pDL->m_ObjMatrix.GetColumn1().GetNormalized();
        Vec3 u2 = pDL->m_ObjMatrix.GetColumn2().GetNormalized();

        // Check if OBB intersects near plane
        float r = e.x * fabs(n.dot(u0)) + e.y * fabs(n.dot(u1)) + e.z * fabs(n.dot(u2));
        float s = pNearPlane->DistFromPlane(pDL->m_Origin);
        bUseLightVolumes = fabs(s) > r;
    }

    int MultplyState = m_nRenderState;

    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr)) // we'll do our own programmable blending in GMEM path
    {
        MultplyState &= ~GS_BLEND_MASK;
    }
    else
    {
        MultplyState &= ~GS_BLEND_MASK;
        MultplyState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;

        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDebug == 2)
        {
            // Debug mode
            MultplyState &= ~GS_BLEND_MASK;
            MultplyState |= (GS_BLSRC_ONE | GS_BLDST_ONE);
        }
    }

    if (bStencilMask)
    {
        MultplyState |= GS_STENCIL;
    }

    MultplyState &= ~(GS_NODEPTHTEST | GS_DEPTHFUNC_MASK);      // Ensure zcull used.
    MultplyState |= GS_DEPTHFUNC_LEQUAL;
    
    // Directional occlusion
    if (CRenderer::CV_r_ssdo)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_APPLY_SSDO];
    }
    
    // Render..
    if (bUseLightVolumes)
    {
        rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pCubemapsVolumeTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    }
    else
    {
        rRP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_CUBEMAP0];
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pCubemapsTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    }

    rd->FX_SetState(MultplyState);

    if (bStencilMask)
    {
        rd->FX_StencilTestCurRef(true, false);
    }

    m_pShader->FXSetPSFloat(m_pParamLightPos, &pLightPosCS, 1);
    m_pShader->FXSetPSFloat(m_pParamLightDiffuse, &pLightDiffuse, 1);

    if (CD3D9Renderer::eGT_256bpp_PATH != gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        m_pDepthRT->Apply(0, m_nTexStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);
        m_pNormalsRT->Apply(1, m_nTexStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);
        m_pDiffuseRT->Apply(2, m_nTexStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);
        m_pSpecularRT->Apply(3, m_nTexStatePoint, EFTT_UNKNOWN, -1, SResourceView::DefaultView);
    }

    static CCryNameR pszProbeOBBParams("g_mProbeOBBParams");
    Vec4 vProbeOBBParams[3];
    vProbeOBBParams[0] = Vec4(pDL->m_ObjMatrix.GetColumn0().GetNormalized(), 1 / pDL->m_ProbeExtents.x);
    vProbeOBBParams[1] = Vec4(pDL->m_ObjMatrix.GetColumn1().GetNormalized(), 1 / pDL->m_ProbeExtents.y);
    vProbeOBBParams[2] = Vec4(pDL->m_ObjMatrix.GetColumn2().GetNormalized(), 1 / pDL->m_ProbeExtents.z);
    m_pShader->FXSetPSFloat(pszProbeOBBParams, vProbeOBBParams, 3);

    if (pDL->m_Flags & DLF_BOX_PROJECTED_CM)
    {
        static CCryNameR pszBoxProjectionMin("g_vBoxProjectionMin");
        static CCryNameR pszBoxProjectionMax("g_vBoxProjectionMax");
        Vec4 vBoxProjectionMin(-pDL->m_fBoxLength * 0.5f, -pDL->m_fBoxWidth * 0.5f, -pDL->m_fBoxHeight * 0.5f, 0);
        Vec4 vBoxProjectionMax(pDL->m_fBoxLength * 0.5f,  pDL->m_fBoxWidth * 0.5f,  pDL->m_fBoxHeight * 0.5f, 0);

        m_pShader->FXSetPSFloat(pszBoxProjectionMin, &vBoxProjectionMin, 1);
        m_pShader->FXSetPSFloat(pszBoxProjectionMax, &vBoxProjectionMax, 1);
    }

    CTexture* texDiffuse  = (CTexture*)pDL->GetDiffuseCubemap();
    CTexture* texSpecular = (CTexture*)pDL->GetSpecularCubemap();

    // Use 4x4 mip for lowest gloss values
    Vec4 vCubemapParams(IntegerLog2((uint32)texSpecular->GetWidthNonVirtual()) - 2, 0, 0, 0);
    m_pShader->FXSetPSFloat(m_pGeneralParams, &vCubemapParams, 1);
    SD3DPostEffectsUtils::SetTexture(texDiffuse, 5, FILTER_BILINEAR, 1, texDiffuse->IsSRGB());
    SD3DPostEffectsUtils::SetTexture(texSpecular, 6, FILTER_TRILINEAR, 1, texSpecular->IsSRGB());

    uint32 stencilID = (pDL->m_nStencilRef[1] + 1) << 16 | pDL->m_nStencilRef[0] + 1;
    const Vec4 vAttenParams(fFadeout, *alias_cast<float*>(&stencilID), 0, fAttenFalloffMax);
    m_pShader->FXSetPSFloat(m_pAttenParams, &vAttenParams, 1);

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        m_pMSAAMaskRT->Apply(7, m_nTexStatePoint);
    }
    
    // Directional occlusion
    const int ssdoTexSlot = 8;
    SetSSDOParameters(ssdoTexSlot);
    
    if (nNumClipVolumes > 0)
    {
        if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            m_pResolvedStencilRT->Apply(9, m_nTexStatePoint, -1, -1, -1);
        }

        m_pShader->FXSetPSFloat(m_pClipVolumeParams, m_vClipVolumeParams, min((uint32)MAX_DEFERRED_CLIP_VOLUMES, nNumClipVolumes + VIS_AREAS_OUTDOOR_STENCIL_OFFSET));

        if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            // Global blend weight
            static CCryNameR clipVolGlobalBendWeight("g_fGlobalClipVolumeBlendWeight");
            Vec4 blendWeight(CRenderer::CV_r_GMEMVisAreasBlendWeight, 0, 0, 0);
            m_pShader->FXSetPSFloat(clipVolGlobalBendWeight, &blendWeight, 1);
        }
    }

    CTextureManager::Instance()->GetDefaultTexture("EnvironmentBRDF")->Apply(10, m_nTexStateLinear);

    //  If the texture is not loaded Metal runtime will assert
    if (texDiffuse->IsTextureLoaded() && (!bHasSpecular || texSpecular->IsTextureLoaded()))
    {
        if (bUseLightVolumes)
        {
            gcpRendD3D->D3DSetCull(eCULL_Back);

            Matrix33 rotMat(pDL->m_ObjMatrix.GetColumn0().GetNormalized() * pDL->m_ProbeExtents.x,
                pDL->m_ObjMatrix.GetColumn1().GetNormalized() * pDL->m_ProbeExtents.y,
                pDL->m_ObjMatrix.GetColumn2().GetNormalized() * pDL->m_ProbeExtents.z);
            Matrix34 mUnitVolumeToWorld = Matrix34::CreateTranslationMat(pDL->m_Origin) * rotMat * Matrix34::CreateScale(Vec3(2, 2, 2), Vec3(-1, -1, -1));

            DrawLightVolume(SHAPE_BOX, mUnitVolumeToWorld.GetTransposed());
        }
        else
        {
            rd->D3DSetCull(eCULL_Back, true);
            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight(), pDepthBounds.x);
        }
    }

    SD3DPostEffectsUtils::ShEndPass();

    if (bStencilMask)
    {
        rd->FX_StencilTestCurRef(false);
    }
    else if (rd->m_bDeviceSupports_NVDBT && CRenderer::CV_r_DeferredShadingDepthBoundsTest == 1)
    {
        rd->SetDepthBoundTest(0.f, 1.f, false);
    }

    if (rd->m_bDeviceSupports_NVDBT && bStencilMask && CRenderer::CV_r_DeferredShadingDepthBoundsTest && CRenderer::CV_r_deferredshadingDBTstencil)
    {
        rd->SetDepthBoundTest(0.f, 1.f, false);
    }

#if !defined(CRY_USE_METAL) && !defined(ANDROID)
    SpecularAccEnableMRT(true);
#endif

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingScissor)
    {
        rd->EF_Scissor(false, 0, 0, 0, 0);
    }

    rRP.m_FlagsShader_RT = nOldFlags;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::ScreenSpaceReflectionPass()
{
    if (CRenderer::CV_r_GraphicsPipeline & 1)
    {
        gcpRendD3D->GetGraphicsPipeline().RenderScreenSpaceReflections();
        return;
    }
    if (!CRenderer::CV_r_SSReflections || !CTexture::s_ptexHDRTarget)  // Sketch mode disables HDR rendering
    {
        return;
    }

    // SSR only supported on 128bpp GMEM path
    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        assert(CD3D9Renderer::eGT_128bpp_PATH == gcpRendD3D->FX_GetEnabledGmemPath(nullptr));
    }

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    const uint32 nPrevPersFlags = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags;

    Matrix44 mViewProj = rd->m_ViewMatrix * rd->m_ProjMatrix;

    if (rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        mViewProj = ReverseDepthHelper::Convert(mViewProj);
        rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_REVERSE_DEPTH;
    }

    Matrix44 mViewport(0.5f, 0,    0,    0,
        0,   -0.5f, 0,    0,
        0,    0,    1.0f, 0,
        0.5f, 0.5f, 0,    1.0f);
    const uint32 numGPUs = rd->GetActiveGPUCount();

    const int frameID = GetUtils().m_iFrameCounter;
    Matrix44 mViewProjPrev = m_prevViewProj[max((frameID - (int)numGPUs) % MAX_GPU_NUM, 0)] * mViewport;

    PROFILE_LABEL_SCOPE("SS_REFLECTIONS");

    const uint64 shaderFlags = rd->m_RP.m_FlagsShader_RT;

    if(CRenderer::CV_r_SlimGBuffer)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    // Get current viewport
    int prevVpX, prevVpY, prevVpWidth, prevVpHeight;
    gRenDev->GetViewport(&prevVpX, &prevVpY, &prevVpWidth, &prevVpHeight);

    {
        PROFILE_LABEL_SCOPE("SSR_RAYTRACE");

        if (CRenderer::CV_r_SlimGBuffer == 1)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
        }

        CTexture* dstTex = CRenderer::CV_r_SSReflHalfRes ? CTexture::s_ptexHDRTargetScaled[0] : CTexture::s_ptexHDRTarget;

        rd->FX_PushRenderTarget(0, dstTex, NULL);

#if defined(CRY_USE_METAL) || defined(ANDROID)
        const Vec2& vDownscaleFactor = gcpRendD3D->m_RP.m_CurDownscaleFactor;
        rd->RT_SetViewport(0, 0, dstTex->GetWidth() * vDownscaleFactor.x + 0.5f, dstTex->GetHeight() * vDownscaleFactor.y + 0.5f);
#else
        rd->RT_SetViewport(0, 0, dstTex->GetWidth(), dstTex->GetHeight());
#endif


        rd->FX_SetState(GS_NODEPTHTEST);
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pReflectionTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        m_pDepthRT->Apply(0, m_nTexStatePoint);
        m_pNormalsRT->Apply(1, m_nTexStateLinear);
        m_pSpecularRT->Apply(2, m_nTexStateLinear);
        CTexture::s_ptexZTargetScaled->Apply(3, m_nTexStatePoint);
        SD3DPostEffectsUtils::SetTexture(CTexture::s_ptexHDRTargetPrev, 4, FILTER_LINEAR, TADDR_BORDER);
        CTexture::s_ptexHDRMeasuredLuminance[rd->RT_GetCurrGpuID()]->Apply(5, m_nTexStatePoint);    // Current luminance

        static CCryNameR param0("g_mViewProj");
        m_pShader->FXSetPSFloat(param0, (Vec4*) mViewProj.GetData(), 4);

        static CCryNameR param1("g_mViewProjPrev");
        m_pShader->FXSetPSFloat(param1, (Vec4*) mViewProjPrev.GetData(), 4);

        SD3DPostEffectsUtils::DrawFullScreenTriWPOS(dstTex->GetWidth(), dstTex->GetHeight());
        SD3DPostEffectsUtils::ShEndPass();

        rd->FX_PopRenderTarget(0);
    }

    if (!CRenderer::CV_r_SSReflHalfRes)
    {
#if defined(CRY_USE_METAL) || defined(ANDROID)
        PostProcessUtils().StretchRect(CTexture::s_ptexHDRTarget, CTexture::s_ptexHDRTargetScaled[0], false, false, false, false, SPostEffectsUtils::eDepthDownsample_None, false, &gcpRendD3D->m_HalfResRect);
#else
        PostProcessUtils().StretchRect(CTexture::s_ptexHDRTarget, CTexture::s_ptexHDRTargetScaled[0]);
#endif
    }

    // Convolve sharp reflections

#if defined(CRY_USE_METAL) || defined(ANDROID)
    const Vec2& vDownscaleFactor = gcpRendD3D->m_RP.m_CurDownscaleFactor;
    gRenDev->RT_SetScissor(true, 0, 0, CTexture::s_ptexHDRTargetScaled[1]->GetWidth() * vDownscaleFactor.x + 0.5f, CTexture::s_ptexHDRTargetScaled[1]->GetHeight() * vDownscaleFactor.y + 0.5f);
#endif
    PostProcessUtils().StretchRect(CTexture::s_ptexHDRTargetScaled[0], CTexture::s_ptexHDRTargetScaled[1]);
    PostProcessUtils().TexBlurGaussian(CTexture::s_ptexHDRTargetScaled[1], 1, 1.0f, 3.0f, false, 0, false, CTexture::s_ptexHDRTargetScaledTempRT[1]);

#if defined(CRY_USE_METAL) || defined(ANDROID)
    gRenDev->RT_SetScissor(true, 0, 0, CTexture::s_ptexHDRTargetScaled[2]->GetWidth() * vDownscaleFactor.x + 0.5f, CTexture::s_ptexHDRTargetScaled[2]->GetHeight() * vDownscaleFactor.y + 0.5f);
#endif
    PostProcessUtils().StretchRect(CTexture::s_ptexHDRTargetScaled[1], CTexture::s_ptexHDRTargetScaled[2]);
    PostProcessUtils().TexBlurGaussian(CTexture::s_ptexHDRTargetScaled[2], 1, 1.0f, 3.0f, false, 0, false, CTexture::s_ptexHDRTargetScaledTempRT[2]);

#if defined(CRY_USE_METAL) || defined(ANDROID)
    gRenDev->RT_SetScissor(true, 0, 0, CTexture::s_ptexHDRTargetScaled[3]->GetWidth() * vDownscaleFactor.x + 0.5f, CTexture::s_ptexHDRTargetScaled[3]->GetHeight() * vDownscaleFactor.y + 0.5f);
#endif
    PostProcessUtils().StretchRect(CTexture::s_ptexHDRTargetScaled[2], CTexture::s_ptexHDRTargetScaled[3]);
    PostProcessUtils().TexBlurGaussian(CTexture::s_ptexHDRTargetScaled[3], 1, 1.0f, 3.0f, false, 0, false, CTexture::s_ptexHDRTargetScaledTempRT[3]);

    {
        PROFILE_LABEL_SCOPE("SSR_COMPOSE");

        if (CRenderer::CV_r_SlimGBuffer == 1)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
        }

        static CCryNameTSCRC tech("SSReflection_Comp");

        CTexture* dstTex = CTexture::s_ptexHDRTargetScaledTmp[0];
        dstTex->Unbind();

        rd->FX_SetState(GS_NODEPTHTEST);
        rd->FX_PushRenderTarget(0, dstTex, NULL);
        rd->RT_SetViewport(0, 0, dstTex->GetWidth(), dstTex->GetHeight());

        m_pSpecularRT->Apply(0, m_nTexStateLinear);
        CTexture::s_ptexHDRTargetScaled[0]->Apply(1, m_nTexStateLinear);
        CTexture::s_ptexHDRTargetScaled[1]->Apply(2, m_nTexStateLinear);
        CTexture::s_ptexHDRTargetScaled[2]->Apply(3, m_nTexStateLinear);
        CTexture::s_ptexHDRTargetScaled[3]->Apply(4, m_nTexStateLinear);

#if defined(CRY_USE_METAL) || defined(ANDROID)
        const Vec2& vDownscaleFactor = gcpRendD3D->m_RP.m_CurDownscaleFactor;
        gRenDev->RT_SetScissor(true, 0, 0, CTexture::s_ptexHDRTargetScaled[0]->GetWidth() * vDownscaleFactor.x + 0.5f, CTexture::s_ptexHDRTargetScaled[0]->GetHeight() * vDownscaleFactor.y + 0.5f);
#endif

        SD3DPostEffectsUtils::ShBeginPass(m_pShader, tech, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        SD3DPostEffectsUtils::DrawFullScreenTri(dstTex->GetWidth(), dstTex->GetHeight());
        SD3DPostEffectsUtils::ShEndPass();
        rd->FX_PopRenderTarget(0);

#if defined(CRY_USE_METAL) || defined(ANDROID)
        gRenDev->RT_SetScissor(false, 0, 0, 0, 0);
#endif
    }

    // Restore the old flags
    rd->m_RP.m_FlagsShader_RT = shaderFlags;
    rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags = nPrevPersFlags;

    if (rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        uint32 depthState = ReverseDepthHelper::ConvertDepthFunc(rd->m_RP.m_CurState);
        rd->FX_SetState(rd->m_RP.m_CurState, rd->m_RP.m_CurAlphaRef, depthState);
    }

    rd->RT_SetViewport(prevVpX, prevVpY, prevVpWidth, prevVpHeight);

    // Array used for MGPU support
    m_prevViewProj[frameID % MAX_GPU_NUM] = mViewProj;
}

void CDeferredShading::ApplySSReflections()
{
    if (!CRenderer::CV_r_SSReflections || !CTexture::s_ptexHDRTarget)  // Sketch mode disables HDR rendering
    {
        return;
    }

    // SSR only supported on 128bpp GMEM path
    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        assert(CD3D9Renderer::eGT_128bpp_PATH == gcpRendD3D->FX_GetEnabledGmemPath(nullptr));
    }

    CTexture* pSSRTarget = CTexture::s_ptexHDRTargetScaledTmp[0];

    PROFILE_LABEL_SCOPE("SSR_APPLY");

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    if (CRenderer::CV_r_SlimGBuffer == 1)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        SpecularAccEnableMRT(false);
        gcpRendD3D->FX_PushRenderTarget(0, m_pLBufferSpecularRT, NULL);
    }

    static CCryNameTSCRC tech("ApplySSR");
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, tech, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    gcpRendD3D->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);

    pSSRTarget->Apply(0, m_nTexStateLinear);
    m_pDepthRT->Apply(1, m_nTexStatePoint);
    m_pNormalsRT->Apply(2, m_nTexStatePoint);
    m_pDiffuseRT->Apply(3, m_nTexStatePoint);
    m_pSpecularRT->Apply(4, m_nTexStatePoint);

    CTextureManager::Instance()->GetDefaultTexture("EnvironmentBRDF")->Apply(5, m_nTexStateLinear);

#if defined(CRY_USE_METAL) || defined(ANDROID)
    SD3DPostEffectsUtils::DrawFullScreenTriWPOS(pSSRTarget->GetWidth(), pSSRTarget->GetHeight(), 0, &gcpRendD3D->m_HalfResRect);
#else
    SD3DPostEffectsUtils::DrawFullScreenTriWPOS(pSSRTarget->GetWidth(), pSSRTarget->GetHeight());
#endif
    SD3DPostEffectsUtils::ShEndPass();

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        gcpRendD3D->FX_PopRenderTarget(0);
    }
    SpecularAccEnableMRT(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::HeightMapOcclusionPass(ShadowMapFrustum*& pHeightMapFrustum, CTexture*& pHeightMapAOScreenDepth, CTexture*& pHeightmapAO)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    const int nThreadID = rd->m_RP.m_nProcessThreadID;
    pHeightMapFrustum = NULL;
    pHeightMapAOScreenDepth = pHeightmapAO = NULL;

    if (!CRenderer::CV_r_HeightMapAO || rd->m_RP.m_pSunLight == NULL)
    {
        return;
    }

    // find shadow frustum for height map AO
    for (int nFrustum = 0; nFrustum < rd->m_RP.m_SMFrustums[nThreadID][0].size(); ++nFrustum)
    {
        ShadowMapFrustum* pCurFr = &rd->m_RP.m_SMFrustums[nThreadID][0][nFrustum];
        if (pCurFr->m_eFrustumType == ShadowMapFrustum::e_HeightMapAO && pCurFr->pDepthTex)
        {
            rd->ConfigShadowTexgen(0, pCurFr, -1, false, false);
            pHeightMapFrustum = pCurFr;
            break;
        }
    }

    if (pHeightMapFrustum)
    {
        PROFILE_LABEL_SCOPE("HEIGHTMAP_OCC");

        const int resolutionIndex = clamp_tpl(CRenderer::CV_r_HeightMapAO - 1, 0, 2);
        CTexture* pDepth[] = {  CTexture::s_ptexZTargetScaled2, CTexture::s_ptexZTargetScaled, m_pDepthRT };
        CTexture* pDst = CTexture::s_ptexHeightMapAO[0];

        if (!CTexture::s_ptexHeightMapAODepth[0]->IsResolved())
        {
            PROFILE_LABEL_SCOPE("GENERATE_MIPS");

            rd->GetDeviceContext().CopySubresourceRegion(
                CTexture::s_ptexHeightMapAODepth[1]->GetDevTexture()->GetBaseTexture(), 0, 0, 0, 0,
                CTexture::s_ptexHeightMapAODepth[0]->GetDevTexture()->GetBaseTexture(), 0, NULL
                );

            CTexture::s_ptexHeightMapAODepth[1]->GenerateMipMaps();
            CTexture::s_ptexHeightMapAODepth[0]->SetResolved(true);
        }

        // Generate occlusion
        {
            PROFILE_LABEL_SCOPE("GENERATE_OCCL");

            rd->FX_PushRenderTarget(0, pDst, NULL);

            static CCryNameTSCRC tech("HeightMapAOPass");
            SD3DPostEffectsUtils::ShBeginPass(m_pShader, tech, FEF_DONTSETSTATES);
            rd->FX_SetState(GS_NODEPTHTEST);

            int TsLinearWithBorder = CTexture::GetTexState(STexState(FILTER_TRILINEAR, TADDR_BORDER, TADDR_BORDER, TADDR_BORDER, 0xFFFFFFFF));

            m_pNormalsRT->Apply(0, m_nTexStatePoint);
            pDepth[resolutionIndex]->Apply(1, m_nTexStatePoint);
            CTexture::s_ptexSceneNormalsBent->Apply(10, m_nTexStatePoint);
            CTexture::s_ptexHeightMapAODepth[1]->Apply(11, TsLinearWithBorder);

            Matrix44A matHMAOTransform = gRenDev->m_TempMatrices[0][0];
            Matrix44A texToWorld = matHMAOTransform.GetInverted();

            static CCryNameR paramNameHMAO("HMAO_Params");
            const float texelsPerMeter = CRenderer::CV_r_HeightMapAOResolution / CRenderer::CV_r_HeightMapAORange;
            const bool enableMinMaxSampling = CRenderer::CV_r_HeightMapAO < 3;
            Vec4 paramHMAO(CRenderer::CV_r_HeightMapAOAmount, texelsPerMeter / CTexture::s_ptexHeightMapAODepth[1]->GetWidth(), enableMinMaxSampling ? 1.0 : 0.0, 0.0f);
            m_pShader->FXSetPSFloat(paramNameHMAO, &paramHMAO, 1);

            static CCryNameR paramNameHMAO_TexToWorldT("HMAO_TexToWorldTranslation");
            Vec4 vTranslation = Vec4(texToWorld.m03, texToWorld.m13, texToWorld.m23, 0);
            m_pShader->FXSetPSFloat(paramNameHMAO_TexToWorldT, &vTranslation, 1);

            static CCryNameR paramNameHMAO_TexToWorldS("HMAO_TexToWorldScale");
            Vec4 vScale = Vec4(texToWorld.m00, texToWorld.m11, texToWorld.m22, 1);
            m_pShader->FXSetPSFloat(paramNameHMAO_TexToWorldS, &vScale, 1);

            static CCryNameR paramHMAO_Transform("HMAO_Transform");
            m_pShader->FXSetPSFloat(paramHMAO_Transform, (Vec4*)matHMAOTransform.GetData(), 4);

            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(pDst->GetWidth(), pDst->GetHeight());
            SD3DPostEffectsUtils::ShEndPass();

            rd->FX_PopRenderTarget(0);
        }

        // depth aware blur
        {
            PROFILE_LABEL_SCOPE("BLUR");

            CShader* pSH = rd->m_cEF.s_ShaderShadowBlur;

            CTexture* tpSrc = pDst;
            rd->FX_PushRenderTarget(0, CTexture::s_ptexHeightMapAO[1], NULL);

            const Vec4* pClipVolumeParams = NULL;
            uint32 nClipVolumeCount = 0;
            CDeferredShading::Instance().GetClipVolumeParams(pClipVolumeParams, nClipVolumeCount);

            rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]);
            rd->m_RP.m_FlagsShader_RT |= nClipVolumeCount > 0 ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0;

            static CCryNameTSCRC TechName("HMAO_Blur");
            SD3DPostEffectsUtils::ShBeginPass(pSH, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            tpSrc->Apply(0, m_nTexStatePoint, -2);
            pDepth[resolutionIndex]->Apply(1, m_nTexStatePoint, -2);

            if (nClipVolumeCount)
            {
                static CCryNameR paramClipVolumeData("HMAO_ClipVolumeData");
                pSH->FXSetPSFloat(paramClipVolumeData, pClipVolumeParams, min((uint32)MAX_DEFERRED_CLIP_VOLUMES, nClipVolumeCount + VIS_AREAS_OUTDOOR_STENCIL_OFFSET));
                SD3DPostEffectsUtils::SetTexture(CDeferredShading::Instance().GetResolvedStencilRT(), 2, FILTER_POINT);
            }

            rd->D3DSetCull(eCULL_Back);
            rd->FX_SetState(GS_NODEPTHTEST);

            Vec4 v(0, 0, tpSrc->GetWidth(), tpSrc->GetHeight());
            static CCryNameR Param1Name("PixelOffset");
            pSH->FXSetVSFloat(Param1Name, &v, 1);

            SD3DPostEffectsUtils::DrawFullScreenTri(CTexture::s_ptexHeightMapAO[1]->GetWidth(), CTexture::s_ptexHeightMapAO[1]->GetHeight());
            SD3DPostEffectsUtils::ShEndPass();

            rd->FX_PopRenderTarget(0);
        }

        pHeightMapAOScreenDepth = pDepth[resolutionIndex];
        pHeightmapAO = CTexture::s_ptexHeightMapAO[1];
    }
}

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void CDeferredShading::DirectionalOcclusionPass()
{
    if (CRenderer::CV_r_GraphicsPipeline & 1)
    {
        gcpRendD3D->GetGraphicsPipeline().RenderScreenSpaceObscurance();
        return;
    }

    AZ_TRACE_METHOD();

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    if (!CRenderer::CV_r_ssdo)
    {
        if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            gcpRendD3D->FX_ClearTarget(CTexture::s_ptexSceneNormalsBent, Clr_Median);
        }
        return;
    }

    // SSDO only supported on 128bpp GMEM path
    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        assert(CD3D9Renderer::eGT_128bpp_PATH == gcpRendD3D->FX_GetEnabledGmemPath(nullptr));
    }

    // calculate height map AO first
    ShadowMapFrustum* pHeightMapFrustum = NULL;
    CTexture* pHeightMapAODepth, * pHeightMapAO;
    HeightMapOcclusionPass(pHeightMapFrustum, pHeightMapAODepth, pHeightMapAO);

    rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]);
    CTexture* pDstSSDO = CTexture::s_ptexStereoR;// re-using stereo buffers (only full resolution 32bit non-multisampled available at this step)
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DDEFERREDSHADING_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(D3DDeferredShading_cpp)
#endif

    const bool bLowResOutput = (CRenderer::CV_r_ssdoHalfRes == 3);
    if (bLowResOutput)
    {
        pDstSSDO = CTexture::s_ptexBackBufferScaled[0];
    }

    rd->m_RP.m_FlagsShader_RT |= CRenderer::CV_r_ssdoHalfRes ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0;
    rd->m_RP.m_FlagsShader_RT |= pHeightMapFrustum ? g_HWSR_MaskBit[HWSR_SAMPLE1] : 0;

    bool isRenderingFur = FurPasses::GetInstance().IsRenderingFur();
    rd->m_RP.m_FlagsShader_RT |= isRenderingFur ? g_HWSR_MaskBit[HWSR_SAMPLE2] : 0;

    // Extreme magnification as happening with small FOVs will cause banding issues with half-res depth
    if (CRenderer::CV_r_ssdoHalfRes == 2 && RAD2DEG(rd->GetCamera().GetFov()) < 30)
    {
        rd->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    PROFILE_LABEL_PUSH("DIRECTIONAL_OCC");

    bool allowDepthBounds = !bLowResOutput;
    rd->FX_PushRenderTarget(0, pDstSSDO, allowDepthBounds ? &gcpRendD3D->m_DepthBufferOrig : NULL);
    rd->FX_SetColorDontCareActions(0, true, false);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.0f, DBT_SKY_CULL_DEPTH, allowDepthBounds);
    }

    static CCryNameTSCRC tech("DirOccPass");
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, tech, FEF_DONTSETSTATES);

    rd->FX_SetState(GS_NODEPTHTEST);
    m_pNormalsRT->Apply(0, m_nTexStatePoint);
    CTexture::s_ptexZTarget->Apply(1, m_nTexStatePoint);
    SPostEffectsUtils::SetTexture(CTextureManager::Instance()->GetDefaultTexture("AOVOJitter"), 3, FILTER_POINT, 0);
    if (bLowResOutput)
    {
        CTexture::s_ptexZTargetScaled2->Apply(5, m_nTexStatePoint);
    }
    else
    {
        CTexture::s_ptexZTargetScaled->Apply(5, m_nTexStatePoint);
    }

    if (isRenderingFur)
    {
        // Bind fur Z target - difference of the two Z targets indicates a stipple that needs avoided for SSDO
        CTexture::s_ptexFurZTarget->Apply(2, m_nTexStatePoint);
    }

    Matrix44A matView;
    matView = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_cam.GetViewMatrix();

    // Adjust the camera matrix so that the camera space will be: +y = down, +z - towards, +x - right
    Vec3 zAxis = matView.GetRow(1);
    matView.SetRow(1, -matView.GetRow(2));
    matView.SetRow(2, zAxis);
    float z = matView.m13;
    matView.m13 = -matView.m23;
    matView.m23 = z;

    float radius = CRenderer::CV_r_ssdoRadius / rd->GetViewParameters().fFar;
#if defined(FEATURE_SVO_GI)
    if (CSvoRenderer::GetInstance()->IsActive())
    {
        radius *= CSvoRenderer::GetInstance()->GetSsaoAmount();
    }
#endif
    static CCryNameR paramName1("SSDOParams");
    Vec4 param1(radius * 0.5f * rd->m_ProjMatrix.m00, radius * 0.5f * rd->m_ProjMatrix.m11,
        CRenderer::CV_r_ssdoRadiusMin, CRenderer::CV_r_ssdoRadiusMax);
    m_pShader->FXSetPSFloat(paramName1, &param1, 1);

    static CCryNameR viewspaceParamName("ViewSpaceParams");
    Vec4 vViewSpaceParam(2.0f / rd->m_ProjMatrix.m00, 2.0f / rd->m_ProjMatrix.m11, -1.0f / rd->m_ProjMatrix.m00, -1.0f / rd->m_ProjMatrix.m11);
    m_pShader->FXSetPSFloat(viewspaceParamName, &vViewSpaceParam, 1);

    static CCryNameR paramName2("SSDO_CameraMatrix");
    m_pShader->FXSetPSFloat(paramName2, (Vec4*)matView.GetData(), 3);

    matView.Invert();
    static CCryNameR paramName3("SSDO_CameraMatrixInv");
    m_pShader->FXSetPSFloat(paramName3, (Vec4*)matView.GetData(), 3);

    // set up height map AO
    if (pHeightMapFrustum)
    {
        pHeightMapAODepth->Apply(11, -2);
        pHeightMapAO->Apply(12, -2);

        static CCryNameR paramNameHMAO("HMAO_Params");
        Vec4 paramHMAO(CRenderer::CV_r_HeightMapAOAmount, 1.0f / pHeightMapFrustum->nTexSize, 0, 0);
        m_pShader->FXSetPSFloat(paramNameHMAO, &paramHMAO, 1);
    }

#if defined(CRY_USE_METAL) || defined(ANDROID)
    const Vec2& vDownscaleFactor = gcpRendD3D->m_RP.m_CurDownscaleFactor;
    gRenDev->RT_SetScissor(true, 0, 0, pDstSSDO->GetWidth() * vDownscaleFactor.x + 0.5f, pDstSSDO->GetHeight() * vDownscaleFactor.y + 0.5f);
#endif

    SD3DPostEffectsUtils::DrawFullScreenTriWPOS(pDstSSDO->GetWidth(), pDstSSDO->GetHeight(), 0);//, &gcpRendD3D->m_FullResRect);
    SD3DPostEffectsUtils::ShEndPass();

#if defined(CRY_USE_METAL) || defined(ANDROID)
    gRenDev->RT_SetScissor(false, 0, 0, 0, 0);
#endif

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.0f, DBT_SKY_CULL_DEPTH, false);
    }

    rd->FX_PopRenderTarget(0);

    if (CRenderer::CV_r_ssdo != 99)
    {
        CShader* pSH = rd->m_cEF.s_ShaderShadowBlur;
        CTexture* tpSrc = pDstSSDO;

        const int32 nSizeX = m_pDepthRT->GetWidth();
        const int32 nSizeY = m_pDepthRT->GetHeight();

        const int32 nSrcSizeX = tpSrc->GetWidth();
        const int32 nSrcSizeY = tpSrc->GetHeight();

        PROFILE_LABEL_SCOPE("SSDO_BLUR");
        rd->FX_PushRenderTarget(0, CTexture::s_ptexSceneNormalsBent, NULL);

        static CCryNameTSCRC TechName("SSDO_Blur");
        SD3DPostEffectsUtils::ShBeginPass(pSH, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        tpSrc->Apply(0, m_nTexStateLinear);
        CTexture::s_ptexZTarget->Apply(1, m_nTexStatePoint);

        rd->D3DSetCull(eCULL_Back);
        rd->FX_SetState(GS_NODEPTHTEST);

        Vec4 v(0, 0, nSrcSizeX, nSrcSizeY);
        static CCryNameR Param1Name("PixelOffset");
        pSH->FXSetVSFloat(Param1Name, &v, 1);

        v = Vec4(0.5f / (float)nSizeX, 0.5f / (float)nSizeY, 1.f / (float)nSrcSizeX, 1.f / (float)nSrcSizeY);
        static CCryNameR Param2Name("BlurOffset");
        pSH->FXSetPSFloat(Param2Name, &v, 1);

        v = Vec4(2.f / nSrcSizeX, 0, 2.f / nSrcSizeY, 10.f); //w = Weight coef
        static CCryNameR Param3Name("SSAO_BlurKernel");
        pSH->FXSetPSFloat(Param3Name, &v, 1);

        SD3DPostEffectsUtils::DrawFullScreenTri(CTexture::s_ptexSceneNormalsBent->GetWidth(), CTexture::s_ptexSceneNormalsBent->GetHeight(), 0, &gcpRendD3D->m_FullResRect);
        SD3DPostEffectsUtils::ShEndPass();

        rd->FX_PopRenderTarget(0);
    }
    else  // For debugging
    {
        PostProcessUtils().StretchRect(pDstSSDO, CTexture::s_ptexSceneNormalsBent);
    }

    rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]);

    if (CRenderer::CV_r_ssdoColorBleeding)
    {
        // Generate low frequency scene albedo for color bleeding (convolution not gamma correct but acceptable)
        PostProcessUtils().StretchRect(CTexture::s_ptexSceneDiffuse, CTexture::s_ptexBackBufferScaled[0], false, false, false, false);
        PostProcessUtils().StretchRect(CTexture::s_ptexBackBufferScaled[0], CTexture::s_ptexBackBufferScaled[1]);
        PostProcessUtils().StretchRect(CTexture::s_ptexBackBufferScaled[1], CTexture::s_ptexAOColorBleed);
        PostProcessUtils().TexBlurGaussian(CTexture::s_ptexAOColorBleed, 1, 1.0f, 4.0f, false, NULL, false, CTexture::s_ptexBackBufferScaled[2]);
    }

    PROFILE_LABEL_POP("DIRECTIONAL_OCC");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CDeferredShading::DeferredSubsurfaceScattering(CTexture* tmpTex)
{
    if (CRenderer::CV_r_GraphicsPipeline & 1)
    {
        m_pLBufferDiffuseRT->Unbind();
        gcpRendD3D->GetGraphicsPipeline().RenderScreenSpaceSSS(tmpTex);
        return;
    }

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if (!CTexture::s_ptexHDRTarget)  // Sketch mode disables HDR rendering
    {
        return;
    }

    PROFILE_LABEL_SCOPE("SSSSS");

    const uint64 nFlagsShaderRT = rd->m_RP.m_FlagsShader_RT;
    rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_DEBUG0]);

    if (CRenderer::CV_r_SlimGBuffer == 1)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    static CCryNameTSCRC techBlur("SSSSS_Blur");
    static CCryNameR blurParamName("SSSBlurDir");
    static CCryNameR viewspaceParamName("ViewSpaceParams");
    Vec4 vViewSpaceParam(2.0f / rd->m_ProjMatrix.m00, 2.0f / rd->m_ProjMatrix.m11, -1.0f / rd->m_ProjMatrix.m00, -1.0f / rd->m_ProjMatrix.m11);
    Vec4 vBlurParam;

    float fProjScaleX = 0.5f * rd->m_ProjMatrix.m00;
    float fProjScaleY = 0.5f * rd->m_ProjMatrix.m11;

    m_pLBufferDiffuseRT->Unbind();
    m_pDepthRT->Apply(1, m_nTexStatePoint);
    m_pNormalsRT->Apply(2, m_nTexStatePoint);
    m_pDiffuseRT->Apply(3, m_nTexStatePoint);
    m_pSpecularRT->Apply(4, m_nTexStatePoint);

    rd->FX_SetState(GS_NODEPTHTEST);

    // Selectively enables debug mode permutation if a debug parameter is non-zero.
    bool bEnableDebug = false;
    Vec4 vDebugParams(
        (float)rd->CV_r_DeferredShadingTiledDebugDirect,
        (float)rd->CV_r_DeferredShadingTiledDebugIndirect,
        (float)rd->CV_r_DeferredShadingTiledDebugAccumulation,
        (float)rd->CV_r_DeferredShadingTiledDebugAlbedo);
    if (vDebugParams.Dot(vDebugParams) > 0.0f) // Simple check to see if anything is enabled.
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
        bEnableDebug = true;
    }
    static CCryNameR paramDebug("LightingDebugParams");

#if defined (CRY_USE_METAL)
    //Need to clear this texture as it can have undefined or bad data on load. 
    rd->FX_ClearTarget(CTexture::s_ptexSceneTargetR11G11B10F[1], Clr_Empty);
#endif

    // Horizontal pass
    rd->FX_PushRenderTarget(0, CTexture::s_ptexSceneTargetR11G11B10F[1], NULL);
    
    tmpTex->Apply(0, m_nTexStatePoint);  // Irradiance
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, techBlur, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    m_pShader->FXSetPSFloat(viewspaceParamName, &vViewSpaceParam, 1);
    vBlurParam(fProjScaleX, 0, 0, 0);
    m_pShader->FXSetPSFloat(blurParamName, &vBlurParam, 1);
    if (bEnableDebug)
    {
        CShaderMan::s_shDeferredShading->FXSetPSFloat(paramDebug, &vDebugParams, 1);
    }
    SD3DPostEffectsUtils::DrawFullScreenTri(CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());
    SD3DPostEffectsUtils::ShEndPass();
    rd->FX_PopRenderTarget(0);

    rd->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);
    rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

    // Vertical pass
    rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRTarget, NULL);
    CTexture::s_ptexSceneTargetR11G11B10F[1]->Apply(0, m_nTexStatePoint);
    tmpTex->Apply(5, m_nTexStatePoint);  // Original irradiance
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, techBlur, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    m_pShader->FXSetPSFloat(viewspaceParamName, &vViewSpaceParam, 1);
    vBlurParam(0, fProjScaleY, 0, 0);
    m_pShader->FXSetPSFloat(blurParamName, &vBlurParam, 1);
    if (bEnableDebug)
    {
        CShaderMan::s_shDeferredShading->FXSetPSFloat(paramDebug, &vDebugParams, 1);
    }
    SD3DPostEffectsUtils::DrawFullScreenTri(CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());
    SD3DPostEffectsUtils::ShEndPass();
    rd->FX_PopRenderTarget(0);

    rd->m_RP.m_FlagsShader_RT = nFlagsShaderRT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DeferredShadingPass()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;


    if (rd->IsShadowPassEnabled())
    {
        PROFILE_LABEL_SCOPE("SHADOWMASK");
        rd->FX_DeferredShadowMaskGen(TArray<uint32>());
    }

    rd->D3DSetCull(eCULL_Back, true); //fs quads should not revert test..

    PROFILE_LABEL_PUSH("DEFERRED_SHADING");

    CTexture* tmpTexSSS = CTexture::s_ptexSceneTargetR11G11B10F[0];

    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        tmpTexSSS = nullptr;
    }

    const uint64 nFlagsShaderRT = rd->m_RP.m_FlagsShader_RT;
    rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_APPLY_SSDO] | g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION] |RT_CLIPVOLUME_ID);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.0f, DBT_SKY_CULL_DEPTH, true);
    }

    bool deferredSSS = CRenderer::CV_r_DeferredShadingSSS != 0;

    // Deferred subsurface scattering
    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        deferredSSS = false; // Explicitly disable deferredSSS as it's not currently supported on GMEM path
    }

    bool isRenderingFur = FurPasses::GetInstance().IsRenderingFur();
    if (deferredSSS || isRenderingFur)
    {
        // Output diffuse accumulation if SSS is enabled or if there are render items using fur
        rd->FX_PushRenderTarget(1, tmpTexSSS, nullptr);
    }

    PROFILE_LABEL_PUSH("COMPOSITION");
    static CCryNameTSCRC techComposition("DeferredShadingPass");

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        // CONFETTI BEGIN: David Srour
        // Metal Load/Store Actions
        rd->FX_SetDepthDontCareActions(0, false, true);
        rd->FX_SetStencilDontCareActions(0, false, true);
        rd->FX_SetDepthDontCareActions(1, false, true);
        rd->FX_SetStencilDontCareActions(1, false, true);
        // CONFETTI END
    }

    if (deferredSSS || isRenderingFur)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingAreaLights)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    }

    const uint32 nNumClipVolumes = m_nClipVolumesCount[m_nThreadID][m_nRecurseLevel];
    if (nNumClipVolumes)
    {
        rd->m_RP.m_FlagsShader_RT |= RT_CLIPVOLUME_ID;
    }

    // Enable sun permutation (eg: when fully inside vis areas and sun not visible/used, skip sun computation)
    if (rd->m_RP.m_pSunLight)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
    }
    
    if (CRenderer::CV_r_SlimGBuffer)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingLBuffersFmt == 2)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEFERRED_RENDER_TARGET_OPTIMIZATION];
    }

    // Directional occlusion
    if (CRenderer::CV_r_ssdo)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_APPLY_SSDO];
    }
    
    
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, techComposition, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    rd->FX_SetState(GS_NODEPTHTEST);
    if (CD3D9Renderer::eGT_256bpp_PATH != gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        m_pDiffuseRT->Apply(2, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1);
        m_pSpecularRT->Apply(3, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1);
        m_pNormalsRT->Apply(4, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1);
        m_pDepthRT->Apply(5, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1);
    }

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))     // Following are already in GMEM
    {
        m_pLBufferDiffuseRT->Apply(0, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1);
        m_pLBufferSpecularRT->Apply(1, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1);
        m_pResolvedStencilRT->Apply(6, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1);
    }
    
    // Directional occlusion
    const int ssdoTexSlot = 7;
    SetSSDOParameters(ssdoTexSlot);
    
    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))     // Following are already in GMEM
    {
        CTexture::s_ptexShadowMask->Apply(8, m_nTexStatePoint);
        m_pDepthRT->Apply(9, m_nTexStatePoint);
    }

    Vec3 sunColor;
    gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SUN_COLOR, sunColor);

    static CCryNameR paramNameSunColor("SunColor");
    const Vec4 paramSunColor(sunColor, gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SUN_SPECULAR_MULTIPLIER));
    m_pShader->FXSetPSFloat(paramNameSunColor, &paramSunColor, 1);

    if (nNumClipVolumes)
    {
        m_pShader->FXSetPSFloat(m_pClipVolumeParams, m_vClipVolumeParams, min((uint32)MAX_DEFERRED_CLIP_VOLUMES, nNumClipVolumes + VIS_AREAS_OUTDOOR_STENCIL_OFFSET));

        if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            // Global blend weight
            static CCryNameR clipVolGlobalBendWeight("g_fGlobalClipVolumeBlendWeight");
            Vec4 blendWeight(CRenderer::CV_r_GMEMVisAreasBlendWeight, 0, 0, 0);
            m_pShader->FXSetPSFloat(clipVolGlobalBendWeight, &blendWeight, 1);
        }
    }

    const float SunSourceDiameter = 94.0f;      // atan(AngDiameterSun) * 2 * SunDistance, where AngDiameterSun=0.54deg and SunDistance=10000
    static CCryNameR arealightMatrixName("g_AreaLightMatrix");
    Matrix44 mAreaLightMatrix;
    mAreaLightMatrix.SetIdentity();
    mAreaLightMatrix.SetRow4(3, Vec4(SunSourceDiameter,  SunSourceDiameter, 0, 1.0f));
    m_pShader->FXSetPSFloat(arealightMatrixName, alias_cast<Vec4*>(&mAreaLightMatrix), 4);

    SD3DPostEffectsUtils::DrawFullScreenTriWPOS(m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight(), 0, &gcpRendD3D->m_FullResRect);
    SD3DPostEffectsUtils::ShEndPass();

    PROFILE_LABEL_POP("COMPOSITION");

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.0f, DBT_SKY_CULL_DEPTH, false);
    }

    if (deferredSSS || isRenderingFur)
    {
        rd->FX_PopRenderTarget(1);
        rd->FX_SetActiveRenderTargets();
        DeferredSubsurfaceScattering(tmpTexSSS);
    }

    FurPasses::GetInstance().ExecuteObliteratePass();

    rd->m_RP.m_FlagsShader_RT = nFlagsShaderRT;

    PROFILE_LABEL_POP("DEFERRED_SHADING");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DeferredLights(TArray<SRenderLight>& rLights, bool bCastShadows)
{
    if (rLights.Num())
    {
        PROFILE_LABEL_SCOPE("DEFERRED_LIGHTS");

        if (bCastShadows)
        {
            PackAllShadowFrustums(rLights, false);
        }

        for (uint32 nCurrentLight = 0; nCurrentLight < rLights.Num(); ++nCurrentLight)
        {
            const SRenderLight& pDL = rLights[nCurrentLight];
            if (pDL.m_Flags & (DLF_FAKE | DLF_VOLUMETRIC_FOG_ONLY))
            {
                continue;
            }

            assert(pDL.GetSpecularCubemap() == NULL);
            if (!(pDL.m_Flags & DLF_CASTSHADOW_MAPS))
            {
                LightPass(&pDL);
            }

            m_nLightsProcessedCount++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

StaticInstance<CPowerOf2BlockPacker> CDeferredShading::m_blockPack;
TArray<SShadowAllocData> CDeferredShading::m_shadowPoolAlloc;

bool CDeferredShading::PackAllShadowFrustums(TArray<SRenderLight>& arrLights, bool bPreLoop)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    const uint64 nPrevFlagsShaderRT = rd->m_RP.m_FlagsShader_RT;

    static ICVar* p_e_ShadowsPoolSize = iConsole->GetCVar("e_ShadowsPoolSize");
    const bool isGmemEnabled = gcpRendD3D->FX_GetEnabledGmemPath(nullptr) != CD3D9Renderer::eGT_REGULAR_PATH;

    int nRequestedPoolSize = p_e_ShadowsPoolSize->GetIVal();
    if (m_nShadowPoolSize !=  nRequestedPoolSize)
    {
        m_blockPack->UpdateSize(nRequestedPoolSize >> TEX_POOL_BLOCKLOGSIZE, nRequestedPoolSize >> TEX_POOL_BLOCKLOGSIZE);
        m_nShadowPoolSize = nRequestedPoolSize;

        // clear pool and reset allocations
        m_blockPack->Clear();
        m_shadowPoolAlloc.SetUse(0);
    }

    // light pass here
    if (!bPreLoop)
    {
        for (uint nLightPacked = m_nFirstCandidateShadowPoolLight; nLightPacked < m_nCurrentShadowPoolLight; ++nLightPacked)
        {
            const SRenderLight& rLight = arrLights[nLightPacked];
            if (rLight.m_Flags & DLF_FAKE)
            {
                continue;
            }
            ShadowLightPasses(rLight);
        }
    }

    while (m_nCurrentShadowPoolLight < arrLights.Num()) //pre-loop to avoid 0.5 ms restore/resolve
    {
        SRenderLight& rLight = arrLights[m_nCurrentShadowPoolLight];
        if (!(rLight.m_Flags & DLF_DIRECTIONAL) && (rLight.m_Flags & DLF_CASTSHADOW_MAPS))
        {
            break;
        }
        m_nCurrentShadowPoolLight++;
    }

    if (bPreLoop && m_nCurrentShadowPoolLight < arrLights.Num()) // Shadow allocation tick, free old shadows.
    {
        uint32 nAllocs = m_shadowPoolAlloc.Num();
        for (uint32 i = 0; i < nAllocs; i++)
        {
            SShadowAllocData* pAlloc = &m_shadowPoolAlloc[i];
            uint32 currFrame = gcpRendD3D->GetFrameID(false) & 0xFF;
            if (!pAlloc->isFree()  && ((currFrame - pAlloc->m_frameID) > (uint)CRenderer::CV_r_ShadowPoolMaxFrames))
            {
                m_blockPack->RemoveBlock(pAlloc->m_blockID);
                pAlloc->Clear();
                break;  //Max one delete per frame, this should spread updates across more frames
            }
        }
    }

    // In GMEM we only pack shadows during the preloop (that happens before the deferred lighting pass)
    // We don't do it during the deferred lighting pass (preloop = false) because that would cause a resolve of the lighting accumulation buffers.
    bool packShadows = bPreLoop || !isGmemEnabled;
    bool bShadowRendered = false;
    while (m_nCurrentShadowPoolLight < arrLights.Num() && (!bPreLoop || !bShadowRendered))
    {
        m_nFirstCandidateShadowPoolLight = m_nCurrentShadowPoolLight;

        // init before shadowgen
        SetupPasses();
        rd->FX_ResetPipe();
        rd->EF_Scissor(false, 0, 0, 0, 0);
        rd->SetDepthBoundTest(0.0f, 1.0f, false);

        {
            PROFILE_LABEL_SCOPE("SHADOWMAP_POOL");

            if (!bPreLoop && !isGmemEnabled)
            {
                ResolveCurrentBuffers();
            }

            while (m_nCurrentShadowPoolLight < arrLights.Num())
            {
                SRenderLight& rLight = arrLights[m_nCurrentShadowPoolLight];

                if (packShadows && !(rLight.m_Flags & (DLF_DIRECTIONAL | DLF_FAKE)) && (rLight.m_Flags & DLF_CASTSHADOW_MAPS))
                {
                    bool bPacked = PackToPool(m_blockPack, rLight, m_bClearPool);
                    m_bClearPool = !bPacked;
                    if (!bPacked)
                    {
                        break;
                    }
                }
                ++m_nCurrentShadowPoolLight;
                bShadowRendered = true;
            }

#ifndef _RELEASE
            {
                uint32 nAllocs = m_shadowPoolAlloc.Num();
                for (uint32 i = 0; i < nAllocs; i++)
                {
                    if (!m_shadowPoolAlloc[i].isFree())
                    {
                        rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumShadowPoolFrustums++;
                    }
                }
            }
#endif
        }

        if (!bPreLoop && bShadowRendered)
        {
            if (!isGmemEnabled)
            {
                RestoreCurrentBuffers();
            }

            size_t numLightsWithoutShadow = 0;
            //insert light pass here
            for (uint nLightPacked = m_nFirstCandidateShadowPoolLight; nLightPacked < m_nCurrentShadowPoolLight; ++nLightPacked)
            {
                SRenderLight& rLight = arrLights[nLightPacked];
                if ((rLight.m_Flags & (DLF_FAKE | DLF_DIRECTIONAL)) || !(rLight.m_Flags & DLF_CASTSHADOW_MAPS))
                {
                    continue;
                }

                if (packShadows)
                {
                    ShadowLightPasses(rLight);
                }
                else
                {
                    // We are not allow to pack shadows (like in GMEM during the ligthing pass) so this light doesn't have a shadowmap.
                    // Remove the cast shadow flag so it get's render without shadows later.
                    rLight.m_Flags &= ~DLF_CASTSHADOW_MAPS;
                    ++numLightsWithoutShadow;
                }
            }

            AZ_Warning("Rendering", numLightsWithoutShadow == 0, "%d lights will be rendered without shadows because there's no more space in the shadowmap pool texture.\
                Try decreasing the number of lights casting shadows or increasing the size of the shadowmap pool (e_ShadowsPoolSize)",
                numLightsWithoutShadow);
        }
    }

    rd->m_RP.m_FlagsShader_RT = nPrevFlagsShaderRT;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CDeferredShading::PackToPool(CPowerOf2BlockPacker* pBlockPack, SRenderLight& light, bool bClearPool)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    //////////////////////////////////////////////////////////////////////////
    int nDLights = rd->m_RP.m_DLights[m_nThreadID][m_nRecurseLevel].Num();

    //////////////////////////////////////////////////////////////////////////
    int nFrustumIdx = light.m_lightId + nDLights;
    int nStartIdx = SRendItem::m_StartFrust[m_nThreadID][nFrustumIdx];
    int nEndIdx = SRendItem::m_EndFrust[m_nThreadID][nFrustumIdx];

    int nUpdatedThisFrame = 0;

    assert((unsigned int) nFrustumIdx < (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS));
    if ((unsigned int) nFrustumIdx >= (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS))
    {
        int nFrameID = gcpRendD3D->GetFrameID(false);
        if (m_nWarningFrame != nFrameID)
        {
            Warning("CDeferredShading::ShadowLightPasses: Too many light sources used ...");
            m_nWarningFrame = nFrameID;
        }
        //reset castshadow flag for further processing
        light.m_Flags &= ~DLF_CASTSHADOW_MAPS;
        return true;
    }
    //////////////////////////////////////////////////////////////////////////

    //no single frustum was allocated for this light
    if (nEndIdx <= nStartIdx)
    {
        //reset castshadow flag for further processing
        light.m_Flags &= ~DLF_CASTSHADOW_MAPS;
        return true;
    }

    if (m_nRecurseLevel < 0 || m_nRecurseLevel >= (int)rd->m_RP.m_SMFrustums[m_nThreadID]->Num())
    {
        //reset castshadow flag for further processing
        light.m_Flags &= ~DLF_CASTSHADOW_MAPS;
        return true;
    }

    ShadowMapFrustum& firstFrustum = rd->m_RP.m_SMFrustums[m_nThreadID][m_nRecurseLevel][nStartIdx];

    //////////////////////////////////////////////////////////////////////////
    int nBlockW = firstFrustum.nTexSize >> TEX_POOL_BLOCKLOGSIZE;
    int nLogBlockW = IntegerLog2((uint32)nBlockW);
    int nLogBlockH = nLogBlockW;

    bool bNeedsUpdate = false;

    if (bClearPool)
    {
        pBlockPack->Clear();
        m_shadowPoolAlloc.SetUse(0);
    }

    uint32 currFrame = gcpRendD3D->GetFrameID(false) & 0xFF;

    uint32 lightID = light.m_nEntityId;

    assert(lightID != (uint32) - 1);

    uint SidesNum = firstFrustum.bUnwrapedOmniDirectional ? OMNI_SIDES_NUM : 1;
    uint nUpdateMask = firstFrustum.bUnwrapedOmniDirectional ? 0x3F : 0x1;

    for (uint nSide = 0; nSide < SidesNum; nSide++)
    {
        bNeedsUpdate = false;
        uint nX1, nX2, nY1, nY2;

        //Find block allocation info (alternative: store in frustum data, but this does not persist)
        bool bFoundAlloc = false;
#if defined(WIN32)
        int  nMGPUUpdate = -1;
#endif
        uint32 nAllocs = m_shadowPoolAlloc.Num();
        SShadowAllocData* pAlloc = NULL;
        for (uint32 i = 0; i < nAllocs; i++)
        {
            pAlloc = &m_shadowPoolAlloc[i];

            if (pAlloc->m_lightID == lightID  && pAlloc->m_side == nSide)
            {
                bFoundAlloc = true;
                break;
            }
        }

        if (bFoundAlloc)
        {
            uint32 nID = pBlockPack->GetBlockInfo(pAlloc->m_blockID, nX1, nY1, nX2, nY2);

            int nFrameCompare = (currFrame - pAlloc->m_frameID) % 256;

            if (nID == 0xFFFFFFFF)
            {
                bNeedsUpdate =  true;
            }
            else
            {
                if (firstFrustum.nShadowPoolUpdateRate == 0)  // forced update, always do this
                {
                    bNeedsUpdate = true;
                }
                else if (firstFrustum.nShadowPoolUpdateRate < (uint8) nFrameCompare)
                {
                    if (nUpdatedThisFrame < CRenderer::CV_r_ShadowPoolMaxTimeslicedUpdatesPerFrame)
                    {
                        bNeedsUpdate = true;
                        nUpdatedThisFrame++;
                    }
                }
#if defined(WIN32) // AFR support
                else if (rd->GetActiveGPUCount() > 1)
                {
                    if (gRenDev->GetActiveGPUCount() > nFrameCompare)
                    {
                        bNeedsUpdate = true;
                        nMGPUUpdate = pAlloc->m_frameID;
                    }
                }
#endif
            }

            if (!bNeedsUpdate)
            {
                if (nX1 != 0xFFFFFFFF &&  nBlockW == (nX2 - nX1))  // ignore Y, is square
                {
                    pBlockPack->GetBlockInfo(nID, nX1, nY1, nX2, nY2);
                    firstFrustum.packX[nSide] = nX1 << TEX_POOL_BLOCKLOGSIZE;
                    firstFrustum.packY[nSide] = nY1 << TEX_POOL_BLOCKLOGSIZE;
                    firstFrustum.packWidth[nSide]  = (nX2 - nX1) << TEX_POOL_BLOCKLOGSIZE;
                    firstFrustum.packHeight[nSide] = (nY2 - nY1) << TEX_POOL_BLOCKLOGSIZE;
                    firstFrustum.nShadowGenID[m_nThreadID][nSide] = 0xFFFFFFFF; // turn off shadow gen for this side

                    nUpdateMask &= ~(1 << nSide);
                    continue; // All currently valid, skip
                }
            }

            if (nID != 0xFFFFFFFF && nX1 != 0xFFFFFFFF) // Valid block, realloc
            {
                pBlockPack->RemoveBlock(nID);
                pAlloc->Clear();
            }
        }

        uint32 nID = pBlockPack->AddBlock(nLogBlockW, nLogBlockH);
        bool isAllocated = (nID != 0xFFFFFFFF) ? true : false;

#ifndef _RELEASE
        rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumShadowPoolAllocsThisFrame++;
#endif

        if (isAllocated)
        {
            bNeedsUpdate = true;

            if (!bFoundAlloc)
            {
                pAlloc = NULL;
                nAllocs = m_shadowPoolAlloc.Num();
                for (uint32 i = 0; i < nAllocs; i++)
                {
                    if (m_shadowPoolAlloc[i].isFree())
                    {
                        pAlloc = &m_shadowPoolAlloc[i];
                        break;
                    }
                }

                if (!pAlloc)
                {
                    pAlloc = m_shadowPoolAlloc.AddIndex(1);
                }
            }

            pAlloc->m_blockID = nID;
            pAlloc->m_lightID = lightID;
            pAlloc->m_side = nSide;
#if defined(WIN32)
            pAlloc->m_frameID = (nMGPUUpdate == -1) ? gcpRendD3D->GetFrameID(false) & 0xFF : nMGPUUpdate;
#else
            pAlloc->m_frameID = gcpRendD3D->GetFrameID(false) & 0xFF;
#endif
            bClearPool = true;
        }
        else
        {
#ifndef _RELEASE // failed alloc, will thrash!
            if (CRenderer::CV_r_ShadowPoolMaxFrames != 0 || CRenderer::CV_r_DeferredShadingTiled > 1)
            {
                rd->m_RP.m_PS[rd->m_RP.m_nProcessThreadID].m_NumShadowPoolAllocsThisFrame |= 0x80000000;
            }
#endif

            return false;
        }

        pBlockPack->GetBlockInfo(nID, nX1, nY1, nX2, nY2);
        firstFrustum.packX[nSide] = nX1 << TEX_POOL_BLOCKLOGSIZE;
        firstFrustum.packY[nSide] = nY1 << TEX_POOL_BLOCKLOGSIZE;
        firstFrustum.packWidth[nSide]  = (nX2 - nX1) << TEX_POOL_BLOCKLOGSIZE;
        firstFrustum.packHeight[nSide] = (nY2 - nY1) << TEX_POOL_BLOCKLOGSIZE;
    }

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //  next step is to use these values in shadowgen

    //////////////////////////////////////////////////////////////////////////

    if (firstFrustum.bUseShadowsPool && nUpdateMask > 0)
    {
        rd->FX_PrepareDepthMapsForLight(light, nFrustumIdx, bClearPool);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::ResolveCurrentBuffers()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    PROFILE_LABEL_SCOPE("FLUSH_RESOLVE");
    rd->FX_PopRenderTarget(1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::RestoreCurrentBuffers()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    rd->FX_PushRenderTarget(1, m_pLBufferSpecularRT, NULL, -1, false, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CDeferredShading::ShadowLightPasses(const SRenderLight& light)
{
    PROFILE_SHADER_SCOPE;

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    float scissorInt2Float[] = { (float)(light.m_sX), (float)(light.m_sY), (float)(light.m_sWidth), (float)(light.m_sHeight) };

    rd->m_RP.m_nDeferredPrimitiveID = SHAPE_PROJECTOR;
    int nDLights = rd->m_RP.m_DLights[m_nThreadID][m_nRecurseLevel].Num();

    //////////////////////////////////////////////////////////////////////////
    int nFrustumIdx = light.m_lightId + nDLights;
    int nStartIdx = SRendItem::m_StartFrust[m_nThreadID][nFrustumIdx];
    int nEndIdx = SRendItem::m_EndFrust[m_nThreadID][nFrustumIdx];


    assert((unsigned int) nFrustumIdx < (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS));
    if ((unsigned int) nFrustumIdx >= (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS))
    {
        //Warning("CDeferredShading::ShadowLightPasses: Too many light sources used ..."); // redundant
        return false;
    }

    //no single frustum was allocated for this light
    if (nEndIdx <= nStartIdx)
    {
        return false;
    }

    if (m_nRecurseLevel < 0 || m_nRecurseLevel >= (int)rd->m_RP.m_SMFrustums[m_nThreadID]->Num())
    {
        return false;
    }

    // Area lights are a non-uniform box, not a cone in 1 of 6 directions, so we skip clipping/stencil testing and let the light pass take care of it.
    bool bAreaLight = (light.m_Flags & DLF_AREA_LIGHT) && light.m_fAreaWidth && light.m_fAreaHeight && light.m_fLightFrustumAngle && CRenderer::CV_r_DeferredShadingAreaLights;

    ShadowMapFrustum& firstFrustum = rd->m_RP.m_SMFrustums[m_nThreadID][m_nRecurseLevel][nStartIdx];

    int nSides = 1;
    if (firstFrustum.bOmniDirectionalShadow && !bAreaLight)
    {
        nSides = 6;
    }

    // omni lights with clip bounds require two stencil tests (one for the side and one for the clip bound)
    const int stencilValuesPerSide = 1;

    //enable shadow mapping
    rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];

    //enable hw-pcf per frustum
    if (firstFrustum.bHWPCFCompare)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];
    }

    Vec4 pLightRect  = Vec4(scissorInt2Float[0], scissorInt2Float[1], scissorInt2Float[2], scissorInt2Float[3]);
    Vec4 scaledLightRect = Vec4(pLightRect.x * rd->m_RP.m_CurDownscaleFactor.x, pLightRect.y * rd->m_RP.m_CurDownscaleFactor.y,
            pLightRect.z * rd->m_RP.m_CurDownscaleFactor.x, pLightRect.w * rd->m_RP.m_CurDownscaleFactor.y);

    if (!bAreaLight)
    {
        rd->m_nStencilMaskRef += (nSides * stencilValuesPerSide + 2);
        if (rd->m_nStencilMaskRef > STENC_MAX_REF)
        {
            int sX = 0, sY = 0, sWidth = 0, sHeight = 0;
            bool bScissorEnabled = rd->EF_GetScissorState(sX, sY, sWidth, sHeight);
            rd->EF_Scissor(false, 0, 0, 0, 0);

            if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
            {
                // Avoid any resolve. We clear stencil with full screen pass.
                uint32 prevState = rd->m_RP.m_CurState;
                uint32 newState = 0;
                newState |= GS_COLMASK_NONE;
                newState |= GS_STENCIL;
                rd->FX_SetStencilState(STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
                    STENCOP_FAIL(FSS_STENCOP_ZERO) |
                    STENCOP_ZFAIL(FSS_STENCOP_ZERO) |
                    STENCOP_PASS(FSS_STENCOP_ZERO),
                    0, 0xFFFFFFFF, 0xFFFF);
                rd->FX_SetState(newState);
                SD3DPostEffectsUtils::ClearScreen(0, 0, 0, 0);
                rd->FX_SetState(prevState);
            }
            else
            {
                rd->EF_ClearTargetsImmediately(FRT_CLEAR_STENCIL);
            }

            rd->EF_Scissor(bScissorEnabled, sX, sY, sWidth, sHeight);
            rd->m_nStencilMaskRef = nSides * stencilValuesPerSide + 1;
        }
    }

    uint16 scaledX = 0, scaledY = 0, scaledWidth = 0, scaledHeight = 0;

    for (int nS = 0; nS < nSides; nS++)
    {
        uint32 nPersFlagsPrev = rd->m_RP.m_TI[m_nThreadID].m_PersFlags;

        bool bIsMirrored =  (rd->m_RP.m_TI[m_nThreadID].m_PersFlags & RBPF_MIRRORCULL)  ? true : false;
        bool bRequiresMirroring = (!(light.m_Flags & (DLF_PROJECT | DLF_AREA_LIGHT))) ? true : false;

        if (bIsMirrored ^ bRequiresMirroring) // Enable mirror culling for omni-shadows, or if we are in cubemap-gen. If both, they cancel-out, so disable.
        {
            rd->m_RP.m_TI[m_nThreadID].m_PersFlags |= RBPF_MIRRORCULL;
        }
        else
        {
            rd->m_RP.m_TI[m_nThreadID].m_PersFlags &= ~RBPF_MIRRORCULL;
        }

#if !defined(CRY_USE_METAL) && !defined(ANDROID)
        SpecularAccEnableMRT(false);
#endif

        if (CRenderer::CV_r_DeferredShadingDepthBoundsTest == 1 && !bAreaLight)
        {
            Vec4 pDepthBounds = GetLightDepthBounds(&light, (rd->m_RP.m_TI[m_nThreadID].m_PersFlags & RBPF_REVERSE_DEPTH) != 0);
            rd->SetDepthBoundTest(pDepthBounds.x, pDepthBounds.z, true);
        }

        if (nS == 0)
        {
            scaledX = (uint16)(scaledLightRect.x);
            scaledY = (uint16)(scaledLightRect.y);
            scaledWidth = (uint16)(scaledLightRect.z) + 1;
            scaledHeight = (uint16)(scaledLightRect.w) + 1;
        }

        if (!bAreaLight)
        {
            //use current WorldProj matrix
            rd->FX_StencilFrustumCull(-2, &light, &firstFrustum, nS);

            rd->SetDepthBoundTest(0, 1, false);
        }

        rd->m_RP.m_TI[m_nThreadID].m_PersFlags = nPersFlagsPrev;
        if (!bAreaLight)
        {
            rd->FX_StencilTestCurRef(true, true);
        }

        SetupPasses();

        if (!bAreaLight)
        {
            m_nRenderState |= GS_STENCIL;
        }

#if !defined(CRY_USE_METAL) && !defined(ANDROID)
        SpecularAccEnableMRT(true);
#endif

        if (firstFrustum.nShadowGenMask & (1 << nS))
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
            rd->ConfigShadowTexgen(0, &firstFrustum, nS);

            if (firstFrustum.bUseShadowsPool)
            {
                const int nTexFilter = firstFrustum.bHWPCFCompare ? FILTER_LINEAR : FILTER_POINT;
                STexState TS;
                TS.SetFilterMode(nTexFilter);
                TS.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);
                TS.m_bSRGBLookup = false;
                TS.SetComparisonFilter(true);
                CTexture::s_ptexRT_ShadowPool->Apply(3, CTexture::GetTexState(TS), EFTT_UNKNOWN, 6);
                //  this assigned comparison sampler to correct sampler slot for shadowmapped light sources
                if (!rd->UseHalfFloatRenderTargets())
                {
                    CTexture::SetSamplerState(CTexture::GetTexState(TS), 0, eHWSC_Pixel);
                }
            }
            else
            {
                SD3DPostEffectsUtils::SetTexture(firstFrustum.pDepthTex, 3, FILTER_POINT, 0);
            }

            SD3DPostEffectsUtils::SetTexture(CTextureManager::Instance()->GetDefaultTexture("ShadowJitterMap"), 7, FILTER_POINT, 0);
        }
        else
        {
            rd->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE4];
        }

        //////////////////////////////////////////////////////////////////////////


        m_nCurLighID = light.m_lightId;

        LightPass(&light, true);

        if (!bAreaLight)
        {
            rd->FX_StencilTestCurRef(false);
        }
    }

    //assign range
    if (!bAreaLight)
    {
        rd->m_nStencilMaskRef += nSides * stencilValuesPerSide;
    }
    //////////////////////////////////////////////////////////////////////////
    rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ]);

    if (!bAreaLight)
    {
        m_nRenderState &= ~GS_STENCIL;
    }
    rd->FX_SetState(m_nRenderState);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::CreateDeferredMaps()
{
    AZ_TRACE_METHOD();
    static uint32 nPrevLBuffersFmt = CRenderer::CV_r_DeferredShadingLBuffersFmt;
    if (!CTexture::s_ptexSceneNormalsMap || CTexture::s_ptexSceneNormalsMap->IsMSAAChanged()
        || CTexture::s_ptexSceneNormalsMap->GetWidth() != gcpRendD3D->m_MainViewport.nWidth
        || CTexture::s_ptexSceneNormalsMap->GetHeight() != gcpRendD3D->m_MainViewport.nHeight
        || nPrevLBuffersFmt != CRenderer::CV_r_DeferredShadingLBuffersFmt)
    {
        nPrevLBuffersFmt = CRenderer::CV_r_DeferredShadingLBuffersFmt;

        int nWidth = gcpRendD3D->GetWidth();
        int nHeight = gcpRendD3D->GetHeight();
        int nMSAAUsageFlag = ((CRenderer::CV_r_msaa ? FT_USAGE_MSAA : 0));
        int nMsaaAndSrgbFlag = nMSAAUsageFlag;

        if (RenderCapabilities::SupportsTextureViews())
        {
            // Currently android nor mac(GL) support srgb render targets so only add this
            // flag for other platforms
            nMsaaAndSrgbFlag |= FT_USAGE_ALLOWREADSRGB;
        }
   
        // This texture is reused for SMAA...
        // grab format from backbuffer - normals map doubles as a previous backbuffer target elsewhere, so it has to be the same type as the backbuffer.
        ETEX_Format fmt = CTexture::s_ptexBackBuffer->GetDstFormat();
        SD3DPostEffectsUtils::CreateRenderTarget("$SceneNormalsMap", CTexture::s_ptexSceneNormalsMap, nWidth, nHeight, Clr_Unknown, true, false, fmt, TO_SCENE_NORMALMAP, nMsaaAndSrgbFlag);
        SD3DPostEffectsUtils::CreateRenderTarget("$SceneNormalsBent", CTexture::s_ptexSceneNormalsBent, nWidth, nHeight, Clr_Median, true, false, eTF_R8G8B8A8);
        SD3DPostEffectsUtils::CreateRenderTarget("$AOColorBleed", CTexture::s_ptexAOColorBleed, nWidth >> 3, nHeight >> 3, Clr_Unknown, true, false, eTF_R8G8B8A8);

        ETEX_Format sceneDiffuseAccTexFormat  = eTF_R16G16B16A16F;
        ETEX_Format sceneSpecularAccTexFormat = eTF_R16G16B16A16F;

#if defined(OPENGL_ES) // might be no fp rendering support
        if (!gcpRendD3D->UseHalfFloatRenderTargets())
        {
            sceneSpecularAccTexFormat = eTF_R10G10B10A2;
            sceneDiffuseAccTexFormat = gcpRendD3D->FX_GetEnabledGmemPath(nullptr) ? eTF_R16G16B16A16 : eTF_R10G10B10A2;
        }
#elif defined(WIN32) || defined(APPLE) || defined(LINUX) || defined(SUPPORTS_DEFERRED_SHADING_L_BUFFERS_FORMAT)
        if (CRenderer::CV_r_DeferredShadingLBuffersFmt == 1)
        {
            sceneSpecularAccTexFormat = eTF_R11G11B10F;
            sceneDiffuseAccTexFormat = gcpRendD3D->FX_GetEnabledGmemPath(nullptr) ? eTF_R16G16B16A16F : eTF_R11G11B10F;
        }
#endif
        
        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingLBuffersFmt == 2 && gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            sceneDiffuseAccTexFormat = eTF_R8;
            sceneSpecularAccTexFormat = eTF_R11G11B10F;
#if defined(OPENGL_ES)
            if (!gcpRendD3D->UseHalfFloatRenderTargets())
            {
                sceneSpecularAccTexFormat = eTF_R10G10B10A2;
            }
#endif
        }
     
        SD3DPostEffectsUtils::CreateRenderTarget("$SceneDiffuseAcc", CTexture::s_ptexSceneDiffuseAccMap, nWidth, nHeight, Clr_Transparent, true, false,
            // In GMEM Paths:
            // - Alpha channel is used for shadow mask
            // - Used as a tmp buffer to hold normals while computing deferred decals
            sceneDiffuseAccTexFormat,
            TO_SCENE_DIFFUSE_ACC, nMSAAUsageFlag);

        CTexture::s_ptexCurrentSceneDiffuseAccMap = CTexture::s_ptexSceneDiffuseAccMap;
        
        // When the device orientation changes on mobile, we need to regenerate HDR maps before calling CreateRenderTarget.
        // Otherwise, the width and height of the texture get updated before HDRPostProcess::Begin call and we never regenerate the HDR maps which results in visual artifacts.
        if (CTexture::s_ptexHDRTarget && (CTexture::s_ptexHDRTarget->IsMSAAChanged() || CTexture::s_ptexHDRTarget->GetWidth() != gcpRendD3D->GetWidth() || CTexture::s_ptexHDRTarget->GetHeight() != gcpRendD3D->GetHeight()))
        {
            CTexture::GenerateHDRMaps();
        }

        SD3DPostEffectsUtils::CreateRenderTarget("$SceneSpecularAcc", CTexture::s_ptexSceneSpecularAccMap, nWidth, nHeight, Clr_Transparent, true, false, sceneSpecularAccTexFormat, TO_SCENE_SPECULAR_ACC, nMSAAUsageFlag);

        if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            // Point s_ptexHDRTarget to s_ptexSceneSpecularAccMap for GMEM paths
            CTexture::s_ptexHDRTarget = CTexture::s_ptexSceneSpecularAccMap;

            // Point m_pResolvedStencilRT to s_ptexGmemStenLinDepth for GMEM paths
            m_pResolvedStencilRT = CTexture::s_ptexGmemStenLinDepth;
        }

        SD3DPostEffectsUtils::CreateRenderTarget("$SceneDiffuse", CTexture::s_ptexSceneDiffuse, nWidth, nHeight, Clr_Empty, true, false, eTF_R8G8B8A8, -1, nMsaaAndSrgbFlag);
        
        // Slimming of GBuffer requires only one channel for specular due to packing of RGB values into YPbPr and
        // specular components into less channels
        ETEX_Format rtTextureFormat = eTF_R8G8B8A8;
        if (CRenderer::CV_r_SlimGBuffer == 1)
        {
            rtTextureFormat = eTF_R8;
        }
        SD3DPostEffectsUtils::CreateRenderTarget("$SceneSpecular", CTexture::s_ptexSceneSpecular, nWidth, nHeight, Clr_Empty, true, false, rtTextureFormat, -1, nMsaaAndSrgbFlag);
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DDEFERREDSHADING_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(D3DDeferredShading_cpp)
#endif

        ETEX_Format fmtZScaled = gcpRendD3D->UseHalfFloatRenderTargets() ? eTF_R16G16F : eTF_R16G16U;
        SD3DPostEffectsUtils::CreateRenderTarget("$ZTargetScaled", CTexture::s_ptexZTargetScaled, nWidth >> 1, nHeight >> 1, Clr_FarPlane, 1, 0, fmtZScaled, TO_DOWNSCALED_ZTARGET_FOR_AO);
        SD3DPostEffectsUtils::CreateRenderTarget("$ZTargetScaled2", CTexture::s_ptexZTargetScaled2, nWidth >> 2, nHeight >> 2, Clr_FarPlane, 1, 0, fmtZScaled, TO_QUARTER_ZTARGET_FOR_AO);

        SD3DPostEffectsUtils::CreateRenderTarget("$AmbientLookup", CTexture::s_ptexAmbientLookup, 64, 1, Clr_Empty, true, false, eTF_R8G8B8A8, -1, FT_DONT_RELEASE);
        SD3DPostEffectsUtils::CreateRenderTarget("$DepthBufferQuarter", CTexture::s_ptexDepthBufferQuarter, nWidth >> 2, nHeight >> 2, Clr_FarPlane, false, false, eTF_D32F, -1, FT_USAGE_DEPTHSTENCIL);
    }

    // Pre-create shadow pool
    IF (gcpRendD3D->m_pRT->IsRenderThread() && gEnv->p3DEngine, 1)
    {
        //init shadow pool size
        static ICVar* p_e_ShadowsPoolSize = iConsole->GetCVar("e_ShadowsPoolSize");
        gcpRendD3D->m_nShadowPoolHeight = p_e_ShadowsPoolSize->GetIVal();
        gcpRendD3D->m_nShadowPoolWidth = gcpRendD3D->m_nShadowPoolHeight; //square atlas

        ETEX_Format eShadTF = gcpRendD3D->CV_r_shadowtexformat == 1 ? eTF_D16 : eTF_D32F;
        CTexture::s_ptexRT_ShadowPool->Invalidate(gcpRendD3D->m_nShadowPoolWidth, gcpRendD3D->m_nShadowPoolHeight, eShadTF);
        if (!CTexture::IsTextureExist(CTexture::s_ptexRT_ShadowPool))
        {
            CTexture::s_ptexRT_ShadowPool->CreateRenderTarget(eTF_Unknown, Clr_FarPlane);
        }

        CTexture::s_ptexRT_ShadowStub->Invalidate(1, 1, eShadTF);
        if (!CTexture::IsTextureExist(CTexture::s_ptexRT_ShadowStub))
        {
            CTexture::s_ptexRT_ShadowStub->CreateRenderTarget(eTF_Unknown, Clr_FarPlane);
        }
    }

    if (CRenderer::CV_r_DeferredShadingTiled > 0)
    {
        gcpRendD3D->GetTiledShading().CreateResources();
    }

    gcpRendD3D->GetVolumetricFog().CreateResources();

    // shadow mask
    {
        if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr) && !RenderCapabilities::SupportsPLSExtension())
        {
            // Gmem only supports one shadow mask texture and it's saved on the alpha channel of the diffuse light acc texture.
            CTexture::s_ptexShadowMask = CTexture::s_ptexCurrentSceneDiffuseAccMap;
        }
        else
        {
            if (CTexture::s_ptexShadowMask)
            {
                CTexture::s_ptexShadowMask->Invalidate(gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight(), eTF_R8G8B8A8);
            }

            if (!CTexture::IsTextureExist(CTexture::s_ptexShadowMask))
            {
#if defined(CRY_USE_METAL)
                const int nArraySize = 1; // iOS currently only supports one shadow mask texture
#else
                const int nArraySize = (gcpRendD3D->CV_r_ShadowCastingLightsMaxCount + 3) / 4;
#endif
                CTexture::s_ptexShadowMask = CTexture::CreateTextureArray("$ShadowMask", eTT_2D, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight(), nArraySize, 1, FT_DONT_STREAM | FT_USAGE_RENDERTARGET, eTF_R8G8B8A8, TO_SHADOWMASK);
            }
        }
    }

    // height map AO mask
    if (CRenderer::CV_r_HeightMapAO > 0)
    {
        const int nShift = clamp_tpl(3 - CRenderer::CV_r_HeightMapAO, 0, 2);
        const int hmaoWidth = gcpRendD3D->GetWidth() >> nShift;
        const int hmaoHeight =  gcpRendD3D->GetHeight() >> nShift;

        for (int i = 0; i < 2; ++i)
        {
            if (CTexture::s_ptexHeightMapAO[i])
            {
                CTexture::s_ptexHeightMapAO[i]->Invalidate(hmaoWidth, hmaoHeight, eTF_R8G8);
            }

            if (!CTexture::IsTextureExist(CTexture::s_ptexHeightMapAO[i]))
            {
                char buf[128];
                sprintf_s(buf, "$HeightMapAO_%d", i);

                SD3DPostEffectsUtils::CreateRenderTarget(buf, CTexture::s_ptexHeightMapAO[i], hmaoWidth,  hmaoHeight, Clr_Neutral, true, false, eTF_R8G8);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DestroyDeferredMaps()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

struct CubemapsCompare
{
    bool operator()(const SRenderLight& l0, const SRenderLight& l1) const
    {
        // Cubes sort by: Sort priority first, light radius, lastly by entity id (insertion order every frame is not guaranteed)
        if (l0.m_nSortPriority != l1.m_nSortPriority)
        {
            return l0.m_nSortPriority < l1.m_nSortPriority;
        }

        if (fcmp(l0.m_fRadius, l1.m_fRadius))
        {
            return l0.m_nEntityId < l1.m_nEntityId;
        }

        return l0.m_fRadius > l1.m_fRadius;
    }
};

struct CubemapsCompareInv
{
    bool operator()(const SRenderLight& l0, const SRenderLight& l1) const
    {
        // Cubes sort by: Sort priority first, light radius, lastly by entity id (insertion order every frame is not guaranteed)
        if (l0.m_nSortPriority != l1.m_nSortPriority)
        {
            return l0.m_nSortPriority > l1.m_nSortPriority;
        }

        if (fcmp(l0.m_fRadius, l1.m_fRadius))
        {
            return l0.m_nEntityId > l1.m_nEntityId;
        }

        return l0.m_fRadius < l1.m_fRadius;
    }
};

struct LightsCompare
{
    bool operator()(const SRenderLight& l0, const SRenderLight& l1) const
    {
        if (!(l0.m_Flags & DLF_CASTSHADOW_MAPS) && (l1.m_Flags & DLF_CASTSHADOW_MAPS))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

struct DeffDecalSort
{
    bool operator()(const SDeferredDecal& decal0, const SDeferredDecal& decal1) const
    {
        uint bBump0 = (decal0.nFlags & DECAL_HAS_NORMAL_MAP);
        uint bBump1 = (decal1.nFlags & DECAL_HAS_NORMAL_MAP);
        //bump-mapped decals first
        if (bBump0 !=  bBump1)
        {
            return (bBump0 < bBump1);
        }
        return
            (decal0.nSortOrder < decal1.nSortOrder);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void CDeferredShading::SetupGmemPath()
{
#if !defined(NDEBUG)
    bool bTiledDeferredShading = CRenderer::CV_r_DeferredShadingTiled >= 2;
    assert(!bTiledDeferredShading); // NOT SUPPORTED IN GMEM PATH!
#endif

    SetupPasses();
    TArray<SRenderLight>& rDeferredLights = m_pLights[eDLT_DeferredLight][m_nThreadID][m_nRecurseLevel];

    m_bClearPool |= CRenderer::CV_r_ShadowPoolMaxFrames == 0;

    m_nCurrentShadowPoolLight = 0;
    m_nFirstCandidateShadowPoolLight = 0;

#if !defined(_RELEASE)
    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    rd->m_RP.m_PS[m_nThreadID].m_NumShadowPoolFrustums = 0;
    rd->m_RP.m_PS[m_nThreadID].m_NumShadowPoolAllocsThisFrame = 0;
    rd->m_RP.m_PS[m_nThreadID].m_NumShadowMaskChannels = 0;
#endif

    SortLigths(rDeferredLights);
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingLights)
    {
        PackAllShadowFrustums(rDeferredLights, true);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::AmbientOcclusionPasses()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if (CRenderer::CV_r_DeferredShadingTiled >= 2)
    {
        return;
    }

    PROFILE_LABEL_SCOPE("AO_APPLY");

    SpecularAccEnableMRT(false);

    rd->EF_Scissor(false, 0, 0, 0, 0);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.f, DBT_SKY_CULL_DEPTH, true); // skip sky and near objects for SSAO/LPVs
    }
    SpecularAccEnableMRT(true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DebugShadowMaskClear()
{
    // This function is only useful when shadows get turned off at run-time.
    // TODO: should only clear once when shadows get turned off... not every frame!
    // For GMEM, we by-pass this function completely as the RT has don't care actions set.
    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        return;
    }

#ifndef _RELEASE
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    static const ICVar* pCVarShadows = iConsole->GetCVar("e_Shadows");
    if ((pCVarShadows && pCVarShadows->GetIVal() == 0) || CRenderer::CV_r_ShadowPass == 0)
    {
        rd->FX_ClearTarget(CTexture::s_ptexBackBuffer, Clr_Transparent);
    }
#endif
}

void CDeferredShading::SortLigths(TArray<SRenderLight>& ligths) const
{
    if (CRenderer::CV_r_DeferredShadingSortLights <= 0 || ligths.size() <= 1)
    {
        return;
    }

    int swapPos = -1;
    if (CRenderer::CV_r_DeferredShadingSortLights == 2 ||
        CRenderer::CV_r_DeferredShadingSortLights == 3)
    {
        // Sort the lights so we process the ones that are packed first.
        // This reduce the probability of trashing the shadowmap.
        for (size_t i = 0; i < ligths.Num(); ++i)
        {
            const SRenderLight& light = ligths[i];
            bool isPacked = m_shadowPoolAlloc.end() != AZStd::find_if(m_shadowPoolAlloc.begin(), m_shadowPoolAlloc.end(), [&light](const SShadowAllocData& data) {return data.m_lightID == light.m_nEntityId; });
            if (isPacked)
            {
                ++swapPos;
                if (i != swapPos)
                {
                    AZStd::swap(ligths[i], ligths[swapPos]);
                }
            }
        }
    }

    if (CRenderer::CV_r_DeferredShadingSortLights == 1 ||
        CRenderer::CV_r_DeferredShadingSortLights == 3)
    {
        CD3D9Renderer* const __restrict rd = gcpRendD3D;
        auto sortFunc = [&rd](const SRenderLight& lhs, const SRenderLight& rhs)
        {
            // First compare by influence area in screen area (bigger area goes first)
            // If they have the same area then render the closest to the camera first
            // If they are at the same distance then just use the entityId (to break the tie)
            int lhsSize = lhs.m_sWidth * lhs.m_sHeight;
            int rhsSize = rhs.m_sWidth * rhs.m_sHeight;
            if (lhsSize == rhsSize)
            {
                Vec3 cameraPos = rd->GetCamera().GetPosition();
                float lhsDistance = cameraPos.GetDistance(lhs.GetPosition());
                float rhsDistance = cameraPos.GetDistance(rhs.GetPosition());
                return lhsDistance == rhsDistance ? lhs.m_nEntityId < rhs.m_nEntityId : lhsDistance < rhsDistance;
            }

            return lhsSize > rhsSize;
        };

        // Sort in two halfs so we don't break the previous order. 
        // The first half are the lights that are already packed (if r_DeferredShadingSortLights = 3).
        // The second half are the rest of the lights. 
        SRenderLight* firstHalfElement = swapPos < 0 ? ligths.end() : ligths.begin() + swapPos + 1;
        AZStd::sort(ligths.begin(), firstHalfElement, sortFunc);
        // Check if there's even a second half.
        if (firstHalfElement != ligths.end())
        {
            AZStd::sort(firstHalfElement, ligths.end(), sortFunc);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::Render()
{
    m_nLightsProcessedCount = 0;

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    uint64 nFlagsShaderRT = rd->m_RP.m_FlagsShader_RT;

    rd->FX_ResetPipe();

    SetupPasses();

    // Calculate screenspace scissor bounds
    CalculateLightScissorBounds();

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
#ifndef _RELEASE
        rd->m_RP.m_PS[m_nThreadID].m_NumShadowPoolFrustums = 0;
        rd->m_RP.m_PS[m_nThreadID].m_NumShadowPoolAllocsThisFrame = 0;
        rd->m_RP.m_PS[m_nThreadID].m_NumShadowMaskChannels = 0;
#endif
    }

    TArray<SRenderLight>& rDeferredLights                       = m_pLights[eDLT_DeferredLight][m_nThreadID][m_nRecurseLevel];
    TArray<SRenderLight>& rDeferredCubemaps                 = m_pLights[eDLT_DeferredCubemap][m_nThreadID][m_nRecurseLevel];
    TArray<SRenderLight>& rDeferredAmbientLights        = m_pLights[eDLT_DeferredAmbientLight][m_nThreadID][m_nRecurseLevel];

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.f, DBT_SKY_CULL_DEPTH, true); // skip sky for ambient and deferred cubemaps
    }
    int iTempX, iTempY, iWidth, iHeight;
    rd->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    bool bOutdoorVisible = false;
    PrepareClipVolumeData(bOutdoorVisible);

    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        rd->FX_GmemTransition(CD3D9Renderer::eGT_POST_Z_PRE_DEFERRED);
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingScissor)
    {
        rd->EF_Scissor(false, 0, 0, 0, 0);
    }

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        SortLigths(rDeferredLights);
        FilterGBuffer();

        // Generate directional occlusion information
        DirectionalOcclusionPass();

        // Generate glossy screenspace reflections
        ScreenSpaceReflectionPass();

        m_bClearPool = (CRenderer::CV_r_ShadowPoolMaxFrames > 0) ? false : true;

        m_nCurrentShadowPoolLight = 0;
        m_nFirstCandidateShadowPoolLight = 0;

        PackAllShadowFrustums(rDeferredLights, true);

        rd->FX_PushRenderTarget(0, m_pLBufferDiffuseRT, &gcpRendD3D->m_DepthBufferOrigMSAA, -1, false, 1);
        rd->FX_PushRenderTarget(1, m_pLBufferSpecularRT, NULL, -1, false, 1);
        m_bSpecularState = true;
    }

    // sort lights
    if (rDeferredCubemaps.Num())
    {
        if (CRenderer::CV_r_DeferredShadingTiled <= 1)
        {
            std::sort(rDeferredCubemaps.begin(), rDeferredCubemaps.end(), CubemapsCompare());
        }
        else
        {
            std::sort(rDeferredCubemaps.begin(), rDeferredCubemaps.end(), CubemapsCompareInv());
        }
    }

    rd->FX_SetState(GS_BLSRC_ONE | GS_BLDST_ZERO);


    if (gRenDev->GetWireframeMode())
    {
        rd->FX_ClearTarget(m_pLBufferDiffuseRT);
        rd->FX_ClearTarget(m_pLBufferSpecularRT);
        rd->FX_ClearTarget(&gcpRendD3D->m_DepthBufferOrigMSAA, CLEAR_STENCIL, Clr_Unused.r, 1);
        rd->m_nStencilMaskRef = 1;// Stencil initialized to 1 - 0 is reserved for MSAAed samples
    }

    rd->RT_SetViewport(0, 0, gRenDev->GetWidth(), gRenDev->GetHeight());

    uint32 nCurrentDeferredCubemap = 0;

    bool bTiledDeferredShading = CRenderer::CV_r_DeferredShadingTiled >= 2;

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDebug == 2)
    {
        gcpRendD3D->m_RP.m_FlagsShader_RT |= RT_OVERDRAW_DEBUG;
        bTiledDeferredShading = false;
    }
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_Unlit)
    {
        bTiledDeferredShading = false;
    }

    // Currently cubemap atlas update is not working with OpenGL - remove when fixed
#if defined(OPENGL)
    bTiledDeferredShading = false;
#endif //defined(OPENGL)

    // determine if we have a global cubemap in the scene
    SRenderLight* pGlobalCubemap = NULL;
    AZ_PUSH_DISABLE_WARNING(, "-Wconstant-logical-operand")
    if (rDeferredCubemaps.Num() && CRenderer::CV_r_DeferredShadingEnvProbes)
    AZ_POP_DISABLE_WARNING
    {
        SRenderLight& pFirstLight = rDeferredCubemaps[0];
        ITexture* pDiffuseCubeCheck = pFirstLight.GetDiffuseCubemap();
        float fRadius = pFirstLight.m_fRadius;

        if (pDiffuseCubeCheck && fRadius >= 100000.f)
        {
            pGlobalCubemap = &pFirstLight;
            ++nCurrentDeferredCubemap;
        }
    }

    if (!bTiledDeferredShading)
    {
        if (CRenderer::CV_r_DeferredShadingAmbient && AmbientPass(pGlobalCubemap, bOutdoorVisible))
        {
            m_nLightsProcessedCount++;
        }

        DeferredCubemaps(rDeferredCubemaps, nCurrentDeferredCubemap);

        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingAmbientLights)
        {
            DeferredLights(rDeferredAmbientLights, false);
        }

        ApplySSReflections();  // TODO: Try to merge with another pass
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.0f, 1.0f, false);
    }

    if (CRenderer::CV_r_DeferredShadingLights && !bTiledDeferredShading)
    {
        DeferredLights(rDeferredLights, true);
    }

    // SSAO affects all light accumulation. Todo: batch into deferred shading pass.
    AmbientOcclusionPasses();

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingScissor)
    {
        rd->EF_Scissor(false, 0, 0, 0, 0);
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDepthBoundsTest)
    {
        rd->SetDepthBoundTest(0.0f, 1.0f, false);
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_Unlit)
    {
        rd->FX_ClearTarget(m_pLBufferDiffuseRT, Clr_MedianHalf);
        rd->FX_ClearTarget(m_pLBufferSpecularRT, Clr_Transparent);
        rd->FX_ClearTarget(&gcpRendD3D->m_DepthBufferOrigMSAA, CLEAR_STENCIL, Clr_Unused.r, 0);

    }

    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        // Commit any potential render target changes - required for deprecated platform resolves, do not remove this plz.
        rd->FX_SetActiveRenderTargets(false);

        rd->FX_PopRenderTarget(0);
        rd->FX_PopRenderTarget(1);
        m_bSpecularState = false;

        // Water volume caustics not supported in GMEM paths
        rd->FX_WaterVolumesCaustics();
    }

    if (bTiledDeferredShading)
    {
        gcpRendD3D->GetTiledShading().Render(rDeferredCubemaps, rDeferredAmbientLights, rDeferredLights, m_vClipVolumeParams);

        if (CRenderer::CV_r_DeferredShadingSSS)  // Explicitly disabling deferred SSS has its incompatible with msaa in current stage
        {
            DeferredSubsurfaceScattering(CTexture::s_ptexSceneTargetR11G11B10F[0]);
        }

        FurPasses::GetInstance().ExecuteObliteratePass();
    }
    else
    {
        // GPU light culling
        if (CRenderer::CV_r_DeferredShadingTiled == 1)
        {
            // Sort cubemaps in inverse order for tiled forward shading
            std::sort(rDeferredCubemaps.begin(), rDeferredCubemaps.end(), CubemapsCompareInv());

            gcpRendD3D->GetTiledShading().Render(rDeferredCubemaps, rDeferredAmbientLights, rDeferredLights, m_vClipVolumeParams);
        }

        DeferredShadingPass();
    }

    rd->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

#ifndef _RELEASE
    if (CRenderer::CV_r_DeferredShadingDebugGBuffer)
    {
        DebugGBuffer();
    }
#endif

    DebugShadowMaskClear();

    // Commit any potential render target changes - required for when shadows disabled
    rd->FX_SetActiveRenderTargets(false);

    rd->m_RP.m_FlagsShader_RT = nFlagsShaderRT;
    rd->m_RP.m_PersFlags2 &= ~RBPF2_WRITEMASK_RESERVED_STENCIL_BIT;

    rd->FX_ResetPipe();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::Release()
{
    DestroyDeferredMaps();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::GetScissors(const Vec3& vCenter, float fRadius, short& sX, short& sY, short& sWidth, short& sHeight) const
{
    Vec3 vViewVec = vCenter - gcpRendD3D->GetCamera().GetPosition();
    float fDistToLS =  vViewVec.GetLength();

    bool bInsideLightVolume = fDistToLS <= fRadius;
    if (bInsideLightVolume)
    {
        sX = sY = 0;
        sWidth = gcpRendD3D->GetWidth();
        sHeight = gcpRendD3D->GetHeight();
        return;
    }

    Matrix44 mProj, mView;
    gcpRendD3D->GetProjectionMatrix(mProj.GetData());
    gcpRendD3D->GetModelViewMatrix(mView.GetData());

    Vec3 pBRectVertices[4];
    Vec4 vCenterVS = Vec4(vCenter, 1) * mView;

    {
        // Compute tangent planes
        float r = fRadius;
        float sq_r = r * r;

        Vec3 vLPosVS = Vec3(vCenterVS.x, vCenterVS.y, vCenterVS.z);
        float lx = vLPosVS.x;
        float ly = vLPosVS.y;
        float lz = vLPosVS.z;
        float sq_lx = lx * lx;
        float sq_ly = ly * ly;
        float sq_lz = lz * lz;

        // Compute left and right tangent planes to light sphere
        float sqrt_d = sqrt_tpl(max(sq_r * sq_lx  - (sq_lx + sq_lz) * (sq_r - sq_lz), 0.0f));
        float nx = (r * lx + sqrt_d) / (sq_lx + sq_lz);
        float nz = iszero(lz) ? 1.0f : (r - nx * lx) / lz;

        Vec3 vTanLeft = Vec3(nx, 0, nz).normalized();

        nx = (r * lx - sqrt_d) / (sq_lx + sq_lz);
        nz = iszero(lz) ? 1.0f : (r - nx * lx) / lz;
        Vec3 vTanRight = Vec3(nx, 0, nz).normalized();

        pBRectVertices[0] = vLPosVS - r * vTanLeft;
        pBRectVertices[1] = vLPosVS - r * vTanRight;

        // Compute top and bottom tangent planes to light sphere
        sqrt_d = sqrt_tpl(max(sq_r * sq_ly  - (sq_ly + sq_lz) * (sq_r - sq_lz), 0.0f));
        float ny = (r * ly - sqrt_d) / (sq_ly + sq_lz);
        nz = iszero(lz) ? 1.0f : (r - ny * ly) / lz;
        Vec3 vTanBottom = Vec3(0, ny, nz).normalized();

        ny = (r * ly + sqrt_d) / (sq_ly + sq_lz);
        nz = iszero(lz) ? 1.0f : (r - ny * ly) / lz;
        Vec3 vTanTop = Vec3(0, ny, nz).normalized();

        pBRectVertices[2] = vLPosVS - r * vTanTop;
        pBRectVertices[3] = vLPosVS - r * vTanBottom;
    }

    Vec2 vPMin = Vec2(1, 1);
    Vec2 vPMax = Vec2(0, 0);
    Vec2 vMin = Vec2(1, 1);
    Vec2 vMax = Vec2(0, 0);

    // Project all vertices
    for (int i = 0; i < 4; i++)
    {
        Vec4 vScreenPoint = Vec4(pBRectVertices[i], 1.0) * mProj;

        //projection space clamping
        vScreenPoint.w = max(vScreenPoint.w, 0.00000000000001f);
        vScreenPoint.x = max(vScreenPoint.x, -(vScreenPoint.w));
        vScreenPoint.x = min(vScreenPoint.x, vScreenPoint.w);
        vScreenPoint.y = max(vScreenPoint.y, -(vScreenPoint.w));
        vScreenPoint.y = min(vScreenPoint.y, vScreenPoint.w);

        //NDC
        vScreenPoint /= vScreenPoint.w;

        //output coords
        //generate viewport (x=0,y=0,height=1,width=1)
        Vec2 vWin;
        vWin.x = (1.0f + vScreenPoint.x) *  0.5f;
        vWin.y = (1.0f + vScreenPoint.y) *  0.5f;  //flip coords for y axis

        assert(vWin.x >= 0.0f && vWin.x <= 1.0f);
        assert(vWin.y >= 0.0f && vWin.y <= 1.0f);

        // Get light sphere screen bounds
        vMin.x = min(vMin.x, vWin.x);
        vMin.y = min(vMin.y, vWin.y);
        vMax.x = max(vMax.x, vWin.x);
        vMax.y = max(vMax.y, vWin.y);
    }

    float fWidth = (float)gcpRendD3D->GetWidth();
    float fHeight = (float)gcpRendD3D->GetHeight();

    sX = (short)(vMin.x * fWidth);
    sY = (short)((1.0f - vMax.y) * fHeight);
    sWidth = (short)ceilf((vMax.x - vMin.x) * fWidth);
    sHeight = (short)ceilf((vMax.y - vMin.y) * fHeight);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::SetupScissors(bool bEnable, uint16 x, uint16 y, uint16 w, uint16 h) const
{
    gcpRendD3D->EF_Scissor(bEnable, x, y, w, h);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::CalculateLightScissorBounds()
{
    // Update our light scissor bounds.
    for (int lightType=0; lightType<eDLT_NumLightTypes; ++lightType)
    {
        TArray<SRenderLight>& lightArray = m_pLights[lightType][m_nThreadID][m_nRecurseLevel];
        for (uint32 nCurrentLight = 0; nCurrentLight < lightArray.Num(); ++nCurrentLight)
        {
            SRenderLight& light = lightArray[nCurrentLight];
            light.CalculateScissorRect();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::Debug()
{
    uint64 nFlagsShaderRT = gcpRendD3D->m_RP.m_FlagsShader_RT;

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_DeferredShadingDebug == 2)
    {
        SD3DPostEffectsUtils::ShBeginPass(m_pShader, m_pDebugTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
        m_pLBufferDiffuseRT->Apply(0, m_nTexStatePoint);
        SD3DPostEffectsUtils::SetTexture( CTextureManager::Instance()->GetDefaultTexture("PaletteDebug"), 1, FILTER_LINEAR, 1);
        SD3DPostEffectsUtils::DrawFullScreenTriWPOS(m_pLBufferDiffuseRT->GetWidth(), m_pLBufferDiffuseRT->GetHeight());
        SD3DPostEffectsUtils::ShEndPass();
    }

    gcpRendD3D->m_RP.m_FlagsShader_RT = nFlagsShaderRT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::DebugGBuffer()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    static CCryNameTSCRC techShadingDebug("DebugGBuffer");

    CTexture* dstTex = CTexture::s_ptexStereoR;

    m_pDepthRT->Apply(0, m_nTexStatePoint);
    m_pNormalsRT->Apply(1, m_nTexStatePoint);
    m_pDiffuseRT->Apply(2, m_nTexStatePoint);
    m_pSpecularRT->Apply(3, m_nTexStatePoint);

    rd->FX_SetState(GS_NODEPTHTEST);

    rd->FX_PushRenderTarget(0, dstTex, NULL);
    SD3DPostEffectsUtils::ShBeginPass(m_pShader, techShadingDebug, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    static CCryNameR paramName("DebugViewMode");
    Vec4 param(CRenderer::CV_r_DeferredShadingDebugGBuffer, 0, 0, 0);
    m_pShader->FXSetPSFloat(paramName, &param, 1);
    SD3DPostEffectsUtils::DrawFullScreenTri(dstTex->GetWidth(), dstTex->GetHeight());
    SD3DPostEffectsUtils::ShEndPass();
    rd->FX_PopRenderTarget(0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CDeferredShading::SetSSDOParameters(const int texSlot)
{
    if (CRenderer::CV_r_ssdo)
    {
        Vec4 ssdoParams(CRenderer::CV_r_ssdoAmountDirect, CRenderer::CV_r_ssdoAmountAmbient, CRenderer::CV_r_ssdoAmountReflection, 0);
        static CCryNameR ssdoParamsName("SSDOParams");
        m_pShader->FXSetPSFloat(ssdoParamsName, &ssdoParams, 1);
        CTexture::s_ptexSceneNormalsBent->Apply(texSlot, m_nTexStatePoint);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// Utility function for setting up and binding textures.
// Calculates and sets texture transforms as well as mipLevel
// If the ESetTexture_MipFactorProvided flag is set, the passed in mipLevelFactor will be used
// If it isn't set, mipLevelFactor will be calculated and output for possible reuse with other textures
ITexture* CDeferredShading::SetTexture(const SShaderItem& sItem, EEfResTextures tex, int slot, const RectF texRect, float surfaceSize, float& mipLevelFactor, int flags)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    AZ_Assert(0 <= slot, "Texture slot index must be positive");
    AZ_Assert(slot < EMaxTextureSlots, "Only %d texture slots available", int(EMaxTextureSlots));
    AZ_Assert(texRect.w * texRect.h > 0.f, "Texture rect has invalid dimensions");

    ITexture* pTexture = nullptr;

    if (SEfResTexture* pResTexture = sItem.m_pShaderResources->GetTextureResource((uint16)tex))
    {
        if (pResTexture->m_Sampler.m_pITex)
        {
            pTexture = pResTexture->m_Sampler.m_pITex;

            // Shader HWSR_SAMPLE flag
            if (flags & ESetTexture_HWSR)
            {
                // Asserts
                const static int maxHWSRSample = HWSR_SAMPLE5;
                static_assert((maxHWSRSample + 1) == HWSR_DEBUG0, "HWSR_SAMPLE5 is no longer the last HWSR_SAMPLE"); // If this assert triggers please ensure HWSR_SAMPLE5 is the last HWSR_SAMPLE
                AZ_Assert(slot <= (maxHWSRSample - HWSR_SAMPLE0), "Slot index too big to set HWSR_SAMPLE");

                // Set HWSR slot
                rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0 + slot];
            }

            // Texture transform
            if (flags & ESetTexture_Transform)
            {
                // Texture matrix
                Matrix44 texMatrix;
                if (pResTexture->IsHasModificators())
                {
                    pResTexture->UpdateWithModifier(tex);
                    texMatrix = pResTexture->m_Ext.m_pTexModifier->m_TexMatrix;
                }
                else
                {
                    texMatrix.SetIdentity();
                }

                // If mip level factor not provided, calculate it
                if (!(flags & ESetTexture_MipFactorProvided))
                {
                    //                 tan(fov) * (textureSize * tiling / decalSize) * distance
                    // MipLevel = log2 --------------------------------------------------------
                    //                 screenResolution * dot(viewVector, decalNormal)

                    const float screenRes = (float)rd->GetWidth() * 0.5f + (float)rd->GetHeight() * 0.5f;
                    const float texScale = max(texMatrix.GetColumn(0).GetLength() * texRect.w, texMatrix.GetColumn(1).GetLength() * texRect.h);
                    mipLevelFactor = (tan(rd->GetCamera().GetFov()) * texScale) / (surfaceSize * screenRes);
                }
                float fMipLevel = mipLevelFactor * (float)max(pTexture->GetWidth(), pTexture->GetHeight());

                // Set transform (don't forget to bind m_vTextureTransforms after calls to this function)
                m_vTextureTransforms[slot][0] = Vec4(texRect.w * texMatrix.m00, texRect.h * texMatrix.m10, texRect.x * texMatrix.m00 + texRect.y * texMatrix.m10 + texMatrix.m30, fMipLevel);
                m_vTextureTransforms[slot][1] = Vec4(texRect.w * texMatrix.m01, texRect.h * texMatrix.m11, texRect.x * texMatrix.m01 + texRect.y * texMatrix.m11 + texMatrix.m31, 0.0f);
            }
            else if (flags & ESetTexture_MipFactorProvided)
            {
                // Mip level
                float fMipLevel = mipLevelFactor * (float)max(pTexture->GetWidth(), pTexture->GetHeight());
                m_vTextureTransforms[slot][0].w = fMipLevel;
            }
             
            // Texture state
            STexState texState;
            texState.SetFilterMode(FILTER_TRILINEAR);
            texState.m_bSRGBLookup = (flags & ESetTexture_bSRGBLookup) != 0;
            texState.SetClampMode( pResTexture->m_bUTile ? TADDR_WRAP : TADDR_CLAMP,
                                   pResTexture->m_bVTile ? TADDR_WRAP : TADDR_CLAMP,
                                   TADDR_CLAMP);

            // Set Texture
            ((CTexture*)pTexture)->Apply(slot, CTexture::GetTexState(texState));
        }
    }

    // Default texture
    if ((flags & ESetTexture_AllowDefault) && (pTexture == nullptr))
    {
        ITexture* pTex = TextureHelpers::LookupTexDefault(tex);
        SD3DPostEffectsUtils::SetTexture((CTexture*)pTex, slot, FILTER_TRILINEAR, 0);
    }

    return pTexture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

uint32 CRenderer::EF_GetDeferredLightsNum(const eDeferredLightType eLightType /*= eDLT_DeferredLight*/)
{
    return CDeferredShading::Instance().GetLightsNum(eLightType);
}

TArray<SRenderLight>* CRenderer::EF_GetDeferredLights(const SRenderingPassInfo& passInfo, const eDeferredLightType eLightType /*= eDLT_DeferredLight*/)
{
    uint32 nThreadID = passInfo.ThreadID();
    int32 nRecurseLevel = passInfo.GetRecursiveLevel();

#ifndef _RELEASE
    if (nRecurseLevel < 0)
    {
        __debugbreak();
    }
#endif

    return &CDeferredShading::Instance().GetLights(nThreadID, nRecurseLevel, eLightType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

int CRenderer::EF_AddDeferredLight(const CDLight& pLight, float fMult, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    int nLightID = -1;
    CDeferredShading& pDS = CDeferredShading::Instance();
    nLightID = pDS.AddLight(pLight, fMult, passInfo, rendItemSorter);

    int nThreadID = m_RP.m_nFillThreadID;
    float mipFactor =  (m_RP.m_TI[nThreadID].m_cam.GetPosition() - pLight.m_Origin).GetLengthSquared() / max(0.001f, pLight.m_fRadius * pLight.m_fRadius);
    EF_PrecacheResource(const_cast<CDLight*>(&pLight), mipFactor, 0.1f, FPR_STARTLOADING, gRenDev->m_RP.m_TI[nThreadID].m_arrZonesRoundId[1]);
    return nLightID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::EF_ClearDeferredLightsList()
{
    if (CDeferredShading::IsValid())
    {
        CDeferredShading::Instance().ResetLights();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::EF_ReleaseDeferredData()
{
    if (CDeferredShading::IsValid())
    {
        CDeferredShading::Instance().ReleaseData();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CRenderer::EF_ClearDeferredClipVolumesList()
{
    if (CDeferredShading::IsValid())
    {
        CDeferredShading::Instance().ResetClipVolumes();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

uint8 CRenderer::EF_AddDeferredClipVolume(const IClipVolume* pClipVolume)
{
    if (CDeferredShading::IsValid() && pClipVolume)
    {
        return CDeferredShading::Instance().AddClipVolume(pClipVolume);
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
bool CRenderer::EF_SetDeferredClipVolumeBlendData(const IClipVolume* pVolume, const SClipVolumeBlendInfo& blendInfo)
{
    if (CDeferredShading::IsValid())
    {
        return CDeferredShading::Instance().SetClipVolumeBlendData(pVolume, blendInfo);
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

SRenderLight* CRenderer::EF_GetDeferredLightByID(const uint16 nLightID, const eDeferredLightType eLightType)
{
    return CDeferredShading::Instance().GetLightByID(nLightID, eLightType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_DeferredRendering(bool bDebugPass, bool bUpdateRTOnly)
{
    CDeferredShading& pDS = CDeferredShading::Instance();

    if (!CTexture::s_ptexSceneTarget)
    {
        pDS.Release();
        return false;
    }

    if (bUpdateRTOnly)
    {
        pDS.CreateDeferredMaps();
        return true;
    }

    if (!bDebugPass)
    {
        pDS.Render();
    }
    else
    {
        pDS.Debug();
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool IsDeferredDecalsSupported()
{
    bool isSupported = true;
    // For deferred decals we need to access the Normals and Linearize Depth. In GMEM both textures are bound as RT at this point.
    // Check if we can access both of them to see if we support Deferred Decals.
    if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        RenderCapabilities::FrameBufferFetchMask capabilities = RenderCapabilities::GetFrameBufferFetchCapabilities();
        isSupported =   (capabilities.test(RenderCapabilities::FBF_ALL_COLORS)) || // We can access to the Normals and Linearize depth render targets directly.
                        ((capabilities.test(RenderCapabilities::FBF_COLOR0)) && (capabilities.test(RenderCapabilities::FBF_DEPTH))); // Normals are in COLOR0 and with access to the Depth buffer we can linearize in the shader.
    }

    return isSupported;
}

bool CD3D9Renderer::FX_DeferredDecals()
{
    if IsCVarConstAccess(constexpr) (!CV_r_deferredDecals)
    {
        return false;
    }

    CD3D9Renderer* const __restrict rd = gcpRendD3D;
    uint32 nThreadID = rd->m_RP.m_nProcessThreadID;
    int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nRecurseLevel >= 0);

    DynArray<SDeferredDecal>& deferredDecals = rd->m_RP.m_DeferredDecals[nThreadID][nRecurseLevel];
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DDEFERREDSHADING_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(D3DDeferredShading_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    // Want the buffer cleared or we'll just get black out
    if (deferredDecals.empty())
    {
        return false;
    }
#endif

    if (!IsDeferredDecalsSupported())
    {
        AZ_Warning("Rendering", false, "Deferred decals is not supported in the current configuration");
        return false;
    }

    PROFILE_LABEL_SCOPE("DEFERRED_DECALS");

    CDeferredShading& rDS = CDeferredShading::Instance();
    rDS.SetupPasses();

    if (eGT_256bpp_PATH == gcpRendD3D->FX_GetEnabledGmemPath(nullptr) && CRenderer::CV_r_DeferredShadingLBuffersFmt != 2)
    {
        // GMEM 256bpp path copies normals temporarily to the diffuse light buffer (rgba16) if it exists.
        uint32 prevState = m_RP.m_CurState;
        uint32 newState = 0;
        FX_SetState(newState);
        SD3DPostEffectsUtils::PrepareGmemDeferredDecals();
        FX_SetState(prevState);
    }
    else if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        ID3D11Texture2D* pBBRes = CTexture::s_ptexBackBuffer->GetDevTexture()->Get2DTexture();
        ID3D11Texture2D* pNMRes = NULL;

        assert(CTexture::s_ptexBackBuffer->m_pPixelFormat->DeviceFormat == CTexture::s_ptexSceneNormalsMap->m_pPixelFormat->DeviceFormat);

        if (rd->m_RP.m_MSAAData.Type > 1) //always copy when deferredDecals is not empty
        {
            pNMRes = (D3DTexture*)CTexture::s_ptexSceneNormalsMap->m_pRenderTargetData->m_pDeviceTextureMSAA->Get2DTexture();
            rd->GetDeviceContext().ResolveSubresource(pBBRes, 0, pNMRes, 0, (DXGI_FORMAT)CTexture::s_ptexBackBuffer->m_pPixelFormat->DeviceFormat);
        }
        else
        {
            pNMRes = CTexture::s_ptexSceneNormalsMap->GetDevTexture()->Get2DTexture();
            rd->GetDeviceContext().CopyResource(pBBRes, pNMRes);
        }
    }

    std::stable_sort(deferredDecals.begin(), deferredDecals.end(), DeffDecalSort());

    //if (CV_r_deferredDecalsDebug == 0)
    //  rd->FX_PushRenderTarget(1, CTexture::s_ptexSceneNormalsMap, NULL);

    const uint32 nNumDecals = deferredDecals.size();
    for (uint32 d = 0; d < nNumDecals; ++d)
    {
        rDS.DeferredDecalPass(deferredDecals[d], d);
    }

    //if (CV_r_deferredDecalsDebug == 0)
    //  rd->FX_PopRenderTarget(1);

    rd->SetCullMode(R_CULL_BACK);

    // Commit any potential render target changes - required for when shadows disabled
    rd->FX_SetActiveRenderTargets(false);
    rd->FX_ResetPipe();

    return true;
}

// Renders emissive part of all deferred decals
// This is called after the deferred lighting resolve since emissive
// lighting is additive in relation to diffuse and specular 
// Called by CD3D9Renderer::FX_RenderForwardOpaque
bool CD3D9Renderer::FX_DeferredDecalsEmissive()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if IsCVarConstAccess(constexpr) (!CV_r_deferredDecals)
        return false;

    if (!IsDeferredDecalsSupported())
    {
        return false;
    }

    uint32 nThreadID = rd->m_RP.m_nProcessThreadID;
    int32 nRecurseLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nRecurseLevel >= 0);

    DynArray<SDeferredDecal>& deferredDecals = rd->m_RP.m_DeferredDecals[nThreadID][nRecurseLevel];

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DDEFERREDSHADING_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(D3DDeferredShading_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    // Want the buffer cleared or we'll just get black out
    if (deferredDecals.empty())
    {
        return false;
    }
#endif

    PROFILE_LABEL_SCOPE("DEFERRED_DECALS");

    CDeferredShading& rDS = CDeferredShading::Instance();

    const uint32 nNumDecals = deferredDecals.size();
    for (uint32 d = 0; d < nNumDecals; ++d)
    {
        rDS.DeferredDecalEmissivePass(deferredDecals[d], d);
    }

    rd->SetCullMode(R_CULL_BACK);

    // Commit any potential render target changes - required for when shadows disabled
    rd->FX_SetActiveRenderTargets(false);
    rd->FX_ResetPipe();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool CREDeferredShading::mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    if (gcpRendD3D->m_bDeviceLost)
    {
        return 0;
    }

    gcpRendD3D->FX_DeferredRendering();
    return true;
}
