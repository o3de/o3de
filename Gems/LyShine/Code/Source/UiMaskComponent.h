/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/Bus/UiRenderControlBus.h>
#include <LyShine/Bus/UiMaskBus.h>
#include <LyShine/Bus/UiInteractionMaskBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/IRenderGraph.h>
#include <LyShine/UiRenderFormats.h>

#include <AzCore/Component/Component.h>
#include <Atom/RHI.Reflect/AttachmentId.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiMaskComponent
    : public AZ::Component
    , public UiRenderControlBus::Handler
    , public UiMaskBus::Handler
    , public UiInteractionMaskBus::Handler
    , public UiTransformChangeNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiMaskComponent, LyShine::UiMaskComponentUuid, AZ::Component);

    UiMaskComponent();
    ~UiMaskComponent() override;

    // UiRenderControlInterface
    void Render(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
        UiRenderInterface* renderInterface, int numChildren, bool isInGame) override;
    // ~UiRenderControlInterface

    // UiMaskInterface
    bool GetIsMaskingEnabled() override;
    void SetIsMaskingEnabled(bool enableMasking) override;
    bool GetIsInteractionMaskingEnabled() override;
    void SetIsInteractionMaskingEnabled(bool enableMasking) override;
    bool GetDrawBehind() override;
    void SetDrawBehind(bool drawMaskVisualBehindChildren) override;
    bool GetDrawInFront() override;
    void SetDrawInFront(bool drawMaskVisualInFrontOfChildren) override;
    bool GetUseAlphaTest() override;
    void SetUseAlphaTest(bool useAlphaTest) override;
    bool GetUseRenderToTexture() override;
    void SetUseRenderToTexture(bool useRenderToTexture) override;
    // ~UiMaskInterface

    // UiInteractionMaskInterface
    bool IsPointMasked(AZ::Vector2 point) override;
    // ~UiInteractionMaskInterface

    // UiTransformChangeNotification
    void OnCanvasSpaceRectChanged(AZ::EntityId entityId, const UiTransformInterface::Rect& oldRect, const UiTransformInterface::Rect& newRect) override;
    void OnTransformToViewportChanged() override;
    // ~UiTransformChangeNotification

public:  // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("UiMaskService"));
        provided.push_back(AZ_CRC_CE("UiRenderControlService"));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("UiRenderControlService"));
    }

    static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // Note that the UiVisualService is not required because a child mask element can be used instead.
        required.push_back(AZ_CRC_CE("UiElementService"));
        required.push_back(AZ_CRC_CE("UiTransformService"));
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiMaskComponent);

private: // member functions

    //! Method used to populate the drop down for the m_childMaskElement property field
    using EntityComboBoxVec = AZStd::vector< AZStd::pair< AZ::EntityId, AZStd::string > >;
    EntityComboBoxVec PopulateChildEntityList();

    //! Mark the render graph as dirty, this should be done when any change is made affects the structure of the graph
    void MarkRenderGraphDirty();

    //! Called when a property changed in property pane that invalidates render settings
    void OnEditorRenderSettingChange();

    //! Called when something changed that invalidates render target
    void OnRenderTargetChange();

    //! When m_useRenderToTexture is true this is used to create the render targets and depth surface or resize them if they exist
    void CreateOrResizeRenderTarget(const AZ::Vector2& pixelAlignedTopLeft, const AZ::Vector2& pixelAlignedBottomRight);

    //! Destroy the render targets and depth surface that are used when m_useRenderToTexture is true
    void DestroyRenderTarget();

    //! Update cached primitive vertices
    void UpdateCachedPrimitive(const AZ::Vector2& pixelAlignedTopLeft, const AZ::Vector2& pixelAlignedBottomRight);

    // compute pixel aligned bounds of element in viewport space
    void ComputePixelAlignedBounds(AZ::Vector2& pixelAlignedTopLeft, AZ::Vector2& pixelAlignedBottomRight);

    //! Some properties are only visible when this is a stencil mask as opposed to a gradient mask
    bool IsStencilMask();

    // render the element and its children using stencil mask
    void RenderUsingStencilMask(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
        UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, int numChildren, bool isInGame);

    // render the element and its children using render-to-texture and an alpha gradient mask
    void RenderUsingGradientMask(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
        UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, int numChildren, bool isInGame);

    // render a disabled mask (in case where we don't have both draw behind and draw in front enabled)
    void RenderDisabledMask(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
        UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, int numChildren, bool isInGame);

    // render a disabled mask (in case where we do have both draw behind and draw in front enabled
    void RenderDisabledMaskWithDoubleRender(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
        UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, int numChildren, bool isInGame);

    // render this element's component and the child mask element
    void RenderMaskPrimitives(LyShine::IRenderGraph* renderGraph,
        UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, bool isInGame);

    // render this element's child elements (excluding the child mask element)
    void RenderContentPrimitives(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
        UiElementInterface* childMaskElementInterface, int numChildren, bool isInGame);

    // validate that nested mask configurations are valid during rendering and output a warning if not
    bool ValidateMaskConfiguration(const LyShine::IRenderGraph* renderGraph);

    // Get the element interface child mask element after checking that the configuration of it is valid
    // output a warning if not (used during render).
    UiElementInterface* GetValidatedChildMaskElement();

private: // data

    // Serialized members

    //! flag allows for easy debugging, also can be used to turn mask on/off from C++
    //! or in an animation.
    bool m_enableMasking;

    //! flags to control whether the mask is drawn to color buffer as well as to the
    //! stencil in the first and second passes
    bool m_drawMaskVisualBehindChildren;
    bool m_drawMaskVisualInFrontOfChildren;

    //! Whether to enable alphatest when drawing mask visual.
    bool m_useAlphaTest;

    //! Whether to mask interaction
    bool m_maskInteraction;

    //! An optional child element that defines additional mask visuals
    AZ::EntityId m_childMaskElement;

    bool m_useRenderToTexture = false;  //!< If true, render this element and children to a separate render target and fade that

    // Non-serialized members

    //! This is generated from the entity ID and cached
    AZStd::string m_renderTargetName;

    //! This is generated from the entity ID and cached
    AZStd::string m_maskRenderTargetName;

    //! When rendering to a texture this is the attachment image for the render target
    AZ::RHI::AttachmentId m_contentAttachmentImageId;
    
    //! When rendering to a texture this is the texture ID of the render target
    //! When rendering to a texture this is the attachment image for the render target
    AZ::RHI::AttachmentId m_maskAttachmentImageId;

    //! The positions used for the render to texture viewport and to render the render target to the screen
    AZ::Vector2 m_viewportTopLeft = AZ::Vector2::CreateZero();
    AZ::Vector2 m_viewportSize = AZ::Vector2::CreateZero();

    // currently allocated size of render target
    int m_renderTargetWidth = 0;
    int m_renderTargetHeight = 0;

    //! cached rendering data for performance optimization of rendering the render target to screen
    LyShine::UiPrimitive m_cachedPrimitive;

#ifndef _RELEASE
    //! This variable is only used to prevent spamming a warning message each frame (for nested stencil masks)
    bool m_reportedNestedStencilWarning = false;

    //! This variable is only used to prevent spamming a warning message each frame (for invalid child mask elements)
    bool m_reportedInvalidChildMaskElementWarning = false;
#endif
};
