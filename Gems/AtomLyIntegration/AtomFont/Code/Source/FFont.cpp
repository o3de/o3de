/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Font class.



#if !defined(USE_NULLFONT_ALWAYS)

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzFramework/Viewport/ScreenGeometry.h>

#include <AzFramework/Archive/Archive.h>

#include <AtomLyIntegration/AtomFont/FFont.h>
#include <AtomLyIntegration/AtomFont/AtomFont.h>
#include <AtomLyIntegration/AtomFont/FontTexture.h>
#include <CryCommon/MathConversion.h>

#include <AzCore/std/parallel/lock.h>

#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <AzCore/Interface/Interface.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/ImagePool.h>

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

static const int TabCharCount = 4;
// set buffer sizes to hold max characters that can be drawn in 1 DrawString call
static const size_t MaxVerts = 8 * 1024; // 2048 quads
static const size_t MaxIndices = (MaxVerts * 6) / 4; // 6 indices per quad, 6/4 * MaxVerts

AZ::FFont::FFont(AZ::AtomFont* atomFont, const char* fontName)
    : m_name(fontName)
    , m_atomFont(atomFont)
{
    assert(m_name.c_str());
    assert(m_atomFont);

    // create default effect
    FontEffect* effect = AddEffect("default");
    effect->AddPass();

    // Create cpu memory to cache the font draw data before submit
    m_vertexBuffer = new SVF_P3F_C4B_T2F[MaxVerts];
    m_indexBuffer = new u16[MaxIndices];

    m_vertexCount = 0;
    m_indexCount = 0;

    AddRef();
}

AZ::RPI::ViewportContextPtr AZ::FFont::GetDefaultViewportContext() const
{
    auto viewContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
    return viewContextManager->GetDefaultViewportContext();
}

AZ::RPI::WindowContextSharedPtr AZ::FFont::GetDefaultWindowContext() const
{
    if (auto defaultViewportContext = GetDefaultViewportContext())
    {
        return defaultViewportContext->GetWindowContext();
    }
    return {};
}

AZ::FFont::~FFont()
{
    AZ_Assert(m_atomFont == nullptr, "The font should already be unregistered through a call to AZ::FFont::Release()");

    delete[] m_vertexBuffer;
    delete[] m_indexBuffer;

    Free();
}

int32_t AZ::FFont::AddRef()
{
    ref_count::add_ref();
    return aznumeric_cast<int32_t>(ref_count::use_count());
}

int32_t AZ::FFont::Release()
{
    int32_t useCount = aznumeric_cast<int32_t>(ref_count::use_count()) - 1;
    ref_count::release();
    return useCount;
}

// Load a font from a TTF file
bool AZ::FFont::Load(const char* fontFilePath, unsigned int width, unsigned int height, unsigned int widthNumSlots, unsigned int heightNumSlots, unsigned int flags, float sizeRatio)
{
    if (!fontFilePath)
    {
        return false;
    }

    Free();

    auto fileIoBase = AZ::IO::FileIOBase::GetInstance();

    AZ::IO::Path fullFile(m_curPath);
    fullFile /= fontFilePath;

    int smoothMethodFlag = (flags & TTFFLAG_SMOOTH_MASK) >> TTFFLAG_SMOOTH_SHIFT;
    AZ::FontSmoothMethod smoothMethod = AZ::FontSmoothMethod::None;
    switch (smoothMethodFlag)
    {
    case TTFFLAG_SMOOTH_BLUR:
        smoothMethod = AZ::FontSmoothMethod::Blur;
        break;
    case TTFFLAG_SMOOTH_SUPERSAMPLE:
        smoothMethod = AZ::FontSmoothMethod::SuperSample;
        break;
    }

    int smoothAmountFlag = (flags & TTFFLAG_SMOOTH_AMOUNT_MASK);
    AZ::FontSmoothAmount smoothAmount = AZ::FontSmoothAmount::None;
    switch (smoothAmountFlag)
    {
    case TTFLAG_SMOOTH_AMOUNT_2X:
        smoothAmount = AZ::FontSmoothAmount::x2;
        break;
    case TTFLAG_SMOOTH_AMOUNT_4X:
        smoothAmount = AZ::FontSmoothAmount::x4;
        break;
    }


    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
    fileIoBase->Open(fullFile.c_str(), AZ::IO::GetOpenModeFromStringMode("rb"), fileHandle);
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return false;
    }

    AZ::u64 fileSize{};
    fileIoBase->Size(fileHandle, fileSize);
    if (!fileSize)
    {
        fileIoBase->Close(fileHandle);
        return false;
    }

    auto buffer = AZStd::make_unique<uint8_t[]>(fileSize);
    if (!fileIoBase->Read(fileHandle, buffer.get(), fileSize))
    {
        fileIoBase->Close(fileHandle);
        return false;
    }

    fileIoBase->Close(fileHandle);

    if (!m_fontTexture)
    {
        m_fontTexture = new FontTexture();
    }
    if (!m_fontTexture || !m_fontTexture->CreateFromMemory(buffer.get(), (int)fileSize, width, height, smoothMethod, smoothAmount, widthNumSlots, heightNumSlots, sizeRatio))
    {
        return false;
    }

    m_monospacedFont = m_fontTexture->GetMonospaced();
    m_fontBuffer = AZStd::move(buffer);
    m_fontBufferSize = fileSize;
    m_fontTexDirty = false;
    m_sizeRatio = sizeRatio;

    InitCache();

    return true;
}


void AZ::FFont::Free()
{
    m_fontImage = nullptr;
    m_fontImageVersion = 0;

    delete m_fontTexture;
    m_fontTexture = nullptr;

    m_fontBuffer.reset();
    m_fontBufferSize = 0;
}

