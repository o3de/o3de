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

// Description : Font class.


#include "CryFont_precompiled.h"

#if !defined(USE_NULLFONT_ALWAYS)

#include "FFont.h"
#include "CryFont.h"
#include "FontTexture.h"
#include "UnicodeIterator.h"

#include <AzCore/std/parallel/lock.h>
#include <AzFramework/Archive/Archive.h>

static ColorB ColorTable[10] =
{
    ColorB(0x00, 0x00, 0x00), // black
    ColorB(0xff, 0xff, 0xff), // white
    ColorB(0x00, 0x00, 0xff), // blue
    ColorB(0x00, 0xff, 0x00), // green
    ColorB(0xff, 0x00, 0x00), // red
    ColorB(0x00, 0xff, 0xff), // cyan
    ColorB(0xff, 0xff, 0x00), // yellow
    ColorB(0xff, 0x00, 0xff), // purple
    ColorB(0xff, 0x80, 0x00), // orange
    ColorB(0x8f, 0x8f, 0x8f), // grey
};

static const int TabCharCount = 4;
static const size_t MsgBufferSize = 1024;
static const size_t MaxDrawVBQuads = 128;

CFFont::CFFont(ISystem* pSystem, CCryFont* pCryFont, const char* pFontName)
    : m_name(pFontName)
    , m_curPath("")
    , m_pFontTexture(0)
    , m_fontBufferSize(0)
    , m_pFontBuffer(0)
    , m_texID(-1)
    , m_textureVersion(0)
    , m_pSystem(pSystem)
    , m_pCryFont(pCryFont)
    , m_fontTexDirty(false)
    , m_effects()
    , m_pDrawVB(0)
    , m_nRefCount(0)
    , m_monospacedFont(false)
{
    assert(m_name.c_str());
    assert(m_pSystem);
    assert(m_pCryFont);

    // create default effect
    SEffect* pEffect = AddEffect("default");
    pEffect->AddPass();

    m_pDrawVB = new SVF_P3F_C4B_T2F[MaxDrawVBQuads * 6];

    AddRef();
}

CFFont::~CFFont()
{
    // The font should already be unregistered through a call to
    // CFFont::Release() at this point.
    CRY_ASSERT(m_pCryFont == nullptr);

    Free();

    SAFE_DELETE_ARRAY(m_pDrawVB);
}

int32 CFFont::AddRef()
{
    int32 nRef = CryInterlockedIncrement(&m_nRefCount);
    return nRef;
}

int32 CFFont::Release()
{
    if (m_nRefCount > 0)
    {
        int32 nRef = CryInterlockedDecrement(&m_nRefCount);
        if (nRef < 0)
        {
            CryFatalError("CBaseResource::Release() called more than once!");
        }

        if (nRef <= 0)
        {
            if (m_pCryFont)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> locker(m_fontMutex);

                // Unregister font so no one can increase it's ref count while it is queued for deletion
                m_pCryFont->UnregisterFont(m_name);
                m_pCryFont = nullptr;
            }

            if (gEnv->pRenderer)
            {
                gEnv->pRenderer->DeleteFont(this);
            }
            return 0;
        }
        return nRef;
    }
    return 0;
}

// Load a font from a TTF file
bool CFFont::Load(const char* pFontFilePath, unsigned int width, unsigned int height, unsigned int widthNumSlots, unsigned int heightNumSlots, unsigned int flags, float sizeRatio)
{
    if (!pFontFilePath)
    {
        return false;
    }

    Free();

    auto pPak = gEnv->pCryPak;

    string fullFile;
    if (pPak->IsAbsPath(pFontFilePath))
    {
        fullFile = pFontFilePath;
    }
    else
    {
        fullFile = m_curPath + pFontFilePath;
    }

    int iSmoothMethod = (flags & TTFFLAG_SMOOTH_MASK) >> TTFFLAG_SMOOTH_SHIFT;
    int iSmoothAmount = (flags & TTFFLAG_SMOOTH_AMOUNT_MASK) >> TTFFLAG_SMOOTH_AMOUNT_SHIFT;

    AZ::IO::HandleType fileHandle = pPak->FOpen(fullFile.c_str(), "rb");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return false;
    }

    size_t fileSize = pPak->FGetSize(fileHandle);
    if (!fileSize)
    {
        pPak->FClose(fileHandle);
        return false;
    }

    unsigned char* pBuffer = new unsigned char[fileSize];
    if (!pPak->FReadRaw(pBuffer, fileSize, 1, fileHandle))
    {
        pPak->FClose(fileHandle);
        delete [] pBuffer;
        return false;
    }

    pPak->FClose(fileHandle);

    if (!m_pFontTexture)
    {
        m_pFontTexture = new CFontTexture();
    }

    if (!m_pFontTexture || !m_pFontTexture->CreateFromMemory(pBuffer, (int)fileSize, width, height, iSmoothMethod, iSmoothAmount, widthNumSlots, heightNumSlots, sizeRatio))
    {
        delete [] pBuffer;
        return false;
    }

    m_monospacedFont = m_pFontTexture->GetMonospaced();
    m_pFontBuffer = pBuffer;
    m_fontBufferSize = fileSize;
    m_fontTexDirty = false;
    m_sizeRatio = sizeRatio;

    InitCache();

    return true;
}


void CFFont::Free()
{
    IRenderer* pRenderer = gEnv->pRenderer;
    if (m_texID >= 0 && pRenderer)
    {
        pRenderer->RemoveTexture(m_texID);
        m_texID = -1;
        m_textureVersion = 0;
    }

    delete m_pFontTexture;
    m_pFontTexture = 0;

    delete [] m_pFontBuffer;
    m_pFontBuffer = 0;
}

void CFFont::DrawString(float x, float y, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
    IF (!pStr, 0)
    {
        return;
    }

    DrawStringUInternal(x, y, 1.0f, pStr, asciiMultiLine, ctx);
}

void CFFont::DrawString(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
    IF (!pStr, 0)
    {
        return;
    }

    DrawStringUInternal(x, y, z, pStr, asciiMultiLine, ctx);
}

