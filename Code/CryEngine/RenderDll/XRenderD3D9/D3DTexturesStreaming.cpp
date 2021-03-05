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
#include "StringUtils.h"
#include <AzCore/Debug/Profiler.h>

#include "../Common/Textures/TextureStreamPool.h"

// checks for MT-safety of called functions
#define D3D_CHK_RENDTH assert(gcpRendD3D->m_pRT->IsRenderThread())
#define D3D_CHK_MAINTH assert(gcpRendD3D->m_pRT->IsMainThread())
#define D3D_CHK_MAINORRENDTH assert(gRenDev->m_pRT->IsMainThread() || gRenDev->m_pRT->IsRenderThread())

void CTexture::InitStreamingDev()
{

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DTEXTURESSTREAMING_CPP_SECTION_1 1
#define D3DTEXTURESSTREAMING_CPP_SECTION_2 2
#define D3DTEXTURESSTREAMING_CPP_SECTION_3 3
#define D3DTEXTURESSTREAMING_CPP_SECTION_4 4
#define D3DTEXTURESSTREAMING_CPP_SECTION_5 5
#define D3DTEXTURESSTREAMING_CPP_SECTION_6 6
#define D3DTEXTURESSTREAMING_CPP_SECTION_7 7
#endif

#if defined(TEXSTRM_DEFERRED_UPLOAD)
    if (CRenderer::CV_r_texturesstreamingDeferred)
    {
        if (!s_pStreamDeferredCtx)
        {
            gcpRendD3D->GetDevice().CreateDeferredContext(0, &s_pStreamDeferredCtx);
        }
    }
#endif
}

bool CTexture::IsStillUsedByGPU()
{
    CDeviceTexture* pDeviceTexture = m_pDevTexture;
    if (pDeviceTexture)
    {
        D3D_CHK_RENDTH;
        D3DBaseTexture* pD3DTex = pDeviceTexture->GetBaseTexture();
    }
    return false;
}

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURESSTREAMING_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DTexturesStreaming_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else

bool CTexture::StreamPrepare_Platform()
{
    return true;
}

#endif


#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURESSTREAMING_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DTexturesStreaming_cpp)
#endif

void CTexture::StreamExpandMip(const void* vpRawData, int nMip, int nBaseMipOffset, int nSideDelta)
{
    FUNCTION_PROFILER_RENDERER;

    const uint32 nCurMipWidth = (m_nWidth >> (nMip + nBaseMipOffset));
    const uint32 nCurMipHeight = (m_nHeight >> (nMip + nBaseMipOffset));

    const STexMipHeader& mh = m_pFileTexMips->m_pMipHeader[nBaseMipOffset + nMip];

    const byte* pRawData = (const byte*)vpRawData;

    const int nSides = StreamGetNumSlices();
    const Vec2i vMipAlign = CTexture::GetBlockDim(m_eTFDst);

    const int nSrcSurfaceSize = CTexture::TextureDataSize(nCurMipWidth, nCurMipHeight, 1, 1, 1, m_eTFSrc, m_eSrcTileMode);
    const int nSrcSidePitch = nSrcSurfaceSize + nSideDelta;

    SRenderThread* pRT = gRenDev->m_pRT;
    if (mh.m_Mips && mh.m_SideSize > 0)
    {
        for (int iSide = 0; iSide < nSides; ++iSide)
        {
            SMipData* mp = &mh.m_Mips[iSide];
            if (mp)
            {
                if (!mp->DataArray)
                {
                    mp->Init(mh.m_SideSize, Align(max(1, m_nWidth >> nMip), vMipAlign.x), Align(max(1, m_nHeight >> nMip), vMipAlign.y));
                }

                const byte* pRawSideData = pRawData + nSrcSidePitch * iSide;
                CTexture::ExpandMipFromFile(&mp->DataArray[0], mh.m_SideSize, pRawSideData, nSrcSurfaceSize, m_eTFSrc);
            }
        }
    }
}

#ifdef TEXSTRM_ASYNC_TEXCOPY
void STexStreamOutState::CopyMips()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::System);

    CTexture* tp = m_pTexture;

    if (m_nStartMip < MAX_MIP_LEVELS)
    {
        const int nOldMipOffset = m_nStartMip - tp->m_nMinMipVidUploaded;
        const int nNumMips = tp->GetNumMipsNonVirtual() - m_nStartMip;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURESSTREAMING_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(D3DTexturesStreaming_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        CTexture::StreamCopyMipsTexToTex(tp->m_pFileTexMips->m_pPoolItem, 0 + nOldMipOffset, m_pNewPoolItem, 0, nNumMips);
#endif
    }
    else
    {
        // Stream unload case - pull persistent mips into local memory
        tp->StreamCopyMipsTexToMem(tp->m_nMips - tp->m_CacheFileHeader.m_nMipsPersistent, tp->m_nMips - 1, false, NULL);
    }

    m_bDone = true;
}
#endif

