/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Render a glyph outline into a bitmap using FreeType 2

#if !defined(USE_NULLFONT_ALWAYS)

#pragma once

#include <AtomLyIntegration/AtomFont/GlyphBitmap.h>
#include <AtomLyIntegration/AtomFont/FFont.h>
#include <ft2build.h>
#pragma push_macro("generic")
#define generic GenericFromFreeTypeLibrary
#include <freetype/freetype.h>
#undef generic
#pragma pop_macro("generic")


// Corresponds to the Unicode character set. This value covers all versions of the Unicode repertoire,
// including ASCII and Latin-1. Most fonts include a Unicode charmap, but not all of them.
#define AZ_FONT_ENCODING_UNICODE       (FT_ENCODING_UNICODE)

// Corresponds to the Microsoft Symbol encoding, used to encode mathematical symbols in the 32..255 character code range.
// For more information, see `http://www.ceviz.net/symbol.htm'.
#define AZ_FONT_ENCODING_SYMBOL        (FT_ENCODING_MS_SYMBOL)

// Corresponds to Microsoft's Japanese SJIS encoding.
// More info at `http://langsupport.japanreference.com/encoding.shtml'. See note on multi-byte encodings below.
#define AZ_FONT_ENCODING_SJIS          (FT_ENCODING_MS_SJIS)

// Corresponds to the encoding system for Simplified Chinese, as used in China. Only found in some TrueType fonts.
#define AZ_FONT_ENCODING_GB2312        (FT_ENCODING_MS_GB2312)

// Corresponds to the encoding system for Traditional Chinese, as used in Taiwan and Hong Kong. Only found in some TrueType fonts.
#define AZ_FONT_ENCODING_BIG5          (FT_ENCODING_MS_BIG5)

// Corresponds to the Korean encoding system known as Wansung.
// This is a Microsoft encoding that is only found in some TrueType fonts.
// For more information, see `http://www.microsoft.com/typography/unicode/949.txt'.
#define AZ_FONT_ENCODING_WANSUNG       (FT_ENCODING_MS_WANSUNG)

// The Korean standard character set (KS C-5601-1992), which corresponds to Windows code page 1361.
// This character set includes all possible Hangeul character combinations. Only found on some rare TrueType fonts.
#define AZ_FONT_ENCODING_JOHAB     (FT_ENCODING_MS_JOHAB)


namespace AZ
{
    class FontRenderer
    {
    public:

        FontRenderer();
        ~FontRenderer();

        int         LoadFromFile(const AZStd::string& fileName);
        int         LoadFromMemory(unsigned char* buffer, int bufferSize);
        int         Release();

        int         SetGlyphBitmapSize(int width, int height, float sizeRatio);
        int         GetGlyphBitmapSize(int*width, int* height);

        int         SetSizeRatio(float sizeRatio) { m_sizeRatio = sizeRatio; return 1; };
        float       GetSizeRatio() { return m_sizeRatio; };

        int         SetEncoding(FT_Encoding encoding);
        FT_Encoding GetEncoding() { return m_encoding; };

        //! Populates the given glyphBitmap's buffer from the FreeType bitmap buffer
        //! \param characterCode Used as a character index to retrieve the FreeType glyph and it's associated bitmap buffer for the character
        //! \param glyphBitmap The FreeType glyph buffer is essentially copied into this GlyphBitmap buffer
        int         GetGlyph(GlyphBitmap* glyphBitmap, int* horizontalAdvance, uint8_t* glyphWidth, uint8_t* glyphHeight, int32_t& m_characterOffsetX, int32_t& m_characterOffsetY, int iX, int iY, int characterCode, const FFont::FontHintParams& glyphFlags = FFont::FontHintParams());
        int         GetGlyphScaled(GlyphBitmap* glyphBitmap, int* glyphWidth, int* glyphHeight, int iX, int iY, float scaleX, float scaleY, int characterCode);

        bool GetMonospaced() const { return FT_IS_FIXED_WIDTH(m_face) != 0; }

        Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph);

        float GetAscenderToHeightRatio();

    private:

        FT_Library      m_library;
        FT_Face         m_face;
        FT_GlyphSlot    m_glyph;
        float           m_sizeRatio;

        FT_Encoding     m_encoding;

        int             m_glyphBitmapWidth;
        int             m_glyphBitmapHeight;
    };
} // namespace AZ

#endif // #if !defined(USE_NULLFONT_ALWAYS)
