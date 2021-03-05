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

// Description : common RE functions.


#include "RenderDll_precompiled.h"

//TArray<CRendElementBase *> CRendElementBase::m_AllREs;

CRendElement CRendElement::m_RootGlobal;
CRendElement CRendElement::m_RootRelease[4];

//===============================================================

CryCriticalSection m_sREResLock;

//============================================================================

void CRendElement::ShutDown()
{
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_releaseallresourcesonexit)
    {
        return;
    }

    AUTO_LOCK(m_sREResLock); // Not thread safe without this

    CRendElement* pRE;
    CRendElement* pRENext;
    for (pRE = CRendElement::m_RootGlobal.m_NextGlobal; pRE != &CRendElement::m_RootGlobal; pRE = pRENext)
    {
        pRENext = pRE->m_NextGlobal;
        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_printmemoryleaks)
        {
            iLog->Log("Warning: CRendElementBase::ShutDown: RenderElement %s was not deleted", pRE->mfTypeString());
        }
        pRE->Release();
    }
}

void CRendElement::Tick()
{
#ifndef STRIP_RENDER_THREAD
    assert(gRenDev->m_pRT->IsMainThread(true));
#endif
    int nFrameID = gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nFillThreadID].m_nFrameUpdateID;
    int nFrame = nFrameID - 3;
    CRendElement& Root = CRendElement::m_RootRelease[nFrame & 3];
    CRendElement* pRENext = NULL;

    for (CRendElement* pRE = Root.m_NextGlobal; pRE != &Root; pRE = pRENext)
    {
        pRENext = pRE->m_NextGlobal;
        SAFE_DELETE(pRE);
    }
}

void CRendElement::Cleanup()
{
    gRenDev->m_pRT->FlushAndWait();

    AUTO_LOCK(m_sREResLock); // Not thread safe without this

    for (int i = 0; i < 4; ++i)
    {
        CRendElement& Root = CRendElement::m_RootRelease[i];
        CRendElement* pRENext = NULL;

        for (CRendElement* pRE = Root.m_NextGlobal; pRE != &Root; pRE = pRENext)
        {
            pRENext = pRE->m_NextGlobal;
            SAFE_DELETE(pRE);
        }
    }
}

CRendElement::CRendElement()
{
    m_Type = eDATA_Unknown;
    if (!m_RootGlobal.m_NextGlobal)
    {
        m_RootGlobal.m_NextGlobal = &m_RootGlobal;
        m_RootGlobal.m_PrevGlobal = &m_RootGlobal;
        for (int i = 0; i < 4; i++)
        {
            m_RootRelease[i].m_NextGlobal = &m_RootRelease[i];
            m_RootRelease[i].m_PrevGlobal = &m_RootRelease[i];
        }
    }
}


CRendElement::~CRendElement()
{
    assert(m_Type == eDATA_Unknown || m_Type == eDATA_Particle || m_Type == eDATA_GPUParticle || m_Type == eDATA_Gem || m_Type == eDATA_VolumeObject);

    //@TODO: Fix later, prevent crash on exit in single executable
    if (this == &m_RootRelease[0] || this == &m_RootRelease[1] || this == &m_RootRelease[2] || this == &m_RootRelease[3] || this == &m_RootGlobal)
    {
        return;
    }

    AUTO_LOCK(m_sREResLock);
    UnlinkGlobal();
}

void CRendElement::Release(bool bForce)
{
    CRendElementBase* pThis = (CRendElementBase*)this;
    pThis->m_Flags |= FCEF_DELETED;

    m_Type = eDATA_Unknown;
    if (bForce)
    {
        delete this;
        return;
    }
    int nFrame = gRenDev->GetFrameID(false);

    AUTO_LOCK(m_sREResLock);
    CRendElement& Root = CRendElement::m_RootRelease[nFrame & 3];
    UnlinkGlobal();
    LinkGlobal(&Root);
}

