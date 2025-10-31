/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiTooltipComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipComponent::UiTooltipComponent()
    : m_curTriggerMode(UiTooltipDisplayInterface::TriggerMode::OnHover)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipComponent::~UiTooltipComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::Update([[maybe_unused]] float deltaTime)
{
    if (m_curDisplayElementId.IsValid())
    {
        UiTooltipDisplayBus::Event(m_curDisplayElementId, &UiTooltipDisplayBus::Events::Update);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnHoverStart()
{
    if (GetDisplayElementTriggerMode() == UiTooltipDisplayInterface::TriggerMode::OnHover)
    {
        TriggerTooltip(UiTooltipDisplayInterface::TriggerMode::OnHover);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnHoverEnd()
{
    if (IsTriggeredWithMode(UiTooltipDisplayInterface::TriggerMode::OnHover))
    {
        HideDisplayElement();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnPressed()
{
    if (IsTriggeredWithMode(UiTooltipDisplayInterface::TriggerMode::OnHover))
    {
        HideDisplayElement();
    }
    else
    {
        if (GetDisplayElementTriggerMode() == UiTooltipDisplayInterface::TriggerMode::OnPress)
        {
            TriggerTooltip(UiTooltipDisplayInterface::TriggerMode::OnPress);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnReleased()
{
    if (IsTriggeredWithMode(UiTooltipDisplayInterface::TriggerMode::OnPress))
    {
        HideDisplayElement();
    }
    else
    {
        if (GetDisplayElementTriggerMode() == UiTooltipDisplayInterface::TriggerMode::OnClick)
        {
            TriggerTooltip(UiTooltipDisplayInterface::TriggerMode::OnClick);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnCanvasPrimaryReleased([[maybe_unused]] AZ::EntityId entityId)
{
    // This callback is needed because OnReleased is only called when the mouse is over the element
    if (IsTriggeredWithMode(UiTooltipDisplayInterface::TriggerMode::OnPress))
    {
        HideDisplayElement();
    }
    if (IsTriggeredWithMode(UiTooltipDisplayInterface::TriggerMode::OnClick))
    {
        HideDisplayElement();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::PushDataToDisplayElement(AZ::EntityId displayEntityId)
{
    AZ::EntityId textEntityId;
    UiTooltipDisplayBus::EventResult(textEntityId, displayEntityId, &UiTooltipDisplayBus::Events::GetTextEntity);

    if (textEntityId.IsValid())
    {
        UiTextBus::Event(textEntityId, &UiTextBus::Events::SetText, m_text);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTooltipComponent::GetText()
{
    return m_text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::SetText(const AZStd::string& text)
{
    m_text = text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnHiding()
{
    HandleDisplayElementHidden();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::OnHidden()
{
    HandleDisplayElementHidden();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiTooltipComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiTooltipComponent, AZ::Component>()
            ->Version(1)
            ->Field("Text", &UiTooltipComponent::m_text);

        AZ::EditContext* ec = serializeContext->GetEditContext();

        if (ec)
        {
            auto editInfo = ec->Class<UiTooltipComponent>("Tooltip", "A component that provides the data needed to display a tooltip.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiTooltip.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiTooltip.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            editInfo->DataElement(0, &UiTooltipComponent::m_text, "Text", "The text string.");
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiTooltipBus>("UiTooltipBus")
            ->Event("GetText", &UiTooltipBus::Events::GetText)
            ->Event("SetText", &UiTooltipBus::Events::SetText);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::Activate()
{
    UiInteractableNotificationBus::Handler::BusConnect(GetEntityId());
    UiTooltipDataPopulatorBus::Handler::BusConnect(GetEntityId());
    UiTooltipBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::Deactivate()
{
    UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
    UiInteractableNotificationBus::Handler::BusDisconnect();
    UiCanvasInputNotificationBus::Handler::BusDisconnect();
    UiTooltipDisplayNotificationBus::Handler::BusDisconnect();
    UiTooltipDataPopulatorBus::Handler::BusDisconnect();
    UiTooltipBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::HideDisplayElement()
{
    if (m_curDisplayElementId.IsValid())
    {
        UiTooltipDisplayBus::Event(m_curDisplayElementId, &UiTooltipDisplayBus::Events::Hide);
        HandleDisplayElementHidden();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::HandleDisplayElementHidden()
{
    if (m_curDisplayElementId.IsValid())
    {
        m_curDisplayElementId.SetInvalid();
        UiCanvasUpdateNotificationBus::Handler::BusDisconnect();
        UiCanvasInputNotificationBus::Handler::BusDisconnect();
        UiTooltipDisplayNotificationBus::Handler::BusDisconnect();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTooltipComponent::TriggerTooltip(UiTooltipDisplayInterface::TriggerMode triggerMode)
{
    if (IsTriggered())
    {
        return;
    }

    // Get display element
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    AZ::EntityId displayElementId;
    UiCanvasBus::EventResult(displayElementId, canvasEntityId, &UiCanvasBus::Events::GetTooltipDisplayElement);

    if (displayElementId.IsValid())
    {
        UiTooltipDisplayBus::Event(displayElementId, &UiTooltipDisplayBus::Events::PrepareToShow, GetEntityId());

        m_curDisplayElementId = displayElementId;
        m_curTriggerMode = triggerMode;

        UiCanvasUpdateNotificationBus::Handler::BusConnect(canvasEntityId);
        UiTooltipDisplayNotificationBus::Handler::BusConnect(GetEntityId());

        if (m_curTriggerMode != UiTooltipDisplayInterface::TriggerMode::OnHover)
        {
            UiCanvasInputNotificationBus::Handler::BusConnect(canvasEntityId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTooltipComponent::IsTriggered()
{
    return m_curDisplayElementId.IsValid();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTooltipComponent::IsTriggeredWithMode(UiTooltipDisplayInterface::TriggerMode triggerMode)
{
    return IsTriggered() && m_curTriggerMode == triggerMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTooltipDisplayInterface::TriggerMode UiTooltipComponent::GetDisplayElementTriggerMode()
{
    UiTooltipDisplayInterface::TriggerMode triggerMode = UiTooltipDisplayInterface::TriggerMode::OnHover;

    // Get display element
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    AZ::EntityId displayElementId;
    UiCanvasBus::EventResult(displayElementId, canvasEntityId, &UiCanvasBus::Events::GetTooltipDisplayElement);
    // Get display element's trigger mode
    UiTooltipDisplayBus::EventResult(triggerMode, displayElementId, &UiTooltipDisplayBus::Events::GetTriggerMode);

    return triggerMode;
}
