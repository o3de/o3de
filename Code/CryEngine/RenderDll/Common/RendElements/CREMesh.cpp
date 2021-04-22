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
#include "CREMeshImpl.h"

#if !defined(NULL_RENDERER)
#include "XRenderD3D9/DriverD3D.h"
#endif

void CREMeshImpl::mfReset()
{
}

void CREMeshImpl::mfCenter(Vec3& Pos, CRenderObject* pObj)
{
    Vec3 Mins = m_pRenderMesh->m_vBoxMin;
    Vec3 Maxs = m_pRenderMesh->m_vBoxMax;
    Pos = (Mins + Maxs) * 0.5f;
    if (pObj)
    {
        Pos += pObj->GetTranslation();
    }
}

void CREMeshImpl::mfGetBBox(Vec3& vMins, Vec3& vMaxs)
{
    vMins = m_pRenderMesh->_GetVertexContainer()->m_vBoxMin;
    vMaxs = m_pRenderMesh->_GetVertexContainer()->m_vBoxMax;
}


///////////////////////////////////////////////////////////////////

void CREMeshImpl::mfPrepare(bool bCheckOverflow)
{
    DETAILED_PROFILE_MARKER("CREMeshImpl::mfPrepare");
    CRenderer* rd = gRenDev;

    if (bCheckOverflow)
    {
        rd->FX_CheckOverflow(0, 0, this);
    }

    IF (!m_pRenderMesh, 0)
    {
        return;
    }

    rd->m_RP.m_CurVFormat = m_pChunk->m_vertexFormat;

    {
        rd->m_RP.m_pRE = this;

        rd->m_RP.m_FirstVertex = m_nFirstVertId;
        rd->m_RP.m_FirstIndex =  m_nFirstIndexId;
        rd->m_RP.m_RendNumIndices = m_nNumIndices;
        rd->m_RP.m_RendNumVerts = m_nNumVerts;

        if (rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & (RBPF_SHADOWGEN) && (gRenDev->m_RP.m_PersFlags2 & RBPF2_DISABLECOLORWRITES))
        {
            _smart_ptr<IMaterial> pMaterial = (gRenDev->m_RP.m_pCurObject) ? (gRenDev->m_RP.m_pCurObject->m_pCurrMaterial) : NULL;
            m_pRenderMesh->AddShadowPassMergedChunkIndicesAndVertices(m_pChunk, pMaterial, rd->m_RP.m_RendNumVerts, rd->m_RP.m_RendNumIndices);
        }
    }
}

TRenderChunkArray* CREMeshImpl::mfGetMatInfoList()
{
    return &m_pRenderMesh->m_Chunks;
}

int CREMeshImpl::mfGetMatId()
{
    return m_pChunk->m_nMatID;
}

CRenderChunk* CREMeshImpl::mfGetMatInfo()
{
    return m_pChunk;
}

void CREMeshImpl::mfPrecache(const SShaderItem& SH)
{
    DETAILED_PROFILE_MARKER("CREMeshImpl::mfPrecache");
    CShader* pSH = (CShader*)SH.m_pShader;
    IF (!pSH, 0)
    {
        return;
    }
    IF (!m_pRenderMesh, 0)
    {
        return;
    }
    IF (m_pRenderMesh->_HasVBStream(VSF_GENERAL), 0)
    {
        return;
    }

    mfCheckUpdate(VSM_TANGENTS, gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID);
}

