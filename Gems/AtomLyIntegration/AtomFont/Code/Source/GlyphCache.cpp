/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Purpose:
//  - Manage and cache glyphs, retrieving them from the renderer as needed

#include <AtomLyIntegration/AtomFont/AtomFont_precompiled.h>

#if !defined(USE_NULLFONT_ALWAYS)

#include <AtomLyIntegration/AtomFont/GlyphCache.h>
#include <AtomLyIntegration/AtomFont/FontTexture.h>



//-------------------------------------------------------------------------------------------------
AZ::GlyphCache::GlyphCache()
    : m_usage(1)
    , m_glyphBitmapWidth(0)
    , m_glyphBitmapHeight(0)
    , m_scaleBitmap(0)
{
    m_cacheTable.clear();
    m_slotList.clear();
}

//-------------------------------------------------------------------------------------------------
AZ::GlyphCache::~GlyphCache()
{
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::Create(int iCacheSize, int glyphBitmapWidth, int glyphBitmapHeight, AZ::FontSmoothMethod smoothMethod, AZ::FontSmoothAmount smoothAmount, float sizeRatio)
{
    m_smoothMethod = smoothMethod;
    m_smoothAmount = smoothAmount;

    m_glyphBitmapWidth = glyphBitmapWidth;
    m_glyphBitmapHeight = glyphBitmapHeight;


    if (!CreateSlotList(iCacheSize))
    {
        ReleaseSlotList();

        return 0;
    }

    int iScaledGlyphWidth = 0;
    int iScaledGlyphHeight = 0;

    switch (m_smoothMethod)
    {
    case AZ::FontSmoothMethod::SuperSample:
    {
        switch (m_smoothAmount)
        {
        case AZ::FontSmoothAmount::x2:
            iScaledGlyphWidth = m_glyphBitmapWidth << 1;
            iScaledGlyphHeight = m_glyphBitmapHeight << 1;
            break;
        case AZ::FontSmoothAmount::x4:
            iScaledGlyphWidth = m_glyphBitmapWidth << 2;
            iScaledGlyphHeight = m_glyphBitmapHeight << 2;
            break;
        }
    }
    break;
    }

    if (iScaledGlyphWidth)
    {
        m_scaleBitmap = new GlyphBitmap;

        if (!m_scaleBitmap)
        {
            Release();

            return 0;
        }

        if (!m_scaleBitmap->Create(iScaledGlyphWidth, iScaledGlyphHeight))
        {
            Release();

            return 0;
        }

        m_fontRenderer.SetGlyphBitmapSize(iScaledGlyphWidth, iScaledGlyphHeight, sizeRatio);
    }
    else
    {
        m_fontRenderer.SetGlyphBitmapSize(m_glyphBitmapWidth, m_glyphBitmapHeight, sizeRatio);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::Release()
{
    ReleaseSlotList();

    m_cacheTable.clear();

    if (m_scaleBitmap)
    {
        m_scaleBitmap->Release();

        delete m_scaleBitmap;

        m_scaleBitmap = 0;
    }

    m_glyphBitmapWidth = 0;
    m_glyphBitmapHeight = 0;

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::LoadFontFromFile(const string& fileName)
{
    return m_fontRenderer.LoadFromFile(fileName);
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::LoadFontFromMemory(unsigned char* fileBuffer, int dataSize)
{
    return m_fontRenderer.LoadFromMemory(fileBuffer, dataSize);
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::ReleaseFont()
{
    m_fontRenderer.Release();

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::GetGlyphBitmapSize(int* width, int* height)
{
    if (width)
    {
        *width = m_glyphBitmapWidth;
    }

    if (height)
    {
        *height = m_glyphBitmapHeight;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
void AZ::GlyphCache::SetGlyphBitmapSize(int width, int height, float sizeRatio)
{
    m_fontRenderer.SetGlyphBitmapSize(width, height, sizeRatio);
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::PreCacheGlyph(uint32_t character, const AtomFont::GlyphSize& glyphSize, const FFont::FontHintParams& fontHintParams)
{
    CacheTable::iterator pItor = m_cacheTable.find(GetCacheSlotKey(character, glyphSize));

    if (pItor != m_cacheTable.end())
    {
        pItor->second->m_usage = m_usage;

        return 1;
    }

    CacheSlot* slot = GetLRUSlot();

    if (!slot)
    {
        return 0;
    }

    if (slot->m_usage > 0)
    {
        UnCacheGlyph(slot->m_currentCharacter, slot->m_glyphSize);
    }

    if (m_scaleBitmap)
    {
        int iOffsetMult = 1;

        switch (m_smoothAmount)
        {
        case AZ::FontSmoothAmount::x2:
            iOffsetMult = 2;
            break;
        case AZ::FontSmoothAmount::x4:
            iOffsetMult = 4;
            break;
        }

        m_scaleBitmap->Clear();

        if (!m_fontRenderer.GetGlyph(m_scaleBitmap, &slot->m_horizontalAdvance, &slot->m_characterWidth, &slot->m_characterHeight, slot->m_characterOffsetX, slot->m_characterOffsetY, 0, 0, character, fontHintParams))
        {
            return 0;
        }

        slot->m_characterWidth >>= iOffsetMult >> 1;
        slot->m_characterHeight >>= iOffsetMult >> 1;

        m_scaleBitmap->BlitScaledTo8(slot->m_glyphBitmap.GetBuffer(), 0, 0, m_scaleBitmap->GetWidth(), m_scaleBitmap->GetHeight(), 0, 0, slot->m_glyphBitmap.GetWidth(), slot->m_glyphBitmap.GetHeight(), slot->m_glyphBitmap.GetWidth());
    }
    else
    {
        if (!m_fontRenderer.GetGlyph(&slot->m_glyphBitmap, &slot->m_horizontalAdvance, &slot->m_characterWidth, &slot->m_characterHeight, slot->m_characterOffsetX, slot->m_characterOffsetY, 0, 0, character, fontHintParams))
        {
            return 0;
        }
    }

    if (m_smoothMethod == AZ::FontSmoothMethod::Blur)
    {
        slot->m_glyphBitmap.Blur(m_smoothAmount);
    }

    slot->m_usage = m_usage;
    slot->m_currentCharacter = character;
    slot->m_glyphSize = glyphSize;

    m_cacheTable.insert(AZStd::pair<CacheTableKey, CacheSlot*>(GetCacheSlotKey(character, glyphSize), slot));

    return 1;
}

int AZ::GlyphCache::UnCacheGlyph(uint32_t character, const AtomFont::GlyphSize& glyphSize)
{
    CacheTable::iterator pItor = m_cacheTable.find(GetCacheSlotKey(character, glyphSize));

    if (pItor != m_cacheTable.end())
    {
        CacheSlot* slot = pItor->second;

        slot->Reset();

        m_cacheTable.erase(pItor);

        return 1;
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::GlyphCached(uint32_t character, const AtomFont::GlyphSize& glyphSize)
{
    return (m_cacheTable.find(GetCacheSlotKey(character, glyphSize)) != m_cacheTable.end());
}

//-------------------------------------------------------------------------------------------------
AZ::CacheSlot* AZ::GlyphCache::GetLRUSlot()
{
    unsigned int    dwMinUsage = 0xffffffff;
    CacheSlot* pLRUSlot = 0;
    CacheSlot* slot;

    CacheSlotListItor pItor = m_slotList.begin();

    while (pItor != m_slotList.end())
    {
        slot = *pItor;

        if (slot->m_usage == 0)
        {
            return slot;
        }
        else
        {
            if (slot->m_usage < dwMinUsage)
            {
                pLRUSlot = slot;
                dwMinUsage = slot->m_usage;
            }
        }

        pItor++;
    }

    return pLRUSlot;
}

//-------------------------------------------------------------------------------------------------
AZ::CacheSlot* AZ::GlyphCache::GetMRUSlot()
{
    unsigned int    dwMaxUsage = 0;
    CacheSlot* pMRUSlot = 0;
    CacheSlot* slot;

    CacheSlotListItor pItor = m_slotList.begin();

    while (pItor != m_slotList.end())
    {
        slot = *pItor;

        if (slot->m_usage != 0)
        {
            if (slot->m_usage > dwMaxUsage)
            {
                pMRUSlot = slot;
                dwMaxUsage = slot->m_usage;
            }
        }

        pItor++;
    }

    return pMRUSlot;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::GetGlyph(AZ::GlyphBitmap** glyph, int* horizontalAdvance, int* width, int* height, int32_t& m_characterOffsetX, int32_t& m_characterOffsetY, uint32_t character, const AZ::AtomFont::GlyphSize& glyphSize, const AZ::FFont::FontHintParams& fontHintParams)
{
    CacheTable::iterator pItor = m_cacheTable.find(GetCacheSlotKey(character, glyphSize));

    if (pItor == m_cacheTable.end())
    {
        if (!PreCacheGlyph(character, glyphSize, fontHintParams))
        {
            return 0;
        }
    }

    pItor = m_cacheTable.find(GetCacheSlotKey(character, glyphSize));

    pItor->second->m_usage = m_usage++;
    (*glyph) = &pItor->second->m_glyphBitmap;

    if (horizontalAdvance)
    {
        *horizontalAdvance = pItor->second->m_horizontalAdvance;
    }

    if (width)
    {
        *width = pItor->second->m_characterWidth;
    }

    if (height)
    {
        *height = pItor->second->m_characterHeight;
    }

    m_characterOffsetX = pItor->second->m_characterOffsetX;
    m_characterOffsetY = pItor->second->m_characterOffsetY;

    return 1;
}

//-------------------------------------------------------------------------------------------------
Vec2 AZ::GlyphCache::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph)
{
    return m_fontRenderer.GetKerning(leftGlyph, rightGlyph);
}

//-------------------------------------------------------------------------------------------------
float AZ::GlyphCache::GetAscenderToHeightRatio()
{
    return m_fontRenderer.GetAscenderToHeightRatio();
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::CreateSlotList(int listSize)
{
    for (int i = 0; i < listSize; i++)
    {
        CacheSlot* cacheSlot = new CacheSlot;

        if (!cacheSlot)
        {
            return 0;
        }

        if (!cacheSlot->m_glyphBitmap.Create(m_glyphBitmapWidth, m_glyphBitmapHeight))
        {
            delete cacheSlot;

            return 0;
        }

        cacheSlot->Reset();

        cacheSlot->m_slotIndex = i;

        m_slotList.push_back(cacheSlot);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::GlyphCache::ReleaseSlotList()
{
    CacheSlotListItor pItor = m_slotList.begin();

    while (pItor != m_slotList.end())
    {
        (*pItor)->m_glyphBitmap.Release();

        delete (*pItor);

        pItor = m_slotList.erase(pItor);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
AZ::GlyphCache::CacheTableKey AZ::GlyphCache::GetCacheSlotKey(uint32_t character, const AZ::AtomFont::GlyphSize& glyphSize) const
{
    const AZ::AtomFont::GlyphSize clampedGlyphSize = AZ::FontTexture::ClampGlyphSize(glyphSize, m_glyphBitmapWidth, m_glyphBitmapHeight);
    return CacheTableKey(clampedGlyphSize, character);
}

#endif // #if !defined(USE_NULLFONT_ALWAYS)