int CTexture::StreamTrim(int nToMip)
{
    FUNCTION_PROFILER_RENDERER;
    D3D_CHK_RENDTH;

    if (IsUnloaded() || !IsStreamed() || IsStreaming())
    {
        return 0;
    }

    // clamp mip level
    nToMip = max(0, min(nToMip, m_nMips - m_CacheFileHeader.m_nMipsPersistent));

    if (m_nMinMipVidUploaded >= nToMip)
    {
        return 0;
    }

    int nFreeSize = StreamComputeDevDataSize(m_nMinMipVidUploaded) - StreamComputeDevDataSize(nToMip);

#ifndef _RELEASE
    if (CRenderer::CV_r_TexturesStreamingDebug == 2)
    {
        iLog->Log("Shrinking texture: %s - From mip: %i, To mip: %i", m_SrcName.c_str(), m_nMinMipVidUploaded, GetRequiredMipNonVirtual());
    }
#endif

    STexPoolItem* pNewPoolItem = StreamGetPoolItem(nToMip, m_nMips - nToMip, false, false, true, true);
    assert(pNewPoolItem != m_pFileTexMips->m_pPoolItem);
    if (pNewPoolItem)
    {
        const int nOldMipOffset = nToMip - m_nMinMipVidUploaded;
        const int nNumMips = GetNumMipsNonVirtual() - nToMip;

#ifdef TEXSTRM_ASYNC_TEXCOPY

        bool bCopying = false;

        if (CanAsyncCopy() && (TryAddRef() > 0))
        {
            STexStreamOutState* pStreamState = StreamState_AllocateOut();
            if (pStreamState)
            {
                pStreamState->m_nStartMip = nToMip;
                pStreamState->m_pNewPoolItem = pNewPoolItem;
                pStreamState->m_pTexture = this;

                SetStreamingInProgress(StreamOutMask | (uint8)s_StreamOutTasks.GetIdxFromPtr(pStreamState));

                pStreamState->m_jobExecutor.StartJob([pStreamState]()
                {
                    pStreamState->CopyMips();
                });

                bCopying = true;

#ifdef DO_RENDERLOG
                if (gRenDev->m_logFileStrHandle != AZ::IO::InvalidHandle)
                {
                    gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Async Start SetLod '%s', Lods: [%d-%d], Time: %.3f\n", m_SrcName.c_str(), nToMip, m_nMips - 1, iTimer->GetAsyncCurTime());
                }
#endif
            }
            else
            {
                Release();
            }
        }

        if (!bCopying)
#endif
        {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURESSTREAMING_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(D3DTexturesStreaming_cpp)
#endif
            // it is a sync operation anyway, so we do it in the render thread
            CTexture::StreamCopyMipsTexToTex(m_pFileTexMips->m_pPoolItem, 0 + nOldMipOffset, pNewPoolItem, 0, nNumMips);
            StreamAssignPoolItem(pNewPoolItem, nToMip);
        }
    }
    else
    {
        s_pTextureStreamer->FlagOutOfMemory();
    }

    return nFreeSize;
}


int CTexture::StreamUnload()
{
    D3D_CHK_RENDTH;

    if (IsUnloaded() || !IsStreamed() || !CRenderer::CV_r_texturesstreaming)
    {
        return 0;
    }

    AbortStreamingTasks(this);
    assert(!IsStreaming());

    int nDevSize = m_nActualSize;

#ifdef TEXSTRM_ASYNC_TEXCOPY
    bool bCopying = false;

    if (CanAsyncCopy() && (TryAddRef() > 0))
    {
        STexStreamOutState* pStreamState = StreamState_AllocateOut();
        if (pStreamState)
        {
            pStreamState->m_pTexture = this;
            pStreamState->m_nStartMip = MAX_MIP_LEVELS;

            SetStreamingInProgress(StreamOutMask | (uint8)s_StreamOutTasks.GetIdxFromPtr(pStreamState));

            pStreamState->m_jobExecutor.StartJob([pStreamState]()
            {
                pStreamState->CopyMips();
            });

            bCopying = true;
        }
        else
        {
            Release();
        }
    }

    if (!bCopying)
#endif
    {
        // copy old mips to system memory
        StreamCopyMipsTexToMem(m_nMips - m_CacheFileHeader.m_nMipsPersistent, m_nMips - 1, false, NULL);
        ReleaseDeviceTexture(true);
        SetWasUnload(true);
    }

#ifndef _RELEASE
    if (CRenderer::CV_r_TexturesStreamingDebug == 2)
    {
        iLog->Log("Unloading unused texture: %s", m_SrcName.c_str());
    }
#endif

    return nDevSize;
}

