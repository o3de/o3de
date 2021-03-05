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
#include "../Common/Textures/TextureStreamPool.h"

#include "DriverD3D.h"

#if !defined(CHK_RENDTH)
#   define CHK_RENDTH assert(gRenDev->m_pRT->IsRenderThread())
#endif
#if !defined(CHK_MAINTH)
#   define CHK_MAINTH assert(gRenDev->m_pRT->IsMainThread())
#endif
#if !defined(CHK_MAINORRENDTH)
#   define CHK_MAINORRENDTH assert(gRenDev->m_pRT->IsMainThread() || gRenDev->m_pRT->IsRenderThread())
#endif

CryCriticalSection STexPoolItemHdr::s_sSyncLock;

bool STexPoolItem::IsStillUsedByGPU(uint32 nTick)
{
    CDeviceTexture* pDeviceTexture = m_pDevTexture;
    if (pDeviceTexture)
    {
        CHK_MAINORRENDTH;
        D3DBaseTexture* pD3DTex = pDeviceTexture->GetBaseTexture();
    }
    return (nTick - m_nFreeTick) < 4;
}

STexPoolItem::STexPoolItem (STexPool* pOwner, CDeviceTexture* pDevTexture, size_t devSize)
    : m_pOwner(pOwner)
    , m_pTex(NULL)
    , m_pDevTexture(pDevTexture)
    , m_nDeviceTexSize(devSize)
    , m_nFreeTick(0)
    , m_nActiveLod(0)
{
    assert (pDevTexture);
    ++pOwner->m_nItems;
}

STexPoolItem::~STexPoolItem()
{
    assert (m_pDevTexture);
    if (m_pOwner)
    {
        --m_pOwner->m_nItems;
        if (IsFree())
        {
            --m_pOwner->m_nItemsFree;
        }
    }
    Unlink();
    UnlinkFree();

    m_pDevTexture->Unbind();
    m_pDevTexture->Release();
}

CTextureStreamPoolMgr::CTextureStreamPoolMgr()
{
    m_FreeTexPoolItems.m_NextFree = &m_FreeTexPoolItems;
    m_FreeTexPoolItems.m_PrevFree = &m_FreeTexPoolItems;

    m_nDeviceMemReserved = 0;
    m_nDeviceMemInUse = 0;

#ifdef TEXSTRM_USE_FREEPOOL
    m_nFreePoolBegin = 0;
    m_nFreePoolEnd = 0;
#endif

    m_nTick = 0;

#if !defined(_RELEASE)
    m_bComputeStats = false;
#endif
}

CTextureStreamPoolMgr::~CTextureStreamPoolMgr()
{
    Flush();
}

void CTextureStreamPoolMgr::Flush()
{
    AUTO_LOCK(STexPoolItem::s_sSyncLock);

    FlushFree();

    // Free pools first, as that will attempt to remove streamed textures in release, which will end up
    // in the free list.

    for (TexturePoolMap::iterator it = m_TexturesPools.begin(), itEnd = m_TexturesPools.end(); it != itEnd; ++it)
    {
        SAFE_DELETE(it->second);
    }
    stl::free_container(m_TexturesPools);

    // Now nuke the free items.

    FlushFree();
#ifdef TEXSTRM_USE_FREEPOOL
    while (m_nFreePoolBegin != m_nFreePoolEnd)
    {
        assert (m_nFreePoolEnd < sizeof(m_FreePool) / sizeof(m_FreePool[0]));
        void* p = m_FreePool[m_nFreePoolBegin];
        m_nFreePoolBegin = (m_nFreePoolBegin + 1) % MaxFreePool;
        ::operator delete(p);
    }
#endif
}

STexPool* CTextureStreamPoolMgr::GetPool(int nWidth, int nHeight, int nMips, int nArraySize, ETEX_Format eTF, bool bIsSRGB, ETEX_Type eTT)
{
    D3DFormat d3dFmt = CTexture::DeviceFormatFromTexFormat(eTF);
    if (bIsSRGB)
    {
        d3dFmt = CTexture::ConvertToSRGBFmt(d3dFmt);
    }

    TexturePoolKey poolKey((uint16)nWidth, (uint16)nHeight, d3dFmt, (uint8)eTT, (uint8)nMips, (uint16)nArraySize);

    TexturePoolMap::iterator it = m_TexturesPools.find(poolKey);
    if (it != m_TexturesPools.end())
    {
        return it->second;
    }

    return NULL;
}

