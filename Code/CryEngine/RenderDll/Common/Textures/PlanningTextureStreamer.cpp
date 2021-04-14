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
#include "PlanningTextureStreamer.h"

#include "TextureStreamPool.h"

#ifndef CHK_RENDTH
#define CHK_RENDTH assert(gRenDev->m_pRT->IsRenderThread())
#endif

CPlanningTextureStreamer::CPlanningTextureStreamer()
    : m_state(S_Idle)
    , m_nRTList(0)
    , m_nJobList(1)
    , m_nBias(0)
    , m_nStreamAllocFails(0)
    , m_bOverBudget(false)
    , m_nPrevListSize(0)
{
    m_schedule.requestList.reserve(1024);
    m_schedule.trimmableList.reserve(4096);
    m_schedule.unlinkList.reserve(4096);
    m_schedule.actionList.reserve(4096);

#if defined(TEXSTRM_DEFER_UMR)
    m_updateMipRequests[0].reserve(8192);
    m_updateMipRequests[1].reserve(8192);
#endif

    memset(m_umrState.arrRoundIds, 0, sizeof(m_umrState.arrRoundIds));
    memset(&m_sortState, 0, sizeof(m_sortState));
}

void CPlanningTextureStreamer::BeginUpdateSchedule()
{
    CryAutoLock<CryCriticalSection> lock(m_lock);

    if (m_state != S_Idle)
    {
        return;
    }

    ITextureStreamer::BeginUpdateSchedule();

    TStreamerTextureVec& textures = GetTextures();

    IF_UNLIKELY (textures.empty())
    {
        return;
    }

    SPlanningSortState& sortInput = m_sortState;

    sortInput.pTextures = &textures;
    sortInput.nTextures = textures.size();

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_texturesstreamingSuppress)
    {
        m_state = S_QueuedForSync;
        return;
    }

    std::swap(m_nJobList, m_nRTList);

    SPlanningMemoryState ms = GetMemoryState();

    // set up the limits
    const SThreadInfo& ti = gRenDev->m_RP.m_TI[gRenDev->m_pRT->GetThreadList()];

    SPlanningScheduleState& schedule = m_schedule;

    schedule.requestList.resize(0);
    schedule.trimmableList.resize(0);
    schedule.unlinkList.resize(0);
    schedule.actionList.resize(0);

    sortInput.nStreamLimit = ms.nStreamLimit;
    for (int z = 0; z < sizeof(sortInput.arrRoundIds) / sizeof(sortInput.arrRoundIds[0]); ++z)
    {
        sortInput.arrRoundIds[z] = ti.m_arrZonesRoundId[z] - CRenderer::CV_r_texturesstreamingPrecacheRounds;
    }
    sortInput.nFrameId = ti.m_nFrameUpdateID;
    sortInput.nBias = m_nBias;

#if defined(TEXSTRM_BYTECENTRIC_MEMORY)
    if (CTexture::s_bPrestreamPhase)
    {
        sortInput.fpMinBias = 1 << 8;
        sortInput.fpMaxBias = 1 << 8;
    }
    else
#endif
    {
        sortInput.fpMinBias = -(8 << 8);
        sortInput.fpMaxBias = 1 << 8;
    }

    sortInput.fpMinMip = GetMinStreamableMip() << 8;
    sortInput.memState = ms;

    sortInput.nBalancePoint = 0;
    sortInput.nOnScreenPoint = 0;
    sortInput.nPrecachedTexs = 0;
    sortInput.nListSize = 0;

    sortInput.pRequestList = &schedule.requestList;
    sortInput.pTrimmableList = &schedule.trimmableList;
    sortInput.pUnlinkList = &schedule.unlinkList;
    sortInput.pActionList = &schedule.actionList;

    SPlanningUMRState& umrState = m_umrState;
    for (int i = 0; i < MAX_PREDICTION_ZONES; ++i)
    {
        umrState.arrRoundIds[i] = ti.m_arrZonesRoundId[i];
    }

    m_state = S_QueuedForUpdate;

    if (CRenderer::CV_r_texturesstreamingJobUpdate)
    {
        StartUpdateJob();
    }
    else
    {
        Job_UpdateEntry();
    }