bool CREMeshImpl::mfUpdate(int Flags, bool bTessellation)
{
    DETAILED_PROFILE_MARKER("CREMeshImpl::mfUpdate");
    FUNCTION_PROFILER_RENDER_FLAT
    IF (m_pRenderMesh == NULL, 0)
    {
        return false;
    }

    CRenderer* rd = gRenDev;
    const int threadId = rd->m_RP.m_nProcessThreadID;

    bool bSucceed = true;

    CRenderMesh* pVContainer = m_pRenderMesh->_GetVertexContainer();
    
    m_pRenderMesh->m_nFlags &= ~FRM_SKINNEDNEXTDRAW;

    if (m_pRenderMesh->m_Modified[threadId].linked() || bTessellation) // TODO: use the modified list also for tessellated meshes
    {
        m_pRenderMesh->SyncAsyncUpdate(gRenDev->m_RP.m_nProcessThreadID);

        bSucceed = m_pRenderMesh->RT_CheckUpdate(pVContainer, Flags | VSM_MASK, bTessellation);
        if (bSucceed)
        {
            m_pRenderMesh->m_Modified[threadId].erase();
        }
    }

    if (!bSucceed || !pVContainer->_HasVBStream(VSF_GENERAL))
    {
        return false;
    }

    bool bSkinned = (m_pRenderMesh->m_nFlags & (FRM_SKINNED | FRM_SKINNEDNEXTDRAW)) != 0;
    if ((Flags | VSM_MASK) & VSM_TANGENTS)
    {
        if (bSkinned && pVContainer->_HasVBStream(VSF_QTANGENTS))
        {
            rd->m_RP.m_FlagsStreams_Stream &= ~VSM_TANGENTS;
            rd->m_RP.m_FlagsStreams_Decl &= ~VSM_TANGENTS;
            rd->m_RP.m_FlagsStreams_Stream |= (1 << VSF_QTANGENTS);
            rd->m_RP.m_FlagsStreams_Decl |= (1 << VSF_QTANGENTS);
        }
    }

    rd->m_RP.m_CurVFormat = m_pChunk->m_vertexFormat;
    m_Flags &= ~FCEF_DIRTY;

    return true;
}

void* CREMeshImpl::mfGetPointer(ESrcPointer ePT, int* Stride, [[maybe_unused]] EParamType Type, [[maybe_unused]] ESrcPointer Dst, [[maybe_unused]] int Flags)
{
    DETAILED_PROFILE_MARKER("CREMeshImpl::mfGetPointer");
    CRenderMesh* pRM = m_pRenderMesh->_GetVertexContainer();
    byte* pD = NULL;
    IRenderMesh::ThreadAccessLock lock(pRM);

    switch (ePT)
    {
    case eSrcPointer_Vert:
        pD = pRM->GetPosPtr(*Stride,  FSL_READ);
        break;
    case eSrcPointer_Tex:
        pD = pRM->GetUVPtr(*Stride,  FSL_READ);
        break;
    case eSrcPointer_Normal:
        pD = pRM->GetNormPtr(*Stride,  FSL_READ);
        break;
    case eSrcPointer_Tangent:
        pD = pRM->GetTangentPtr(*Stride, FSL_READ);
        break;
    case eSrcPointer_Color:
        pD = pRM->GetColorPtr(*Stride, FSL_READ);
        break;
    default:
        assert(false);
        break;
    }
    if (m_nFirstVertId && pD)
    {
        pD += m_nFirstVertId * (*Stride);
    }
    return pD;
}

void CREMeshImpl::mfGetPlane(Plane& pl)
{

    // fixme: plane orientation based on biggest bbox axis
    Vec3 pMin, pMax;
    mfGetBBox(pMin, pMax);
    Vec3 p0 = pMin;
    Vec3 p1 = Vec3(pMax.x, pMin.y, pMin.z);
    Vec3 p2 = Vec3(pMin.x, pMax.y, pMin.z);
    pl.SetPlane(p2, p0, p1);
}

