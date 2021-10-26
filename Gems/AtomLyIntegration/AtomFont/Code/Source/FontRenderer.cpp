/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


//
// Purpose:
//  - Render a glyph outline into a bitmap using FreeType 2


#if !defined(USE_NULLFONT_ALWAYS)

#include <AtomLyIntegration/AtomFont/FontRenderer.h>

#include <freetype/ftoutln.h>
#include <freetype/ftglyph.h>
#include <freetype/ftimage.h>

#include <AzCore/Casting/lossy_cast.h>

#include <CryCommon/ISystem.h>

// Sizes are defined in in 26.6 fixed float format (TT_F26Dot6), where
// 1 unit is 1/64 of a pixel.
constexpr int FractionalPixelUnits = 64;

namespace
{
    FT_Int32 GetLoadFlags(AZ::FFont::HintBehavior hintBehavior)
    {
        switch (hintBehavior)
        {
        case AZ::FFont::HintBehavior::NoHinting:
        {
            return FT_LOAD_NO_HINTING;
            break;
        }
        case AZ::FFont::HintBehavior::AutoHint:
        {
            return FT_LOAD_FORCE_AUTOHINT;
            break;
        }
        }
        return FT_LOAD_DEFAULT;
    }

    FT_Int32 GetLoadTarget(AZ::FFont::HintStyle hintStyle)
    {
        if (hintStyle == AZ::FFont::HintStyle::Light)
        {
            return FT_LOAD_TARGET_LIGHT;
        }

        return FT_LOAD_TARGET_NORMAL;
    }

    FT_Render_Mode GetRenderMode(AZ::FFont::HintStyle hintStyle)
    {
        // We use the hint style to drive the render mode also. These should 
        // usually be correlated with each other for best results, even though 
        // they could technically be different.
        if (hintStyle == AZ::FFont::HintStyle::Light)
        {
            return FT_RENDER_MODE_LIGHT;
        }

        return FT_RENDER_MODE_NORMAL;
    }
}


//-------------------------------------------------------------------------------------------------
AZ::FontRenderer::FontRenderer()
    : m_library(0)
    , m_face(0)
    , m_glyph(0)
    , m_sizeRatio(IFFontConstants::defaultSizeRatio)
    , m_encoding(AZ_FONT_ENCODING_UNICODE)
    , m_glyphBitmapWidth(0)
    , m_glyphBitmapHeight(0)
{
}