#if defined(TEXSTRM_DEFER_UMR)
    m_updateMipRequests[m_nRTList].resize(0);
#endif
}

void CPlanningTextureStreamer::ApplySchedule(EApplyScheduleFlags asf)
{
    CHK_RENDTH;

    FUNCTION_PROFILER_RENDERER;

    CryAutoLock<CryCriticalSection> lock(m_lock);

    SyncWithJob_Locked();

    SPlanningScheduleState& schedule = m_schedule;

    switch (m_state)
    {
    case S_QueuedForSchedule:
        break;

    case S_QueuedForScheduleDiscard:
        schedule.trimmableList.resize(0);
        schedule.unlinkList.resize(0);
        schedule.requestList.resize(0);
        schedule.actionList.resize(0);
        m_state = S_Idle;
        break;

    default:
        ITextureStreamer::ApplySchedule(asf);
        return;
    }

    TStreamerTextureVec& textures = GetTextures();
    TStreamerTextureVec& trimmable = schedule.trimmableList;
    TStreamerTextureVec& unlinkList = schedule.unlinkList;
    TPlanningActionVec& actions = schedule.actionList;
    TPlanningTextureReqVec& requested = schedule.requestList;

    for (TPlanningActionVec::iterator it = actions.begin(), itEnd = actions.end(); it != itEnd; ++it)
    {
        CTexture* pTex = textures[it->nTexture];

        switch (it->eAction)
        {
        case SPlanningAction::Abort:
        {
            if (pTex->IsStreaming())
            {
                STexStreamInState* pSIS = CTexture::s_StreamInTasks.GetPtrFromIdx(pTex->m_nStreamSlot & CTexture::StreamIdxMask);
                pSIS->m_bAborted = true;
            }
        }
        break;
        }
    }

    ITextureStreamer::ApplySchedule(asf);

    {
        bool bOverflow = m_nStreamAllocFails > 0;
        m_nStreamAllocFails = 0;

        int nMaxItemsToFree = bOverflow ? 1000 : 2;
        size_t nGCLimit = schedule.memState.nMemLimit;
#if !defined(CONSOLE)
        nGCLimit = static_cast<size_t>(static_cast<int64>(nGCLimit) * 120 / 100);
#endif
        size_t nPoolSize = CTexture::s_pPoolMgr->GetReservedSize();
        CTexture::s_pPoolMgr->GarbageCollect(&nPoolSize, nGCLimit, nMaxItemsToFree);
    }

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_texturesstreamingSuppress)
    {
#ifndef NULL_RENDERER
        ptrdiff_t nMemFreeUpper = schedule.memState.nMemFreeUpper;
        ptrdiff_t nMemFreeLower = schedule.memState.nMemFreeLower;
        int nBalancePoint = schedule.nBalancePoint;

        // Everything < nBalancePoint can only be trimmed (trimmable list), everything >= nBalancePoint can be kicked
        // We should be able to load everything in the requested list

        int nKickIdx = (int)textures.size() - 1;
        int nNumSubmittedLoad = CTexture::s_nMipsSubmittedToStreaming;
        size_t nAmtSubmittedLoad = CTexture::s_nBytesSubmittedToStreaming;

        if (!requested.empty())
        {
            size_t nMaxRequestedBytes = CTexture::s_bPrestreamPhase
                ? 1024 * 1024 * 1024
                : (size_t)(CRenderer::CV_r_TexturesStreamingMaxRequestedMB * 1024.f * 1024.f);
            int nMaxRequestedJobs = CTexture::s_bPrestreamPhase
                ? CTexture::MaxStreamTasks
                : CRenderer::CV_r_TexturesStreamingMaxRequestedJobs;

            const int posponeThresholdKB = (CRenderer::CV_r_texturesstreamingPostponeMips && !CTexture::s_bStreamingFromHDD) ? (CRenderer::CV_r_texturesstreamingPostponeThresholdKB * 1024) : INT_MAX;
            const int posponeThresholdMip = (CRenderer::CV_r_texturesstreamingPostponeMips) ? CRenderer::CV_r_texturesstreamingPostponeThresholdMip : 0;
            const int nMinimumMip = max(posponeThresholdMip, (int)(CRenderer::CV_r_TexturesStreamingMipBias + gRenDev->m_fTexturesStreamingGlobalMipFactor));

            if (gRenDev->m_nFlushAllPendingTextureStreamingJobs && nMaxRequestedBytes && nMaxRequestedJobs)
            {
                nMaxRequestedBytes = 1024 * 1024 * 1024;
                nMaxRequestedJobs = 1024 * 1024;
                --gRenDev->m_nFlushAllPendingTextureStreamingJobs;
            }

            bool bPreStreamPhase = CTexture::s_bPrestreamPhase;

            IStreamEngine* pStreamEngine = gEnv->pSystem->GetStreamEngine();

            pStreamEngine->BeginReadGroup();

            for (
                int nReqIdx = 0, nReqCount = (int)requested.size();
                nReqIdx < nReqCount &&
                CTexture::s_StreamInTasks.GetNumFree() > 0 &&
                nNumSubmittedLoad < nMaxRequestedJobs &&
                nAmtSubmittedLoad < nMaxRequestedBytes
                ;
                ++nReqIdx)
            {
                CTexture* pTex = requested[nReqIdx].first;
                IF_UNLIKELY (!pTex->m_bStreamed)
                {
                    continue;
                }

                int nTexRequestedMip = requested[nReqIdx].second;

                int nTexPersMip = pTex->m_nMips - pTex->m_CacheFileHeader.m_nMipsPersistent;
                int nTexWantedMip = min(nTexRequestedMip, nTexPersMip);
                int nTexAvailMip = pTex->m_nMinMipVidUploaded;

                STexMipHeader* pMH = pTex->m_pFileTexMips->m_pMipHeader;
                int nSides = (pTex->m_nFlags & FT_REPLICATE_TO_ALL_SIDES) ? 1 : pTex->m_CacheFileHeader.m_nSides;

                if (!bPreStreamPhase)
                {
                    // Don't load top mips unless the top mip is the only mip we want
                    const int nMipSizeLargest = pMH[nTexWantedMip].m_SideSize * nSides;
                    if ((nMipSizeLargest >= posponeThresholdKB || posponeThresholdMip > nTexWantedMip) && nTexWantedMip < min(nTexPersMip, nTexAvailMip - 1))
                    {
                        ++nTexWantedMip;
                    }
                }
                else
                {
                    if (nTexWantedMip == 0)
                    {
                        ++nTexWantedMip;
                    }
                }

                if (nTexWantedMip < nTexAvailMip)
                {
                    if (!(pTex->CTexture::GetFlags() & FT_COMPOSITE))
                    {
                        if (!TryBegin_FromDisk(
                                pTex, nTexPersMip, nTexWantedMip, nTexAvailMip, schedule.nBias, nBalancePoint,
                                textures, trimmable, nMemFreeLower, nMemFreeUpper, nKickIdx,
                                nNumSubmittedLoad, nAmtSubmittedLoad))
                        {
                            break;
                        }
                    }
                    else
                    {
                        if (!TryBegin_Composite(
                                pTex, nTexPersMip, nTexWantedMip, nTexAvailMip, schedule.nBias, nBalancePoint,
                                textures, trimmable, nMemFreeLower, nMemFreeUpper, nKickIdx,
                                nNumSubmittedLoad, nAmtSubmittedLoad))
                        {
                            break;
                        }
                    }
                }
            }

            pStreamEngine->EndReadGroup();
        }

        CTexture::s_nStatsAllocFails = m_nStreamAllocFails;
#endif
    }
    else
    {
        for (TStreamerTextureVec::iterator it = textures.begin(), itEnd = textures.end(); it != itEnd; ++it)
        {
            CTexture* pTex = *it;
            if (!pTex->IsStreamingInProgress())
            {
                int nPersMip = pTex->GetNumMipsNonVirtual() - pTex->GetNumPersistentMips();
                if (pTex->StreamGetLoadedMip() < nPersMip)
                {
                    pTex->StreamTrim(nPersMip);
                }
            }
        }
    }

    for (TStreamerTextureVec::const_iterator it = unlinkList.begin(), itEnd = unlinkList.end(); it != itEnd; ++it)
    {
        CTexture* pTexture = *it;
        Unlink(pTexture);
    }

    trimmable.resize(0);
    unlinkList.resize(0);
    requested.resize(0);
    actions.resize(0);

    m_state = S_Idle;
}

