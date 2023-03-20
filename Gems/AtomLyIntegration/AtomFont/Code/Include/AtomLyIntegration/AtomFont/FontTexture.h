/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(USE_NULLFONT_ALWAYS)

#include <AtomLyIntegration/AtomFont/FontCommon.h>
#include <AtomLyIntegration/AtomFont/GlyphCache.h>
#include <AtomLyIntegration/AtomFont/GlyphBitmap.h>
#include <AtomLyIntegration/AtomFont/AtomFont.h>
#include <AtomLyIntegration/AtomFont/FFont.h>


namespace AZ
{
    //! Stores glyph meta-data read from the font (FreeType).
    //!
    //! \sa CacheSlot
    struct TextureSlot
    {
        AtomFont::GlyphSize m_glyphSize = AtomFont::defaultGlyphSize; //!< Size of the rendered glyph stored in the font texture
        uint16_t            m_slotUsage;                              //!< For LRU strategy, 0xffff is never released
        uint32_t            m_currentCharacter;                       //!< ~0 if not used for characters
        int32_t             m_textureSlot;
        int32_t             m_horizontalAdvance;                      //!< Advance width. See FT_Glyph_Metrics::horiAdvance.
        float               m_texCoords[2];                           //!< Character position in the texture (not yet half texel corrected)
        uint8_t             m_characterWidth;                         //!< Glyph width (in pixel)
        uint8_t             m_characterHeight;                        //!< Glyph height (in pixel)
        int32_t             m_characterOffsetX;                       //!< Glyph's left-side bearing (in pixels). See FT_GlyphSlotRec::bitmap_left.
        int32_t             m_characterOffsetY;                       //!< Glyph's top bearing (in pixels). See FT_GlyphSlotRec::bitmap_top.

        void Reset()
        {
            m_slotUsage = 0;
            m_currentCharacter = std::numeric_limits<uint32_t>::max();
            m_horizontalAdvance = 0;
            m_characterWidth = 0;
            m_characterHeight = 0;
            m_characterOffsetX = 0;
            m_characterOffsetY = 0;
        }

        void SetNotReusable()
        {
            m_slotUsage = 0xffff;
        }
    };

    //! Stores the glyphs of a font within a single cpu texture.
    //!
    //! The texture resolution is configurable, as is the number of slots within
    //! the texture. 
    //!
    //! A texture slot contains a single glyph within the font and are uniform
    //! size throughout the font texture (each slot occupies the same size 
    //! regardless of the size of a glyph being stored, so a '.' occupies the 
    //! same amount of space as a 'W', for example).
    //!
    //! Font glyph buffrs are read from FreeType and copied into the texture.
    //!
    //! \sa TextureSlot, FontRenderer
    class FontTexture
    {
    public:
        FontTexture();
        ~FontTexture();

        int CreateFromFile(const AZStd::string& fileName, int width, int height, AZ::FontSmoothMethod smoothMethod, AZ::FontSmoothAmount smoothAmount, int widthCharCount = 16, int heightCharCount = 16);

        //! Default texture slot width/height is 16x8 slots, allowing for 128 glyphs to be stored in the font texture. This was
        //! previously 16x16, allowing 256 glyphs to be stored. For reference, there are 95 printable ASCII characters, so by
        //! reducing the number of slots, the height of the font texture can be halved (for some nice memory savings). We may
        //! want to make this configurable in the font XML (especially for languages with a large number of printable chars).
        int CreateFromMemory(unsigned char* fileData, int dataSize, int width, int height, AZ::FontSmoothMethod smoothMethod, AZ::FontSmoothAmount smoothAmount, int widthCharCount, int heightCharCount, float sizeRatio);

        int Create(int width, int height, AZ::FontSmoothMethod smoothMethod, AZ::FontSmoothAmount smoothAmount, int widthCharCount = 16, int heightCharCount = 16, float sizeRatio = IFFontConstants::defaultSizeRatio);
        int Release();

        int SetEncoding(FT_Encoding encoding) { return m_glyphCache.SetEncoding(encoding); }
        FT_Encoding GetEncoding() { return m_glyphCache.GetEncoding(); }

