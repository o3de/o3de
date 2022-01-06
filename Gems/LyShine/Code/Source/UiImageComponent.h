/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorPropertyTypes.h"

#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiImageBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiAnimateEntityBus.h>
#include <LyShine/Bus/UiLayoutCellDefaultBus.h>
#include <LyShine/Bus/Sprite/UiSpriteBus.h>
#include <LyShine/Bus/UiIndexableImageBus.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/UiRenderFormats.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Vector3.h>

#include <LmbrCentral/Rendering/MaterialAsset.h>

class ITexture;
class ISprite;

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiImageComponent
    : public AZ::Component
    , public UiVisualBus::Handler
    , public UiRenderBus::Handler
    , public UiImageBus::Handler
    , public UiAnimateEntityBus::Handler
    , public UiTransformChangeNotificationBus::Handler
    , public UiLayoutCellDefaultBus::Handler
    , public UiCanvasPixelAlignmentNotificationBus::Handler
    , public UiSpriteSettingsChangeNotificationBus::Handler
    , public UiIndexableImageBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiImageComponent, LyShine::UiImageComponentUuid, AZ::Component);

    UiImageComponent();
    ~UiImageComponent() override;

    // UiVisualInterface
    void ResetOverrides() override;
    void SetOverrideColor(const AZ::Color& color) override;
    void SetOverrideAlpha(float alpha) override;
    void SetOverrideSprite(ISprite* sprite, AZ::u32 cellIndex = 0) override;
    // ~UiVisualInterface

    // UiRenderInterface
    void Render(LyShine::IRenderGraph* renderGraph) override;
    // ~UiRenderInterface

    // UiImageInterface
    AZ::Color GetColor() override;
    void SetColor(const AZ::Color& color) override;
    float GetAlpha() override;
    void SetAlpha(float alpha) override;
    ISprite* GetSprite() override;
    void SetSprite(ISprite* sprite) override;
    AZStd::string GetSpritePathname() override;
    void SetSpritePathname(AZStd::string spritePath) override;
    bool SetSpritePathnameIfExists(AZStd::string spritePath) override;
    AZStd::string GetRenderTargetName() override;
    void SetRenderTargetName(AZStd::string renderTargetName) override;
    bool GetIsRenderTargetSRGB() override;
    void SetIsRenderTargetSRGB(bool isSRGB) override;
    SpriteType GetSpriteType() override;
    void SetSpriteType(SpriteType spriteType) override;
    ImageType GetImageType() override;
    void SetImageType(ImageType imageType) override;
    FillType GetFillType() override;
    void SetFillType(FillType fillType) override;
    float GetFillAmount() override;
    void SetFillAmount(float fillAmount) override;
    float GetRadialFillStartAngle() override;
    void SetRadialFillStartAngle(float radialFillStartAngle) override;
    FillCornerOrigin GetCornerFillOrigin() override;
    void SetCornerFillOrigin(FillCornerOrigin cornerOrigin) override;
    FillEdgeOrigin GetEdgeFillOrigin() override;
    void SetEdgeFillOrigin(FillEdgeOrigin edgeOrigin) override;
    bool GetFillClockwise() override;
    void SetFillClockwise(bool fillClockwise) override;
    bool GetFillCenter() override;
    void SetFillCenter(bool fillCenter) override;
    // ~UiImageInterface

    // UiIndexableImageBus
    void SetImageIndex(AZ::u32 index) override;
    const AZ::u32 GetImageIndex() override;
    const AZ::u32 GetImageIndexCount() override;
    AZStd::string GetImageIndexAlias(AZ::u32 index) override;
    void SetImageIndexAlias(AZ::u32 index, const AZStd::string& alias) override;
    AZ::u32 GetImageIndexFromAlias(const AZStd::string& alias) override;
    // ~UiIndexableImageBus

    // UiAnimateEntityInterface
    void PropertyValuesChanged() override;
    // ~UiAnimateEntityInterface

    // UiTransformChangeNotification
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    void OnTransformToViewportChanged() override;
    // ~UiTransformChangeNotification

    // UiLayoutCellDefaultInterface
    float GetMinWidth() override;
    float GetMinHeight() override;
    float GetTargetWidth(float maxWidth) override;
    float GetTargetHeight(float maxHeight) override;
    float GetExtraWidthRatio() override;
    float GetExtraHeightRatio() override;
    // ~UiLayoutCellDefaultInterface

    // UiCanvasPixelAlignmentNotification
    void OnCanvasPixelAlignmentChange() override;
    // ~UiCanvasPixelAlignmentNotification

    // UiSpriteSettingsChangeNotification
    void OnSpriteSettingsChanged() override;
    // ~UiSpriteSettingsChangeNotification

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
        provided.push_back(AZ_CRC("UiImageService"));
        provided.push_back(AZ_CRC("UiIndexableImageService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiVisualService", 0xa864fdf8));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    //! Resets the current sprite-sheet cell index based on whether we're showing a sprite or a sprite-sheet.
    //!
    //! This is necessary since the render routines reference the sprite-sheet cell index regardless of
    //! whether a sprite-sheet is being displayed or not. It's possible to have a sprite-sheet asset loaded
    //! but the image component sprite type be a basic sprite. In that case, indexing the sprite-sheet is
    //! still technically possible, so we assign a special index (-1) to indicate not to index a particular
    //! cell, but rather the whole image.
    void ResetSpriteSheetCellIndex();