void CFFont::DrawStringUInternal(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
    IF (!pStr || !m_pFontTexture || ctx.m_fxIdx >= m_effects.size() || m_effects[ctx.m_fxIdx].m_passes.empty(), 0)
    {
        return;
    }

    AZStd::lock_guard<AZStd::recursive_mutex> locker(m_fontMutex);

    IRenderer* pRenderer = gEnv->pRenderer;
    AZ_Assert(pRenderer, "gEnv->pRenderer is NULL");

    pRenderer->DrawStringU(this, x, y, z, pStr, asciiMultiLine, ctx);
}

ILINE uint32 COLCONV(uint32 clr)
{
    return ((clr & 0xff00ff00) | ((clr & 0xff0000) >> 16) | ((clr & 0xff) << 16));
}

void CFFont::RenderCallback(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
    AZStd::lock_guard<AZStd::recursive_mutex> locker(m_fontMutex);

    const size_t fxSize = m_effects.size();

    IF (fxSize && m_texID == -1 && !InitTexture(), 0)
    {
        return;
    }

    // if the font is about to be deleted then m_pCryFont can be nullptr
    if (!m_pCryFont)
    {
        return;
    }

    SVF_P3F_C4B_T2F* pVertex = m_pDrawVB;
    size_t vbOffset = 0;

    bool isFontRenderStateSet = false;
    TransformationMatrices backupSceneMatrices;
    IRenderer* pRenderer = gEnv->pRenderer;
    AZ_Assert(pRenderer, "gEnv->pRenderer is NULL");

    int baseState = ctx.m_baseState;
    bool overrideViewProjMatrices = ctx.m_overrideViewProjMatrices;
    int texId = m_texID;

    // Local function to share code needed when we render the vertex buffer so far
    AZStd::function<void(void)> RenderVB = [&pVertex, &vbOffset, pRenderer]
    {
        gEnv->pRenderer->DrawDynVB(pVertex, 0, vbOffset, 0, prtTriangleList);
        vbOffset = 0;
    };

    // Local function that is passed into CreateQuadsForText as the AddQuad function
    AddFunction AddQuad = [&pVertex, &vbOffset, RenderVB]
        (const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec2& tc0, const Vec2& tc1, const Vec2& tc2, const Vec2& tc3, uint32 packedColor)
    {
        // define char quad
        pVertex[vbOffset].xyz = v0;
        pVertex[vbOffset].color.dcolor = packedColor;
        pVertex[vbOffset].st = tc0;

        pVertex[vbOffset + 1].xyz = v1;
        pVertex[vbOffset + 1].color.dcolor = packedColor;
        pVertex[vbOffset + 1].st = tc1;

        pVertex[vbOffset + 2].xyz = v2;
        pVertex[vbOffset + 2].color.dcolor = packedColor;
        pVertex[vbOffset + 2].st = tc2;

        pVertex[vbOffset + 3].xyz = v2;
        pVertex[vbOffset + 3].color.dcolor = packedColor;
        pVertex[vbOffset + 3].st = tc2;

        pVertex[vbOffset + 4].xyz = v3;
        pVertex[vbOffset + 4].color.dcolor = packedColor;
        pVertex[vbOffset + 4].st = tc3;

        pVertex[vbOffset + 5].xyz = v0;
        pVertex[vbOffset + 5].color.dcolor = packedColor;
        pVertex[vbOffset + 5].st = tc0;

        vbOffset += 6;

        if (vbOffset >= MaxDrawVBQuads * 6)
        {
            RenderVB();
        }
    };

    // Local function that is passed into CreateQuadsForText as the BeginPass function
    BeginPassFunction BeginPass = [&pVertex, &vbOffset, &isFontRenderStateSet, &backupSceneMatrices, texId, pRenderer, baseState, RenderVB, overrideViewProjMatrices]
        (const SRenderingPass* pPass)
    {
        // We don't want to set this state before the call to CreateQuadsForText since that calls Prepare,
        // which is needed before FontSetTexture
        if (!isFontRenderStateSet)
        {
            pRenderer->FontSetTexture(texId, FILTER_TRILINEAR);
            pRenderer->FontSetRenderingState(overrideViewProjMatrices, backupSceneMatrices);
            gEnv->pRenderer->FontSetBlending(pPass->m_blendSrc, pPass->m_blendDest, baseState);
            isFontRenderStateSet = true;
        }
        
        if (vbOffset > 0)
        {
            RenderVB();
        }

        gEnv->pRenderer->FontSetBlending(pPass->m_blendSrc, pPass->m_blendDest, baseState);
    };

    CreateQuadsForText(x, y, z, pStr, asciiMultiLine, ctx, AddQuad, BeginPass);

    if (vbOffset > 0)
    {
        RenderVB();
    }

    // restore the old states
    if (isFontRenderStateSet)
    {
        pRenderer->FontRestoreRenderingState(overrideViewProjMatrices, backupSceneMatrices);
    }
}

Vec2 CFFont::GetTextSize(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
    IF (!pStr, 0)
    {
        return Vec2(0.0f, 0.0f);
    }

    return GetTextSizeUInternal(pStr, asciiMultiLine, ctx);
}

