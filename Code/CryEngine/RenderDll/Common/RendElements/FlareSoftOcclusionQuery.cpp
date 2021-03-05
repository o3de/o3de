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
#include "FlareSoftOcclusionQuery.h"
#include "../CryNameR.h"
#include "../../XRenderD3D9/DriverD3D.h"
#include "../Textures/Texture.h"
#include "I3DEngine.h"

unsigned char CFlareSoftOcclusionQuery::s_paletteRawCache[s_nIDMax * 4];
char CFlareSoftOcclusionQuery::s_idHashTable[s_nIDMax];
int CFlareSoftOcclusionQuery::s_idCount = 0;
int CFlareSoftOcclusionQuery::s_ringReadIdx = 1;
int CFlareSoftOcclusionQuery::s_ringWriteIdx = 0;
int CFlareSoftOcclusionQuery::s_ringSize = MAX_OCCLUSION_READBACK_TEXTURES;

static bool g_bCreatedGlobalResources = false;

const float CFlareSoftOcclusionQuery::s_fSectorWidth = (float)s_nGatherEachSectorWidth / (float)s_nGatherTextureWidth;
const float CFlareSoftOcclusionQuery::s_fSectorHeight = (float)s_nGatherEachSectorHeight / (float)s_nGatherTextureHeight;

float CSoftOcclusionVisiblityFader::UpdateVisibility(const float newTargetVisibility, const float duration)
{
    m_TimeLine.duration = duration;

    if (fabs(newTargetVisibility - m_fTargetVisibility) > 0.1f)
    {
        m_TimeLine.startValue = m_TimeLine.GetPrevYValue();
        m_fTargetVisibility = newTargetVisibility;
        m_TimeLine.rewind();
    }
    else if (newTargetVisibility < 0.05f)
    {
        m_fTargetVisibility = newTargetVisibility;
        m_TimeLine.endValue = newTargetVisibility;
    }
    else
    {
        m_TimeLine.endValue = m_fTargetVisibility;
    }

    // Fade in fixed time (in ms), fade out user controlled
    const float fadeInTime = m_TimeLine.duration * (1.0f / 0.15f);
    const float fadeOutTime = 1e3f;
    float dt = (newTargetVisibility > m_fVisibilityFactor) ? fadeInTime : fadeOutTime;

    m_fVisibilityFactor = m_TimeLine.step(gEnv->pTimer->GetFrameTime() * dt); // interpolate

    return m_fVisibilityFactor;
}

void ReportIDPoolOverflow(int idCount, int idMax)
{
    iLog->Log("Number of soft occlusion queries [%d] exceeds current allowed range %d", idCount, idMax);
}

int CFlareSoftOcclusionQuery::GenID()
{
    int hashCode = s_idCount % s_nIDMax;

    while (hashCode < s_nIDMax && s_idHashTable[hashCode])
    {
        hashCode++;
    }

    if (hashCode >= s_nIDMax)
    {
        ReportIDPoolOverflow(s_idCount + 1, s_nIDMax);
        hashCode = 0;
    }

    s_idHashTable[hashCode] = (char)1;
    s_idCount++;

    return hashCode;
}

void CFlareSoftOcclusionQuery::ReleaseID(int id)
{
    if (id < s_nIDMax)
    {
        s_idHashTable[id] = (char)0;
    }
}

void CFlareSoftOcclusionQuery::InitGlobalResources()
{
    if (g_bCreatedGlobalResources)
    {
        return;
    }

    const uint32 numGPUs = (gRenDev->GetActiveGPUCount() <= MAX_OCCLUSION_READBACK_TEXTURES / 2) ? gRenDev->GetActiveGPUCount() : 1;
    s_ringWriteIdx = 0;
    s_ringReadIdx = numGPUs;
    s_ringSize = MAX_OCCLUSION_READBACK_TEXTURES;

    memset(s_idHashTable, 0, sizeof(s_idHashTable));
    memset(s_paletteRawCache, 0, sizeof(s_paletteRawCache));

    g_bCreatedGlobalResources = true;
}

void CFlareSoftOcclusionQuery::GetDomainInTexture(float& out_x0, float& out_y0, float& out_x1, float& out_y1)
{
    out_x0 = (m_nID % s_nIDColMax) * s_fSectorWidth;
    out_y0 = (m_nID / s_nIDColMax) * s_fSectorHeight;
    out_x1 = out_x0 + s_fSectorWidth;
    out_y1 = out_y0 + s_fSectorHeight;
}

