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

//
// Purpose:
//  - Render a glyph outline into a bitmap using FreeType 2

#include "CryFont_precompiled.h"

#if !defined(USE_NULLFONT_ALWAYS)

#include "FontRenderer.h"

#include <freetype/ftoutln.h>
#include <freetype/ftglyph.h>
#include <freetype/ftimage.h>

#include <AzCore/Casting/lossy_cast.h>

// Sizes are defined in in 26.6 fixed float format (TT_F26Dot6), where
// 1 unit is 1/64 of a pixel.
static const int fractionalPixelUnits = 64;

namespace
{
    FT_Int32 GetLoadFlags(CFFont::HintBehavior hintBehavior)
    {
        switch (hintBehavior)
        {
        case CFFont::HintBehavior::NoHinting:
        {
            return FT_LOAD_NO_HINTING;
            break;
        }
        case CFFont::HintBehavior::AutoHint:
        {
            return FT_LOAD_FORCE_AUTOHINT;
            break;
        }
        }
        return FT_LOAD_DEFAULT;
    }

    FT_Int32 GetLoadTarget(CFFont::HintStyle hintStyle)
    {
        if (hintStyle == CFFont::HintStyle::Light)
        {
            return FT_LOAD_TARGET_LIGHT;
        }

        return FT_LOAD_TARGET_NORMAL;
    }

    FT_Render_Mode GetRenderMode(CFFont::HintStyle hintStyle)
    {
        // We use the hint style to drive the render mode also. These should 
        // usually be correlated with each other for best results, even though 
        // they could technically be different.
        if (hintStyle == CFFont::HintStyle::Light)
        {
            return FT_RENDER_MODE_LIGHT;
        }

        return FT_RENDER_MODE_NORMAL;
    }
}

//-------------------------------------------------------------------------------------------------
CFontRenderer::CFontRenderer()
    : m_pLibrary(0)
    , m_pFace(0)
    , m_pGlyph(0)
    , m_fSizeRatio(IFFontConstants::defaultSizeRatio)
    , m_pEncoding(FONT_ENCODING_UNICODE)
    , m_iGlyphBitmapWidth(0)
    , m_iGlyphBitmapHeight(0)
{
}

//-------------------------------------------------------------------------------------------------
CFontRenderer::~CFontRenderer()
{
    FT_Done_Face(m_pFace);
    ;
    FT_Done_FreeType(m_pLibrary);
    m_pFace = NULL;
    m_pLibrary = NULL;
}