        int GetCellWidth() { return m_cellWidth; }
        int GetCellHeight() { return m_cellHeight; }

        int GetWidth() { return m_width; }
        int GetHeight() { return m_height; }

        int GetWidthCellCount() { return m_widthCellCount; }
        int GetHeightCellCount() { return m_heightCellCount; }

        float GetTextureCellWidth() { return m_textureCellWidth; }
        float GetTextureCellHeight() { return m_textureCellHeight; }

        FONT_TEXTURE_TYPE* GetBuffer() { return m_buffer; }

        uint32_t GetSlotChar(int slotIndex) const;
        TextureSlot* GetCharSlot(uint32_t character, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize);
        TextureSlot* GetGradientSlot();

        TextureSlot* GetLRUSlot();
        TextureSlot* GetMRUSlot();

        //! Returns 1 if texture updated, returns 2 if texture not updated, returns 0 on error
        //! \param string A string of glyphs (UTF8) to added to the font texture (if they don't already exist in the font texture)
        //! \param updated is the number of slots updated
        //! \param sizeRatio A sizing scale that gets applied to all glyphs sizes before they are stored in the font texture.
        //! \param glyphSize The resolution to render the glyphs in string at. 
        //! \param glyphFlags Controls hinting behavior for glyphs rendered to the font texture.
        int PreCacheString(const char* string, int* updated = 0, float sizeRatio = IFFontConstants::defaultSizeRatio, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize, const FFont::FontHintParams& glyphFlags = FFont::FontHintParams());
        // Arguments:
        //   slot - function does nothing if this pointer is 0
        void GetTextureCoord(TextureSlot * slot, float texCoords[4], int& characterSizeX, int& characterSizeY, int& m_characterOffsetX, int& m_characterOffsetY, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize) const;
        int GetCharacterWidth(uint32_t character) const;
        //! Gets the horizontal advance for the given glyph/char.
        //! \param character The glyph (UTF32) to get the horizontal advance for.
        //! \param glyphSize The rendered size of the glyph to get the advance for (the same glyph could be stored in the font texture at multiple sizes).
        int GetHorizontalAdvance(uint32_t character, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize) const;
        //  int GetCharHeightByChar(wchar_t character);

        // useful for special feature rendering interleaved with fonts (e.g. box behind the text)
        void CreateGradientSlot();

        int WriteToFile(const AZStd::string& fileName);

        bool GetMonospaced() const { return m_glyphCache.GetMonospaced(); }

        Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph);

        float GetAscenderToHeightRatio();

        //! Clamps the given glyph size to the given max cell width and height dimensions.
        static AtomFont::GlyphSize ClampGlyphSize(const AtomFont::GlyphSize& glyphSize, int cellWidth, int cellHeight);

    private: // ---------------------------------------------------------------

        using TextureSlotList       = std::vector<TextureSlot*>;
        using TextureSlotListItor   = std::vector<TextureSlot*>::iterator;

        using GlyphSizeType = AtomFont::GlyphSize;

        //! Pair for mapping a height and width size to a UTF32 character/glyph
        using TextureSlotKey = AZStd::pair<GlyphSizeType, uint32_t>;

        //! Hasher for texture slot table keys (glyphsize-char code pair)
        //!
        //! Instead of creating our own custom hash, the types are broken down to their
        //! native types (ints) and passed to existing hashes that handle those types.
        struct HashTextureSlotTableKey
        {
            using ArgumentType      = TextureSlotKey;
            using ResultType        = AZStd::size_t;
            using Int32Pair         = AZStd::pair<int32_t, int32_t>;
            using Int32PairU32Pair  = AZStd::pair<Int32Pair, uint32_t>;
            ResultType operator()(const ArgumentType& value) const
            {
                // Utiliize existing hash function for pairs of ints
                AZStd::hash<Int32PairU32Pair> pairHash;
                return pairHash(Int32PairU32Pair(Int32Pair(value.first.x, value.first.y), value.second));
            }
        };

