/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiCustomImageComponent.h"

#include <LyShineExamples/UiCustomImageBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/ISprite.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTransformBus.h>

namespace LyShineExamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PUBLIC MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiCustomImageComponent::UiCustomImageComponent()
        : m_color(1.f, 1.f, 1.f, 1.f)
        , m_alpha(1.f)
        , m_sprite(nullptr)
        , m_uvs(0, 0, 1, 1)
        , m_clamp(true)
        , m_overrideColor(m_color)
        , m_overrideAlpha(m_alpha)
        , m_overrideSprite(nullptr)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiCustomImageComponent::~UiCustomImageComponent()
    {
        SAFE_RELEASE(m_sprite);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::ResetOverrides()
    {
        m_overrideColor = m_color;
        m_overrideAlpha = m_alpha;
        m_overrideSprite = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetOverrideColor(const AZ::Color& color)
    {
        m_overrideColor.Set(color.GetAsVector3());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetOverrideAlpha(float alpha)
    {
        m_overrideAlpha = alpha;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetOverrideSprite(ISprite* sprite, AZ::u32 /* cellIndex */)
    {
        m_overrideSprite = sprite;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Render(LyShine::IRenderGraph* renderGraph)
    {
        // get fade value (tracked by UiRenderer) and compute the desired alpha for the image
        float fade = renderGraph->GetAlphaFade();
        float desiredAlpha = m_overrideAlpha * fade;
        uint8 desiredPackedAlpha = static_cast<uint8>(desiredAlpha * 255.0f);

        if (m_isRenderCacheDirty)
        {
            RenderToCache(renderGraph);
            m_isRenderCacheDirty = false;
        }

        // if desired alpha is zero then no need to do any more
        if (desiredPackedAlpha == 0)
        {
            return;
        }

        // Render cache is now valid - render using the cache

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

        ISprite* sprite = (m_overrideSprite) ? m_overrideSprite : m_sprite;
        AZ::Data::Instance<AZ::RPI::Image> image;

        if (sprite != nullptr)
        {
            image = sprite->GetImage();
        }

        bool isTextureSRGB = false;
        bool isTexturePremultipliedAlpha = false; // we are not rendering from a render target with alpha in it
        LyShine::BlendMode blendMode = LyShine::BlendMode::Normal;
        renderGraph->AddPrimitive(&m_cachedPrimitive, image, m_clamp, isTextureSRGB, isTexturePremultipliedAlpha, blendMode);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Color UiCustomImageComponent::GetColor()
    {
        return AZ::Color::CreateFromVector3AndFloat(m_color.GetAsVector3(), m_alpha);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetColor(const AZ::Color& color)
    {
        m_color.Set(color.GetAsVector3());
        m_alpha = color.GetA();
        m_overrideColor = m_color;
        m_overrideAlpha = m_alpha;
        MarkRenderCacheDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    ISprite* UiCustomImageComponent::GetSprite()
    {
        return m_sprite;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetSprite(ISprite* sprite)
    {
        if (m_sprite)
        {
            m_sprite->Release();
            m_spritePathname.SetAssetPath("");
        }

        m_sprite = sprite;

        if (m_sprite)
        {
            m_sprite->AddRef();
            m_spritePathname.SetAssetPath(m_sprite->GetPathname().c_str());
        }

        MarkRenderGraphDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string UiCustomImageComponent::GetSpritePathname()
    {
        return m_spritePathname.GetAssetPath();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetSpritePathname(AZStd::string spritePath)
    {
        m_spritePathname.SetAssetPath(spritePath.c_str());
        MarkRenderGraphDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiCustomImageInterface::UVRect UiCustomImageComponent::GetUVs()
    {
        return m_uvs;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetUVs(UiCustomImageInterface::UVRect uvs)
    {
        m_uvs = uvs;
        MarkRenderCacheDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool UiCustomImageComponent::GetClamp()
    {
        return m_clamp;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::SetClamp(bool clamp)
    {
        m_clamp = clamp;
        MarkRenderGraphDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::OnCanvasSpaceRectChanged(AZ::EntityId /*entityId*/, const UiTransformInterface::Rect& /*oldRect*/, const UiTransformInterface::Rect& /*newRect*/)
    {
        MarkRenderCacheDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::OnTransformToViewportChanged()
    {
        MarkRenderCacheDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PUBLIC STATIC MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);

        // Serialize this component
        if (serializeContext)
        {
            serializeContext->Class<UiCustomImageComponent, AZ::Component>()
                ->Field("SpritePath", &UiCustomImageComponent::m_spritePathname)
                ->Field("Color", &UiCustomImageComponent::m_color)
                ->Field("Alpha", &UiCustomImageComponent::m_alpha)
                ->Field("UVCoords", &UiCustomImageComponent::m_uvs)
                ->Field("Clamp", &UiCustomImageComponent::m_clamp);

            AZ::EditContext* ec = serializeContext->GetEditContext();
            if (ec)
            {
                auto editInfo = ec->Class<UiCustomImageComponent>("Custom Image", "A visual component to draw a rectangle with an optional sprite/texture");

                editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiImage.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiImage.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement("Sprite", &UiCustomImageComponent::m_spritePathname, "Sprite path", "The sprite path. Can be overridden by another component such as an interactable.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiCustomImageComponent::OnSpritePathnameChange);

                editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiCustomImageComponent::m_color, "Color", "The color tint for the image. Can be overridden by another component such as an interactable.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiCustomImageComponent::OnColorChange);

                editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiCustomImageComponent::m_alpha, "Alpha", "The transparency. Can be overridden by another component such as an interactable.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiCustomImageComponent::OnColorChange)
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f);

                editInfo->DataElement(0, &UiCustomImageComponent::m_uvs, "UV Rect", "The UV coordinates of the rectangle for rendering the texture.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiCustomImageComponent::OnRenderSettingChange)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshValues"))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show); // needed because sub-elements are hidden

                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiCustomImageComponent::m_clamp, "Clamp", "Whether the image should be clamped or not.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiCustomImageComponent::OnRenderSettingChange)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshValues"));
            }
        }

        if (behaviorContext)
        {
            behaviorContext->EBus<UiCustomImageBus>("UiCustomImageBus")
                ->Event("GetColor", &UiCustomImageBus::Events::GetColor)
                ->Event("SetColor", &UiCustomImageBus::Events::SetColor)
                ->Event("GetSpritePathname", &UiCustomImageBus::Events::GetSpritePathname)
                ->Event("SetSpritePathname", &UiCustomImageBus::Events::SetSpritePathname)
                ->Event("GetUVs", &UiCustomImageBus::Events::GetUVs)
                ->Event("SetUVs", &UiCustomImageBus::Events::SetUVs)
                ->Event("GetClamp", &UiCustomImageBus::Events::GetClamp)
                ->Event("SetClamp", &UiCustomImageBus::Events::SetClamp);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PROTECTED MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Init()
    {
        // If this is called from RC.exe for example these pointers will not be set. In that case
        // we only need to be able to load, init and save the component. It will never be
        // activated.
        if (!AZ::Interface<ILyShine>::Get())
        {
            return;
        }

        // Load our sprite from the path at the beginning of the game
        if (!m_sprite)
        {
            if (!m_spritePathname.GetAssetPath().empty())
            {
                m_sprite = AZ::Interface<ILyShine>::Get()->LoadSprite(m_spritePathname.GetAssetPath().c_str());
            }
        }

        m_overrideColor = m_color;
        m_overrideAlpha = m_alpha;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Activate()
    {
        UiVisualBus::Handler::BusConnect(m_entity->GetId());
        UiRenderBus::Handler::BusConnect(m_entity->GetId());
        UiCustomImageBus::Handler::BusConnect(m_entity->GetId());
        UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::Deactivate()
    {
        UiVisualBus::Handler::BusDisconnect();
        UiRenderBus::Handler::BusDisconnect();
        UiCustomImageBus::Handler::BusDisconnect();
        UiTransformChangeNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // PRIVATE MEMBER FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::RenderToCache(LyShine::IRenderGraph* renderGraph)
    {
        UiTransformInterface::RectPoints points;
        UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetViewportSpacePoints, points);

        // points are a clockwise quad
        const AZ::Vector2 uvs[4] = {
            AZ::Vector2(m_uvs.m_left, m_uvs.m_top), AZ::Vector2(m_uvs.m_right, m_uvs.m_top)
            , AZ::Vector2(m_uvs.m_right, m_uvs.m_bottom), AZ::Vector2(m_uvs.m_left, m_uvs.m_bottom)
        };
        RenderSingleQuad(renderGraph, points.pt, uvs);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::RenderSingleQuad(LyShine::IRenderGraph* renderGraph, const AZ::Vector2* positions, const AZ::Vector2* uvs)
    {
        float fade = renderGraph->GetAlphaFade();
        float desiredAlpha = m_overrideAlpha * fade;

        AZ::Color color = AZ::Color::CreateFromVector3AndFloat(m_overrideColor.GetAsVector3(), desiredAlpha);
        color = color.GammaToLinear();   // the colors are specified in sRGB but we want linear colors in the shader

        uint32 packedColor = (color.GetA8() << 24) | (color.GetR8() << 16) | (color.GetG8() << 8) | color.GetB8();
        IDraw2d::Rounding pixelRounding = IsPixelAligned() ? IDraw2d::Rounding::Nearest : IDraw2d::Rounding::None;

        const int numVertices = 4;
        if (numVertices != m_cachedPrimitive.m_numVertices)
        {
            if (m_cachedPrimitive.m_vertices)
            {
                delete [] m_cachedPrimitive.m_vertices;
            }
    
            m_cachedPrimitive.m_vertices = new LyShine::UiPrimitiveVertex[numVertices];
            m_cachedPrimitive.m_numVertices = numVertices;
        }

        // points are a clockwise quad
        for (int i = 0; i < numVertices; ++i)
        {
            AZ::Vector2 pos = Draw2dHelper::RoundXY(positions[i], pixelRounding);
            m_cachedPrimitive.m_vertices[i].xy = Vec2(pos.GetX(), pos.GetY());
            m_cachedPrimitive.m_vertices[i].color.dcolor = packedColor;
            m_cachedPrimitive.m_vertices[i].st = Vec2(uvs[i].GetX(), uvs[i].GetY());
            m_cachedPrimitive.m_vertices[i].texIndex = 0;
            m_cachedPrimitive.m_vertices[i].texHasColorChannel = 1;
            m_cachedPrimitive.m_vertices[i].texIndex2 = 0;
            m_cachedPrimitive.m_vertices[i].pad = 0;
        }

        static uint16 indices[6] = { 0, 1, 2, 2, 3, 0 };
        m_cachedPrimitive.m_numIndices = 6;
        m_cachedPrimitive.m_indices = indices;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool UiCustomImageComponent::IsPixelAligned()
    {
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        bool isPixelAligned = true;
        UiCanvasBus::EventResult(isPixelAligned, canvasEntityId, &UiCanvasBus::Events::GetIsPixelAligned);
        return isPixelAligned;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::OnSpritePathnameChange()
    {
        ISprite* newSprite = nullptr;

        if (!m_spritePathname.GetAssetPath().empty())
        {
            // Load the new texture.
            newSprite = AZ::Interface<ILyShine>::Get()->LoadSprite(m_spritePathname.GetAssetPath().c_str());
        }

        SAFE_RELEASE(m_sprite);
        m_sprite = newSprite;
        MarkRenderGraphDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::OnColorChange()
    {
        m_overrideColor = m_color;
        m_overrideAlpha = m_alpha;
        MarkRenderCacheDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::OnRenderSettingChange()
    {
        MarkRenderCacheDirty();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::MarkRenderCacheDirty()
    {
        if (!m_isRenderCacheDirty)
        {
            m_isRenderCacheDirty = true;
            MarkRenderGraphDirty();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void UiCustomImageComponent::MarkRenderGraphDirty()
    {
        // tell the canvas to invalidate the render graph (never want to do this while rendering)
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasComponentImplementationBus::Event(canvasEntityId, &UiCanvasComponentImplementationBus::Events::MarkRenderGraphDirty);
    }

}