#ifndef NULL_RENDERER
bool CPlanningTextureStreamer::TryBegin_FromDisk(CTexture* pTex, uint32 nTexPersMip, uint32 nTexWantedMip, uint32 nTexAvailMip, int nBias, int nBalancePoint,
    TStreamerTextureVec& textures, TStreamerTextureVec& trimmable,
    ptrdiff_t& nMemFreeLower, ptrdiff_t& nMemFreeUpper, int& nKickIdx,
    int& nNumSubmittedLoad, size_t& nAmtSubmittedLoad)
{
    uint32 nTexActivateMip = clamp_tpl((uint32)pTex->GetRequiredMipNonVirtual(), nTexWantedMip, nTexPersMip);
    int estp = CTexture::s_bStreamingFromHDD ? estpNormal : estpBelowNormal;

    if (pTex->IsStreamHighPriority())
    {
        --estp;
    }

    if (nTexActivateMip < nTexAvailMip)
    {
        // Split stream tasks so that mips needed for the working set are loaded first, then additional
        // mips for caching can be loaded next time around.
        nTexWantedMip = max(nTexWantedMip, nTexActivateMip);
    }

    if (nTexWantedMip < nTexActivateMip)
    {
        // Caching additional mips - no need to request urgently.
        ++estp;
    }


    ptrdiff_t nRequired = pTex->StreamComputeDevDataSize(nTexWantedMip) - pTex->StreamComputeDevDataSize(nTexAvailMip);

    STexPoolItem* pNewPoolItem = NULL;

    int nTexWantedMips = pTex->m_nMips - nTexWantedMip;

#if defined(TEXSTRM_TEXTURECENTRIC_MEMORY)
    // First, try and allocate an existing texture that we own - don't allow D3D textures to be made yet
    pNewPoolItem = pTex->StreamGetPoolItem(nTexWantedMip, nTexWantedMips, false, false, false);

    if (!pNewPoolItem)
    {
        STexPool* pPrioritisePool = pTex->StreamGetPool(nTexWantedMip, pTex->m_nMips - nTexWantedMip);

        // That failed, so try and find a trimmable texture with the dimensions we want
        if (TrimTexture(nBias, trimmable, pPrioritisePool))
        {
            // Found a trimmable texture that matched - now wait for the next update, when it should be done
            return true;
        }
    }
#endif

    bool bShouldStopRequesting = false;

    if (!pNewPoolItem && nRequired > nMemFreeUpper)
    {
        // Not enough room in the pool. Can we trim some existing textures?
        ptrdiff_t nFreed = TrimTextures(nRequired - nMemFreeLower, nBias, trimmable);
        nMemFreeLower += nFreed;
        nMemFreeUpper += nFreed;

        if (nRequired > nMemFreeUpper)
        {
            ptrdiff_t nKicked = KickTextures(&textures[0], nRequired - nMemFreeLower, nBalancePoint, nKickIdx);

            nMemFreeLower += nKicked;
            nMemFreeUpper += nKicked;
        }
    }
    else
    {
        // The requested job may be for a force-stream-high-res texture that only has persistent mips.
        // However texture kicking may have already evicted it to make room for another texture, and as such,
        // streaming may now be in progress, even though it wasn't when the request was queued.
        if (!pTex->IsStreaming())
        {
            // There should be room in the pool, so try and start streaming.

            bool bRequestStreaming = true;

            if (!pNewPoolItem)
            {
                pNewPoolItem = pTex->StreamGetPoolItem(nTexWantedMip, nTexWantedMips, false);
            }

            if (!pNewPoolItem)
            {
                bRequestStreaming = false;
            }

            if (bRequestStreaming)
            {
                if (CTexture::StartStreaming(pTex, pNewPoolItem, nTexWantedMip, nTexAvailMip - 1, nTexActivateMip, static_cast<EStreamTaskPriority>(estp)))
                {
                    nMemFreeUpper -= nRequired;
                    nMemFreeLower -= nRequired;

                    ++nNumSubmittedLoad;
                    nAmtSubmittedLoad += nRequired;
                }

                // StartStreaming takes ownership
                pNewPoolItem = NULL;
            }
            else
            {
                bShouldStopRequesting = true;
            }
        }
    }

    if (pNewPoolItem)
    {
        CTexture::s_pPoolMgr->ReleaseItem(pNewPoolItem);
    }

    if (bShouldStopRequesting)
    {
        return false;
    }

    return true;
}