bool CFlareSoftOcclusionQuery::ComputeProjPos(const Vec3& vWorldPos, const Matrix44A& viewMat, const Matrix44A& projMat, Vec3& outProjPos)
{
    static int vp[] = {0, 0, 1, 1};
    if (mathVec3Project(&outProjPos, &vWorldPos, vp, &projMat, &viewMat, &gRenDev->m_IdentityMatrix) == 0.0f)
    {
        return false;
    }
    return true;
}

void CFlareSoftOcclusionQuery::GetSectorSize(float& width, float& height)
{
    width = s_fSectorWidth;
    height = s_fSectorWidth;
}

void CFlareSoftOcclusionQuery::GetOcclusionSectorInfo(SOcclusionSectorInfo& out_Info)
{
    Vec3 vProjectedPos;
    if (ComputeProjPos(m_PosToBeChecked, gRenDev->m_ViewMatrix, gRenDev->m_ProjMatrix, vProjectedPos) == false)
    {
        return;
    }
    out_Info.lineardepth = clamp_tpl(ComputeLinearDepth(m_PosToBeChecked, gRenDev->m_CameraMatrix, gRenDev->GetViewParameters().fNear, gRenDev->GetViewParameters().fFar), -1.0f, 0.99f);

    out_Info.u0 = vProjectedPos.x - m_fOccPlaneWidth / 2;
    out_Info.v0 = vProjectedPos.y - m_fOccPlaneHeight / 2;
    out_Info.u1 = vProjectedPos.x + m_fOccPlaneWidth / 2;
    out_Info.v1 = vProjectedPos.y + m_fOccPlaneHeight / 2;

    if (out_Info.u0 < 0)
    {
        out_Info.u0 = 0;
    }
    if (out_Info.v0 < 0)
    {
        out_Info.v0 = 0;
    }
    if (out_Info.u1 > 1)
    {
        out_Info.u1 = 1;
    }
    if (out_Info.v1 > 1)
    {
        out_Info.v1 = 1;
    }

    GetDomainInTexture(out_Info.x0, out_Info.y0, out_Info.x1, out_Info.y1);

    out_Info.x0 = out_Info.x0 * 2.0f - 1;
    out_Info.y0 = -(out_Info.y0 * 2.0f - 1.0f);
    out_Info.x1 = out_Info.x1 * 2.0f - 1;
    out_Info.y1 = -(out_Info.y1 * 2.0f - 1.0f);
}

float CFlareSoftOcclusionQuery::ComputeLinearDepth(const Vec3& worldPos, const Matrix44A& cameraMat, float nearDist, float farDist)
{
    Vec4 out, wPos4;
    wPos4.x = worldPos.x;
    wPos4.y = worldPos.y;
    wPos4.z = worldPos.z;
    wPos4.w = 1;
    mathVec4Transform((f32*)&out, (f32*)(&cameraMat), (f32*)&wPos4);
    if (out.w == 0.0f)
    {
        return 0;
    }
    const CameraViewParameters& rc = gRenDev->GetViewParameters();
    float linearDepth = (-out.z - rc.fNear) / (farDist - nearDist);

    return linearDepth;
}

void CFlareSoftOcclusionQuery::UpdateCachedResults()
{
    int cacheIdx = 4 * m_nID;
    m_fOccResultCache = s_paletteRawCache[cacheIdx + 0] / 255.0f;
    m_fDirResultCache = (s_paletteRawCache[cacheIdx + 3] / 255.0f) * 2.0f * PI;
    sincos_tpl(m_fDirResultCache, &m_DirVecResultCache.y, &m_DirVecResultCache.x);
}

CTexture* CFlareSoftOcclusionQuery::GetGatherTexture() const
{
    return CTexture::s_ptexFlaresGather;
}

void CFlareSoftOcclusionQuery::BatchReadResults()
{
    if (!g_bCreatedGlobalResources)
    {
        return;
    }
    CTexture::s_ptexFlaresOcclusionRing[s_ringWriteIdx]->GetDevTexture()->AccessCurrStagingResource(0, false, [=](void* pData, uint32 rowPitch, [[maybe_unused]] uint32 slicePitch)
        {
            unsigned char* pTexBuf = reinterpret_cast<unsigned char*>(pData);
            int validLineStrideBytes = s_nIDColMax * 4;
            for (int i = 0; i < s_nIDRowMax; i++)
            {
                memcpy(s_paletteRawCache + i * validLineStrideBytes, pTexBuf + i * rowPitch, validLineStrideBytes);
            }

            return true;
        });
}

void CFlareSoftOcclusionQuery::ReadbackSoftOcclQuery()
{
    CTexture::s_ptexFlaresOcclusionRing[s_ringWriteIdx]->GetDevTexture()->DownloadToStagingResource(0);

    // sync point. Move to next texture to read and write
    s_ringReadIdx = (s_ringReadIdx + 1) % s_ringSize;
    s_ringWriteIdx = (s_ringWriteIdx + 1) % s_ringSize;
}

