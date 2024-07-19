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
#include <LyShine/Bus/UiImageSequenceBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiIndexableImageBus.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/UiRenderFormats.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <LmbrCentral/Rendering/TextureAsset.h>

//! \brief Image component capable of indexing and displaying from multiple image files in a directory.
//!
//! This component offers functionality similar to a sprite-sheet being used with
//! an image component. Instead of indexing multiple images mapped within a single
//! sprite-sheet, this component indexes multiple image files.
//!
//! Note that this only supports fixed image types - the image component is more
//! fully featured for rendering images.
class UiImageSequenceComponent
    : public AZ::Component
    , public UiVisualBus::Handler
    , public UiRenderBus::Handler
    , public UiImageSequenceBus::Handler
    , public UiIndexableImageBus::Handler
    , public UiTransformChangeNotificationBus::Handler
    , public UiCanvasPixelAlignmentNotificationBus::Handler
    , public UiEditorRefreshDirectoryNotificationBus::Handler
{
public: // types

    using TextureAssetRef = AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>;
    using ImageList = AZStd::vector<TextureAssetRef>;
    using SpriteList = AZStd::vector<ISprite*>;

public: // member functions

    AZ_COMPONENT(UiImageSequenceComponent, LyShine::UiImageSequenceComponentUuid, AZ::Component);

    UiImageSequenceComponent();
    ~UiImageSequenceComponent() override;

    // UiEditorRefreshDirectoryNotificationInterface
    void OnRefreshDirectory() override { OnImageSequenceDirectoryChange(); }
    // ~UiEditorRefreshDirectoryNotificationInterface

    // UiVisualInterface
    void ResetOverrides() override {}
    void SetOverrideColor(const AZ::Color& /* color */) override {}
    void SetOverrideAlpha(float /* alpha */) override {}
    void SetOverrideSprite(ISprite* /* sprite */, AZ::u32 /*cellIndex = 0 */) override {}
    // ~UiVisualInterface

    // UiRenderInterface
    void Render(LyShine::IRenderGraph* renderGraph) override;
    // ~UiRenderInterface

    // UiImageSequenceInterface
    ImageType GetImageType() override;
    void SetImageType(ImageType imageType) override;
    // ~UiImageSequenceInterface

    // UiIndexableImageBus
    void SetImageIndex(AZ::u32 index) override;
    const AZ::u32 GetImageIndex() override;
    const AZ::u32 GetImageIndexCount() override;
    AZStd::string GetImageIndexAlias(AZ::u32 index) override;
    void SetImageIndexAlias(AZ::u32 index, const AZStd::string& alias) override;
    AZ::u32 GetImageIndexFromAlias(const AZStd::string& alias) override;
    // ~UiIndexableImageBus

    // UiTransformChangeNotification
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    void OnTransformToViewportChanged() override;
    // ~UiTransformChangeNotification

    // UiCanvasPixelAlignmentNotification
    void OnCanvasPixelAlignmentChange() override;
    // ~UiCanvasPixelAlignmentNotification

public: // static member functions

    static void Reflect(AZ::ReflectContext* context);

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiVisualService"));
        provided.push_back(AZ_CRC_CE("UiIndexableImageService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiVisualService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

protected: // member functions

    // AZ::Component
    void Init() override;
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

private: // member functions

    AZ_DISABLE_COPY_MOVE(UiImageSequenceComponent);

    LyShine::AZu32ComboBoxVec PopulateIndexStringList();

    void OnImageTypeChange();
    void OnImageSequenceDirectoryChange();
    void OnImageSequenceIndexChange();

    void RenderStretchedSprite(ISprite* sprite, int cellIndex, uint32 packedColor);
    void RenderFixedSprite(ISprite* sprite, int cellIndex, uint32 packedColor);
    void RenderStretchedToFitOrFillSprite(ISprite* sprite, int cellIndex, uint32 packedColor, bool toFit);
    void RenderSingleQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor);
    bool IsPixelAligned();
    void RenderTriangleList(const LyShine::UiPrimitiveVertex* vertices, const uint16* indices, int numVertices, int numIndices);
    void ClearCachedVertices();
    void ClearCachedIndices();
    void MarkRenderCacheDirty();

private: // static member functions

    static bool VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement);

private: // data

    ImageList m_imageList;                          //!< List of image files to load 
    SpriteList m_spriteList;                        //!< List of loaded sprites

    AZStd::string m_imageSequenceDirectory;         //!< Used to populate m_imageList, only populated from the editor
    AZ::u32 m_sequenceIndex = 0;                    //!< Index of image currently displayed
    ImageType m_imageType = ImageType::Fixed;       //!< Affects how the texture/sprite is mapped to the image rectangle

    // cached rendering data for performance optimization
    LyShine::UiPrimitive m_cachedPrimitive;
    bool m_isRenderCacheDirty = true;
};
