/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiImageComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/UiSerializeHelpers.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>
#include <LyShine/Bus/UiLayoutControllerBus.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>
#include <LyShine/Bus/UiIndexableImageBus.h>
#include <LyShine/ISprite.h>
#include <LyShine/IRenderGraph.h>

#include "UiSerialize.h"
#include "UiLayoutHelpers.h"
#include "Sprite.h"
#include "RenderGraph.h"

namespace
{
    //! Given a sprite with a cell index, populates the UV/ST coords array for traditional (stretched) 9-sliced image types.
    //!
    //! It's assumed that the given left/right/top/bottom border values have
    //! already had a "correction scaling" applied. This scaling gets applied
    //! when the left/right and/or top/bottom border sizes are greater than
    //! the image element's width or height (respectively).
    //!
    //! \param sValues Output array of U/S texture coords for 9-sliced image.
    //!
    //! \param tValues Output array of V/T texture coords for 9-sliced image.
    //!
    //! \param sprite Pointer to sprite  object info.
    //!
    //! \param cellIndex Index of a particular cell in sprite-sheet, if applicable.
    //!
    //! \param leftBorder The "left" border info for the cell, with "correction scaling" applied.
    //! \param rightBorder One minus the "right" border info for the cell, with "correction scaling" applied.
    //!
    //! \param topBorder Same as leftBorder, but for top cell border value.
    //! \param bottomBorder Same as rightBorder, but for bottom cell border value.
    void GetSlicedStValuesFromCorrectionalScaleBorders(
        float sValues[4],
        float tValues[4],
        const ISprite* sprite,
        const int cellIndex,
        const float leftBorder,
        const float rightBorder,
        const float topBorder,
        const float bottomBorder)
    {
        const float cellMinUCoord = sprite->GetCellUvCoords(cellIndex).TopLeft().GetX();
        const float cellMaxUCoord = sprite->GetCellUvCoords(cellIndex).TopRight().GetX();
        const float cellMinVCoord = sprite->GetCellUvCoords(cellIndex).TopLeft().GetY();
        const float cellMaxVCoord = sprite->GetCellUvCoords(cellIndex).BottomLeft().GetY();

        // Transform border values from cell space to texture space
        const AZ::Vector2 cellUvSize = sprite->GetCellUvSize(cellIndex);
        const float leftBorderTextureSpace = leftBorder * cellUvSize.GetX();
        const float rightBorderTextureSpace = (1.0f - rightBorder) * cellUvSize.GetX();
        const float topBorderTextureSpace = topBorder * cellUvSize.GetY();
        const float bottomBorderTextureSpace = (1.0f - bottomBorder) * cellUvSize.GetY();

        // The texture coords are just based on the border values
        int uIndex = 0;
        sValues[uIndex++] = cellMinUCoord;
        sValues[uIndex++] = cellMinUCoord + leftBorderTextureSpace;
        sValues[uIndex++] = cellMinUCoord + rightBorderTextureSpace;
        sValues[uIndex++] = cellMaxUCoord;

        int vIndex = 0;
        tValues[vIndex++] = cellMinVCoord;
        tValues[vIndex++] = cellMinVCoord + topBorderTextureSpace;
        tValues[vIndex++] = cellMinVCoord + bottomBorderTextureSpace;
        tValues[vIndex++] = cellMaxVCoord;
    }

    //! Given a sprite with a cell index, populates the UV/ST coords array for "fixed" 9-sliced image types.
    //!
    //! It's assumed that the given left/right/top/bottom border values have
    //! already had a "correction scaling" applied. This scaling gets applied
    //! when the left/right and/or top/bottom border sizes are greater than
    //! the image element's width or height (respectively).
    //!
    //! \param sValues Output array of U/S texture coords for 9-sliced image.
    //!
    //! \param tValues Output array of V/T texture coords for 9-sliced image.
    //!
    //! \param sprite Pointer to sprite  object info.
    //!
    //! \param cellIndex Index of a particular cell in sprite-sheet, if applicable.
    //!
    //! \param leftBorder The "left" border info for the cell, with "correction scaling" applied.
    //! \param rightBorder One minus the "right" border info for the cell, with "correction scaling" applied.
    //!
    //! \param topBorder Same as leftBorder, but for top cell border value.
    //! \param bottomBorder Same as rightBorder, but for bottom cell border value.
    //!
    //! \param pivot The pivot value for the element, this controls how the central part of the texture moves as the element resizes
    //!
    //! \param centerUvWidth The width of the central quad of the 9-slice in cell UV coords, with "correction scaling" applied.
    //! \param centerUvHeight The height of the central quad of the 9-slice in cell UV coords, with "correction scaling" applied.
    void GetSlicedFixedStValuesFromCorrectionalScaleBorders(
        float sValues[6],
        float tValues[6],
        const ISprite* sprite,
        const int cellIndex,
        const float leftBorder,
        const float rightBorder,
        const float topBorder,
        const float bottomBorder,
        const AZ::Vector2& pivot,
        const float centerUvWidth,
        const float centerUvHeight)
    {
        const float cellMinUCoord = sprite->GetCellUvCoords(cellIndex).TopLeft().GetX();
        const float cellMaxUCoord = sprite->GetCellUvCoords(cellIndex).TopRight().GetX();
        const float cellMinVCoord = sprite->GetCellUvCoords(cellIndex).TopLeft().GetY();
        const float cellMaxVCoord = sprite->GetCellUvCoords(cellIndex).BottomLeft().GetY();

        // Transform border values from cell space to texture space
        const AZ::Vector2 cellUvSize = sprite->GetCellUvSize(cellIndex);
        const float leftBorderTextureSpace = leftBorder * cellUvSize.GetX();
        const float rightBorderTextureSpace = (1.0f - rightBorder) * cellUvSize.GetX();
        const float topBorderTextureSpace = topBorder * cellUvSize.GetY();
        const float bottomBorderTextureSpace = (1.0f - bottomBorder) * cellUvSize.GetY();

        // This width and height take into account the size of the rect in pixels
        const float centerUvWidthTextureSpace = centerUvWidth * cellUvSize.GetX();
        const float centerUvHeightTextureSpace = centerUvHeight * cellUvSize.GetY();

        // This width and height is what would happen if we were stretching, if the centerUv... equals the betweenBordersUv...
        // the the rect is sized to fit the texture perfectly and stretched and fixed would look the same
        const float betweenBordersUvWidthTextureSpace = rightBorderTextureSpace - leftBorderTextureSpace;
        const float betweenBordersUvHeightTextureSpace = bottomBorderTextureSpace - topBorderTextureSpace;

        // compute the four UV values for the internal verts, the pivot controls where the texture is fixed, i.e. if pivot.x is zero
        // the left edge of the center quad is fixed and the right edge reveals more texture as the size of the rect increases.
        float centerLeftTextureSpace = leftBorderTextureSpace + (betweenBordersUvWidthTextureSpace - centerUvWidthTextureSpace) * pivot.GetX();
        float centerRightTextureSpace = centerLeftTextureSpace + centerUvWidthTextureSpace;

        // clamp the values so they never go into the borders, eventually we might want to support tiling in this case
        centerLeftTextureSpace = AZ::GetClamp(centerLeftTextureSpace, leftBorderTextureSpace, rightBorderTextureSpace);
        centerRightTextureSpace = AZ::GetClamp(centerRightTextureSpace, leftBorderTextureSpace, rightBorderTextureSpace);

        float centerTopTextureSpace = topBorderTextureSpace + (betweenBordersUvHeightTextureSpace - centerUvHeightTextureSpace) * pivot.GetY();
        float centerBottomTextureSpace = centerTopTextureSpace + centerUvHeightTextureSpace;

        // clamp the values so they never go into the borders, eventually we might want to support tiling in this case
        centerTopTextureSpace = AZ::GetClamp(centerTopTextureSpace, topBorderTextureSpace, bottomBorderTextureSpace);
        centerBottomTextureSpace = AZ::GetClamp(centerBottomTextureSpace, topBorderTextureSpace, bottomBorderTextureSpace);

        // The texture coords 0,1,4,5 are just based on the border values while coords 2 & 3 are the internal ones that handle the sliced/fixed behavior
        int uIndex = 0;
        sValues[uIndex++] = cellMinUCoord;
        sValues[uIndex++] = cellMinUCoord + leftBorderTextureSpace;
        sValues[uIndex++] = cellMinUCoord + centerLeftTextureSpace;
        sValues[uIndex++] = cellMinUCoord + centerRightTextureSpace;
        sValues[uIndex++] = cellMinUCoord + rightBorderTextureSpace;
        sValues[uIndex++] = cellMaxUCoord;

        int vIndex = 0;
        tValues[vIndex++] = cellMinVCoord;
        tValues[vIndex++] = cellMinVCoord + topBorderTextureSpace;

        tValues[vIndex++] = cellMinVCoord + centerTopTextureSpace;
        tValues[vIndex++] = cellMinVCoord + centerBottomTextureSpace;

        tValues[vIndex++] = cellMinVCoord + bottomBorderTextureSpace;
        tValues[vIndex++] = cellMaxVCoord;
    }

    //! Set the values for an image vertex
    //! This helper function is used so that we only have to initialize textIndex and texHasColorChannel in one place
    void SetVertex(LyShine::UiPrimitiveVertex& vert, const Vec2& pos, uint32 color, const Vec2& uv)
    {
        vert.xy = pos;
        vert.color.dcolor = color;
        vert.st = uv;
        vert.texIndex = 0;
        vert.texHasColorChannel = 1;
        vert.texIndex2 = 0;
        vert.pad = 0;
    }

    //! Set the values for an image vertex
    //! This version of the helper function takes AZ vectors
    void SetVertex(LyShine::UiPrimitiveVertex& vert, const AZ::Vector2& pos, uint32 color, const AZ::Vector2& uv)
    {
        SetVertex(vert, Vec2(pos.GetX(), pos.GetY()), color, Vec2(uv.GetX(), uv.GetY()));
    }

    //! Given the xValues, yValues, sValues and tValues, fill out the verts array with transformed points.
    //!
    //! \param verts The vertex array to be filled, must be the correct size for "numVerts" verts
    //! \param numVerts The number of vertices to be generated
    //! \param numX The size of the xValues and sValues input arrays
    //! \param numY The size of the yValues and tValues input arrays
    //! \param packedColor The color value to be put in every vertex
    //! \param transform The transform to be applied to the points
    //! \param xValues The x-values for the edges and borders
    void FillVerts(LyShine::UiPrimitiveVertex* verts, [[maybe_unused]] uint32 numVerts, uint32 numX, uint32 numY, uint32 packedColor, const AZ::Matrix4x4& transform,
        float* xValues, float* yValues, float* sValues, float* tValues,
        bool isPixelAligned)
    {
        AZ_Assert(numVerts == numX * numY, "Error: array size does not match dimensions");

        IDraw2d::Rounding pixelRounding = isPixelAligned ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;
        float z = 1.0f;
        int i = 0;
        for (uint32 y = 0; y < numY; ++y)
        {
            for (uint32 x = 0; x < numX; x += 1)
            {
                AZ::Vector3 point3(xValues[x], yValues[y], z);
                point3 = transform * point3;
                point3 = Draw2dHelper::RoundXY(point3, pixelRounding);

                AZ::Vector2 point2(point3.GetX(), point3.GetY());
                AZ::Vector2 uv(sValues[x], tValues[y]);
                SetVertex(verts[i], point2, packedColor, uv);

                ++i;
            }
        }
    }

    const uint32 numQuadsIn9Slice = 9;
    const uint32 numIndicesIn9Slice = numQuadsIn9Slice * 6;
    const uint32 numQuadsIn9SliceExcludingCenter = 8;
    const uint32 numIndicesIn9SliceExcludingCenter = numQuadsIn9SliceExcludingCenter * 6;

    // The vertices are in the order of top row left->right, then next row left->right etc
    //  0  1  2  3
    //  4  5  6  7
    //  8  9 10 11
    // 12 13 14 15
    const uint16 indicesFor9SliceWhenVertsAre4x4[numIndicesIn9Slice] = {
        0,  1,  4,  1,  5,  4,    1,  2,  5,  2,  6,  5,     2,  3,  6,  3,  7,  6,
        4,  5,  8,  5,  9,  8,                               6,  7, 10,  7, 11, 10,
        8,  9, 12,  9, 13, 12,    9, 10, 13, 10, 14, 13,    10, 11, 14, 11, 15, 14,
        5,  6,  9,  6, 10,  9,  // center quad
    };