//-------------------------------------------------------------------------------------------------
AZ::FontRenderer::~FontRenderer()
{
    FT_Done_Face(m_face);
    ;
    FT_Done_FreeType(m_library);
    m_face = NULL;
    m_library = NULL;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontRenderer::LoadFromFile(const AZStd::string& fileName)
{
    int iError = FT_Init_FreeType(&m_library);

    if (iError)
    {
        return 0;
    }

    if (m_face)
    {
        FT_Done_Face(m_face);
        m_face = 0;
    }

    iError = FT_New_Face(m_library, fileName.c_str(), 0, &m_face);

    if (iError)
    {
        return 0;
    }

    SetEncoding(AZ_FONT_ENCODING_UNICODE);

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontRenderer::LoadFromMemory(unsigned char* buffer, int bufferSize)
{
    int iError = FT_Init_FreeType(&m_library);

    if (iError)
    {
        return 0;
    }

    if (m_face)
    {
        FT_Done_Face(m_face);
        m_face = 0;
    }
    iError = FT_New_Memory_Face(m_library, buffer, bufferSize, 0, &m_face);

    if (iError)
    {
        return 0;
    }

    SetEncoding(AZ_FONT_ENCODING_UNICODE);

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontRenderer::Release()
{
    FT_Done_Face(m_face);
    ;
    FT_Done_FreeType(m_library);
    m_face = NULL;
    m_library = NULL;

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontRenderer::SetGlyphBitmapSize(int width, int height, float sizeRatio)
{
    m_glyphBitmapWidth = width;
    m_glyphBitmapHeight = height;

    // Assign the given scale for texture slots as long as its positive
    m_sizeRatio = sizeRatio > 0.0f ? sizeRatio : m_sizeRatio;

    FT_Set_Pixel_Sizes(m_face, (int)(m_glyphBitmapWidth * m_sizeRatio), (int)(m_glyphBitmapHeight * m_sizeRatio));

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontRenderer::GetGlyphBitmapSize(int* width, int* height)
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
int AZ::FontRenderer::SetEncoding(FT_Encoding encoding)
{
    if (FT_Select_Charmap(m_face, encoding))
    {
        return 0;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int AZ::FontRenderer::GetGlyph(GlyphBitmap* glyphBitmap, int* horizontalAdvance, uint8_t* glyphWidth, uint8_t* glyphHeight, int32_t& m_characterOffsetX, int32_t& m_characterOffsetY, int iX, int iY, int characterCode, const FFont::FontHintParams& fontHintParams)
{
    FT_Int32 loadFlags = GetLoadFlags(fontHintParams.hintBehavior);
    loadFlags |= GetLoadTarget(fontHintParams.hintStyle);

    int iError = FT_Load_Char(m_face, characterCode, loadFlags);

    if (iError)
    {
        return 0;
    }

    FT_Render_Mode renderMode = GetRenderMode(fontHintParams.hintStyle);

    m_glyph = m_face->glyph;

    iError = FT_Render_Glyph(m_glyph, renderMode);

    if (iError)
    {
        return 0;
    }

    if (horizontalAdvance)
    {
        *horizontalAdvance = m_glyph->metrics.horiAdvance / FractionalPixelUnits;
    }

    if (glyphWidth)
    {
        *glyphWidth = static_cast<uint8_t>(m_glyph->bitmap.width);
    }

    if (glyphHeight)
    {
        *glyphHeight = static_cast<uint8_t>(m_glyph->bitmap.rows);
    }

    unsigned char* buffer = glyphBitmap->GetBuffer();
    AZ_Assert(buffer, "GlyphBitmap: bad buffer");

    uint32_t dwGlyphWidth = glyphBitmap->GetWidth();
    m_characterOffsetX = m_glyph->bitmap_left;
    m_characterOffsetY = (static_cast<int32_t>(round(m_glyphBitmapHeight * m_sizeRatio)) - m_glyph->bitmap_top);

    const int textureSlotBufferWidth = glyphBitmap->GetWidth();
    const int textureSlotBufferHeight = glyphBitmap->GetHeight();

    // might happen if font characters are too big or cache dimenstions in font.xml is too small "<font path="VeraMono.ttf" w="320" h="368"/>"
    const bool charWidthFits = static_cast<int>(iX + m_glyph->bitmap.width) <= textureSlotBufferWidth;
    const bool charHeightFits = static_cast<int>(iY + m_glyph->bitmap.rows) <= textureSlotBufferHeight;
    [[maybe_unused]] const bool charFitsInSlot = charWidthFits && charHeightFits;
    AZ_Error("Font", charFitsInSlot, "Character code %d doesn't fit in font texture; check 'sizeRatio' attribute in font XML or adjust this character's sizing in the font.", characterCode);

    // Since we might be re-rendering/overwriting a glyph that already exists
    // in the font texture, clear the contents of this particular slot so no
    // artifacts of the previous glyph remain.
    glyphBitmap->Clear();

    // Restrict iteration to smallest of either the texture slot or glyph 
    // bitmap buffer ranges
    const int bufferMaxIterWidth = AZStd::GetMin<int>(textureSlotBufferWidth, m_glyph->bitmap.width);
    const int bufferMaxIterHeight = AZStd::GetMin<int>(textureSlotBufferHeight, m_glyph->bitmap.rows);

    for (int i = 0; i < bufferMaxIterHeight; i++)
    {
        int iNewY = i + iY;

        for (int j = 0; j < bufferMaxIterWidth; j++)
        {
            unsigned char   cColor = m_glyph->bitmap.buffer[(i * m_glyph->bitmap.width) + j];
            int             iOffset = iNewY * dwGlyphWidth + iX + j;

            if (iOffset >= (int)dwGlyphWidth * m_glyphBitmapHeight)
            {
                continue;
            }

            buffer[iOffset] = cColor;
            //          buffer[iOffset] = cColor/2+32;     // debug - visualize character in a block
        }
    }

    return 1;
}

int AZ::FontRenderer::GetGlyphScaled([[maybe_unused]] GlyphBitmap* glyphBitmap, [[maybe_unused]] int* glyphWidth, [[maybe_unused]] int* glyphHeight, [[maybe_unused]] int iX, [[maybe_unused]] int iY, [[maybe_unused]] float scaleX, [[maybe_unused]] float scaleY, [[maybe_unused]] int characterCode)
{
    return 1;
}

Vec2 AZ::FontRenderer::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph)
{
    FT_Vector kerningOffsets;
    kerningOffsets.x = kerningOffsets.y = 0;

    if (FT_HAS_KERNING(m_face))
    {
        const FT_UInt leftGlyphIndex = FT_Get_Char_Index(m_face, leftGlyph);
        const FT_UInt rightGlyphIndex = FT_Get_Char_Index(m_face, rightGlyph);

        [[maybe_unused]] FT_Error ftError = FT_Get_Kerning(m_face, leftGlyphIndex, rightGlyphIndex, FT_KERNING_DEFAULT, &kerningOffsets);

#if !defined(_RELEASE)
        if (0 != ftError)
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "FT_Get_Kerning returned %d", ftError);
        }
#endif
    }

    return Vec2(azlossy_cast<float>(kerningOffsets.x / FractionalPixelUnits), azlossy_cast<float>(kerningOffsets.y / FractionalPixelUnits));
}

float AZ::FontRenderer::GetAscenderToHeightRatio()
{
    return (static_cast<float>(m_face->ascender) / static_cast<float>(m_face->height));
}

#endif // #if !defined(USE_NULLFONT_ALWAYS)
