/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiFaderComponent.h"
#include "RenderGraph.h"
#include <LyShine/Draw2d.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <AtomCore/Instance/Instance.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiRenderBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/IRenderGraph.h>

#include "UiSerialize.h"
#include "RenderToTextureBus.h"

// BehaviorContext UiFaderNotificationBus forwarder
class BehaviorUiFaderNotificationBusHandler
    : public UiFaderNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiFaderNotificationBusHandler, "{CAD44770-3D5E-4E67-8F05-D2A89E8C501A}", AZ::SystemAllocator,
        OnFadeComplete, OnFadeInterrupted, OnFaderDestroyed);

    void OnFadeComplete() override
    {
        Call(FN_OnFadeComplete);
    }

    void OnFadeInterrupted() override
    {
        Call(FN_OnFadeInterrupted);
    }

    void OnFaderDestroyed() override
    {
        Call(FN_OnFaderDestroyed);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiFaderComponent::UiFaderComponent()
    : m_fade(1.0f)
    , m_isFading(false)
    , m_fadeTarget(1.0f)
    , m_fadeSpeedInSeconds(1.0f)
{
    m_cachedPrimitive.m_vertices = nullptr;
    m_cachedPrimitive.m_numVertices = 0;
    m_cachedPrimitive.m_indices = nullptr;
    m_cachedPrimitive.m_numIndices = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiFaderComponent::~UiFaderComponent()
{
    if (m_isFading && m_entity)
    {
        EBUS_EVENT_ID(GetEntityId(), UiFaderNotificationBus, OnFaderDestroyed);
    }

    DestroyRenderTarget();

    // We only deallocate the vertices on destruction rather than every time we recreate the render
    // target. Changing the size of the element requires recreating render target but doesn't change
    // the number of vertices. Note this may be nullptr which is fine for delete.
    delete [] m_cachedPrimitive.m_vertices;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::Update(float deltaTime)
{
    if (!m_isFading)
    {
        return;
    }

    // Update fade
    SetFadeValueInternal(m_fade + m_fadeSpeedInSeconds * deltaTime);

    // Check for completion
    if (m_fadeSpeedInSeconds == 0 ||
        m_fadeSpeedInSeconds > 0 && m_fade >= m_fadeTarget ||
        m_fadeSpeedInSeconds < 0 && m_fade <= m_fadeTarget)
    {
        CompleteFade();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::Render(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
    UiRenderInterface* renderInterface, int numChildren, bool isInGame)
{
    static const float epsilon = 1.0f / 255.0f; // less than this value means alpha will be zero when converted to a uint8

    // if the fader is at (or close to) zero then do not render this element or its children at all
    if (m_fade < epsilon)
    {
        return;
    }

    if (GetUseRenderToTexture())
    {
        AZ::Vector2 pixelAlignedTopLeft, pixelAlignedBottomRight;
        ComputePixelAlignedBounds(pixelAlignedTopLeft, pixelAlignedBottomRight);
        AZ::Vector2 renderTargetSize = pixelAlignedBottomRight - pixelAlignedTopLeft;

        bool needsResize = static_cast<int>(renderTargetSize.GetX()) != m_renderTargetWidth || static_cast<int>(renderTargetSize.GetY()) != m_renderTargetHeight;
        if (m_attachmentImageId.IsEmpty() || needsResize)
        {
            // We delay first creation of the render target until render time since size is not known in Activate
            // We also call this if the size has changed
            CreateOrResizeRenderTarget(pixelAlignedTopLeft, pixelAlignedBottomRight);
        }

        // if the render target failed to be created (zero size for example) we don't render the element at all
        if (m_attachmentImageId.IsEmpty())
        {
            return;
        }

        // Do render-to-texture fade, this renders this element and its children to a render target, then renders that
        RenderRttFader(renderGraph, elementInterface, renderInterface, numChildren, isInGame);
    }
    else
    {
        // destroy previous render target, if exists
        if (!m_attachmentImageId.IsEmpty())
        {
            DestroyRenderTarget();
        }

        // do standard (non-render-to-texture) fade, this renders this element and its children
        RenderStandardFader(renderGraph, elementInterface, renderInterface, numChildren, isInGame);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiFaderComponent::GetFadeValue()
{
    return m_fade;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::SetFadeValue(float fade)
{
    if (m_isFading)
    {
        EBUS_EVENT_ID(GetEntityId(), UiFaderNotificationBus, OnFadeInterrupted);
        m_isFading = false;
    }

    SetFadeValueInternal(fade);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::Fade(float targetValue, float speed)
{
    if (m_isFading)
    {
        EBUS_EVENT_ID(GetEntityId(), UiFaderNotificationBus, OnFadeInterrupted);
    }

    // Connect to UpdateBus for updates while fading
    if (!UiCanvasUpdateNotificationBus::Handler::BusIsConnected())
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);

        // if this element has not been fixed up then canvasEntityId will be invalid. We handle this
        // in OnUiElementFixup
        if (canvasEntityId.IsValid())
        {
            UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }

    m_isFading = true;
    m_fadeTarget = clamp_tpl(targetValue, 0.0f, 1.0f);

    // Give speed a direction
    float fadeChange = m_fadeTarget - m_fade;
    float fadeDirection = fadeChange >= 0.0f ? 1.0f : -1.0f;
    m_fadeSpeedInSeconds = fadeDirection * speed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiFaderComponent::IsFading()
{
    return m_isFading;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiFaderComponent::GetUseRenderToTexture()
{
    return m_useRenderToTexture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::SetUseRenderToTexture(bool useRenderToTexture)
{
    if (GetUseRenderToTexture() != useRenderToTexture)
    {
        m_useRenderToTexture = useRenderToTexture;
        OnRenderTargetChange();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::PropertyValuesChanged()
{
    MarkRenderGraphDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::OnUiElementFixup(AZ::EntityId canvasEntityId, [[maybe_unused]] AZ::EntityId parentEntityId)
{
    // If we are fading but not already connected to UpdateBus for updates then connect
    // This would only happen if Fade was called during activate (before fixup)
    if (m_isFading && !UiCanvasUpdateNotificationBus::Handler::BusIsConnected())
    {
        UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::OnCanvasSpaceRectChanged(AZ::EntityId /*entityId*/, const UiTransformInterface::Rect& /*oldRect*/, const UiTransformInterface::Rect& /*newRect*/)
{
    // we only listen for this if using render target, if rect changed recreate render target
    OnRenderTargetChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::OnTransformToViewportChanged()
{
    // we only listen for this if using render target, if transform changed recreate render target
    OnRenderTargetChange();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiFaderComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiFaderComponent, AZ::Component>()
            ->Version(1)
            ->Field("Fade", &UiFaderComponent::m_fade)
            ->Field("UseRenderToTexture", &UiFaderComponent::m_useRenderToTexture);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiFaderComponent>("Fader", "A component that can fade its element and all its child elements");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiFader.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiFader.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::Slider, &UiFaderComponent::m_fade, "Fade", "The initial fade value")
                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiFaderComponent::OnFadeValueChanged);
 
            editInfo->DataElement(0, &UiFaderComponent::m_useRenderToTexture, "Use render to texture",
                "If true, this element and all children are rendered to a separate render target\n"
                "and then that target is rendered to the screen. This avoids child elements\n"
                "blending with each other as they fade. But it is more expensive.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiFaderComponent::OnRenderTargetChange);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiFaderBus>("UiFaderBus")
            ->Event("GetFadeValue", &UiFaderBus::Events::GetFadeValue)
            ->Event("SetFadeValue", &UiFaderBus::Events::SetFadeValue)
            ->Event("Fade", &UiFaderBus::Events::Fade)
            ->Event("IsFading", &UiFaderBus::Events::IsFading)
            ->Event("GetUseRenderToTexture", &UiFaderBus::Events::GetUseRenderToTexture)
            ->Event("SetUseRenderToTexture", &UiFaderBus::Events::SetUseRenderToTexture)
            ->VirtualProperty("Fade", "GetFadeValue", "SetFadeValue");

        behaviorContext->Class<UiFaderComponent>()->RequestBus("UiFaderBus");

        behaviorContext->EBus<UiFaderNotificationBus>("UiFaderNotificationBus")
            ->Handler<BehaviorUiFaderNotificationBusHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::Activate()
{
    UiRenderControlBus::Handler::BusConnect(GetEntityId());
    UiFaderBus::Handler::BusConnect(GetEntityId());
    UiAnimateEntityBus::Handler::BusConnect(GetEntityId());
    UiElementNotificationBus::Handler::BusConnect(GetEntityId());

    if (GetUseRenderToTexture())
    {
        UiTransformChangeNotificationBus::Handler::BusConnect(m_entity->GetId());
    }

    // The first time the component is activated we don't want to connect to the update bus. However if
    // the element starts a fade and then we deactivate and reactivate we need to reconnect to the
    // update bus.
    if (m_isFading)
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        if (canvasEntityId.IsValid())
        {
            UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }

    // set the render target name to an automatically generated name based on entity Id
    m_renderTargetName = "FaderTarget_";
    m_renderTargetName += GetEntityId().ToString();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::Deactivate()
{
    UiRenderControlBus::Handler::BusDisconnect();
    UiFaderBus::Handler::BusDisconnect();
    UiAnimateEntityBus::Handler::BusDisconnect();
    UiElementNotificationBus::Handler::BusDisconnect();

    if (UiTransformChangeNotificationBus::Handler::BusIsConnected())
    {
        UiTransformChangeNotificationBus::Handler::BusDisconnect();
    }

    // if deactivated during a fade we either have to cancel the fade or rely on activate reconnecting
    // to the UpdateBus. We do the latter.
    if (UiCanvasUpdateNotificationBus::Handler::BusIsConnected())
    {
        UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::CompleteFade()
{
    SetFadeValueInternal(m_fadeTarget);
    // Queue the OnFadeComplete event to prevent deletions during the canvas update
    EBUS_QUEUE_EVENT_ID(GetEntityId(), UiFaderNotificationBus, OnFadeComplete);
    m_isFading = false;

    // Disconnect from UpdateBus
    if (UiCanvasUpdateNotificationBus::Handler::BusIsConnected())
    {
        UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::SetFadeValueInternal(float fade)
{
    if (m_fade != fade)
    {
        m_fade = fade;
        MarkRenderGraphDirty();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::OnFadeValueChanged()
{
    MarkRenderGraphDirty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::OnRenderTargetChange()
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
void UiFaderComponent::MarkRenderGraphDirty()
{
    // tell the canvas to invalidate the render graph
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID(canvasEntityId, UiCanvasComponentImplementationBus, MarkRenderGraphDirty);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::CreateOrResizeRenderTarget(const AZ::Vector2& pixelAlignedTopLeft, const AZ::Vector2& pixelAlignedBottomRight)
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
    EBUS_EVENT_ID_RESULT(m_attachmentImageId, canvasEntityId, LyShine::RenderToTextureRequestBus, UseRenderTarget, AZ::Name(m_renderTargetName.c_str()), imageSize);
    if (m_attachmentImageId.IsEmpty())
    {
        AZ_Warning("UI", false, "Failed to create render target for UiFaderComponent");
    }

    // at this point either all render targets and depth surfaces are created or none are.
    // If all succeeded then update the render target size
    if (!m_attachmentImageId.IsEmpty())
    {
        m_renderTargetWidth = static_cast<int>(renderTargetSize.GetX());
        m_renderTargetHeight = static_cast<int>(renderTargetSize.GetY());
    }

    UpdateCachedPrimitive(pixelAlignedTopLeft, pixelAlignedBottomRight);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::DestroyRenderTarget()
{
    if (!m_attachmentImageId.IsEmpty())
    {
        AZ::EntityId canvasEntityId;
        EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
        EBUS_EVENT_ID(canvasEntityId, LyShine::RenderToTextureRequestBus, ReleaseRenderTarget, m_attachmentImageId);
        m_attachmentImageId = AZ::RHI::AttachmentId{};
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::UpdateCachedPrimitive(const AZ::Vector2& pixelAlignedTopLeft, const AZ::Vector2& pixelAlignedBottomRight)
{
    // update viewport position
    m_viewportTopLeft = pixelAlignedTopLeft;

    // now create the verts to render the texture to the screen, we cache these in m_cachedPrimitive
    const int numVertices = 4;
    if (!m_cachedPrimitive.m_vertices)
    {
        // verts not yet allocated, allocate them now
        const int numIndices = 6;
        m_cachedPrimitive.m_vertices = new SVF_P2F_C4B_T2F_F4B[numVertices];
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
void UiFaderComponent::ComputePixelAlignedBounds(AZ::Vector2& pixelAlignedTopLeft, AZ::Vector2& pixelAlignedBottomRight)
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
void UiFaderComponent::RenderStandardFader(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
    UiRenderInterface* renderInterface, int numChildren, bool isInGame)
{
    // Push the fade value that is used for this element and children
    renderGraph->PushAlphaFade(m_fade);

    // Render this element and its children
    RenderElementAndChildren(renderGraph, elementInterface, renderInterface, numChildren, isInGame);

    // Pop off the fade from this fader
    renderGraph->PopAlphaFade();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::RenderRttFader(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
    UiRenderInterface* renderInterface, int numChildren, bool isInGame)
{
    // Get the render target
    AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage;
    AZ::EntityId canvasEntityId;
    EBUS_EVENT_ID_RESULT(canvasEntityId, GetEntityId(), UiElementBus, GetCanvasEntityId);
    EBUS_EVENT_ID_RESULT(attachmentImage, canvasEntityId, LyShine::RenderToTextureRequestBus, GetRenderTarget, m_attachmentImageId);

    // Render the element and its children to a render target
    {
        // we always clear to transparent black - the accumulation of alpha in the render target requires it
        AZ::Color clearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // Start building the render to texture node in the render graph
        LyShine::RenderGraph* lyRenderGraph = static_cast<LyShine::RenderGraph*>(renderGraph); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
        lyRenderGraph->BeginRenderToTexture(attachmentImage, m_viewportTopLeft, m_viewportSize, clearColor);

        // We don't want this fader or parent faders to affect what is rendered to the render target since we will
        // apply those fades when we render from the render target.
        renderGraph->PushOverrideAlphaFade(1.0f);

        // Render this element and its children
        RenderElementAndChildren(renderGraph, elementInterface, renderInterface, numChildren, isInGame);

        // finish building the render to texture node in the render graph
        renderGraph->EndRenderToTexture();

        // pop off the override alpha fade
        renderGraph->PopAlphaFade();
    }

    // render from the render target to the screen (or a parent render target) with the fade value
    {
        // set the alpha in the verts
        {
            float desiredAlpha = renderGraph->GetAlphaFade() * m_fade;
            uint8 desiredPackedAlpha = static_cast<uint8>(desiredAlpha * 255.0f);

            // If the fade value has changed we need to update the alpha values in the vertex colors but we do
            // not want to touch or recompute the RGB values
            if (m_cachedPrimitive.m_vertices[0].color.a != desiredPackedAlpha)
            {
                // go through all the cached vertices and update the alpha values
                UCol desiredPackedColor = m_cachedPrimitive.m_vertices[0].color;
                desiredPackedColor.a = desiredPackedAlpha;
                for (int i = 0; i < m_cachedPrimitive.m_numVertices; ++i)
                {
                    m_cachedPrimitive.m_vertices[i].color = desiredPackedColor;
                }
            }
        }

        // Add a primitive to render a quad using the render target we have created
        {
            LyShine::RenderGraph* lyRenderGraph = static_cast<LyShine::RenderGraph*>(renderGraph); // LYSHINE_ATOM_TODO - find a different solution from downcasting - GHI #3570
            if (lyRenderGraph)
            {
                // Set the texture and other render state required
                AZ::Data::Instance<AZ::RPI::Image> image = attachmentImage;
                bool isClampTextureMode = true;
                bool isTextureSRGB = true;
                bool isTexturePremultipliedAlpha = true;
                LyShine::BlendMode blendMode = LyShine::BlendMode::Normal;
                lyRenderGraph->AddPrimitiveAtom(&m_cachedPrimitive, image, isClampTextureMode, isTextureSRGB, isTexturePremultipliedAlpha, blendMode);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiFaderComponent::RenderElementAndChildren(LyShine::IRenderGraph* renderGraph, UiElementInterface* elementInterface,
     UiRenderInterface* renderInterface, int numChildren, bool isInGame)
{
    // Render the visual component for this element (if there is one)
    if (renderInterface)
    {
        renderInterface->Render(renderGraph);
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