bool CPlanningTextureStreamer::TryBegin_Composite(CTexture* pTex, [[maybe_unused]] uint32 nTexPersMip, uint32 nTexWantedMip, uint32 nTexAvailMip, int nBias, int nBalancePoint,
    TStreamerTextureVec& textures, TStreamerTextureVec& trimmable,
    ptrdiff_t& nMemFreeLower, ptrdiff_t& nMemFreeUpper, int& nKickIdx,
    [[maybe_unused]] int& nNumSubmittedLoad, [[maybe_unused]] size_t& nAmtSubmittedLoad)
{

    ptrdiff_t nRequired = pTex->StreamComputeDevDataSize(nTexWantedMip) - pTex->StreamComputeDevDataSize(nTexAvailMip);

    // Test source textures, to ensure they're all ready.

    DynArray<STexComposition>& composite = pTex->m_composition;

    for (int i = 0, c = composite.size(); i != c; ++i)
    {
        const STexComposition& tc = composite[i];
        CTexture* p = (CTexture*)&*tc.pTexture;
        if (p->StreamGetLoadedMip() > nTexWantedMip)
        {
            // Source isn't ready yet. Try again later.
            return true;
        }
    }

    STexPoolItem* pNewPoolItem = NULL;

    int nTexWantedMips = pTex->m_nMips - nTexWantedMip;

#if defined(TEXSTRM_TEXTURECENTRIC_MEMORY)
    // First, try and allocate an existing texture that we own - don't allow D3D textures to be made yet
    pNewPoolItem = pTex->StreamGetPoolItem(nTexWantedMip, nTexWantedMips, false, false, false);

    if (!pNewPoolItem)
    {
        STexPool* pPrioritisePool = pTex->StreamGetPool(nTexWantedMip, pTex->m_nMips - nTexWantedMip);

        // That failed, so try and find a trimmable texture with the dimensions we want
        if (TrimTexture(nBias, trimmable, pPrioritisePool))
        {
            // Found a trimmable texture that matched - now wait for the next update, when it should be done
            return true;
        }
    }
#endif

    if (!pNewPoolItem && nRequired > nMemFreeUpper)
    {
        // Not enough room in the pool. Can we trim some existing textures?
        ptrdiff_t nFreed = TrimTextures(nRequired - nMemFreeLower, nBias, trimmable);
        nMemFreeLower += nFreed;
        nMemFreeUpper += nFreed;

        if (nRequired > nMemFreeUpper)
        {
            ptrdiff_t nKicked = KickTextures(&textures[0], nRequired - nMemFreeLower, nBalancePoint, nKickIdx);

            nMemFreeLower += nKicked;
            nMemFreeUpper += nKicked;
        }
    }
    else if (!pTex->IsStreaming())
    {
        // Bake!

        if (!pNewPoolItem)
        {
            pNewPoolItem = pTex->StreamGetPoolItem(nTexWantedMip, nTexWantedMips, false);
        }

        if (pNewPoolItem)
        {
            for (int i = 0, c = composite.size(); i != c; ++i)
            {
                const STexComposition& tc = composite[i];
                CTexture* p = (CTexture*)&*tc.pTexture;
                CDeviceTexture* pSrcDevTex = p->m_pDevTexture;
                uint32 nSrcDevMips = p->GetNumMipsNonVirtual() - p->StreamGetLoadedMip();

                CTexture::CopySliceChain(
                    pNewPoolItem->m_pDevTexture, pNewPoolItem->m_pOwner->m_nMips, tc.nDstSlice, 0,
                    pSrcDevTex, tc.nSrcSlice, nTexWantedMip - (pTex->m_nMips - nSrcDevMips), nSrcDevMips,
                    pTex->m_nMips - nTexWantedMip);
            }

            // Commit!

            pTex->StreamAssignPoolItem(pNewPoolItem, nTexWantedMip);
            pNewPoolItem = NULL;
        }
    }

    if (pNewPoolItem)
    {
        CTexture::s_pPoolMgr->ReleaseItem(pNewPoolItem);
    }

    return true;
}
#endif