//-------------------------------------------------------------------------------------------------
int CFontRenderer::LoadFromFile(const string& szFileName)
{
    int iError = FT_Init_FreeType(&m_pLibrary);

    if (iError)
    {
        return 0;
    }

    if (m_pFace)
    {
        FT_Done_Face(m_pFace);
        m_pFace = 0;
    }

    iError = FT_New_Face(m_pLibrary, szFileName.c_str(), 0, &m_pFace);

    if (iError)
    {
        return 0;
    }

    SetEncoding(FONT_ENCODING_UNICODE);

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontRenderer::LoadFromMemory(unsigned char* pBuffer, int iBufferSize)
{
    int iError = FT_Init_FreeType(&m_pLibrary);

    if (iError)
    {
        return 0;
    }

    if (m_pFace)
    {
        FT_Done_Face(m_pFace);
        m_pFace = 0;
    }
    iError = FT_New_Memory_Face(m_pLibrary, pBuffer, iBufferSize, 0, &m_pFace);

    if (iError)
    {
        return 0;
    }

    SetEncoding(FONT_ENCODING_UNICODE);

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontRenderer::Release()
{
    FT_Done_Face(m_pFace);
    ;
    FT_Done_FreeType(m_pLibrary);
    m_pFace = NULL;
    m_pLibrary = NULL;

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontRenderer::SetGlyphBitmapSize(int iWidth, int iHeight, float sizeRatio)
{
    m_iGlyphBitmapWidth = iWidth;
    m_iGlyphBitmapHeight = iHeight;

    // Assign the given scale for texture slots as long as its positive
    m_fSizeRatio = sizeRatio > 0.0f ? sizeRatio : m_fSizeRatio;

    FT_Set_Pixel_Sizes(m_pFace, (int)(m_iGlyphBitmapWidth * m_fSizeRatio), (int)(m_iGlyphBitmapHeight * m_fSizeRatio));

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontRenderer::GetGlyphBitmapSize(int* pWidth, int* pHeight)
{
    if (pWidth)
    {
        *pWidth = m_iGlyphBitmapWidth;
    }

    if (pHeight)
    {
        *pHeight = m_iGlyphBitmapHeight;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontRenderer::SetEncoding(FT_Encoding pEncoding)
{
    if (FT_Select_Charmap(m_pFace, pEncoding))
    {
        return 0;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int CFontRenderer::GetGlyph(CGlyphBitmap* pGlyphBitmap, int* iHoriAdvance, uint8* iGlyphWidth, uint8* iGlyphHeight, AZ::s32& iCharOffsetX, AZ::s32& iCharOffsetY, int iX, int iY, int iCharCode, const CFFont::FontHintParams& fontHintParams)
{
    FT_Int32 loadFlags = GetLoadFlags(fontHintParams.hintBehavior);
    loadFlags |= GetLoadTarget(fontHintParams.hintStyle);

    int iError = FT_Load_Char(m_pFace, iCharCode, loadFlags);

    if (iError)
    {
        return 0;
    }

    FT_Render_Mode renderMode = GetRenderMode(fontHintParams.hintStyle);

    m_pGlyph = m_pFace->glyph;

    iError = FT_Render_Glyph(m_pGlyph, renderMode);

    if (iError)
    {
        return 0;
    }

    if (iHoriAdvance)
    {
        *iHoriAdvance = m_pGlyph->metrics.horiAdvance / fractionalPixelUnits;
    }

    if (iGlyphWidth)
    {
        *iGlyphWidth = m_pGlyph->bitmap.width;
    }

    if (iGlyphHeight)
    {
        *iGlyphHeight = m_pGlyph->bitmap.rows;
    }

    unsigned char* pBuffer = pGlyphBitmap->GetBuffer();
    AZ_Assert(pBuffer, "CGlyphBitmap: bad buffer");

    uint32 dwGlyphWidth = pGlyphBitmap->GetWidth();
    iCharOffsetX = m_pGlyph->bitmap_left;
    iCharOffsetY = (static_cast<AZ::s32>(round(m_iGlyphBitmapHeight * m_fSizeRatio)) - m_pGlyph->bitmap_top);

    const int textureSlotBufferWidth = pGlyphBitmap->GetWidth();
    const int textureSlotBufferHeight = pGlyphBitmap->GetHeight();

    // might happen if font characters are too big or cache dimenstions in font.xml is too small "<font path="VeraMono.ttf" w="320" h="368"/>"
    const bool charWidthFits = iX + m_pGlyph->bitmap.width <= textureSlotBufferWidth;
    const bool charHeightFits = iY + m_pGlyph->bitmap.rows <= textureSlotBufferHeight;
    const bool charFitsInSlot = charWidthFits && charHeightFits;
    AZ_Error("Font", charFitsInSlot, "Character code %d doesn't fit in font texture; check 'sizeRatio' attribute in font XML or adjust this character's sizing in the font.", iCharCode);

    // Since we might be re-rendering/overwriting a glyph that already exists
    // in the font texture, clear the contents of this particular slot so no
    // artifacts of the previous glyph remain.
    pGlyphBitmap->Clear();

    // Restrict iteration to smallest of either the texture slot or glyph 
    // bitmap buffer ranges
    const int bufferMaxIterWidth = AZStd::GetMin<int>(textureSlotBufferWidth, m_pGlyph->bitmap.width);
    const int bufferMaxIterHeight = AZStd::GetMin<int>(textureSlotBufferHeight, m_pGlyph->bitmap.rows);

    for (int i = 0; i < bufferMaxIterHeight; i++)
    {
        int iNewY = i + iY;

        for (int j = 0; j < bufferMaxIterWidth; j++)
        {
            unsigned char   cColor = m_pGlyph->bitmap.buffer[(i * m_pGlyph->bitmap.width) + j];
            int             iOffset = iNewY * dwGlyphWidth + iX + j;

            if (iOffset >= (int)dwGlyphWidth * m_iGlyphBitmapHeight)
            {
                continue;
            }

            pBuffer[iOffset] = cColor;
            //          pBuffer[iOffset] = cColor/2+32;     // debug - visualize character in a block
        }
    }

    return 1;
}

int CFontRenderer::GetGlyphScaled([[maybe_unused]] CGlyphBitmap* pGlyphBitmap, [[maybe_unused]] int* iGlyphWidth, [[maybe_unused]] int* iGlyphHeight, [[maybe_unused]] int iX, [[maybe_unused]] int iY, [[maybe_unused]] float fScaleX, [[maybe_unused]] float fScaleY, [[maybe_unused]] int iCharCode)
{
    return 1;
}

Vec2 CFontRenderer::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph)
{
    FT_Vector kerningOffsets;
    kerningOffsets.x = kerningOffsets.y = 0;

    if (FT_HAS_KERNING(m_pFace))
    {
        const FT_UInt leftGlyphIndex = FT_Get_Char_Index(m_pFace, leftGlyph);
        const FT_UInt rightGlyphIndex = FT_Get_Char_Index(m_pFace, rightGlyph);

        FT_Error ftError = FT_Get_Kerning(m_pFace, leftGlyphIndex, rightGlyphIndex, FT_KERNING_DEFAULT, &kerningOffsets);

#if !defined(_RELEASE)
        if (0 != ftError)
        {
            string warnMsg;
            warnMsg.Format("FT_Get_Kerning returned %d", ftError);
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, warnMsg.c_str());
        }
#endif
    }

    return Vec2(azlossy_cast<float>(kerningOffsets.x / fractionalPixelUnits), azlossy_cast<float>(kerningOffsets.y / fractionalPixelUnits));
}

float CFontRenderer::GetAscenderToHeightRatio()
{
    return (static_cast<float>(m_pFace->ascender) / static_cast<float>(m_pFace->height));
}

//-------------------------------------------------------------------------------------------------
/*
int CFontRenderer::FT_GetIndex(int iCharCode)
{
if (iCharCode < 256)
{ 
int iIndex = 0;
int iUnicode;

// try unicode
for (int i = 0; i < m_pFace->num_charmaps; i++)
{
if ((m_pFace->charmaps[i]->platform_id == 3) && (m_pFace->charmaps[i]->encoding_id == 1))
{
iUnicode = i;

FT_Set_Charmap(m_pFace, m_pFace->charmaps[i]);

iIndex = FT_Get_Char_Index(m_pFace, iCharCode);

// not unicode, try ascii
if (iIndex == 0)
{
for (int i = 0; i < m_pFace->num_charmaps; i++)
{
if ((m_pFace->charmaps[i]->platform_id == 0) && (m_pFace->charmaps[i]->encoding_id == 0))
{
FT_Set_Charmap(m_pFace, m_pFace->charmaps[i]);

iIndex = FT_Get_Char_Index(m_pFace, iCharCode);

// not ascii either, reuse unicode default "missing char"
if (iIndex == 0)
{
FT_Set_Charmap(m_pFace, m_pFace->charmaps[iUnicode]);

return FT_Get_Char_Index(m_pFace, iCharCode);
}
}
}
}

return  iIndex;
}
}

return 0;
}
else
{
for (int i = 0; i < m_pFace->num_charmaps; i++)
{
if ((m_pFace->charmaps[i]->platform_id == 3) && (m_pFace->charmaps[i]->encoding_id == 1))
{
FT_Set_Charmap(m_pFace, m_pFace->charmaps[i]);

return FT_Get_Char_Index(m_pFace, iCharCode);
}
}

return 0;
}

return 0;
}
*/

#endif // #if !defined(USE_NULLFONT_ALWAYS)
