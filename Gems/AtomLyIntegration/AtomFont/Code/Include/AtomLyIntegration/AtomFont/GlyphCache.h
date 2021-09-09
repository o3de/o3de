/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Purpose:
//  - Manage and cache glyphs, retrieving them from the renderer as needed

#pragma once

#if !defined(USE_NULLFONT_ALWAYS)

#include <vector>
#include <AtomLyIntegration/AtomFont/GlyphBitmap.h>
#include <AtomLyIntegration/AtomFont/FontRenderer.h>
#include <AtomLyIntegration/AtomFont/AtomFont.h>
#include <StlUtils.h>
#include <AtomLyIntegration/AtomFont/FFont.h>


namespace AZ
{
    //! Glyph cache slots store the bitmap buffer and glyph metadata from FreeType.
    //!
    //! This bitmap buffer is eventually copied to a FontTexture texture buffer.
    //! A glyph cache slot bitmap buffer only holds a single glyph, whereas the 
    //! FontTexture stores multiple glyphs in a grid (row/col) format.
    struct CacheSlot
    {
        AtomFont::GlyphSize m_glyphSize = AtomFont::defaultGlyphSize; //!< The render resolution of the glyph in the glyph bitmap
        unsigned int        m_usage;
        int                 m_slotIndex;
        int                 m_horizontalAdvance;                      //!< Advance width. See FT_Glyph_Metrics::horiAdvance.
        uint32_t            m_currentCharacter;

        uint8_t             m_characterWidth;                         //!< Glyph width (in pixel)
        uint8_t             m_characterHeight;                        //!< Glyph height (in pixel)
        int32_t             m_characterOffsetX;                       //!< Glyph's left-side bearing (in pixels). See FT_GlyphSlotRec::bitmap_left.
        int32_t             m_characterOffsetY;                       //!< Glyph's top bearing (in pixels). See FT_GlyphSlotRec::bitmap_top.

        GlyphBitmap         m_glyphBitmap;                            //!< Contains a buffer storing a copy of the glyph from FreeType

        void            Reset()
        {
            m_usage = 0;
            m_currentCharacter = std::numeric_limits<uint32_t>::max();

            m_characterWidth = 0;
            m_characterHeight = 0;
            m_characterOffsetX = 0;
            m_characterOffsetY = 0;

            m_glyphBitmap.Clear();
        }
    };

    //! The glyph cache maps UTF32 codepoints to their corresponding FreeType data.
    //!
    //! This cache is used to associate font glyph info (read from FreeType) with
    //! UTF32 codepoints. Ultimately the glyph info will be read into a font texture
    //! (FontTexture) to avoid future FreeType lookups.
    //!
    //! If a FontTexture is missing a glyph that is currently stored in the glyph 
    //! cache, the cached data can be returned instead of having to be rendered from
    //! FreeType again.
    //!
    //! \sa FontTexture
    class GlyphCache
    {
    public:
        GlyphCache();
        ~GlyphCache();

        int Create(int iCacheSize, int glyphBitmapWidth, int glyphBitmapHeight, FontSmoothMethod smoothMethod, FontSmoothAmount smoothAmount, float sizeRatio);
        int Release();

        int LoadFontFromFile(const AZStd::string& fileName);
        int LoadFontFromMemory(unsigned char* fileBuffer, int dataSize);
        int ReleaseFont();

        int SetEncoding(FT_Encoding encoding) { return m_fontRenderer.SetEncoding(encoding); };
        FT_Encoding GetEncoding() { return m_fontRenderer.GetEncoding(); };

        int GetGlyphBitmapSize(int* width, int* height);
        void SetGlyphBitmapSize(int width, int height, float sizeRatio);

        int PreCacheGlyph(uint32_t character, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize, const FFont::FontHintParams& glyphFlags = FFont::FontHintParams());
        int UnCacheGlyph(uint32_t character, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize);
        int GlyphCached(uint32_t character, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize);

        CacheSlot* GetLRUSlot();
        CacheSlot* GetMRUSlot();

        //! Obtains glyph information for the given UTF32 codepoint.
        //! This information is obtained from a CacheSlot that corresponds to
        //! the given codepoint. If the codepoint doesn't exist within the cache
        //! table (m_cacheTable), then the information is obtain from FreeType
        //! directly via FontRenderer.
        //!
        //! Ultimately the glyph bitmap is copied into a font texture 
        //! (FontTexture). Once the glyph is copied into the font texture then
        //! the font texture is referenced directly rather than relying on the
        //! glyph cache or FreeType.
        //!
        //! \sa FontRenderer::GetGlyph, FontTexture::UpdateSlot
        int GetGlyph(GlyphBitmap** glyph, int* horizontalAdvance, int* width, int* height, int32_t& m_characterOffsetX, int32_t& m_characterOffsetY, uint32_t character, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize, const FFont::FontHintParams& glyphFlags = FFont::FontHintParams());

        bool GetMonospaced() const { return m_fontRenderer.GetMonospaced(); }

        Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph);

        float GetAscenderToHeightRatio();

    private:
        //! Height and width pair for glyph size mapping
        using CacheTableGlyphSizeType = AtomFont::GlyphSize;

        //! Pair for mapping a height and width size to a UTF32 character/glyph
        using CacheTableKey = AZStd::pair<CacheTableGlyphSizeType, uint32_t>;

        //! Hasher for glyph cache table keys (glyphsize-char code pair)
        //!
        //! Instead of creating our own custom hash, the types are broken down to their
        //! native types (ints) and passed to existing hashes that handle those types.
        struct HashGlyphCacheTableKey
        {
            using ArgumentType      = CacheTableKey;
            using ResultType        = AZStd::size_t;
            using Int32Pair         = AZStd::pair<int32_t, int32_t>;
            using Int32PairU32Pair  = AZStd::pair<Int32Pair, uint32_t>;
            ResultType operator()(const ArgumentType& value) const
            {
                AZStd::hash<Int32PairU32Pair> pairHash;
                return pairHash(Int32PairU32Pair(Int32Pair(value.first.x, value.first.y), value.second));
            }
        };

        //! Maps size-specifc UTF32 glyphs to their corresponding cache slots
        using CacheTable        = AZStd::unordered_map<CacheTableKey, CacheSlot*, HashGlyphCacheTableKey>;

        using CacheSlotList     = std::vector<CacheSlot*>;
        using CacheSlotListItor = std::vector<CacheSlot*>::iterator;


        //! Returns a key for the cache table where the given char is mapped at the given size.
        CacheTableKey GetCacheSlotKey(uint32_t character, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize) const;

        int             CreateSlotList(int listSize);
        int             ReleaseSlotList();

        CacheSlotList  m_slotList;
        CacheTable     m_cacheTable;

        int             m_glyphBitmapWidth;
        int             m_glyphBitmapHeight;

        FontSmoothMethod m_smoothMethod;
        FontSmoothAmount m_smoothAmount;

        GlyphBitmap* m_scaleBitmap;

        FontRenderer   m_fontRenderer;

        unsigned int    m_usage;
    };

}
#endif // #if !defined(USE_NULLFONT_ALWAYS)