void AZ::FFont::DrawString(float x, float y, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx)
{
    if (!str)
    {
        return;
    }

    DrawStringUInternal(GetDefaultWindowContext()->GetViewport(), GetDefaultViewportContext(), x, y, 1.0f, str, asciiMultiLine, ctx);
}

void AZ::FFont::DrawString(float x, float y, float z, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx)
{
    if (!str)
    {
        return;
    }

    DrawStringUInternal(GetDefaultWindowContext()->GetViewport(), GetDefaultViewportContext(), x, y, z, str, asciiMultiLine, ctx);
}

void AZ::FFont::DrawStringUInternal(
    const RHI::Viewport& viewport, 
    RPI::ViewportContextPtr viewportContext, 
    float x,
    float y, 
    float z, 
    const char* str, 
    const bool asciiMultiLine, 
    const TextDrawContext& ctx)
{
    // Lazily ensure we're initialized before attempting to render.
    // Validate that there is a render scene before attempting to init.
    if (!viewportContext || !viewportContext->GetRenderScene())
    {
        return;
    }

    if (!str
        || !m_vertexBuffer // vertex buffer isn't created until BootstrapScene is ready, Editor tries to render text before that.
        || !m_fontTexture
        || ctx.m_fxIdx >= m_effects.size()
        || m_effects[ctx.m_fxIdx].m_passes.empty())
    {
        return;
    }

    const size_t fxSize = m_effects.size();
    if (fxSize && !m_fontImage && !InitTexture())
    {
        return;
    }

    const bool orthoMode = ctx.m_overrideViewProjMatrices;

    const float viewX = viewport.m_minX;
    const float viewY = viewport.m_minY;
    const float viewWidth = viewport.m_maxX - viewport.m_minX;
    const float viewHeight = viewport.m_maxY - viewport.m_minY;
    const float zf = viewport.m_minZ;
    const float zn = viewport.m_maxZ;

    Matrix4x4 modelViewProjMat;
    if (!orthoMode)
    {
        AZ::RPI::ViewPtr view = viewportContext->GetDefaultView();
        modelViewProjMat = view->GetWorldToClipMatrix();
    }
    else
    {
        if (viewWidth == 0 || viewHeight == 0)
        {
            return;
        }
        AZ::MakeOrthographicMatrixRH(modelViewProjMat, viewX, viewX + viewWidth, viewY + viewHeight, viewY, zn, zf);
    }

    size_t startingVertexCount = m_vertexCount;

    // Local function that is passed into CreateQuadsForText as the AddQuad function
    AZ::FFont::AddFunction AddQuad = [this, startingVertexCount]
            (const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec2& tc0, const Vec2& tc1, const Vec2& tc2, const Vec2& tc3, uint32_t packedColor)
        {
            const bool vertexSpaceLeft = m_vertexCount + 4 < MaxVerts;
            const bool indexSpaceLeft = m_indexCount + 6 < MaxIndices;
            if (!vertexSpaceLeft || !indexSpaceLeft)
            {
                return false;
            }

            size_t vertexOffset = m_vertexCount;
            m_vertexCount += 4;
            size_t indexOffset = m_indexCount;
            m_indexCount += 6;

            // define char quad
            m_vertexBuffer[vertexOffset + 0].xyz = v0;
            m_vertexBuffer[vertexOffset + 0].color.dcolor = packedColor;
            m_vertexBuffer[vertexOffset + 0].st = tc0;

            m_vertexBuffer[vertexOffset + 1].xyz = v1;
            m_vertexBuffer[vertexOffset + 1].color.dcolor = packedColor;
            m_vertexBuffer[vertexOffset + 1].st = tc1;

            m_vertexBuffer[vertexOffset + 2].xyz = v2;
            m_vertexBuffer[vertexOffset + 2].color.dcolor = packedColor;
            m_vertexBuffer[vertexOffset + 2].st = tc2;

            m_vertexBuffer[vertexOffset + 3].xyz = v3;
            m_vertexBuffer[vertexOffset + 3].color.dcolor = packedColor;
            m_vertexBuffer[vertexOffset + 3].st = tc3;

            uint16_t startingIndex = static_cast<uint16_t>(vertexOffset - startingVertexCount);
            m_indexBuffer[indexOffset + 0] = startingIndex + 0;
            m_indexBuffer[indexOffset + 1] = startingIndex + 1;
            m_indexBuffer[indexOffset + 2] = startingIndex + 2;
            m_indexBuffer[indexOffset + 3] = startingIndex + 2;
            m_indexBuffer[indexOffset + 4] = startingIndex + 3;
            m_indexBuffer[indexOffset + 5] = startingIndex + 0;
            return true;
        };

    int numQuads = 0;
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_vertexDataMutex);
        numQuads = CreateQuadsForText(viewport, x, y, z, str, asciiMultiLine, ctx, AddQuad);
    }

    if (numQuads)
    {
        AZ::RPI::Ptr<AZ::RPI::DynamicDrawContext> dynamicDraw = AZ::AtomBridge::PerViewportDynamicDraw::Get()->GetDynamicDrawContextForViewport(m_dynamicDrawContextName, viewportContext->GetId());
        if (dynamicDraw)
        {
            //setup per draw srg
            auto drawSrg = dynamicDraw->NewDrawSrg();
            drawSrg->SetConstant(m_fontShaderData.m_viewProjInputIndex, modelViewProjMat);
            drawSrg->SetImageView(m_fontShaderData.m_imageInputIndex, m_fontStreamingImage->GetImageView());
            drawSrg->Compile();

            dynamicDraw->DrawIndexed(m_vertexBuffer, m_vertexCount, m_indexBuffer, m_indexCount, RHI::IndexFormat::Uint16, drawSrg);
        }
        m_indexCount = 0;
        m_vertexCount = 0;
    }
}

