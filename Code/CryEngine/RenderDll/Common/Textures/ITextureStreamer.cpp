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
#include "ITextureStreamer.h"
#include "Texture.h"

ITextureStreamer::ITextureStreamer()
{
    m_pendingRelinks.reserve(4096);
    m_pendingUnlinks.reserve(4096);
    m_textures.reserve(4096);
}

void ITextureStreamer::BeginUpdateSchedule()
{
    SyncTextureList();

    if (CTexture::s_bStatsComputeStreamPoolWanted || CTexture::s_pStatsTexWantedLists)
    {
        CTexture::s_nStatsStreamPoolWanted = StatsComputeRequiredMipMemUsage();
    }

#if !defined(_RELEASE) && !defined(NULL_RENDERER)
    if (CRenderer::CV_r_TexturesStreamingDebug >= 3)
    {
        CTexture::OutputDebugInfo();
    }
#endif
}

void ITextureStreamer::ApplySchedule(EApplyScheduleFlags asf)
{
    AZ_TRACE_METHOD();
    if (asf & eASF_InOut)
    {
        CTexture::StreamState_Update();
    }

    if (asf & eASF_Prep)
    {
        CTexture::StreamState_UpdatePrep();
    }
}

void ITextureStreamer::Relink(CTexture* pTexture)
{
    m_pendingRelinks.push_back(pTexture);
    pTexture->m_bInDistanceSortedList = true;
}

void ITextureStreamer::Unlink(CTexture* pTexture)
{
    using std::swap;

    TStreamerTextureVec::iterator it = std::find(m_pendingRelinks.begin(), m_pendingRelinks.end(), pTexture);
    if (it == m_pendingRelinks.end())
    {
        m_pendingUnlinks.push_back(pTexture);
    }
    else
    {
        swap(*it, m_pendingRelinks.back());
        m_pendingRelinks.pop_back();
    }

    pTexture->m_bInDistanceSortedList = false;
}

int ITextureStreamer::GetMinStreamableMip() const
{
    return CTexture::s_bStreamingFromHDD ? 0 : CRenderer::CV_r_TexturesStreamingMipClampDVD;
}

int ITextureStreamer::GetMinStreamableMipWithSkip() const
{
    return (CTexture::s_bStreamingFromHDD ? 0 : CRenderer::CV_r_TexturesStreamingMipClampDVD);
}

size_t ITextureStreamer::StatsComputeRequiredMipMemUsage()
{
#ifdef STRIP_RENDER_THREAD
    int nThreadList = m_nCurThreadProcess;
#else
    int nThreadList = gRenDev->m_pRT->m_nCurThreadProcess;
#endif
    std::vector<CTexture::WantedStat>* pLists = CTexture::s_pStatsTexWantedLists;
    std::vector<CTexture::WantedStat>* pList = pLists ? &pLists[nThreadList] : NULL;

    if (pList)
    {
        pList->clear();
    }

    SyncTextureList();

    TStreamerTextureVec& textures = GetTextures();

    size_t nSizeToLoad = 0;

    TStreamerTextureVec::iterator item = textures.begin(), end = textures.end();
    for (; item != end; ++item)
    {
        CTexture* tp = *item;

        bool bStale = StatsWouldUnload(tp);
        {
            int nPersMip = tp->m_nMips - tp->m_CacheFileHeader.m_nMipsPersistent;
            int nReqMip = tp->m_bForceStreamHighRes ? 0 : (bStale ? nPersMip : tp->GetRequiredMipNonVirtual());
            nReqMip = min(nReqMip, nPersMip);

            int nWantedSize = tp->StreamComputeDevDataSize(nReqMip);

            nSizeToLoad += nWantedSize;
            if (nWantedSize && pList)
            {
                if (tp->TryAddRef())
                {
                    CTexture::WantedStat ws;
                    ws.pTex = tp;
                    tp->Release();
                    ws.nWanted = nWantedSize;
                    pList->push_back(ws);
                }
            }
        }
    }

    return nSizeToLoad;
}

void ITextureStreamer::StatsFetchTextures(std::vector<CTexture*>& out)
{
    SyncTextureList();

    out.reserve(out.size() + m_textures.size());
    std::copy(m_textures.begin(), m_textures.end(), std::back_inserter(out));
}

bool ITextureStreamer::StatsWouldUnload(const CTexture* pTexture)
{
    SThreadInfo& ti = gRenDev->m_RP.m_TI[gRenDev->m_pRT->GetThreadList()];
    const int nCurrentFarZoneRoundId = ti.m_arrZonesRoundId[MAX_PREDICTION_ZONES - 1];
    const int nCurrentNearZoneRoundId = ti.m_arrZonesRoundId[0];

    return
        (nCurrentFarZoneRoundId - pTexture->GetStreamRoundInfo(MAX_PREDICTION_ZONES - 1).nRoundUpdateId > 3) &&
        (nCurrentNearZoneRoundId - pTexture->GetStreamRoundInfo(0).nRoundUpdateId > 3);
}

void ITextureStreamer::SyncTextureList()
{
    if (!m_pendingUnlinks.empty())
    {
        std::sort(m_pendingUnlinks.begin(), m_pendingUnlinks.end());
        m_pendingUnlinks.resize((int)(std::unique(m_pendingUnlinks.begin(), m_pendingUnlinks.end()) - m_pendingUnlinks.begin()));

        TStreamerTextureVec::iterator it = m_textures.begin(), wrIt = it, itEnd = m_textures.end();
        for (; it != itEnd; ++it)
        {
            if (!std::binary_search(m_pendingUnlinks.begin(), m_pendingUnlinks.end(), *it))
            {
                *wrIt++ = *it;
            }
        }
        m_textures.erase(wrIt, itEnd);

        m_pendingUnlinks.resize(0);
    }

    if (!m_pendingRelinks.empty())
    {
        std::sort(m_pendingRelinks.begin(), m_pendingRelinks.end());
        m_pendingRelinks.resize((int)(std::unique(m_pendingRelinks.begin(), m_pendingRelinks.end()) - m_pendingRelinks.begin()));
        memcpy(m_textures.grow_raw(m_pendingRelinks.size()), &m_pendingRelinks[0], sizeof(m_pendingRelinks[0]) * m_pendingRelinks.size());

        m_pendingRelinks.resize(0);
    }
}