    // The vertices are in the order of top row left->right, then next row left->right etc
    //  0  1    2  3    4  5
    //  6  7    8  9   10 11
    //
    // 12 13   14 15   16 17
    // 18 19   20 21   22 23
    //
    // 24 25   26 27   28 29
    // 30 31   32 33   34 35
    const uint16 indicesFor9SliceWhenVertsAre6x6[numIndicesIn9Slice] =
    {
        0,   1,  7,  7,  6,  0,    2,  3,  9,  9,  8,  2,    4,  5, 11, 11, 10,  4,
        12, 13, 19, 19, 18, 12,                             16, 17, 23, 23, 22, 16,
        24, 25, 31, 31, 30, 24,   26, 27, 33, 33, 32, 26,   28, 29, 35, 35, 34, 28,
        14, 15, 21, 21, 20, 14,     // center quad
    };

    AZ::Data::Instance<AZ::RPI::Image> GetSpriteImage(ISprite* sprite)
    {
        AZ::Data::Instance<AZ::RPI::Image> image;
        if (sprite)
        {
            image = sprite->GetImage();
        }

        return image;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::UiImageComponent()
    : m_isColorOverridden(false)
    , m_isAlphaOverridden(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageComponent::~UiImageComponent()
{
    SAFE_RELEASE(m_sprite);
    SAFE_RELEASE(m_overrideSprite);

    ClearCachedVertices();
    ClearCachedIndices();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ResetOverrides()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
    SAFE_RELEASE(m_overrideSprite);

    m_isColorOverridden = false;
    m_isAlphaOverridden = false;
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetOverrideColor(const AZ::Color& color)
{
    m_overrideColor.Set(color.GetAsVector3());

    m_isColorOverridden = true;
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetOverrideAlpha(float alpha)
{
    m_overrideAlpha = alpha;

    m_isAlphaOverridden = true;
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetOverrideSprite(ISprite* sprite, AZ::u32 cellIndex)
{
    SAFE_RELEASE(m_overrideSprite);
    m_overrideSprite = sprite;

    if (m_overrideSprite)
    {
        m_overrideSprite->AddRef();
        m_overrideSpriteCellIndex = cellIndex;
    }
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Render(LyShine::IRenderGraph* renderGraph)
{
    // get fade value (tracked by UiRenderer) and compute the desired alpha for the image
    float fade = renderGraph->GetAlphaFade();
    float desiredAlpha = m_overrideAlpha * fade;
    uint8 desiredPackedAlpha = static_cast<uint8>(desiredAlpha * 255.0f);

    ISprite* sprite = (m_overrideSprite) ? m_overrideSprite : m_sprite;

    if (m_isRenderCacheDirty)
    {
        int cellIndex = (m_overrideSprite) ? m_overrideSpriteCellIndex : m_spriteSheetCellIndex;

        bool isTextureSRGB = IsSpriteTypeRenderTarget() && m_isRenderTargetSRGB;

        AZ::Color color = AZ::Color::CreateFromVector3AndFloat(m_overrideColor.GetAsVector3(), 1.0f);
        if (!isTextureSRGB)
        {
            color = color.GammaToLinear();   // the colors are specified in sRGB but we want linear colors in the shader
        }
        uint32 packedColor = (desiredPackedAlpha << 24) | (color.GetR8() << 16) | (color.GetG8() << 8) | color.GetB8();

        ImageType imageType = m_imageType;

        // if there is no texture we will just use a white texture and want to stretch it
        const bool spriteOrTextureIsNull = sprite == nullptr || sprite->GetImage() == nullptr;

        // Zero texture size may occur even if the UiImageComponent has a valid non-zero-sized texture,
        // because a canvas can be requested to Render() before the texture asset is done loading.
        if (!spriteOrTextureIsNull)
        {
            const AZ::Vector2 textureSize = sprite->GetSize();
            if (textureSize.GetX() == 0 || textureSize.GetY() == 0)
            {
                // don't render to cache and leave m_isRenderCacheDirty set to true
                return;        
            }
        }

        // if the borders are zero width then sliced is the same as stretched and stretched is simpler to render
        const bool spriteIsSlicedAndBordersAreZeroWidth = imageType == ImageType::Sliced && sprite && sprite->AreCellBordersZeroWidth(cellIndex);

        if (spriteOrTextureIsNull || spriteIsSlicedAndBordersAreZeroWidth)
        {
            imageType = ImageType::Stretched;
        }

        switch (imageType)
        {
        case ImageType::Stretched:
            RenderStretchedSprite(sprite, cellIndex, packedColor);
            break;
        case ImageType::Sliced:
            AZ_Assert(sprite, "Should not get here if no sprite path is specified");
            RenderSlicedSprite(sprite, cellIndex, packedColor);   // will not get here if sprite is null since we change type in that case above
            break;
        case ImageType::Fixed:
            AZ_Assert(sprite, "Should not get here if no sprite path is specified");
            RenderFixedSprite(sprite, cellIndex, packedColor);
            break;
        case ImageType::Tiled:
            AZ_Assert(sprite, "Should not get here if no sprite path is specified");
            RenderTiledSprite(sprite, packedColor);
            break;
        case ImageType::StretchedToFit:
            AZ_Assert(sprite, "Should not get here if no sprite path is specified");
            RenderStretchedToFitOrFillSprite(sprite, cellIndex, packedColor, true);
            break;
        case ImageType::StretchedToFill:
            AZ_Assert(sprite, "Should not get here if no sprite path is specified");
            RenderStretchedToFitOrFillSprite(sprite, cellIndex, packedColor, false);
            break;
        }

        if (!UiCanvasPixelAlignmentNotificationBus::Handler::BusIsConnected())
        {
            AZ::EntityId canvasEntityId;
            UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
            UiCanvasPixelAlignmentNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }

    // if desired alpha is zero then no need to do any more
    if (desiredPackedAlpha == 0)
    {
        return;
    }

    // Render cache is now valid - render using the cache

    // this should always be true but test to be safe
    if (m_cachedPrimitive.m_numVertices > 0)
    {
        // If the fade value has changed we need to update the alpha values in the vertex colors but we do
        // not want to touch or recompute the RGB values
        if (m_cachedPrimitive.m_vertices[0].color.a != desiredPackedAlpha)
        {
            // go through all the cached vertices and update the alpha values
            LyShine::UCol desiredPackedColor = m_cachedPrimitive.m_vertices[0].color;
            desiredPackedColor.a = desiredPackedAlpha;
            for (int i = 0; i < m_cachedPrimitive.m_numVertices; ++i)
            {
                m_cachedPrimitive.m_vertices[i].color = desiredPackedColor;
            }
        }

        AZ::Data::Instance<AZ::RPI::Image> image = GetSpriteImage(sprite);
        bool isClampTextureMode = m_imageType == ImageType::Tiled ? false : true;
        bool isTextureSRGB = IsSpriteTypeRenderTarget() && m_isRenderTargetSRGB;
        bool isTexturePremultipliedAlpha = false; // we are not rendering from a render target with alpha in it

        renderGraph->AddPrimitive(&m_cachedPrimitive, image, isClampTextureMode, isTextureSRGB, isTexturePremultipliedAlpha, m_blendMode);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiImageComponent::GetColor()
{
    return AZ::Color::CreateFromVector3AndFloat(m_color.GetAsVector3(), m_alpha);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetColor(const AZ::Color& color)
{
    m_color.Set(color.GetAsVector3());
    m_alpha = color.GetA();

    AZ::Color oldOverrideColor = m_overrideColor;
    float oldOverrideAlpha = m_overrideAlpha;

    if (!m_isColorOverridden)
    {
        m_overrideColor = m_color;
    }
    if (!m_isAlphaOverridden)
    {
        m_overrideAlpha = m_alpha;
    }

    if (oldOverrideColor != m_overrideColor)
    {
        MarkRenderCacheDirty();
    }
    else if (oldOverrideAlpha != m_overrideAlpha)
    {
        // alpha changed so we need RenderGraph to be rebuilt but not render cache
        MarkRenderGraphDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetAlpha()
{
    return m_alpha;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetAlpha(float alpha)
{
    float oldOverrideAlpha = m_overrideAlpha;

    m_alpha = alpha;
    if (!m_isAlphaOverridden)
    {
        m_overrideAlpha = m_alpha;
    }

    if (oldOverrideAlpha != m_overrideAlpha)
    {
        // alpha changed so we need RenderGraph to be rebuilt but not render cache
        MarkRenderGraphDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ISprite* UiImageComponent::GetSprite()
{
    return m_sprite;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetSprite(ISprite* sprite)
{
    if (m_sprite)
    {
        if (UiSpriteSettingsChangeNotificationBus::Handler::BusIsConnected())
        {
            UiSpriteSettingsChangeNotificationBus::Handler::BusDisconnect();
        }

        m_sprite->Release();
        m_spritePathname.SetAssetPath("");
    }

    m_sprite = sprite;

    if (m_sprite)
    {
        m_sprite->AddRef();
        m_spritePathname.SetAssetPath(m_sprite->GetPathname().c_str());

        UiSpriteSettingsChangeNotificationBus::Handler::BusConnect(m_sprite);
    }

    InvalidateLayouts();
    UiSpriteSourceNotificationBus::Event(GetEntityId(), &UiSpriteSourceNotificationBus::Events::OnSpriteSourceChanged);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiImageComponent::GetSpritePathname()
{
    return m_spritePathname.GetAssetPath();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetSpritePathname(AZStd::string spritePath)
{
    m_spritePathname.SetAssetPath(spritePath.c_str());

    if (IsSpriteTypeAsset())
    {
        OnSpritePathnameChange();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::SetSpritePathnameIfExists(AZStd::string spritePath)
{
    if (AZ::Interface<ILyShine>::Get()->DoesSpriteTextureAssetExist(spritePath))
    {
        SetSpritePathname(spritePath);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Data::Asset<AZ::RPI::AttachmentImageAsset> UiImageComponent::GetAttachmentImageAsset()
{
    return m_attachmentImageAsset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetAttachmentImageAsset(const AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>& attachmentImageAsset)
{
    m_attachmentImageAsset = attachmentImageAsset;

    if (m_spriteType == UiImageInterface::SpriteType::RenderTarget)
    {
        OnSpriteAttachmentImageAssetChange();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::GetIsRenderTargetSRGB()
{
    return m_isRenderTargetSRGB;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetIsRenderTargetSRGB(bool isSRGB)
{
    if (m_isRenderTargetSRGB != isSRGB)
    {
        m_isRenderTargetSRGB = isSRGB;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::SpriteType UiImageComponent::GetSpriteType()
{
    return m_spriteType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetSpriteType(SpriteType spriteType)
{
    m_spriteType = spriteType;

    OnSpriteTypeChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::ImageType UiImageComponent::GetImageType()
{
    return m_imageType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetImageType(ImageType imageType)
{
    if (m_imageType != imageType)
    {
        m_imageType = imageType;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetImageIndex(AZ::u32 index)
{
    if (m_spriteSheetCellIndex != index)
    {
        m_spriteSheetCellIndex = index;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZ::u32 UiImageComponent::GetImageIndex()
{
    return m_spriteSheetCellIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZ::u32 UiImageComponent::GetImageIndexCount()
{
    if (m_sprite)
    {
        return static_cast<AZ::u32>(m_sprite->GetSpriteSheetCells().size());
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiImageComponent::GetImageIndexAlias(AZ::u32 index)
{
    return m_sprite ? m_sprite->GetCellAlias(index) : AZStd::string();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetImageIndexAlias(AZ::u32 index, const AZStd::string& alias)
{
    m_sprite ? m_sprite->SetCellAlias(index, alias) : AZ_UNUSED(0);
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u32 UiImageComponent::GetImageIndexFromAlias(const AZStd::string& alias)
{
    return m_sprite ? m_sprite->GetCellIndexFromAlias(alias) : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::FillType UiImageComponent::GetFillType()
{
    return m_fillType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetFillType(UiImageInterface::FillType fillType)
{
    if (m_fillType != fillType)
    {
        m_fillType = fillType;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetFillAmount()
{
    return m_fillAmount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetFillAmount(float fillAmount)
{
    float clampedFillAmount = AZ::GetClamp(fillAmount, 0.0f, 1.0f);
    if (m_fillAmount != clampedFillAmount)
    {
        m_fillAmount = clampedFillAmount;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetRadialFillStartAngle()
{
    return m_fillStartAngle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetRadialFillStartAngle(float radialFillStartAngle)
{
    if (m_fillStartAngle != radialFillStartAngle)
    {
        m_fillStartAngle = radialFillStartAngle;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::FillCornerOrigin UiImageComponent::GetCornerFillOrigin()
{
    return m_fillCornerOrigin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetCornerFillOrigin(UiImageInterface::FillCornerOrigin cornerOrigin)
{
    if (m_fillCornerOrigin != cornerOrigin)
    {
        m_fillCornerOrigin = cornerOrigin;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageInterface::FillEdgeOrigin UiImageComponent::GetEdgeFillOrigin()
{
    return m_fillEdgeOrigin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetEdgeFillOrigin(UiImageInterface::FillEdgeOrigin edgeOrigin)
{
    if (m_fillEdgeOrigin != edgeOrigin)
    {
        m_fillEdgeOrigin = edgeOrigin;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::GetFillClockwise()
{
    return m_fillClockwise;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetFillClockwise(bool fillClockwise)
{
    if (m_fillClockwise != fillClockwise)
    {
        m_fillClockwise = fillClockwise;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::GetFillCenter()
{
    return m_fillCenter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SetFillCenter(bool fillCenter)
{
    if (m_fillCenter != fillCenter)
    {
        m_fillCenter = fillCenter;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::PropertyValuesChanged()
{
    if (!m_isColorOverridden)
    {
        m_overrideColor = m_color;
    }
    if (!m_isAlphaOverridden)
    {
        m_overrideAlpha = m_alpha;
    }
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnCanvasSpaceRectChanged(AZ::EntityId /*entityId*/, const UiTransformInterface::Rect& /*oldRect*/, const UiTransformInterface::Rect& /*newRect*/)
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnTransformToViewportChanged()
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetMinWidth()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetMinHeight()
{
    return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetTargetWidth(float /*maxWidth*/)
{
    float targetWidth = 0.0f;

    if (m_sprite)
    {
        switch (m_imageType)
        {
        case ImageType::Fixed:
        {
            AZ::Vector2 textureSize = m_sprite->GetCellSize(m_spriteSheetCellIndex);
            targetWidth = textureSize.GetX();
        }
        break;

        case ImageType::Sliced:
        {
            AZ::Vector2 textureSize = m_sprite->GetCellSize(m_spriteSheetCellIndex);
            targetWidth = (m_sprite->GetBorders().m_left * textureSize.GetX()) + ((1.0f - m_sprite->GetBorders().m_right) * textureSize.GetX());
        }
        break;

        default:
        {
            targetWidth = 0.0f;
        }
        break;
        }
    }

    return targetWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetTargetHeight(float /*maxHeight*/)
{
    float targetHeight = 0.0f;

    if (m_sprite)
    {
        switch (m_imageType)
        {
        case ImageType::Fixed:
        {
            AZ::Vector2 textureSize = m_sprite->GetCellSize(m_spriteSheetCellIndex);
            targetHeight = textureSize.GetY();
        }
        break;

        case ImageType::Sliced:
        {
            AZ::Vector2 textureSize = m_sprite->GetCellSize(m_spriteSheetCellIndex);
            targetHeight = (m_sprite->GetBorders().m_top * textureSize.GetY()) + ((1.0f - m_sprite->GetBorders().m_bottom) * textureSize.GetY());
        }
        break;

        case ImageType::StretchedToFit:
        {
            AZ::Vector2 textureSize = m_sprite->GetCellSize(m_spriteSheetCellIndex);
            if (textureSize.GetX() > 0.0f)
            {
                // Get element size
                AZ::Vector2 size(0.0f, 0.0f);
                UiTransformBus::EventResult(size, GetEntityId(), &UiTransformBus::Events::GetCanvasSpaceSizeNoScaleRotate);

                targetHeight = textureSize.GetY() * (size.GetX() / textureSize.GetX());
            }
            else
            {
                targetHeight = 0.0f;
            }
        }
        break;

        default:
        {
            targetHeight = 0.0f;
        }
        break;
        }
    }

    return targetHeight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetExtraWidthRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiImageComponent::GetExtraHeightRatio()
{
    return 1.0f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnCanvasPixelAlignmentChange()
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnSpriteSettingsChanged()
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiImageComponent, AZ::Component>()
            ->Version(8, &VersionConverter)
            ->Field("SpriteType", &UiImageComponent::m_spriteType)
            ->Field("SpritePath", &UiImageComponent::m_spritePathname)
            ->Field("Index", &UiImageComponent::m_spriteSheetCellIndex)
            ->Field("AttachmentImageAsset", &UiImageComponent::m_attachmentImageAsset)
            ->Field("IsRenderTargetSRGB", &UiImageComponent::m_isRenderTargetSRGB)
            ->Field("Color", &UiImageComponent::m_color)
            ->Field("Alpha", &UiImageComponent::m_alpha)
            ->Field("ImageType", &UiImageComponent::m_imageType)
            ->Field("FillCenter", &UiImageComponent::m_fillCenter)
            ->Field("StretchSliced", &UiImageComponent::m_isSlicingStretched)
            ->Field("BlendMode", &UiImageComponent::m_blendMode)
            ->Field("FillType", &UiImageComponent::m_fillType)
            ->Field("FillAmount", &UiImageComponent::m_fillAmount)
            ->Field("FillStartAngle", &UiImageComponent::m_fillStartAngle)
            ->Field("FillCornerOrigin", &UiImageComponent::m_fillCornerOrigin)
            ->Field("FillEdgeOrigin", &UiImageComponent::m_fillEdgeOrigin)
            ->Field("FillClockwise", &UiImageComponent::m_fillClockwise);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiImageComponent>("Image", "A visual component to draw a rectangle with an optional sprite/texture");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiImage.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiImage.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_spriteType, "SpriteType", "The sprite type.")
                ->EnumAttribute(UiImageInterface::SpriteType::SpriteAsset, "Sprite/Texture asset")
                ->EnumAttribute(UiImageInterface::SpriteType::RenderTarget, "Render target")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorSpriteTypeChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));
            editInfo->DataElement("Sprite", &UiImageComponent::m_spritePathname, "Sprite path", "The sprite path. Can be overridden by another component such as an interactable.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSpriteTypeAsset)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorSpritePathnameChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_spriteSheetCellIndex, "Index", "Sprite-sheet index. Defines which cell in a sprite-sheet is displayed.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSpriteTypeSpriteSheet)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnIndexChange)
                ->Attribute("EnumValues", &UiImageComponent::PopulateIndexStringList);
            editInfo->DataElement(AZ::Edit::UIHandlers::Default, &UiImageComponent::m_attachmentImageAsset, "Attachment Image Asset", "The render target associated with the sprite.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSpriteTypeRenderTarget)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnSpriteAttachmentImageAssetChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiImageComponent::m_isRenderTargetSRGB, "Render Target sRGB", "Check this box if the render target is in sRGB space instead of linear RGB space.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSpriteTypeRenderTarget)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiImageComponent::m_color, "Color", "The color tint for the image. Can be overridden by another component such as an interactable.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnColorChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiImageComponent::m_alpha, "Alpha", "The transparency. Can be overridden by another component such as an interactable.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnColorChange)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_imageType, "ImageType", "The image type. Affects how the texture/sprite is mapped to the image rectangle.")
                ->EnumAttribute(UiImageInterface::ImageType::Stretched, "Stretched")
                ->EnumAttribute(UiImageInterface::ImageType::Sliced, "Sliced")
                ->EnumAttribute(UiImageInterface::ImageType::Fixed, "Fixed")
                ->EnumAttribute(UiImageInterface::ImageType::Tiled, "Tiled")
                ->EnumAttribute(UiImageInterface::ImageType::StretchedToFit, "Stretched To Fit")
                ->EnumAttribute(UiImageInterface::ImageType::StretchedToFill, "Stretched To Fill")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorImageTypeChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiImageComponent::m_fillCenter, "Fill Center", "Sliced image center is filled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSliced)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiImageComponent::m_isSlicingStretched, "Stretch Center/Edges",
                "If true, sliced image center and edges are stretched. If false, they act as fixed in the same way as the corners and the pivot controls how they are anchored.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsSliced)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_blendMode, "BlendMode", "The blend mode used to draw the image")
                ->EnumAttribute(LyShine::BlendMode::Normal, "Normal")
                ->EnumAttribute(LyShine::BlendMode::Add, "Add")
                ->EnumAttribute(LyShine::BlendMode::Screen, "Screen")
                ->EnumAttribute(LyShine::BlendMode::Darken, "Darken")
                ->EnumAttribute(LyShine::BlendMode::Lighten, "Lighten")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_fillType, "Fill Type", "The fill style used to draw the image.")
                ->EnumAttribute(UiImageComponent::FillType::None, "None")
                ->EnumAttribute(UiImageComponent::FillType::Linear, "Linear")
                ->EnumAttribute(UiImageComponent::FillType::Radial, "Radial")
                ->EnumAttribute(UiImageComponent::FillType::RadialCorner, "RadialCorner")
                ->EnumAttribute(UiImageComponent::FillType::RadialEdge, "RadialEdge")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiImageComponent::m_fillAmount, "Fill Amount", "The amount of the image to be filled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsFilled)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiImageComponent::m_fillStartAngle, "Fill Start Angle", "The start angle for the fill in degrees measured clockwise from straight up.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsRadialFilled)
                ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_fillCornerOrigin, "Corner Fill Origin", "The corner from which the image is filled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsCornerFilled)
                ->EnumAttribute(UiImageComponent::FillCornerOrigin::TopLeft, "TopLeft")
                ->EnumAttribute(UiImageComponent::FillCornerOrigin::TopRight, "TopRight")
                ->EnumAttribute(UiImageComponent::FillCornerOrigin::BottomRight, "BottomRight")
                ->EnumAttribute(UiImageComponent::FillCornerOrigin::BottomLeft, "BottomLeft")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageComponent::m_fillEdgeOrigin, "Edge Fill Origin", "The edge from which the image is filled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsEdgeFilled)
                ->EnumAttribute(UiImageComponent::FillEdgeOrigin::Left, "Left")
                ->EnumAttribute(UiImageComponent::FillEdgeOrigin::Top, "Top")
                ->EnumAttribute(UiImageComponent::FillEdgeOrigin::Right, "Right")
                ->EnumAttribute(UiImageComponent::FillEdgeOrigin::Bottom, "Bottom")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiImageComponent::m_fillClockwise, "Fill Clockwise", "Image is filled clockwise about the origin.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiImageComponent::IsRadialAnyFilled)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageComponent::OnEditorRenderSettingChange);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)UiImageInterface::ImageType::Stretched>("eUiImageType_Stretched")
            ->Enum<(int)UiImageInterface::ImageType::Sliced>("eUiImageType_Sliced")
            ->Enum<(int)UiImageInterface::ImageType::Fixed>("eUiImageType_Fixed")
            ->Enum<(int)UiImageInterface::ImageType::Tiled>("eUiImageType_Tiled")
            ->Enum<(int)UiImageInterface::ImageType::StretchedToFit>("eUiImageType_StretchedToFit")
            ->Enum<(int)UiImageInterface::ImageType::StretchedToFill>("eUiImageType_StretchedToFill")
            ->Enum<(int)UiImageInterface::SpriteType::SpriteAsset>("eUiSpriteType_SpriteAsset")
            ->Enum<(int)UiImageInterface::SpriteType::RenderTarget>("eUiSpriteType_RenderTarget")
            ->Enum<(int)UiImageInterface::FillType::None>("eUiFillType_None")
            ->Enum<(int)UiImageInterface::FillType::Linear>("eUiFillType_Linear")
            ->Enum<(int)UiImageInterface::FillType::Radial>("eUiFillType_Radial")
            ->Enum<(int)UiImageInterface::FillType::RadialCorner>("eUiFillType_RadialCorner")
            ->Enum<(int)UiImageInterface::FillType::RadialEdge>("eUiFillType_RadialEdge")
            ->Enum<(int)UiImageInterface::FillCornerOrigin::TopLeft>("eUiFillCornerOrigin_TopLeft")
            ->Enum<(int)UiImageInterface::FillCornerOrigin::TopRight>("eUiFillCornerOrigin_TopRight")
            ->Enum<(int)UiImageInterface::FillCornerOrigin::BottomRight>("eUiFillCornerOrigin_BottomRight")
            ->Enum<(int)UiImageInterface::FillCornerOrigin::BottomLeft>("eUiFillCornerOrigin_BottomLeft")
            ->Enum<(int)UiImageInterface::FillEdgeOrigin::Left>("eUiFillEdgeOrigin_Left")
            ->Enum<(int)UiImageInterface::FillEdgeOrigin::Top>("eUiFillEdgeOrigin_Top")
            ->Enum<(int)UiImageInterface::FillEdgeOrigin::Right>("eUiFillEdgeOrigin_Right")
            ->Enum<(int)UiImageInterface::FillEdgeOrigin::Bottom>("eUiFillEdgeOrigin_Bottom");

        behaviorContext->EBus<UiImageBus>("UiImageBus")
            ->Event("GetColor", &UiImageBus::Events::GetColor)
            ->Event("SetColor", &UiImageBus::Events::SetColor)
            ->Event("GetAlpha", &UiImageBus::Events::GetAlpha)
            ->Event("SetAlpha", &UiImageBus::Events::SetAlpha)
            ->Event("GetSpritePathname", &UiImageBus::Events::GetSpritePathname)
            ->Event("SetSpritePathname", &UiImageBus::Events::SetSpritePathname)
            ->Event("SetSpritePathnameIfExists", &UiImageBus::Events::SetSpritePathnameIfExists)
            ->Event("GetAttachmentImageAsset", &UiImageBus::Events::GetAttachmentImageAsset)
            ->Event("SetAttachmentImageAsset", &UiImageBus::Events::SetAttachmentImageAsset)
            ->Event("GetIsRenderTargetSRGB", &UiImageBus::Events::GetIsRenderTargetSRGB)
            ->Event("SetIsRenderTargetSRGB", &UiImageBus::Events::SetIsRenderTargetSRGB)
            ->Event("GetSpriteType", &UiImageBus::Events::GetSpriteType)
            ->Event("SetSpriteType", &UiImageBus::Events::SetSpriteType)
            ->Event("GetImageType", &UiImageBus::Events::GetImageType)
            ->Event("SetImageType", &UiImageBus::Events::SetImageType)
            ->Event("GetFillType", &UiImageBus::Events::GetFillType)
            ->Event("SetFillType", &UiImageBus::Events::SetFillType)
            ->Event("GetFillAmount", &UiImageBus::Events::GetFillAmount)
            ->Event("SetFillAmount", &UiImageBus::Events::SetFillAmount)
            ->Event("GetRadialFillStartAngle", &UiImageBus::Events::GetRadialFillStartAngle)
            ->Event("SetRadialFillStartAngle", &UiImageBus::Events::SetRadialFillStartAngle)
            ->Event("GetCornerFillOrigin", &UiImageBus::Events::GetCornerFillOrigin)
            ->Event("SetCornerFillOrigin", &UiImageBus::Events::SetCornerFillOrigin)
            ->Event("GetEdgeFillOrigin", &UiImageBus::Events::GetEdgeFillOrigin)
            ->Event("SetEdgeFillOrigin", &UiImageBus::Events::SetEdgeFillOrigin)
            ->Event("GetFillClockwise", &UiImageBus::Events::GetFillClockwise)
            ->Event("SetFillClockwise", &UiImageBus::Events::SetFillClockwise)
            ->Event("GetFillCenter", &UiImageBus::Events::GetFillCenter)
            ->Event("SetFillCenter", &UiImageBus::Events::SetFillCenter)
            ->VirtualProperty("Color", "GetColor", "SetColor")
            ->VirtualProperty("Alpha", "GetAlpha", "SetAlpha")
            ->VirtualProperty("FillAmount", "GetFillAmount", "SetFillAmount")
            ->VirtualProperty("RadialFillStartAngle", "GetRadialFillStartAngle", "SetRadialFillStartAngle")
            ;
        behaviorContext->Class<UiImageComponent>()->RequestBus("UiImageBus");

        behaviorContext->EBus<UiIndexableImageBus>("UiIndexableImageBus")
            ->Event("GetImageIndex", &UiIndexableImageBus::Events::GetImageIndex)
            ->Event("SetImageIndex", &UiIndexableImageBus::Events::SetImageIndex)
            ->Event("GetImageIndexCount", &UiIndexableImageBus::Events::GetImageIndexCount)
            ->Event("GetImageIndexAlias", &UiIndexableImageBus::Events::GetImageIndexAlias)
            ->Event("SetImageIndexAlias", &UiIndexableImageBus::Events::SetImageIndexAlias)
            ->Event("GetImageIndexFromAlias", &UiIndexableImageBus::Events::GetImageIndexFromAlias);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Init()
{
    // If this is called from RC.exe for example these pointers will not be set. In that case
    // we only need to be able to load, init and save the component. It will never be
    // activated.
    if (!AZ::Interface<ILyShine>::Get())
    {
        return;
    }

    // This supports serialization. If we have a sprite pathname but no sprite is loaded
    // then load the sprite
    if (!m_sprite)
    {
        if (IsSpriteTypeAsset())
        {
            if (!m_spritePathname.GetAssetPath().empty())
            {
                m_sprite = AZ::Interface<ILyShine>::Get()->LoadSprite(m_spritePathname.GetAssetPath().c_str());
            }
        }
        else if (m_spriteType == UiImageInterface::SpriteType::RenderTarget)
        {
            if (m_attachmentImageAsset)
            {
                m_sprite = AZ::Interface<ILyShine>::Get()->CreateSprite(m_attachmentImageAsset);
            }
        }
        else
        {
            AZ_Assert(false, "unhandled sprite type");
        }
    }

    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Activate()
{
    UiVisualBus::Handler::BusConnect(m_entity->GetId());
    UiRenderBus::Handler::BusConnect(m_entity->GetId());
    UiImageBus::Handler::BusConnect(m_entity->GetId());
    UiIndexableImageBus::Handler::BusConnect(m_entity->GetId());
    UiAnimateEntityBus::Handler::BusConnect(m_entity->GetId());
    UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());
    UiLayoutCellDefaultBus::Handler::BusConnect(m_entity->GetId());

    if (m_sprite)
    {
        UiSpriteSettingsChangeNotificationBus::Handler::BusConnect(m_sprite);
    }

    // If this is the first time the entity has been activated this is just the same as calling
    // MarkRenderCacheDirty since the canvas is not known. But if an image component has just been
    // added to an existing entity we need to invalidate the layout in case that affects things.
    InvalidateLayouts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::Deactivate()
{
    UiVisualBus::Handler::BusDisconnect();
    UiRenderBus::Handler::BusDisconnect();
    UiImageBus::Handler::BusDisconnect();
    UiIndexableImageBus::Handler::BusDisconnect();
    UiAnimateEntityBus::Handler::BusDisconnect();
    UiTransformChangeNotificationBus::Handler::BusDisconnect();
    UiLayoutCellDefaultBus::Handler::BusDisconnect();

    if (UiCanvasPixelAlignmentNotificationBus::Handler::BusIsConnected())
    {
        UiCanvasPixelAlignmentNotificationBus::Handler::BusDisconnect();
    }
 
    if (UiSpriteSettingsChangeNotificationBus::Handler::BusIsConnected())
    {
        UiSpriteSettingsChangeNotificationBus::Handler::BusDisconnect();
    }

    // We could be about to remove this component and then reactivate the entity
    // which could affect the layout if there is a parent layout component
    InvalidateLayouts();

    // reduce memory use on deactivate
    ClearCachedVertices();
    ClearCachedIndices();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ResetSpriteSheetCellIndex()
{
    m_spriteSheetCellIndex = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderStretchedSprite(ISprite* sprite, int cellIndex, uint32 packedColor)
{
    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetViewportSpacePoints, points);

    if (sprite)
    {
        const UiTransformInterface::RectPoints& uvCoords = sprite->GetCellUvCoords(cellIndex);
        const AZ::Vector2 uvs[4] =
        {
            uvCoords.TopLeft(),
            uvCoords.TopRight(),
            uvCoords.BottomRight(),
            uvCoords.BottomLeft(),
        };
        if (m_fillType != FillType::None)
        {
            RenderFilledQuad(points.pt, uvs, packedColor);
        }
        else
        {
            RenderSingleQuad(points.pt, uvs, packedColor);
        }
    }
    else
    {
        // points are a clockwise quad
        static const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(1, 1), AZ::Vector2(0, 1) };
        if (m_fillType != FillType::None)
        {
            RenderFilledQuad(points.pt, uvs, packedColor);
        }
        else
        {
            RenderSingleQuad(points.pt, uvs, packedColor);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSlicedSprite(ISprite* sprite, int cellIndex, uint32 packedColor)
{
    // get the details of the texture
    AZ::Vector2 textureSize(sprite->GetCellSize(cellIndex));

    // get the untransformed rect for the element plus it's transform matrix
    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, points);

    ISprite::Borders cellUvBorders(sprite->GetCellUvBorders(cellIndex));
    float leftBorder = cellUvBorders.m_left;
    float rightBorder = 1.0f - cellUvBorders.m_right;
    float rectWidth = points.TopRight().GetX() - points.TopLeft().GetX();
    float leftPlusRightBorderWidth = (leftBorder + rightBorder) * textureSize.GetX();

    if (leftPlusRightBorderWidth > rectWidth)
    {
        // the width of the element rect is less that the right and left borders combined
        // so we need to adjust so that they don't get drawn overlapping. We adjust them
        // proportionally
        float correctionScale = rectWidth / leftPlusRightBorderWidth;
        leftBorder *= correctionScale;
        rightBorder *= correctionScale;
    }

    float topBorder = cellUvBorders.m_top;
    float bottomBorder = 1.0f - cellUvBorders.m_bottom;
    float rectHeight = points.BottomLeft().GetY() - points.TopLeft().GetY();
    float topPlusBottomBorderHeight = (topBorder + bottomBorder) * textureSize.GetY();

    if (topPlusBottomBorderHeight > rectHeight)
    {
        // the height of the element rect is less that the top and bottom borders combined
        // so we need to adjust so that they don't get drawn overlapping. We adjust them
        // proportionally
        float correctionScale = rectHeight / topPlusBottomBorderHeight;
        topBorder *= correctionScale;
        bottomBorder *= correctionScale;
    }

    AZ::Matrix4x4 transform;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetTransformToViewport, transform);

    if (m_isSlicingStretched)
    {
        RenderSlicedStretchedSprite(sprite, cellIndex, packedColor, transform,
            textureSize, points, leftBorder, rightBorder, topBorder, bottomBorder);
    }
    else
    {
        float centerUvWidth = 0.0f;
        if (leftPlusRightBorderWidth < rectWidth)
        {
            // Only used for the SlicedFixed case - compute the width of the unstretched center
            float centerWidthInTexels = rectWidth - leftPlusRightBorderWidth;
            centerUvWidth = centerWidthInTexels / textureSize.GetX();
        }

        float centerUvHeight = 0.0f;
        if (topPlusBottomBorderHeight < rectHeight)
        {
            // Only used for the SlicedFixed case - compute the height of the unstretched center
            float centerHeightInTexels = rectHeight - topPlusBottomBorderHeight;
            centerUvHeight = centerHeightInTexels / textureSize.GetY();
        }

        RenderSlicedFixedSprite(sprite, cellIndex, packedColor, transform,
            textureSize, points, leftBorder, rightBorder, topBorder, bottomBorder,
            rectWidth, rectHeight, centerUvWidth, centerUvHeight);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderFixedSprite(ISprite* sprite, int cellIndex, uint32 packedColor)
{
    AZ::Vector2 textureSize(sprite->GetCellSize(cellIndex));

    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, points);

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, GetEntityId(), &UiTransformBus::Events::GetPivot);

    // change width and height to match texture
    AZ::Vector2 rectSize = points.GetAxisAlignedSize();
    AZ::Vector2 sizeDiff = textureSize - rectSize;

    AZ::Vector2 topLeftOffset(sizeDiff.GetX() * pivot.GetX(), sizeDiff.GetY() * pivot.GetY());
    AZ::Vector2 bottomRightOffset(sizeDiff.GetX() * (1.0f - pivot.GetX()), sizeDiff.GetY() * (1.0f - pivot.GetY()));

    points.TopLeft() -= topLeftOffset;
    points.BottomRight() += bottomRightOffset;
    points.TopRight() = AZ::Vector2(points.BottomRight().GetX(), points.TopLeft().GetY());
    points.BottomLeft() = AZ::Vector2(points.TopLeft().GetX(), points.BottomRight().GetY());

    // now apply scale and rotation
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::RotateAndScalePoints, points);

    // now draw the same as Stretched
    const UiTransformInterface::RectPoints& uvCoords = sprite->GetCellUvCoords(cellIndex);
    const AZ::Vector2 uvs[4] =
    {
        uvCoords.TopLeft(),
        uvCoords.TopRight(),
        uvCoords.BottomRight(),
        uvCoords.BottomLeft(),
    };
    if (m_fillType == FillType::None)
    {
        RenderSingleQuad(points.pt, uvs, packedColor);
    }
    else
    {
        RenderFilledQuad(points.pt, uvs, packedColor);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderTiledSprite(ISprite* sprite, uint32 packedColor)
{
    AZ::Vector2 textureSize = sprite->GetSize();

    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, points);

    // scale UV's so that one texel is one pixel on screen
    AZ::Vector2 rectSize = points.GetAxisAlignedSize();
    AZ::Vector2 uvScale(rectSize.GetX() / textureSize.GetX(), rectSize.GetY() / textureSize.GetY());

    // now apply scale and rotation to points
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::RotateAndScalePoints, points);

    // now draw the same as Stretched but with UV's adjusted
    const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(uvScale.GetX(), 0), AZ::Vector2(uvScale.GetX(), uvScale.GetY()), AZ::Vector2(0, uvScale.GetY()) };
    if (m_fillType == FillType::None)
    {
        RenderSingleQuad(points.pt, uvs, packedColor);
    }
    else
    {
        RenderFilledQuad(points.pt, uvs, packedColor);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderStretchedToFitOrFillSprite(ISprite* sprite, int cellIndex, uint32 packedColor, bool toFit)
{
    AZ::Vector2 textureSize = sprite->GetCellSize(cellIndex);

    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, points);

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, GetEntityId(), &UiTransformBus::Events::GetPivot);

    // scale the texture so it either fits or fills the enclosing rect
    AZ::Vector2 rectSize = points.GetAxisAlignedSize();
    const float scaleFactorX = rectSize.GetX() / textureSize.GetX();
    const float scaleFactorY = rectSize.GetY() / textureSize.GetY();
    const float scaleFactor = toFit ?
        AZ::GetMin(scaleFactorX, scaleFactorY) :
        AZ::GetMax(scaleFactorX, scaleFactorY);

    AZ::Vector2 scaledTextureSize = textureSize * scaleFactor;
    AZ::Vector2 sizeDiff = scaledTextureSize - rectSize;

    AZ::Vector2 topLeftOffset(sizeDiff.GetX() * pivot.GetX(), sizeDiff.GetY() * pivot.GetY());
    AZ::Vector2 bottomRightOffset(sizeDiff.GetX() * (1.0f - pivot.GetX()), sizeDiff.GetY() * (1.0f - pivot.GetY()));

    points.TopLeft() -= topLeftOffset;
    points.BottomRight() += bottomRightOffset;
    points.TopRight() = AZ::Vector2(points.BottomRight().GetX(), points.TopLeft().GetY());
    points.BottomLeft() = AZ::Vector2(points.TopLeft().GetX(), points.BottomRight().GetY());

    // now apply scale and rotation
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::RotateAndScalePoints, points);

    // now draw the same as Stretched
    const UiTransformInterface::RectPoints& uvCoords = sprite->GetCellUvCoords(cellIndex);
    const AZ::Vector2 uvs[4] =
    {
        uvCoords.TopLeft(),
        uvCoords.TopRight(),
        uvCoords.BottomRight(),
        uvCoords.BottomLeft(),
    };
    if (m_fillType == FillType::None)
    {
        RenderSingleQuad(points.pt, uvs, packedColor);
    }
    else
    {
        RenderFilledQuad(points.pt, uvs, packedColor);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSingleQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor)
{
    // points are a clockwise quad
    IDraw2d::Rounding pixelRounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;
    const uint32 numVertices = 4;
    LyShine::UiPrimitiveVertex vertices[numVertices];
    for (int i = 0; i < numVertices; ++i)
    {
        AZ::Vector2 roundedPoint = Draw2dHelper::RoundXY(positions[i], pixelRounding);
        SetVertex(vertices[i], roundedPoint, packedColor, uvs[i]);
    }

    const uint32 numIndices = 6;
    uint16 indices[numIndices] = { 0, 1, 2, 2, 3, 0 };

    RenderTriangleList(vertices, indices, numVertices, numIndices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor)
{
    if (m_fillType == FillType::Linear)
    {
        RenderLinearFilledQuad(positions, uvs, packedColor);
    }
    else if (m_fillType == FillType::Radial)
    {
        RenderRadialFilledQuad(positions, uvs, packedColor);
    }
    else if (m_fillType == FillType::RadialCorner)
    {
        RenderRadialCornerFilledQuad(positions, uvs, packedColor);
    }
    else if (m_fillType == FillType::RadialEdge)
    {
        RenderRadialEdgeFilledQuad(positions, uvs, packedColor);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderLinearFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor)
{
    // This fills the vertices (rotating them based on the origin edge) similar to RenderSingleQuad but then edits 2 vertices based on m_fillAmount.
    int vertexOffset = 0;
    if (m_fillEdgeOrigin == FillEdgeOrigin::Left)
    {
        vertexOffset = 0;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Top)
    {
        vertexOffset = 1;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Right)
    {
        vertexOffset = 2;
    }
    else
    {
        vertexOffset = 3;
    }

    // points are a clockwise quad
    IDraw2d::Rounding pixelRounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;
    const uint32 numVertices = 4;
    LyShine::UiPrimitiveVertex vertices[numVertices];

    for (int i = 0; i < numVertices; ++i)
    {
        int index = (i + vertexOffset) % 4;
        AZ::Vector2 roundedPoint = Draw2dHelper::RoundXY(positions[index], pixelRounding);
        SetVertex(vertices[i], roundedPoint, packedColor, uvs[index]);
    }

    vertices[1].xy = vertices[0].xy + m_fillAmount * (vertices[1].xy - vertices[0].xy);
    vertices[2].xy = vertices[3].xy + m_fillAmount * (vertices[2].xy - vertices[3].xy);

    vertices[1].st = vertices[0].st + m_fillAmount * (vertices[1].st - vertices[0].st);
    vertices[2].st = vertices[3].st + m_fillAmount * (vertices[2].st - vertices[3].st);

    const uint32 numIndices = 6;
    uint16 indices[numIndices] = { 0, 1, 2, 2, 3, 0 };

    RenderTriangleList(vertices, indices, numVertices, numIndices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderRadialFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor)
{
    // 1. Fill all vertices/indices as if m_fillAmount is 1.0f
    // 2. Calculate which vertex needs to be moved based on the current value of m_fillAmount and set it's new position/UVs accordingly.
    // 3. Submit only the required amount of vertices/indices

    // The vertices are in the following order. If the calculated fillOffset does not lie on the top edge, the indices are rotated to keep the fill algorithm the same.
    // 5 1/6 2
    //    0
    // 4     3

    int firstIndexOffset = 0;
    int secondIndexOffset = 1;
    int currentVertexToFill = 0;
    int windingDirection = 1;

    float fillOffset = (fmod(m_fillStartAngle, 360.0f) / 360.0f) + 1.125f; // Offsets below are calculated from vertex 5, keep the value is above 0
                                                                           // and offset by 1/8 here so it behaves as if calculated from vertex 1.
    if (!m_fillClockwise)
    {
        fillOffset = 1 - (fmod(m_fillStartAngle, 360.0f) / 360.0f) + 1.875f; // Start angle should be clockwise and offsets below are now measured
                                                                             // counter-clockwise so offset further here back into vertex 1 position.
    }

    int startingEdge = static_cast<int>((fillOffset) / 0.25f) % 4;
    if (!m_fillClockwise)
    {
        // Flip vertices and direction so that the fill algorithm is unchanged.
        firstIndexOffset = 1;
        secondIndexOffset = 0;
        currentVertexToFill = 6;
        windingDirection = -1;
        startingEdge *= -1;
    }

    // Fill vertices (rotated based on startingEdge).
    const int numVertices = 7; // The maximum amount of vertices that can be used
    LyShine::UiPrimitiveVertex verts[numVertices];
    for (int i = 1; i < 5; ++i)
    {
        int srcIndex = (4 + i + startingEdge) % 4;
        int dstIndex = currentVertexToFill % 4 + 2;
        SetVertex(verts[dstIndex], positions[srcIndex], packedColor, uvs[srcIndex]);
        currentVertexToFill += windingDirection;
    }

    const int numIndices = 15;
    uint16 indices[numIndices];
    for (uint16 ix = 0; ix < 5; ++ix)
    {
        indices[ix * 3 + firstIndexOffset] = ix + 1;
        indices[ix * 3 + secondIndexOffset] = ix + 2;
        indices[ix * 3 + 2] = 0;
    }

    float startingEdgeRemainder = fmod(fillOffset, 0.25f);
    float startingEdgePercentage = startingEdgeRemainder * 4;

    // Set start/end vertices
    SetVertex(verts[1], verts[5].xy + startingEdgePercentage * (verts[2].xy - verts[5].xy),
        packedColor, verts[5].st + startingEdgePercentage * (verts[2].st - verts[5].st));
    verts[6] = verts[1];

    // Set center vertex
    SetVertex(verts[0], (verts[5].xy + verts[3].xy) * 0.5f,
        packedColor, (verts[5].st + verts[3].st) * 0.5f);

    int finalEdge = static_cast<int>((startingEdgeRemainder + m_fillAmount) / 0.25f);
    float finalEdgePercentage = fmod(fillOffset + m_fillAmount, 0.25f) * 4;

    // Calculate which vertex should be moved for the current m_fillAmount value and set it's new position/UV.
    int editedVertexIndex = finalEdge + 2;
    int previousVertexIndex = ((3 + editedVertexIndex - 2) % 4) + 2;
    int nextVertexIndex = ((editedVertexIndex - 2) % 4) + 2;
    verts[editedVertexIndex].xy = verts[previousVertexIndex].xy + finalEdgePercentage * (verts[nextVertexIndex].xy - verts[previousVertexIndex].xy);
    verts[editedVertexIndex].st = verts[previousVertexIndex].st + finalEdgePercentage * (verts[nextVertexIndex].st - verts[previousVertexIndex].st);

    RenderTriangleList(verts, indices, editedVertexIndex + 1, 3 * (editedVertexIndex - 1));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderRadialCornerFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor)
{
    // This fills the vertices (rotating them based on the origin edge) similar to RenderSingleQuad, then edits a vertex based on m_fillAmount.
    const uint32 numVerts = 4;
    LyShine::UiPrimitiveVertex verts[numVerts];
    int vertexOffset = 0;
    if (m_fillCornerOrigin == FillCornerOrigin::TopLeft)
    {
        vertexOffset = 0;
    }
    else if (m_fillCornerOrigin == FillCornerOrigin::TopRight)
    {
        vertexOffset = 1;
    }
    else if (m_fillCornerOrigin == FillCornerOrigin::BottomRight)
    {
        vertexOffset = 2;
    }
    else
    {
        vertexOffset = 3;
    }
    for (int i = 0; i < 4; ++i)
    {
        int srcIndex = (i + vertexOffset) % 4;
        SetVertex(verts[i], positions[srcIndex], packedColor, uvs[srcIndex]);
    }

    const uint32 numIndices = 6;
    uint16 indicesCW[numIndices] = { 1, 2, 0, 2, 0, 3 };
    uint16 indicesCCW[numIndices] = { 3, 0, 2, 0, 2, 1 };

    uint16* indices = indicesCW;
    if (!m_fillClockwise)
    {
        // Change index order as we're now filling from the end edge back to the start.
        indices = indicesCCW;
    }

    // Calculate which vertex needs to be moved based on m_fillAmount and set its new position and UV.
    int half = static_cast<int>(floor(m_fillAmount + 0.5f));
    float s = (m_fillAmount - (0.5f * half)) * 2;
    int order = m_fillClockwise ? 1 : -1;
    int vertexToEdit = (half * order) + 2;
    verts[vertexToEdit].xy = verts[vertexToEdit - order].xy + (verts[vertexToEdit].xy - verts[vertexToEdit - order].xy) * s;
    verts[vertexToEdit].st = verts[vertexToEdit - order].st + (verts[vertexToEdit].st - verts[vertexToEdit - order].st) * s;

    int numIndicesToDraw = 3 + half * 3;

    RenderTriangleList(verts, indices, numVerts, numIndicesToDraw);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderRadialEdgeFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor)
{
    // This fills the vertices (rotating them based on the origin edge) similar to RenderSingleQuad, then edits a vertex based on m_fillAmount.
    const uint32 numVertices = 5; // Need an extra vertex for the origin.
    LyShine::UiPrimitiveVertex verts[numVertices];
    int vertexOffset = 0;
    if (m_fillEdgeOrigin == FillEdgeOrigin::Left)
    {
        vertexOffset = 0;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Top)
    {
        vertexOffset = 1;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Right)
    {
        vertexOffset = 2;
    }
    else
    {
        vertexOffset = 3;
    }

    // Generate the vertex on the edge.
    AZ::Vector2 calculatedPosition = (positions[(0 + vertexOffset) % 4] + positions[(3 + vertexOffset) % 4]) * 0.5f;
    AZ::Vector2 calculatedUV = (uvs[(0 + vertexOffset) % 4] + uvs[(3 + vertexOffset) % 4]) * 0.5f;
    SetVertex(verts[0], calculatedPosition, packedColor, calculatedUV);

    // Fill other vertices
    for (int i = 1; i < 5; ++i)
    {
        calculatedPosition = positions[(i - 1 + vertexOffset) % 4];
        calculatedUV = uvs[(i - 1 + vertexOffset) % 4];
        SetVertex(verts[i], calculatedPosition, packedColor, calculatedUV);
    }

    const uint32 numIndices = 9;

    uint16 indicesCW[numIndices] = { 0, 1, 2, 0, 2, 3, 0, 3, 4 };
    uint16 indicesCCW[numIndices] = { 0, 3, 4, 0, 2, 3, 0, 1, 2 };

    uint16* indices = indicesCW;
    int segment = static_cast<int>(fmin(m_fillAmount * 3, 2.0f));
    float s = (m_fillAmount - (0.3333f * segment)) * 3;
    int order = 1;
    int firstVertex = 2;
    if (!m_fillClockwise)
    {
        // Change order as we're now filling from the end back to the start.
        order = -1;
        firstVertex = 3;
        indices = indicesCCW;
    }
    // Calculate which vertex needs to be moved based on m_fillAmount and set its new position and UV.
    int vertexToEdit = (segment * order) + firstVertex;
    verts[vertexToEdit].xy = verts[vertexToEdit - order].xy + (verts[vertexToEdit].xy - verts[vertexToEdit - order].xy) * s;
    verts[vertexToEdit].st = verts[vertexToEdit - order].st + (verts[vertexToEdit].st - verts[vertexToEdit - order].st) * s;

    int numIndicesToDraw = 3 * (segment + 1);

    RenderTriangleList(verts, indices, numVertices, numIndicesToDraw);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSlicedStretchedSprite(ISprite* sprite, int cellIndex, uint32 packedColor, const AZ::Matrix4x4& transform,
    const AZ::Vector2& textureSize, const UiTransformInterface::RectPoints& points,
    float leftBorder, float rightBorder, float topBorder, float bottomBorder)
{
    const uint32 numValues = 4; // the number of values in the xValues, yValues, sValues and tValues arrays

    // compute the values for the vertex positions, the mid positions are a fixed number of pixels in from the
    // edges. This is based on the border percentage of the texture size
    float xValues[] = {
        points.TopLeft().GetX(),
        points.TopLeft().GetX() + textureSize.GetX() * leftBorder,
        points.BottomRight().GetX() - textureSize.GetX() * rightBorder,
        points.BottomRight().GetX(),
    };
    float yValues[] = {
        points.TopLeft().GetY(),
        points.TopLeft().GetY() + textureSize.GetY() * topBorder,
        points.BottomRight().GetY() - textureSize.GetY() * bottomBorder,
        points.BottomRight().GetY(),
    };

    float sValues[numValues];
    float tValues[numValues];
    GetSlicedStValuesFromCorrectionalScaleBorders(sValues, tValues, sprite, cellIndex, leftBorder, rightBorder, topBorder, bottomBorder);

    if (m_fillType == FillType::None)
    {
        RenderSlicedFillModeNoneSprite<numValues>(packedColor, transform, xValues, yValues, sValues, tValues);
    }
    else if (m_fillType == FillType::Linear)
    {
        RenderSlicedLinearFilledSprite<numValues>(packedColor, transform, xValues, yValues, sValues, tValues);
    }
    else if (m_fillType == FillType::Radial)
    {
        RenderSlicedRadialFilledSprite<numValues>(packedColor, transform, xValues, yValues, sValues, tValues);
    }
    else if (m_fillType == FillType::RadialCorner || m_fillType == FillType::RadialEdge)
    {
        RenderSlicedRadialCornerOrEdgeFilledSprite<numValues>(packedColor, transform, xValues, yValues, sValues, tValues);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderSlicedFixedSprite(ISprite* sprite, int cellIndex, uint32 packedColor, const AZ::Matrix4x4& transform,
    const AZ::Vector2& textureSize, const UiTransformInterface::RectPoints& points,
    float leftBorder, float rightBorder, float topBorder, float bottomBorder,
    [[maybe_unused]] float rectWidth, [[maybe_unused]] float rectHeight, float centerUvWidth, float centerUvHeight)
{
    const uint32 numValues = 6; // the number of values in the xValues, yValues, sValues and tValues arrays

    // compute the values for the vertex positions, the mid positions are a fixed number of pixels in from the
    // edges. This is based on the border percentage of the texture size
    float xValues[] = {
        points.TopLeft().GetX(),
        points.TopLeft().GetX() + textureSize.GetX() * leftBorder,
        points.TopLeft().GetX() + textureSize.GetX() * leftBorder,
        points.BottomRight().GetX() - textureSize.GetX() * rightBorder,
        points.BottomRight().GetX() - textureSize.GetX() * rightBorder,
        points.BottomRight().GetX(),
    };
    float yValues[] = {
        points.TopLeft().GetY(),
        points.TopLeft().GetY() + textureSize.GetY() * topBorder,
        points.TopLeft().GetY() + textureSize.GetY() * topBorder,
        points.BottomRight().GetY() - textureSize.GetY() * bottomBorder,
        points.BottomRight().GetY() - textureSize.GetY() * bottomBorder,
        points.BottomRight().GetY(),
    };

    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, GetEntityId(), &UiTransformBus::Events::GetPivot);

    float sValues[numValues];
    float tValues[numValues];
    GetSlicedFixedStValuesFromCorrectionalScaleBorders(sValues, tValues, sprite, cellIndex, leftBorder, rightBorder, topBorder, bottomBorder,
        pivot, centerUvWidth, centerUvHeight);

    if (m_fillType == FillType::None)
    {
        RenderSlicedFillModeNoneSprite<numValues>(packedColor, transform, xValues, yValues, sValues, tValues);
    }
    else if (m_fillType == FillType::Linear)
    {
        RenderSlicedLinearFilledSprite<numValues>(packedColor, transform, xValues, yValues, sValues, tValues);
    }
    else if (m_fillType == FillType::Radial)
    {
        RenderSlicedRadialFilledSprite<numValues>(packedColor, transform, xValues, yValues, sValues, tValues);
    }
    else if (m_fillType == FillType::RadialCorner || m_fillType == FillType::RadialEdge)
    {
        RenderSlicedRadialCornerOrEdgeFilledSprite<numValues>(packedColor, transform, xValues, yValues, sValues, tValues);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<uint32 numValues> void UiImageComponent::RenderSlicedFillModeNoneSprite(uint32 packedColor, const AZ::Matrix4x4& transform,
    float* xValues, float* yValues, float* sValues, float* tValues)
{
    // fill out the verts
    const uint32 numVertices = numValues * numValues;
    LyShine::UiPrimitiveVertex vertices[numVertices];
    FillVerts(vertices, numVertices, numValues, numValues, packedColor, transform, xValues, yValues, sValues, tValues, IsPixelAligned());

    int totalIndices = m_fillCenter ? numIndicesIn9Slice : numIndicesIn9SliceExcludingCenter;
    const uint16* indices = (numValues == 4) ? indicesFor9SliceWhenVertsAre4x4 : indicesFor9SliceWhenVertsAre6x6;
    RenderTriangleList(vertices, indices, numVertices, totalIndices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<uint32 numValues> void UiImageComponent::RenderSlicedLinearFilledSprite(uint32 packedColor, const AZ::Matrix4x4& transform,
    float* xValues, float* yValues, float* sValues, float* tValues)
{
    // 1. Clamp x and y and corresponding s and t values based on m_fillAmount.
    // 2. Fill vertices in the same way as a standard sliced sprite

    const uint32 numVertices = numValues * numValues;
    LyShine::UiPrimitiveVertex vertices[numVertices];

    ClipValuesForSlicedLinearFill(numValues, xValues, yValues, sValues, tValues);

    // Fill the vertices with the generated xy and st values.
    FillVerts(vertices, numVertices, numValues, numValues, packedColor, transform, xValues, yValues, sValues, tValues, IsPixelAligned());

    int totalIndices = m_fillCenter ? numIndicesIn9Slice : numIndicesIn9SliceExcludingCenter;
    const uint16* indices = (numValues == 4) ? indicesFor9SliceWhenVertsAre4x4 : indicesFor9SliceWhenVertsAre6x6;
    RenderTriangleList(vertices, indices, numVertices, totalIndices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<uint32 numValues> void UiImageComponent::RenderSlicedRadialFilledSprite(uint32 packedColor, const AZ::Matrix4x4& transform,
    float* xValues, float* yValues, float* sValues, float* tValues)
{
    // build the verts on the stack
    const uint32 numVertices = numValues * numValues;
    LyShine::UiPrimitiveVertex verts[numVertices];

    // Fill the vertices with the generated xy and st values.
    FillVerts(verts, numVertices, numValues, numValues, packedColor, transform, xValues, yValues, sValues, tValues, IsPixelAligned());

    int totalIndices = (m_fillCenter ? numIndicesIn9Slice : numIndicesIn9SliceExcludingCenter);
    const uint16* indices = (numValues == 4) ? indicesFor9SliceWhenVertsAre4x4 : indicesFor9SliceWhenVertsAre6x6;

    // clip the quads generating new verts and indices and render them to the cache
    ClipAndRenderForSlicedRadialFill(numValues, numVertices, verts, totalIndices, indices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<uint32 numValues> void UiImageComponent::RenderSlicedRadialCornerOrEdgeFilledSprite(uint32 packedColor, const AZ::Matrix4x4& transform,
    float* xValues, float* yValues, float* sValues, float* tValues)
{
    // build the verts on the stack
    const uint32 numVertices = numValues * numValues;
    LyShine::UiPrimitiveVertex verts[numVertices];

    // Fill the vertices with the generated xy and st values.
    FillVerts(verts, numVertices, numValues, numValues, packedColor, transform, xValues, yValues, sValues, tValues, IsPixelAligned());

    int totalIndices = (m_fillCenter ? numIndicesIn9Slice : numIndicesIn9SliceExcludingCenter);
    const uint16* indices = (numValues == 4) ? indicesFor9SliceWhenVertsAre4x4 : indicesFor9SliceWhenVertsAre6x6;

    // clip the quads generating new verts and indices and render them to the cache
    ClipAndRenderForSlicedRadialCornerOrEdgeFill(numValues, numVertices, verts, totalIndices, indices);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ClipValuesForSlicedLinearFill(uint32 numValues, float* xValues, float* yValues, float* sValues, float* tValues)
{
    // 1. Clamp x and y and corresponding s and t values based on m_fillAmount.
    // 2. Fill vertices in the same way as a standard sliced sprite

    float* clipPosition = xValues;
    float* clipSTs = sValues;
    int startClip = 0;
    int clipInc = 1;

    if (m_fillEdgeOrigin == FillEdgeOrigin::Left)
    {
        clipPosition = xValues;
        clipSTs = sValues;
        startClip = 0;
        clipInc = 1;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Top)
    {
        clipPosition = yValues;
        clipSTs = tValues;
        startClip = 0;
        clipInc = 1;
    }
    else if (m_fillEdgeOrigin == FillEdgeOrigin::Right)
    {
        clipPosition = xValues;
        clipSTs = sValues;
        // Start from the end of the array and work back.
        startClip = numValues-1;
        clipInc = -1;
    }
    else
    {
        clipPosition = yValues;
        clipSTs = tValues;
        // Start from the end of the array and work back.
        startClip = numValues-1;
        clipInc = -1;
    }

    // If a segment ends before m_fillAmount then it is fully displayed.
    // If m_fillAmount lies in this segment, change the x/y position and clamp all remaining values to this.
    float totalLength = (clipPosition[startClip + ((numValues-1) * clipInc)] - clipPosition[startClip]);
    float previousPercentage = 0;
    int previousIndex = startClip;
    int clampIndex = -1; // to clamp all values greater than m_fillAmount in specified direction.
    for (uint32 arrayPos = 1; arrayPos < numValues; ++arrayPos)
    {
        int currentIndex = startClip + arrayPos * clipInc;
        float thisPercentage = (clipPosition[currentIndex] - clipPosition[startClip]) / totalLength;

        if (clampIndex != -1) // we've already passed m_fillAmount.
        {
            clipPosition[currentIndex] = clipPosition[clampIndex];
            clipSTs[currentIndex] = clipSTs[clampIndex];
        }
        else if (thisPercentage > m_fillAmount)
        {
            // this index is greater than m_fillAmount but the previous one was not, so calculate our new position and UV.
            float segmentPercentFilled = (m_fillAmount - previousPercentage) / (thisPercentage - previousPercentage);
            clipPosition[currentIndex] = clipPosition[previousIndex] + segmentPercentFilled * (clipPosition[currentIndex] - clipPosition[previousIndex]);
            clipSTs[currentIndex] = clipSTs[previousIndex] + segmentPercentFilled * (clipSTs[currentIndex] - clipSTs[previousIndex]);
            clampIndex = currentIndex; // clamp remaining values to this one to generate degenerate triangles.
        }

        previousPercentage = thisPercentage;
        previousIndex = currentIndex;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ClipAndRenderForSlicedRadialFill(uint32 numVertsPerSide, uint32 numVerts, const LyShine::UiPrimitiveVertex* verts, uint32 totalIndices, const uint16* indices)
{
    // 1. Calculate two points of lines from the center to a point based on m_fillAmount and m_fillOrigin.
    // 2. Clip the triangles of the sprite against those lines based on the fill amount.

    LyShine::UiPrimitiveVertex renderVerts[numIndicesIn9Slice * 4]; // ClipToLine doesn't check for duplicate vertices for speed, so this is the maximum we'll need.
    uint16 renderIndices[numIndicesIn9Slice * 4] = { 0 };

    float fillOffset = AZ::DegToRad(m_fillStartAngle);

    Vec2 lineOrigin = (verts[0].xy + verts[numVerts-1].xy) * 0.5f;
    Vec2 rotatingLineEnd = ((verts[0].xy + verts[numVertsPerSide-1].xy) * 0.5f) - lineOrigin;
    Vec2 firstHalfFixedLineEnd = (rotatingLineEnd * -1.0f);
    Vec2 secondHalfFixedLineEnd = rotatingLineEnd;
    float startAngle = 0;
    float endAngle = -AZ::Constants::TwoPi;

    if (!m_fillClockwise)
    {
        // Clip from the opposite side of the line and rotate the line in the opposite direction.
        float temp = startAngle;
        startAngle = endAngle;
        endAngle = temp;
        rotatingLineEnd = rotatingLineEnd * -1.0f;
        firstHalfFixedLineEnd = firstHalfFixedLineEnd * -1.0f;
        secondHalfFixedLineEnd = secondHalfFixedLineEnd * -1.0f;
    }
    Matrix33 lineRotationMatrix;

    lineRotationMatrix.SetRotationZ(startAngle - fillOffset);
    firstHalfFixedLineEnd = firstHalfFixedLineEnd * lineRotationMatrix;
    firstHalfFixedLineEnd = lineOrigin + firstHalfFixedLineEnd;
    secondHalfFixedLineEnd = secondHalfFixedLineEnd * lineRotationMatrix;
    secondHalfFixedLineEnd = lineOrigin + secondHalfFixedLineEnd;

    lineRotationMatrix.SetRotationZ(startAngle - fillOffset + (endAngle - startAngle) * m_fillAmount);
    rotatingLineEnd = rotatingLineEnd * lineRotationMatrix;
    rotatingLineEnd = lineOrigin + rotatingLineEnd;

    int numIndicesToRender = 0;
    int vertexOffset = 0;
    int indicesUsed = 0;
    const int maxTemporaryVerts = 4;
    const int maxTemporaryIndices = 6;
    if (m_fillAmount < 0.5f)
    {
        // Clips against first half line and then rotating line and adds results to render list.
        for (uint32 currentIndex = 0; currentIndex < totalIndices; currentIndex += 3)
        {
            LyShine::UiPrimitiveVertex intermediateVerts[maxTemporaryVerts];
            uint16 intermediateIndices[maxTemporaryIndices];
            int intermedateVertexOffset = 0;
            int intermediateIndicesUsed = ClipToLine(verts, &indices[currentIndex], intermediateVerts, intermediateIndices, intermedateVertexOffset, 0, lineOrigin, firstHalfFixedLineEnd);
            for (int currentIntermediateIndex = 0; currentIntermediateIndex < intermediateIndicesUsed; currentIntermediateIndex += 3)
            {
                indicesUsed = ClipToLine(intermediateVerts, &intermediateIndices[currentIntermediateIndex], renderVerts, renderIndices, vertexOffset, numIndicesToRender, lineOrigin, rotatingLineEnd);
                numIndicesToRender += indicesUsed;
            }
        }
    }
    else
    {
        // Clips against first half line and adds results to render list then clips against the second half line and rotating line and also adds those results to render list.
        for (uint32 currentIndex = 0; currentIndex < totalIndices; currentIndex += 3)
        {
            LyShine::UiPrimitiveVertex intermediateVerts[maxTemporaryVerts];
            uint16 intermediateIndices[maxTemporaryIndices];
            indicesUsed = ClipToLine(verts, &indices[currentIndex], renderVerts, renderIndices, vertexOffset, numIndicesToRender, lineOrigin, firstHalfFixedLineEnd);
            numIndicesToRender += indicesUsed;

            int intermedateVertexOffset = 0;
            int intermediateIndicesUsed = ClipToLine(verts, &indices[currentIndex], intermediateVerts, intermediateIndices, intermedateVertexOffset, 0, lineOrigin, secondHalfFixedLineEnd);
            for (int currentIntermediateIndex = 0; currentIntermediateIndex < intermediateIndicesUsed; currentIntermediateIndex += 3)
            {
                indicesUsed = ClipToLine(intermediateVerts, &intermediateIndices[currentIntermediateIndex], renderVerts, renderIndices, vertexOffset, numIndicesToRender, lineOrigin, rotatingLineEnd);
                numIndicesToRender += indicesUsed;
            }
        }
    }

    RenderTriangleList(renderVerts, renderIndices, vertexOffset, numIndicesToRender);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ClipAndRenderForSlicedRadialCornerOrEdgeFill(uint32 numVertsPerSide, uint32 numVerts, const LyShine::UiPrimitiveVertex* verts, uint32 totalIndices, const uint16* indices)
{
    // 1. Calculate two points of a line from either the corner or center of an edge to a point based on m_fillAmount.
    // 2. Clip the triangles of the sprite against that line.

    LyShine::UiPrimitiveVertex renderVerts[numIndicesIn9Slice * 2]; // ClipToLine doesn't check for duplicate vertices for speed, so this is the maximum we'll need.
    uint16 renderIndices[numIndicesIn9Slice * 2] = { 0 };

    // Generate the start and direction of the line to clip against based on the fill origin and fill amount.
    int originVertex = 0;
    int targetVertex = 0;

    if ((m_fillType == FillType::RadialCorner && m_fillCornerOrigin == FillCornerOrigin::TopLeft) ||
        (m_fillType == FillType::RadialEdge && m_fillEdgeOrigin == FillEdgeOrigin::Top))
    {
        originVertex = 0;
        targetVertex = numVertsPerSide-1;
    }
    else if ((m_fillType == FillType::RadialCorner && m_fillCornerOrigin == FillCornerOrigin::TopRight) ||
             (m_fillType == FillType::RadialEdge && m_fillEdgeOrigin == FillEdgeOrigin::Right))
    {
        originVertex = numVertsPerSide-1;
        targetVertex = numVerts-1;
    }
    else if ((m_fillType == FillType::RadialCorner && m_fillCornerOrigin == FillCornerOrigin::BottomRight) ||
             (m_fillType == FillType::RadialEdge && m_fillEdgeOrigin == FillEdgeOrigin::Bottom))
    {
        originVertex = numVerts-1;
        targetVertex = numVertsPerSide * (numVertsPerSide-1);
    }
    else if ((m_fillType == FillType::RadialCorner && m_fillCornerOrigin == FillCornerOrigin::BottomLeft) ||
             (m_fillType == FillType::RadialEdge && m_fillEdgeOrigin == FillEdgeOrigin::Left))
    {
        originVertex = numVertsPerSide * (numVertsPerSide-1);
        targetVertex = 0;
    }

    Vec2 lineOrigin(verts[originVertex].xy);
    if (m_fillType == FillType::RadialEdge)
    {
        lineOrigin = (verts[originVertex].xy + verts[targetVertex].xy) * 0.5f;
    }

    Vec2 lineEnd = verts[targetVertex].xy - verts[originVertex].xy;
    float startAngle = 0;
    float endAngle = m_fillType == FillType::RadialCorner ? -AZ::Constants::HalfPi : -AZ::Constants::Pi;

    if (!m_fillClockwise)
    {
        // Clip from the opposite side of the line and rotate the line in the opposite direction.
        float temp = startAngle;
        startAngle = endAngle;
        endAngle = temp;
        lineEnd = lineEnd * -1.0f;
    }
    Matrix33 lineRotationMatrix;
    lineRotationMatrix.SetRotationZ(startAngle + (endAngle - startAngle) * m_fillAmount);
    lineEnd = lineEnd * lineRotationMatrix;
    lineEnd = lineOrigin + lineEnd;

    int numIndicesToRender = 0;
    int vertexOffset = 0;
    for (uint32 ix = 0; ix < totalIndices; ix += 3)
    {
        int indicesUsed = ClipToLine(verts, &indices[ix], renderVerts, renderIndices, vertexOffset, numIndicesToRender, lineOrigin, lineEnd);
        numIndicesToRender += indicesUsed;
    }

    RenderTriangleList(renderVerts, renderIndices, vertexOffset, numIndicesToRender);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiImageComponent::ClipToLine(const LyShine::UiPrimitiveVertex* vertices, const uint16* indices, LyShine::UiPrimitiveVertex* renderVertices, uint16* renderIndices, int& vertexOffset, int renderIndexOffset, const Vec2& lineOrigin, const Vec2& lineEnd)
{
    Vec2 lineVector = lineEnd - lineOrigin;
    LyShine::UiPrimitiveVertex lastVertex = vertices[indices[2]];
    LyShine::UiPrimitiveVertex currentVertex;
    int verticesAdded = 0;

    for (int i = 0; i < 3; ++i)
    {
        currentVertex = vertices[indices[i]];
        Vec2 triangleEdgeDirection = currentVertex.xy - lastVertex.xy;
        Vec2 currentPointVector = (currentVertex.xy - lineOrigin);
        Vec2 lastPointVector = (lastVertex.xy - lineOrigin);
        float currentPointDeterminant = (lineVector.x * currentPointVector.y) - (lineVector.y * currentPointVector.x);
        float lastPointDeterminant = (lineVector.x * lastPointVector.y) - (lineVector.y * lastPointVector.x);
        const float epsilon = 0.001f;

        Vec2 perpendicularLineVector(-lineVector.y, lineVector.x);
        Vec2 vertexToLine = lineOrigin - lastVertex.xy;

        if (currentPointDeterminant < epsilon)
        {
            if (lastPointDeterminant > -epsilon && fabs(currentPointDeterminant) > epsilon && fabs(lastPointDeterminant) > epsilon)
            {
                //add calculated intersection
                float intersectionDistance = (vertexToLine.x * perpendicularLineVector.x + vertexToLine.y * perpendicularLineVector.y) / (triangleEdgeDirection.x * perpendicularLineVector.x + triangleEdgeDirection.y * perpendicularLineVector.y);
                LyShine::UiPrimitiveVertex intersectPoint;
                SetVertex(intersectPoint, lastVertex.xy + triangleEdgeDirection * intersectionDistance,
                    lastVertex.color.dcolor, lastVertex.st + (currentVertex.st - lastVertex.st) * intersectionDistance);

                renderVertices[vertexOffset] = intersectPoint;
                vertexOffset++;
                verticesAdded++;
            }
            //add currentvertex
            renderVertices[vertexOffset] = currentVertex;
            vertexOffset++;
            verticesAdded++;
        }
        else if (lastPointDeterminant < epsilon)
        {
            //add calculated intersection
            float intersectionDistance = (vertexToLine.x * perpendicularLineVector.x + vertexToLine.y * perpendicularLineVector.y) / (triangleEdgeDirection.x * perpendicularLineVector.x + triangleEdgeDirection.y * perpendicularLineVector.y);
            LyShine::UiPrimitiveVertex intersectPoint;
            SetVertex(intersectPoint, lastVertex.xy + triangleEdgeDirection * intersectionDistance,
                lastVertex.color.dcolor, lastVertex.st + (currentVertex.st - lastVertex.st) * intersectionDistance);

            renderVertices[vertexOffset] = intersectPoint;
            vertexOffset++;
            verticesAdded++;
        }
        lastVertex = currentVertex;
    }

    int indicesAdded = 0;
    if (verticesAdded == 3)
    {
        renderIndices[renderIndexOffset] = static_cast<uint16>(vertexOffset - 3);
        renderIndices[renderIndexOffset + 1] = static_cast<uint16>(vertexOffset - 2);
        renderIndices[renderIndexOffset + 2] = static_cast<uint16>(vertexOffset - 1);
        indicesAdded = 3;
    }
    else if (verticesAdded == 4)
    {
        renderIndices[renderIndexOffset] = static_cast<uint16>(vertexOffset - 4);
        renderIndices[renderIndexOffset + 1] = static_cast<uint16>(vertexOffset - 3);
        renderIndices[renderIndexOffset + 2] = static_cast<uint16>(vertexOffset - 2);

        renderIndices[renderIndexOffset + 3] = static_cast<uint16>(vertexOffset - 4);
        renderIndices[renderIndexOffset + 4] = static_cast<uint16>(vertexOffset - 2);
        renderIndices[renderIndexOffset + 5] = static_cast<uint16>(vertexOffset - 1);
        indicesAdded = 6;
    }


    return indicesAdded;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::RenderTriangleList(const LyShine::UiPrimitiveVertex* vertices, const uint16* indices, int numVertices, int numIndices)
{
    if (numVertices != m_cachedPrimitive.m_numVertices)
    {
        ClearCachedVertices();
        m_cachedPrimitive.m_vertices = new LyShine::UiPrimitiveVertex[numVertices];
        m_cachedPrimitive.m_numVertices = numVertices;
    }

    if (numIndices != m_cachedPrimitive.m_numIndices)
    {
        ClearCachedIndices();
        m_cachedPrimitive.m_indices = new uint16[numIndices];
        m_cachedPrimitive.m_numIndices = numIndices;
    }

    memcpy(m_cachedPrimitive.m_vertices, vertices, sizeof(LyShine::UiPrimitiveVertex) * numVertices);
    memcpy(m_cachedPrimitive.m_indices, indices, sizeof(uint16) * numIndices);

    m_isRenderCacheDirty = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ClearCachedVertices()
{
    if (m_cachedPrimitive.m_vertices)
    {
        delete [] m_cachedPrimitive.m_vertices;
    }
    m_cachedPrimitive.m_vertices = nullptr;
    m_cachedPrimitive.m_numVertices = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::ClearCachedIndices()
{
    if (m_cachedPrimitive.m_indices)
    {
        delete [] m_cachedPrimitive.m_indices;
    }
    m_cachedPrimitive.m_indices = nullptr;
    m_cachedPrimitive.m_numIndices = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::MarkRenderCacheDirty()
{
    m_isRenderCacheDirty = true;

    MarkRenderGraphDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::MarkRenderGraphDirty()
{
    // tell the canvas to invalidate the render graph (never want to do this while rendering)
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    UiCanvasComponentImplementationBus::Event(canvasEntityId, &UiCanvasComponentImplementationBus::Events::MarkRenderGraphDirty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::SnapOffsetsToFixedImage()
{
    // check that the element is using transform2d - if not then can't adjust the offsets
    if (!UiTransform2dBus::FindFirstHandler(GetEntityId()))
    {
        return;
    }

    // if the image has no texture it will not use Fixed rendering so do nothing
    AZ::Data::Instance<AZ::RPI::Image> image = GetSpriteImage(m_sprite);
    if (!image)
    {
        return;
    }

    // Check that this element is not controlled by a parent layout component
    AZ::EntityId parentElementId;
    UiElementBus::EventResult(parentElementId, GetEntityId(), &UiElementBus::Events::GetParentEntityId);
    bool isControlledByParent = false;
    UiLayoutBus::EventResult(isControlledByParent, parentElementId, &UiLayoutBus::Events::IsControllingChild, GetEntityId());
    if (isControlledByParent)
    {
        return;
    }

    // get the anchors and offsets from the element's transform component
    UiTransform2dInterface::Anchors anchors;
    UiTransform2dInterface::Offsets offsets;
    UiTransform2dBus::EventResult(anchors, GetEntityId(), &UiTransform2dBus::Events::GetAnchors);
    UiTransform2dBus::EventResult(offsets, GetEntityId(), &UiTransform2dBus::Events::GetOffsets);

    // Get the size of the element rect before scale/rotate
    UiTransformInterface::RectPoints points;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetCanvasSpacePointsNoScaleRotate, points);
    AZ::Vector2 rectSize = points.GetAxisAlignedSize();

    // get the texture size
    AZ::Vector2 textureSize = m_sprite->GetCellSize(m_spriteSheetCellIndex);

    // calculate difference in the current rect size and the texture size
    AZ::Vector2 sizeDiff = textureSize - rectSize;

    // get the pivot of the element, the fixed image will render the texture aligned with the pivot
    AZ::Vector2 pivot;
    UiTransformBus::EventResult(pivot, GetEntityId(), &UiTransformBus::Events::GetPivot);

    // if the anchors are together (no stretching) in either dimension
    // and that dimension is not controlled by a LayoutFitter
    // then adjust the offsets in that dimension to fit the texture
    bool offsetsChanged = false;
    if (anchors.m_left == anchors.m_right && !UiLayoutHelpers::IsControlledByHorizontalFit(GetEntityId()))
    {
        offsets.m_left -= sizeDiff.GetX() * pivot.GetX();
        offsets.m_right += sizeDiff.GetX() * (1.0f - pivot.GetX());
        offsetsChanged = true;
    }

    if (anchors.m_top == anchors.m_bottom && !UiLayoutHelpers::IsControlledByVerticalFit(GetEntityId()))
    {
        offsets.m_top -= sizeDiff.GetY() * pivot.GetY();
        offsets.m_bottom += sizeDiff.GetY() * (1.0f - pivot.GetY());
        offsetsChanged = true;
    }

    if (offsetsChanged)
    {
        UiTransform2dBus::Event(GetEntityId(), &UiTransform2dBus::Events::SetOffsets, offsets);
        UiEditorChangeNotificationBus::Broadcast(&UiEditorChangeNotificationBus::Events::OnEditorTransformPropertiesNeedRefresh);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsPixelAligned()
{
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    bool isPixelAligned = true;
    UiCanvasBus::EventResult(isPixelAligned, canvasEntityId, &UiCanvasBus::Events::GetIsPixelAligned);
    return isPixelAligned;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsSpriteTypeAsset()
{
    return m_spriteType == UiImageInterface::SpriteType::SpriteAsset;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsSpriteTypeSpriteSheet()
{
    return GetImageIndexCount() > 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsSpriteTypeRenderTarget()
{
    return (m_spriteType == UiImageInterface::SpriteType::RenderTarget);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsFilled()
{
    return (m_fillType != FillType::None);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsLinearFilled()
{
    return (m_fillType == FillType::Linear);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsRadialFilled()
{
    return (m_fillType == FillType::Radial);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsRadialAnyFilled()
{
    return (m_fillType == FillType::Radial) || (m_fillType == FillType::RadialCorner) || (m_fillType == FillType::RadialEdge);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsCornerFilled()
{
    return (m_fillType == FillType::RadialCorner);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsEdgeFilled()
{
    return (m_fillType == FillType::RadialEdge) || (m_fillType == FillType::Linear);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::IsSliced()
{
    return m_imageType == ImageType::Sliced;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnEditorSpritePathnameChange()
{
    OnSpritePathnameChange();

    if (m_imageType == ImageType::Fixed)
    {
        SnapOffsetsToFixedImage();
    }

    UiEditorChangeNotificationBus::Broadcast(&UiEditorChangeNotificationBus::Events::OnEditorPropertiesRefreshEntireTree);
    CheckLayoutFitterAndRefreshEditorTransformProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnEditorSpriteTypeChange()
{
    OnSpriteTypeChange();

    if (m_imageType == ImageType::Fixed)
    {
        SnapOffsetsToFixedImage();
    }

    CheckLayoutFitterAndRefreshEditorTransformProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnEditorImageTypeChange()
{
    if (m_imageType == ImageType::Fixed)
    {
        SnapOffsetsToFixedImage();
    }

    InvalidateLayouts();
    CheckLayoutFitterAndRefreshEditorTransformProperties();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnEditorRenderSettingChange()
{
    // something changed in the properties that requires re-rendering
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnSpritePathnameChange()
{
    ISprite* newSprite = nullptr;

    if (!m_spritePathname.GetAssetPath().empty())
    {
        // Load the new texture.
        newSprite = AZ::Interface<ILyShine>::Get()->LoadSprite(m_spritePathname.GetAssetPath().c_str());
    }

    // if listening for notifications from a current sprite then disconnect
    if (UiSpriteSettingsChangeNotificationBus::Handler::BusIsConnected())
    {
        UiSpriteSettingsChangeNotificationBus::Handler::BusDisconnect();
    }

    SAFE_RELEASE(m_sprite);
    m_sprite = newSprite;

    // Listen for change notifications from the new sprite
    if (m_sprite)
    {
        UiSpriteSettingsChangeNotificationBus::Handler::BusConnect(m_sprite);
    }

    InvalidateLayouts();
    ResetSpriteSheetCellIndex();
    UiSpriteSourceNotificationBus::Event(GetEntityId(), &UiSpriteSourceNotificationBus::Events::OnSpriteSourceChanged);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnSpriteAttachmentImageAssetChange()
{
    ISprite* newSprite = nullptr;

    if (m_attachmentImageAsset)
    {
        newSprite = AZ::Interface<ILyShine>::Get()->CreateSprite(m_attachmentImageAsset);
    }

    SAFE_RELEASE(m_sprite);
    m_sprite = newSprite;

    InvalidateLayouts();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnSpriteTypeChange()
{
    if (IsSpriteTypeAsset())
    {
        OnSpritePathnameChange();
    }
    else if (m_spriteType == UiImageInterface::SpriteType::RenderTarget)
    {
        OnSpriteAttachmentImageAssetChange();
    }
    else
    {
        AZ_Assert(false, "unhandled sprite type");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnColorChange()
{
    m_overrideColor = m_color;
    m_overrideAlpha = m_alpha;
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::InvalidateLayouts()
{
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    UiLayoutManagerBus::Event(
        canvasEntityId, &UiLayoutManagerBus::Events::MarkToRecomputeLayoutsAffectedByLayoutCellChange, GetEntityId(), true);
    UiLayoutManagerBus::Event(canvasEntityId, &UiLayoutManagerBus::Events::MarkToRecomputeLayout, GetEntityId());

    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::OnIndexChange()
{
    // Index update logic will go here
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::AZu32ComboBoxVec UiImageComponent::PopulateIndexStringList() const
{
    // There may not be a sprite loaded for this component
    if (m_sprite)
    {
        const AZ::u32 numCells = static_cast<AZ::u32>(m_sprite->GetSpriteSheetCells().size());

        if (numCells != 0)
        {
            return LyShine::GetEnumSpriteIndexList(GetEntityId(), 0, numCells - 1);
        }
    }

    return LyShine::AZu32ComboBoxVec();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageComponent::CheckLayoutFitterAndRefreshEditorTransformProperties() const
{
    UiLayoutHelpers::CheckFitterAndRefreshEditorTransformProperties(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1:
    // - Need to convert CryString elements to AZStd::string
    // - Need to convert Color to Color and Alpha
    AZ_Assert(classElement.GetVersion() > 1, "Unsupported UiImageComponent version: %d", classElement.GetVersion());

    // conversion from version 1 or 2 to current:
    // - Need to convert AZStd::string sprites to AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>
    if (classElement.GetVersion() <= 2)
    {
        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "SpritePath"))
        {
            return false;
        }
    }

    // conversion from version 3 to current:
    // - Strip off any leading forward slashes from sprite path
    if (classElement.GetVersion() <= 3)
    {
        if (!LyShine::RemoveLeadingForwardSlashesFromAssetPath(context, classElement, "SpritePath"))
        {
            return false;
        }
    }

    // conversion from version 4 to current:
    // - Need to convert AZ::Vector3 to AZ::Color
    if (classElement.GetVersion() <= 4)
    {
        if (!LyShine::ConvertSubElementFromVector3ToAzColor(context, classElement, "Color"))
        {
            return false;
        }
    }

    // conversion to version 8:
    // - Need to remove render target name as it was replaced with attachment image asset
    if (classElement.GetVersion() <= 7)
    {
        if (!LyShine::RemoveRenderTargetAsString(context, classElement, "RenderTargetName"))
        {
            return false;
        }
    }

    return true;
}
