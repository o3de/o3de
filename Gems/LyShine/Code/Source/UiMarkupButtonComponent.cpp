/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiMarkupButtonComponent.h"

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiMarkupButtonBus.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace
{
    //! Given a UI element on a canvas, return the mouse position.
    AZ::Vector2 GetMousePosition(AZ::EntityId entityId)
    {
        AZ::EntityId canvasId;
        UiElementBus::EventResult(canvasId, entityId, &UiElementBus::Events::GetCanvasEntityId);
        AZ::Vector2 mousePos = AZ::Vector2::CreateZero();
        UiCanvasBus::EventResult(mousePos, canvasId, &UiCanvasBus::Events::GetMousePosition);
        return mousePos;
    }

    //! Returns an index to the given list of clickable text rects that contains the given mouse position, otherwise returns negative.
    int FindClickableTextRectIndexFromCanvasSpacePoint(const AZ::Vector2& canvasSpacePosition, const UiClickableTextInterface::ClickableTextRects& clickableTextRects)
    {
        // Iterate through the clickable rects to find one that contains the point
        int clickableRectIndex = -1;
        const int numClickableRects = static_cast<int>(clickableTextRects.size());
        for (int i = 0; i < numClickableRects; ++i)
        {
            const auto& clickableRect = clickableTextRects[i];
            const UiTransformInterface::Rect& rect = clickableRect.rect;
            const bool containedX = canvasSpacePosition.GetX() >= rect.left && canvasSpacePosition.GetX() <= rect.right;
            if (containedX)
            {
                const bool containedY = canvasSpacePosition.GetY() >= rect.top && canvasSpacePosition.GetY() <= rect.bottom;
                if (containedY)
                {
                    clickableRectIndex = i;
                    break;
                }
            }
        }

        return clickableRectIndex;
    }

    //! Returns an index to the given list of clickable text rects that contains the given mouse position, otherwise returns negative.
    int FindClickableTextRectIndexFromViewportSpacePoint(AZ::EntityId entityId, const AZ::Vector2& mousePos, const UiClickableTextInterface::ClickableTextRects& clickableTextRects)
    {
        // first transform the mousePos from viewport space to "canvas space no-scale-rotate", which is the space that clickableTextRects
        // are stored in.
        AZ::Matrix4x4 transformFromViewport;
        UiTransformBus::Event(entityId, &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);
        AZ::Vector3 point3(mousePos.GetX(), mousePos.GetY(), 0.0f);
        point3 = transformFromViewport * point3;
        AZ::Vector2 canvasSpacePosition(point3.GetX(), point3.GetY());

        return FindClickableTextRectIndexFromCanvasSpacePoint(canvasSpacePosition, clickableTextRects);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiMarkupButtonNotificationBus Behavior context handler class
class UiMarkupButtonNotificationBusBehaviorHandler
    : public UiMarkupButtonNotificationsBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiMarkupButtonNotificationBusBehaviorHandler, "{ACCF73DC-86DD-4D1C-85B3-1E016BAAA495}", AZ::SystemAllocator,
        OnHoverStart,
        OnHoverEnd,
        OnPressed,
        OnReleased,
        OnClick);

    void OnHoverStart(int id, const AZStd::string& action, const AZStd::string& data) override
    {
        Call(FN_OnHoverStart, id, action, data);
    }

    void OnHoverEnd(int id, const AZStd::string& action, const AZStd::string& data) override
    {
        Call(FN_OnHoverEnd, id, action, data);
    }

    void OnPressed(int id, const AZStd::string& action, const AZStd::string& data) override
    {
        Call(FN_OnPressed, id, action, data);
    }

    void OnReleased(int id, const AZStd::string& action, const AZStd::string& data) override
    {
        Call(FN_OnReleased, id, action, data);
    }

    void OnClick(int id, const AZStd::string& action, const AZStd::string& data) override
    {
        Call(FN_OnClick, id, action, data);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiMarkupButtonComponent::UiMarkupButtonComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiMarkupButtonComponent::~UiMarkupButtonComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiMarkupButtonComponent::GetLinkColor()
{
    return m_linkColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::SetLinkColor(const AZ::Color& linkColor)
{
    m_linkColor = linkColor;
    OnLinkColorChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiMarkupButtonComponent::GetLinkHoverColor()
{
    return m_linkHoverColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::SetLinkHoverColor(const AZ::Color& linkHoverColor)
{
    m_linkHoverColor = linkHoverColor;
    OnLinkHoverColorChanged();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMarkupButtonComponent::HandlePressed(AZ::Vector2 point, bool& shouldStayActive)
{
    const bool handled = UiInteractableComponent::HandlePressed(point, shouldStayActive);

    if (!handled)
    {
        return false;
    }

    const int clickableRectIndex = FindClickableTextRectIndexFromViewportSpacePoint(GetEntityId(), point, m_clickableTextRects);
    if (clickableRectIndex >= 0)
    {
        const int clickableId = m_clickableTextRects[clickableRectIndex].id;
        const AZStd::string& action = m_clickableTextRects[clickableRectIndex].action;
        const AZStd::string& data = m_clickableTextRects[clickableRectIndex].data;
        m_clickableRectPressedIndex = clickableRectIndex;
        UiMarkupButtonNotificationsBus::Event(GetEntityId(), &UiMarkupButtonNotificationsBus::Events::OnPressed, clickableId, action, data);
    }

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMarkupButtonComponent::HandleReleased(AZ::Vector2 point)
{
    const bool handled = UiInteractableComponent::HandleReleased(point);

    if (!handled)
    {
        m_clickableRectPressedIndex = -1;
        return false;
    }

    // This could be negative if the clickable text change since the pressed
    // event occurred (OnClickableTextChanged resets the pressed index value).
    if (m_clickableRectPressedIndex < 0)
    {
        UiMarkupButtonNotificationsBus::Event(
            GetEntityId(), &UiMarkupButtonNotificationsBus::Events::OnReleased, -1, AZStd::string(), AZStd::string());
    }
    else
    {
        const int pressedClickableId = m_clickableTextRects[m_clickableRectPressedIndex].id;
        const AZStd::string& action = m_clickableTextRects[m_clickableRectPressedIndex].action;
        const AZStd::string& data = m_clickableTextRects[m_clickableRectPressedIndex].data;
        UiMarkupButtonNotificationsBus::Event(
            GetEntityId(), &UiMarkupButtonNotificationsBus::Events::OnReleased, pressedClickableId, action, data);

        bool onClickTriggered = false;
        const int releasedClickableRectIndex = FindClickableTextRectIndexFromViewportSpacePoint(GetEntityId(), point, m_clickableTextRects);
        if (releasedClickableRectIndex >= 0)
        {
            // If the release happens on the pressed link ID, trigger a click.
            const int releasedClickableId = m_clickableTextRects[releasedClickableRectIndex].id;
            if (releasedClickableId == pressedClickableId)
            {
                UiMarkupButtonNotificationsBus::Event(
                    GetEntityId(), &UiMarkupButtonNotificationsBus::Events::OnClick, pressedClickableId, action, data);
                onClickTriggered = true;
            }
        }

        if (!onClickTriggered)
        {
            // Clear the hover state now in case this entity is no longer 
            // being hovered. This can happen when the user releases the 
            // mouse outside of the clickable text rect.
            HandleClickableHoverEnd();
        }
    }

    m_clickableRectPressedIndex = -1;

    return handled;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::Update(float deltaTime)
{
    UiInteractableComponent::Update(deltaTime);
    UpdateHover();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::OnClickableTextChanged()
{
    m_clickableTextRects.clear();
    UiClickableTextBus::Event(GetEntityId(), &UiClickableTextBus::Events::GetClickableTextRects, m_clickableTextRects);

    // Reset all links back to their non-hover color
    int lastClickableId = -1;
    for (const auto& clickableText : m_clickableTextRects)
    {
        // Color is assigned by clickable ID and it's possible for 
        // multiple clickable text rects to share the same ID, so guard
        // against unnecessary calls to set the color for IDs that have
        // previously been set.
        if (lastClickableId != clickableText.id)
        {
            UiClickableTextBus::Event(GetEntityId(), &UiClickableTextBus::Events::SetClickableTextColor, clickableText.id, m_linkColor);
            lastClickableId = clickableText.id;
        }
    }

    // Because the clickable text has changed, our current hover and pressed
    // states may no longer apply. Update it again based on the new clickable
    // text rects and current mouse position.
    m_clickableRectHoverIndex = -1;
    m_clickableRectPressedIndex = -1;
    UpdateHover();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiMarkupButtonBus::Handler::BusConnect(GetEntityId());
    UiClickableTextNotificationsBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::Deactivate()
{
    UiInteractableComponent::Deactivate();
    UiMarkupButtonBus::Handler::BusDisconnect(GetEntityId());
    UiClickableTextNotificationsBus::Handler::BusDisconnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::UpdateHover()
{
    // Don't update hover state when we're actively being pressed. If we ever
    // add a pressed color, we could update this logic so that the pressed
    // color updates when the mouse moves on/off the clickable text.
    if (m_isHandlingEvents && !m_isPressed)
    {
        AZ::EntityId canvasId;
        UiElementBus::EventResult(canvasId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);

        AZ::EntityId hoverInteractable;
        UiCanvasBus::EventResult(hoverInteractable, canvasId, &UiCanvasBus::Events::GetHoverInteractable);

        // Similarly, the hover interactable won't updated while another 
        // element is being pressed - we don't want to update hover state
        // of any clickable text (on any entity) while a press is happening.
        if (hoverInteractable == GetEntityId())
        {
            const int rectIndex = FindClickableTextRectIndexFromViewportSpacePoint(GetEntityId(), GetMousePosition(GetEntityId()), m_clickableTextRects);
            int rectIndexClickableId = -1;

            if (rectIndex >= 0)
            {
                rectIndexClickableId = m_clickableTextRects[rectIndex].id;
            }

            int hoverClickableId = -1;
            if (m_clickableRectHoverIndex >= 0)
            {
                hoverClickableId = m_clickableTextRects[m_clickableRectHoverIndex].id;
            }
            
            const bool enteringHover = rectIndexClickableId >= 0 && hoverClickableId < 0;
            const bool leavingHover = rectIndexClickableId < 0 && hoverClickableId >= 0;
            const bool switchingHoverRect = rectIndexClickableId >= 0 && hoverClickableId >= 0 && rectIndexClickableId != hoverClickableId;
            if (enteringHover)
            {
                HandleClickableHoverStart(rectIndex);
            }
            else if (leavingHover)
            {
                HandleClickableHoverEnd();
            }
            else if (switchingHoverRect)
            {
                HandleClickableHoverEnd();
                HandleClickableHoverStart(rectIndex);
            }
        }
        else
        {
            // Not being pressed or hovered, so reset the hover index element
            // just in case it is set (this can occur if we never receive a
            // release event for the interactable).
            HandleClickableHoverEnd();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::HandleClickableHoverStart(int clickableRectIndex)
{
    m_clickableRectHoverIndex = clickableRectIndex;
    const int clickableId = m_clickableTextRects[m_clickableRectHoverIndex].id;

    // Set the link color prior to notification being triggered in case listeners want to set
    // the color themselves.
    UiClickableTextBus::Event(GetEntityId(), &UiClickableTextBus::Events::SetClickableTextColor, clickableId, m_linkHoverColor);

    const AZStd::string& action = m_clickableTextRects[m_clickableRectHoverIndex].action;
    const AZStd::string& data = m_clickableTextRects[m_clickableRectHoverIndex].data;
    UiMarkupButtonNotificationsBus::Event(GetEntityId(), &UiMarkupButtonNotificationsBus::Events::OnHoverStart, clickableId, action, data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::HandleClickableHoverEnd()
{
    if (m_clickableRectHoverIndex >= 0)
    {
        const int clickableId = m_clickableTextRects[m_clickableRectHoverIndex].id;

        // Set the link color prior to notification being triggered in case listeners want to set
        // the color themselves.
        UiClickableTextBus::Event(GetEntityId(), &UiClickableTextBus::Events::SetClickableTextColor, clickableId, m_linkColor);

        const AZStd::string& action = m_clickableTextRects[m_clickableRectHoverIndex].action;
        const AZStd::string& data = m_clickableTextRects[m_clickableRectHoverIndex].data;
        m_clickableRectHoverIndex = -1;
        UiMarkupButtonNotificationsBus::Event(GetEntityId(), &UiMarkupButtonNotificationsBus::Events::OnHoverEnd, clickableId, action, data);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::OnLinkColorChanged()
{
    // If  a link is being hovered (e.g. if SetLinkColor called at runtime while a link is being hovered)
    // then we do not want to set the color of that link
    int hoverClickableId = -1;
    if (m_clickableRectHoverIndex >= 0)
    {
        hoverClickableId = m_clickableTextRects[m_clickableRectHoverIndex].id;
    }

    // Set all links to the new link color (unless they are currently being hovered)
    int lastClickableId = -1;
    for (const auto& clickableText : m_clickableTextRects)
    {
        // Color is assigned by clickable ID and it's possible for 
        // multiple clickable text rects to share the same ID, so guard
        // against unnecessary calls to set the color for IDs that have
        // previously been set.
        // We also don't want to set the text to the link color if it is currently being hovered.
        if (lastClickableId != clickableText.id && clickableText.id != hoverClickableId)
        {
            UiClickableTextBus::Event(GetEntityId(), &UiClickableTextBus::Events::SetClickableTextColor, clickableText.id, m_linkColor);
            lastClickableId = clickableText.id;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::OnLinkHoverColorChanged()
{
    // If  a link is being hovered (e.g. if SetLinkColor called at runtime while a link is being hovered)
    // then we want to set the color of that link to the new hover color
    int hoverClickableId = -1;
    if (m_clickableRectHoverIndex >= 0)
    {
        hoverClickableId = m_clickableTextRects[m_clickableRectHoverIndex].id;
    }

    // Set any hovered links to the new link hover color
    int lastClickableId = -1;
    for (const auto& clickableText : m_clickableTextRects)
    {
        // Color is assigned by clickable ID and it's possible for 
        // multiple clickable text rects to share the same ID, so guard
        // against unnecessary calls to set the color for IDs that have
        // previously been set.
        if (lastClickableId != clickableText.id)
        {
            // If it is currently being hovered then set its color to the new link hover color
            if (clickableText.id == hoverClickableId)
            {
                UiClickableTextBus::Event(
                    GetEntityId(), &UiClickableTextBus::Events::SetClickableTextColor, clickableText.id, m_linkHoverColor);
            }

            lastClickableId = clickableText.id;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiMarkupButtonComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiMarkupButtonComponent, UiInteractableComponent>()
            ->Version(1, &VersionConverter)
            ->Field("LinkColor", &UiMarkupButtonComponent::m_linkColor)
            ->Field("LinkHoverColor", &UiMarkupButtonComponent::m_linkHoverColor);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiMarkupButtonComponent>("MarkupButton", "An interactable component for enabling clicks from markup text (mouse support only).");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                // Need to request markup button component icons for LY ML
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiMarkupButton.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiMarkupButton.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiMarkupButtonComponent::m_linkColor, "Link Color", "Link text color.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiMarkupButtonComponent::OnLinkColorChanged);

            editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiMarkupButtonComponent::m_linkHoverColor, "Link Hover Color", "Link text hover color.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiMarkupButtonComponent::OnLinkHoverColorChanged);
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiMarkupButtonBus>("UiMarkupButtonBus")
            ->Event("GetLinkColor", &UiMarkupButtonBus::Events::GetLinkColor)
            ->Event("SetLinkColor", &UiMarkupButtonBus::Events::SetLinkColor)
            ->Event("GetLinkHoverColor", &UiMarkupButtonBus::Events::GetLinkHoverColor)
            ->Event("SetLinkHoverColor", &UiMarkupButtonBus::Events::SetLinkHoverColor);

        behaviorContext->EBus<UiMarkupButtonNotificationsBus>("UiMarkupButtonNotificationsBus")
            ->Handler<UiMarkupButtonNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiMarkupButtonComponent::VersionConverter([[maybe_unused]] AZ::SerializeContext& context,
    [[maybe_unused]] AZ::SerializeContext::DataElementNode& classElement)
{
    return true;
}

#include "Tests/internal/test_UiMarkupButtonComponent.cpp"