Vec2 CFFont::GetTextSizeUInternal(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
    const size_t fxSize = m_effects.size();

    IF (!pStr || !m_pFontTexture || !fxSize, 0)
    {
        return Vec2(0, 0);
    }

    AZStd::lock_guard<AZStd::recursive_mutex> locker(m_fontMutex);

    Prepare(pStr, false, ctx.m_requestSize);

    IRenderer* pRenderer = gEnv->pRenderer;
    AZ_Assert(pRenderer, "gEnv->pRenderer is NULL");

    // This is the "logical" size of the font (in pixels). The actual size of
    // the glyphs in the font texture may have additional scaling applied or
    // could have been re-rendered at a different size.
    Vec2 size = ctx.m_size;
    if (ctx.m_sizeIn800x600)
    {
        pRenderer->ScaleCoord(size.x, size.y);
    }

    // This scaling takes into account the logical size of the font relative
    // to any additional scaling applied (such as from "size ratio").
    const TextScaleInfoInternal scaleInfo(CalculateScaleInternal(ctx));

    float maxW = 0;
    float maxH = 0;

    const size_t fxIdx = ctx.m_fxIdx < fxSize ? ctx.m_fxIdx : 0;
    const SEffect& fx = m_effects[fxIdx];

    for (size_t i = 0, numPasses = fx.m_passes.size(); i < numPasses; ++i)
    {
        const SRenderingPass* pPass = &fx.m_passes[numPasses - i - 1];

        // gather pass data
        Vec2 offset = pPass->m_posOffset;

        float charX = offset.x;
        float charY = offset.y + size.y;

        if (charY > maxH)
        {
            maxH = charY;
        }

        // parse the string, ignoring control characters
        uint32_t nextCh = 0;
        Unicode::CIterator<const char*, false> pChar(pStr);
        while (uint32_t ch = *pChar)
        {
            ++pChar;
            nextCh = *pChar;
            switch (ch)
            {
            case '\\':
            {
                if (*pChar != 'n' || !asciiMultiLine)
                {
                    break;
                }

                ++pChar;
            }
            case '\n':
            {
                if (charX > maxW)
                {
                    maxW = charX;
                }

                charX = offset.x;
                charY += size.y;

                if (charY > maxH)
                {
                    maxH = charY;
                }

                continue;
            }
            break;
            case '\r':
            {
                if (charX > maxW)
                {
                    maxW = charX;
                }

                charX = offset.x;
                continue;
            }
            break;
            case '\t':
            {
                if (ctx.m_proportional)
                {
                    charX += TabCharCount * size.x * FONT_SPACE_SIZE;
                }
                else
                {
                    charX += TabCharCount * size.x * ctx.m_widthScale;
                }
                continue;
            }
            break;
            case '$':
            {
                if (ctx.m_processSpecialChars)
                {
                    if (*pChar == '$')
                    {
                        ++pChar;
                    }
                    else if (isdigit(*pChar))
                    {
                        ++pChar;
                        continue;
                    }
                    else if (*pChar == 'O' || *pChar == 'o')
                    {
                        ++pChar;
                        continue;
                    }
                }
            }
            break;
            default:
                break;
            }

            const bool rerenderGlyphs = m_sizeBehavior == SizeBehavior::Rerender;
            const Vec2i requestSize = rerenderGlyphs ? ctx.m_requestSize : CCryFont::defaultGlyphSize;
            int horizontalAdvance = m_pFontTexture->GetHorizontalAdvance(ch, requestSize);
            float advance;

            if (ctx.m_proportional)
            {
                advance = horizontalAdvance * scaleInfo.scale.x;
            }
            else
            {
                advance = size.x * ctx.m_widthScale;
            }

            // Adjust "advance" here for kerning purposes
            Vec2 kerningOffset(Vec2_Zero);
            if (ctx.m_kerningEnabled && nextCh)
            {
                kerningOffset = m_pFontTexture->GetKerning(ch, nextCh) * scaleInfo.scale.x;
            }

            // Adjust char width with tracking only if there is a next character
            if (nextCh)
            {
                charX += ctx.m_tracking;
            }

            charX += advance + kerningOffset.x;
        }

        if (charX > maxW)
        {
            maxW = charX;
        }
    }

    return Vec2(maxW, maxH);
}

uint32 CFFont::GetNumQuadsForText(const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
    uint32 numQuads = 0;

    const size_t fxSize = m_effects.size();
    const size_t fxIdx = ctx.m_fxIdx < fxSize ? ctx.m_fxIdx : 0;
    const SEffect& fx = m_effects[fxIdx];

    for (size_t j = 0, numPasses = fx.m_passes.size(); j < numPasses; ++j)
    {
        size_t i = numPasses - j - 1;
        bool drawFrame = ctx.m_framed && i == numPasses - 1;

        if (drawFrame)
        {
            ++numQuads;
        }
            
        uint32_t nextCh = 0;
        Unicode::CIterator<const char*, false> pChar(pStr);
        while (uint32_t ch = *pChar)
        {
            ++pChar;
            nextCh = *pChar;

            switch (ch)
            {
            case '\\':
            {
                if (*pChar != 'n' || !asciiMultiLine)
                {
                    break;
                }
                ++pChar;
            }
            case '\n':
            {
                continue;
            }
            break;
            case '\r':
            {
                continue;
            }
            break;
            case '\t':
            {
                continue;
            }
            break;
            case '$':
            {
                if (ctx.m_processSpecialChars)
                {
                    if (*pChar == '$')
                    {
                        ++pChar;
                    }
                    else if (isdigit(*pChar))
                    {
                        ++pChar;
                        continue;
                    }
                    else if (*pChar == 'O' || *pChar == 'o')
                    {
                        ++pChar;
                        continue;
                    }
                }
            }
            break;
            default:
                break;
            }
        
            ++numQuads;
        }
    }

    return numQuads;
}

