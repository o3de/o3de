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

// Description : Render a glyph outline into a bitmap using FreeType 2

#if !defined(USE_NULLFONT_ALWAYS)

#ifndef CRYINCLUDE_CRYFONT_FONTRENDERER_H
#define CRYINCLUDE_CRYFONT_FONTRENDERER_H
#pragma once



#include "GlyphBitmap.h"
#include "FFont.h"
#include <ft2build.h>
#pragma push_macro("generic")
#define generic GenericFromFreeTypeLibrary
#include <freetype/freetype.h>
#undef generic
#pragma pop_macro("generic")


// Corresponds to the Unicode character set. This value covers all versions of the Unicode repertoire,
// including ASCII and Latin-1. Most fonts include a Unicode charmap, but not all of them.
#define FONT_ENCODING_UNICODE       (FT_ENCODING_UNICODE)

// Corresponds to the Microsoft Symbol encoding, used to encode mathematical symbols in the 32..255 character code range.
// For more information, see `http://www.ceviz.net/symbol.htm'.
#define FONT_ENCODING_SYMBOL        (FT_ENCODING_MS_SYMBOL)

// Corresponds to Microsoft's Japanese SJIS encoding.
// More info at `http://langsupport.japanreference.com/encoding.shtml'. See note on multi-byte encodings below.
#define FONT_ENCODING_SJIS          (FT_ENCODING_MS_SJIS)

// Corresponds to the encoding system for Simplified Chinese, as used in China. Only found in some TrueType fonts.
#define FONT_ENCODING_GB2312        (FT_ENCODING_MS_GB2312)

// Corresponds to the encoding system for Traditional Chinese, as used in Taiwan and Hong Kong. Only found in some TrueType fonts.
#define FONT_ENCODING_BIG5          (FT_ENCODING_MS_BIG5)

// Corresponds to the Korean encoding system known as Wansung.
// This is a Microsoft encoding that is only found in some TrueType fonts.
// For more information, see `http://www.microsoft.com/typography/unicode/949.txt'.
#define FONT_ENCODING_WANSUNG       (FT_ENCODING_MS_WANSUNG)

// The Korean standard character set (KS C-5601-1992), which corresponds to Windows code page 1361.
// This character set includes all possible Hangeul character combinations. Only found on some rare TrueType fonts.
#define FONT_ENCODING_JOHAB     (FT_ENCODING_MS_JOHAB)



//------------------------------------------------------------------------------------
class CFontRenderer
{
public:

    CFontRenderer();
    ~CFontRenderer();

    int         LoadFromFile(const string& szFileName);
    int         LoadFromMemory(unsigned char* pBuffer, int iBufferSize);
    int         Release();

    int         SetGlyphBitmapSize(int iWidth, int iHeight, float sizeRatio);
    int         GetGlyphBitmapSize(int* pWidth, int* pHeight);

    int         SetSizeRatio(float fSizeRatio) { m_fSizeRatio = fSizeRatio; return 1; };
    float       GetSizeRatio() { return m_fSizeRatio; };

    int         SetEncoding(FT_Encoding pEncoding);
    FT_Encoding GetEncoding() { return m_pEncoding; };

    //! Populates the given pGlyphBitmap's buffer from the FreeType bitmap buffer
    //! \param iCharCode Used as a character index to retrieve the FreeType glyph and it's associated bitmap buffer for the character
    //! \param pGlyphBitmap The FreeType glyph buffer is essentially copied into this CGlyphBitmap buffer
    int         GetGlyph(CGlyphBitmap* pGlyphBitmap, int* iHoriAdvance, uint8* iGlyphWidth, uint8* iGlyphHeight, AZ::s32& iCharOffsetX, AZ::s32& iCharOffsetY, int iX, int iY, int iCharCode, const CFFont::FontHintParams& glyphFlags = CFFont::FontHintParams());
    int         GetGlyphScaled(CGlyphBitmap* pGlyphBitmap, int* iGlyphWidth, int* iGlyphHeight, int iX, int iY, float fScaleX, float fScaleY, int iCharCode);

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const {}

    bool GetMonospaced() const { return FT_IS_FIXED_WIDTH(m_pFace) != 0; }

    Vec2 GetKerning(uint32_t leftGlyph, uint32_t rightGlyph);

    float GetAscenderToHeightRatio();

private:

    FT_Library      m_pLibrary;
    FT_Face         m_pFace;
    FT_GlyphSlot    m_pGlyph;
    float           m_fSizeRatio;

    FT_Encoding     m_pEncoding;

    int             m_iGlyphBitmapWidth;
    int             m_iGlyphBitmapHeight;
};

#endif // CRYINCLUDE_CRYFONT_FONTRENDERER_H

#endif // #if !defined(USE_NULLFONT_ALWAYS)
