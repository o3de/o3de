/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiImageSequenceComponent.h"
#include "Sprite.h"
#include "RenderGraph.h"

#include <LyShine/IDraw2d.h>
#include <LyShine/ISprite.h>
#include <LyShine/IRenderGraph.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/Sprite/UiSpriteBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace
{
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

    //! \brief Loads assets from disk and populates the sprite list with loaded sprites.
    void PopulateSpriteListFromImageList(UiImageSequenceComponent::SpriteList& spriteList, UiImageSequenceComponent::ImageList& imageList)
    {
        AZStd::unordered_set<AZStd::string> invalidTextures;
        spriteList.clear();
        spriteList.reserve(imageList.size());
        for (auto& textureAssetRef : imageList)
        {
            ISprite* sprite = AZ::Interface<ILyShine>::Get()->LoadSprite(textureAssetRef.GetAssetPath().c_str());
            if (sprite)
            {
                spriteList.push_back(sprite);
            }
            else
            {
                invalidTextures.insert(textureAssetRef.GetAssetPath());
            }
        }

        auto newEndIter = AZStd::remove_if(imageList.begin(), imageList.end(), 
            [&invalidTextures](const UiImageSequenceComponent::TextureAssetRef& assetRef)
            {
                return invalidTextures.find(assetRef.GetAssetPath()) != invalidTextures.end();
            });
        imageList.erase(newEndIter, imageList.end());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageSequenceComponent::UiImageSequenceComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageSequenceComponent::~UiImageSequenceComponent()
{
    for (ISprite* sprite : m_spriteList)
    {
        SAFE_RELEASE(sprite);
    }

    ClearCachedVertices();
    ClearCachedIndices();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::Render(LyShine::IRenderGraph* renderGraph)
{
    if (m_spriteList.empty())
    {
        return;
    }

    ISprite* sprite = m_spriteList[m_sequenceIndex];

    // get fade value (tracked by UiRenderer) and compute the desired alpha for the image
    float fade = renderGraph->GetAlphaFade();
    uint8 desiredPackedAlpha = static_cast<uint8>(fade * 255.0f);

    if (m_isRenderCacheDirty)
    {
        uint32 packedColor = 0xffffffff;
        switch (m_imageType)
        {
        case ImageType::Stretched:
            RenderStretchedSprite(sprite, 0, packedColor);
            break;
        case ImageType::Fixed:
            AZ_Assert(sprite, "Should not get here if no sprite path is specified");
            RenderFixedSprite(sprite, 0, packedColor);
            break;
        case ImageType::StretchedToFit:
            AZ_Assert(sprite, "Should not get here if no sprite path is specified");
            RenderStretchedToFitOrFillSprite(sprite, 0, packedColor, true);
            break;
        case ImageType::StretchedToFill:
            AZ_Assert(sprite, "Should not get here if no sprite path is specified");
            RenderStretchedToFitOrFillSprite(sprite, 0, packedColor, false);
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

        AZ::Data::Instance<AZ::RPI::Image> image;
        if (sprite)
        {
            image = sprite->GetImage();
        }
        bool isClampTextureMode = false;
        bool isTextureSRGB = false;
        bool isTexturePremultipliedAlpha = false;
        LyShine::BlendMode blendMode = LyShine::BlendMode::Normal;

        // Add the quad to the render graph
        renderGraph->AddPrimitive(&m_cachedPrimitive, image,
            isClampTextureMode, isTextureSRGB, isTexturePremultipliedAlpha, blendMode);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiImageSequenceComponent::ImageType UiImageSequenceComponent::GetImageType()
{
    return m_imageType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::SetImageType(ImageType imageType)
{
    if (m_imageType != imageType)
    {
        m_imageType = imageType;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiImageSequenceComponent, AZ::Component>()
            ->Version(0, &VersionConverter)
            ->Field("ImageType", &UiImageSequenceComponent::m_imageType)
            ->Field("ImageList", &UiImageSequenceComponent::m_imageList)
            ->Field("ImageSequenceDirectory", &UiImageSequenceComponent::m_imageSequenceDirectory)
            ->Field("Index", &UiImageSequenceComponent::m_sequenceIndex);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiImageSequenceComponent>("ImageSequence", "A visual component that displays one of multiple images in a sequence.");

            // :TODO: update the icon for image sequence
            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiImage.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiImage.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageSequenceComponent::m_imageType, "ImageType", "The image type. Affects how the texture/sprite is mapped to the image rectangle.")
                ->EnumAttribute(UiImageSequenceInterface::ImageType::Stretched, "Stretched")
                ->EnumAttribute(UiImageSequenceInterface::ImageType::Fixed, "Fixed")
                ->EnumAttribute(UiImageSequenceInterface::ImageType::StretchedToFit, "Stretched To Fit")
                ->EnumAttribute(UiImageSequenceInterface::ImageType::StretchedToFill, "Stretched To Fill")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageSequenceComponent::OnImageTypeChange);
            editInfo->DataElement("Directory", &UiImageSequenceComponent::m_imageSequenceDirectory, "Sequence Directory", "A directory containing images of the sequence.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageSequenceComponent::OnImageSequenceDirectoryChange)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));
            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiImageSequenceComponent::m_sequenceIndex, "Sequence Index", "Image index to display.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiImageSequenceComponent::OnImageSequenceIndexChange)
                ->Attribute("EnumValues", &UiImageSequenceComponent::PopulateIndexStringList);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->Enum<(int)UiImageSequenceInterface::ImageType::Stretched>("eUiImageSequenceImageType_Stretched")
            ->Enum<(int)UiImageSequenceInterface::ImageType::Fixed>("eUiImageSequenceImageType_Fixed")
            ->Enum<(int)UiImageSequenceInterface::ImageType::StretchedToFit>("eUiImageSequenceImageType_StretchedToFit")
            ->Enum<(int)UiImageSequenceInterface::ImageType::StretchedToFill>("eUiImageSequenceImageType_StretchedToFill");

        behaviorContext->EBus<UiImageSequenceBus>("UiImageSequenceBus")
            ->Event("GetImageType", &UiImageSequenceBus::Events::GetImageType)
            ->Event("SetImageType", &UiImageSequenceBus::Events::SetImageType);

        behaviorContext->Class<UiImageSequenceComponent>()->RequestBus("UiImageSequenceBus");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::SetImageIndex(AZ::u32 index)
{
    if (index < m_spriteList.size())
    {
        m_sequenceIndex = index;
        MarkRenderCacheDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZ::u32 UiImageSequenceComponent::GetImageIndex()
{
    return m_sequenceIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const AZ::u32 UiImageSequenceComponent::GetImageIndexCount()
{
    return static_cast<AZ::u32>(m_spriteList.size());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiImageSequenceComponent::GetImageIndexAlias([[maybe_unused]] AZ::u32 index)
{
    return AZStd::string();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::SetImageIndexAlias([[maybe_unused]] AZ::u32 index, [[maybe_unused]] const AZStd::string& alias)
{
    // Purposefully empty
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::u32 UiImageSequenceComponent::GetImageIndexFromAlias([[maybe_unused]] const AZStd::string& alias)
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::OnCanvasSpaceRectChanged(AZ::EntityId /*entityId*/, const UiTransformInterface::Rect& /*oldRect*/, const UiTransformInterface::Rect& /*newRect*/)
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::OnTransformToViewportChanged()
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::OnCanvasPixelAlignmentChange()
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
LyShine::AZu32ComboBoxVec UiImageSequenceComponent::PopulateIndexStringList()
{
    const AZ::u32 indexCount = GetImageIndexCount();
    return LyShine::GetEnumSpriteIndexList(GetEntityId(), 0, indexCount - 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::OnImageTypeChange()
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::OnImageSequenceDirectoryChange()
{
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    if (fileIO)
    {
        // Add all files in the directory - we'll try to load them all as sprites and toss
        // out the ones that don't later.
        AZStd::set<AZStd::string> spriteFilepaths;
        fileIO->FindFiles(m_imageSequenceDirectory.c_str(), "*", [&spriteFilepaths](const char* pFilename) -> bool
        {
            spriteFilepaths.insert(pFilename);
            return true;
        });

        // Take all non *.sprite files and look for *.sprite counterparts; if they exist,
        // then keep the *.sprite counterpart but removed the other image from the
        // list (to prevent loading duplicate images).
        for (auto iter = spriteFilepaths.begin(); iter != spriteFilepaths.end(); )
        {
            AZStd::string filepathCopy = *iter;
            AZStd::string fileExtension;
            const bool hasExtension = AzFramework::StringFunc::Path::GetExtension(filepathCopy.c_str(), fileExtension);
            const bool checkForSpriteFile = hasExtension ? fileExtension != ".sprite" : false;
            if (checkForSpriteFile)
            {
                AzFramework::StringFunc::Path::ReplaceExtension(filepathCopy, "sprite");
                const bool spriteFileExists = spriteFilepaths.find(filepathCopy) != spriteFilepaths.end();
                if (spriteFileExists)
                {
                    iter = spriteFilepaths.erase(iter);
                    continue;
                }
            }
            ++iter;
        }

        // Build list of TextureAssetRefs from list of paths that contain *.sprite
        // files (for those images that have them)
        m_imageList.clear();
        for (const auto& spriteFilepath : spriteFilepaths)
        {
            TextureAssetRef textureAsset;
            textureAsset.SetAssetPath(spriteFilepath.c_str());
            m_imageList.push_back(textureAsset);
        }

        // Finally, load the sprites in the sequence and notify listeners accordingly
        PopulateSpriteListFromImageList(m_spriteList, m_imageList);
        m_sequenceIndex = 0;
        MarkRenderCacheDirty();
        UiSpriteSourceNotificationBus::Event(GetEntityId(), &UiSpriteSourceNotificationBus::Events::OnSpriteSourceChanged);
        UiEditorChangeNotificationBus::Broadcast(&UiEditorChangeNotificationBus::Events::OnEditorPropertiesRefreshEntireTree);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::OnImageSequenceIndexChange()
{
    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::Init()
{
    // If this is called from RC.exe for example these pointers will not be set. In that case
    // we only need to be able to load, init and save the component. It will never be
    // activated.
    if (!AZ::Interface<ILyShine>::Get())
    {
        return;
    }

    PopulateSpriteListFromImageList(m_spriteList, m_imageList);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::Activate()
{
    UiIndexableImageBus::Handler::BusConnect(m_entity->GetId());
    UiEditorRefreshDirectoryNotificationBus::Handler::BusConnect();
    UiRenderBus::Handler::BusConnect(m_entity->GetId());
    UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());

    MarkRenderCacheDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::Deactivate()
{
    UiIndexableImageBus::Handler::BusDisconnect();
    UiEditorRefreshDirectoryNotificationBus::Handler::BusDisconnect();
    UiRenderBus::Handler::BusDisconnect();
    UiTransformChangeNotificationBus::Handler::BusDisconnect();

    if (UiCanvasPixelAlignmentNotificationBus::Handler::BusIsConnected())
    {
        UiCanvasPixelAlignmentNotificationBus::Handler::BusDisconnect();
    }

    // reduce memory use on deactivate
    ClearCachedVertices();
    ClearCachedIndices();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::RenderStretchedSprite(ISprite* sprite, int cellIndex, uint32 packedColor)
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
        RenderSingleQuad(points.pt, uvs, packedColor);
    }
    else
    {
        // points are a clockwise quad
        static const AZ::Vector2 uvs[4] = { AZ::Vector2(0, 0), AZ::Vector2(1, 0), AZ::Vector2(1, 1), AZ::Vector2(0, 1) };
        RenderSingleQuad(points.pt, uvs, packedColor);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::RenderFixedSprite(ISprite* sprite, int cellIndex, uint32 packedColor)
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

    RenderSingleQuad(points.pt, uvs, packedColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::RenderStretchedToFitOrFillSprite(ISprite* sprite, int cellIndex, uint32 packedColor, bool toFit)
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
    RenderSingleQuad(points.pt, uvs, packedColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::RenderSingleQuad(const AZ::Vector2* positions, const AZ::Vector2* uvs, uint32 packedColor)
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
void UiImageSequenceComponent::RenderTriangleList(const LyShine::UiPrimitiveVertex* vertices, const uint16* indices, int numVertices, int numIndices)
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
void UiImageSequenceComponent::ClearCachedVertices()
{
    if (m_cachedPrimitive.m_vertices)
    {
        delete[] m_cachedPrimitive.m_vertices;
    }
    m_cachedPrimitive.m_vertices = nullptr;
    m_cachedPrimitive.m_numVertices = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::ClearCachedIndices()
{
    if (m_cachedPrimitive.m_indices)
    {
        delete[] m_cachedPrimitive.m_indices;
    }
    m_cachedPrimitive.m_indices = nullptr;
    m_cachedPrimitive.m_numIndices = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiImageSequenceComponent::MarkRenderCacheDirty()
{
    m_isRenderCacheDirty = true;

    // tell the canvas to invalidate the render graph (never want to do this while rendering)
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    UiCanvasComponentImplementationBus::Event(canvasEntityId, &UiCanvasComponentImplementationBus::Events::MarkRenderGraphDirty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageSequenceComponent::IsPixelAligned()
{
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    bool isPixelAligned = true;
    UiCanvasBus::EventResult(isPixelAligned, canvasEntityId, &UiCanvasBus::Events::GetIsPixelAligned);
    return isPixelAligned;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiImageSequenceComponent::VersionConverter([[maybe_unused]] AZ::SerializeContext& context,
    [[maybe_unused]] AZ::SerializeContext::DataElementNode& classElement)
{
    return true;
}