        //! Maps size-specifc UTF32 glyphs to their corresponding texture slots
        using TextureSlotTableEntry     = AZStd::pair<TextureSlotKey, TextureSlot*>;
        using TextureSlotTable          = AZStd::unordered_map<TextureSlotKey, TextureSlot*, HashTextureSlotTableKey>;
        using TextureSlotTableItor      = TextureSlotTable::iterator;
        using TextureSlotTableItorConst = TextureSlotTable::const_iterator;

        // --------------------------------
        int CreateSlotList(int listSize);
        int ReleaseSlotList();

        //! Updates the given font texture slot with the given glyph (UTF8) with the given parameters. If the glyph doesn't
        //! exist in the font texture at the given size, then the glyph will be rendered to the font texture with the given
        //! parameters.
        //! \param slotIndex Index of the texture slot to update within the slot list.
        //! \param slotUsage Used for LRU strategy to determine how many times a glyph is referenced for retention within the font texture (before eviction).
        //! \param character UTF32 glyph to store within the slot.
        //! \param sizeRatio A sizing scale that should be applied to the glyph before being stored within the font texture.
        //! \param glyphSize The size of the glyph to be rendered at within the font texture.
        //! \param glyphFlags Specifies hinting behavior that should be applied to the glyph when rendered to the font texture.
        int UpdateSlot(int slotIndex, uint16_t slotUsage, uint32_t character, float sizeRatio, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize, const FFont::FontHintParams& glyphFlags = FFont::FontHintParams());

        TextureSlotKey GetTextureSlotKey(uint32_t character, const AtomFont::GlyphSize& glyphSize = AtomFont::defaultGlyphSize) const;

        //! Calculates scaling info that should be applied when the rendered glyph size doesn't match the maximum glyph slot resolution.
        //!
        //! Glyphs can be re-rendered to glyph slots at smaller resolutions for pixel-perfect resolution (rather than applying a scale
        //! to glyphs rendered at larger sizes). These scaling values allow clients of the font texture to use the glyphs without regard
        //! to whether the glyphs have been re-rendered or not.
        float GetRequestSizeWidthScale(const AtomFont::GlyphSize& glyphSize) const
        {
            const int cellWidth = m_cellWidth == 0 ? 1 : m_cellWidth;
            const float invCellWidth = 1.0f / cellWidth;
            return glyphSize.x > 0 ? glyphSize.x * invCellWidth : 1.0f;
        }

        //! Calculates scaling info that should be applied when the rendered glyph size doesn't match the maximum glyph slot resolution.
        //!
        //! Glyphs can be re-rendered to glyph slots at smaller resolutions for pixel-perfect resolution (rather than applying a scale
        //! to glyphs rendered at larger sizes). These scaling values allow clients of the font texture to use the glyphs without regard
        //! to whether the glyphs have been re-rendered or not.
        float GetRequestSizeHeightScale(const AtomFont::GlyphSize& glyphSize) const
        {
            const int cellHeight = m_cellHeight == 0 ? 1 : m_cellHeight;
            const float invCellHeight = 1.0f / cellHeight;
            return glyphSize.y > 0 ? glyphSize.y * invCellHeight : 1.0f;
        }

        // --------------------------------

        int                         m_width;                           // whole texture cache width
        int                         m_height;                          // whole texture cache height
    
        float                       m_invWidth;
        float                       m_invHeight;
    
        int                         m_cellWidth;
        int                         m_cellHeight;
    
        float                       m_textureCellWidth;
        float                       m_textureCellHeight;
    
        int                         m_widthCellCount;
        int                         m_heightCellCount;
    
        int                         m_textureSlotCount;
    
        FontSmoothMethod            m_smoothMethod;
        FontSmoothAmount            m_smoothAmount;
    
        GlyphCache                  m_glyphCache;
        TextureSlotList             m_slotList;
        TextureSlotTable            m_slotIndexMap;

        FONT_TEXTURE_TYPE*          m_buffer;                           // [y*width * x] x=0..width-1, y=0..height-1

        uint16_t                    m_slotUsage;
    };
}
#endif // #if !defined(USE_NULLFONT_ALWAYS)
