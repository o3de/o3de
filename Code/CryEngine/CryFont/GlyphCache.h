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

#ifndef CRYINCLUDE_CRYFONT_GLYPHCACHE_H
#define CRYINCLUDE_CRYFONT_GLYPHCACHE_H
#pragma once

#if !defined(USE_NULLFONT_ALWAYS)

#include <vector>
#include "GlyphBitmap.h"
#include "FontRenderer.h"
#include "CryFont.h"
#include <StlUtils.h>

//! Glyph cache slots store the bitmap buffer and glyph metadata from FreeType.
//!
//! This bitmap buffer is eventually copied to a CFontTexture texture buffer.
//! A glyph cache slot bitmap buffer only holds a single glyph, whereas the 
//! CFontTexture stores multiple glyphs in a grid (row/col) format.
typedef struct CCacheSlot
{
    Vec2i           glyphSize = CCryFont::defaultGlyphSize; //!< The render resolution of the glyph in the glyph bitmap
    unsigned int    dwUsage;
    int             iCacheSlot;
    int             iHoriAdvance;                           //!< Advance width. See FT_Glyph_Metrics::horiAdvance.
    uint32          cCurrentChar;

    uint8           iCharWidth;                             //!< Glyph width (in pixel)
    uint8           iCharHeight;                            //!< Glyph height (in pixel)
    AZ::s32         iCharOffsetX;                           //!< Glyph's left-side bearing (in pixels). See FT_GlyphSlotRec::bitmap_left.
    AZ::s32         iCharOffsetY;                           //!< Glyph's top bearing (in pixels). See FT_GlyphSlotRec::bitmap_top.

    CGlyphBitmap    pGlyphBitmap;                           //!< Contains a buffer storing a copy of the glyph from FreeType

    void            Reset()
    {
        dwUsage = 0;
        cCurrentChar = ~0;

        iCharWidth = 0;
        iCharHeight = 0;
        iCharOffsetX = 0;
        iCharOffsetY = 0;

        pGlyphBitmap.Clear();
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(pGlyphBitmap);
    }
} CCacheSlot;

namespace CryFont
{
    namespace GlyphCache
    {
        //! Height and width pair for glyph size mapping
        typedef Vec2i                                               CCacheTableGlyphSizeType;

        //! Pair for mapping a height and width size to a UTF32 character/glyph
        typedef AZStd::pair<CCacheTableGlyphSizeType, uint32>       CCacheTableKey;

        //! Hasher for glyph cache table keys (glyphsize-char code pair)
        //!
        //! Instead of creating our own custom hash, the types are broken down to their
        //! native types (ints) and passed to existing hashes that handle those types.
        struct HashGlyphCacheTableKey
        {
            typedef CCacheTableKey                  ArgumentType;
            typedef AZStd::size_t                   ResultType;
            typedef AZStd::pair<int32, int32>       Int32Pair;
            typedef AZStd::pair<Int32Pair, uint32>  Int32PairU32Pair;
            ResultType operator()(const ArgumentType& value) const
            {
                AZStd::hash<Int32PairU32Pair> pairHash;
                return pairHash(Int32PairU32Pair(Int32Pair(value.first.x, value.first.y), value.second));
            }
        };
    }
}

//! Maps size-speicifc UTF32 glyphs to their corresponding cache slots
typedef AZStd::unordered_map<CryFont::GlyphCache::CCacheTableKey, CCacheSlot*, CryFont::GlyphCache::HashGlyphCacheTableKey> CCacheTable;

typedef std::vector<CCacheSlot*>            CCacheSlotList;
typedef std::vector<CCacheSlot*>::iterator  CCacheSlotListItor;


#ifdef WIN64
#undef GetCharWidth
#undef GetCharHeight
#endif

