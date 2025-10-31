/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiHierarchyInteractivityToggleComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiInteractableBus.h>

AZ_COMPONENT_IMPL(UiHierarchyInteractivityToggleComponent, "UiHierarchyInteractivityToggleComponent", "{B8C5A864-1A98-48B9-BEBB-1FDE06E6D463}");

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::Activate()
{
    UiHierarchyInteractivityToggleBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::Deactivate()
{
    UiHierarchyInteractivityToggleBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::InGamePostActivate()
{
    UiInitializationBus::Handler::BusDisconnect();

    if (!m_isInteractionLocallyEnabled)
    {
        SetInteractivity(false);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiHierarchyInteractivityToggleComponent::SetInteractivity(bool enabled)
{
    m_isInteractionLocallyEnabled = enabled;
    UpdateInteractiveState();
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiHierarchyInteractivityToggleComponent::SetParentInteractivity(bool parentEnabled)
{
    m_isInteractionParentEnabled = parentEnabled;
    UpdateInteractiveState();
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::UpdateInteractiveState()
{
    const bool effectiveState = GetInteractiveState();

    // Affect the current entity.
    UiInteractableBus::Event(GetEntityId(), &UiInteractableInterface::SetIsHandlingEvents, effectiveState);
    UiInteractableBus::Event(GetEntityId(), &UiInteractableInterface::SetIsHandlingMultiTouchEvents, effectiveState);

    DoRecursiveSetInteractivityToChildren(GetEntityId(), effectiveState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::DoRecursiveSetInteractivityToChildren(const AZ::EntityId& parentId, bool parentState)
{
    AZStd::vector<AZ::EntityId> children;
    UiElementBus::EventResult(children, parentId, &UiElementInterface::GetChildEntityIds);

    for (const AZ::EntityId& child : children)
    {
        // If has InteractivityToggleComp, will return true;
        bool hasGroup = false;
        UiHierarchyInteractivityToggleBus::EventResult(hasGroup, child, &UiHierarchyInteractivityToggleInterface::SetParentInteractivity, parentState);

        // Because no toggle found, affect child and recurse.
        if (!hasGroup)
        {
            // Affect interactable state directly
            UiInteractableBus::Event(child, &UiInteractableInterface::SetIsHandlingEvents, parentState);
            UiInteractableBus::Event(child, &UiInteractableInterface::SetIsHandlingMultiTouchEvents, parentState);

            // Recurse into this child's children
            DoRecursiveSetInteractivityToChildren(child, parentState);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<UiHierarchyInteractivityToggleComponent, AZ::Component>()
            ->Version(1)
            ->Field("LocalInteraction", &UiHierarchyInteractivityToggleComponent::m_isInteractionLocallyEnabled)
            ;

        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
        {
            editContext
                ->Class<UiHierarchyInteractivityToggleComponent>(
                    "HierarchyInteractivityToggle", "A grouping handler that allows interaction and rendering for the entire hierarchy of children.")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->DataElement(
                    AZ::Edit::UIHandlers::Default,
                    &UiHierarchyInteractivityToggleComponent::m_isInteractionLocallyEnabled,
                    "Is Interactive",
                    "Whether this entity and children will be interactable.")
                    ;
        }
    }

    if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
    {
        behaviorContext->EBus<UiHierarchyInteractivityToggleBus>("UiHierarchyInteractivityToggleBus")
            ->Event("Set Interactive State", &UiHierarchyInteractivityToggleInterface::SetInteractivity)
            ->Event("Get Interactive State", &UiHierarchyInteractivityToggleInterface::GetInteractiveState)
            ;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC_CE("UiHierarchyInteractivityToggleComponentService"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
{
    incompatible.push_back(AZ_CRC_CE("UiHierarchyInteractivityToggleComponentService"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
{
    required.push_back(AZ_CRC_CE("UiElementService"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiHierarchyInteractivityToggleComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
{
}