STexPoolItem* CTextureStreamPoolMgr::GetPoolItem(int nWidth, int nHeight, int nMips, int nArraySize, ETEX_Format eTF, bool bIsSRGB, ETEX_Type eTT, bool bShouldBeCreated, const char* sName, STextureInfo* pTI, bool bCanCreate, bool bWaitForIdle)
{
    D3DFormat d3dFmt = CTexture::DeviceFormatFromTexFormat(eTF);
    if (bIsSRGB)
    {
        d3dFmt = CTexture::ConvertToSRGBFmt(d3dFmt);
    }

    STexPool* pPool = CreatePool(nWidth, nHeight, nMips, nArraySize, d3dFmt, eTT);
    if (!pPool)
    {
        return NULL;
    }

    AUTO_LOCK(STexPoolItem::s_sSyncLock);

    STexPoolItemHdr* pITH = &m_FreeTexPoolItems;
    STexPoolItem* pIT = static_cast<STexPoolItem*>(pITH);

    bool bFoundACoolingMatch = false;

#ifndef TSP_GC_ALL_ITEMS

    if (!pTI)
    {
        pITH = m_FreeTexPoolItems.m_PrevFree;
        pIT = static_cast<STexPoolItem*>(pITH);

        // Try to find empty item in the pool
        while (pITH != &m_FreeTexPoolItems)
        {
            if (pIT->m_pOwner == pPool)
            {
                if (!bWaitForIdle || !pIT->IsStillUsedByGPU(m_nTick))
                {
                    break;
                }

                bFoundACoolingMatch = true;
            }

            pITH = pITH->m_PrevFree;
            pIT = static_cast<STexPoolItem*>(pITH);
        }
    }

#endif

    if (pITH != &m_FreeTexPoolItems)
    {
        pIT->UnlinkFree();
#if defined(DO_RENDERLOG)
        if IsCVarConstAccess(constexpr) (CRenderer::CV_r_logTexStreaming == 2)
        {
            gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Remove from FreePool '%s', [%d x %d], Size: %d\n", sName, pIT->m_pOwner->m_Width, pIT->m_pOwner->m_Height, pIT->m_pOwner->m_Size);
        }
#endif

#if !defined(_RELEASE)
        ++m_frameStats.nSoftCreates;
#endif

        --pIT->m_pOwner->m_nItemsFree;
    }
    else
    {
        if (!bCanCreate)
        {
            return NULL;
        }

#if defined(TEXSTRM_TEXTURECENTRIC_MEMORY)
        if (bFoundACoolingMatch)
        {
            // Really, really, don't want to create a texture if one will be available soon
            if (!bShouldBeCreated)
            {
                return NULL;
            }
        }
#endif

        // Create API texture for the item in DEFAULT pool
        const char* poolTextureName = "StreamingTexturePool";

        HRESULT h = S_OK;
        CDeviceTexture* pDevTex = NULL;
        if (eTT != eTT_Cube)
        {
            h = gcpRendD3D->m_DevMan.Create2DTexture(poolTextureName, nWidth, nHeight, nMips, nArraySize, STREAMED_TEXTURE_USAGE, Clr_Transparent, d3dFmt, D3DPOOL_DEFAULT, &pDevTex, pTI, bShouldBeCreated);
        }
        else
        {
            h = gcpRendD3D->m_DevMan.CreateCubeTexture(poolTextureName, nWidth, nMips, 1, STREAMED_TEXTURE_USAGE, Clr_Transparent, d3dFmt, D3DPOOL_DEFAULT, &pDevTex, pTI, bShouldBeCreated);
        }
        if (FAILED(h) || !pDevTex)
        {
            return NULL;
        }

#ifdef TEXSTRM_USE_FREEPOOL
        if (m_nFreePoolBegin != m_nFreePoolEnd)
        {
            assert (m_nFreePoolBegin < (sizeof(m_FreePool) / sizeof(m_FreePool[0])));

            void* p = m_FreePool[m_nFreePoolBegin];
            m_nFreePoolBegin = (m_nFreePoolBegin + 1) % MaxFreePool;
            pIT = new (p) STexPoolItem(pPool, pDevTex, pPool->m_Size);
        }
        else
#endif
        {
            pIT = new STexPoolItem(pPool, pDevTex, pPool->m_Size);
        }

        pIT->Link(&pPool->m_ItemsList);
        CryInterlockedAddSize(&m_nDeviceMemReserved, (ptrdiff_t)pPool->m_Size);

#if !defined(_RELEASE)
        ++m_frameStats.nHardCreates;
#endif
    }

    CryInterlockedAddSize(&m_nDeviceMemInUse, (ptrdiff_t)pIT->m_nDeviceTexSize);

    return pIT;
}

