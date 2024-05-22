/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Purpose:
//  - Create and update a texture with the most recently used glyphs


#if !defined(USE_NULLFONT_ALWAYS)

#include <AtomLyIntegration/AtomFont/FontTexture.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/string/conversions.h>

//-------------------------------------------------------------------------------------------------
AZ::FontTexture::FontTexture()
    : m_slotUsage(1)
    , m_width(0)
    , m_height(0)
    , m_invWidth(0.0f)
    , m_invHeight(0.0f)
    , m_cellWidth(0)
    , m_cellHeight(0)
    , m_textureCellWidth(0)
    , m_textureCellHeight(0)
    , m_widthCellCount(0)
    , m_heightCellCount(0)
    , m_textureSlotCount(0)
    , m_buffer(0)
    , m_smoothMethod(AZ::FontSmoothMethod::None)
    , m_smoothAmount(AZ::FontSmoothAmount::None)
{
}

//-------------------------------------------------------------------------------------------------
AZ::FontTexture::~FontTexture()
{
    Release();
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::CreateFromFile(const AZStd::string& fileName, int width, int height, AZ::FontSmoothMethod smoothMethod, AZ::FontSmoothAmount smoothAmount, int widthCellCount, int heightCellCount)
{
    if (!m_glyphCache.LoadFontFromFile(fileName))
    {
        Release();

        return 0;
    }

    if (!Create(width, height, smoothMethod, smoothAmount, widthCellCount, heightCellCount))
    {
        return 0;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::CreateFromMemory(unsigned char* fileData, int dataSize, int width, int height, AZ::FontSmoothMethod smoothMethod, AZ::FontSmoothAmount smoothAmount, int widthCellCount, int heightCellCount, float sizeRatio)
{
    if (!m_glyphCache.LoadFontFromMemory(fileData, dataSize))
    {
        Release();

        return 0;
    }

    if (!Create(width, height, smoothMethod, smoothAmount, widthCellCount, heightCellCount, sizeRatio))
    {
        return 0;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::Create(int width, int height, AZ::FontSmoothMethod smoothMethod, AZ::FontSmoothAmount smoothAmount, int widthCellCount, int heightCellCount, float sizeRatio)
{
    m_buffer = new FONT_TEXTURE_TYPE[width * height];
    if (!m_buffer)
    {
        return 0;
    }

    memset(m_buffer, 0, width * height * sizeof(FONT_TEXTURE_TYPE));

    if (!(widthCellCount * heightCellCount))
    {
        return 0;
    }

    m_width = width;
    m_height = height;
    m_invWidth = 1.0f / (float)width;
    m_invHeight = 1.0f / (float)height;

    m_widthCellCount = widthCellCount;
    m_heightCellCount = heightCellCount;
    m_textureSlotCount = m_widthCellCount * m_heightCellCount;

    m_smoothMethod = smoothMethod;
    m_smoothAmount = smoothAmount;

    m_cellWidth = m_width / m_widthCellCount;
    m_cellHeight = m_height / m_heightCellCount;

    m_textureCellWidth = m_cellWidth * m_invWidth;
    m_textureCellHeight = m_cellHeight * m_invHeight;

    if (!m_glyphCache.Create(AZ_FONT_GLYPH_CACHE_SIZE, m_cellWidth, m_cellHeight, smoothMethod, smoothAmount, sizeRatio))
    {
        Release();

        return 0;
    }

    if (!CreateSlotList(m_textureSlotCount))
    {
        Release();

        return 0;
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::Release()
{
    delete[] m_buffer;
    m_buffer = 0;

    ReleaseSlotList();

    m_slotIndexMap.clear();

    m_glyphCache.Release();

    m_widthCellCount = 0;
    m_heightCellCount = 0;
    m_textureSlotCount = 0;

    m_width = 0;
    m_height = 0;
    m_invWidth = 0.0f;
    m_invHeight = 0.0f;

    m_cellWidth = 0;
    m_cellHeight = 0;

    m_smoothMethod = AZ::FontSmoothMethod::None;
    m_smoothAmount = AZ::FontSmoothAmount::None;

    m_textureCellWidth = 0.0f;
    m_textureCellHeight = 0.0f;

    m_slotUsage = 1;

    return 1;
}

//-------------------------------------------------------------------------------------------------
uint32_t AZ::FontTexture::GetSlotChar(int slotIndex) const
{
    return m_slotList[slotIndex]->m_currentCharacter;
}

//-------------------------------------------------------------------------------------------------
AZ::TextureSlot* AZ::FontTexture::GetCharSlot(uint32_t character, const  AZ::AtomFont::GlyphSize& glyphSize)
{
    TextureSlotKey slotKey = GetTextureSlotKey(character, glyphSize);
    TextureSlotTableItor pItor = m_slotIndexMap.find(slotKey);

    if (pItor != m_slotIndexMap.end())
    {
        return pItor->second;
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------
AZ::TextureSlot* AZ::FontTexture::GetLRUSlot()
{
    uint16_t wMaxSlotAge = 0;
    TextureSlot* pLRUSlot = 0;
    TextureSlot* slot;

    TextureSlotListItor pItor = m_slotList.begin();

    while (pItor != m_slotList.end())
    {
        slot = *pItor;

        if (slot->m_slotUsage == 0)
        {
            return slot;
        }
        else
        {
            uint16_t slotAge = m_slotUsage - slot->m_slotUsage;
            if (slotAge > wMaxSlotAge)
            {
                pLRUSlot = slot;
                wMaxSlotAge = slotAge;
            }
        }

        ++pItor;
    }

    return pLRUSlot;
}

//-------------------------------------------------------------------------------------------------
AZ::TextureSlot* AZ::FontTexture::GetMRUSlot()
{
    uint16_t wMinSlotAge = 0xFFFF;
    TextureSlot* pMRUSlot = 0;
    TextureSlot* slot;

    TextureSlotListItor pItor = m_slotList.begin();

    while (pItor != m_slotList.end())
    {
        slot = *pItor;

        if (slot->m_slotUsage != 0)
        {
            uint16_t slotAge = m_slotUsage - slot->m_slotUsage;
            if (slotAge > wMinSlotAge)
            {
                pMRUSlot = slot;
                wMinSlotAge = slotAge;
            }
        }

        ++pItor;
    }

    return pMRUSlot;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::PreCacheString(const char* string, int* updated, float sizeRatio, const  AZ::AtomFont::GlyphSize& glyphSize, const FFont::FontHintParams& fontHintParams)
{
     AZ::AtomFont::GlyphSize clampedGlyphSize = ClampGlyphSize(glyphSize, m_cellWidth, m_cellHeight);

    uint16_t slotUsage = m_slotUsage++;
    int updateCount = 0;

    AZStd::wstring stringW;
    AZStd::to_wstring(stringW, string);
    for (wchar_t character : stringW)
    {
        TextureSlot* slot = GetCharSlot(character, clampedGlyphSize);

        if (!slot)
        {
            slot = GetLRUSlot();

            if (!slot)
            {
                return 0;
            }

            if (!UpdateSlot(slot->m_textureSlot, slotUsage, character, sizeRatio, clampedGlyphSize, fontHintParams))
            {
                return 0;
            }

            ++updateCount;
        }
        else
        {
            slot->m_slotUsage = slotUsage;
        }
    }

    if (updated)
    {
        *updated = updateCount;
    }

    if (updateCount)
    {
        return 1;
    }

    return 2;
}

//-------------------------------------------------------------------------------------------------
void AZ::FontTexture::GetTextureCoord(AZ::TextureSlot* slot, float texCoords[4],
    int& characterSizeX, int& characterSizeY, int& m_characterOffsetX, int& m_characterOffsetY,
    const AZ::AtomFont::GlyphSize& glyphSize) const
{
    if (!slot)
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
    int iChWidth = static_cast<int>(slot->m_characterWidth * invRequestSizeWidthScale);
    int iChHeight = static_cast<int>(slot->m_characterHeight * invRequestSizeHeightScale);

    float slotCoord0 = slot->m_texCoords[0];
    float slotCoord1 = slot->m_texCoords[1];
    texCoords[0] = slotCoord0 - m_invWidth;                                      // extra pixel for nicer bilinear filter
    texCoords[1] = slotCoord1 - m_invHeight;                                     // extra pixel for nicer bilinear filter
    
    // UV coordinates also must be scaled relative to the re-rendered glyph size
    // as well. Width scale must be capped at 1.0f since glyph can't grow 
    // beyond the slot's resolution.
    texCoords[2] = slotCoord0 + (((float)iChWidth * m_invWidth) * requestSizeWidthScale);
    texCoords[3] = slotCoord1 + (((float)iChHeight * m_invHeight) * requestSizeHeightScale);

    characterSizeX = iChWidth + 1;                                        // extra pixel for nicer bilinear filter
    characterSizeY = iChHeight + 1;                                   // extra pixel for nicer bilinear filter

    // Offsets are scaled accordingly when the rendered glyph size is smaller 
    // than the glyph/slot dimensions, but otherwise we expect the text draw 
    // context to apply scaling beyond that.
    m_characterOffsetX = static_cast<int>(slot->m_characterOffsetX * invRequestSizeWidthScale);
    m_characterOffsetY = static_cast<int>(slot->m_characterOffsetY * invRequestSizeHeightScale);
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::GetCharacterWidth(uint32_t character) const
{
    TextureSlotTableItorConst pItor = m_slotIndexMap.find(GetTextureSlotKey(character));

    if (pItor == m_slotIndexMap.end())
    {
        return 0;
    }

    const TextureSlot& rSlot = *pItor->second;

    // For proportional fonts, add one pixel of spacing for aesthetic reasons
    int proportionalOffset = GetMonospaced() ? 0 : 1;

    return rSlot.m_characterWidth + proportionalOffset;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::GetHorizontalAdvance(uint32_t character, const AZ::AtomFont::GlyphSize& glyphSize) const
{
    TextureSlotTableItorConst pItor = m_slotIndexMap.find(GetTextureSlotKey(character, glyphSize));

    if (pItor == m_slotIndexMap.end())
    {
        return 0;
    }

    const TextureSlot& rSlot = *pItor->second;

    // Re-rendered glyphs are stored at smaller sizes than glyphs rendered at
    // the (maximum) font texture slot resolution. We scale the returned width
    // and height of the (actual) rendered glyph sizes so its transparent to 
    // callers that the glyph is actually smaller (from being re-rendered).
    const float requestSizeWidthScale = GetRequestSizeWidthScale(glyphSize);
    const float invRequestSizeWidthScale = 1.0f / requestSizeWidthScale;

    // Only multiply by 1.0f when glyphsize is greater than cell width because we assume that callers
    // will use the font draw text context to scale the value appropriately.
    return static_cast<int>(rSlot.m_horizontalAdvance * AZ::GetMax<float>(1.0f, invRequestSizeWidthScale));
}

//-------------------------------------------------------------------------------------------------
/*
int AZ::FontTexture::GetCharHeightByChar(wchar_t character)
{
TextureSlotTableItor pItor = m_slotIndexMap.find(character);

if (pItor != m_slotIndexMap.end())
{
return pItor->second->m_characterHeight;
}

return 0;
}
*/

//-------------------------------------------------------------------------------------------------
Vec2 AZ::FontTexture::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph)
{
    return m_glyphCache.GetKerning(leftGlyph, rightGlyph);
}

//-------------------------------------------------------------------------------------------------
float AZ::FontTexture::GetAscenderToHeightRatio()
{
    return m_glyphCache.GetAscenderToHeightRatio();
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::CreateSlotList(int listSize)
{
    int y, x;

    for (int i = 0; i < listSize; i++)
    {
        TextureSlot* pTextureSlot = new TextureSlot;

        if (!pTextureSlot)
        {
            return 0;
        }

        pTextureSlot->m_textureSlot = i;
        pTextureSlot->Reset();

        y = i / m_widthCellCount;
        x = i % m_widthCellCount;

        pTextureSlot->m_texCoords[0] = (float)(x * m_textureCellWidth) + (0.5f / (float)m_width);
        pTextureSlot->m_texCoords[1] = (float)(y * m_textureCellHeight) + (0.5f / (float)m_height);

        m_slotList.push_back(pTextureSlot);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::ReleaseSlotList()
{
    TextureSlotListItor pItor = m_slotList.begin();

    while (pItor != m_slotList.end())
    {
        delete (*pItor);

        pItor = m_slotList.erase(pItor);
    }

    return 1;
}

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::UpdateSlot(int slotIndex, uint16_t slotUsage, uint32_t character, float sizeRatio, const AZ::AtomFont::GlyphSize& glyphSize, const FFont::FontHintParams& fontHintParams)
{
    TextureSlot* slot = m_slotList[slotIndex];

    if (!slot)
    {
        return 0;
    }

    TextureSlotTableItor pItor = m_slotIndexMap.find(GetTextureSlotKey(slot->m_currentCharacter, slot->m_glyphSize));

    if (pItor != m_slotIndexMap.end())
    {
        m_slotIndexMap.erase(pItor);
    }
    m_slotIndexMap.insert(TextureSlotTableEntry(GetTextureSlotKey(character, glyphSize), slot));
    slot->m_glyphSize = glyphSize;
    slot->m_slotUsage = slotUsage;
    slot->m_currentCharacter = character;

    int width = 0;
    int height = 0;

    // blit the char glyph into the texture
    int x = slot->m_textureSlot % m_widthCellCount;
    int y = slot->m_textureSlot / m_widthCellCount;

    GlyphBitmap* glyphBitmap;

    if (glyphSize.x > 0 && glyphSize.y > 0)
    {
        m_glyphCache.SetGlyphBitmapSize(glyphSize.x, glyphSize.y, sizeRatio);
    }

    if (!m_glyphCache.GetGlyph(&glyphBitmap, &slot->m_horizontalAdvance, &width, &height, slot->m_characterOffsetX, slot->m_characterOffsetY, character, glyphSize, fontHintParams))
    {
        return 0;
    }

    slot->m_characterWidth = static_cast<uint8_t>(width);
    slot->m_characterHeight = static_cast<uint8_t>(height);

    // Add a pixel along width and height to avoid artifacts being rendered
    // from a previous glyph in this slot due to bilinear filtering. The source
    // glyph bitmap buffer is presumed to be cleared prior to FreeType rendering
    // to the bitmap.
    const int blitWidth = AZ::GetMin<int>(width + 1, m_cellWidth);
    const int blitHeight = AZ::GetMin<int>(height + 1, m_cellHeight);

    glyphBitmap->BlitTo8(m_buffer, 0, 0,
        blitWidth, blitHeight, x * m_cellWidth, y * m_cellHeight, m_width);

    return 1;
}

//-------------------------------------------------------------------------------------------------
void AZ::FontTexture::CreateGradientSlot()
{
    TextureSlot* slot = GetGradientSlot();
    assert(slot->m_currentCharacter == (uint32_t)~0);      // 0 needs to be unused spot

    slot->Reset();
    slot->m_characterWidth = static_cast<uint8_t>(m_cellWidth - 2);
    slot->m_characterHeight = static_cast<uint8_t>(m_cellHeight - 2);
    slot->SetNotReusable();

    int x = slot->m_textureSlot % m_widthCellCount;
    int y = slot->m_textureSlot / m_widthCellCount;

    assert(sizeof(*m_buffer) == sizeof(uint8_t));
    uint8_t* buffer = &m_buffer[x * m_cellWidth + y * m_cellHeight * m_width];

    for (uint32_t dwY = 0; dwY < slot->m_characterHeight; ++dwY)
    {
        for (uint32_t dwX = 0; dwX < slot->m_characterWidth; ++dwX)
        {
            buffer[dwX + dwY * m_width] = static_cast<uint8_t>(dwY * 255 / (slot->m_characterHeight - 1));
        }
    }
}

//-------------------------------------------------------------------------------------------------
AZ::TextureSlot* AZ::FontTexture::GetGradientSlot()
{
    return m_slotList[0];
}

//-------------------------------------------------------------------------------------------------
AZ::FontTexture::TextureSlotKey AZ::FontTexture::GetTextureSlotKey(uint32_t character, const AZ::AtomFont::GlyphSize& glyphSize) const
{
    const AZ::AtomFont::GlyphSize clampedGlyphSize(ClampGlyphSize(glyphSize, m_cellWidth, m_cellHeight));
    return AZ::FontTexture::TextureSlotKey(clampedGlyphSize, character);
}

AZ::AtomFont::GlyphSize AZ::FontTexture::ClampGlyphSize(const AZ::AtomFont::GlyphSize& glyphSize, int cellWidth, int cellHeight)
{
    const AZ::AtomFont::GlyphSize maxCellDimensions(cellWidth, cellHeight);
    AZ::AtomFont::GlyphSize clampedGlyphSize(glyphSize);

    const bool hasZeroDimension = glyphSize.x == 0 || glyphSize.y == 0;
    const bool isDefaultSize = glyphSize == AZ::AtomFont::defaultGlyphSize;
    const bool exceedsDimensions = glyphSize.x > cellWidth || glyphSize.y > cellHeight;
    const bool useMaxCellDimension = hasZeroDimension || isDefaultSize || exceedsDimensions;
    if (useMaxCellDimension)
    {
        clampedGlyphSize = maxCellDimensions;
    }

    return clampedGlyphSize;
}

#endif // #if !defined(USE_NULLFONT_ALWAYS)
