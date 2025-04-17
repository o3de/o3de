/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiRadioButtonGroupComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiRadioButtonBus.h>
#include <LyShine/Bus/UiRadioButtonCommunicationBus.h>

#include <LyShine/UiSerializeHelpers.h>
#include "UiSerialize.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiRadioButtonGroupNotificationBus Behavior context handler class
class UiRadioButtonGroupNotificationBusBehaviorHandler
    : public UiRadioButtonGroupNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiRadioButtonGroupNotificationBusBehaviorHandler, "{A8D1A53C-7419-4EBA-8B73-EA4C5F6ED2DA}", AZ::SystemAllocator,
        OnRadioButtonGroupStateChange);

    void OnRadioButtonGroupStateChange(AZ::EntityId checked) override
    {
        Call(FN_OnRadioButtonGroupStateChange, checked);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRadioButtonGroupComponent::UiRadioButtonGroupComponent()
    : m_allowUncheck(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiRadioButtonGroupComponent::~UiRadioButtonGroupComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiRadioButtonGroupComponent::GetCheckedRadioButton()
{
    return m_checkedEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::SetState(AZ::EntityId radioButton, bool isOn)
{
    SetStateCommon(radioButton, isOn, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiRadioButtonGroupComponent::GetAllowUncheck()
{
    return m_allowUncheck;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::SetAllowUncheck(bool allowUncheck)
{
    m_allowUncheck = allowUncheck;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::AddRadioButton(AZ::EntityId radioButton)
{
    if (RegisterRadioButton(radioButton))
    {
        // Let it know it is now in the group
        UiRadioButtonCommunicationBus::Event(radioButton, &UiRadioButtonCommunicationBus::Events::SetGroup, GetEntityId());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::RemoveRadioButton(AZ::EntityId radioButton)
{
    UiRadioButtonCommunicationBus::Event(radioButton, &UiRadioButtonCommunicationBus::Events::SetGroup, AZ::EntityId());
    UnregisterRadioButton(radioButton);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiRadioButtonGroupComponent::ContainsRadioButton(AZ::EntityId radioButton)
{
    return m_radioButtons.find(radioButton) != m_radioButtons.end();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiRadioButtonGroupComponent::GetChangedActionName()
{
    return m_changedActionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::SetChangedActionName(const LyShine::ActionName& actionName)
{
    m_changedActionName = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiRadioButtonGroupComponent::RegisterRadioButton(AZ::EntityId radioButton)
{
    // Check if the given entity actually has the radio button component on it
    bool isRadioButton = UiRadioButtonBus::FindFirstHandler(radioButton) != nullptr;

    // Check that the button is actually a radio button before adding it to the group
    if (isRadioButton)
    {
        // Try adding the button to the group, and if it wasn't in the group before then
        if (m_radioButtons.insert(radioButton).second)
        {
            // If the button that is getting added is already checked, uncheck the currently checked one
            bool isOn;
            UiRadioButtonBus::EventResult(isOn, radioButton, &UiRadioButtonBus::Events::GetState);
            if (isOn)
            {
                if (m_checkedEntity.IsValid())
                {
                    // Uncheck our currently checked entity
                    UiRadioButtonCommunicationBus::Event(m_checkedEntity, &UiRadioButtonCommunicationBus::Events::SetState, false, false);
                    m_checkedEntity.SetInvalid();
                }
                m_checkedEntity = radioButton;
            }

            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::UnregisterRadioButton(AZ::EntityId radioButton)
{
    m_radioButtons.erase(radioButton);

    // If the button that is getting removed was the checked entity, set the check entity to invalid
    if (radioButton == m_checkedEntity)
    {
        m_checkedEntity.SetInvalid();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::RequestRadioButtonStateChange(AZ::EntityId radioButton, bool newState)
{
    SetStateCommon(radioButton, newState, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::Activate()
{
    UiRadioButtonGroupBus::Handler::BusConnect(GetEntityId());
    UiRadioButtonGroupCommunicationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::Deactivate()
{
    UiRadioButtonGroupBus::Handler::BusDisconnect(GetEntityId());
    UiRadioButtonGroupCommunicationBus::Handler::BusDisconnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiRadioButtonGroupComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiRadioButtonGroupComponent, AZ::Component>()
            ->Version(1)
        // Settings group
            ->Field("AllowRestoreUnchecked", &UiRadioButtonGroupComponent::m_allowUncheck)
        // Actions group
            ->Field("ChangedActionName", &UiRadioButtonGroupComponent::m_changedActionName);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiRadioButtonGroupComponent>("RadioButtonGroup", "A component for RadioButtonGroup behavior.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiRadioButtonGroup.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiRadioButtonGroup.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Settings group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Settings")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiRadioButtonGroupComponent::m_allowUncheck, "Allow uncheck", "Allow clicking on the selected radio button to uncheck it.");
            }

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiRadioButtonGroupComponent::m_changedActionName, "Change", "The action triggered when value changes.");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiRadioButtonGroupBus>("UiRadioButtonGroupBus")
            ->Event("GetState", &UiRadioButtonGroupBus::Events::GetCheckedRadioButton)
            ->Event("SetState", &UiRadioButtonGroupBus::Events::SetState)
            ->Event("GetAllowUncheck", &UiRadioButtonGroupBus::Events::GetAllowUncheck)
            ->Event("SetAllowUncheck", &UiRadioButtonGroupBus::Events::SetAllowUncheck)
            ->Event("AddRadioButton", &UiRadioButtonGroupBus::Events::AddRadioButton)
            ->Event("RemoveRadioButton", &UiRadioButtonGroupBus::Events::RemoveRadioButton)
            ->Event("ContainsRadioButton", &UiRadioButtonGroupBus::Events::ContainsRadioButton)
            ->Event("GetChangedActionName", &UiRadioButtonGroupBus::Events::GetChangedActionName)
            ->Event("SetChangedActionName", &UiRadioButtonGroupBus::Events::SetChangedActionName);

        behaviorContext->EBus<UiRadioButtonGroupNotificationBus>("UiRadioButtonGroupNotificationBus")
            ->Handler<UiRadioButtonGroupNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiRadioButtonGroupComponent::SetStateCommon(AZ::EntityId radioButton, bool isOn, bool sendNotifications)
{
    // First check if the button is actually in the group
    if (radioButton.IsValid() && ContainsRadioButton(radioButton))
    {
        // If this is a request to be checked
        if (isOn)
        {
            // Check if we currently have a checked radio button
            if (m_checkedEntity.IsValid())
            {
                UiRadioButtonCommunicationBus::Event(
                    m_checkedEntity, &UiRadioButtonCommunicationBus::Events::SetState, false, sendNotifications);
                m_checkedEntity.SetInvalid();
            }

            m_checkedEntity = radioButton;
            UiRadioButtonCommunicationBus::Event(
                m_checkedEntity, &UiRadioButtonCommunicationBus::Events::SetState, true, sendNotifications);
        }
        else if (m_allowUncheck && radioButton == m_checkedEntity) // && isOn == false
        {
            UiRadioButtonCommunicationBus::Event(
                m_checkedEntity, &UiRadioButtonCommunicationBus::Events::SetState, false, sendNotifications);
            m_checkedEntity.SetInvalid();
        }
        else // we didn't change anything, don't send events
        {
            return;
        }

        if (sendNotifications)
        {
            if (!m_changedActionName.empty())
            {
                AZ::EntityId canvasEntityId;
                UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
                UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_changedActionName);
            }
            UiRadioButtonGroupNotificationBus::Event(
                GetEntityId(), &UiRadioButtonGroupNotificationBus::Events::OnRadioButtonGroupStateChange, m_checkedEntity);
        }
    }
}

