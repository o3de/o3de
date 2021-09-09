/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LyShineDebug.h"
#include "IConsole.h"
#include <LyShine/Draw2d.h>

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/ISprite.h>
#include <LyShine/UiSerializeHelpers.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/Bus/UiButtonBus.h>
#include <LyShine/Bus/UiCheckboxBus.h>
#include <LyShine/Bus/UiSliderBus.h>
#include <LyShine/Bus/UiTextInputBus.h>
#include <LyShine/Bus/UiInteractableStatesBus.h>
#include <LyShine/Bus/UiInitializationBus.h>

#include <LyShine/UiComponentTypes.h>

AllocateConstIntCVar(LyShineDebug, CV_r_DebugUIDraw2dFont);
AllocateConstIntCVar(LyShineDebug, CV_r_DebugUIDraw2dImage);
AllocateConstIntCVar(LyShineDebug, CV_r_DebugUIDraw2dLine);
AllocateConstIntCVar(LyShineDebug, CV_r_DebugUIDraw2dDefer);

static const int g_numColors = 8;
[[maybe_unused]] static const char* g_colorNames[g_numColors] =
{
    "white",
    "red",
    "green",
    "blue",
    "yellow",
    "cyan",
    "magenta",
    "black"
};

[[maybe_unused]] static AZ::Vector3 g_colorVec3[g_numColors] =
{
    AZ::Vector3(1.0f, 1.0f, 1.0f),
    AZ::Vector3(1.0f, 0.0f, 0.0f),
    AZ::Vector3(0.0f, 1.0f, 0.0f),
    AZ::Vector3(0.0f, 0.0f, 1.0f),
    AZ::Vector3(1.0f, 1.0f, 0.0f),
    AZ::Vector3(0.0f, 1.0f, 1.0f),
    AZ::Vector3(1.0f, 0.0f, 1.0f),
    AZ::Vector3(0.0f, 0.0f, 0.0f),
};

static const int g_numSrcBlendModes = 11;
[[maybe_unused]] static AZ::RHI::BlendFactor g_srcBlendModes[g_numSrcBlendModes] =
{
    AZ::RHI::BlendFactor::Zero,
    AZ::RHI::BlendFactor::One,
    AZ::RHI::BlendFactor::ColorDest,
    AZ::RHI::BlendFactor::ColorDestInverse,
    AZ::RHI::BlendFactor::AlphaSource,
    AZ::RHI::BlendFactor::AlphaSourceInverse,
    AZ::RHI::BlendFactor::AlphaDest,
    AZ::RHI::BlendFactor::AlphaDestInverse,
    AZ::RHI::BlendFactor::AlphaSourceSaturate,
    AZ::RHI::BlendFactor::Factor,
    AZ::RHI::BlendFactor::AlphaSource1
};

static const int g_numDstBlendModes = 10;
[[maybe_unused]] static AZ::RHI::BlendFactor g_dstBlendModes[g_numDstBlendModes] =
{
    AZ::RHI::BlendFactor::Zero,
    AZ::RHI::BlendFactor::One,
    AZ::RHI::BlendFactor::ColorSource,
    AZ::RHI::BlendFactor::ColorSourceInverse,
    AZ::RHI::BlendFactor::AlphaSource,
    AZ::RHI::BlendFactor::AlphaSourceInverse,
    AZ::RHI::BlendFactor::AlphaDest,
    AZ::RHI::BlendFactor::AlphaDestInverse,
    AZ::RHI::BlendFactor::FactorInverse,
    AZ::RHI::BlendFactor::AlphaSource1Inverse
};

[[maybe_unused]] static bool g_deferDrawsToEndOfFrame = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
// LOCAL STATIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
#ifdef LYSHINE_ATOM_TODO
static int Create2DTexture(int width, int height, byte* data, ETEX_Format format)
{
    IRenderer* renderer = gEnv->pRenderer;
    return renderer->DownLoadToVideoMemory(data, width, height, format, format, 1);
}
#endif
#endif