uint32 CFFont::WriteTextQuadsToBuffers(SVF_P2F_C4B_T2F_F4B* verts, uint16* indices, uint32 maxQuads, float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
    AZStd::lock_guard<AZStd::recursive_mutex> locker(m_fontMutex);

    uint32 numQuadsWritten = 0;

    const size_t fxSize = m_effects.size();
    IF (fxSize && m_texID == -1 && !InitTexture(), 0)
    {
        return numQuadsWritten;
    }

    // if the font is about to be deleted then m_pCryFont can be nullptr
    if (!m_pCryFont)
    {
        return numQuadsWritten;
    }

    SVF_P2F_C4B_T2F_F4B* pVertex = verts;
    uint16* pIndex = indices;
    size_t vbOffset = 0;
    size_t ibOffset = 0;

    // Local function that is passed into CreateQuadsForText as the AddQuad function
    AddFunction AddQuad = [&pVertex, &pIndex, &vbOffset, &ibOffset, maxQuads, &numQuadsWritten]
        (const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec2& tc0, const Vec2& tc1, const Vec2& tc2, const Vec2& tc3, uint32 packedColor)
    {
        Vec2 xy0(v0);
        Vec2 xy1(v1);
        Vec2 xy2(v2);
        Vec2 xy3(v3);

        AZ_Assert(vbOffset + 3 < maxQuads * 4, "Vertex buffer overflow");
        AZ_Assert(ibOffset + 5 < maxQuads * 6, "Index buffer overflow");

        // This should never happen but for safety make sure we never write off end of buffers (should hit asserts above if this is the case)
        if (numQuadsWritten < maxQuads)
        {
            // define char quad
            pVertex[vbOffset].xy = xy0;
            pVertex[vbOffset].color.dcolor = packedColor;
            pVertex[vbOffset].st = tc0;
            pVertex[vbOffset].texIndex = 0;
            pVertex[vbOffset].texHasColorChannel = 0;
            pVertex[vbOffset].texIndex2 = 0;
            pVertex[vbOffset].pad = 0;

            pVertex[vbOffset + 1].xy = xy1;
            pVertex[vbOffset + 1].color.dcolor = packedColor;
            pVertex[vbOffset + 1].st = tc1;
            pVertex[vbOffset + 1].texIndex = 0;
            pVertex[vbOffset + 1].texHasColorChannel = 0;
            pVertex[vbOffset + 1].texIndex2 = 0;
            pVertex[vbOffset + 1].pad = 0;

            pVertex[vbOffset + 2].xy = xy2;
            pVertex[vbOffset + 2].color.dcolor = packedColor;
            pVertex[vbOffset + 2].st = tc2;
            pVertex[vbOffset + 2].texIndex = 0;
            pVertex[vbOffset + 2].texHasColorChannel = 0;
            pVertex[vbOffset + 2].texIndex2 = 0;
            pVertex[vbOffset + 2].pad = 0;

            pVertex[vbOffset + 3].xy = xy3;
            pVertex[vbOffset + 3].color.dcolor = packedColor;
            pVertex[vbOffset + 3].st = tc3;
            pVertex[vbOffset + 3].texIndex = 0;
            pVertex[vbOffset + 3].texHasColorChannel = 0;
            pVertex[vbOffset + 3].texIndex2 = 0;
            pVertex[vbOffset + 3].pad = 0;

            pIndex[ibOffset] = vbOffset;
            pIndex[ibOffset + 1] = vbOffset + 1;
            pIndex[ibOffset + 2] = vbOffset + 2;
            pIndex[ibOffset + 3] = vbOffset + 2;
            pIndex[ibOffset + 4] = vbOffset + 3;
            pIndex[ibOffset + 5] = vbOffset + 0;

            vbOffset += 4;
            ibOffset += 6;

            ++numQuadsWritten;
        }
    };

    // Local function that is passed into CreateQuadsForText as the BeginPass function
    BeginPassFunction BeginPass = []
        (const SRenderingPass* /* pPass */)
    {
    };


    CreateQuadsForText(x, y, z, pStr, asciiMultiLine, ctx, AddQuad, BeginPass);

    return numQuadsWritten;
}

int CFFont::GetFontTextureId()
{
    if (m_texID == -1 && !InitTexture())
    {
        return -1;
    }

    return m_texID;
}

uint32 CFFont::GetFontTextureVersion()
{
    return m_textureVersion;
}