AZ::Vertex::Format CREMeshImpl::GetVertexFormat() const
{
    if (m_pChunk)
    {
        return m_pChunk->m_vertexFormat;
    }
    else if (m_pRenderMesh)
    {
        return m_pRenderMesh->_GetVertexContainer()->_GetVertexFormat();
    }
    return AZ::Vertex::Format(eVF_Unknown);
}
bool CREMeshImpl::GetGeometryInfo(SGeometryInfo &geomInfo)
{
    if (!m_pRenderMesh)
        return false;

    CRenderMesh *pVContainer = m_pRenderMesh->_GetVertexContainer();

    geomInfo.nFirstIndex = m_nFirstIndexId;
    geomInfo.nFirstVertex = m_nFirstVertId;
    geomInfo.nNumVertices = m_nNumVerts;
    geomInfo.nNumIndices = m_nNumIndices;

    geomInfo.vertexFormat = pVContainer->_GetVertexFormat();
    geomInfo.primitiveType = pVContainer->GetPrimitiveType();

    geomInfo.streamMask = 0;

    const bool bSkinned = (m_pRenderMesh->m_nFlags & (FRM_SKINNED | FRM_SKINNEDNEXTDRAW)) != 0;
    if (bSkinned && pVContainer->_HasVBStream(VSF_QTANGENTS))
        geomInfo.streamMask |= BIT(VSF_QTANGENTS);

    {
        // Check if needs updating.
        //TODO Fix constant | 0x80000000
        //bool bTessEnabled = (pso->m_ShaderFlags_RT & g_HWSR_MaskBit[HWSR_NO_TESSELLATION]) != 0;
        bool bTessEnabled = false;
        uint32 streamMask = 0;
        uint16 nFrameId = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
        if (!mfCheckUpdate((uint32)streamMask | 0x80000000, (uint16)nFrameId, bTessEnabled))
            return false;
    }

    if (!m_pRenderMesh->FillGeometryInfo(geomInfo))
        return false;

    return true;
}

bool CREMeshImpl::BindRemappedSkinningData([[maybe_unused]] uint32 guid)
{
#if !defined(NULL_RENDERER)
    CD3D9Renderer *rd = gcpRendD3D;

    SGeometryStreamInfo streamInfo;
    CRenderMesh *pRM = m_pRenderMesh->_GetVertexContainer();
    if (pRM->GetRemappedSkinningData(guid, streamInfo))
    {
        rd->FX_SetVStream(VSF_HWSKIN_INFO, streamInfo.pStream, streamInfo.nOffset, streamInfo.nStride);
        return true;
    }
#endif
    return false;
}

#if !defined(NULL_RENDERER)
bool CREMeshImpl::mfPreDraw([[maybe_unused]] SShaderPass *sl)
{
    DETAILED_PROFILE_MARKER("CREMeshImpl::mfPreDraw");
    IF(!m_pRenderMesh, 0)
        return false;

    CRenderMesh *pRM = m_pRenderMesh->_GetVertexContainer();
    pRM->PrefetchVertexStreams();

    // Should never happen. Video buffer is missing
    if (!pRM->_HasVBStream(VSF_GENERAL) || !m_pRenderMesh->_HasIBStream())
        return false;

    m_pRenderMesh->BindStreamsToRenderPipeline();

    m_Flags |= FCEF_PRE_DRAW_DONE;

    return true;
}

#if !defined(_RELEASE)
inline bool CREMeshImpl::ValidateDraw(EShaderType shaderType)
{
    bool ret = true;

    if (shaderType != eST_General &&
        shaderType != eST_PostProcess &&
        shaderType != eST_FX &&
        shaderType != eST_Glass &&
        shaderType != eST_Water)
    {
        CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Incorrect shader set for mesh type: %s : %d", m_pRenderMesh->GetSourceName(), shaderType);
        ret = false;
    }

    if (!(m_Flags&FCEF_PRE_DRAW_DONE))
    {
        CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "PreDraw not called for mesh: %s", m_pRenderMesh->GetSourceName());
        ret = false;
    }

    return ret;
}
#endif

bool CREMeshImpl::mfDraw(CShader *ef, [[maybe_unused]] SShaderPass *sl)
{
    DETAILED_PROFILE_MARKER("CREMeshImpl::mfDraw");
    FUNCTION_PROFILER_RENDER_FLAT
        CD3D9Renderer *r = gcpRendD3D;

#if !defined(_RELEASE)
    if (!ValidateDraw(ef->m_eShaderType))
    {
        return false;
    }
#endif

    CRenderMesh *pRM = m_pRenderMesh;
    if (ef->m_HWTechniques.Num() && pRM->CanRender())
    {
        r->FX_DrawIndexedMesh(r->m_RP.m_RendNumGroup >= 0 ? eptHWSkinGroups : pRM->GetPrimitiveType());
    }
    return true;
}
#endif
