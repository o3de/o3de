/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiMaskComponent.h"
#include <LyShine/Draw2d.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "RenderToTextureBus.h"
#include "RenderGraph.h"
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiCanvasBus.h>

#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <AtomCore/Instance/Instance.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiMaskComponent::UiMaskComponent()
    : m_enableMasking(true)
    , m_drawMaskVisualBehindChildren(false)
    , m_drawMaskVisualInFrontOfChildren(false)
    , m_useAlphaTest(false)
    , m_maskInteraction(true)
{
    m_cachedPrimitive.m_vertices = nullptr;
    m_cachedPrimitive.m_numVertices = 0;
    m_cachedPrimitive.m_indices = nullptr;
    m_cachedPrimitive.m_numIndices = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiMaskComponent::~UiMaskComponent()
{
    // destroy render targets if we are using them
    DestroyRenderTarget();

    // We only deallocate the vertices on destruction rather than every time we recreate the render
    // target. Changing the size of the element requires recreating render target but doesn't change
    // the number of vertices. Note this may be nullptr which is fine for delete.
    delete [] m_cachedPrimitive.m_vertices;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::Render(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
    UiRenderInterface* renderInterface, int numChildren, bool isInGame)
{
    // Check if this mask is a stencil mask being used while rendering to stencil for a parent mask, if so ignore it
    if (!ValidateMaskConfiguration(renderGraph))
    {
        return;
    }

    // Get the element interface of the child mask element (if any is setup and it is render enabled)
    // If the child mask element can't be used because it is not a descendant or is used by multiple masks
    // then this sets it to null (and reports warnings)
    UiElementInterface* childMaskElementInterface = GetValidatedChildMaskElement();

    if (m_enableMasking)
    {
        if (GetUseRenderToTexture())
        {
            // First create the render targets if they are not already created or resize them if needed.
            // We delay first creation of the render targets until render time since size is not known in Activate.
            {
                // render targets are always snapped to pixels in viewport space, compute the snapped element bounds
                // and, from that, the size of the render target
                AZ::Vector2 pixelAlignedTopLeft, pixelAlignedBottomRight;
                ComputePixelAlignedBounds(pixelAlignedTopLeft, pixelAlignedBottomRight);
                AZ::Vector2 renderTargetSize = pixelAlignedBottomRight - pixelAlignedTopLeft;

                bool needsResize = static_cast<int>(renderTargetSize.GetX()) != m_renderTargetWidth || static_cast<int>(renderTargetSize.GetY()) != m_renderTargetHeight;
                if (m_contentAttachmentImageId.IsEmpty() || needsResize)
                {
                    // Need to create or resize the render target
                    CreateOrResizeRenderTarget(pixelAlignedTopLeft, pixelAlignedBottomRight);
                }

                // if the render targets failed to be created (zero size for example) we don't render anything
                // in theory the child mask element could still be non-zero size and could reveal things. But the way gradient masks
                // currently work is that the size of the render target is defined by the size of this element, therefore nothing would
                // be revealed by the mask if it is zero sized.
                if (m_contentAttachmentImageId.IsEmpty())
                {
                    return;
                }
            }

            // Do gradient mask render
            RenderUsingGradientMask(renderGraph, elementInterface, renderInterface, childMaskElementInterface, numChildren, isInGame);
        }
        else
        {
            // using stencil mask, not going to use render targets, destroy previous render target, if exists
            if (!m_contentAttachmentImageId.IsEmpty())
            {
                DestroyRenderTarget();
            }

            // Do stencil mask render
            RenderUsingStencilMask(renderGraph, elementInterface, renderInterface, childMaskElementInterface, numChildren, isInGame);
        }
    }
    else
    {
        // masking disabled, not going to use render targets, destroy previous render target, if exists
        if (!m_contentAttachmentImageId.IsEmpty())
        {
            DestroyRenderTarget();
        }

        // draw behind and draw in front are only options when not using gradient masks. If they are both set then we need
        // to use a mask render node in the render graph, so handle that specially
        if (!GetUseRenderToTexture() && m_drawMaskVisualBehindChildren && m_drawMaskVisualInFrontOfChildren)
        {
            RenderDisabledMaskWithDoubleRender(renderGraph, elementInterface, renderInterface, childMaskElementInterface, numChildren, isInGame);
        }
        else
        {
            RenderDisabledMask(renderGraph, elementInterface, renderInterface, childMaskElementInterface, numChildren, isInGame);
        }
    }

    // re-enable the rendering of the child mask (if it was enabled before we changed it)
    // This allows the game code to turn the child mask element on and off if so desired.
    if (childMaskElementInterface)
    {
        childMaskElementInterface->SetIsRenderEnabled(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetIsMaskingEnabled()
{
    return m_enableMasking;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetIsMaskingEnabled(bool enableMasking)
{
    if (m_enableMasking != enableMasking)
    {
        // tell the canvas to invalidate the render graph
        MarkRenderGraphDirty();

        m_enableMasking = enableMasking;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetIsInteractionMaskingEnabled()
{
    return m_maskInteraction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetIsInteractionMaskingEnabled(bool enableInteractionMasking)
{
    m_maskInteraction = enableInteractionMasking;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetDrawBehind()
{
    return m_drawMaskVisualBehindChildren;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetDrawBehind(bool drawMaskVisualBehindChildren)
{
    if (m_drawMaskVisualBehindChildren != drawMaskVisualBehindChildren)
    {
        // tell the canvas to invalidate the render graph
        MarkRenderGraphDirty();

        m_drawMaskVisualBehindChildren = drawMaskVisualBehindChildren;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetDrawInFront()
{
    return m_drawMaskVisualInFrontOfChildren;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetDrawInFront(bool drawMaskVisualInFrontOfChildren)
{
    if (m_drawMaskVisualInFrontOfChildren != drawMaskVisualInFrontOfChildren)
    {
        // tell the canvas to invalidate the render graph
        MarkRenderGraphDirty();

        m_drawMaskVisualInFrontOfChildren = drawMaskVisualInFrontOfChildren;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetUseAlphaTest()
{
    return m_useAlphaTest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetUseAlphaTest(bool useAlphaTest)
{
    if (m_useAlphaTest != useAlphaTest)
    {
        // tell the canvas to invalidate the render graph
        MarkRenderGraphDirty();

        m_useAlphaTest = useAlphaTest;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::GetUseRenderToTexture()
{
    return m_useRenderToTexture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::SetUseRenderToTexture(bool useRenderToTexture)
{
    if (GetUseRenderToTexture() != useRenderToTexture)
    {
        m_useRenderToTexture = useRenderToTexture;
        OnRenderTargetChange();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::IsPointMasked(AZ::Vector2 point)
{
    bool isMasked = false;

    // it is never masked if the flag to mask interactions is not checked
    if (m_maskInteraction)
    {
        // Initially consider it outside all of the mask visuals. If it is inside any we return false.
        isMasked = true;

        // Right now we only do a check for the rectangle. If the point is outside of the rectangle
        // then it is masked.
        // In the future we will add the option to check the alpha of the mask texture for interaction masking

        // first check this element if there is a visual component
        if (UiVisualBus::FindFirstHandler(GetEntityId()))
        {
            bool isInRect = false;
            EBUS_EVENT_ID_RESULT(isInRect, GetEntityId(), UiTransformBus, IsPointInRect, point);
            if (isInRect)
            {
                return false;
            }
        }

        // If there is a child mask element
        if (m_childMaskElement.IsValid())
        {
            // if it has a Visual component check if the point is in its rect
            if (UiVisualBus::FindFirstHandler(m_childMaskElement))
            {
                bool isInRect = false;
                EBUS_EVENT_ID_RESULT(isInRect, m_childMaskElement, UiTransformBus, IsPointInRect, point);
                if (isInRect)
                {
                    return false;
                }
            }

            // get any descendants of the child mask element that have visual components
            LyShine::EntityArray childMaskElements;
            EBUS_EVENT_ID(m_childMaskElement, UiElementBus, FindDescendantElements, 
                [](const AZ::Entity* descendant) { return UiVisualBus::FindFirstHandler(descendant->GetId()) != nullptr; },
                childMaskElements);

            // if the point is in any of their rects then it is not masked out
            for (auto child : childMaskElements)
            {
                bool isInRect = false;
                EBUS_EVENT_ID_RESULT(isInRect, child->GetId(), UiTransformBus, IsPointInRect, point);
                if (isInRect)
                {
                    return false;
                }
            }
        }
    }

    return isMasked;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::OnCanvasSpaceRectChanged(AZ::EntityId /*entityId*/, const UiTransformInterface::Rect& /*oldRect*/, const UiTransformInterface::Rect& /*newRect*/)
{
    // we only listen for this if using render target, if rect changed potentially recreate render target
    OnRenderTargetChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::OnTransformToViewportChanged()
{
    // we only listen for this if using render target, if transform changed potentially recreate render target
    OnRenderTargetChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiMaskComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiMaskComponent, AZ::Component>()
            ->Version(1)
            ->Field("EnableMasking", &UiMaskComponent::m_enableMasking)
            ->Field("MaskInteraction", &UiMaskComponent::m_maskInteraction)
            ->Field("ChildMaskElement", &UiMaskComponent::m_childMaskElement)
            ->Field("UseRenderToTexture", &UiMaskComponent::m_useRenderToTexture)
            ->Field("DrawBehind", &UiMaskComponent::m_drawMaskVisualBehindChildren)
            ->Field("DrawInFront", &UiMaskComponent::m_drawMaskVisualInFrontOfChildren)
            ->Field("UseAlphaTest", &UiMaskComponent::m_useAlphaTest)
            ;

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiMaskComponent>("Mask", "A component that masks child elements using its visual component");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiMask.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiMask.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_enableMasking, "Enable masking",
                "When checked, only the parts of child elements that are revealed by the mask will be seen.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiMaskComponent::OnEditorRenderSettingChange);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_maskInteraction, "Mask interaction",
                "Check this box to prevent children hidden by the mask from getting input events.");

            editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiMaskComponent::m_childMaskElement, "Child mask element",
                "A child element that is rendered as part of the mask.")
                ->Attribute(AZ::Edit::Attributes::EnumValues, &UiMaskComponent::PopulateChildEntityList)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiMaskComponent::OnEditorRenderSettingChange);

             editInfo->DataElement(0, &UiMaskComponent::m_useRenderToTexture, "Use alpha gradient",
                "If true, this element's content and the mask are rendered to separate render targets\n"
                "and then rendered to the screen using the mask render target as an alpha gradient mask.\n"
                "This allows soft-edged masking. The effect is limited to the rect of this element.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshEntireTree", 0xefbc823c))
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiMaskComponent::OnRenderTargetChange);

             editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_drawMaskVisualBehindChildren, "Draw behind",
                "Check this box to draw the mask visual behind the child elements.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiMaskComponent::IsStencilMask)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiMaskComponent::OnEditorRenderSettingChange);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_drawMaskVisualInFrontOfChildren, "Draw in front",
                "Check this box to draw the mask in front of the child elements.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiMaskComponent::IsStencilMask)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiMaskComponent::OnEditorRenderSettingChange);

            editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiMaskComponent::m_useAlphaTest, "Use alpha test",
                "Check this box to use the alpha channel in the mask visual's texture to define the mask.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &UiMaskComponent::IsStencilMask)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiMaskComponent::OnEditorRenderSettingChange);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiMaskBus>("UiMaskBus")
            ->Event("GetIsMaskingEnabled", &UiMaskBus::Events::GetIsMaskingEnabled)
            ->Event("SetIsMaskingEnabled", &UiMaskBus::Events::SetIsMaskingEnabled)
            ->Event("GetIsInteractionMaskingEnabled", &UiMaskBus::Events::GetIsInteractionMaskingEnabled)
            ->Event("SetIsInteractionMaskingEnabled", &UiMaskBus::Events::SetIsInteractionMaskingEnabled)
            ->Event("GetDrawBehind", &UiMaskBus::Events::GetDrawBehind)
            ->Event("SetDrawBehind", &UiMaskBus::Events::SetDrawBehind)
            ->Event("GetDrawInFront", &UiMaskBus::Events::GetDrawInFront)
            ->Event("SetDrawInFront", &UiMaskBus::Events::SetDrawInFront)
            ->Event("GetUseAlphaTest", &UiMaskBus::Events::GetUseAlphaTest)
            ->Event("SetUseAlphaTest", &UiMaskBus::Events::SetUseAlphaTest)
            ->Event("GetUseRenderToTexture", &UiMaskBus::Events::GetUseRenderToTexture)
            ->Event("SetUseRenderToTexture", &UiMaskBus::Events::SetUseRenderToTexture);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::Activate()
{
    UiRenderControlBus::Handler::BusConnect(m_entity->GetId());
    UiMaskBus::Handler::BusConnect(m_entity->GetId());
    UiInteractionMaskBus::Handler::BusConnect(m_entity->GetId());

    if (GetUseRenderToTexture())
    {
        UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());
    }

    MarkRenderGraphDirty();

    // set the render target name to an automatically generated name based on entity Id
    AZStd::string idString = GetEntityId().ToString();
    m_renderTargetName = "ContentTarget_";
    m_renderTargetName += idString;
    m_maskRenderTargetName = "MaskTarget_";
    m_maskRenderTargetName += idString;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::Deactivate()
{
    UiRenderControlBus::Handler::BusDisconnect();
    UiMaskBus::Handler::BusDisconnect();
    UiInteractionMaskBus::Handler::BusDisconnect();

    if (UiTransformChangeNotificationBus::Handler::BusIsConnected())
    {
        UiTransformChangeNotificationBus::Handler::BusDisconnect();
    }

    MarkRenderGraphDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiMaskComponent::EntityComboBoxVec UiMaskComponent::PopulateChildEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // Get a list of all child elements
    LyShine::EntityArray matchingElements;
    EBUS_EVENT_ID(GetEntityId(), UiElementBus, FindDescendantElements,
        []([[maybe_unused]] const AZ::Entity* entity) { return true; },
        matchingElements);

    // add their names to the StringList and their IDs to the id list
    // also note whether the current value of m_childMaskElement is in the list
    bool isCurrentValueInList = (m_childMaskElement == AZ::EntityId());
    for (auto childEntity : matchingElements)
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(childEntity->GetId()), childEntity->GetName()));

        if (!isCurrentValueInList && m_childMaskElement == childEntity->GetId())
        {
            isCurrentValueInList = true;
        }
    }

    if (!isCurrentValueInList)
    {
        // The current value is not in the list. It is invalid for the child mask element to not be a decscendant element
        // But that can be the case if the child is reparented. In this case a warning will be output during render.
        // However, if we don't add the current value into the list then the collapsed combo box will say <None> even though
        // it is set - making it confusing (and hard to change if there are no children).
        // So we add the current value to the list even though it is not a descendant.
        AZ::Entity* childMaskEntity = nullptr;
        EBUS_EVENT_RESULT(childMaskEntity, AZ::ComponentApplicationBus, FindEntity, m_childMaskElement);
        if (childMaskEntity)
        {
            result.push_back(AZStd::make_pair(AZ::EntityId(m_childMaskElement), childMaskEntity->GetName()));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::MarkRenderGraphDirty()
{
    // tell the canvas to invalidate the render graph
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID(canvasEntityId, UiCanvasComponentImplementationBus, MarkRenderGraphDirty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::OnEditorRenderSettingChange()
{
    // something changed in the properties that requires re-rendering
    MarkRenderGraphDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::OnRenderTargetChange()
{
    // mark render graph dirty so next render will recreate render targets if necessary
    MarkRenderGraphDirty();

    // update cached primitive to reflect new transforms
    AZ::Vector2 pixelAlignedTopLeft, pixelAlignedBottomRight;
    ComputePixelAlignedBounds(pixelAlignedTopLeft, pixelAlignedBottomRight);
    UpdateCachedPrimitive(pixelAlignedTopLeft, pixelAlignedBottomRight);

    // if using a render target we need to know if the element size or position changes since that
    // affects the render target and the viewport
    if (GetUseRenderToTexture())
    {
        if (!UiTransformChangeNotificationBus::Handler::BusIsConnected())
        {
            UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());
        }
    }
    else
    {
        if (UiTransformChangeNotificationBus::Handler::BusIsConnected())
        {
            UiTransformChangeNotificationBus::Handler::BusDisconnect();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::CreateOrResizeRenderTarget(const AZ::Vector2& pixelAlignedTopLeft, const AZ::Vector2& pixelAlignedBottomRight)
{
    // The render target size is the pixel aligned element size.
    AZ::Vector2 renderTargetSize = pixelAlignedBottomRight - pixelAlignedTopLeft;

    if (renderTargetSize.GetX() <= 0 || renderTargetSize.GetY() <= 0)
    {
        // if render targets exist then destroy them (just to be in a consistent state)
        DestroyRenderTarget();
        return;
    }

    m_viewportTopLeft = pixelAlignedTopLeft;
    m_viewportSize = renderTargetSize;

    // LYSHINE_ATOM_TODO: optimize by reusing/resizing targets
    DestroyRenderTarget();

    // Create a render target that this element and its children will be rendered to
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    AZ::RHI::Size imageSize(static_cast<uint32_t>(renderTargetSize.GetX()), static_cast<uint32_t>(renderTargetSize.GetY()), 1);
    EBUS_EVENT_ID_RESULT(m_contentAttachmentImageId, canvasEntityId, LyShine::RenderToTextureRequestBus, UseRenderTarget, AZ::Name(m_renderTargetName.c_str()), imageSize);
    if (m_contentAttachmentImageId.IsEmpty())
    {
        AZ_Warning("UI", false, "Failed to create content render target for UiMaskComponent");
    }

    // Create separate render target for the mask texture
    EBUS_EVENT_ID_RESULT(m_maskAttachmentImageId, canvasEntityId, LyShine::RenderToTextureRequestBus, UseRenderTarget, AZ::Name(m_maskRenderTargetName.c_str()), imageSize);
    if (m_maskAttachmentImageId.IsEmpty())
    {
        AZ_Warning("UI", false, "Failed to create mask render target for UiMaskComponent");
        DestroyRenderTarget();
    }

    // at this point either all render targets and depth surfaces are created or none are.
    // If all succeeded then update the render target size
    if (!m_contentAttachmentImageId.IsEmpty())
    {
        m_renderTargetWidth = static_cast<int>(renderTargetSize.GetX());
        m_renderTargetHeight = static_cast<int>(renderTargetSize.GetY());
    }

    UpdateCachedPrimitive(pixelAlignedTopLeft, pixelAlignedBottomRight);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::DestroyRenderTarget()
{
    if (!m_contentAttachmentImageId.IsEmpty())
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        EBUS_EVENT_ID(canvasEntityId, LyShine::RenderToTextureRequestBus, ReleaseRenderTarget, m_contentAttachmentImageId);

        m_contentAttachmentImageId = AZ::RHI::AttachmentId{};
    }

    if (!m_maskAttachmentImageId.IsEmpty())
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        EBUS_EVENT_ID(canvasEntityId, LyShine::RenderToTextureRequestBus, ReleaseRenderTarget, m_maskAttachmentImageId);

        m_maskAttachmentImageId = AZ::RHI::AttachmentId{};
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::UpdateCachedPrimitive(const AZ::Vector2& pixelAlignedTopLeft, const AZ::Vector2& pixelAlignedBottomRight)
{
    // update viewport position
    m_viewportTopLeft = pixelAlignedTopLeft;

    // now create the verts to render the texture to the screen, we cache these in m_cachedPrimitive
    const int numVertices = 4;
    if (!m_cachedPrimitive.m_vertices)
    {
        // verts not yet allocated, allocate them now
        const int numIndices = 6;
        m_cachedPrimitive.m_vertices = new LyShine::UiPrimitiveVertex[numVertices];
        m_cachedPrimitive.m_numVertices = numVertices;

        static uint16 indices[numIndices] = { 0, 1, 2, 2, 3, 0 };
        m_cachedPrimitive.m_indices = indices;
        m_cachedPrimitive.m_numIndices = numIndices;
    }

    float left = pixelAlignedTopLeft.GetX();
    float right = pixelAlignedBottomRight.GetX();
    float top = pixelAlignedTopLeft.GetY();
    float bottom = pixelAlignedBottomRight.GetY();
    Vec2 positions[numVertices] = { Vec2(left, top), Vec2(right, top), Vec2(right, bottom), Vec2(left, bottom) };

    static const Vec2 uvs[numVertices] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };

    for (int i = 0; i < numVertices; ++i)
    {
        m_cachedPrimitive.m_vertices[i].xy = positions[i];
        m_cachedPrimitive.m_vertices[i].color.dcolor = 0xFFFFFFFF;
        m_cachedPrimitive.m_vertices[i].st = uvs[i];
        m_cachedPrimitive.m_vertices[i].texIndex = 0;   // this will be set later by render graph
        m_cachedPrimitive.m_vertices[i].texHasColorChannel = 1;
        m_cachedPrimitive.m_vertices[i].texIndex2 = 0;
        m_cachedPrimitive.m_vertices[i].pad = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::ComputePixelAlignedBounds(AZ::Vector2& pixelAlignedTopLeft, AZ::Vector2& pixelAlignedBottomRight)
{
    // The viewport has to be axis aligned so we get the axis-aligned top-left and bottom-right of the element
    // in main viewport space. We then snap them to the nearest pixel since the render target has to be an exact number
    // of pixels.
    UiTransformInterface::RectPoints points;
    EBUS_EVENT_ID(GetEntityId(), UiTransformBus, GetViewportSpacePoints, points);
    pixelAlignedTopLeft = Draw2dHelper::RoundXY(points.GetAxisAlignedTopLeft(), IDraw2d::Rounding::Nearest);
    pixelAlignedBottomRight = Draw2dHelper::RoundXY(points.GetAxisAlignedBottomRight(), IDraw2d::Rounding::Nearest);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::IsStencilMask()
{
    return !GetUseRenderToTexture();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::RenderUsingStencilMask(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
    UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, int numChildren, bool isInGame)
{
    // begin the mask render node
    renderGraph->BeginMask(m_enableMasking, m_useAlphaTest, m_drawMaskVisualBehindChildren, m_drawMaskVisualInFrontOfChildren);
            
    // We never want to apply the fade value when rendering the mask visual to stencil buffer
    renderGraph->PushOverrideAlphaFade(1.0f);

    // Render the visual component for this element (if there is one) plus the child mask element (if there is one)
    renderGraph->SetIsRenderingToMask(true);
    RenderMaskPrimitives(renderGraph, renderInterface, childMaskElementInterface, isInGame);
    renderGraph->SetIsRenderingToMask(false);

    // Pop off the temporary fade value we pushed while rendering the mask to the stencil buffer
    renderGraph->PopAlphaFade();

    // tell render graph we have finished rendering the mask primitives and are starting the content primitives
    renderGraph->StartChildrenForMask();

    // Render the "content" - the child elements excluding the child mask element (if any)
    RenderContentPrimitives(renderGraph, elementInterface, childMaskElementInterface, numChildren, isInGame);

    // end the mask render node
    renderGraph->EndMask();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::RenderUsingGradientMask(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
    UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, int numChildren, bool isInGame)
{
    // we always clear to transparent black - the accumulation of alpha in the render target requires it
    AZ::Color clearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Get the render targets
    AZ::Data::Instance<AZ::RPI::AttachmentImage> contentAttachmentImage;
    AZ::Data::Instance<AZ::RPI::AttachmentImage> maskAttachmentImage;
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID_RESULT(contentAttachmentImage, canvasEntityId, LyShine::RenderToTextureRequestBus, GetRenderTarget, m_contentAttachmentImageId);
    EBUS_EVENT_ID_RESULT(maskAttachmentImage, canvasEntityId, LyShine::RenderToTextureRequestBus, GetRenderTarget, m_maskAttachmentImageId);

    // We don't want parent faders to affect what is rendered to the render target since we will
    // apply those fades when we render from the render target.
    // Note that this means that, if there are parent (no render to texture) faders, we get a "free"
    // improved fade for the children of the mask. We could avoid this but it seems desirable.
    renderGraph->PushOverrideAlphaFade(1.0f);

    // mask render target
    {
        // Start building the render to texture node in the render graph
        LyShine::RenderGraph* lyRenderGraph = static_cast<LyShine::RenderGraph*>(renderGraph); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
        lyRenderGraph->BeginRenderToTexture(maskAttachmentImage, m_viewportTopLeft, m_viewportSize, clearColor);

        // Render the visual component for this element (if there is one) plus the child mask element (if there is one)
        RenderMaskPrimitives(renderGraph, renderInterface, childMaskElementInterface, isInGame);

        // finish building the render to texture node for the mask render target in the render graph
        renderGraph->EndRenderToTexture();
    }

    // content render target
    {
        // Start building the render to texture node for the content render target in the render graph
        LyShine::RenderGraph* lyRenderGraph = static_cast<LyShine::RenderGraph*>(renderGraph); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
        lyRenderGraph->BeginRenderToTexture(contentAttachmentImage, m_viewportTopLeft, m_viewportSize, clearColor);

        // Render the "content" - the child elements excluding the child mask element (if any)
        RenderContentPrimitives(renderGraph, elementInterface, childMaskElementInterface, numChildren, isInGame);

        // finish building the render to texture node in the render graph
        renderGraph->EndRenderToTexture();
    }

    // pop off the override alpha fade
    renderGraph->PopAlphaFade();

    // render from the render target to the screen (or a parent render target) with the any fade value
    // from ancestor faders
    {
        // set the alpha in the verts
        {
            float desiredAlpha = renderGraph->GetAlphaFade();
            uint32 desiredPackedAlpha = static_cast<uint8>(desiredAlpha * 255.0f);

            // If the fade value has changed we need to update the alpha values in the vertex colors but we do
            // not want to touch or recompute the RGB values
            if (m_cachedPrimitive.m_vertices[0].color.a != desiredPackedAlpha)
            {
                // go through all the cached vertices and update the alpha values
                LyShine::UCol desiredPackedColor = m_cachedPrimitive.m_vertices[0].color;
                desiredPackedColor.a = static_cast<uint8>(desiredPackedAlpha);
                for (int i = 0; i < m_cachedPrimitive.m_numVertices; ++i)
                {
                    m_cachedPrimitive.m_vertices[i].color = desiredPackedColor;
                }
            }
        }

        // Add a primitive to do the alpha mask
        {
            LyShine::RenderGraph* lyRenderGraph = static_cast<LyShine::RenderGraph*>(renderGraph); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
            if (lyRenderGraph)
            {
                // Set the texture and other render state required
                AZ::Data::Instance<AZ::RPI::Image> contentImage = contentAttachmentImage;
                AZ::Data::Instance<AZ::RPI::Image> maskImage = maskAttachmentImage;
                bool isClampTextureMode = true;
                bool isTextureSRGB = true;
                bool isTexturePremultipliedAlpha = false;
                LyShine::BlendMode blendMode = LyShine::BlendMode::Normal;

                // add a render node to render using the two render targets, one as an alpha mask of the other
                lyRenderGraph->AddAlphaMaskPrimitiveAtom(&m_cachedPrimitive,
                    contentAttachmentImage,
                    maskAttachmentImage,
                    isClampTextureMode,
                    isTextureSRGB,
                    isTexturePremultipliedAlpha,
                    blendMode);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::RenderDisabledMask(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
    UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, int numChildren, bool isInGame)
{
    // The calling function ensures this method is never called when we want to render both behind and in front.
    // That would not work because the same primitive cannot be added to the render graph twice due to the use of
    // intrusive lists.
    // Note that (currently at least) we never draw behind or in front when render to texture is enabled, so the value of 
    // m_drawMaskVisualBehindChildren and m_drawMaskVisualInFrontOfChildren is irrelevant in that case.
    AZ_Assert(!(!GetUseRenderToTexture() && m_drawMaskVisualBehindChildren && m_drawMaskVisualInFrontOfChildren),
        "Cannot use RenderDisabledMask when we are drawing behind and in front");

    if (!GetUseRenderToTexture() && m_drawMaskVisualBehindChildren)
    {
        // Render the visual component for this element (if there is one) plus the child mask element (if there is one)
        RenderMaskPrimitives(renderGraph, renderInterface, childMaskElementInterface, isInGame);
    }

    // Render the "content" - the child elements excluding the child mask element (if any)
    RenderContentPrimitives(renderGraph, elementInterface, childMaskElementInterface, numChildren, isInGame);

    if (!GetUseRenderToTexture() && m_drawMaskVisualInFrontOfChildren)
    {
        // Render the visual component for this element (if there is one) plus the child mask element (if there is one)
        RenderMaskPrimitives(renderGraph, renderInterface, childMaskElementInterface, isInGame);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::RenderDisabledMaskWithDoubleRender(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
    [[maybe_unused]] UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, int numChildren, bool isInGame)
{
    // we still need to use a mask render node in order to render the behind and in-front mask visual twice
    renderGraph->BeginMask(m_enableMasking, m_useAlphaTest, m_drawMaskVisualBehindChildren, m_drawMaskVisualInFrontOfChildren);
            
    // No need to render the mask primitives since masking is disabled

    // tell render graph we have finished rendering the mask primitives and are starting the content primitives
    renderGraph->StartChildrenForMask();

    // Render the "content" - the child elements excluding the child mask element (if any)
    RenderContentPrimitives(renderGraph, elementInterface, childMaskElementInterface, numChildren, isInGame);

    // end the mask render node
    renderGraph->EndMask();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::RenderMaskPrimitives(LyShine::IRenderGraph* renderGraph,
    UiRenderInterface* renderInterface, UiElementInterface* childMaskElementInterface, bool isInGame)
{
    // Render the visual component for this element (if there is one)
    if (renderInterface)
    {
        renderInterface->Render(renderGraph);
    }

    // If there is a child mask element, that was render enabled at start of render,
    // then render that (and any children it has) also
    if (childMaskElementInterface)
    {
        // enable the rendering of the child mask element
        childMaskElementInterface->SetIsRenderEnabled(true);

        // Render the child mask element, this can render a whole hierarchy into the stencil buffer or mask render target
        // as part of the mask.
        childMaskElementInterface->RenderElement(renderGraph, isInGame);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMaskComponent::RenderContentPrimitives(LyShine::IRenderGraph* renderGraph,
    UiElementInterface* elementInterface,
    UiElementInterface* childMaskElementInterface,
    int numChildren, bool isInGame)
{
    if (childMaskElementInterface)
    {
        // disable the rendering of the child mask with the other children
        childMaskElementInterface->SetIsRenderEnabled(false);
    }

    // Render the child elements
    for (int childIndex = 0; childIndex < numChildren; ++childIndex)
    {
        UiElementInterface* childElementInterface = elementInterface->GetChildElementInterface(childIndex);
        // childElementInterface should never be nullptr but check just to be safe
        if (childElementInterface)
        {
            childElementInterface->RenderElement(renderGraph, isInGame);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMaskComponent::ValidateMaskConfiguration(const LyShine::IRenderGraph* renderGraph)
{
    bool isValidConfiguration = true;

    // Check if this mask is a stencil mask being used while rendering to stencil for a parent mask, if so ignore it
    if (renderGraph->IsRenderingToMask() && m_enableMasking && !GetUseRenderToTexture())
    {
        // We are in the process of rendering a child element hierarchy into the stencil buffer for another stencil mask component.
        // Additional stencil masking while rendering a stencil mask is not supported.
#ifndef _RELEASE
        // If this situation is new since last frame then output a warning message
        if (!m_reportedNestedStencilWarning)
        {
            const char* elementName = GetEntity()->GetName().c_str();
            AZ_Warning("UI", false,
                "Element \"%s\" with a stencil mask component is being used as a Child Mask Element for another stencil mask component, it will not be rendered.",
                elementName);
            m_reportedNestedStencilWarning = true;
        }
#endif

        // this means we render nothing for this element or its children
        isValidConfiguration = false;
    }
    else
    {
#ifndef _RELEASE
        // This allows us to report a warning if the situation is fixed but reintroduced
        m_reportedNestedStencilWarning = false;
#endif
    }

    return isValidConfiguration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiElementInterface* UiMaskComponent::GetValidatedChildMaskElement()
{
    // If there is a child mask element we need to know if it is render enabled. If not then we can ignore it.
    // If it is then we will enable it for the mask render and disable it for the content render.
    UiElementInterface* childMaskElementInterface = nullptr;
    if (m_childMaskElement.IsValid())
    {
        // There is a child mask element, get the UiElementInterface for the element
        childMaskElementInterface = UiElementBus::FindFirstHandler(m_childMaskElement);

        if (childMaskElementInterface)
        {
            if (!childMaskElementInterface->IsRenderEnabled())
            {
                // not render enabled, we can just ignore it
                childMaskElementInterface = nullptr;
            }
            else
            {
                // check if this is a valid descendant and not being used by a descendant mask
                bool isValidConfiguration = false;
                AZ::Entity* parent = childMaskElementInterface->GetParent();
                while (parent)
                {
                    if (parent == GetEntity())
                    {
                        // we found this element as a parent (ancestor) of the child mask element, without finding any
                        // other descendant mask that uses it so this is valid
                        isValidConfiguration = true;
                        break;
                    }

                    // Check if this parent (ancestor) of the child mask element has a Mask component that is using the
                    // same child mask element
                    UiMaskComponent* otherMask = parent->FindComponent<UiMaskComponent>();
                    if (otherMask && otherMask->m_childMaskElement == m_childMaskElement)
                    {
                        // this other mask is using the same child mask element
                        break;
                    }

                    // move up the parent chain
                    UiElementInterface* parentElementInterface = UiElementBus::FindFirstHandler(parent->GetId());
                    parent = parentElementInterface->GetParent();
                }

                if (!isValidConfiguration)
                {
                    // we will return a null ptr becuase we want to ignore the child mask element in the case of an invalid configuration
                    childMaskElementInterface = nullptr;

#ifndef _RELEASE
                    // If this situation is new since last frame then output a warning message
                    if (!m_reportedInvalidChildMaskElementWarning)
                    {
                        const char* elementName = GetEntity()->GetName().c_str();
                        const char* childMaskElementName = "";
                        AZ::Entity* childMaskEntity = nullptr;
                        EBUS_EVENT_RESULT(childMaskEntity, AZ::ComponentApplicationBus, FindEntity, m_childMaskElement);
                        if (childMaskEntity)
                        {
                            childMaskElementName = childMaskEntity->GetName().c_str();
                        }

                        if (!parent)
                        {
                            // we never found this mask component's entity as a parent of the child mask element
                            AZ_Warning("UI", false,
                                "Element \"%s\" with a mask component references a child mask element \"%s\" which is not a descendant, the child mask element will be ignored.",
                                elementName, childMaskElementName);
                        }
                        else
                        {
                            // only other error condition is that another mask was using the same child mask element
                            // parent will be pointing at the other mask element
                            const char* otherMaskElementName = parent->GetName().c_str();
                            AZ_Warning("UI", false,
                                "Element \"%s\" with a mask component references a child mask element \"%s\" which is also used as a child mask element by another mask \"%s\", the child mask element will be ignored.",
                                elementName, childMaskElementName, otherMaskElementName);
                        }

                        m_reportedInvalidChildMaskElementWarning = true;
                    }
#endif
                }
                else
                {
#ifndef _RELEASE
                    // This allows us to report a warning if the situation is fixed but reintroduced
                    m_reportedInvalidChildMaskElementWarning = false;
#endif
                }
            }
        }
    }

    return childMaskElementInterface;
}