void CTextureStreamPoolMgr::ReleaseItem(STexPoolItem* pItem)
{
    assert(!pItem->m_NextFree);

#ifdef DO_RENDERLOG
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_logTexStreaming == 2)
    {
        const char* name = pItem->m_pTex ? pItem->m_pTex->GetSourceName() : "";
        gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Add to FreePool '%s', [%d x %d], Size: %d\n", name, pItem->m_pOwner->m_Width, pItem->m_pOwner->m_Height, pItem->m_pOwner->m_Size);
    }
#endif

    CryInterlockedAddSize(&m_nDeviceMemInUse, -(ptrdiff_t)pItem->m_nDeviceTexSize);

    pItem->m_pTex = NULL;
    pItem->m_nFreeTick = m_nTick;
    pItem->LinkFree(&m_FreeTexPoolItems);

    ++pItem->m_pOwner->m_nItemsFree;

#if !defined(_RELEASE)
    ++m_frameStats.nSoftFrees;
#endif
}

STexPool* CTextureStreamPoolMgr::CreatePool(int nWidth, int nHeight, int nMips, int nArraySize, D3DFormat eTF, ETEX_Type eTT)
{
    STexPool* pPool = NULL;

    TexturePoolKey poolKey((uint16)nWidth, (uint16)nHeight, eTF, (uint8)eTT, (uint8)nMips, (uint16)nArraySize);

    TexturePoolMap::iterator it = m_TexturesPools.find(poolKey);
    if (it == m_TexturesPools.end())
    {
        // Create new pool
        pPool = new STexPool;
        pPool->m_eTT = eTT;
        pPool->m_eFormat = eTF;
        pPool->m_Width = nWidth;
        pPool->m_Height = nHeight;
        pPool->m_nArraySize = nArraySize;
        pPool->m_nMips = nMips;
        pPool->m_Size = CDeviceTexture::TextureDataSize(nWidth, nHeight, 1, nMips, nArraySize * (eTT == eTT_Cube ? 6 : 1), CTexture::TexFormatFromDeviceFormat(eTF));

        pPool->m_nItems = 0;
        pPool->m_nItemsFree = 0;

        pPool->m_ItemsList.m_Next = &pPool->m_ItemsList;
        pPool->m_ItemsList.m_Prev = &pPool->m_ItemsList;

        it = m_TexturesPools.insert(std::make_pair(poolKey, pPool)).first;
    }

    return it->second;
}

void CTextureStreamPoolMgr::FlushFree()
{
    STexPoolItemHdr* pITH = m_FreeTexPoolItems.m_PrevFree;
    while (pITH != &m_FreeTexPoolItems)
    {
        STexPoolItemHdr* pITHNext = pITH->m_PrevFree;
        STexPoolItem* pIT = static_cast<STexPoolItem*>(pITH);

        assert (!pIT->m_pTex);
        CryInterlockedAddSize(&m_nDeviceMemReserved, -(ptrdiff_t)pIT->m_nDeviceTexSize);

#ifdef TEXSTRM_USE_FREEPOOL
        unsigned int nFreePoolSize = (m_nFreePoolEnd - m_nFreePoolBegin) % MaxFreePool;
        if (nFreePoolSize < MaxFreePool - 1) // -1 to avoid needing a separate count
        {
            pIT->~STexPoolItem();
            m_FreePool[m_nFreePoolEnd] = pIT;
            m_nFreePoolEnd = (m_nFreePoolEnd + 1) % MaxFreePool;
        }
        else
#endif
        {
            delete pIT;
        }

        pITH = pITHNext;
    }
}

