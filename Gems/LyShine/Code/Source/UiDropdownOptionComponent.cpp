/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiDropdownOptionComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>

#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiDropdownBus.h>
#include <LyShine/UiSerializeHelpers.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiDropdownNotificationBusBehaviorHandler  Behavior context handler class
class UiDropdownOptionNotificationBusBehaviorHandler
    : public UiDropdownOptionNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(UiDropdownOptionNotificationBusBehaviorHandler, "{3A13D6AF-70BF-4C8D-ACD3-A098FDC8D0C4}", AZ::SystemAllocator,
        OnDropdownOptionSelected);

    void OnDropdownOptionSelected() override
    {
        Call(FN_OnDropdownOptionSelected);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropdownOptionComponent::UiDropdownOptionComponent()
    : m_owningDropdown()
    , m_textElement()
    , m_iconElement()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropdownOptionComponent::~UiDropdownOptionComponent()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownOptionComponent::GetOwningDropdown()
{
    return m_owningDropdown;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownOptionComponent::SetOwningDropdown(AZ::EntityId owningDropdown)
{
    m_owningDropdown = owningDropdown;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownOptionComponent::GetTextElement()
{
    return m_textElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownOptionComponent::SetTextElement(AZ::EntityId textElement)
{
    m_textElement = textElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiDropdownOptionComponent::GetIconElement()
{
    return m_iconElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownOptionComponent::SetIconElement(AZ::EntityId iconElement)
{
    m_iconElement = iconElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownOptionComponent::InGamePostActivate()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownOptionComponent::OnReleased()
{
    // Tell our dropdown that we were selected
    UiDropdownBus::Event(m_owningDropdown, &UiDropdownBus::Events::SetValue, GetEntityId());

    // Tell our listeners that we were selected
    UiDropdownOptionNotificationBus::Event(GetEntityId(), &UiDropdownOptionNotificationBus::Events::OnDropdownOptionSelected);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownOptionComponent::Activate()
{
    UiDropdownOptionBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
    UiInteractableNotificationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiDropdownOptionComponent::Deactivate()
{
    UiDropdownOptionBus::Handler::BusDisconnect(GetEntityId());
    UiInitializationBus::Handler::BusDisconnect(GetEntityId());
    UiInteractableNotificationBus::Handler::BusDisconnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiDropdownOptionComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiDropdownOptionComponent, AZ::Component>()
            ->Version(1)
        // Elements group
            ->Field("OwningDropdown", &UiDropdownOptionComponent::m_owningDropdown)
            ->Field("TextElement", &UiDropdownOptionComponent::m_textElement)
            ->Field("IconElement", &UiDropdownOptionComponent::m_iconElement);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiDropdownOptionComponent>("DropdownOption", "An interactable component for DropdownOption behavior.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiDropdownOption.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiDropdownOption.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Elements group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Elements")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiDropdownOptionComponent::m_owningDropdown, "Owning Dropdown", "The dropdown this option belongs to (does not have to be its parent dropdown).")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiDropdownOptionComponent::PopulateDropdownsEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiDropdownOptionComponent::m_textElement, "Text Element", "The text element to use to show this option is selected.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiDropdownOptionComponent::PopulateChildEntityList);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiDropdownOptionComponent::m_iconElement, "Icon Element", "The icon element to use to show this option is selected.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiDropdownOptionComponent::PopulateChildEntityList);
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiDropdownOptionBus>("UiDropdownOptionBus")
            ->Event("GetOwningDropdown", &UiDropdownOptionBus::Events::GetOwningDropdown)
            ->Event("SetOwningDropdown", &UiDropdownOptionBus::Events::SetOwningDropdown)
            ->Event("GetTextElement", &UiDropdownOptionBus::Events::GetTextElement)
            ->Event("SetTextElement", &UiDropdownOptionBus::Events::SetTextElement)
            ->Event("GetIconElement", &UiDropdownOptionBus::Events::GetIconElement)
            ->Event("SetIconElement", &UiDropdownOptionBus::Events::SetIconElement);

        behaviorContext->EBus<UiDropdownOptionNotificationBus>("UiDropdownOptionNotificationBus")
            ->Handler<UiDropdownOptionNotificationBusBehaviorHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropdownOptionComponent::EntityComboBoxVec UiDropdownOptionComponent::PopulateDropdownsEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // Get a list of all elements in the canvas with the dropdown component
    AZ::EntityId canvasEntityId;
    UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
    LyShine::EntityArray dropdowns;
    UiCanvasBus::Event(
        canvasEntityId,
        &UiCanvasBus::Events::FindElements,
        [](const AZ::Entity* entity)
        {
            return UiDropdownBus::FindFirstHandler(entity->GetId()) != nullptr;
        },
        dropdowns);

    // Sort the elements by name
    AZStd::sort(dropdowns.begin(), dropdowns.end(),
        [](const AZ::Entity* e1, const AZ::Entity* e2) { return e1->GetName() < e2->GetName(); });

    // add their names to the StringList and their IDs to the id list
    for (auto dropdown : dropdowns)
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(dropdown->GetId()), dropdown->GetName()));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiDropdownOptionComponent::EntityComboBoxVec UiDropdownOptionComponent::PopulateChildEntityList()
{
    EntityComboBoxVec result;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // Get a list of all child elements
    LyShine::EntityArray children;
    UiElementBus::EventResult(children, GetEntityId(), &UiElementBus::Events::GetChildElements);

    // add their names to the StringList and their IDs to the id list
    for (auto childEntity : children)
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(childEntity->GetId()), childEntity->GetName()));
    }

    return result;
}