void CFFont::CreateQuadsForText(float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx,
    AddFunction AddQuad, BeginPassFunction BeginPass)
{
    const size_t fxSize = m_effects.size();

    Prepare(pStr, true, ctx.m_requestSize);

    const size_t fxIdx = ctx.m_fxIdx < fxSize ? ctx.m_fxIdx : 0;
    const SEffect& fx = m_effects[fxIdx];

    bool isRGB = m_pCryFont->RndPropIsRGBA();

    float halfTexelShift = m_pCryFont->RndPropHalfTexelOffset();

    bool passZeroColorOverridden = ctx.IsColorOverridden();

    uint32 alphaBlend = passZeroColorOverridden ? ctx.m_colorOverride.a : fx.m_passes[0].m_color.a;
    if (alphaBlend > 128)
    {
        ++alphaBlend; // 0..256 for proper blending
    }
    IRenderer* pRenderer = gEnv->pRenderer;
    AZ_Assert(pRenderer, "gEnv->pRenderer is NULL");

    // This is the "logical" size of the font (in pixels). The actual size of
    // the glyphs in the font texture may have additional scaling applied or
    // could have been re-rendered at a different size.
    Vec2 size = ctx.m_size;
    if (ctx.m_sizeIn800x600)
    {
        pRenderer->ScaleCoord(size.x, size.y);
    }

    // This scaling takes into account the logical size of the font relative
    // to any additional scaling applied (such as from "size ratio").
    const TextScaleInfoInternal scaleInfo(CalculateScaleInternal(ctx));
    
    Vec2 baseXY = Vec2(x, y); // in pixels
    if (ctx.m_sizeIn800x600)
    {
        pRenderer->ScaleCoord(baseXY.x, baseXY.y);
    }

    // Apply overscan border
    const int flags = ctx.GetFlags();
    if ((flags & eDrawText_2D) && !(flags & eDrawText_IgnoreOverscan))
    {
        Vec2 overscanBorder = Vec2(0.0f, 0.0f);
        pRenderer->EF_Query(EFQ_OverscanBorders, overscanBorder);

        const float screenWidth = (float)pRenderer->GetOverlayWidth();
        const float screenHeight = (float)pRenderer->GetOverlayHeight();
        Vec2 overscanBorderOffset = ZERO;
        if (!(flags & eDrawText_Center))
        {
            overscanBorderOffset.x = (overscanBorder.x * screenWidth);
        }
        if (!(flags & eDrawText_CenterV))
        {
            overscanBorderOffset.y = (overscanBorder.y * screenHeight);
        }
        if (flags & eDrawText_Right)
        {
            overscanBorderOffset.x = -overscanBorderOffset.x;
        }
        if (flags & eDrawText_Bottom)
        {
            overscanBorderOffset.y = -overscanBorderOffset.y;
        }
        baseXY += overscanBorderOffset;
    }

    // snap for pixel perfect rendering (better quality for text)
    if (ctx.m_pixelAligned)
    {
        baseXY.x = floor(baseXY.x);
        baseXY.y = floor(baseXY.y);

        // for smaller fonts (half res or less) it's better to average multiple pixels (we don't miss lines)
        if (scaleInfo.scale.x < 0.9f)
        {
            baseXY.x += 0.5f; // try to average two columns (for exact half res)
        }
        if (scaleInfo.scale.y < 0.9f)
        {
            baseXY.y += 0.25f; // hand tweaked value to get a good result with tiny font (640x480 underscore in console)
        }
    }

    for (size_t j = 0, numPasses = fx.m_passes.size(); j < numPasses; ++j)
    {
        size_t i = numPasses - j - 1;

        const SRenderingPass* pPass = &fx.m_passes[i];

        if (!i)
        {
            alphaBlend = 256;
        }

        ColorB passColor = !i && passZeroColorOverridden ? ctx.m_colorOverride : fx.m_passes[i].m_color;

        // gather pass data
        Vec2 offset = pPass->m_posOffset; // in pixels

        float charX = baseXY.x + offset.x; // in pixels
        float charY = baseXY.y + offset.y; // in pixels

        ColorB color = passColor;

        bool drawFrame = ctx.m_framed && i == numPasses - 1;

        BeginPass(pPass);

        if (drawFrame)
        {
            ColorB tempColor(255, 255, 255, 255);
            uint32 frameColor = tempColor.pack_abgr8888();
            if (!isRGB)
            {
                frameColor = COLCONV(frameColor);
            }

            Vec2 textSize = GetTextSizeUInternal(pStr, asciiMultiLine, ctx);

            float x0 = baseXY.x - 12;
            float y0 = baseXY.y - 6;
            float x1 = baseXY.x + textSize.x + 12;
            float y1 = baseXY.y + textSize.y + 6;

            bool culled = false;
            IF (ctx.m_clippingEnabled, 0)
            {
                float clipX = ctx.m_clipX;
                float clipY = ctx.m_clipY;
                float clipR = ctx.m_clipX + ctx.m_clipWidth;
                float clipB = ctx.m_clipY + ctx.m_clipHeight;

                if ((x0 >= clipR) || (y0 >= clipB) || (x1 < clipX) || (y1 < clipY))
                {
                    culled = true;
                }

                x0 = max(clipX, x0);
                y0 = max(clipY, y0);
                x1 = min(clipR, x1);
                y1 = min(clipB, y1);

                //if (!culled && ((x1 - x0 <= 0.0f) || (y1 - y0 <= 0.0f)))
                //  culled = true;
            }

            IF (!culled, 1)
            {
                Vec3 v0(x0, y0, z);
                Vec3 v2(x1, y1, z);
                Vec3 v1(v2.x, v0.y, v0.z); // to avoid float->half conversion
                Vec3 v3(v0.x, v2.y, v0.z); // to avoid float->half conversion

                if (ctx.m_drawTextFlags & eDrawText_UseTransform)
                {
                    v0 = ctx.m_transform * v0;
                    v2 = ctx.m_transform * v2;
                    v1 = ctx.m_transform * v1;
                    v3 = ctx.m_transform * v3;
                }

                Vec2 gradientUvMin, gradientUvMax;
                GetGradientTextureCoord(gradientUvMin.x, gradientUvMin.y, gradientUvMax.x, gradientUvMax.y);

                // define the frame quad
                Vec2 uv(gradientUvMin.x, gradientUvMax.y);
                AddQuad(v0, v1, v2, v3, uv, uv, uv, uv, frameColor);
            }
        }

        // parse the string, ignoring control characters
        uint32_t nextCh = 0;
        Unicode::CIterator<const char*, false> pChar(pStr);
        while (uint32_t ch = *pChar)
        {
            ++pChar;
            nextCh = *pChar;

            switch (ch)
            {
            case '\\':
            {
                if (*pChar != 'n' || !asciiMultiLine)
                {
                    break;
                }
                ++pChar;
            }
            case '\n':
            {
                charX = baseXY.x + offset.x;
                charY += size.y;
                continue;
            }
            break;
            case '\r':
            {
                charX = baseXY.x + offset.x;
                continue;
            }
            break;
            case '\t':
            {
                if (ctx.m_proportional)
                {
                    charX += TabCharCount * size.x * FONT_SPACE_SIZE;
                }
                else
                {
                    charX += TabCharCount * size.x * ctx.m_widthScale;
                }
                continue;
            }
            break;
            case '$':
            {
                if (ctx.m_processSpecialChars)
                {
                    if (*pChar == '$')
                    {
                        ++pChar;
                    }
                    else if (isdigit(*pChar))
                    {
                        if (!i)
                        {
                            int colorIndex = (*pChar) - '0';
                            ColorB newColor = ColorTable[colorIndex];
                            color.r = newColor.r;
                            color.g = newColor.g;
                            color.b = newColor.b;
                            // Leave alpha at original value!
                        }
                        ++pChar;
                        continue;
                    }
                    else if (*pChar == 'O' || *pChar == 'o')
                    {
                        if (!i)
                        {
                            color = passColor;
                        }
                        ++pChar;
                        continue;
                    }
                }
            }
            break;
            default:
                break;
            }

            // get texture coordinates
            float texCoord[4];

            int charOffsetX, charOffsetY; // in font texels
            int charSizeX, charSizeY; // in font texels
            const bool rerenderGlyphs = m_sizeBehavior == SizeBehavior::Rerender;
            const Vec2i requestSize = rerenderGlyphs ? ctx.m_requestSize : CCryFont::defaultGlyphSize;
            m_pFontTexture->GetTextureCoord(m_pFontTexture->GetCharSlot(ch, requestSize), texCoord, charSizeX, charSizeY, charOffsetX, charOffsetY, requestSize);

            int horizontalAdvance = m_pFontTexture->GetHorizontalAdvance(ch, requestSize);
            float advance;

            if (ctx.m_proportional)
            {
                advance = horizontalAdvance * scaleInfo.scale.x;
            }
            else
            {
                advance = size.x * ctx.m_widthScale;
            }

            Vec2 kerningOffset(Vec2_Zero);
            if (ctx.m_kerningEnabled && nextCh)
            {
                kerningOffset = m_pFontTexture->GetKerning(ch, nextCh) * scaleInfo.scale.x;
            }

            float trackingOffset = 0.0f;
            if (nextCh)
            {
                trackingOffset = ctx.m_tracking;
            }

            float px = charX + charOffsetX * scaleInfo.scale.x; // in pixels
            float py = charY + charOffsetY * scaleInfo.scale.y; // in pixels
            float pr = px + charSizeX * scaleInfo.scale.x;
            float pb = py + charSizeY * scaleInfo.scale.y;

            // compute clipping
            float newX = px; // in pixels
            float newY = py; // in pixels
            float newR = pr; // in pixels
            float newB = pb; // in pixels

            if (ctx.m_clippingEnabled)
            {
                float clipX = ctx.m_clipX;
                float clipY = ctx.m_clipY;
                float clipR = ctx.m_clipX + ctx.m_clipWidth;
                float clipB = ctx.m_clipY + ctx.m_clipHeight;

                // clip non visible
                if ((px >= clipR) || (py >= clipB) || (pr < clipX) || (pb < clipY))
                {
                    charX += advance + kerningOffset.x + trackingOffset;
                    continue;
                }
                // clip partially visible
                else
                {
                    float width = horizontalAdvance * scaleInfo.rcpCellWidth;
                    if ((width <= 0.0f) || (size.y <= 0.0f))
                    {
                        charX += advance + kerningOffset.x + trackingOffset;
                        continue;
                    }

                    // clip the image to the scissor rect
                    newX = max(clipX, px);
                    newY = max(clipY, py);
                    newR = min(clipR, pr);
                    newB = min(clipB, pb);

                    float rcpWidth = 1.0f / width;
                    float rcpHeight = 1.0f / size.y;

                    float texW = texCoord[2] - texCoord[0];
                    float texH = texCoord[3] - texCoord[1];

                    // clip horizontal
                    texCoord[0] = texCoord[0] + texW * (newX - px) * rcpWidth;
                    texCoord[2] = texCoord[2] + texW * (newR - pr) * rcpWidth;

                    // clip vertical
                    texCoord[1] = texCoord[1] + texH * (newY - py) * rcpHeight;
                    texCoord[3] = texCoord[3] + texH * (newB - pb) * rcpHeight;
                }
            }

            newX += halfTexelShift;
            newY += halfTexelShift;
            newR += halfTexelShift;
            newB += halfTexelShift;

            //int offset = vbLen * 6;

            Vec3 v0(newX, newY, z);
            Vec3 v2(newR, newB, z);
            Vec3 v1(v2.x, v0.y, v0.z); // to avoid float->half conversion
            Vec3 v3(v0.x, v2.y, v0.z); // to avoid float->half conversion

            Vec2 tc0(texCoord[0], texCoord[1]);
            Vec2 tc2(texCoord[2], texCoord[3]);
            Vec2 tc1(tc2.x, tc0.y); // to avoid float->half conversion
            Vec2 tc3(tc0.x, tc2.y); // to avoid float->half conversion

            uint32 packedColor = 0;
            {
                ColorB tempColor = color;
                tempColor.a = ((uint32) tempColor.a * alphaBlend) >> 8;
                packedColor = tempColor.pack_abgr8888();

                if (!isRGB)
                {
                    packedColor = COLCONV(packedColor);
                }
            }

            if (ctx.m_drawTextFlags & eDrawText_UseTransform)
            {
                v0 = ctx.m_transform * v0;
                v2 = ctx.m_transform * v2;
                v1 = ctx.m_transform * v1;
                v3 = ctx.m_transform * v3;
            }

            AddQuad(v0, v1, v2, v3, tc0, tc1, tc2, tc3, packedColor);

            charX += advance + kerningOffset.x + trackingOffset;
        }
    }
}

