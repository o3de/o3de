/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiCanvasUpdateNotificationBus.h>
#include <LyShine/Bus/UiRenderControlBus.h>
#include <LyShine/Bus/UiFaderBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiAnimateEntityBus.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/IRenderGraph.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>
#include <Atom/RHI.Reflect/AttachmentId.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiFaderComponent
    : public AZ::Component
    , public UiCanvasUpdateNotificationBus::Handler
    , public UiRenderControlBus::Handler
    , public UiFaderBus::Handler
    , public UiAnimateEntityBus::Handler
    , public UiElementNotificationBus::Handler
    , public UiTransformChangeNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiFaderComponent, LyShine::UiFaderComponentUuid, AZ::Component);

    UiFaderComponent();
    ~UiFaderComponent() override;

    // UiCanvasUpdateNotification
    void Update(float deltaTime) override;
    // ~UiCanvasUpdateNotification

    // UiRenderControlInterface
    void Render(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
        UiRenderInterface* renderInterface, int numChildren, bool isInGame) override;
    // ~UiRenderControlInterface

    // UiFaderInterface
    float GetFadeValue() override;
    void SetFadeValue(float fade) override;
    void Fade(float targetValue, float speed) override;
    bool IsFading() override;

    bool GetUseRenderToTexture() override;
    void SetUseRenderToTexture(bool useRenderToTexture) override;
    // ~UiFaderInterface

    // UiAnimateEntityInterface
    void PropertyValuesChanged() override;
    // ~UiAnimateEntityInterface

    // UiElementNotifications
    void OnUiElementFixup(AZ::EntityId canvasEntityId, AZ::EntityId parentEntityId) override;
    // ~UiElementNotifications

    // UiTransformChangeNotification
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    void OnTransformToViewportChanged() override;
    // ~UiTransformChangeNotification

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiFaderService", 0x3c5847e9));
        provided.push_back(AZ_CRC("UiRenderControlService", 0x4e302454));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiFaderService", 0x3c5847e9));
        incompatible.push_back(AZ_CRC("UiRenderControlService", 0x4e302454));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UiElementService", 0x3dca7ad4));
        required.push_back(AZ_CRC("UiTransformService", 0x3a838e34));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    //! Called when the fade animation completes
    void CompleteFade();

    //! Common function for updating fade value
    void SetFadeValueInternal(float fade);

    //! Called when the fade property changed in property pane
    void OnFadeValueChanged();

    //! Called when something changed that invalidates render target
    void OnRenderTargetChange();

    //! Mark the render graph as dirty, this should be done when any change is made affects the structure of the graph
    void MarkRenderGraphDirty();

    //! When m_useRenderToTexture is true this is used to create the render target and depth surface or resize them if they exist
    void CreateOrResizeRenderTarget(const AZ::Vector2& pixelAlignedTopLeft, const AZ::Vector2& pixelAlignedBottomRight);

    //! Destroy the render target and depth surface that are used when m_useRenderToTexture is true
    void DestroyRenderTarget();

    //! Update cached primitive vertices
    void UpdateCachedPrimitive(const AZ::Vector2& pixelAlignedTopLeft, const AZ::Vector2& pixelAlignedBottomRight);

    // compute pixel aligned bounds of element in viewport space
    void ComputePixelAlignedBounds(AZ::Vector2& pixelAlignedTopLeft, AZ::Vector2& pixelAlignedBottomRight);

    // render the element and its children using standard fade (non-render-to-texture)
    void RenderStandardFader(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
        UiRenderInterface* renderInterface, int numChildren, bool isInGame);

    // render the element and its children using render-to-texture fade
    void RenderRttFader(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
        UiRenderInterface* renderInterface, int numChildren, bool isInGame);

    // render this element's visual component (if any) and child elements
    void RenderElementAndChildren(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
         UiRenderInterface* renderInterface, int numChildren, bool isInGame);

    AZ_DISABLE_COPY_MOVE(UiFaderComponent);

private: // data

    // Serialized members

    float m_fade;   //!< The initial/current fade value

    bool m_useRenderToTexture = false;  //!< If true, render this element and children to a separate render target and fade that

    // Non-serialized members

    // Used for fade animation
    bool m_isFading;
    float m_fadeTarget;
    float m_fadeSpeedInSeconds;

    //! This is generated from the entity ID and cached
    AZStd::string m_renderTargetName;

    //! When rendering to a texture this is the attachment image for the render target
    AZ::RHI::AttachmentId m_attachmentImageId;
    
    //! The positions used for the render to texture viewport and to render the render target to the screen
    AZ::Vector2 m_viewportTopLeft;
    AZ::Vector2 m_viewportSize;

    // currently allocated size of render target
    int m_renderTargetWidth = 0;
    int m_renderTargetHeight = 0;

    //! cached rendering data for performance optimization of rendering the render target to screen
    LyShine::UiPrimitive m_cachedPrimitive;
};