void CTexture::StreamActivateLod(int nMinMip)
{
    FUNCTION_PROFILER_RENDERER;

    STexPoolItem* pItem = m_pFileTexMips->m_pPoolItem;
    STexPool* pPool = pItem->m_pOwner;
    int nMipOffset = m_nMips - pPool->m_nMips;
    int nDevMip = min((int)pPool->m_nMips - 1, max(0, nMinMip - nMipOffset));

    if (pItem->m_nActiveLod != nDevMip)
    {
        pItem->m_nActiveLod = nDevMip;

        //D3DDevice* dv = gcpRendD3D->GetD3DDevice();
        //gcpRendD3D->GetDeviceContext().SetResourceMinLOD(m_pDevTexture->GetBaseTexture(), (FLOAT)nDevMip);
    }

    m_nMinMipVidActive = nMinMip;
}

void CTexture::StreamCopyMipsTexToMem(int nStartMip, int nEndMip, bool bToDevice, STexPoolItem* pNewPoolItem)
{
    PROFILE_FRAME(Texture_StreamUpload);

    CD3D9Renderer* r = gcpRendD3D;
    CDeviceManager* pDevMan = &r->m_DevMan;
    HRESULT h = S_OK;
    nEndMip =   min(nEndMip + 1, (int)m_nMips) - 1;//+1 -1 needed as the compare is <=
    STexMipHeader* mh = m_pFileTexMips->m_pMipHeader;

    const Vec2i vMipAlign = CTexture::GetBlockDim(m_eTFDst);

    const int nOldMinMipVidUploaded = m_nMinMipVidUploaded;

    if (bToDevice && !pNewPoolItem)
    {
        SetMinLoadedMip(nStartMip);
    }

    D3DFormat fmt = DeviceFormatFromTexFormat(GetDstFormat());
    if (m_bIsSRGB)
    {
        fmt = ConvertToSRGBFmt(fmt);
    }

    CDeviceTexture* pDevTexture = m_pDevTexture;
    uint32 nTexMips = m_nMips;
    if (m_pFileTexMips->m_pPoolItem)
    {
        assert(m_pFileTexMips->m_pPoolItem->m_pDevTexture);
        assert(pDevTexture == m_pFileTexMips->m_pPoolItem->m_pDevTexture);
        nTexMips = m_pFileTexMips->m_pPoolItem->m_pOwner->m_nMips;
    }
    if (bToDevice && pNewPoolItem)
    {
        assert(pNewPoolItem->m_pDevTexture);
        pDevTexture = pNewPoolItem->m_pDevTexture;
        nTexMips = pNewPoolItem->m_pOwner->m_nMips;
    }

    if (!pDevTexture)
    {
        if (m_eTT != eTT_Cube)
        {
            h = pDevMan->Create2DTexture(m_SrcName, m_nWidth, m_nHeight, m_nMips, m_nArraySize, STREAMED_TEXTURE_USAGE, m_cClearColor, fmt, (D3DPOOL)0, &pDevTexture);
        }
        else
        {
            h = pDevMan->CreateCubeTexture(m_SrcName, m_nWidth, m_nMips, 1, STREAMED_TEXTURE_USAGE, m_cClearColor, fmt, (D3DPOOL)0, &pDevTexture);
        }
        assert(h == S_OK);

        // If a pool item was provided, it would have a valid dev texture, so we shouldn't have ended up here..
        assert (!bToDevice || !pNewPoolItem);
        SetDevTexture(pDevTexture);
    }

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_texturesstreamingnoupload && bToDevice)
    {
        return;
    }

    const int nMipOffset = m_nMips - nTexMips;
    const int32 nSides = StreamGetNumSlices();

    D3DBaseTexture* pID3DTexture = pDevTexture->GetBaseTexture();

    int SizeToLoad = 0;
    for (int32 iSide = 0; iSide < nSides; ++iSide)
    {
        const int32 iSideLockIndex = m_eTT != eTT_Cube ? -1 : iSide;
        for (int nLod = nStartMip; nLod <= nEndMip; nLod++)
        {
            SMipData* mp = &mh[nLod].m_Mips[iSide];
            int nMipW = m_nWidth >> nLod;
            int nMipH = m_nHeight >> nLod;

            if (bToDevice && !mp->DataArray && s_bStreamDontKeepSystem)  // we have this mip already loaded
            {
                continue;
            }

            const int nDevTexMip = nLod - nMipOffset;

            if (bToDevice)
            {
                if (mp->DataArray)
                {
                    CryInterlockedAdd(&CTexture::s_nTexturesDataBytesUploaded, mh[nLod].m_SideSize);
                    const int nRowPitch = CTexture::TextureDataSize(nMipW, 1, 1, 1, 1, m_eTFDst);
                    const int nSlicePitch = CTexture::TextureDataSize(nMipW, nMipH, 1, 1, 1, m_eTFDst);
                    STALL_PROFILER("update texture");
                    {
                        gcpRendD3D->GetDeviceContext().UpdateSubresource(pID3DTexture, D3D11CalcSubresource(nDevTexMip, iSide, nTexMips), NULL, &mp->DataArray[0], nRowPitch, nSlicePitch);
                    }
                }
                else
                {
                    assert(0);
                }
            }
            else
            {
                const int nMipSize = mh[nLod].m_SideSize;
                mp->Init(nMipSize, nMipW, nMipH);
                const int nRowPitch = CTexture::TextureDataSize(nMipW, 1, 1, 1, 1, m_eTFDst);
                const int nRows = nMipSize / nRowPitch;
                assert(nMipSize % nRowPitch == 0);

                STALL_PROFILER("update texture");
                pDevTexture->DownloadToStagingResource(D3D11CalcSubresource(nDevTexMip, iSide, nTexMips), [&](void* pData, uint32 rowPitch, [[maybe_unused]] uint32 slicePitch)
                {
                    for (int iRow = 0; iRow < nRows; ++iRow)
                    {
                        memcpy(&mp->DataArray[iRow * nRowPitch], (byte*)pData + rowPitch * iRow, nRowPitch);
                    }
                    return true;
                });
                // mark as native
                mp->m_bNative = true;
            }
            SizeToLoad += m_pFileTexMips->m_pMipHeader[nLod].m_SideSize;

            if (s_bStreamDontKeepSystem && bToDevice)
            {
                mp->Free();
            }
        }
    }