bool CPlanningTextureStreamer::BeginPrepare(CTexture* pTexture, const char* sFilename, uint32 nFlags)
{
    CryAutoLock<CryCriticalSection> lock(m_lock);

    STexStreamPrepState** pState = CTexture::s_StreamPrepTasks.Allocate();
    if (pState)
    {
        // Initialise prep state privately, in case any concurrent prep updates are running.
        STexStreamPrepState* state = new STexStreamPrepState;
        state->m_pTexture = pTexture;
        state->m_pImage = CImageFile::mfStream_File(sFilename, nFlags, state);
        if (state->m_pImage)
        {
            *const_cast<STexStreamPrepState* volatile*>(pState) = state;
            return true;
        }

        delete state;

        CTexture::s_StreamPrepTasks.Release(pState);
    }

    return false;
}

void CPlanningTextureStreamer::EndPrepare(STexStreamPrepState*& pState)
{
    CryAutoLock<CryCriticalSection> lock(m_lock);

    delete pState;
    pState = NULL;

    CTexture::s_StreamPrepTasks.Release(&pState);
}

void CPlanningTextureStreamer::Precache(CTexture* pTexture)
{
    if (pTexture->IsForceStreamHighRes())
    {
        for (int i = 0; i < MAX_PREDICTION_ZONES; ++i)
        {
            pTexture->m_streamRounds[i].nRoundUpdateId = (1 << 29) - 1;
            pTexture->m_pFileTexMips->m_arrSPInfo[i].fMinMipFactor = 0;
        }
        if (pTexture->IsUnloaded())
        {
            pTexture->StreamLoadFromCache(0);
        }
    }
}