private: // member functions

    void RenderStretchedSprite(ISprite* sprite, int cellIndex, uint32 packedColor);
    void RenderSlicedSprite(ISprite* sprite, int cellIndex, uint32 packedColor);
    void RenderFixedSprite(ISprite* sprite, int cellIndex, uint32 packedColor);
    void RenderTiledSprite(ISprite* sprite, uint32 packedColor);
    void RenderStretchedToFitOrFillSprite(ISprite* sprite, int cellIndex, uint32 packedColor, bool toFit);

    void RenderSingleQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor);

    void RenderFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor);
    void RenderLinearFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor);
    void RenderRadialFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor);
    void RenderRadialCornerFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor);
    void RenderRadialEdgeFilledQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor);

    void RenderSlicedStretchedSprite(ISprite* sprite, int cellIndex, uint32 packedColor, const AZ::Matrix4x4& transform,
        const AZ::Vector2& textureSize, const UiTransformInterface::RectPoints& points,
        float leftBorder, float rightBorder, float topBorder, float bottomBorder);
    void RenderSlicedFixedSprite(ISprite* sprite, int cellIndex, uint32 packedColor, const AZ::Matrix4x4& transform,
        const AZ::Vector2& textureSize, const UiTransformInterface::RectPoints& points,
        float leftBorder, float rightBorder, float topBorder, float bottomBorder,
        float rectWidth, float rectHeight, float centerUvWidth, float centerUvHeight);

    template<uint32 numValues> void RenderSlicedFillModeNoneSprite(uint32 packedColor, const AZ::Matrix4x4& transform, float* xValues, float* yValues, float* sValues, float* tValues);
    template<uint32 numValues> void RenderSlicedLinearFilledSprite(uint32 packedColor, const AZ::Matrix4x4& transform, float* xValues, float* yValues, float* sValues, float* tValues);
    template<uint32 numValues> void RenderSlicedRadialFilledSprite(uint32 packedColor, const AZ::Matrix4x4& transform, float* xValues, float* yValues, float* sValues, float* tValues);
    template<uint32 numValues> void RenderSlicedRadialCornerOrEdgeFilledSprite(uint32 packedColor, const AZ::Matrix4x4& transform, float* xValues, float* yValues, float* sValues, float* tValues);

    void ClipValuesForSlicedLinearFill(uint32 numValues, float* xValues, float* yValues, float* sValues, float* tValues);
    void ClipAndRenderForSlicedRadialFill(uint32 numVertsPerside, uint32 numVerts, const LyShine::UiPrimitiveVertex* verts, uint32 totalIndices, const uint16* indices);
    void ClipAndRenderForSlicedRadialCornerOrEdgeFill(uint32 numVertsPerside, uint32 numVerts, const LyShine::UiPrimitiveVertex* verts, uint32 totalIndices, const uint16* indices);

    int ClipToLine(const LyShine::UiPrimitiveVertex* vertices, const uint16* indices, LyShine::UiPrimitiveVertex* newVertex, uint16* renderIndices, int& vertexOffset, int idxOffset, const Vec2& lineOrigin, const Vec2& lineEnd);

    void RenderTriangleList(const LyShine::UiPrimitiveVertex* vertices, const uint16* indices, int numVertices, int numIndices);
    void ClearCachedVertices();
    void ClearCachedIndices();
    void MarkRenderCacheDirty();
    void MarkRenderGraphDirty();

    void SnapOffsetsToFixedImage();

    bool IsPixelAligned();

    bool IsSpriteTypeAsset();
    bool IsSpriteTypeSpriteSheet();
    bool IsSpriteTypeRenderTarget();

    bool IsFilled();
    bool IsLinearFilled();
    bool IsRadialFilled();
    bool IsRadialAnyFilled();
    bool IsCornerFilled();
    bool IsEdgeFilled();

    bool IsSliced();

    AZ_DISABLE_COPY_MOVE(UiImageComponent);

    void OnEditorSpritePathnameChange();
    void OnEditorSpriteTypeChange();
    void OnEditorImageTypeChange();
    void OnEditorRenderSettingChange();

    void OnSpritePathnameChange();
    void OnSpriteRenderTargetNameChange();
    void OnSpriteTypeChange();

    //! ChangeNotify callback for color change
    void OnColorChange();

    //! Invalidate this element and its parent's layouts. Called when a property that
    //! is used to calculate default layout cell values has changed
    void InvalidateLayouts();

    //! Refresh the transform properties in the editor's properties pane
    void CheckLayoutFitterAndRefreshEditorTransformProperties() const;

    //! ChangeNotify callback for when the index string value selection changes.
    void OnIndexChange();

    //! Returns a string representation of the indices used to index sprite-sheet types.
    LyShine::AZu32ComboBoxVec PopulateIndexStringList() const;

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset> m_spritePathname;
    AZStd::string m_renderTargetName;
    bool m_isRenderTargetSRGB           = false;
    SpriteType m_spriteType             = SpriteType::SpriteAsset;
    AZ::Color m_color                   = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
    float m_alpha                       = 1.0f;
    ImageType m_imageType               = ImageType::Stretched;
    LyShine::BlendMode m_blendMode      = LyShine::BlendMode::Normal;

    ISprite* m_sprite                   = nullptr;

    ISprite* m_overrideSprite           = nullptr;
    AZ::u32 m_overrideSpriteCellIndex   = 0;
    AZ::Color m_overrideColor           = m_color;
    float m_overrideAlpha               = m_alpha;

    AZ::u32 m_spriteSheetCellIndex      = 0;    //!< Current index for sprite-sheet (if this sprite is a sprite-sheet type).

    FillType m_fillType                 = FillType::None;
    float m_fillAmount                  = 1.0f;
    float m_fillStartAngle              = 0.0f; // Start angle for fill measured in degrees clockwise.
    FillCornerOrigin m_fillCornerOrigin = FillCornerOrigin::TopLeft;
    FillEdgeOrigin m_fillEdgeOrigin     = FillEdgeOrigin::Left;
    bool m_fillClockwise                = true;
    bool m_fillCenter                   = true;

    bool m_isSlicingStretched           = true;  //!< When true the central parts of a 9-slice are stretched, when false the have the same pixel to texel ratio as the corners.

    bool m_isColorOverridden;
    bool m_isAlphaOverridden;

    // cached rendering data for performance optimization
    LyShine::UiPrimitive m_cachedPrimitive;
    bool m_isRenderCacheDirty = true;
};