//! The glyph cache maps UTF32 codepoints to their corresponding FreeType data.
//!
//! This cache is used to associate font glyph info (read from FreeType) with
//! UTF32 codepoints. Ultimately the glyph info will be read into a font texture
//! (CFontTexture) to avoid future FreeType lookups.
//!
//! If a CFontTexture is missing a glyph that is currently stored in the glyph 
//! cache, the cached data can be returned instead of having to be rendered from
//! FreeType again.
//!
//! \sa CFontTexture
class CGlyphCache
{
public:
    CGlyphCache();
    ~CGlyphCache();

    int Create(int iCacheSize, int iGlyphBitmapWidth, int iGlyphBitmapHeight, int iSmoothMethod, int iSmoothAmount, float sizeRatio);
    int Release();

    int LoadFontFromFile(const string& szFileName);
    int LoadFontFromMemory(unsigned char* pFileBuffer, int iDataSize);
    int ReleaseFont();

    int SetEncoding(FT_Encoding pEncoding) { return m_pFontRenderer.SetEncoding(pEncoding); };
    FT_Encoding GetEncoding() { return m_pFontRenderer.GetEncoding(); };

    int GetGlyphBitmapSize(int* pWidth, int* pHeight);
    void SetGlyphBitmapSize(int width, int height, float sizeRatio);

    int PreCacheGlyph(uint32 cChar, const Vec2i& glyphSize = CCryFont::defaultGlyphSize, const CFFont::FontHintParams& glyphFlags = CFFont::FontHintParams());
    int UnCacheGlyph(uint32 cChar, const Vec2i& glyphSize = CCryFont::defaultGlyphSize);
    int GlyphCached(uint32 cChar, const Vec2i& glyphSize = CCryFont::defaultGlyphSize);

    CCacheSlot* GetLRUSlot();
    CCacheSlot* GetMRUSlot();

    //! Obtains glyph information for the given UTF32 codepoint.
    //! This information is obtained from a CCacheSlot that corresponds to
    //! the given codepoint. If the codepoint doesn't exist within the cache
    //! table (m_pCacheTable), then the information is obtain from FreeType
    //! directly via CFontRenderer.
    //!
    //! Ultimately the glyph bitmap is copied into a font texture 
    //! (CFontTexture). Once the glyph is copied into the font texture then
    //! the font texture is referenced directly rather than relying on the
    //! glyph cache or FreeType.
    //!
    //! \sa CFontRenderer::GetGlyph, CFontTexture::UpdateSlot
    int GetGlyph(CGlyphBitmap** pGlyph, int* piHoriAdvance, int* piWidth, int* piHeight, AZ::s32& iCharOffsetX, AZ::s32& iCharOffsetY, uint32 cChar, const Vec2i& glyphSize = CCryFont::defaultGlyphSize, const CFFont::FontHintParams& glyphFlags = CFFont::FontHintParams());

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pSlotList);
        //pSizer->AddContainer(m_pCacheTable);
        pSizer->AddObject(m_pScaleBitmap);
        pSizer->AddObject(m_pFontRenderer);
    }

    bool GetMonospaced() const { return m_pFontRenderer.GetMonospaced(); }

    Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph);

    float GetAscenderToHeightRatio();

private:

    //! Returns a key for the cache table where the given char is mapped at the given size.
    CryFont::GlyphCache::CCacheTableKey GetCacheSlotKey(uint32 cChar, const Vec2i& glyphSize = CCryFont::defaultGlyphSize) const;

    int             CreateSlotList(int iListSize);
    int             ReleaseSlotList();

    CCacheSlotList  m_pSlotList;
    CCacheTable     m_pCacheTable;

    int             m_iGlyphBitmapWidth;
    int             m_iGlyphBitmapHeight;

    int             m_iSmoothMethod;
    int             m_iSmoothAmount;

    CGlyphBitmap* m_pScaleBitmap;

    CFontRenderer   m_pFontRenderer;

    unsigned int    m_dwUsage;
};

#endif // #if !defined(USE_NULLFONT_ALWAYS)

#endif // CRYINCLUDE_CRYFONT_GLYPHCACHE_H
