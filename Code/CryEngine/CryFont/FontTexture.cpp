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
//  - Create and update a texture with the most recently used glyphs

#include "CryFont_precompiled.h"

#if !defined(USE_NULLFONT_ALWAYS)

#include "FontTexture.h"
#include "UnicodeIterator.h"
#include <AzCore/IO/FileIO.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetCharWidth
#endif

//-------------------------------------------------------------------------------------------------
CFontTexture::CFontTexture()
    : m_wSlotUsage(1)
    , m_iWidth(0)
    , m_iHeight(0)
    , m_fInvWidth(0.0f)
    , m_fInvHeight(0.0f)
    , m_iCellWidth(0)
    , m_iCellHeight(0)
    , m_fTextureCellWidth(0)
    , m_fTextureCellHeight(0)
    , m_iWidthCellCount(0)
    , m_iHeightCellCount(0)
    , m_nTextureSlotCount(0)
    , m_pBuffer(0)
    , m_iSmoothMethod(FONT_SMOOTH_NONE)
    , m_iSmoothAmount(FONT_SMOOTH_AMOUNT_NONE)
{
}

//-------------------------------------------------------------------------------------------------
CFontTexture::~CFontTexture()
{
    Release();
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::CreateFromFile(const string& szFileName, int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, int iWidthCellCount, int iHeightCellCount)
{
    if (!m_pGlyphCache.LoadFontFromFile(szFileName))
    {
        Release();

        return 0;
    }

    if (!Create(iWidth, iHeight, iSmoothMethod, iSmoothAmount, iWidthCellCount, iHeightCellCount))
    {
        return 0;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::CreateFromMemory(unsigned char* pFileData, int iDataSize, int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, int iWidthCellCount, int iHeightCellCount, float sizeRatio)
{
    if (!m_pGlyphCache.LoadFontFromMemory(pFileData, iDataSize))
    {
        Release();

        return 0;
    }

    if (!Create(iWidth, iHeight, iSmoothMethod, iSmoothAmount, iWidthCellCount, iHeightCellCount, sizeRatio))
    {
        return 0;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::Create(int iWidth, int iHeight, int iSmoothMethod, int iSmoothAmount, int iWidthCellCount, int iHeightCellCount, float sizeRatio)
{
    m_pBuffer = new FONT_TEXTURE_TYPE[iWidth * iHeight];
    if (!m_pBuffer)
    {
        return 0;
    }

    memset(m_pBuffer, 0, iWidth * iHeight * sizeof(FONT_TEXTURE_TYPE));

    if (!(iWidthCellCount * iHeightCellCount))
    {
        return 0;
    }

    m_iWidth = iWidth;
    m_iHeight = iHeight;
    m_fInvWidth = 1.0f / (float)iWidth;
    m_fInvHeight = 1.0f / (float)iHeight;

    m_iWidthCellCount = iWidthCellCount;
    m_iHeightCellCount = iHeightCellCount;
    m_nTextureSlotCount = m_iWidthCellCount * m_iHeightCellCount;

    m_iSmoothMethod = iSmoothMethod;
    m_iSmoothAmount = iSmoothAmount;

    m_iCellWidth = m_iWidth / m_iWidthCellCount;
    m_iCellHeight = m_iHeight / m_iHeightCellCount;

    m_fTextureCellWidth = m_iCellWidth * m_fInvWidth;
    m_fTextureCellHeight = m_iCellHeight * m_fInvHeight;

    if (!m_pGlyphCache.Create(FONT_GLYPH_CACHE_SIZE, m_iCellWidth, m_iCellHeight, iSmoothMethod, iSmoothAmount, sizeRatio))
    {
        Release();

        return 0;
    }

    if (!CreateSlotList(m_nTextureSlotCount))
    {
        Release();

        return 0;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::Release()
{
    delete[] m_pBuffer;
    m_pBuffer = 0;

    ReleaseSlotList();

    m_pSlotTable.clear();

    m_pGlyphCache.Release();

    m_iWidthCellCount = 0;
    m_iHeightCellCount = 0;
    m_nTextureSlotCount = 0;

    m_iWidth = 0;
    m_iHeight = 0;
    m_fInvWidth = 0.0f;
    m_fInvHeight = 0.0f;

    m_iCellWidth = 0;
    m_iCellHeight = 0;

    m_iSmoothMethod = 0;
    m_iSmoothAmount = 0;

    m_fTextureCellWidth = 0.0f;
    m_fTextureCellHeight = 0.0f;

    m_wSlotUsage = 1;

    return 1;
}

//-------------------------------------------------------------------------------------------------
uint32 CFontTexture::GetSlotChar(int iSlot) const
{
    return m_pSlotList[iSlot]->cCurrentChar;
}

//-------------------------------------------------------------------------------------------------
CTextureSlot* CFontTexture::GetCharSlot(uint32 cChar, const Vec2i& glyphSize)
{
    CryFont::FontTexture::CTextureSlotKey slotKey = GetTextureSlotKey(cChar, glyphSize);
    CTextureSlotTableItor pItor = m_pSlotTable.find(slotKey);

    if (pItor != m_pSlotTable.end())
    {
        return pItor->second;
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------
CTextureSlot* CFontTexture::GetLRUSlot()
{
    uint16 wMaxSlotAge = 0;
    CTextureSlot* pLRUSlot = 0;
    CTextureSlot* pSlot;

    CTextureSlotListItor pItor = m_pSlotList.begin();

    while (pItor != m_pSlotList.end())
    {
        pSlot = *pItor;

        if (pSlot->wSlotUsage == 0)
        {
            return pSlot;
        }
        else
        {
            uint16 slotAge = m_wSlotUsage - pSlot->wSlotUsage;
            if (slotAge > wMaxSlotAge)
            {
                pLRUSlot = pSlot;
                wMaxSlotAge = slotAge;
            }
        }

        ++pItor;
    }

    return pLRUSlot;
}

//-------------------------------------------------------------------------------------------------
CTextureSlot* CFontTexture::GetMRUSlot()
{
    uint16 wMinSlotAge = 0xFFFF;
    CTextureSlot* pMRUSlot = 0;
    CTextureSlot* pSlot;

    CTextureSlotListItor pItor = m_pSlotList.begin();

    while (pItor != m_pSlotList.end())
    {
        pSlot = *pItor;

        if (pSlot->wSlotUsage != 0)
        {
            uint16 slotAge = m_wSlotUsage - pSlot->wSlotUsage;
            if (slotAge > wMinSlotAge)
            {
                pMRUSlot = pSlot;
                wMinSlotAge = slotAge;
            }
        }

        ++pItor;
    }

    return pMRUSlot;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::PreCacheString(const char* szString, int* pUpdated, float sizeRatio, const Vec2i& glyphSize, const CFFont::FontHintParams& fontHintParams)
{
    Vec2i clampedGlyphSize = ClampGlyphSize(glyphSize, m_iCellWidth, m_iCellHeight);

    uint16 wSlotUsage = m_wSlotUsage++;
    int iUpdated = 0;

    uint32 cChar;
    for (Unicode::CIterator<const char*, false> it(szString); cChar = *it; ++it)
    {
        CTextureSlot* pSlot = GetCharSlot(cChar, clampedGlyphSize);

        if (!pSlot)
        {
            pSlot = GetLRUSlot();

            if (!pSlot)
            {
                return 0;
            }

            if (!UpdateSlot(pSlot->iTextureSlot, wSlotUsage, cChar, sizeRatio, clampedGlyphSize, fontHintParams))
            {
                return 0;
            }

            ++iUpdated;
        }
        else
        {
            pSlot->wSlotUsage = wSlotUsage;
        }
    }

    if (pUpdated)
    {
        *pUpdated = iUpdated;
    }

    if (iUpdated)
    {
        return 1;
    }

    return 2;
}

//-------------------------------------------------------------------------------------------------
void CFontTexture::GetTextureCoord(CTextureSlot* pSlot, float texCoords[4],
    int& iCharSizeX, int& iCharSizeY, int& iCharOffsetX, int& iCharOffsetY,
    const Vec2i& glyphSize) const
{
    if (!pSlot)
    {
        return;         // expected behavior
    }

    // Re-rendered glyphs are stored at smaller sizes than glyphs rendered at
    // the (maximum) font texture slot resolution. We scale the returned width
    // and height of the (actual) rendered glyph sizes so its transparent to 
    // callers that the glyph is actually smaller (from being re-rendered).
    const float requestSizeWidthScale = AZ::GetMin<float>(1.0f, GetRequestSizeWidthScale(glyphSize));
    const float requestSizeHeightScale = AZ::GetMin<float>(1.0f, GetRequestSizeHeightScale(glyphSize));
    const float invRequestSizeWidthScale = 1.0f / requestSizeWidthScale;
    const float invRequestSizeHeightScale = 1.0f / requestSizeHeightScale;

    // The inverse scale grows as the glyph size decreases. Once the glyph size
    // reaches the font texture's max slot dimensions, we cap width/height scale
    // since the text draw context will apply normal (as opposed to re-rendered)
    // scaling.
    int iChWidth = static_cast<int>(pSlot->iCharWidth * invRequestSizeWidthScale);
    int iChHeight = static_cast<int>(pSlot->iCharHeight * invRequestSizeHeightScale);

    float slotCoord0 = pSlot->vTexCoord[0];
    float slotCoord1 = pSlot->vTexCoord[1];
    texCoords[0] = slotCoord0 - m_fInvWidth;                                      // extra pixel for nicer bilinear filter
    texCoords[1] = slotCoord1 - m_fInvHeight;                                     // extra pixel for nicer bilinear filter
    
    // UV coordinates also must be scaled relative to the re-rendered glyph size
    // as well. Width scale must be capped at 1.0f since glyph can't grow 
    // beyond the slot's resolution.
    texCoords[2] = slotCoord0 + (((float)iChWidth * m_fInvWidth) * requestSizeWidthScale);
    texCoords[3] = slotCoord1 + (((float)iChHeight * m_fInvHeight) * requestSizeHeightScale);

    iCharSizeX = iChWidth + 1;                                        // extra pixel for nicer bilinear filter
    iCharSizeY = iChHeight + 1;                                   // extra pixel for nicer bilinear filter

    // Offsets are scaled accordingly when the rendered glyph size is smaller 
    // than the glyph/slot dimensions, but otherwise we expect the text draw 
    // context to apply scaling beyond that.
    iCharOffsetX = static_cast<int>(pSlot->iCharOffsetX * invRequestSizeWidthScale);
    iCharOffsetY = static_cast<int>(pSlot->iCharOffsetY * invRequestSizeHeightScale);
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::GetCharacterWidth(uint32 cChar) const
{
    CTextureSlotTableItorConst pItor = m_pSlotTable.find(GetTextureSlotKey(cChar));

    if (pItor == m_pSlotTable.end())
    {
        return 0;
    }

    const CTextureSlot& rSlot = *pItor->second;

    // For proportional fonts, add one pixel of spacing for aesthetic reasons
    int proportionalOffset = GetMonospaced() ? 0 : 1;

    return rSlot.iCharWidth + proportionalOffset;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::GetHorizontalAdvance(uint32 cChar, const Vec2i& glyphSize) const
{
    CTextureSlotTableItorConst pItor = m_pSlotTable.find(GetTextureSlotKey(cChar, glyphSize));

    if (pItor == m_pSlotTable.end())
    {
        return 0;
    }

    const CTextureSlot& rSlot = *pItor->second;

    // Re-rendered glyphs are stored at smaller sizes than glyphs rendered at
    // the (maximum) font texture slot resolution. We scale the returned width
    // and height of the (actual) rendered glyph sizes so its transparent to 
    // callers that the glyph is actually smaller (from being re-rendered).
    const float requestSizeWidthScale = GetRequestSizeWidthScale(glyphSize);
    const float invRequestSizeWidthScale = 1.0f / requestSizeWidthScale;

    // Only multiply by 1.0f when glyphsize is greater than cell width because we assume that callers
    // will use the font draw text context to scale the value appropriately.
    return static_cast<int>(rSlot.iHoriAdvance * AZ::GetMax<float>(1.0f, invRequestSizeWidthScale));
}

//-------------------------------------------------------------------------------------------------
/*
int CFontTexture::GetCharHeightByChar(wchar_t cChar)
{
CTextureSlotTableItor pItor = m_pSlotTable.find(cChar);

if (pItor != m_pSlotTable.end())
{
return pItor->second->iCharHeight;
}

return 0;
}
*/

//-------------------------------------------------------------------------------------------------
int CFontTexture::WriteToFile([[maybe_unused]] const string& szFileName)
{
#ifdef WIN32
    AZ::IO::FileIOStream outputFile(szFileName.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);

    if (!outputFile.IsOpen())
    {
        return 0;
    }

    BITMAPFILEHEADER pHeader;
    BITMAPINFOHEADER pInfoHeader;

    memset(&pHeader, 0, sizeof(BITMAPFILEHEADER));
    memset(&pInfoHeader, 0, sizeof(BITMAPINFOHEADER));

    pHeader.bfType = 0x4D42;
    pHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + m_iWidth * m_iHeight * 3;
    pHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    pInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    pInfoHeader.biWidth = m_iWidth;
    pInfoHeader.biHeight = m_iHeight;
    pInfoHeader.biPlanes = 1;
    pInfoHeader.biBitCount = 24;
    pInfoHeader.biCompression = 0;
    pInfoHeader.biSizeImage = m_iWidth * m_iHeight * 3;

    outputFile.Write(sizeof(BITMAPFILEHEADER), &pHeader);
    outputFile.Write(sizeof(BITMAPINFOHEADER), &pInfoHeader);

    unsigned char cRGB[3];

    for (int i = m_iHeight - 1; i >= 0; i--)
    {
        for (int j = 0; j < m_iWidth; j++)
        {
            cRGB[0] = m_pBuffer[(i * m_iWidth) + j];
            cRGB[1] = *cRGB;

            cRGB[2] = *cRGB;

            outputFile.Write(3, cRGB);
        }
    }

#endif
    return 1;
}

//-------------------------------------------------------------------------------------------------
Vec2 CFontTexture::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph)
{
    return m_pGlyphCache.GetKerning(leftGlyph, rightGlyph);
}

//-------------------------------------------------------------------------------------------------
float CFontTexture::GetAscenderToHeightRatio()
{
    return m_pGlyphCache.GetAscenderToHeightRatio();
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::CreateSlotList(int iListSize)
{
    int y, x;

    for (int i = 0; i < iListSize; i++)
    {
        CTextureSlot* pTextureSlot = new CTextureSlot;

        if (!pTextureSlot)
        {
            return 0;
        }

        pTextureSlot->iTextureSlot = i;
        pTextureSlot->Reset();

        y = i / m_iWidthCellCount;
        x = i % m_iWidthCellCount;

        pTextureSlot->vTexCoord[0] = (float)(x * m_fTextureCellWidth) + (0.5f / (float)m_iWidth);
        pTextureSlot->vTexCoord[1] = (float)(y * m_fTextureCellHeight) + (0.5f / (float)m_iHeight);

        m_pSlotList.push_back(pTextureSlot);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::ReleaseSlotList()
{
    CTextureSlotListItor pItor = m_pSlotList.begin();

    while (pItor != m_pSlotList.end())
    {
        delete (*pItor);

        pItor = m_pSlotList.erase(pItor);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int CFontTexture::UpdateSlot(int iSlot, uint16 wSlotUsage, uint32 cChar, float sizeRatio, const Vec2i& glyphSize, const CFFont::FontHintParams& fontHintParams)
{
    CTextureSlot* pSlot = m_pSlotList[iSlot];

    if (!pSlot)
    {
        return 0;
    }

    CTextureSlotTableItor pItor = m_pSlotTable.find(GetTextureSlotKey(pSlot->cCurrentChar, pSlot->glyphSize));

    if (pItor != m_pSlotTable.end())
    {
        m_pSlotTable.erase(pItor);
    }
    m_pSlotTable.insert(AZStd::pair<CryFont::FontTexture::CTextureSlotKey, CTextureSlot*>(GetTextureSlotKey(cChar, glyphSize), pSlot));
    pSlot->glyphSize = glyphSize;
    pSlot->wSlotUsage = wSlotUsage;
    pSlot->cCurrentChar = cChar;

    int iWidth = 0;
    int iHeight = 0;

    // blit the char glyph into the texture
    int x = pSlot->iTextureSlot % m_iWidthCellCount;
    int y = pSlot->iTextureSlot / m_iWidthCellCount;

    CGlyphBitmap* pGlyphBitmap;

    if (glyphSize.x > 0 && glyphSize.y > 0)
    {
        m_pGlyphCache.SetGlyphBitmapSize(glyphSize.x, glyphSize.y, sizeRatio);
    }

    if (!m_pGlyphCache.GetGlyph(&pGlyphBitmap, &pSlot->iHoriAdvance, &iWidth, &iHeight, pSlot->iCharOffsetX, pSlot->iCharOffsetY, cChar, glyphSize, fontHintParams))
    {
        return 0;
    }

    pSlot->iCharWidth = iWidth;
    pSlot->iCharHeight = iHeight;

    // Add a pixel along width and height to avoid artifacts being rendered
    // from a previous glyph in this slot due to bilinear filtering. The source
    // glyph bitmap buffer is presumed to be cleared prior to FreeType rendering
    // to the bitmap.
    const int blitWidth = AZ::GetMin<int>(iWidth + 1, m_iCellWidth);
    const int blitHeight = AZ::GetMin<int>(iHeight + 1, m_iCellHeight);

    pGlyphBitmap->BlitTo8(m_pBuffer, 0, 0,
        blitWidth, blitHeight, x * m_iCellWidth, y * m_iCellHeight, m_iWidth);

    return 1;
}

//-------------------------------------------------------------------------------------------------
void CFontTexture::CreateGradientSlot()
{
    CTextureSlot* pSlot = GetGradientSlot();
    assert(pSlot->cCurrentChar == (uint32)~0);      // 0 needs to be unused spot

    pSlot->Reset();
    pSlot->iCharWidth = m_iCellWidth - 2;
    pSlot->iCharHeight = m_iCellHeight - 2;
    pSlot->SetNotReusable();

    int x = pSlot->iTextureSlot % m_iWidthCellCount;
    int y = pSlot->iTextureSlot / m_iWidthCellCount;

    assert(sizeof(*m_pBuffer) == sizeof(uint8));
    uint8* pBuffer = &m_pBuffer[x * m_iCellWidth + y * m_iCellHeight * m_iWidth];

    for (uint32 dwY = 0; dwY < pSlot->iCharHeight; ++dwY)
    {
        for (uint32 dwX = 0; dwX < pSlot->iCharWidth; ++dwX)
        {
            pBuffer[dwX + dwY * m_iWidth] = dwY * 255 / (pSlot->iCharHeight - 1);
        }
    }
}

//-------------------------------------------------------------------------------------------------
CTextureSlot* CFontTexture::GetGradientSlot()
{
    return m_pSlotList[0];
}

//-------------------------------------------------------------------------------------------------
CryFont::FontTexture::CTextureSlotKey CFontTexture::GetTextureSlotKey(uint32 cChar, const Vec2i& glyphSize) const
{
    const Vec2i clampedGlyphSize(ClampGlyphSize(glyphSize, m_iCellWidth, m_iCellHeight));
    return CryFont::FontTexture::CTextureSlotKey(clampedGlyphSize, cChar);
}

Vec2i CFontTexture::ClampGlyphSize(const Vec2i& glyphSize, int cellWidth, int cellHeight)
{
    const Vec2i maxCellDimensions(cellWidth, cellHeight);
    Vec2i clampedGlyphSize(glyphSize);

    const bool hasZeroDimension = glyphSize.x == 0 || glyphSize.y == 0;
    const bool isDefaultSize = glyphSize == CCryFont::defaultGlyphSize;
    const bool exceedsDimensions = glyphSize.x > cellWidth || glyphSize.y > cellHeight;
    const bool useMaxCellDimension = hasZeroDimension || isDefaultSize || exceedsDimensions;
    if (useMaxCellDimension)
    {
        clampedGlyphSize = maxCellDimensions;
    }

    return clampedGlyphSize;
}

#endif // #if !defined(USE_NULLFONT_ALWAYS)