void CTextureStreamPoolMgr::GarbageCollect(size_t* nCurTexPoolSize, size_t nLowerPoolLimit, int nMaxItemsToFree)
{
    size_t nSize = nCurTexPoolSize ? *nCurTexPoolSize : 1024 * 1024 * 1024;
    ptrdiff_t nFreed = 0;

    AUTO_LOCK(STexPoolItem::s_sSyncLock);

    STexPoolItemHdr* pITH = m_FreeTexPoolItems.m_PrevFree;
    while (pITH != &m_FreeTexPoolItems)
    {
        STexPoolItemHdr* pITHNext = pITH->m_PrevFree;

        STexPoolItem* pIT = static_cast<STexPoolItem*>(pITH);

        if (!pIT->m_pTex)
        {
            pITH = pITHNext;
            continue;
        }

#if defined(TEXSTRM_TEXTURECENTRIC_MEMORY)
        if (pIT->m_pOwner->m_nItemsFree > 20 || pIT->m_pOwner->m_Width > 64 || pIT->m_pOwner->m_Height > 64)
#endif
        {
            if (!pIT->IsStillUsedByGPU(m_nTick))
            {
                nSize -= pIT->m_nDeviceTexSize;
                nFreed += (ptrdiff_t)pIT->m_nDeviceTexSize;

#ifdef TEXSTRM_USE_FREEPOOL
                unsigned int nFreePoolSize = (m_nFreePoolEnd - m_nFreePoolBegin) % MaxFreePool;
                if (nFreePoolSize < MaxFreePool - 1) // -1 to avoid needing a separate count
                {
                    pIT->~STexPoolItem();
                    m_FreePool[m_nFreePoolEnd] = pIT;
                    m_nFreePoolEnd = (m_nFreePoolEnd + 1) % MaxFreePool;
                }
                else
#endif
                {
                    delete pIT;
                }

                --nMaxItemsToFree;

#if !defined(_RELEASE)
                ++m_frameStats.nHardFrees;
#endif
            }
        }

        pITH = pITHNext;

        // we release all textures immediately on consoles
#ifndef TSP_GC_ALL_ITEMS
        if (nMaxItemsToFree <= 0 || nSize < nLowerPoolLimit)
        {
            break;
        }
#endif
    }

    CryInterlockedAddSize(&m_nDeviceMemReserved, -nFreed);

    CTexture::StreamValidateTexSize();

    if (nCurTexPoolSize)
    {
        *nCurTexPoolSize = nSize;
    }

#if !defined(_RELEASE)
    if (m_bComputeStats)
    {
        CryAutoLock<CryCriticalSection> lock(m_statsLock);

        m_poolStats.clear();

        for (TexturePoolMap::iterator it = m_TexturesPools.begin(), itEnd = m_TexturesPools.end(); it != itEnd; ++it)
        {
            STexPool* pPool = it->second;
            SPoolStats ps;
            ps.nWidth = pPool->m_Width;
            ps.nHeight = pPool->m_Height;
            ps.nMips = pPool->m_nMips;
            ps.nFormat = pPool->m_eFormat;
            ps.eTT = pPool->m_eTT;

            ps.nInUse = pPool->m_nItems - pPool->m_nItemsFree;
            ps.nFree = pPool->m_nItemsFree;
            ps.nHardCreatesPerFrame = 0;
            ps.nSoftCreatesPerFrame = 0;

            m_poolStats.push_back(ps);
        }
    }
#endif

    ++m_nTick;
}

void CTextureStreamPoolMgr::GetMemoryUsage(ICrySizer* pSizer)
{
    SIZER_COMPONENT_NAME(pSizer, "Texture Pools");
    pSizer->AddObject(m_TexturesPools);
}