void CPlanningTextureStreamer::UpdateMip(CTexture* pTexture, const float fMipFactor, const int nFlags, const int nUpdateId, [[maybe_unused]] const int nCounter)
{
    CHK_RENDTH;

#if defined(TEXSTRM_DEFER_UMR)

    SPlanningUpdateMipRequest req;
    req.pTexture = pTexture;
    req.fMipFactor = fMipFactor;
    req.nFlags = nFlags;
    req.nUpdateId = nUpdateId;

    m_updateMipRequests[m_nRTList].push_back(req);

#else

    Job_UpdateMip(pTexture, fMipFactor, nFlags, nUpdateId);

#endif
}

void CPlanningTextureStreamer::OnTextureDestroy(CTexture* pTexture)
{
    if (!pTexture->IsStreamed())
    {
        return;
    }

    CryAutoLock<CryCriticalSection> lock(m_lock);

    SyncWithJob_Locked();

    switch (m_state)
    {
    case S_Idle:
    case S_QueuedForScheduleDiscard:
        break;

    case S_QueuedForSchedule:
        m_state = S_QueuedForScheduleDiscard;
        break;

#ifndef _RELEASE
    default:
        __debugbreak();
        break;
#endif
    }

    // Remove the texture from the pending list of mip updates
    using std::swap;

#if defined(TEXSTRM_DEFER_UMR)
    UpdateMipRequestVec& umrv = m_updateMipRequests[m_nRTList];
    for (int i = 0, c = umrv.size(); i != c; )
    {
        SPlanningUpdateMipRequest& umr = umrv[i];
        if (umr.pTexture == pTexture)
        {
            swap(umrv.back(), umr);
            umrv.pop_back();
            --c;
        }
        else
        {
            ++i;
        }
    }
#endif
}