CFFont::TextScaleInfoInternal CFFont::CalculateScaleInternal(const STextDrawContext& ctx) const
{
    Vec2 size = GetRestoredFontSize(ctx); // in pixel

    IRenderer* pRenderer = gEnv->pRenderer;
    AZ_Assert(pRenderer, "gEnv->pRenderer is NULL");

    if (ctx.m_sizeIn800x600)
    {
        pRenderer->ScaleCoord(size.x, size.y);
    }

    float rcpCellWidth;
    Vec2 scale;

    int fontTextureCellWidth = GetFontTexture()->GetCellWidth();
    int fontTextureCellHeight = GetFontTexture()->GetCellHeight();

    if (ctx.m_proportional)
    {
        rcpCellWidth = (1.0f / static_cast<float>(fontTextureCellWidth)) * size.x;
        scale = Vec2(rcpCellWidth * ctx.m_widthScale, size.y / static_cast<float>(fontTextureCellHeight));
    }
    else
    {
        rcpCellWidth = size.x / 16.0f;
        scale = Vec2(rcpCellWidth * ctx.m_widthScale, size.y * ctx.m_widthScale / 16.0f);
    }

    return TextScaleInfoInternal(scale, rcpCellWidth);
}

size_t CFFont::GetTextLength(const char* pStr, const bool asciiMultiLine) const
{
    size_t len = 0;

    // parse the string, ignoring control characters
    const char* pChar = pStr;
    while (char ch = *pChar++)
    {
        if ((ch & 0xC0) == 0x80)
        {
            continue;                      // Skip UTF-8 continuation bytes, we count only the first byte of a code-point
        }
        switch (ch)
        {
        case '\\':
        {
            if (*pChar != 'n' || !asciiMultiLine)
            {
                break;
            }
            ++pChar;
        }
        case '\n':
        case '\r':
        case '\t':
        {
            continue;
        }
        break;
        case '$':
        {
            if (*pChar == '$')
            {
                ++pChar;
            }
            else if (*pChar)
            {
                ++pChar;
                continue;
            }
        }
        break;
        default:
            break;
        }
        ++len;
    }

    return len;
}