CTexture* CFlareSoftOcclusionQuery::GetOcclusionTex()
{
    return CTexture::s_ptexFlaresOcclusionRing[s_ringWriteIdx];
}

void CSoftOcclusionManager::ComputeVisibility()
{
    static STexState ShadowTexState(FILTER_POINT, TADDR_BORDER, TADDR_BORDER, TADDR_BORDER, 0);
    CShader* pShader = CShaderMan::s_ShaderSoftOcclusionQuery;

    gcpRendD3D->FX_ClearTarget(CTexture::s_ptexFlaresGather, Clr_Transparent);
    gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexFlaresGather, NULL);

    pShader->FXBeginPass(0);

    const uint32 vertexCount = GetSize() * 4;

    TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
    vb.Allocate(vertexCount);
    SVF_P3F_C4B_T2F* pDeviceVBAddr = vb.Lock();

    for (int i(0), iSoftOcclusionListSize(GetSize()); i < iSoftOcclusionListSize; ++i)
    {
        CFlareSoftOcclusionQuery* pSoftOcclusion = GetSoftOcclusionQuery(i);
        if (pSoftOcclusion == NULL)
        {
            continue;
        }
        int offset = i * 4;

        CFlareSoftOcclusionQuery::SOcclusionSectorInfo sInfo;
        pSoftOcclusion->GetOcclusionSectorInfo(sInfo);

        for (int k = 0; k < 4; ++k)
        {
            pDeviceVBAddr[offset + k].color.dcolor = 0xFFFFFFFF;
        }

        pDeviceVBAddr[offset + 0].st = Vec2(sInfo.u0 * gcpRendD3D->m_CurViewportScale.x, sInfo.v0 * gcpRendD3D->m_CurViewportScale.y);
        pDeviceVBAddr[offset + 1].st = Vec2(sInfo.u1 * gcpRendD3D->m_CurViewportScale.x, sInfo.v0 * gcpRendD3D->m_CurViewportScale.y);
        pDeviceVBAddr[offset + 2].st = Vec2(sInfo.u1 * gcpRendD3D->m_CurViewportScale.x, sInfo.v1 * gcpRendD3D->m_CurViewportScale.y);
        pDeviceVBAddr[offset + 3].st = Vec2(sInfo.u0 * gcpRendD3D->m_CurViewportScale.x, sInfo.v1 * gcpRendD3D->m_CurViewportScale.y);

        pDeviceVBAddr[offset + 0].xyz = Vec3(sInfo.x0, sInfo.y1, sInfo.lineardepth);
        pDeviceVBAddr[offset + 1].xyz = Vec3(sInfo.x1, sInfo.y1, sInfo.lineardepth);
        pDeviceVBAddr[offset + 2].xyz = Vec3(sInfo.x1, sInfo.y0, sInfo.lineardepth);
        pDeviceVBAddr[offset + 3].xyz = Vec3(sInfo.x0, sInfo.y0, sInfo.lineardepth);
    }

    vb.Unlock();
    vb.Bind(0);
    vb.Release();

    if (pDeviceVBAddr && m_bSuccessGenerateIB)
    {
        CTexture::s_ptexZTargetScaled->Apply(0, CTexture::GetTexState(ShadowTexState));

        gcpRendD3D->FX_Commit();

        if (SUCCEEDED(gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
        {
            gcpRendD3D->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, vertexCount, 0, m_IndexBufferCount);
        }
    }
    pShader->FXEndPass();
    gcpRendD3D->FX_PopRenderTarget(0);
}

bool CSoftOcclusionManager::GenerateIndexBuffer()
{
    m_IndexBufferCount = GetSize() * 2 * 3;
    if (m_IndexBufferCount <= 0)
    {
        return false;
    }

    TempDynIB16 ib(gcpRendD3D);
    ib.Allocate(m_IndexBufferCount);
    uint16* pDeviceIBAddr = ib.Lock();

    for (int i(0), iSoftOcclusionListSize(GetSize()); i < iSoftOcclusionListSize; ++i)
    {
        CFlareSoftOcclusionQuery* pSoftOcclusion = GetSoftOcclusionQuery(i);
        if (pSoftOcclusion == NULL)
        {
            continue;
        }
        int offset0 = i * 6;
        int offset1 = i * 4;
        pDeviceIBAddr[offset0 + 0] = offset1 + 0;
        pDeviceIBAddr[offset0 + 1] = offset1 + 1;
        pDeviceIBAddr[offset0 + 2] = offset1 + 2;
        pDeviceIBAddr[offset0 + 3] = offset1 + 2;
        pDeviceIBAddr[offset0 + 4] = offset1 + 3;
        pDeviceIBAddr[offset0 + 5] = offset1 + 0;
    }

    ib.Unlock();
    m_IndexBufferOffset = 0;
    ib.Bind();
    ib.Release();

    return true;
}