void CPlanningTextureStreamer::FlagOutOfMemory()
{
#if defined(TEXSTRM_BYTECENTRIC_MEMORY)
    ++m_nStreamAllocFails;
#endif
}

void CPlanningTextureStreamer::Flush()
{
}

bool CPlanningTextureStreamer::IsOverflowing() const
{
    return m_bOverBudget;
}

SPlanningMemoryState CPlanningTextureStreamer::GetMemoryState()
{
    SPlanningMemoryState ms = {0};

    ms.nMemStreamed = CTexture::s_nStatsStreamPoolInUseMem;

    ms.nPhysicalLimit = (ptrdiff_t)CRenderer::GetTexturesStreamPoolSize() * 1024 * 1024;

    ms.nMemLimit = (ptrdiff_t)((int64)ms.nPhysicalLimit * 95 / 100);
    ms.nMemFreeSlack = (ptrdiff_t)((int64)ms.nPhysicalLimit * 5 / 100);

    ms.nMemBoundStreamed = CTexture::s_nStatsStreamPoolBoundMem;
    ms.nMemTemp = ms.nMemStreamed - ms.nMemBoundStreamed;
    ms.nMemFreeLower = ms.nMemLimit - ms.nMemStreamed;
    ms.nMemFreeUpper = (ms.nMemLimit + ms.nMemFreeSlack) - ms.nMemStreamed;
    ms.nStreamLimit = ms.nMemLimit - CTexture::s_nStatsStreamPoolBoundPersMem;

    ms.nStreamMid = static_cast<ptrdiff_t>(ms.nStreamLimit + ms.nMemFreeSlack / 2);
    ms.nStreamDelta = static_cast<ptrdiff_t>(m_nPrevListSize) - ms.nStreamMid;

    return ms;
}

void CPlanningTextureStreamer::SyncWithJob_Locked()
{
    FUNCTION_PROFILER_RENDERER;

    m_jobExecutor.WaitForCompletion();

    if (m_state == S_QueuedForSync)
    {
        SPlanningSortState& state = m_sortState;
        TStreamerTextureVec& textures = GetTextures();

        // Commit iteration state

        bool bOverBudget = state.nBalancePoint < state.nPrecachedTexs;
        m_bOverBudget = bOverBudget;

        m_nBias = state.nBias;
        m_nPrevListSize = state.nListSize;

        textures.resize(state.nTextures);

#ifdef TEXSTRM_DEFER_UMR
        SyncTextureList();
#endif

        m_state = S_QueuedForSchedule;
    }
}