#ifdef DO_RENDERLOG
    if (gRenDev->m_logFileStrHandle != AZ::IO::InvalidHandle)
    {
        gRenDev->LogStrv(SRendItem::m_RecurseLevel[gRenDev->m_RP.m_nProcessThreadID], "Uploading mips '%s'. (%d[%d]), Size: %d, Time: %.3f\n", m_SrcName.c_str(), nStartMip, m_nMips, SizeToLoad, iTimer->GetAsyncCurTime());
    }
#endif
}

#if defined(TEXSTRM_DEFERRED_UPLOAD)

ID3D11CommandList* CTexture::StreamCreateDeferred(int nStartMip, int nEndMip, STexPoolItem* pNewPoolItem, STexPoolItem* pSrcPoolItem)
{
    PROFILE_FRAME(Texture_StreamCreateDeferred);

    if ((pNewPoolItem == nullptr) || (pSrcPoolItem == nullptr))
    {
        AZ_Warning("CTexture", false, "CTexture::StreamCreateDeferred called with pNewPoolItem = %p and pSrcPoolItem = %p, command list will not be created", pNewPoolItem, pSrcPoolItem);
        return nullptr;
    }

    ID3D11CommandList* pCmdList = nullptr;

    if (CTexture::s_pStreamDeferredCtx)
    {
        CD3D9Renderer* r = gcpRendD3D;
        CDeviceManager* pDevMan = &r->m_DevMan;
        HRESULT h = S_OK;
        nEndMip =   min(nEndMip + 1, (int)m_nMips) - 1;//+1 -1 needed as the compare is <=
        STexMipHeader* mh = m_pFileTexMips->m_pMipHeader;

        const int nOldMinMipVidUploaded = m_nMinMipVidUploaded;

        D3DFormat fmt = DeviceFormatFromTexFormat(GetDstFormat());
        if (m_bIsSRGB)
        {
            fmt = ConvertToSRGBFmt(fmt);
        }

        CDeviceTexture* pDevTexture = pNewPoolItem->m_pDevTexture;
        uint32 nTexMips = pNewPoolItem->m_pOwner->m_nMips;

        const int nMipOffset = m_nMips - nTexMips;
        const int32 nSides = StreamGetNumSlices();

        const Vec2i vMipAlign = GetBlockDim(m_eTFSrc);

        D3DBaseTexture* pID3DTexture = pDevTexture->GetBaseTexture();

        int SizeToLoad = 0;
        for (int32 iSide = 0; iSide < nSides; ++iSide)
        {
            const int32 iSideLockIndex = m_eTT != eTT_Cube ? -1 : iSide;
            for (int nLod = nStartMip; nLod <= nEndMip; nLod++)
            {
                SMipData* mp = &mh[nLod].m_Mips[iSide];

                if (mp->DataArray)
                {
                    const int nDevTexMip = nLod - nMipOffset;

                    CryInterlockedAdd(&CTexture::s_nTexturesDataBytesUploaded, mh[nLod].m_SideSize);
                    const int nUSize = Align(max(1, m_nWidth >> nLod), vMipAlign.x);
                    const int nVSize = Align(max(1, m_nHeight >> nLod), vMipAlign.y);
                    const int nRowPitch = CTexture::TextureDataSize(nUSize, 1, 1, 1, 1, m_eTFDst);
                    const int nSlicePitch = CTexture::TextureDataSize(nUSize, nVSize, 1, 1, 1, m_eTFDst);
                    STALL_PROFILER("update texture");
                    {
                        s_pStreamDeferredCtx->UpdateSubresource(pID3DTexture, D3D11CalcSubresource(nDevTexMip, iSide, nTexMips), NULL, &mp->DataArray[0], nRowPitch, nSlicePitch);
                    }

                    SizeToLoad += m_pFileTexMips->m_pMipHeader[nLod].m_SideSize;

                    if (s_bStreamDontKeepSystem)
                    {
                        mp->Free();
                    }
                }
            }
        }

        int nMipsSrc = pSrcPoolItem->m_pOwner->m_nMips;
        int nMipsDst = pNewPoolItem->m_pOwner->m_nMips;
        int nMipSrcOffset = m_nMips - nMipsSrc;
        int nMipDstOffset = m_nMips - nMipsDst;

        for (int32 iSide = 0; iSide < nSides; ++iSide)
        {
            for (int i = nEndMip + 1; i < m_nMips; ++i)
            {
                s_pStreamDeferredCtx->CopySubresourceRegion(
                    pID3DTexture,
                    D3D11CalcSubresource(i - nMipDstOffset, iSide, nMipsDst),
                    0, 0, 0, pSrcPoolItem->m_pDevTexture->GetBaseTexture(),
                    D3D11CalcSubresource(i - nMipSrcOffset, iSide, nMipsSrc),
                    NULL);
            }
        }

        s_pStreamDeferredCtx->FinishCommandList(FALSE, &pCmdList);
    }

    return pCmdList;
}