#if !defined(_RELEASE)
static AZ::Vector2 GetTextureSize(AZ::Data::Instance<AZ::RPI::Image> image)
{
    AZ::RHI::Size size = image->GetDescriptor().m_size;
    return AZ::Vector2(static_cast<float>(size.m_width), static_cast<float>(size.m_height));
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
#ifdef LYSHINE_ATOM_TODO
static void FillTextureRectWithCheckerboard(uint32* data, int textureWidth, int textureHeight,
    int minX, int minY, [[maybe_unused]] int rectWidth, int rectHeight,
    int tileWidth, int tileHeight, uint32* colors, bool varyAlpha)
{
    for (int i = minX; i < minX + rectHeight && i < textureWidth; ++i)
    {
        for (int j = minY; j < minY + rectHeight && j < textureHeight; ++j)
        {
            // if both even then colors[0], if one even & one odd then colors[1], if both odd then colors[2]
            int index = ((i / tileWidth) % 2) + ((j / tileHeight) % 2);

            uint32 color = colors[index];

            if (varyAlpha)
            {
                // across y we fade the alpha from 0 at top to 1 at bottom
                uint32 alpha = (j - minY) * 255 / rectHeight;
                color = (color & 0x00ffffff) | (alpha << 24);
            }

            data[i + j * textureWidth] = color;
        }
    }
}
#endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static AZ::Data::Instance<AZ::RPI::Image> CreateMonoTestTexture()
{
#ifdef LYSHINE_ATOM_TODO
    const int width = 32;
    const int height = 32;
    uint32 data[width * height];

    const uint32 black = 0xff000000;
    const uint32 grey =  0xff7f7f7f;
    const uint32 white = 0xffffffff;

    uint32 colors[3] = {black, grey, white};

    // create a checker board

    // top left quadrant, change every pixel
    FillTextureRectWithCheckerboard(data, width, height, 0, 0, width / 2, height / 2,
        1, 1, colors, false);
    // top right quadrant, change every 2 pixels
    FillTextureRectWithCheckerboard(data, width, height, width / 2, 0, width / 2, height / 2,
        2, 2, colors, false);
    // bottom right quadrant, change every 4 pixels
    FillTextureRectWithCheckerboard(data, width, height, width / 2, height / 2, width / 2, height / 2,
        4, 4, colors, false);
    // bottom left quadrant, change every 8 pixels
    FillTextureRectWithCheckerboard(data, width, height, 0, height / 2, width / 2, height / 2,
        8, 8, colors, false);

    int textureId = Create2DTexture(width, height, (uint8*)data, eTF_R8G8B8A8);

    return gEnv->pRenderer->EF_GetTextureByID(textureId);
#else
    auto whiteTexture = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
    return whiteTexture;
#endif
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static AZ::Data::Instance<AZ::RPI::Image> CreateColorTestTexture()
{
#ifdef LYSHINE_ATOM_TODO
    const int width = 32;
    const int height = 32;
    uint32 data[width * height];

    const uint32 red   = 0xffff0000;
    const uint32 green = 0xff00ff00;
    const uint32 blue  = 0xff0000ff;

    uint32 colors[3] = { red, green, blue };

    // create a checker board

    // top left quadrant, change every pixel
    FillTextureRectWithCheckerboard(data, width, height, 0, 0, width / 2, height / 2,
        1, 1, colors, false);
    // top right quadrant, change every 2 pixels
    FillTextureRectWithCheckerboard(data, width, height, width / 2, 0, width / 2, height / 2,
        2, 2, colors, false);
    // bottom right quadrant, change every 4 pixels
    FillTextureRectWithCheckerboard(data, width, height, width / 2, height / 2, width / 2, height / 2,
        4, 4, colors, false);
    // bottom left quadrant, change every 8 pixels
    FillTextureRectWithCheckerboard(data, width, height, 0, height / 2, width / 2, height / 2,
        8, 8, colors, false);

    int textureId = Create2DTexture(width, height, (uint8*)data, eTF_R8G8B8A8);

    return gEnv->pRenderer->EF_GetTextureByID(textureId);
#else
    auto whiteTexture = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
    return whiteTexture;
#endif
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static AZ::Data::Instance<AZ::RPI::Image> CreateMonoAlphaTestTexture()
{
#ifdef LYSHINE_ATOM_TODO
    const int width = 32;
    const int height = 32;
    uint32 data[width * height];

    const uint32 black = 0xff000000;
    const uint32 grey = 0xff7f7f7f;
    const uint32 white = 0xffffffff;

    uint32 colors[3] = { black, grey, white };

    // create a checker board

    // top left quadrant, change every pixel
    FillTextureRectWithCheckerboard(data, width, height, 0, 0, width / 2, height / 2,
        1, 1, colors, true);
    // top right quadrant, change every 2 pixels
    FillTextureRectWithCheckerboard(data, width, height, width / 2, 0, width / 2, height / 2,
        2, 2, colors, true);
    // bottom right quadrant, change every 4 pixels
    FillTextureRectWithCheckerboard(data, width, height, width / 2, height / 2, width / 2, height / 2,
        4, 4, colors, true);
    // bottom left quadrant, change every 8 pixels
    FillTextureRectWithCheckerboard(data, width, height, 0, height / 2, width / 2, height / 2,
        8, 8, colors, true);

    int textureId = Create2DTexture(width, height, (uint8*)data, eTF_R8G8B8A8);

    return gEnv->pRenderer->EF_GetTextureByID(textureId);
#else
    auto whiteTexture = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
    return whiteTexture;
#endif
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static AZ::Data::Instance<AZ::RPI::Image> CreateColorAlphaTestTexture()
{
#ifdef LYSHINE_ATOM_TODO
    const int width = 32;
    const int height = 32;
    uint32 data[width * height];

    const uint32 red = 0x00ff0000;
    const uint32 green = 0x0000ff00;
    const uint32 blue = 0x000000ff;

    uint32 colors[3] = { red, green, blue };

    // create a checker board

    // top left quadrant, change every pixel
    FillTextureRectWithCheckerboard(data, width, height, 0, 0, width / 2, height / 2,
        1, 1, colors, true);
    // top right quadrant, change every 2 pixels
    FillTextureRectWithCheckerboard(data, width, height, width / 2, 0, width / 2, height / 2,
        2, 2, colors, true);
    // bottom right quadrant, change every 4 pixels
    FillTextureRectWithCheckerboard(data, width, height, width / 2, height / 2, width / 2, height / 2,
        4, 4, colors, true);
    // bottom left quadrant, change every 8 pixels
    FillTextureRectWithCheckerboard(data, width, height, 0, height / 2, width / 2, height / 2,
        8, 8, colors, true);

    int textureId = Create2DTexture(width, height, (uint8*)data, eTF_R8G8B8A8);

    return gEnv->pRenderer->EF_GetTextureByID(textureId);
#else
    auto whiteTexture = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
    return whiteTexture;
#endif
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static AZ::Data::Instance<AZ::RPI::Image> GetMonoTestTexture()
{
    static AZ::Data::Instance<AZ::RPI::Image> testImageMono = nullptr;

    if (!testImageMono)
    {
        testImageMono = CreateMonoTestTexture();
    }

    return testImageMono;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static AZ::Data::Instance<AZ::RPI::Image> GetColorTestTexture()
{
    static AZ::Data::Instance<AZ::RPI::Image> testImageColor = nullptr;

    if (!testImageColor)
    {
        testImageColor = CreateColorTestTexture();
    }

    return testImageColor;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static AZ::Data::Instance<AZ::RPI::Image> GetMonoAlphaTestTexture()
{
    static AZ::Data::Instance<AZ::RPI::Image> testImageMonoAlpha = nullptr;

    if (!testImageMonoAlpha)
    {
        testImageMonoAlpha = CreateMonoAlphaTestTexture();
    }

    return testImageMonoAlpha;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static AZ::Data::Instance<AZ::RPI::Image> GetColorAlphaTestTexture()
{
    static AZ::Data::Instance<AZ::RPI::Image> testImageColorAlpha = nullptr;

    if (!testImageColorAlpha)
    {
        testImageColorAlpha = CreateColorAlphaTestTexture();
    }

    return testImageColorAlpha;
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDrawColoredBox(AZ::Vector2 pos, AZ::Vector2 size, AZ::Color color,
    IDraw2d::HAlign horizontalAlignment = IDraw2d::HAlign::Left,
    IDraw2d::VAlign verticalAlignment = IDraw2d::VAlign::Top)
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    CDraw2d::ImageOptions imageOptions = draw2d->GetDefaultImageOptions();
    imageOptions.color = color.GetAsVector3();
    auto whiteTexture = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
    draw2d->DrawImageAligned(whiteTexture, pos, size, horizontalAlignment, verticalAlignment,
        color.GetA(), 0.0f, nullptr, &imageOptions);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDrawStringWithSizeBox(AZStd::string_view font, unsigned int effectIndex, const char* sizeString,
    const char* testString, AZ::Vector2 pos, float spacing, float size)
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    CDraw2d::TextOptions textOptions = draw2d->GetDefaultTextOptions();
    if (!font.empty())
    {
        textOptions.fontName = font;
    }
    textOptions.effectIndex = effectIndex;

    draw2d->DrawText(sizeString, pos, size, 1.0f, &textOptions);
    AZ::Vector2 sizeTextSize = draw2d->GetTextSize(sizeString, size, &textOptions);
    AZ::Vector2 testTextSize = draw2d->GetTextSize(testString, size, &textOptions);
    AZ::Vector2 pos2(pos.GetX() + sizeTextSize.GetX() + spacing, pos.GetY());
    DebugDrawColoredBox(AZ::Vector2(pos2.GetX() - 1, pos2.GetY() - 1), AZ::Vector2(testTextSize.GetX() + 2, testTextSize.GetY() + 2),
        AZ::Color(0.5f, 0.5f, 0.5f, 1.0f));
    DebugDrawColoredBox(pos2, testTextSize, AZ::Color(0.0f, 0.0f, 0.0f, 1.0f));
    draw2d->DrawText(testString, pos2, size, 1.0f, &textOptions);

    AZ::Vector2 pos3(pos2.GetX() + testTextSize.GetX() + spacing, pos.GetY());
    char buffer[32];
    sprintf_s(buffer, "Pixel height = %5.2f", testTextSize.GetY());
    AZ::Vector2 pixelHeightTextSize = draw2d->GetTextSize(buffer, size, &textOptions);
    // Draw a light background to see if there is a drop shadow
    DebugDrawColoredBox(AZ::Vector2(pos3.GetX() - 1, pos3.GetY() - 1),
        AZ::Vector2(pixelHeightTextSize.GetX() + 2, pixelHeightTextSize.GetY() + 2),
        AZ::Color(0.65f, 0.65f, 0.65f, 1.0f));
    draw2d->DrawText(buffer, pos3, size, 1.0f, &textOptions);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDraw2dFontSizes(AZStd::string_view font, unsigned int effectIndex)
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    float xOffset = 20.0f;
    float yOffset = 20.0f;
    float xSpacing = 20.0f;

    char buffer[32];
    sprintf_s(buffer, "Font = %s, effect = %d", font.data(), effectIndex);
    draw2d->DrawText(buffer, AZ::Vector2(xOffset, yOffset), 32);
    yOffset += 40.0f;
    draw2d->DrawText("NOTE: if the effect includes a drop shadow baked into font then the pixel size",
        AZ::Vector2(xOffset, yOffset), 16);
    draw2d->DrawText("NOTE: The pixel height reported takes no account of the actual characters used.",
        AZ::Vector2(xOffset + draw2d->GetViewportWidth() * 0.5f, yOffset), 16);
    yOffset += 20.0f;
    draw2d->DrawText("will include the drop shadow offset.", AZ::Vector2(xOffset, yOffset), 16);
    yOffset += 20.0f;

    const char* testString = "AbdfhkltgjpqyWw|01!";
    const char* minimalTestString = "ace";

    DebugDrawStringWithSizeBox(font, effectIndex, "Size 16", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 16);
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 16", minimalTestString,
        AZ::Vector2(xOffset + draw2d->GetViewportWidth() * 0.5f, yOffset), xSpacing, 16);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 17", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 17);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 18", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 18);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 23", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 23);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 24", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 24);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 25", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 25);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 30", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 30);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 31", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 31);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 32", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 32);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 33", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 33);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 34", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 34);

    yOffset += 40.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 47", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 47);

    yOffset += 55.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 48", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 48);

    yOffset += 55.0f;
    DebugDrawStringWithSizeBox(font, effectIndex, "Size 49", testString, AZ::Vector2(xOffset, yOffset), xSpacing, 49);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDrawAlignedTextWithOriginBox(AZ::Vector2 pos,
    IDraw2d::HAlign horizontalAlignment,
    IDraw2d::VAlign verticalAlignment, float pointSize, AZ::Vector3 textColor)
{
    Draw2dHelper draw2d(g_deferDrawsToEndOfFrame);

    const char* haStr = 0;
    const char* vaStr = 0;
    switch (horizontalAlignment)
    {
    case IDraw2d::HAlign::Left:
        haStr = "Left";
        break;
    case IDraw2d::HAlign::Center:
        haStr = "Center";
        break;
    case IDraw2d::HAlign::Right:
        haStr = "Right";
        break;
    }
    switch (verticalAlignment)
    {
    case IDraw2d::VAlign::Top:
        vaStr = "Top";
        break;
    case IDraw2d::VAlign::Center:
        vaStr = "Center";
        break;
    case IDraw2d::VAlign::Bottom:
        vaStr = "Bottom";
        break;
    }

    char buffer[64];
    sprintf_s(buffer, "%s %s, size=%5.2f", haStr, vaStr, pointSize);

    AZ::Color backgroundColor(0.3f, 0.3f, 0.3f, 1.0f);
    DebugDrawColoredBox(pos, draw2d.GetTextSize(buffer, pointSize), backgroundColor,
        horizontalAlignment, verticalAlignment);

    AZ::Color boxColor(1.0f, 0.25f, 0.25f, 1.0f);
    DebugDrawColoredBox(AZ::Vector2(pos.GetX() - 2.0f, pos.GetY() - 2.0f), AZ::Vector2(5.0f, 5.0f), boxColor);

    draw2d.SetTextAlignment(horizontalAlignment, verticalAlignment);
    draw2d.SetTextColor(textColor);

    draw2d.DrawText(buffer, pos, pointSize, 1.0f);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDraw2dFontAlignment()
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();
    float w = draw2d->GetViewportWidth();
    float yPos = 20;

    {
        Draw2dHelper draw2dHelper(g_deferDrawsToEndOfFrame);
        draw2dHelper.DrawText(
            "Text Alignment. Red dot is the pos passed to DrawText. Default font, effect 0",
            AZ::Vector2(20, yPos), 16);
        yPos += 20;
    }

    AZ::Vector3 color1(1, 1, 1);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(20, yPos),
        IDraw2d::HAlign::Left, IDraw2d::VAlign::Top, 32, color1);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w / 2, yPos),
        IDraw2d::HAlign::Center, IDraw2d::VAlign::Top, 32, color1);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w - 20, yPos),
        IDraw2d::HAlign::Right, IDraw2d::VAlign::Top, 32, color1);

    yPos += 60;
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(20, yPos),
        IDraw2d::HAlign::Left, IDraw2d::VAlign::Center, 32, color1);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w / 2, yPos),
        IDraw2d::HAlign::Center, IDraw2d::VAlign::Center, 32, color1);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w - 20, yPos),
        IDraw2d::HAlign::Right, IDraw2d::VAlign::Center, 32, color1);

    yPos += 60;
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(20, yPos),
        IDraw2d::HAlign::Left, IDraw2d::VAlign::Bottom, 32, color1);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w / 2, yPos),
        IDraw2d::HAlign::Center, IDraw2d::VAlign::Bottom, 32, color1);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w - 20, yPos),
        IDraw2d::HAlign::Right, IDraw2d::VAlign::Bottom, 32, color1);

    AZ::Vector3 color2(0.25f, 0.5f, 1.0f);
    yPos += 30;
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(20, yPos),
        IDraw2d::HAlign::Left, IDraw2d::VAlign::Top, 24, color2);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w / 2, yPos),
        IDraw2d::HAlign::Center, IDraw2d::VAlign::Top, 24, color2);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w - 20, yPos),
        IDraw2d::HAlign::Right, IDraw2d::VAlign::Top, 24, color2);

    yPos += 50;
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(20, yPos),
        IDraw2d::HAlign::Left, IDraw2d::VAlign::Center, 24, color2);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w / 2, yPos),
        IDraw2d::HAlign::Center, IDraw2d::VAlign::Center, 24, color2);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w - 20, yPos),
        IDraw2d::HAlign::Right, IDraw2d::VAlign::Center, 24, color2);

    yPos += 50;
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(20, yPos),
        IDraw2d::HAlign::Left, IDraw2d::VAlign::Bottom, 24, color2);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w / 2, yPos),
        IDraw2d::HAlign::Center, IDraw2d::VAlign::Bottom, 24, color2);
    DebugDrawAlignedTextWithOriginBox(AZ::Vector2(w - 20, yPos),
        IDraw2d::HAlign::Right, IDraw2d::VAlign::Bottom, 24, color2);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static AZ::Vector2 DebugDrawFontColorTestBox(AZ::Vector2 pos, const char* string, AZ::Vector3 color, float opacity)
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    float pointSize = 32.0f;
    const float spacing = 6.0f;

    CDraw2d::TextOptions textOptions = draw2d->GetDefaultTextOptions();
    textOptions.effectIndex = 1;    // no drop shadow baked in
    textOptions.color = color;

    AZ::Vector2 textSize = draw2d->GetTextSize(string, pointSize, &textOptions);

    AZ::Vector2 totalBackgroundSize(textSize.GetX() + spacing * 2.0f, textSize.GetY() + spacing * 4.0f);
    AZ::Vector2 whiteBackgroundSize(totalBackgroundSize.GetX() * 0.5f, totalBackgroundSize.GetY());
    AZ::Vector2 blackBackgroundSize = whiteBackgroundSize;

    AZ::Vector2 whiteBackgroundPos = pos;
    AZ::Vector2 blackBackgroundPos = pos + AZ::Vector2(whiteBackgroundSize.GetX(), 0);
    AZ::Vector2 textPos = pos + AZ::Vector2(spacing, spacing);
    AZ::Vector2 boxPos = pos + AZ::Vector2(spacing, spacing + textSize.GetY() + spacing);

    DebugDrawColoredBox(whiteBackgroundPos, whiteBackgroundSize, AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));
    DebugDrawColoredBox(blackBackgroundPos, blackBackgroundSize, AZ::Color(0.0f, 0.0f, 0.0f, 1.0f));

    draw2d->DrawText(string, textPos, pointSize, opacity, &textOptions);

    DebugDrawColoredBox(boxPos, AZ::Vector2(textSize.GetX(), spacing), AZ::Color::CreateFromVector3AndFloat(color, opacity));

    return totalBackgroundSize;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDraw2dFontColorAndOpacity()
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    AZ::Vector2 size;
    AZ::Vector2 pos(20.0f, 20.0f);

    for (int color = 0; color < g_numColors; ++color)
    {
        AZ::Vector2 boxPos = pos;

        size = DebugDrawFontColorTestBox(boxPos, g_colorNames[color], g_colorVec3[color], 1.0f);
        boxPos.SetX(boxPos.GetX() + 200.0f);
        size = DebugDrawFontColorTestBox(boxPos, g_colorNames[color], g_colorVec3[color], 0.75f);
        boxPos.SetX(boxPos.GetX() + 200.0f);
        size = DebugDrawFontColorTestBox(boxPos, g_colorNames[color], g_colorVec3[color], 0.5f);
        boxPos.SetX(boxPos.GetX() + 200.0f);
        size = DebugDrawFontColorTestBox(boxPos, g_colorNames[color], g_colorVec3[color], 0.25f);
        boxPos.SetX(boxPos.GetX() + 200.0f);
        size = DebugDrawFontColorTestBox(boxPos, g_colorNames[color], g_colorVec3[color], 0.00f);

        pos.SetY(pos.GetY() + size.GetY() + 10.0f);
    }

    draw2d->DrawText("Opacity=1.00f", pos, 24.0f);
    pos.SetX(pos.GetX() + 200.0f);
    draw2d->DrawText("Opacity=0.75f", pos, 24.0f);
    pos.SetX(pos.GetX() + 200.0f);
    draw2d->DrawText("Opacity=0.50f", pos, 24.0f);
    pos.SetX(pos.GetX() + 200.0f);
    draw2d->DrawText("Opacity=0.25f", pos, 24.0f);
    pos.SetX(pos.GetX() + 200.0f);
    draw2d->DrawText("Opacity=0.00f", pos, 24.0f);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDraw2dImageRotations()
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    AZ::Data::Instance<AZ::RPI::Image> texture = GetMonoTestTexture();

    AZ::Vector2 size = GetTextureSize(texture);

    float row = 20.0f;
    float xSpacing = size.GetX() * 2.0f;
    float xStart = 50.0f;
    AZ::Color posBoxColor(1.0f, 0.25f, 0.25f, 1.0f);
    AZ::Color pivotBoxColor(1.0f, 1.00f, 0.00f, 1.0f);

    draw2d->DrawText("No pivot, rotation about pos (drawn in red), increments of 45 degrees", AZ::Vector2(xStart, row), 16);
    row += 16 + 60.0f;
    for (int i = 0; i < 10; ++i)
    {
        AZ::Vector2 pos(xStart + xSpacing * i, row);
        draw2d->DrawImage(texture, pos, size, 1.0f, 45.0f * i);
        DebugDrawColoredBox(AZ::Vector2(pos.GetX() - 2, pos.GetY() - 2), AZ::Vector2(5, 5), posBoxColor);
    }

    row += 60.0f;
    draw2d->DrawText("Rotation about pivot. Pos drawn in red, pivot is yellow. Increments of 45 degrees",
        AZ::Vector2(xStart, row), 16);
    row += 16 + 40.0f;
    AZ::Vector2 pivotOffset(10, 20);
    for (int i = 0; i < 10; ++i)
    {
        AZ::Vector2 pos(xStart + xSpacing * i, row);
        AZ::Vector2 pivot = pos + pivotOffset;
        draw2d->DrawImage(texture, pos, size, 1.0f, 45.0f * i, &pivot);
        DebugDrawColoredBox(AZ::Vector2(pos.GetX() - 2, pos.GetY() - 2), AZ::Vector2(5, 5), posBoxColor);
        DebugDrawColoredBox(AZ::Vector2(pivot.GetX() - 2, pivot.GetY() - 2), AZ::Vector2(5, 5), pivotBoxColor);
    }

    row += 100.0f;
    draw2d->DrawText("DrawImageAligned (center,center). Pos drawn in red. Increments of 45 degrees",
        AZ::Vector2(xStart, row), 16);
    row += 16 + 30.0f;
    for (int i = 0; i < 10; ++i)
    {
        AZ::Vector2 pos(xStart + xSpacing * i + size.GetX() * 0.5f, row + size.GetY() * 0.5f);
        draw2d->DrawImageAligned(texture, pos, size, IDraw2d::HAlign::Center, IDraw2d::VAlign::Center, 1.0f, 45.0f * i);
        DebugDrawColoredBox(AZ::Vector2(pos.GetX() - 2, pos.GetY() - 2), AZ::Vector2(5, 5), posBoxColor);
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDraw2dImageColor()
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    AZ::Data::Instance<AZ::RPI::Image> texture = GetMonoAlphaTestTexture();

    CDraw2d::ImageOptions imageOptions = draw2d->GetDefaultImageOptions();

    draw2d->DrawText(
        "Testing image colors, image is black and white, top row is opacity=1, bottom row is opacity = 0.5",
        AZ::Vector2(20, 20), 16);

    AZ::Vector2 size = GetTextureSize(texture) * 2.0f;

    float xStart = 20.0f;
    float yStart = 50.0f;
    float xSpacing = size.GetX() + 20.0f;
    float ySpacing = size.GetY() + 20.0f;

    for (int color = 0; color < g_numColors; ++color)
    {
        AZ::Vector2 pos(xStart + xSpacing * color, yStart);

        // Draw the image with this color
        imageOptions.color = g_colorVec3[color];
        draw2d->DrawImage(texture, pos, size, 1.0f, 0.0f, 0, 0, &imageOptions);

        // draw below with half opacity to test combination of color and opacity
        pos.SetY(pos.GetY() + ySpacing);
        draw2d->DrawImage(texture, pos, size, 0.5f, 0.0f, 0, 0, &imageOptions);
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDraw2dImageBlendMode()
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    auto whiteTexture = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);

    AZ::Data::Instance<AZ::RPI::Image> texture = GetColorAlphaTestTexture();

    CDraw2d::ImageOptions imageOptions = draw2d->GetDefaultImageOptions();

    draw2d->DrawText("Testing blend modes, src blend changes across x-axis, dst blend changes across y axis",
        AZ::Vector2(20, 20), 16);

    AZ::Vector2 size = GetTextureSize(texture);
    float width = size.GetX();
    float height = size.GetY();

    float xStart = 20.0f;
    float yStart = 60.0f;
    float xSpacing = width + 2.0f;
    float ySpacing = height + 2.0f;

    for (int srcIndex = 0; srcIndex < g_numSrcBlendModes; ++srcIndex)
    {
        for (int dstIndex = 0; dstIndex < g_numDstBlendModes; ++dstIndex)
        {
            AZ::Vector2 pos(xStart + xSpacing * srcIndex, yStart + ySpacing * dstIndex);

            // first draw a background with varying color and alpha
            CDraw2d::VertexPosColUV verts[4] =
            {
                { // top left
                    AZ::Vector2(pos.GetX(), pos.GetY()),
                    AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
                    AZ::Vector2(0.0f, 0.0f)
                },
                { // top right
                    AZ::Vector2(pos.GetX() + width, pos.GetY()),
                    AZ::Color(0.0f, 1.0f, 0.0f, 1.0f),
                    AZ::Vector2(1.0f, 0.0f)
                },
                { // bottom right
                    AZ::Vector2(pos.GetX() + width, pos.GetY() + height),
                    AZ::Color(1.0f, 1.0f, 1.0f, 0.0f),
                    AZ::Vector2(1.0f, 1.0f)
                },
                { // bottom left
                    AZ::Vector2(pos.GetX(), pos.GetY() + height),
                    AZ::Color(0.0f, 0.0f, 1.0f, 1.0f),
                    AZ::Vector2(0.0f, 1.0f)
                },
            };
            draw2d->DrawQuad(whiteTexture, verts);

            // Draw the image with this color

            CDraw2d::RenderState renderState;
            renderState.m_blendState.m_blendSource = g_srcBlendModes[srcIndex];
            renderState.m_blendState.m_blendDest = g_dstBlendModes[dstIndex];
            draw2d->DrawImage(texture, pos, size, 1.0f, 0.0f, 0, 0, &imageOptions);
        }
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDraw2dImageUVs()
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    AZ::Data::Instance<AZ::RPI::Image> texture = GetColorTestTexture();

    CDraw2d::ImageOptions imageOptions = draw2d->GetDefaultImageOptions();

    draw2d->DrawText(
        "Testing DrawImage with minMaxTexCoords. Full image, top left quadrant, middle section, full flipped",
        AZ::Vector2(20, 20), 16);

    AZ::Vector2 size = GetTextureSize(texture) * 2.0f;

    float xStart = 20.0f;
    float yStart = 50.0f;
    float xSpacing = size.GetX() + 20.0f;

    AZ::Vector2 pos(xStart, yStart);

    AZ::Vector2 minMaxTexCoords[2];

    // full image
    minMaxTexCoords[0] = AZ::Vector2(0, 0);
    minMaxTexCoords[1] = AZ::Vector2(1, 1);
    draw2d->DrawImage(texture, pos, size, 1.0f, 0.0f, 0, minMaxTexCoords);

    // top left quadrant of image
    pos.SetX(pos.GetX() + xSpacing);
    minMaxTexCoords[0] = AZ::Vector2(0, 0);
    minMaxTexCoords[1] = AZ::Vector2(0.5, 0.5);
    draw2d->DrawImage(texture, pos, size, 1.0f, 0.0f, 0, minMaxTexCoords);

    // middle of image
    pos.SetX(pos.GetX() + xSpacing);
    minMaxTexCoords[0] = AZ::Vector2(0.25, 0.25);
    minMaxTexCoords[1] = AZ::Vector2(0.75, 0.75);
    draw2d->DrawImage(texture, pos, size, 1.0f, 0.0f, 0, minMaxTexCoords);

    // flip of image
    pos.SetX(pos.GetX() + xSpacing);
    minMaxTexCoords[0] = AZ::Vector2(0.0, 1.0);
    minMaxTexCoords[1] = AZ::Vector2(1.0, 0.0);
    draw2d->DrawImage(texture, pos, size, 1.0f, 0.0f, 0, minMaxTexCoords);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDraw2dImagePixelRounding()
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    AZ::Data::Instance<AZ::RPI::Image> texture = GetColorTestTexture();

    CDraw2d::ImageOptions imageOptions = draw2d->GetDefaultImageOptions();

    draw2d->DrawText("Testing DrawImage pixel rounding options", AZ::Vector2(20, 20), 16);

    AZ::Vector2 size = GetTextureSize(texture);

    float xStart = 20.0f;
    float yStart = 50.0f;
    float xSpacing = size.GetX() + 4.0f;
    float ySpacing = size.GetY() + 4.0f;

    float offsets[4] = { 0.0f, 0.17f, 0.5f, 0.67f };
    IDraw2d::Rounding roundings[4] = {
        IDraw2d::Rounding::None,
        IDraw2d::Rounding::Nearest,
        IDraw2d::Rounding::Down,
        IDraw2d::Rounding::Up
    };

    for (int i = 0; i < 4; ++i) // loop through pixel offsets (along x axis)
    {
        for (int j = 0; j < 4; ++j) // loop through rounding options (along y axis)
        {
            AZ::Vector2 pos(xStart + xSpacing * i + offsets[i], yStart + ySpacing * j + offsets[i]);

            imageOptions.pixelRounding = roundings[j];

            draw2d->DrawImage(texture, pos, size, 1.0f, 0.0f, 0, 0, &imageOptions);
        }
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
static void DebugDraw2dLineBasic()
{
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();

    CDraw2d::ImageOptions imageOptions = draw2d->GetDefaultImageOptions();

    draw2d->DrawText("Testing DrawLine", AZ::Vector2(20, 20), 16);

    AZ::Vector2 center = AZ::Vector2(draw2d->GetViewportWidth() * 0.5f, draw2d->GetViewportHeight() * 0.5f);

    float offset = 300.0f;

    AZ::Color white(1.0f, 1.0f, 1.0f, 1.0f);

    draw2d->DrawLine(center, center + AZ::Vector2(offset,  0),          AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));
    draw2d->DrawLine(center, center + AZ::Vector2(offset,  offset),     AZ::Color(1.0f, 0.0f, 0.0f, 1.0f));
    draw2d->DrawLine(center, center + AZ::Vector2(0,       offset),     AZ::Color(1.0f, 1.0f, 0.0f, 1.0f));
    draw2d->DrawLine(center, center + AZ::Vector2(-offset, offset),     AZ::Color(0.0f, 1.0f, 0.0f, 1.0f));
    draw2d->DrawLine(center, center + AZ::Vector2(-offset, 0),          AZ::Color(0.0f, 1.0f, 1.0f, 1.0f));
    draw2d->DrawLine(center, center + AZ::Vector2(-offset, -offset),    AZ::Color(0.0f, 0.0f, 1.0f, 1.0f));
    draw2d->DrawLine(center, center + AZ::Vector2(0, -offset),          AZ::Color(1.0f, 0.0f, 1.0f, 1.0f));
    draw2d->DrawLine(center, center + AZ::Vector2(offset, -offset),     AZ::Color(0.0f, 0.0f, 0.0f, 1.0f));
}
#endif

static AZ::EntityId g_testCanvasId;

static void CreateComponent(AZ::Entity* entity, const AZ::Uuid& componentTypeId)
{
    entity->Deactivate();
    entity->CreateComponent(componentTypeId);
    entity->Activate();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static AZ::Entity* CreateButton(const char* name, bool atRoot, AZ::EntityId parent,
    UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
    const char* text, AZ::Color baseColor, AZ::Color selectedColor, AZ::Color pressedColor, AZ::Color textColor)
{
    AZ::Entity* buttonElem = nullptr;
    if (atRoot)
    {
        EBUS_EVENT_ID_RESULT(buttonElem, parent, UiCanvasBus, CreateChildElement, name);
    }
    else
    {
        EBUS_EVENT_ID_RESULT(buttonElem, parent, UiElementBus, CreateChildElement, name);
    }

    {
        AZ::EntityId buttonId = buttonElem->GetId();
        // create components for button elem
        CreateComponent(buttonElem, LyShine::UiTransform2dComponentUuid);
        CreateComponent(buttonElem, LyShine::UiImageComponentUuid);
        CreateComponent(buttonElem, LyShine::UiButtonComponentUuid);

        AZ_Assert(UiTransform2dBus::FindFirstHandler(buttonId), "Transform2d component missing");

        EBUS_EVENT_ID(buttonId, UiTransform2dBus, SetAnchors, anchors, false, false);
        EBUS_EVENT_ID(buttonId, UiTransform2dBus, SetOffsets, offsets);
        EBUS_EVENT_ID(buttonId, UiImageBus, SetColor, baseColor);

        EBUS_EVENT_ID(buttonId, UiInteractableStatesBus, SetStateColor, UiInteractableStatesInterface::StateHover, buttonId, selectedColor);
        EBUS_EVENT_ID(buttonId, UiInteractableStatesBus, SetStateAlpha, UiInteractableStatesInterface::StateHover, buttonId,  selectedColor.GetA());
        EBUS_EVENT_ID(buttonId, UiInteractableStatesBus, SetStateColor, UiInteractableStatesInterface::StatePressed, buttonId, pressedColor);
        EBUS_EVENT_ID(buttonId, UiInteractableStatesBus, SetStateAlpha, UiInteractableStatesInterface::StatePressed, buttonId, pressedColor.GetA());

        AZStd::string pathname = "Textures/Basic/Button_Sliced_Normal.sprite";
        ISprite* sprite = gEnv->pLyShine->LoadSprite(pathname);

        EBUS_EVENT_ID(buttonId, UiImageBus, SetSprite, sprite);
        EBUS_EVENT_ID(buttonId, UiImageBus, SetImageType, UiImageInterface::ImageType::Sliced);
    }

    {
        // create child text element for the button
        AZ::Entity* textElem = nullptr;
        EBUS_EVENT_ID_RESULT(textElem, buttonElem->GetId(), UiElementBus, CreateChildElement, "ButtonText");
        AZ::EntityId textId = textElem->GetId();

        CreateComponent(textElem, LyShine::UiTransform2dComponentUuid);
        CreateComponent(textElem, LyShine::UiTextComponentUuid);

        AZ_Assert(UiTransform2dBus::FindFirstHandler(textId), "Transform component missing");

        EBUS_EVENT_ID(textId, UiTransform2dBus, SetAnchors, UiTransform2dInterface::Anchors(0.5, 0.5, 0.5, 0.5), false, false);
        EBUS_EVENT_ID(textId, UiTransform2dBus, SetOffsets, UiTransform2dInterface::Offsets(0, 0, 0, 0));

        EBUS_EVENT_ID(textId, UiTextBus, SetText, text);
        EBUS_EVENT_ID(textId, UiTextBus, SetTextAlignment, IDraw2d::HAlign::Center, IDraw2d::VAlign::Center);
        EBUS_EVENT_ID(textId, UiTextBus, SetColor, textColor);
        EBUS_EVENT_ID(textId, UiTextBus, SetFontSize, 24.0f);
    }

    return buttonElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static AZ::Entity* CreateText(const char* name, bool atRoot, AZ::EntityId parent,
    UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
    const char* text, AZ::Color textColor, IDraw2d::HAlign hAlign, IDraw2d::VAlign vAlign)
{
    // create a text
    AZ::Entity* textElem = nullptr;
    if (atRoot)
    {
        EBUS_EVENT_ID_RESULT(textElem, parent, UiCanvasBus, CreateChildElement, name);
    }
    else
    {
        EBUS_EVENT_ID_RESULT(textElem, parent, UiElementBus, CreateChildElement, name);
    }
    AZ_Assert(textElem, "Failed to create child element for text");
    AZ::EntityId textId = textElem->GetId();

    CreateComponent(textElem, LyShine::UiTransform2dComponentUuid);
    CreateComponent(textElem, LyShine::UiTextComponentUuid);

    AZ_Assert(UiTransform2dBus::FindFirstHandler(textId), "Transform component missing");

    EBUS_EVENT_ID(textId, UiTransform2dBus, SetAnchors, anchors, false, false);
    EBUS_EVENT_ID(textId, UiTransform2dBus, SetOffsets, offsets);

    EBUS_EVENT_ID(textId, UiTextBus, SetText, text);
    EBUS_EVENT_ID(textId, UiTextBus, SetTextAlignment, hAlign, vAlign);
    EBUS_EVENT_ID(textId, UiTextBus, SetColor, textColor);

    return textElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static AZ::Entity* CreateTextInput(const char* name, bool atRoot, AZ::EntityId parent,
    UiTransform2dInterface::Anchors anchors, UiTransform2dInterface::Offsets offsets,
    const char* text, const char* placeHolderText,
    AZ::Color baseColor, AZ::Color selectedColor, AZ::Color pressedColor,
    AZ::Color textColor, AZ::Color placeHolderColor)
{
    AZ::Entity* textInputElem = nullptr;
    if (atRoot)
    {
        EBUS_EVENT_ID_RESULT(textInputElem, parent, UiCanvasBus, CreateChildElement, name);
    }
    else
    {
        EBUS_EVENT_ID_RESULT(textInputElem, parent, UiElementBus, CreateChildElement, name);
    }

    {
        AZ::EntityId textInputId = textInputElem->GetId();
        // create components for text input element
        CreateComponent(textInputElem, LyShine::UiTransform2dComponentUuid);
        CreateComponent(textInputElem, LyShine::UiImageComponentUuid);
        CreateComponent(textInputElem, LyShine::UiTextInputComponentUuid);

        AZ_Assert(UiTransform2dBus::FindFirstHandler(textInputId), "Transform2d component missing");

        EBUS_EVENT_ID(textInputId, UiTransform2dBus, SetAnchors, anchors, false, false);
        EBUS_EVENT_ID(textInputId, UiTransform2dBus, SetOffsets, offsets);
        EBUS_EVENT_ID(textInputId, UiImageBus, SetColor, baseColor);

        EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateColor, UiInteractableStatesInterface::StateHover, textInputId, selectedColor);
        EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateAlpha, UiInteractableStatesInterface::StateHover, textInputId, selectedColor.GetA());

        EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateColor, UiInteractableStatesInterface::StatePressed, textInputId, pressedColor);
        EBUS_EVENT_ID(textInputId, UiInteractableStatesBus, SetStateAlpha, UiInteractableStatesInterface::StatePressed, textInputId, pressedColor.GetA());

        AZStd::string pathname = "Textures/Basic/Button_Sliced_Normal.sprite";
        ISprite* sprite = gEnv->pLyShine->LoadSprite(pathname);

        EBUS_EVENT_ID(textInputId, UiImageBus, SetSprite, sprite);
        EBUS_EVENT_ID(textInputId, UiImageBus, SetImageType, UiImageInterface::ImageType::Sliced);
    }

    // create child text element
    AZ::Entity* textElem = CreateText("Text", false, textInputElem->GetId(),
            UiTransform2dInterface::Anchors(0.0f, 0.0f, 1.0f, 1.0f),
            UiTransform2dInterface::Offsets(5.0f, 5.0f, -5.0f, -5.00f),
            text, textColor, IDraw2d::HAlign::Center, IDraw2d::VAlign::Center);

    // reduce the font size
    EBUS_EVENT_ID(textElem->GetId(), UiTextBus, SetFontSize, 24.0f);

    // now link the textInputComponent to the child text entity
    EBUS_EVENT_ID(textInputElem->GetId(), UiTextInputBus, SetTextEntity, textElem->GetId());

    // create child placeholder text element
    AZ::Entity* placeHolderElem = CreateText("PlaceholderText", false, textInputElem->GetId(),
            UiTransform2dInterface::Anchors(0.0f, 0.0f, 1.0f, 1.0f),
            UiTransform2dInterface::Offsets(5.0f, 5.0f, -5.0f, -5.00f),
            placeHolderText, placeHolderColor, IDraw2d::HAlign::Center, IDraw2d::VAlign::Center);

    // reduce the font size
    EBUS_EVENT_ID(placeHolderElem->GetId(), UiTextBus, SetFontSize, 24.0f);

    // now link the textInputComponent to the child placeholder text entity
    EBUS_EVENT_ID(textInputElem->GetId(), UiTextInputBus, SetPlaceHolderTextEntity, placeHolderElem->GetId());

    // Trigger all InGamePostActivate
    EBUS_EVENT_ID(textInputElem->GetId(), UiInitializationBus, InGamePostActivate);
    EBUS_EVENT_ID(textElem->GetId(), UiInitializationBus, InGamePostActivate);
    EBUS_EVENT_ID(placeHolderElem->GetId(), UiInitializationBus, InGamePostActivate);

    return textInputElem;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Very simple test class implementing IUiCanvasActionListener
// This just calls the callback passed into the constructor
namespace
{
    class ActionListener
        : public UiCanvasNotificationBus::Handler
    {
    public:
        ActionListener(AZ::EntityId canvasId, LyShine::ActionName actionName, AZStd::function<void(void)> fn)
            : m_canvasId(canvasId)
            , m_actionName(actionName)
            , m_fn(fn)
        {
            UiCanvasNotificationBus::Handler::BusConnect(m_canvasId);
        }

        ~ActionListener()
        {
            Unregister();
        }

        void OnAction([[maybe_unused]] AZ::EntityId canvasId, const LyShine::ActionName& actionName) override
        {
            if (actionName == m_actionName)
            {
                m_fn();
            }
        }

        void Unregister()
        {
            if (m_canvasId.IsValid())
            {
                UiCanvasNotificationBus::Handler::BusDisconnect(m_canvasId);
            }
        }
    private:
        AZ::EntityId m_canvasId;
        LyShine::ActionName m_actionName;
        AZStd::function<void(void)> m_fn;
    };
}

static ActionListener* g_testActionListener1 = nullptr;
static ActionListener* g_testActionListener2 = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////
static void DestroyTestCanvas()
{
    if (g_testCanvasId.IsValid())
    {
        delete g_testActionListener1;
        g_testActionListener1 = nullptr;

        delete g_testActionListener2;
        g_testActionListener2 = nullptr;

        gEnv->pLyShine->ReleaseCanvas(g_testCanvasId, false);
        g_testCanvasId.SetInvalid();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void TestCanvasCreate ([[maybe_unused]] IConsoleCmdArgs* Cmd)
{
    if (gEnv->IsEditing())
    {
        return;
    }

    const AZ::Color grey(.3f, .3f, .4f, .5f);
    const AZ::Color blue(.2f, .3f, 1.f, 1.f);
    const AZ::Color red(1.f, .1f, .1f, 1.f);
    const AZ::Color pink(1.f, .5f, .5f, 1.f);
    const AZ::Color white(1.f, 1.f, 1.f, 1.f);
    const AZ::Color yellow(1.f, 1.f, 0.f, 1.f);

    // remove the existing test canvas if it exists
    DestroyTestCanvas();

    // test creation of canvas and some simple elements
    AZ::EntityId canvasEntityId = gEnv->pLyShine->CreateCanvas();
    UiCanvasInterface* canvas = UiCanvasBus::FindFirstHandler(canvasEntityId);
    if (!canvas)
    {
        return;
    }
    g_testCanvasId = canvasEntityId;

    // create an image to be the menu background
    AZ::Entity* pauseMenuElem = canvas->CreateChildElement("Menu1");
    CreateComponent(pauseMenuElem, LyShine::UiTransform2dComponentUuid);
    CreateComponent(pauseMenuElem, LyShine::UiImageComponentUuid);
    AZ::EntityId pauseMenuId = pauseMenuElem->GetId();

    AZ_Assert(UiTransform2dBus::FindFirstHandler(pauseMenuId), "Transform component missing");

    EBUS_EVENT_ID(pauseMenuId, UiTransform2dBus, SetAnchors, UiTransform2dInterface::Anchors(0.25, 0.25, 0.75, 0.75), false, false);
    EBUS_EVENT_ID(pauseMenuId, UiTransform2dBus, SetOffsets, UiTransform2dInterface::Offsets(0, 0, 0, 0));
    EBUS_EVENT_ID(pauseMenuId, UiImageBus, SetColor, grey);

    // create a title, centered at top of menu
    CreateText("Heading", false, pauseMenuId, UiTransform2dInterface::Anchors(0.5f, 0.0f, 0.5f, 0.0f), UiTransform2dInterface::Offsets(0, 20, 0, 50),
        "Pause Menu", yellow, IDraw2d::HAlign::Center, IDraw2d::VAlign::Top);

    // create a test text at left top
    CreateText("TopLeftText", false, pauseMenuId, UiTransform2dInterface::Anchors(0.0f, 0.0f, 0.0f, 0.0f), UiTransform2dInterface::Offsets(0, 0, 50, 50),
        "LT", yellow, IDraw2d::HAlign::Left, IDraw2d::VAlign::Top);
    // create a test text at left center
    CreateText("CenterLeftText", false, pauseMenuId, UiTransform2dInterface::Anchors(0.0f, 0.5f, 0.0f, 0.5f), UiTransform2dInterface::Offsets(0, -25, 50, 25),
        "LC", yellow, IDraw2d::HAlign::Left, IDraw2d::VAlign::Center);
    // create a test text at left bottom
    CreateText("BottomLeftText", false, pauseMenuId, UiTransform2dInterface::Anchors(0.0f, 1.0f, 0.0f, 1.0f), UiTransform2dInterface::Offsets(0, -50, 50, 0),
        "LB", yellow, IDraw2d::HAlign::Left, IDraw2d::VAlign::Bottom);
    // create a test text at center bottom
    CreateText("BottomCenterText", false, pauseMenuId, UiTransform2dInterface::Anchors(0.5f, 1.0f, 0.5f, 1.0f), UiTransform2dInterface::Offsets(-25, -50, 25, 0),
        "CB", yellow, IDraw2d::HAlign::Center, IDraw2d::VAlign::Bottom);
    // create a test text at center bottom
    CreateText("CenterRightText", false, pauseMenuId, UiTransform2dInterface::Anchors(1.0f, 1.0f, 1.0f, 1.0f), UiTransform2dInterface::Offsets(-50, -50, 0, 0),
        "CR", yellow, IDraw2d::HAlign::Right, IDraw2d::VAlign::Bottom);


    // Create a "Show Image" button
    [[maybe_unused]] AZ::Entity* showImageButtonElem = CreateButton("ShowImage", false, pauseMenuId,
            UiTransform2dInterface::Anchors(0.5, 0.5, 0.5, 0.5), UiTransform2dInterface::Offsets(-120.0f, -25.0f, 120.0f, 25.0f),
            "Show Image", blue, pink, red, white);

    // Create a "Hide Image" button
    CreateButton("HideImage", false, pauseMenuId,
        UiTransform2dInterface::Anchors(0.5f, 1.0f, 0.5f, 1.0f), UiTransform2dInterface::Offsets(-120.0f, -100.0f, 120.0f, -50.0f),
        "Hide Image", blue, pink, red, white);

    // Create a "Enter name" text input element
    AZ::Color colGreenYellow(0.678f, 1.000f, 0.184f, 1.0f);
    AZ::Entity* textInputElem = CreateTextInput("EnterName", false, pauseMenuId,
            UiTransform2dInterface::Anchors(0.5f, 0.0f, 0.5f, 0.0f), UiTransform2dInterface::Offsets(-120.0f, 70.0f, 120.0f, 120.0f),
            "", "Enter Name", blue, pink, red, white, colGreenYellow);

    // Create an image that these buttons will show/hide
    // create an image to be the menu background
    AZ::Entity* testImageElem = canvas->CreateChildElement("TestImage");
    CreateComponent(testImageElem, LyShine::UiTransform2dComponentUuid);
    CreateComponent(testImageElem, LyShine::UiImageComponentUuid);
    EBUS_EVENT_ID(testImageElem->GetId(), UiTransform2dBus, SetAnchors, UiTransform2dInterface::Anchors(0.78f, 0.25f, 0.95f, 0.75f), false, false);
    EBUS_EVENT_ID(testImageElem->GetId(), UiTransform2dBus, SetOffsets, UiTransform2dInterface::Offsets(0.0f, 0.0f, 0.0f, 0.0f));
    EBUS_EVENT_ID(testImageElem->GetId(), UiImageBus, SetColor, yellow);

    // create some text items that the textInputItem will edit,
    AZ::Color colGreen(0.000f, 0.502f, 0.000f, 1.0f);
    AZ::Entity* changedTextElem = CreateText("ChangedText", true, canvasEntityId, UiTransform2dInterface::Anchors(0.8f, 0.30f, 0.93f, 0.30f), UiTransform2dInterface::Offsets(0.0f, 0.0f, 0.0f, 50.0f),
            "Changed Text", colGreen, IDraw2d::HAlign::Center, IDraw2d::VAlign::Top);
    AZ::Entity* editedTextElem = CreateText("EditedText", true, canvasEntityId, UiTransform2dInterface::Anchors(0.8f, 0.40f, 0.93f, 0.40f), UiTransform2dInterface::Offsets(0.0f, 0.0f, 0.0f, 50.0f),
            "Edited Text", colGreen, IDraw2d::HAlign::Center, IDraw2d::VAlign::Top);
    AZ::Entity* enteredTextElem = CreateText("EnteredText", true, canvasEntityId, UiTransform2dInterface::Anchors(0.8f, 0.50f, 0.93f, 0.50f), UiTransform2dInterface::Offsets(0.0f, 0.0f, 0.0f, 50.0f),
            "Entered Text", colGreen, IDraw2d::HAlign::Center, IDraw2d::VAlign::Top);

    // now setup on-click callbacks to hide and show the menus, use the various ways of doing it

    // First button uses a simple callback
    AZ::Entity* buttonElem = nullptr;
    EBUS_EVENT_ID_RESULT(buttonElem, pauseMenuId, UiElementBus, FindDescendantByName, "ShowImage");
    auto setEnabledCallbackFn = [testImageElem]([[maybe_unused]] AZ::EntityId clickedEntityId, [[maybe_unused]] AZ::Vector2 point)
        {
            EBUS_EVENT_ID(testImageElem->GetId(), UiElementBus, SetIsEnabled, true);
        };
    EBUS_EVENT_ID(buttonElem->GetId(), UiButtonBus, SetOnClickCallback, setEnabledCallbackFn);

    // Second button uses an ActionListener
    EBUS_EVENT_ID_RESULT(buttonElem, pauseMenuId, UiElementBus, FindDescendantByName, "HideImage");
    LyShine::ActionName actionName1("ShowImage");
    auto setDisabledActionFn = [testImageElem](void)
        {
            EBUS_EVENT_ID(testImageElem->GetId(), UiElementBus, SetIsEnabled, false);
        };
    g_testActionListener1 = new ActionListener(canvasEntityId, actionName1, setDisabledActionFn);
    EBUS_EVENT_ID(buttonElem->GetId(), UiButtonBus, SetOnClickActionName, actionName1);

    // Setup callbacks for the text input field
    auto setChangedTextFn = [changedTextElem]([[maybe_unused]] AZ::EntityId textInputEntityId, LyShine::StringType textString)
        {
            EBUS_EVENT_ID(changedTextElem->GetId(), UiTextBus, SetText, textString);
        };
    auto setEditedTextFn = [editedTextElem]([[maybe_unused]] AZ::EntityId textInputEntityId, LyShine::StringType textString)
        {
            EBUS_EVENT_ID(editedTextElem->GetId(), UiTextBus, SetText, textString);
        };
    auto setEnteredTextFn = [enteredTextElem]([[maybe_unused]] AZ::EntityId textInputEntityId, LyShine::StringType textString)
        {
            EBUS_EVENT_ID(enteredTextElem->GetId(), UiTextBus, SetText, textString);
        };
    EBUS_EVENT_ID(textInputElem->GetId(), UiTextInputBus, SetOnChangeCallback, setChangedTextFn);
    EBUS_EVENT_ID(textInputElem->GetId(), UiTextInputBus, SetOnEndEditCallback, setEditedTextFn);
    EBUS_EVENT_ID(textInputElem->GetId(), UiTextInputBus, SetOnEnterCallback, setEnteredTextFn);

    // test clone feature by cloning the whole pause menu
    AZ::Entity* clonedMenuElem = nullptr;
    EBUS_EVENT_ID_RESULT(clonedMenuElem, canvasEntityId, UiCanvasBus, CloneElement, pauseMenuElem, nullptr);
    AZ::EntityId clonedMenuId = clonedMenuElem->GetId();
    EBUS_EVENT_ID(clonedMenuId, UiTransform2dBus, SetAnchors, UiTransform2dInterface::Anchors(0.0f, 0.25f, 0.23f, 0.75f), false, false);
    EBUS_EVENT_ID(clonedMenuId, UiImageBus, SetColor, grey);

    // The clone will copy the action name on the Hide button but not the callback on the
    // Show button, so set that up on the cloned menu
    buttonElem = nullptr;
    EBUS_EVENT_ID_RESULT(buttonElem, clonedMenuId, UiElementBus, FindDescendantByName, "ShowImage");
    EBUS_EVENT_ID(buttonElem->GetId(), UiButtonBus, SetOnClickCallback, setEnabledCallbackFn);

    //! test GUIDs
    LyShine::ElementId id = 0;
    EBUS_EVENT_ID_RESULT(id, buttonElem->GetId(), UiElementBus, GetElementId);
    AZ::Entity* foundElem = canvas->FindElementById(id);
    AZ_Assert(foundElem == buttonElem, "FindElementById failed");

    //! test find by name
    EBUS_EVENT_ID_RESULT(foundElem, canvasEntityId, UiCanvasBus, FindElementByName, "ChangedText");
    AZ_Assert(foundElem == changedTextElem, "FindElementByName failed");

    LyShine::EntityArray foundElements;
    EBUS_EVENT_ID(canvasEntityId, UiCanvasBus, FindElementsByName, "ShowImage", foundElements);
    AZ_Assert(foundElements.size() == 2, "FindElementsByName failed, should find 2 elements");

    EBUS_EVENT_ID_RESULT(foundElem, canvasEntityId, UiCanvasBus, FindElementByHierarchicalName, "Menu1/ShowImage");
    AZ_Assert(foundElem == showImageButtonElem, "FindElementByHierarchicalName failed to find Menu1/ShowImage");

    EBUS_EVENT_ID_RESULT(foundElem, canvasEntityId, UiCanvasBus, FindElementByHierarchicalName, "/Menu1/ShowImage");
    AZ_Assert(foundElem == showImageButtonElem, "FindElementByHierarchicalName failed to find /Menu1/ShowImage");

    EBUS_EVENT_ID_RESULT(foundElem, canvasEntityId, UiCanvasBus, FindElementByHierarchicalName, "Menu1/ShowImage/ButtonText");
    AZ_Assert(foundElem, "FindElementByHierarchicalName failed to find Menu1/ShowImage/ButtonText");

    EBUS_EVENT_ID_RESULT(foundElem, canvasEntityId, UiCanvasBus, FindElementByHierarchicalName, "Menu1/ShowImage/ButtonText/");
    AZ_Assert(!foundElem, "FindElementByHierarchicalName succeeded with bad path");

    EBUS_EVENT_ID_RESULT(foundElem, canvasEntityId, UiCanvasBus, FindElementByHierarchicalName, "ShowImage");
    AZ_Assert(!foundElem, "FindElementByHierarchicalName found ShowImage when it should not");
}


static void TestCanvasRemove([[maybe_unused]] IConsoleCmdArgs* Cmd)
{
    if (gEnv->IsEditing())
    {
        return;
    }

    // remove the existing test canvas if it exists
    DestroyTestCanvas();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void LyShineDebug::Initialize()
{
#ifndef EXCLUDE_DOCUMENTATION_PURPOSE
    DefineConstIntCVar3("r_DebugUIDraw2dFont", CV_r_DebugUIDraw2dFont, 0, VF_CHEAT,
        "0=off, 1=display various features of the UI font rendering to verify function and to document usage");

    DefineConstIntCVar3("r_DebugUIDraw2dImage", CV_r_DebugUIDraw2dImage, 0, VF_CHEAT,
        "0=off, 1=display various features of the UI image rendering to verify function and to document usage");

    DefineConstIntCVar3("r_DebugUIDraw2dLine", CV_r_DebugUIDraw2dLine, 0, VF_CHEAT,
        "0=off, 1=display various features of the UI line rendering to verify function and to document usage");

    DefineConstIntCVar3("r_DebugUIDraw2dDefer", CV_r_DebugUIDraw2dDefer, 0, VF_CHEAT,
        "0=draws 2D immediately in debug tests, 1=defers calls in debug tests");

    REGISTER_COMMAND("ui_TestCanvasCreate", &TestCanvasCreate, VF_NULL, "");
    REGISTER_COMMAND("ui_TestCanvasRemove", &TestCanvasRemove, VF_NULL, "");
#endif // EXCLUDE_DOCUMENTATION_PURPOSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LyShineDebug::Reset()
{
    // remove the existing test canvas if it exists
    DestroyTestCanvas();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LyShineDebug::RenderDebug()
{
#if !defined(_RELEASE)

#ifndef EXCLUDE_DOCUMENTATION_PURPOSE
    CDraw2d* draw2d = Draw2dHelper::GetDefaultDraw2d();
    if (!draw2d)
    {
        return;
    }

    g_deferDrawsToEndOfFrame = (CV_r_DebugUIDraw2dDefer) ? true : false;

    // Set whether to defer draws or render immediately during scope of this helper
    Draw2dHelper draw2dHelper(g_deferDrawsToEndOfFrame);

    if (CV_r_DebugUIDraw2dFont)
    {
        switch (CV_r_DebugUIDraw2dFont)
        {
        case 1:     // test font sizes (default font, effect 0)
            DebugDraw2dFontSizes("default", 0);
            break;
        case 2:     // test font sizes (default font, effect 1)
            DebugDraw2dFontSizes("default", 1);
            break;
        case 3:     // test font alignment
            DebugDraw2dFontAlignment();
            break;
        case 4:     // test font color and opacity
            DebugDraw2dFontColorAndOpacity();
            break;
        }
    }

    if (CV_r_DebugUIDraw2dImage)
    {
        switch (CV_r_DebugUIDraw2dImage)
        {
        case 1:     // test image rotation
            DebugDraw2dImageRotations();
            break;
        case 2:     // test image color
            DebugDraw2dImageColor();
            break;
        case 3:     // test image blend mode
            DebugDraw2dImageBlendMode();
            break;
        case 4:     // test image UVs
            DebugDraw2dImageUVs();
            break;
        case 5:     // test image pixel rounding
            DebugDraw2dImagePixelRounding();
            break;
        }
    }

    if (CV_r_DebugUIDraw2dLine)
    {
        switch (CV_r_DebugUIDraw2dLine)
        {
        case 1:     // test basic draw line
            DebugDraw2dLineBasic();
            break;
        }
    }

#endif

#endif
}