void CSoftOcclusionManager::GatherOcclusions()
{
    CShader* pShader = CShaderMan::s_ShaderSoftOcclusionQuery;
    static STexState GatherTexState(FILTER_POINT, true);

    gcpRendD3D->FX_ClearTarget(CFlareSoftOcclusionQuery::GetOcclusionTex(), Clr_Transparent);
    gcpRendD3D->FX_PushRenderTarget(0, CFlareSoftOcclusionQuery::GetOcclusionTex(), NULL);

    pShader->FXBeginPass(1);
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;

    const uint32 vertexCount = GetSize() * 4;

    TempDynVB<SVF_P3F_C4B_T2F> vb(gcpRendD3D);
    vb.Allocate(vertexCount);
    SVF_P3F_C4B_T2F* pDeviceVBAddr = vb.Lock();

    for (int i = 0, iSoftOcclusionListSize(GetSize()); i < iSoftOcclusionListSize; ++i)
    {
        CFlareSoftOcclusionQuery* pSoftOcclusion = GetSoftOcclusionQuery(i);
        if (pSoftOcclusion == NULL)
        {
            continue;
        }
        int offset = i * 4;
        pSoftOcclusion->GetDomainInTexture(x0, y0, x1, y1);
        for (int k = 0; k < 4; ++k)
        {
            pDeviceVBAddr[offset + k].st = Vec2((x0 + x1) * 0.5f * gcpRendD3D->m_CurViewportScale.x, (y0 + y1) * 0.5f * gcpRendD3D->m_CurViewportScale.y);
            pDeviceVBAddr[offset + k].color.dcolor = 0xFFFFFFFF;
        }
        x0 = x0 * 2.0f - 1.0f;
        y0 = -(y0 * 2.0f - 1.0f);
        x1 = x1 * 2.0f - 1.0f;
        y1 = -(y1 * 2.0f - 1.0f);
        pDeviceVBAddr[offset + 0].xyz = Vec3(x0, y1, 1);
        pDeviceVBAddr[offset + 1].xyz = Vec3(x1, y1, 1);
        pDeviceVBAddr[offset + 2].xyz = Vec3(x1, y0, 1);
        pDeviceVBAddr[offset + 3].xyz = Vec3(x0, y0, 1);
    }

    vb.Unlock();
    vb.Bind(0);
    vb.Release();

    if (pDeviceVBAddr && m_bSuccessGenerateIB)
    {
        static CCryNameR occlusionNormalizedSizeName("occlusionNormalizedSize");
        const Vec4 occlusionSizeParam(CFlareSoftOcclusionQuery::s_fSectorWidth, CFlareSoftOcclusionQuery::s_fSectorHeight, 0, 0);
        pShader->FXSetPSFloat(occlusionNormalizedSizeName, &occlusionSizeParam, 1);
        CTexture::s_ptexFlaresGather->Apply(0, CTexture::GetTexState(GatherTexState));

        gcpRendD3D->FX_Commit();

        if (SUCCEEDED(gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
        {
            gcpRendD3D->FX_DrawIndexedPrimitive(eptTriangleList, 0, 0, vertexCount, 0, m_IndexBufferCount);
        }
    }
    pShader->FXEndPass();
    gcpRendD3D->FX_PopRenderTarget(0);
}

CFlareSoftOcclusionQuery* CSoftOcclusionManager::GetSoftOcclusionQuery(int nIndex) const
{
    if (nIndex >= CFlareSoftOcclusionQuery::s_nIDMax)
    {
        return NULL;
    }
    return m_SoftOcclusionQueries[nIndex];
}

void CSoftOcclusionManager::AddSoftOcclusionQuery(CFlareSoftOcclusionQuery* pQuery, const Vec3& vPos)
{
    if (m_nPos < CFlareSoftOcclusionQuery::s_nIDMax)
    {
        pQuery->SetPosToBeChecked(vPos);
        m_SoftOcclusionQueries[m_nPos++] = pQuery;
    }
}

bool CSoftOcclusionManager::Begin()
{
    if (GetSize() > 0)
    {
        m_bSuccessGenerateIB = GenerateIndexBuffer();
        return true;
    }
    return false;
}

void CSoftOcclusionManager::End()
{
    m_nPos = 0;
}

void CSoftOcclusionManager::ClearResources()
{
    for (int i = 0; i < CFlareSoftOcclusionQuery::s_nIDMax; ++i)
    {
        m_SoftOcclusionQueries[i] = NULL;
    }
}