void CTexture::StreamApplyDeferred(ID3D11CommandList* pCmdList)
{
    FUNCTION_PROFILER_RENDERER;
    gcpRendD3D->GetDeviceContext().ExecuteCommandList(pCmdList, TRUE);
}

#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURESSTREAMING_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(D3DTexturesStreaming_cpp)
#endif

// Just remove item from the texture object and keep Item in Pool list for future use
// This function doesn't release API texture
void CTexture::StreamRemoveFromPool()
{
    D3D_CHK_MAINORRENDTH;

    if (!m_pFileTexMips || !m_pFileTexMips->m_pPoolItem)
    {
        return;
    }

    AUTO_LOCK(STexPoolItem::s_sSyncLock);

    SAFE_RELEASE(*alias_cast<D3DShaderResourceView**>(&m_pDeviceShaderResource));

    ptrdiff_t nSize = (ptrdiff_t)m_nActualSize;
    ptrdiff_t nPersSize = (ptrdiff_t)m_nPersistentSize;

    s_pPoolMgr->ReleaseItem(m_pFileTexMips->m_pPoolItem);

    m_pFileTexMips->m_pPoolItem = NULL;
    m_nActualSize = 0;
    m_nPersistentSize = 0;
    m_pDevTexture = NULL;

    SetMinLoadedMip(MAX_MIP_LEVELS);
    m_nMinMipVidActive = MAX_MIP_LEVELS;

    CryInterlockedAddSize(&s_nStatsStreamPoolBoundMem, -nSize);
    CryInterlockedAddSize(&s_nStatsStreamPoolBoundPersMem, -nPersSize);
}