#if defined(TEXSTRM_TEXTURECENTRIC_MEMORY)
bool CPlanningTextureStreamer::TrimTexture(int nBias, TStreamerTextureVec& trimmable, STexPool* pPrioritise)
{
    FUNCTION_PROFILER_RENDERER;

    size_t nBestTrimmableIdx = 0;
    int nMostMipsToTrim = 0;
    int nBestTrimTargetMip = 0;

    for (size_t i = 0, c = trimmable.size(); i != c; ++i)
    {
        CTexture* pTrimTex = trimmable[i];


        if (pTrimTex->m_bStreamPrepared)
        {
            STexPool* pTrimItemPool = pTrimTex->GetStreamingInfo()->m_pPoolItem->m_pOwner;

            if (pTrimItemPool == pPrioritise)
            {
                int nPersMip = bsel(pTrimTex->m_bForceStreamHighRes, 0, pTrimTex->m_nMips - pTrimTex->m_CacheFileHeader.m_nMipsPersistent);
                int nTrimMip = pTrimTex->m_nMinMipVidUploaded;
                int nTrimTargetMip = max(0, min((int)(pTrimTex->m_fpMinMipCur + nBias) >> 8, nPersMip));

                int nTrimMips = nTrimTargetMip - nTrimMip;

                if (nTrimMips > nMostMipsToTrim)
                {
                    nBestTrimmableIdx = i;
                    nMostMipsToTrim = nTrimMips;
                    nBestTrimTargetMip = nTrimTargetMip;
                }
            }
        }
    }

    if (nMostMipsToTrim > 0)
    {
        CTexture* pTrimTex = trimmable[nBestTrimmableIdx];

        if (pTrimTex->StreamTrim(nBestTrimTargetMip))
        {
            trimmable[nBestTrimmableIdx] = trimmable.back();
            trimmable.pop_back();

            return true;
        }
    }

    return false;
}
#endif

ptrdiff_t CPlanningTextureStreamer::TrimTextures(ptrdiff_t nRequired, int nBias, TStreamerTextureVec& trimmable)
{
    FUNCTION_PROFILER_RENDERER;

    ptrdiff_t nTrimmed = 0;

    int nTrimIdx = (int)trimmable.size();
    for (; nTrimIdx > 0 && nTrimmed < nRequired; --nTrimIdx)
    {
        CTexture* pTrimTex = trimmable[nTrimIdx - 1];

        if (!pTrimTex->IsUnloaded())
        {
            int nPersMip = bsel(pTrimTex->m_bForceStreamHighRes, 0, pTrimTex->m_nMips - pTrimTex->m_CacheFileHeader.m_nMipsPersistent);
            int nTrimMip = pTrimTex->m_nMinMipVidUploaded;
            int nTrimTargetMip = max(0, min((int)(pTrimTex->m_fpMinMipCur + nBias) >> 8, nPersMip));
            ptrdiff_t nProfit = pTrimTex->StreamComputeDevDataSize(nTrimMip) - pTrimTex->StreamComputeDevDataSize(nTrimTargetMip);

            if (pTrimTex->StreamTrim(nTrimTargetMip))
            {
                nTrimmed += nProfit;
            }
        }
    }

    trimmable.resize(nTrimIdx);

    return nTrimmed;
}

ptrdiff_t CPlanningTextureStreamer::KickTextures(CTexture** pTextures, ptrdiff_t nRequired, int nBalancePoint, int& nKickIdx)
{
    FUNCTION_PROFILER_RENDERER;

    ptrdiff_t nKicked = 0;

    SThreadInfo& ti = gRenDev->m_RP.m_TI[gRenDev->m_pRT->GetThreadList()];
    const int nCurrentFarZoneRoundId = ti.m_arrZonesRoundId[MAX_PREDICTION_ZONES - 1];
    const int nCurrentNearZoneRoundId = ti.m_arrZonesRoundId[0];

    // If we're still lacking space, begin kicking old textures
    for (; nKicked < nRequired && nKickIdx >= nBalancePoint; --nKickIdx)
    {
        CTexture* pKillTex = pTextures[nKickIdx];

        if (!pKillTex->IsUnloaded())
        {
            int nKillMip = pKillTex->m_nMinMipVidUploaded;
            int nKillPersMip = bsel(pKillTex->m_bForceStreamHighRes, 0, pKillTex->m_nMips - pKillTex->m_CacheFileHeader.m_nMipsPersistent);

            // unload textures that are older than 4 update cycles
            if (nKillPersMip > nKillMip)
            {

                // How much is available?
                ptrdiff_t nProfit = pKillTex->StreamComputeDevDataSize(nKillMip) - pKillTex->StreamComputeDevDataSize(nKillPersMip);

                // Begin freeing.
                pKillTex->StreamTrim(nKillPersMip);

                nKicked += nProfit;
            }
        }
    }

    return nKicked;
}