void CFFont::WrapText(string& result, float maxWidth, const char* pStr, const STextDrawContext& ctx)
{
    result = pStr;

    if (ctx.m_sizeIn800x600)
    {
        maxWidth = gEnv->pRenderer->ScaleCoordX(maxWidth);
    }

    Vec2 strSize = GetTextSizeUInternal(result.c_str(), true, ctx);

    if (strSize.x <= maxWidth)
    {
        return;
    }

    // Assume a given string has multiple lines of text if it's height is
    // greater than the height of its font.
    const bool multiLine = strSize.y > GetRestoredFontSize(ctx).y;

    int lastSpace = -1;
    const char* pLastSpace = NULL;
    float lastSpaceWidth = 0.0f;

    float curCharWidth = 0.0f;
    float curLineWidth = 0.0f;
    float biggestLineWidth = 0.0f;
    float widthSum = 0.0f;

    int curChar = 0;
    Unicode::CIterator<const char*, false> pChar(result.c_str());
    while (uint32_t ch = *pChar)
    {
        // Dollar sign escape codes.  The following scenarios can happen with dollar signs embedded in a string.
        // The following character is...
        //    1. ... a digit, 'O' or 'o' which indicates a color code.  Both characters a skipped in the width calculation.
        //    2. ... another dollar sign.  Only 1 dollar sign is skipped in the width calculation.
        //    3. ... anything else.  The dollar sign is processed in the width calculation.
        if (ctx.m_processSpecialChars && ch == '$')
        {
            ++pChar;
            char nextChar = *pChar;

            if (isdigit(nextChar) || nextChar == 'O' || nextChar == 'o')
            {
                ++pChar;
                continue;
            }
            else if (nextChar != '$')
            {
                --pChar;
            }
        }

        // get char width and sum it to the line width
        // Note: This is not unicode compatible, since char-width depends on surrounding context (ie, combining diacritics etc)
        char codepoint[5];
        Unicode::Convert(codepoint, ch);
        curCharWidth = GetTextSizeUInternal(codepoint, true, ctx).x;

        // keep track of spaces
        // they are good for splitting the string
        if (ch == ' ')
        {
            lastSpace = curChar;
            lastSpaceWidth = curLineWidth + curCharWidth;
            pLastSpace = pChar.GetPosition();
            assert(*pLastSpace == ' ');
        }

        bool prevCharWasNewline = false;
        const bool notFirstChar = pChar.GetPosition() != result.c_str();
        if (*pChar && notFirstChar)
        {
            const char* pPrevCharStr = pChar.GetPosition() - 1;
            prevCharWasNewline = pPrevCharStr[0] == '\n';
        }

        // if line exceed allowed width, split it
        if (prevCharWasNewline || (curLineWidth + curCharWidth >= maxWidth && (*pChar)))
        {
            if (prevCharWasNewline)
            {
                // Reset the current line width to account for newline
                curLineWidth = curCharWidth;
                widthSum += curLineWidth;
            }
            else if ((lastSpace > 0) && ((curChar - lastSpace) < 16) && (curChar - lastSpace >= 0)) // 16 is the default threshold
            {
                *(char*)pLastSpace = '\n';  // This is safe inside UTF-8 because space is single-byte codepoint

                if (lastSpaceWidth > biggestLineWidth)
                {
                    biggestLineWidth = lastSpaceWidth;
                }

                curLineWidth = curLineWidth - lastSpaceWidth + curCharWidth;
                widthSum += curLineWidth;
            }
            else
            {
                const char* pBuf = pChar.GetPosition();
                size_t bytesProcessed = pBuf - result.c_str();
                result.insert(bytesProcessed, '\n'); // Insert the newline, this invalidates the iterator
                pBuf = result.c_str() + bytesProcessed; // In case reallocation occurs, we ensure we are inside the new buffer
                assert(*pBuf == '\n');
                pChar.SetPosition(pBuf); // pChar once again points inside the target string, at the current character
                assert(*pChar == ch);
                ++pChar;
                ++curChar;

                if (curLineWidth > biggestLineWidth)
                {
                    biggestLineWidth = curLineWidth;
                }

                widthSum += curLineWidth;
                curLineWidth = curCharWidth;
            }

            // if we don't need any more line breaks, then just stop, but for
            // multiple lines we can't assume that there aren't any more
            // strings to wrap, so continue
            if (strSize.x - widthSum <= maxWidth && !multiLine)
            {
                break;
            }

            lastSpaceWidth = 0;
            lastSpace = 0;
        }
        else
        {
            curLineWidth += curCharWidth;
        }

        ++curChar;
        ++pChar;
    }
}

void CFFont::GetMemoryUsage (ICrySizer* pSizer) const
{
    pSizer->AddObject(m_name);
    pSizer->AddObject(m_curPath);
    pSizer->AddObject(m_pFontTexture);
    pSizer->AddObject(m_pFontBuffer, m_fontBufferSize);
    pSizer->AddObject(m_effects);
    pSizer->AddObject(m_pDrawVB, sizeof(SVF_P3F_C4B_T2F) * MaxDrawVBQuads * 6);
}

void CFFont::GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const
{
    const CTextureSlot* pSlot = m_pFontTexture->GetGradientSlot();
    assert(pSlot);

    float invWidth = 1.0f / (float) m_pFontTexture->GetWidth();
    float invHeight = 1.0f / (float) m_pFontTexture->GetHeight();

    // deflate by one pixel to avoid bilinear filtering on the borders
    minU = pSlot->vTexCoord[0] + invWidth;
    minV = pSlot->vTexCoord[1] + invHeight;
    maxU = pSlot->vTexCoord[0] + (pSlot->iCharWidth - 1) * invWidth;
    maxV = pSlot->vTexCoord[1] + (pSlot->iCharHeight - 1) * invHeight;
}