void CTexture::StreamAssignPoolItem(STexPoolItem* pItem, int nMinMip)
{
    FUNCTION_PROFILER_RENDERER;

    assert (!pItem->IsFree());
    STexPool* pItemOwner = pItem->m_pOwner;

    if (m_pFileTexMips->m_pPoolItem == pItem)
    {
        assert(m_nActualSize == m_pFileTexMips->m_pPoolItem->m_pOwner->m_Size);
        assert(m_pFileTexMips->m_pPoolItem->m_pTex == this);
        assert(m_pDevTexture == m_pFileTexMips->m_pPoolItem->m_pDevTexture);
        return;
    }

    if (m_pFileTexMips->m_pPoolItem == NULL)
    {
        if (m_pDevTexture)
        {
            __debugbreak();
        }
    }

    int nPersMip = m_nMips - m_CacheFileHeader.m_nMipsPersistent;
    size_t nPersSize = StreamComputeDevDataSize(nPersMip);

    // Assign a new pool item
    {
        AUTO_LOCK(STexPoolItem::s_sSyncLock);
        StreamRemoveFromPool();

        m_pFileTexMips->m_pPoolItem = pItem;
        m_nActualSize = pItemOwner->m_Size;
        m_nPersistentSize = nPersSize;
        pItem->m_pTex = this;
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURESSTREAMING_CPP_SECTION_6
    #include AZ_RESTRICTED_FILE(D3DTexturesStreaming_cpp)
#endif

    SAFE_RELEASE(m_pDevTexture);
    m_pDevTexture = pItem->m_pDevTexture;

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    ZeroStruct(SRVDesc);
    SRVDesc.Format = DeviceFormatFromTexFormat(GetDstFormat());
    if (m_bIsSRGB)
    {
        SRVDesc.Format = CTexture::ConvertToSRGBFmt(SRVDesc.Format);
    }

    int nDevMip = m_nMips - pItemOwner->m_nMips;

    // Recreate shader resource view
    if (m_eTT == eTT_2D)
    {
        SRVDesc.Texture2D.MipLevels = -1;
        SRVDesc.Texture2D.MostDetailedMip = 0;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    }
    else if (m_eTT == eTT_2DArray)
    {
        SRVDesc.Texture2DArray.MipLevels = -1;
        SRVDesc.Texture2DArray.MostDetailedMip = 0;
        SRVDesc.Texture2DArray.FirstArraySlice = 0;
        SRVDesc.Texture2DArray.ArraySize = m_nArraySize;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    }
    else
    {
        SRVDesc.TextureCube.MipLevels = -1;
        SRVDesc.TextureCube.MostDetailedMip = 0;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    }

    if (m_pDeviceShaderResource)
    {
        D3DShaderResourceView* pSRV = m_pDeviceShaderResource;
        pSRV->Release();
        m_pDeviceShaderResource = NULL;
    }

    D3DShaderResourceView* pNewResourceView = NULL;
    HRESULT hr = gcpRendD3D->GetDevice().CreateShaderResourceView(pItem->m_pDevTexture->GetBaseTexture(), &SRVDesc, &pNewResourceView);
    assert(hr == S_OK);

    SetShaderResourceView(pNewResourceView, false);
    SetMinLoadedMip(m_nMips - pItemOwner->m_nMips);
    StreamActivateLod(nMinMip);

    CryInterlockedAddSize(&s_nStatsStreamPoolBoundMem, (ptrdiff_t)pItem->m_nDeviceTexSize);
    CryInterlockedAddSize(&s_nStatsStreamPoolBoundPersMem, (ptrdiff_t)nPersSize);
}

STexPool* CTexture::StreamGetPool(int nStartMip, int nMips)
{
    const Vec2i vMipAlign = CTexture::GetBlockDim(m_eTFDst);
    int uSize = Align(max(1, m_nWidth >> nStartMip), vMipAlign.x);
    int vSize = Align(max(1, m_nHeight >> nStartMip), vMipAlign.y);

    return s_pPoolMgr->GetPool(uSize, vSize, nMips, m_nArraySize, m_eTFDst, m_bIsSRGB, m_eTT);
}

STexPoolItem* CTexture::StreamGetPoolItem(int nStartMip, int nMips, bool bShouldBeCreated, bool bCreateFromMipData, bool bCanCreate, bool bForStreamOut)
{
    FUNCTION_PROFILER_RENDERER;

    if (!m_pFileTexMips)
    {
        return NULL;
    }

    assert (nStartMip < m_nMips);
    assert(!IsStreaming());

    SCOPED_RENDERER_ALLOCATION_NAME_HINT(GetSourceName());

    const Vec2i vMipAlign = CTexture::GetBlockDim(m_eTFDst);
    int uSize = Align(max(1, m_nWidth >> nStartMip), vMipAlign.x);
    int vSize = Align(max(1, m_nHeight >> nStartMip), vMipAlign.y);
    int nArraySize = m_nArraySize;
    ETEX_Type texType = m_eTT;

    if (m_pFileTexMips->m_pPoolItem && m_pFileTexMips->m_pPoolItem->m_pOwner)
    {
        STexPoolItem* pPoolItem = m_pFileTexMips->m_pPoolItem;
        if (pPoolItem->m_pOwner->m_nMips == nMips &&
            pPoolItem->m_pOwner->m_Width == uSize &&
            pPoolItem->m_pOwner->m_Height == vSize &&
            pPoolItem->m_pOwner->m_nArraySize == nArraySize)
        {
            return NULL;
        }
    }

    STextureInfo ti;
    std::vector<STextureInfoData> srti;
    STextureInfo* pTI = NULL;

    if (bCreateFromMipData)
    {
        uint32 nSlices = StreamGetNumSlices();

        ti.m_nMSAAQuality = 0;
        ti.m_nMSAASamples = 1;
        srti.resize(nSlices * nMips);
        ti.m_pData = &srti[0];

        for (int nSide = 0; nSide < nSlices; ++nSide)
        {
            for (int nMip = nStartMip, nEndMip = nStartMip + nMips; nMip != nEndMip; ++nMip)
            {
                int nSRIdx = nSide * nMips + (nMip - nStartMip);
                STexMipHeader& mmh = m_pFileTexMips->m_pMipHeader[nMip];
                SMipData& md = mmh.m_Mips[nSide];

                if (md.m_bNative)
                {
                    ti.m_pData[nSRIdx].pSysMem = md.DataArray;
                    ti.m_pData[nSRIdx].SysMemPitch = 0;
                    ti.m_pData[nSRIdx].SysMemSlicePitch = 0;
                    ti.m_pData[nSRIdx].SysMemTileMode = m_eSrcTileMode;
                }
                else
                {
                    int nMipW = Align(max(1, m_nWidth >> nMip), vMipAlign.x);
                    int nPitch = 0;
                    const int BlockDim = vMipAlign.x;
                    if (BlockDim > 1)
                    {
                        int blockSize = BytesPerBlock(m_eTFDst);
                        nPitch = (nMipW + BlockDim - 1) / BlockDim * blockSize;
                    }
                    else
                    {
                        nPitch = TextureDataSize(nMipW, 1, 1, 1, 1, m_eTFSrc);
                    }

                    ti.m_pData[nSRIdx].pSysMem = md.DataArray;
                    ti.m_pData[nSRIdx].SysMemPitch = nPitch;
                    ti.m_pData[nSRIdx].SysMemSlicePitch = 0;
                    ti.m_pData[nSRIdx].SysMemTileMode = eTM_None;
                }
            }
        }

        pTI = &ti;
    }

    bool bGPIMustWaitForIdle = true;

    // For end of C3, preserve existing (idle wait) console behaviour
    bGPIMustWaitForIdle = !bForStreamOut;

    STexPoolItem* pItem = s_pPoolMgr->GetPoolItem(uSize, vSize, nMips, nArraySize, GetDstFormat(), m_bIsSRGB, texType, bShouldBeCreated, m_SrcName.c_str(), pTI, bCanCreate, bGPIMustWaitForIdle);
    if (pItem)
    {
        return pItem;
    }

    s_pTextureStreamer->FlagOutOfMemory();
    return NULL;
}

void CTexture::StreamCopyMipsTexToTex(STexPoolItem* pSrcItem, int nMipSrc, STexPoolItem* pDestItem, int nMipDest, int nNumMips)
{
    D3D_CHK_RENDTH;

    uint32 nSides = pDestItem->m_pOwner->GetNumSlices();

    for (uint32 iSide = 0; iSide < nSides; ++iSide)
    {
        CopySliceChain(pDestItem->m_pDevTexture, pDestItem->m_pOwner->m_nMips, iSide, nMipDest, pSrcItem->m_pDevTexture, iSide, nMipSrc, pSrcItem->m_pOwner->m_nMips, nNumMips);
    }
}

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DTEXTURESSTREAMING_CPP_SECTION_7
    #include AZ_RESTRICTED_FILE(D3DTexturesStreaming_cpp)
#endif

// Debug routines /////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _RELEASE
struct StreamDebugSortFunc
{
    bool operator()(const CTexture* p1, const CTexture* p2) const
    {
        if (!p1)
        {
            return true;
        }
        if (!p2)
        {
            return false;
        }
        if (p1->GetActualSize() != p2->GetActualSize())
        {
            return p1->GetActualSize() < p2->GetActualSize();
        }
        return p1 < p2;
    }
};

struct StreamDebugSortWantedFunc
{
    bool operator()(const CTexture* p1, const CTexture* p2) const
    {
        int nP1ReqMip = p1->GetRequiredMipNonVirtual();
        int nP1Size = p1->StreamComputeDevDataSize(nP1ReqMip);

        int nP2ReqMip = p2->GetRequiredMipNonVirtual();
        int nP2Size = p2->StreamComputeDevDataSize(nP2ReqMip);

        if (nP1Size != nP2Size)
        {
            return nP1Size < nP2Size;
        }

        return p1 < p2;
    }
};

void CTexture::OutputDebugInfo()
{
    CD3D9Renderer* r = gcpRendD3D;

    int nX = 40;
    int nY = 30;

    if (CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
    {
        Vec3i vCamPos = iSystem->GetViewCamera().GetPosition();
        CryLogAlways("===================== Dumping textures streaming debug info for camera position (%d, %d, %d) =====================", vCamPos.x, vCamPos.y, vCamPos.z);
    }

    char szText[512];
    sprintf_s(szText, "Size(MB) | WantedSize | MipFactor | HighPriority | #Mips(Desired/Current/Actual) | RoundID Normal/Fast | RecentlyUsed | Name");
    r->WriteXY(nX, nY, 1.f, 1.f, 1, 1, 0, 1, szText);
    if (CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
    {
        CryLogAlways(szText);
    }

    std::vector<CTexture*> texSorted;
    s_pTextureStreamer->StatsFetchTextures(texSorted);

    const char* const sTexFilter = CRenderer::CV_r_TexturesStreamingDebugfilter->GetString();
    const bool bNameFilter = sTexFilter != 0 && strlen(sTexFilter) > 1;

    if (!texSorted.empty())
    {
        switch (CRenderer::CV_r_TexturesStreamingDebug)
        {
        case 4:
            std::stable_sort(&texSorted[0], &texSorted[0] + texSorted.size(), StreamDebugSortFunc());
            break;

        case 5:
            std::reverse(texSorted.begin(), texSorted.end());
            break;

        case 6:
            std::stable_sort(&texSorted[0], &texSorted[0] + texSorted.size(), StreamDebugSortWantedFunc());
            break;
        }
    }

    SThreadInfo& ti = gRenDev->m_RP.m_TI[gRenDev->m_pRT->GetThreadList()];

    for (int i = (int)texSorted.size() - 1, nTexNum = 0; i >= 0; --i)
    {
        CTexture* tp = texSorted[i];
        if (tp == NULL)
        {
            continue;
        }
        // name filter
        if (bNameFilter && !strstr(tp->m_SrcName.c_str(), sTexFilter))
        {
            continue;
        }
        if (tp->m_nActualSize / 1024 < CRenderer::CV_r_TexturesStreamingDebugMinSize)
        {
            continue;
        }

        ColorF color = (tp->m_nActualSize / 1024 >= CRenderer::CV_r_TexturesStreamingDebugMinSize * 2) ? Col_Red : Col_Green;

        // compute final mip factor
        bool bHighPriority = false;
        float fFinalMipFactor = pow(99.99f, 2.f);
        for (int z = 0; z < MAX_PREDICTION_ZONES; z++)
        {
            if (tp->m_pFileTexMips && (int)tp->m_streamRounds[z].nRoundUpdateId > gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_arrZonesRoundId[z] - 2)
            {
                fFinalMipFactor = min(fFinalMipFactor, tp->m_pFileTexMips->m_arrSPInfo[z].fLastMinMipFactor);
                bHighPriority |= tp->m_streamRounds[z].bLastHighPriority;
            }
        }

        // how many times used in area around
        assert(tp->m_pFileTexMips);

        int nMipIdSigned = tp->StreamCalculateMipsSigned(fFinalMipFactor);

        if (nMipIdSigned > CRenderer::CV_r_TexturesStreamingDebugMinMip)
        {
            continue;
        }

        int nPersMip = tp->m_nMips - tp->m_CacheFileHeader.m_nMipsPersistent;
        int nMipReq = min(tp->GetRequiredMipNonVirtual(), nPersMip);
        const int nWantedSize = tp->StreamComputeDevDataSize(nMipReq);

        assert(tp->m_pFileTexMips);
        PREFAST_ASSUME(tp->m_pFileTexMips);
        sprintf_s(szText, "%.2f | %.2f |%6.2f | %1d | %2d/%d/%d | %i/%i | %i | %s",
            (float)tp->m_nActualSize / (1024 * 1024.0f),
            (float)nWantedSize / (1024 * 1024.0f),
            sqrtf(fFinalMipFactor),
            (int)bHighPriority,
            tp->GetNumMipsNonVirtual() - nMipIdSigned, tp->GetNumMipsNonVirtual() - tp->m_nMinMipVidUploaded, tp->GetNumMipsNonVirtual(),
            tp->m_streamRounds[0].nRoundUpdateId, tp->m_streamRounds[MAX_PREDICTION_ZONES - 1].nRoundUpdateId,
            tp->m_nAccessFrameID >= (int)ti.m_nFrameUpdateID - 8,
            tp->m_SrcName.c_str());

        r->WriteXY(nX, nY + (nTexNum + 1) * 10, 1.f, 1.f, color.r, color.g, color.b, 1, szText);
        if (CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
        {
            CryLogAlways(szText);
        }

        ++nTexNum;
        if (nTexNum > 50 && !CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
        {
            break;
        }
    }

    if (CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
    {
        CryLogAlways("==============================================================================================================");
    }

    if (ICVar* pCVar = gEnv->pConsole->GetCVar("r_TexturesStreamingDebugDumpIntoLog"))
    {
        pCVar->Set(0);
    }
}
#endif