CRendElementBase::CRendElementBase()
{
    m_Flags = 0;
    m_nFrameUpdated = 0xffff;
    m_CustomData = NULL;
    m_NextGlobal = NULL;
    m_PrevGlobal = NULL;
    int i;
    for (i = 0; i < MAX_CUSTOM_TEX_BINDS_NUM; i++)
    {
        m_CustomTexBind[i] = -1;
    }

    AUTO_LOCK(m_sREResLock);
    LinkGlobal(&m_RootGlobal);
}
CRendElementBase::~CRendElementBase()
{
    if ((m_Flags & FCEF_ALLOC_CUST_FLOAT_DATA) && m_CustomData)
    {
        delete [] ((float*)m_CustomData);
        m_CustomData = 0;
    }
}

const char* CRendElement::mfTypeString()
{
    switch (m_Type)
    {
    case eDATA_Sky:
        return "Sky";
    case eDATA_Beam:
        return "Beam";
    case eDATA_ClientPoly:
        return "ClientPoly";
    case eDATA_Flare:
        return "Flare";
    case eDATA_Terrain:
        return "Terrain";
    case eDATA_SkyZone:
        return "SkyZone";
    case eDATA_Mesh:
        return "Mesh";
    case eDATA_Imposter:
        return "Imposter";
    case eDATA_LensOptics:
        return "LensOptics";
    case eDATA_OcclusionQuery:
        return "OcclusionQuery";
    case eDATA_Particle:
        return "Particle";
    case eDATA_GPUParticle:
        return "GPUParticle";
    case eDATA_PostProcess:
        return "PostProcess";
    case eDATA_HDRProcess:
        return "HDRProcess";
    case eDATA_Cloud:
        return "Cloud";
    case eDATA_HDRSky:
        return "HDRSky";
    case eDATA_FogVolume:
        return "FogVolume";
    case eDATA_WaterVolume:
        return "WaterVolume";
    case eDATA_WaterOcean:
        return "WaterOcean";
    case eDATA_VolumeObject:
        return "VolumeObject";
    case eDATA_PrismObject:
        return "PrismObject";
    case eDATA_DeferredShading:
        return "DeferredShading";
    case eDATA_GameEffect:
        return "GameEffect";
    case eDATA_BreakableGlass:
        return "BreakableGlass";
    case eDATA_GeomCache:
        return "GeomCache";
    case eDATA_Gem:
        return "Gem";
    default:
    {
        CRY_ASSERT(false);
        return "Unknown";
    }
    }
}

CRendElementBase* CRendElementBase::mfCopyConstruct(void)
{
    CRendElementBase* re = new CRendElementBase;
    *re = *this;
    return re;
}
void CRendElementBase::mfCenter(Vec3& centr, [[maybe_unused]] CRenderObject* pObj)
{
    centr(0, 0, 0);
}
void CRendElementBase::mfGetPlane(Plane& pl)
{
    pl.n = Vec3(0, 0, 1);
    pl.d = 0;
}

//=============================================================================

void* SRendItem::mfGetPointerCommon(ESrcPointer ePT, int* Stride, [[maybe_unused]] EParamType Type, [[maybe_unused]] ESrcPointer Dst, [[maybe_unused]] int Flags)
{
    int j;
    switch (ePT)
    {
    case eSrcPointer_Vert:
        *Stride = gRenDev->m_RP.m_StreamStride;
        return gRenDev->m_RP.m_StreamPtr.PtrB;

    case eSrcPointer_Color:
        *Stride = gRenDev->m_RP.m_StreamStride;
        return gRenDev->m_RP.m_StreamPtr.PtrB + gRenDev->m_RP.m_StreamOffsetColor;

    case eSrcPointer_Tex:
    case eSrcPointer_TexLM:
        *Stride = gRenDev->m_RP.m_StreamStride;
        j = ePT - eSrcPointer_Tex;
        return gRenDev->m_RP.m_StreamPtr.PtrB + gRenDev->m_RP.m_StreamOffsetTC + j * 16;
    }
    return NULL;
}