unsigned int CFFont::GetEffectId(const char* pEffectName) const
{
    if (pEffectName)
    {
        for (size_t i = 0, numEffects = m_effects.size(); i < numEffects; ++i)
        {
            if (!strcmp(m_effects[i].m_name.c_str(), pEffectName))
            {
                return i;
            }
        }
    }

    return 0;
}

unsigned int CFFont::GetNumEffects() const
{
    return m_effects.size();
}

const char* CFFont::GetEffectName(unsigned int effectId) const
{
    return (effectId < m_effects.size()) ? m_effects[effectId].m_name.c_str() : nullptr;
}

Vec2 CFFont::GetMaxEffectOffset(unsigned int effectId) const
{
    Vec2 maxOffset(0.0f, 0.0f);

    if (effectId < m_effects.size())
    {
        const SEffect& fx = m_effects[effectId];

        for (size_t i = 0, numPasses = fx.m_passes.size(); i < numPasses; ++i)
        {
            const SRenderingPass* pPass = &fx.m_passes[numPasses - i - 1];

            // gather pass data
            Vec2 offset = pPass->m_posOffset;

            if (maxOffset.x < offset.x)
            {
                maxOffset.x = offset.x;
            }

            if (maxOffset.y < offset.y)
            {
                maxOffset.y = offset.y;
            }
        }
    }

    return maxOffset;
}

bool CFFont::DoesEffectHaveTransparency(unsigned int effectId) const
{
    const size_t fxSize = m_effects.size();
    const size_t fxIdx = effectId < fxSize ? effectId : 0;
    const SEffect& fx = m_effects[fxIdx];

    for (auto& pass : fx.m_passes)
    {
        // if the alpha is not 255 then there is transparency
        if (pass.m_color.a != 255)
        {
            return true;
        }
    }

    return false;
}

void CFFont::AddCharsToFontTexture(const char* pChars, int glyphSizeX, int glyphSizeY)
{
    AZStd::lock_guard<AZStd::recursive_mutex> locker(m_fontMutex);
    Vec2i glyphSize(glyphSizeX, glyphSizeY);
    Prepare(pChars, false, glyphSize);
}

Vec2 CFFont::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph, const STextDrawContext& ctx) const
{
    const TextScaleInfoInternal scaleInfo(CalculateScaleInternal(ctx));
    return m_pFontTexture->GetKerning(leftGlyph, rightGlyph) * scaleInfo.scale.x;
}

float CFFont::GetAscender(const STextDrawContext& ctx) const
{
    return (ctx.m_size.y * m_pFontTexture->GetAscenderToHeightRatio());
}

float CFFont::GetBaseline(const STextDrawContext& ctx) const
{
    const TextScaleInfoInternal scaleInfo(CalculateScaleInternal(ctx));
    // Calculate baseline the same way as the font renderer which uses the glyph height * size ratio.
    // Adding 1 because FontTexture always adds 1 to the char height in GetTextureCoord
    return (round(m_pFontTexture->GetCellHeight() * GetSizeRatio()) + 1.0f) * scaleInfo.scale.y;
}

bool CFFont::InitTexture()
{    
    m_texID = gEnv->pRenderer->FontCreateTexture(m_pFontTexture->GetWidth(), m_pFontTexture->GetHeight(), (uint8*)m_pFontTexture->GetBuffer(), eTF_A8, IRenderer::FontCreateTextureGenMipsDefaultValue, m_name.c_str());
    m_textureVersion = 0;
    return m_texID >= 0;
}

bool CFFont::InitCache()
{
    m_pFontTexture->CreateGradientSlot();

    // precache (not required but for faster printout later)
    const char first = ' ';
    const char last = '~';
    char buf[last - first + 2];
    char* p = buf;

    // precache all [normal] printable characters to the string (missing ones are updated on demand)
    for (int i = first; i <= last; ++i)
    {
        *p++ = i;
    }
    *p = 0;

    Prepare(buf, false);

    return true;
}

CFFont::SEffect* CFFont::AddEffect(const char* pEffectName)
{
    m_effects.push_back(SEffect(pEffectName));
    return &m_effects[m_effects.size() - 1];
}

CFFont::SEffect* CFFont::GetDefaultEffect()
{
    return &m_effects[0];
}

void CFFont::Prepare(const char* pStr, bool updateTexture, const Vec2i& glyphSize)
{
    const bool rerenderGlyphs = m_sizeBehavior == SizeBehavior::Rerender;
    const Vec2i usedGlyphSize = rerenderGlyphs ? glyphSize : CCryFont::defaultGlyphSize;
    bool texUpdateNeeded = m_pFontTexture->PreCacheString(pStr, nullptr, m_sizeRatio, usedGlyphSize, m_fontHintParams) == 1 || m_fontTexDirty;
    if (updateTexture && texUpdateNeeded && m_texID >= 0)
    {
        gEnv->pRenderer->FontUpdateTexture(m_texID, 0, 0, m_pFontTexture->GetWidth(), m_pFontTexture->GetHeight(), (unsigned char*)m_pFontTexture->GetBuffer());
        m_fontTexDirty = false;
        ++m_textureVersion;

        // Let any listeners know that the font texture has changed
        EBUS_EVENT(FontNotificationBus, OnFontTextureUpdated, this);
    }
    else
    {
        m_fontTexDirty = texUpdateNeeded;
    }
}

Vec2 CFFont::GetRestoredFontSize(const STextDrawContext& ctx) const
{
    // Calculate the scale that we need to apply to the text size to ensure
    // it's on-screen size is the same regardless of the slot scaling needed
    // to fit the glyphs of the font within the font texture slots.
    float restoringScale = IFFontConstants::defaultSizeRatio / m_sizeRatio;
    return Vec2(ctx.m_size.x * restoringScale, ctx.m_size.y * restoringScale);
}

#endif