Vec2 AZ::FFont::GetTextSize(const char* str, const bool asciiMultiLine, const TextDrawContext& ctx)
{
    if (!str)
    {
        return Vec2(0.0f, 0.0f);
    }

    return GetTextSizeUInternal(GetDefaultWindowContext()->GetViewport(), str, asciiMultiLine, ctx);
}

Vec2 AZ::FFont::GetTextSizeUInternal(
    const RHI::Viewport& viewport, 
    const char* str, 
    const bool asciiMultiLine, 
    const TextDrawContext& ctx)
{
    const size_t fxSize = m_effects.size();

    if (!str || !m_fontTexture || !fxSize)
    {
        return Vec2(0, 0);
    }

    Prepare(str, false, ctx.m_requestSize);

    // This is the "logical" size of the font (in pixels). The actual size of
    // the glyphs in the font texture may have additional scaling applied or
    // could have been re-rendered at a different size.
    Vec2 size = ctx.m_size;
    if (ctx.m_sizeIn800x600)
    {
        ScaleCoord(viewport, size.x, size.y);
    }

    // This scaling takes into account the logical size of the font relative
    // to any additional scaling applied (such as from "size ratio").
    const TextScaleInfoInternal scaleInfo(CalculateScaleInternal(viewport, ctx));

    float maxW = 0;
    float maxH = 0;

    const size_t fxIdx = ctx.m_fxIdx < fxSize ? ctx.m_fxIdx : 0;
    const FontEffect& fx = m_effects[fxIdx];

    AZStd::wstring strW;
    AZStd::to_wstring(strW, str);

    for (size_t i = 0, numPasses = fx.m_passes.size(); i < numPasses; ++i)
    {
        const FontRenderingPass* pass = &fx.m_passes[numPasses - i - 1];

        // gather pass data
        Vec2 offset = pass->m_posOffset;

        float charX = offset.x;
        float charY = offset.y + size.y;

        if (charY > maxH)
        {
            maxH = charY;
        }

        // parse the string, ignoring control characters
        uint32_t nextCh = 0;
        const wchar_t* pChar = strW.c_str();
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
                charY += size.y * (1.f + ctx.GetLineSpacing());

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
                    charX += TabCharCount * size.x * AZ_FONT_SPACE_SIZE;
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
            const AtomFont::GlyphSize requestSize = rerenderGlyphs ? ctx.m_requestSize : AtomFont::defaultGlyphSize;
            int horizontalAdvance = m_fontTexture->GetHorizontalAdvance(ch, requestSize);
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
                kerningOffset = m_fontTexture->GetKerning(ch, nextCh) * scaleInfo.scale.x;
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

uint32_t AZ::FFont::GetNumQuadsForText(const char* str, const bool asciiMultiLine, const TextDrawContext& ctx)
{
    uint32_t numQuads = 0;

    const size_t fxSize = m_effects.size();
    const size_t fxIdx = ctx.m_fxIdx < fxSize ? ctx.m_fxIdx : 0;
    const FontEffect& fx = m_effects[fxIdx];

    AZStd::wstring strW;
    AZStd::to_wstring(strW, str);

    for (size_t j = 0, numPasses = fx.m_passes.size(); j < numPasses; ++j)
    {
        size_t i = numPasses - j - 1;
        bool drawFrame = ctx.m_framed && i == numPasses - 1;

        if (drawFrame)
        {
            ++numQuads;
        }

        const wchar_t* pChar = strW.c_str();
        while (uint32_t ch = *pChar)
        {
            ++pChar;

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

uint32_t AZ::FFont::WriteTextQuadsToBuffers(SVF_P2F_C4B_T2F_F4B* verts, uint16_t* indices, uint32_t maxQuads, float x, float y, float z, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx)
{
    uint32_t numQuadsWritten = 0;

    const size_t fxSize = m_effects.size();
    if (fxSize && !m_fontImage && !InitTexture())
    {
        return numQuadsWritten;
    }

    SVF_P2F_C4B_T2F_F4B* vertexData = verts;
    uint16_t* indexData = indices;
    size_t vertexOffset = 0;
    size_t indexOffset = 0;

    // Local function that is passed into CreateQuadsForText as the AddQuad function
    AddFunction AddQuad = [&vertexData, &indexData, &vertexOffset, &indexOffset, maxQuads, &numQuadsWritten]
            (const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& v3, const Vec2& tc0, const Vec2& tc1, const Vec2& tc2, const Vec2& tc3, uint32_t packedColor)
        {
            Vec2 xy0(v0);
            Vec2 xy1(v1);
            Vec2 xy2(v2);
            Vec2 xy3(v3);

            const bool vertexSpaceLeft = vertexOffset + 3 < maxQuads * 4;
            const bool indexSpaceLeft = indexOffset + 5 < maxQuads * 6;
            if (!vertexSpaceLeft || !indexSpaceLeft)
            {
                return false;
            }

            // This should never happen but for safety make sure we never write off end of buffers (should hit asserts above if this is the case)
            if (numQuadsWritten < maxQuads)
            {
                // define char quad
                vertexData[vertexOffset].xy = xy0;
                vertexData[vertexOffset].color.dcolor = packedColor;
                vertexData[vertexOffset].st = tc0;
                vertexData[vertexOffset].texIndex = 0;
                vertexData[vertexOffset].texHasColorChannel = 0;
                vertexData[vertexOffset].texIndex2 = 0;
                vertexData[vertexOffset].pad = 0;

                vertexData[vertexOffset + 1].xy = xy1;
                vertexData[vertexOffset + 1].color.dcolor = packedColor;
                vertexData[vertexOffset + 1].st = tc1;
                vertexData[vertexOffset + 1].texIndex = 0;
                vertexData[vertexOffset + 1].texHasColorChannel = 0;
                vertexData[vertexOffset + 1].texIndex2 = 0;
                vertexData[vertexOffset + 1].pad = 0;

                vertexData[vertexOffset + 2].xy = xy2;
                vertexData[vertexOffset + 2].color.dcolor = packedColor;
                vertexData[vertexOffset + 2].st = tc2;
                vertexData[vertexOffset + 2].texIndex = 0;
                vertexData[vertexOffset + 2].texHasColorChannel = 0;
                vertexData[vertexOffset + 2].texIndex2 = 0;
                vertexData[vertexOffset + 2].pad = 0;

                vertexData[vertexOffset + 3].xy = xy3;
                vertexData[vertexOffset + 3].color.dcolor = packedColor;
                vertexData[vertexOffset + 3].st = tc3;
                vertexData[vertexOffset + 3].texIndex = 0;
                vertexData[vertexOffset + 3].texHasColorChannel = 0;
                vertexData[vertexOffset + 3].texIndex2 = 0;
                vertexData[vertexOffset + 3].pad = 0;

                indexData[indexOffset + 0] = static_cast<uint16_t>(vertexOffset + 0);
                indexData[indexOffset + 1] = static_cast<uint16_t>(vertexOffset + 1);
                indexData[indexOffset + 2] = static_cast<uint16_t>(vertexOffset + 2);
                indexData[indexOffset + 3] = static_cast<uint16_t>(vertexOffset + 2);
                indexData[indexOffset + 4] = static_cast<uint16_t>(vertexOffset + 3);
                indexData[indexOffset + 5] = static_cast<uint16_t>(vertexOffset + 0);

                vertexOffset += 4;
                indexOffset += 6;

                ++numQuadsWritten;
            }
            return true;
        };

    CreateQuadsForText(GetDefaultWindowContext()->GetViewport(), x, y, z, str, asciiMultiLine, ctx, AddQuad);

    return numQuadsWritten;
}

uint32_t AZ::FFont::GetFontTextureVersion()
{
    return m_fontImageVersion;
}

int AZ::FFont::CreateQuadsForText(const RHI::Viewport& viewport, float x, float y, float z, const char* str, const bool asciiMultiLine, const TextDrawContext& ctx,
    AddFunction AddQuad)
{
    int numQuads = 0;
    const size_t fxSize = m_effects.size();

    Prepare(str, true, ctx.m_requestSize);

    const size_t fxIdx = ctx.m_fxIdx < fxSize ? ctx.m_fxIdx : 0;
    const FontEffect& fx = m_effects[fxIdx];

    bool passZeroColorOverridden = ctx.IsColorOverridden();

    uint32_t alphaBlend = passZeroColorOverridden ? ctx.m_colorOverride.a : fx.m_passes[0].m_color.a;
    if (alphaBlend > 128)
    {
        ++alphaBlend; // 0..256 for proper blending
    }

    // This is the "logical" size of the font (in pixels). The actual size of
    // the glyphs in the font texture may have additional scaling applied or
    // could have been re-rendered at a different size.
    Vec2 size = ctx.m_size;
    if (ctx.m_sizeIn800x600)
    {
        ScaleCoord(viewport, size.x, size.y);
    }

    // This scaling takes into account the logical size of the font relative
    // to any additional scaling applied (such as from "size ratio").
    const TextScaleInfoInternal scaleInfo(CalculateScaleInternal(viewport, ctx));

    Vec2 baseXY = Vec2(x, y); // in pixels
    if (ctx.m_sizeIn800x600)
    {
        ScaleCoord(viewport, baseXY.x, baseXY.y);
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

        const FontRenderingPass* pass = &fx.m_passes[i];

        if (!i)
        {
            alphaBlend = 256;
        }

        const ColorB& passColor = !i && passZeroColorOverridden ? ctx.m_colorOverride : fx.m_passes[i].m_color;

        // gather pass data
        Vec2 offset = pass->m_posOffset; // in pixels

        float charX = baseXY.x + offset.x; // in pixels
        float charY = baseXY.y + offset.y; // in pixels

        ColorB color = passColor;

        bool drawFrame = ctx.m_framed && i == numPasses - 1;

        if (drawFrame)
        {
            ColorB tempColor(255, 255, 255, 255);
            uint32_t frameColor = tempColor.pack_argb8888();        //note: this ends up in r,g,b,a order on little-endian machines

            Vec2 textSize = GetTextSizeUInternal(viewport, str, asciiMultiLine, ctx);

            float x0 = baseXY.x - 12;
            float y0 = baseXY.y - 6;
            float x1 = baseXY.x + textSize.x + 12;
            float y1 = baseXY.y + textSize.y + 6;

            bool culled = false;
            if (ctx.m_clippingEnabled)
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
            }

            if (!culled)
            {
                Vec3 v0(x0, y0, z);
                Vec3 v2(x1, y1, z);
                Vec3 v1(v2.x, v0.y, v0.z);
                Vec3 v3(v0.x, v2.y, v0.z);

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
                if (AddQuad(v0, v1, v2, v3, uv, uv, uv, uv, frameColor))
                {
                    ++numQuads;
                }
                else
                {
                    return numQuads;
                }
            }
        }

        AZStd::wstring strW;
        AZStd::to_wstring(strW, str);

        // parse the string, ignoring control characters
        uint32_t nextCh = 0;
        const wchar_t* pChar = strW.c_str();
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
                charY += size.y * (1.f + ctx.GetLineSpacing());
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
                    charX += TabCharCount * size.x * AZ_FONT_SPACE_SIZE;
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
                            static const AZ::Color ColorTable[10] =
                            {
                                AZ::Colors::Black,
                                AZ::Colors::White,
                                AZ::Colors::Blue,
                                AZ::Colors::Lime,
                                AZ::Colors::Red,
                                AZ::Colors::Cyan,
                                AZ::Colors::Yellow,
                                AZ::Colors::Fuchsia,
                                AZ::Colors::Orange,
                                AZ::Colors::Grey,
                            };

                            int colorIndex = (*pChar) - '0';
                            ColorB newColor = AZColorToLYColorB(ColorTable[colorIndex]);
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
            const AtomFont::GlyphSize requestSize = rerenderGlyphs ? ctx.m_requestSize : AtomFont::defaultGlyphSize;
            m_fontTexture->GetTextureCoord(m_fontTexture->GetCharSlot(ch, requestSize), texCoord, charSizeX, charSizeY, charOffsetX, charOffsetY, requestSize);

            int horizontalAdvance = m_fontTexture->GetHorizontalAdvance(ch, requestSize);
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
                kerningOffset = m_fontTexture->GetKerning(ch, nextCh) * scaleInfo.scale.x;
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

            Vec3 v0(newX, newY, z);
            Vec3 v2(newR, newB, z);
            Vec3 v1(v2.x, v0.y, v0.z);
            Vec3 v3(v0.x, v2.y, v0.z);

            Vec2 tc0(texCoord[0], texCoord[1]);
            Vec2 tc2(texCoord[2], texCoord[3]);
            Vec2 tc1(tc2.x, tc0.y);
            Vec2 tc3(tc0.x, tc2.y);

            uint32_t packedColor = 0xffffffff;
            {
                ColorB tempColor = color;
                tempColor.a = static_cast<uint8_t>(((uint32_t) tempColor.a * alphaBlend) >> 8);
                packedColor = tempColor.pack_argb8888();                    //note: this ends up in r,g,b,a order on little-endian machines
            }

            if (ctx.m_drawTextFlags & eDrawText_UseTransform)
            {
                v0 = ctx.m_transform * v0;
                v2 = ctx.m_transform * v2;
                v1 = ctx.m_transform * v1;
                v3 = ctx.m_transform * v3;
            }

            if (AddQuad(v0, v1, v2, v3, tc0, tc1, tc2, tc3, packedColor))
            {
                ++numQuads;
            }
            else
            {
                return numQuads;
            }
            charX += advance + kerningOffset.x + trackingOffset;
        }
    }
    return numQuads;
}

AZ::FFont::TextScaleInfoInternal AZ::FFont::CalculateScaleInternal(const RHI::Viewport& viewport, const TextDrawContext& ctx) const
{
    Vec2 size = GetRestoredFontSize(ctx); // in pixel

    if (ctx.m_sizeIn800x600)
    {
        ScaleCoord(viewport, size.x, size.y);
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

size_t AZ::FFont::GetTextLength(const char* str, const bool asciiMultiLine) const
{
    size_t len = 0;

    // parse the string, ignoring control characters
    const char* pChar = str;
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

void AZ::FFont::WrapText(AZStd::string& result, float maxWidth, const char* str, const TextDrawContext& ctx)
{
    result = str;

    if (ctx.m_sizeIn800x600)
    {
        // ToDo: Update to work with Atom? LYN-3676
        // maxWidth = ???->ScaleCoordX(maxWidth);
    }

    Vec2 strSize = GetTextSize(result.c_str(), true, ctx);

    if (strSize.x <= maxWidth)
    {
        return;
    }

    // Assume a given string has multiple lines of text if it's height is
    // greater than the height of its font.
    const bool multiLine = strSize.y > GetRestoredFontSize(ctx).y;

    int lastSpace = -1;
    const wchar_t* pLastSpace = NULL;
    float lastSpaceWidth = 0.0f;

    float curCharWidth = 0.0f;
    float curLineWidth = 0.0f;
    float biggestLineWidth = 0.0f;
    float widthSum = 0.0f;

    int curChar = 0;
    AZStd::wstring resultW;
    AZStd::to_wstring(resultW, result.c_str());
    const wchar_t* pChar = resultW.c_str();
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
            char nextChar = static_cast<char>(*pChar);

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
        AZStd::to_string(codepoint, 5, { (wchar_t*)&ch, 1 });
        curCharWidth = GetTextSize(codepoint, true, ctx).x;

        // keep track of spaces
        // they are good for splitting the string
        if (ch == ' ')
        {
            lastSpace = curChar;
            lastSpaceWidth = curLineWidth + curCharWidth;
            pLastSpace = pChar;
            assert(*pLastSpace == ' ');
        }

        bool prevCharWasNewline = false;
        const bool notFirstChar = pChar != resultW.c_str();
        if (*pChar && notFirstChar)
        {
            const wchar_t* pPrevCharStr = pChar - 1;
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
                const wchar_t* buf = pChar;
                size_t bytesProcessed = buf - resultW.c_str();
                resultW.insert(resultW.begin() + bytesProcessed, L'\n'); // Insert the newline, this invalidates the iterator
                buf = resultW.c_str() + bytesProcessed; // In case reallocation occurs, we ensure we are inside the new buffer
                assert(*buf == '\n');
                pChar = buf; // pChar once again points inside the target string, at the current character
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

void AZ::FFont::GetGradientTextureCoord(float& minU, float& minV, float& maxU, float& maxV) const
{
    const TextureSlot* slot = m_fontTexture->GetGradientSlot();
    assert(slot);

    float invWidth = 1.0f / (float) m_fontTexture->GetWidth();
    float invHeight = 1.0f / (float) m_fontTexture->GetHeight();

    // deflate by one pixel to avoid bilinear filtering on the borders
    minU = slot->m_texCoords[0] + invWidth;
    minV = slot->m_texCoords[1] + invHeight;
    maxU = slot->m_texCoords[0] + (slot->m_characterWidth - 1) * invWidth;
    maxV = slot->m_texCoords[1] + (slot->m_characterHeight - 1) * invHeight;
}

unsigned int AZ::FFont::GetEffectId(const char* effectName) const
{
    if (effectName)
    {
        for (size_t i = 0, numEffects = m_effects.size(); i < numEffects; ++i)
        {
            if (!strcmp(m_effects[i].m_name.c_str(), effectName))
            {
                return static_cast<unsigned int>(i);
            }
        }
    }

    return 0;
}

unsigned int AZ::FFont::GetNumEffects() const
{
    return static_cast<unsigned int>(m_effects.size());
}

const char* AZ::FFont::GetEffectName(unsigned int effectId) const
{
    return (effectId < m_effects.size()) ? m_effects[effectId].m_name.c_str() : nullptr;
}

Vec2 AZ::FFont::GetMaxEffectOffset(unsigned int effectId) const
{
    Vec2 maxOffset(0.0f, 0.0f);

    if (effectId < m_effects.size())
    {
        const FontEffect& fx = m_effects[effectId];

        for (size_t i = 0, numPasses = fx.m_passes.size(); i < numPasses; ++i)
        {
            const FontRenderingPass* pass = &fx.m_passes[numPasses - i - 1];

            // gather pass data
            Vec2 offset = pass->m_posOffset;

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

bool AZ::FFont::DoesEffectHaveTransparency(unsigned int effectId) const
{
    const size_t fxSize = m_effects.size();
    const size_t fxIdx = effectId < fxSize ? effectId : 0;
    const FontEffect& fx = m_effects[fxIdx];

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

void AZ::FFont::AddCharsToFontTexture(const char* chars, int glyphSizeX, int glyphSizeY)
{
    AtomFont::GlyphSize glyphSize(glyphSizeX, glyphSizeY);
    Prepare(chars, false, glyphSize);
}

Vec2 AZ::FFont::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph, const TextDrawContext& ctx) const
{
    return GetKerningInternal(GetDefaultWindowContext()->GetViewport(), leftGlyph, rightGlyph, ctx);
}

Vec2 AZ::FFont::GetKerningInternal(const RHI::Viewport& viewport, uint32_t leftGlyph, uint32_t rightGlyph, const TextDrawContext& ctx) const
{
    const TextScaleInfoInternal scaleInfo(CalculateScaleInternal(viewport, ctx));
    return m_fontTexture->GetKerning(leftGlyph, rightGlyph) * scaleInfo.scale.x;
}

float AZ::FFont::GetAscender(const TextDrawContext& ctx) const
{
    return (ctx.m_size.y * m_fontTexture->GetAscenderToHeightRatio());
}

float AZ::FFont::GetBaseline(const TextDrawContext& ctx) const
{
    return GetBaselineInternal(GetDefaultWindowContext()->GetViewport(), ctx);
}

float AZ::FFont::GetBaselineInternal(const RHI::Viewport& viewport, const TextDrawContext& ctx) const
{
    const TextScaleInfoInternal scaleInfo(CalculateScaleInternal(viewport, ctx));
    // Calculate baseline the same way as the font renderer which uses the glyph height * size ratio.
    // Adding 1 because FontTexture always adds 1 to the char height in GetTextureCoord
    return (round(m_fontTexture->GetCellHeight() * GetSizeRatio()) + 1.0f) * scaleInfo.scale.y;
}


bool AZ::FFont::InitTexture()
{
    using namespace AZ;

    RHI::Format rhiImageFormat = RHI::Format::R8_UNORM;
    int width = m_fontTexture->GetWidth();
    int height = m_fontTexture->GetHeight();
    uint8_t* fontImageData = m_fontTexture->GetBuffer();
    uint32_t fontImageDataSize = RHI::GetFormatSize(rhiImageFormat) * width * height;

    Data::Instance<RPI::StreamingImagePool> streamingImagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();
    m_fontStreamingImage = RPI::StreamingImage::CreateFromCpuData(
        *streamingImagePool.get(),
        RHI::ImageDimension::Image2D,
        RHI::Size(width, height, 1),
        rhiImageFormat,
        fontImageData,
        fontImageDataSize);

    m_fontImage = m_fontStreamingImage->GetRHIImage();
    m_fontImage->SetName(Name(m_name.c_str()));

    m_fontImageVersion = 0;
    return true;
}

bool AZ::FFont::UpdateTexture()
{
    using namespace AZ;

    if (!m_fontImage)
    {
        return false;
    }

    if (m_fontTexture->GetWidth() != static_cast<int>(m_fontImage->GetDescriptor().m_size.m_width) || m_fontTexture->GetHeight() != static_cast<int>(m_fontImage->GetDescriptor().m_size.m_height))
    {
        AZ_Assert(false, "AtomFont::FFont:::UpdateTexture size mismatch between texture and image!");
        return false;
    }

    RHI::ImageSubresourceRange range;
    range.m_mipSliceMin = 0;
    range.m_mipSliceMax = 0;
    range.m_arraySliceMin = 0;
    range.m_arraySliceMax = 0;
    RHI::ImageSubresourceLayoutPlaced layout;
    m_fontImage->GetSubresourceLayouts(range, &layout, nullptr);

    RHI::ImageUpdateRequest imageUpdateReq;
    imageUpdateReq.m_image = m_fontImage.get();
    imageUpdateReq.m_imageSubresource = RHI::ImageSubresource{ 0, 0 };
    imageUpdateReq.m_sourceData = m_fontTexture->GetBuffer();
    imageUpdateReq.m_sourceSubresourceLayout = layout;

    m_fontStreamingImage->UpdateImageContents(imageUpdateReq);

    return true;
}

bool AZ::FFont::InitCache()
{
    m_fontTexture->CreateGradientSlot();

    // precache (not required but for faster printout later)
    const char first = ' ';
    const char last = '~';
    char buf[last - first + 2];
    char* p = buf;

    // precache all [normal] printable characters to the string (missing ones are updated on demand)
    for (char i = first; i <= last; ++i)
    {
        *p++ = i;
    }
    *p = 0;

    Prepare(buf, false);

    return true;
}

AZ::FFont::FontEffect* AZ::FFont::AddEffect(const char* effectName)
{
    m_effects.push_back(FontEffect(effectName));
    return &m_effects[m_effects.size() - 1];
}

AZ::FFont::FontEffect* AZ::FFont::GetDefaultEffect()
{
    return &m_effects[0];
}

void AZ::FFont::Prepare(const char* str, bool updateTexture, const AtomFont::GlyphSize& glyphSize)
{
    const bool rerenderGlyphs = m_sizeBehavior == SizeBehavior::Rerender;
    const AtomFont::GlyphSize usedGlyphSize = rerenderGlyphs ? glyphSize : AtomFont::defaultGlyphSize;
    bool texUpdateNeeded = m_fontTexture->PreCacheString(str, nullptr, m_sizeRatio, usedGlyphSize, m_fontHintParams) == 1 || m_fontTexDirty;
    if (updateTexture && texUpdateNeeded && m_fontImage)
    {
        UpdateTexture();
        m_fontTexDirty = false;
        ++m_fontImageVersion;

        // Let any listeners know that the font texture has changed
        // TODO Update to an AZ::Event when Cry use of this bus is cleaned out.
        EBUS_EVENT(FontNotificationBus, OnFontTextureUpdated, this);
    }
    else
    {
        m_fontTexDirty = texUpdateNeeded;
    }
}

Vec2 AZ::FFont::GetRestoredFontSize(const TextDrawContext& ctx) const
{
    // Calculate the scale that we need to apply to the text size to ensure
    // it's on-screen size is the same regardless of the slot scaling needed
    // to fit the glyphs of the font within the font texture slots.
    float restoringScale = IFFontConstants::defaultSizeRatio / m_sizeRatio;
    return Vec2(ctx.m_size.x * restoringScale, ctx.m_size.y * restoringScale);
}

void AZ::FFont::ScaleCoord(const RHI::Viewport& viewport, float& x, float& y) const
{
    float width = viewport.m_maxX - viewport.m_minX;
    float height = viewport.m_maxY - viewport.m_minY;

    x *= width / WindowScaleWidth;
    y *= height / WindowScaleHeight;
}

static void SetCommonContextFlags(AZ::TextDrawContext& ctx, const AzFramework::TextDrawParameters& params)
{
        if (params.m_hAlign == AzFramework::TextHorizontalAlignment::Center)
        {
            ctx.m_drawTextFlags |= eDrawText_Center;
        }

        if (params.m_hAlign == AzFramework::TextHorizontalAlignment::Right)
        {
            ctx.m_drawTextFlags |= eDrawText_Right;
        }
        
        if (params.m_vAlign == AzFramework::TextVerticalAlignment::Center)
        {
            ctx.m_drawTextFlags |= eDrawText_CenterV;
        }
        
        if (params.m_vAlign == AzFramework::TextVerticalAlignment::Bottom)
        {
            ctx.m_drawTextFlags |= eDrawText_Bottom;
        }
        
        if (params.m_monospace)
        {
            ctx.m_drawTextFlags |= eDrawText_Monospace;
        }
        
        if (params.m_depthTest)
        {
            ctx.m_drawTextFlags |= eDrawText_DepthTest;
        }
        
        if (params.m_virtual800x600ScreenSize)
        {
            ctx.m_drawTextFlags |= eDrawText_800x600;
        }
        
        if (!params.m_scaleWithWindow)
        {
            ctx.m_drawTextFlags |= eDrawText_FixedSize;
        }

        if (params.m_useTransform)
        {
            ctx.m_drawTextFlags |= eDrawText_UseTransform;
            ctx.SetTransform(AZMatrix3x4ToLYMatrix3x4(params.m_transform));
        }
}

AZ::FFont::DrawParameters AZ::FFont::ExtractDrawParameters(const AzFramework::TextDrawParameters& params, AZStd::string_view text, bool forceCalculateSize)
{
    DrawParameters internalParams;
    if (params.m_drawViewportId == AzFramework::InvalidViewportId ||
        text.empty())
    {
        return internalParams;
    }

    float posX = params.m_position.GetX();
    float posY = params.m_position.GetY();
    internalParams.m_viewportContext = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get()->GetViewportContextById(params.m_drawViewportId);
    const AZ::RHI::Viewport& viewport = internalParams.m_viewportContext->GetWindowContext()->GetViewport();
    internalParams.m_viewport = &viewport;
    if (params.m_virtual800x600ScreenSize)
    {
        posX *= WindowScaleWidth / (viewport.m_maxX - viewport.m_minX);
        posY *= WindowScaleHeight / (viewport.m_maxY - viewport.m_minY);
    }
    internalParams.m_ctx.SetBaseState(GS_NODEPTHTEST);
    internalParams.m_ctx.SetColor(AZColorToLYColorF(params.m_color));
    internalParams.m_ctx.SetEffect(params.m_effectIndex);
    internalParams.m_ctx.SetCharWidthScale((params.m_monospace || params.m_scaleWithWindow) ? 0.5f : 1.0f);
    internalParams.m_ctx.EnableFrame(false);
    internalParams.m_ctx.SetProportional(!params.m_monospace && params.m_scaleWithWindow);
    internalParams.m_ctx.SetSizeIn800x600(params.m_scaleWithWindow && params.m_virtual800x600ScreenSize);
    internalParams.m_ctx.SetSize(AZVec2ToLYVec2(AZ::Vector2(params.m_textSizeFactor, params.m_textSizeFactor) * params.m_scale * internalParams.m_viewportContext->GetDpiScalingFactor()));
    internalParams.m_ctx.SetLineSpacing(params.m_lineSpacing);

    if (params.m_hAlign != AzFramework::TextHorizontalAlignment::Left ||
        params.m_vAlign != AzFramework::TextVerticalAlignment::Top ||
        forceCalculateSize)
    {
        // We align based on the size of the default font effect because we do not want the
        // text to move when the font effect is changed
        unsigned int effectIndex = internalParams.m_ctx.m_fxIdx;
        internalParams.m_ctx.SetEffect(0);
        Vec2 textSize = GetTextSizeUInternal(viewport, text.data(), params.m_multiline, internalParams.m_ctx);
        internalParams.m_ctx.SetEffect(effectIndex);
        
        // If we're using virtual 800x600 coordinates, convert the text size from
        // pixels to that before using it as an offset.
        if (internalParams.m_ctx.m_sizeIn800x600)
        {
            float width = 1.0f;
            float height = 1.0f;
            ScaleCoord(viewport, width, height);
            textSize.x /= width;
            textSize.y /= height;
        }

        if (params.m_hAlign == AzFramework::TextHorizontalAlignment::Center)
        {
            posX -= textSize.x * 0.5f;
        }
        else if (params.m_hAlign == AzFramework::TextHorizontalAlignment::Right)
        {
            posX -= textSize.x;
        }

        if (params.m_vAlign == AzFramework::TextVerticalAlignment::Center)
        {
            posY -= textSize.y * 0.5f;
        }
        else if (params.m_vAlign == AzFramework::TextVerticalAlignment::Bottom)
        {
            posY -= textSize.y;
        }
        internalParams.m_size = AZ::Vector2{textSize.x, textSize.y} * internalParams.m_viewportContext->GetDpiScalingFactor();
    }
    SetCommonContextFlags(internalParams.m_ctx, params);
    internalParams.m_ctx.m_drawTextFlags |= eDrawText_2D;
    internalParams.m_position = AZ::Vector2{posX, posY};
    return internalParams;
}

void AZ::FFont::DrawScreenAlignedText2d(
    const AzFramework::TextDrawParameters& params,
    AZStd::string_view text)
{
    DrawParameters internalParams = ExtractDrawParameters(params, text, false);
    if (!internalParams.m_viewportContext)
    {
        return;
    }

    DrawStringUInternal(
        *internalParams.m_viewport, 
        internalParams.m_viewportContext, 
        internalParams.m_position.GetX(), 
        internalParams.m_position.GetY(), 
        params.m_position.GetZ(), // Z
        text.data(),
        params.m_multiline,
        internalParams.m_ctx
    );
}

void AZ::FFont::DrawScreenAlignedText3d(
    const AzFramework::TextDrawParameters& params,
    AZStd::string_view text)
{
    DrawParameters internalParams = ExtractDrawParameters(params, text, false);
    if (!internalParams.m_viewportContext)
    {
        return;
    }
    AZ::RPI::ViewPtr currentView = internalParams.m_viewportContext->GetDefaultView();
    if (!currentView)
    {
        return;
    }

    const AZ::Vector3 positionNdc = AzFramework::WorldToScreenNdc(
        params.m_position, currentView->GetWorldToViewMatrixAsMatrix3x4(), currentView->GetViewToClipMatrix());

    // Text behind the camera shouldn't get rendered.  WorldToScreenNdc returns values in the range 0 - 1, so Z < 0.5 is behind the screen
    // and >= 0.5 is in front of the screen.
    if (positionNdc.GetZ() < 0.5f)
    {
        return;
    }

    internalParams.m_ctx.m_sizeIn800x600 = false;

    DrawStringUInternal(
        *internalParams.m_viewport, 
        internalParams.m_viewportContext, 
        positionNdc.GetX() * internalParams.m_viewport->GetWidth(), 
        (1.0f - positionNdc.GetY()) * internalParams.m_viewport->GetHeight(), 
        positionNdc.GetZ(), // Z
        text.data(),
        params.m_multiline,
        internalParams.m_ctx
    );
}

AZ::Vector2 AZ::FFont::GetTextSize(const AzFramework::TextDrawParameters& params, AZStd::string_view text)
{
    DrawParameters sizeParams = ExtractDrawParameters(params, text, true);
    return sizeParams.m_size;
}

#endif //USE_NULLFONT_ALWAYS

