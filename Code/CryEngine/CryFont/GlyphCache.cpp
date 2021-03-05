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

// Purpose:
//  - Manage and cache glyphs, retrieving them from the renderer as needed

#include "CryFont_precompiled.h"

#if !defined(USE_NULLFONT_ALWAYS)

#include "GlyphCache.h"
#include "FontTexture.h"



//-------------------------------------------------------------------------------------------------
CGlyphCache::CGlyphCache()
    : m_dwUsage(1)
    , m_iGlyphBitmapWidth(0)
    , m_iGlyphBitmapHeight(0)
    , m_pScaleBitmap(0)
{
    m_pCacheTable.clear();
    m_pSlotList.clear();
}

//-------------------------------------------------------------------------------------------------
CGlyphCache::~CGlyphCache()
{
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::Create(int iCacheSize, int iGlyphBitmapWidth, int iGlyphBitmapHeight, int iSmoothMethod, int iSmoothAmount, float sizeRatio)
{
    m_iSmoothMethod = iSmoothMethod;
    m_iSmoothAmount = iSmoothAmount;

    m_iGlyphBitmapWidth = iGlyphBitmapWidth;
    m_iGlyphBitmapHeight = iGlyphBitmapHeight;


    if (!CreateSlotList(iCacheSize))
    {
        ReleaseSlotList();

        return 0;
    }

    int iScaledGlyphWidth = 0;
    int iScaledGlyphHeight = 0;

    switch (m_iSmoothMethod)
    {
    case FONT_SMOOTH_SUPERSAMPLE:
    {
        switch (m_iSmoothAmount)
        {
        case FONT_SMOOTH_AMOUNT_2X:
            iScaledGlyphWidth = m_iGlyphBitmapWidth << 1;
            iScaledGlyphHeight = m_iGlyphBitmapHeight << 1;
            break;
        case FONT_SMOOTH_AMOUNT_4X:
            iScaledGlyphWidth = m_iGlyphBitmapWidth << 2;
            iScaledGlyphHeight = m_iGlyphBitmapHeight << 2;
            break;
        }
    }
    break;
    }

    if (iScaledGlyphWidth)
    {
        m_pScaleBitmap = new CGlyphBitmap;

        if (!m_pScaleBitmap)
        {
            Release();

            return 0;
        }

        if (!m_pScaleBitmap->Create(iScaledGlyphWidth, iScaledGlyphHeight))
        {
            Release();

            return 0;
        }

        m_pFontRenderer.SetGlyphBitmapSize(iScaledGlyphWidth, iScaledGlyphHeight, sizeRatio);
    }
    else
    {
        m_pFontRenderer.SetGlyphBitmapSize(m_iGlyphBitmapWidth, m_iGlyphBitmapHeight, sizeRatio);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::Release()
{
    ReleaseSlotList();

    m_pCacheTable.clear();

    if (m_pScaleBitmap)
    {
        m_pScaleBitmap->Release();

        delete m_pScaleBitmap;

        m_pScaleBitmap = 0;
    }

    m_iGlyphBitmapWidth = 0;
    m_iGlyphBitmapHeight = 0;

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::LoadFontFromFile(const string& szFileName)
{
    return m_pFontRenderer.LoadFromFile(szFileName);
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::LoadFontFromMemory(unsigned char* pFileBuffer, int iDataSize)
{
    return m_pFontRenderer.LoadFromMemory(pFileBuffer, iDataSize);
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::ReleaseFont()
{
    m_pFontRenderer.Release();

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::GetGlyphBitmapSize(int* pWidth, int* pHeight)
{
    if (pWidth)
    {
        *pWidth = m_iGlyphBitmapWidth;
    }

    if (pHeight)
    {
        *pHeight = m_iGlyphBitmapWidth;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
void CGlyphCache::SetGlyphBitmapSize(int width, int height, float sizeRatio)
{
    m_pFontRenderer.SetGlyphBitmapSize(width, height, sizeRatio);
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::PreCacheGlyph(uint32 cChar, const Vec2i& glyphSize, const CFFont::FontHintParams& fontHintParams)
{
    CCacheTable::iterator pItor = m_pCacheTable.find(GetCacheSlotKey(cChar, glyphSize));

    if (pItor != m_pCacheTable.end())
    {
        pItor->second->dwUsage = m_dwUsage;

        return 1;
    }

    CCacheSlot* pSlot = GetLRUSlot();

    if (!pSlot)
    {
        return 0;
    }

    if (pSlot->dwUsage > 0)
    {
        UnCacheGlyph(pSlot->cCurrentChar, pSlot->glyphSize);
    }

    if (m_pScaleBitmap)
    {
        int iOffsetMult = 1;

        switch (m_iSmoothAmount)
        {
        case FONT_SMOOTH_AMOUNT_2X:
            iOffsetMult = 2;
            break;
        case FONT_SMOOTH_AMOUNT_4X:
            iOffsetMult = 4;
            break;
        }

        m_pScaleBitmap->Clear();

        if (!m_pFontRenderer.GetGlyph(m_pScaleBitmap, &pSlot->iHoriAdvance, &pSlot->iCharWidth, &pSlot->iCharHeight, pSlot->iCharOffsetX, pSlot->iCharOffsetY, 0, 0, cChar, fontHintParams))
        {
            return 0;
        }

        pSlot->iCharWidth >>= iOffsetMult >> 1;
        pSlot->iCharHeight >>= iOffsetMult >> 1;

        m_pScaleBitmap->BlitScaledTo8(pSlot->pGlyphBitmap.GetBuffer(), 0, 0, m_pScaleBitmap->GetWidth(), m_pScaleBitmap->GetHeight(), 0, 0, pSlot->pGlyphBitmap.GetWidth(), pSlot->pGlyphBitmap.GetHeight(), pSlot->pGlyphBitmap.GetWidth());
    }
    else
    {
        if (!m_pFontRenderer.GetGlyph(&pSlot->pGlyphBitmap, &pSlot->iHoriAdvance, &pSlot->iCharWidth, &pSlot->iCharHeight, pSlot->iCharOffsetX, pSlot->iCharOffsetY, 0, 0, cChar, fontHintParams))
        {
            return 0;
        }
    }

    if (m_iSmoothMethod == FONT_SMOOTH_BLUR)
    {
        pSlot->pGlyphBitmap.Blur(m_iSmoothAmount);
    }

    pSlot->dwUsage = m_dwUsage;
    pSlot->cCurrentChar = cChar;
    pSlot->glyphSize = glyphSize;

    m_pCacheTable.insert(AZStd::pair<CryFont::GlyphCache::CCacheTableKey, CCacheSlot*>(GetCacheSlotKey(cChar, glyphSize), pSlot));

    return 1;
}

int CGlyphCache::UnCacheGlyph(uint32 cChar, const Vec2i& glyphSize)
{
    CCacheTable::iterator pItor = m_pCacheTable.find(GetCacheSlotKey(cChar, glyphSize));

    if (pItor != m_pCacheTable.end())
    {
        CCacheSlot* pSlot = pItor->second;

        pSlot->Reset();

        m_pCacheTable.erase(pItor);

        return 1;
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::GlyphCached(uint32 cChar, const Vec2i& glyphSize)
{
    return (m_pCacheTable.find(GetCacheSlotKey(cChar, glyphSize)) != m_pCacheTable.end());
}

//-------------------------------------------------------------------------------------------------
CCacheSlot* CGlyphCache::GetLRUSlot()
{
    unsigned int    dwMinUsage = 0xffffffff;
    CCacheSlot* pLRUSlot = 0;
    CCacheSlot* pSlot;

    CCacheSlotListItor pItor = m_pSlotList.begin();

    while (pItor != m_pSlotList.end())
    {
        pSlot = *pItor;

        if (pSlot->dwUsage == 0)
        {
            return pSlot;
        }
        else
        {
            if (pSlot->dwUsage < dwMinUsage)
            {
                pLRUSlot = pSlot;
                dwMinUsage = pSlot->dwUsage;
            }
        }

        pItor++;
    }

    return pLRUSlot;
}

//-------------------------------------------------------------------------------------------------
CCacheSlot* CGlyphCache::GetMRUSlot()
{
    unsigned int    dwMaxUsage = 0;
    CCacheSlot* pMRUSlot = 0;
    CCacheSlot* pSlot;

    CCacheSlotListItor pItor = m_pSlotList.begin();

    while (pItor != m_pSlotList.end())
    {
        pSlot = *pItor;

        if (pSlot->dwUsage != 0)
        {
            if (pSlot->dwUsage > dwMaxUsage)
            {
                pMRUSlot = pSlot;
                dwMaxUsage = pSlot->dwUsage;
            }
        }

        pItor++;
    }

    return pMRUSlot;
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::GetGlyph(CGlyphBitmap** pGlyph, int* piHoriAdvance, int* piWidth, int* piHeight, AZ::s32& iCharOffsetX, AZ::s32& iCharOffsetY, uint32 cChar, const Vec2i& glyphSize, const CFFont::FontHintParams& fontHintParams)
{
    CCacheTable::iterator pItor = m_pCacheTable.find(GetCacheSlotKey(cChar, glyphSize));

    if (pItor == m_pCacheTable.end())
    {
        if (!PreCacheGlyph(cChar, glyphSize, fontHintParams))
        {
            return 0;
        }
    }

    pItor = m_pCacheTable.find(GetCacheSlotKey(cChar, glyphSize));

    pItor->second->dwUsage = m_dwUsage++;
    (*pGlyph) = &pItor->second->pGlyphBitmap;

    if (piHoriAdvance)
    {
        *piHoriAdvance = pItor->second->iHoriAdvance;
    }

    if (piWidth)
    {
        *piWidth = pItor->second->iCharWidth;
    }

    if (piHeight)
    {
        *piHeight = pItor->second->iCharHeight;
    }

    iCharOffsetX = pItor->second->iCharOffsetX;
    iCharOffsetY = pItor->second->iCharOffsetY;

    return 1;
}

//-------------------------------------------------------------------------------------------------
Vec2 CGlyphCache::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph)
{
    return m_pFontRenderer.GetKerning(leftGlyph, rightGlyph);
}

//-------------------------------------------------------------------------------------------------
float CGlyphCache::GetAscenderToHeightRatio()
{
    return m_pFontRenderer.GetAscenderToHeightRatio();
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::CreateSlotList(int iListSize)
{
    for (int i = 0; i < iListSize; i++)
    {
        CCacheSlot* pCacheSlot = new CCacheSlot;

        if (!pCacheSlot)
        {
            return 0;
        }

        if (!pCacheSlot->pGlyphBitmap.Create(m_iGlyphBitmapWidth, m_iGlyphBitmapHeight))
        {
            delete pCacheSlot;

            return 0;
        }

        pCacheSlot->Reset();

        pCacheSlot->iCacheSlot = i;

        m_pSlotList.push_back(pCacheSlot);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CGlyphCache::ReleaseSlotList()
{
    CCacheSlotListItor pItor = m_pSlotList.begin();

    while (pItor != m_pSlotList.end())
    {
        (*pItor)->pGlyphBitmap.Release();

        delete (*pItor);

        pItor = m_pSlotList.erase(pItor);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
CryFont::GlyphCache::CCacheTableKey CGlyphCache::GetCacheSlotKey(uint32 cChar, const Vec2i& glyphSize) const
{
    const Vec2i clampedGlyphSize = CFontTexture::ClampGlyphSize(glyphSize, m_iGlyphBitmapWidth, m_iGlyphBitmapHeight);
    return CryFont::GlyphCache::CCacheTableKey(clampedGlyphSize, cChar);
}

#endif // #if !defined(USE_NULLFONT_ALWAYS)
