/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ComponentMode/ComponentModeSwitcher.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelection.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>

#pragma optimize("", off)
#pragma inline_depth(0)
namespace AzToolsFramework::ComponentModeFramework

{
    // ComponentData constructor to fill in component info
    ComponentData::ComponentData(AZ::EntityComponentIdPair pairId)
        : m_pairId(pairId)
    {
        AZ::ComponentApplicationBus::BroadcastResult(
            m_entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(pairId.GetEntityId()));
        AZ::ComponentId componentId = pairId.GetComponentId();

        m_component = m_entity->FindComponent(componentId);

        m_componentName = AzToolsFramework::GetFriendlyComponentName(m_component);

        AzToolsFramework::EditorRequestBus::BroadcastResult(
            m_iconPath,
            &AzToolsFramework::EditorRequestBus::Events::GetComponentEditorIcon,
            m_component->GetUnderlyingComponentType(),
            m_component);
    }

    ComponentModeSwitcher::ComponentModeSwitcher()
    {
        ViewportEditorModeNotificationsBus::Handler::BusConnect(GetEntityContextId());
        EntityCompositionNotificationBus::Handler::BusConnect();
        ToolsApplicationNotificationBus::Handler::BusConnect();


        // Create the switcher
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_switcherId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateSwitcher,
            ViewportUi::Alignment::TopLeft);

        // Initial Transform button
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_transformButtonId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateSwitcherButton,
            m_switcherId,
            "../../../../Assets/Editor/Icons/Components/Transform.svg",
            "Transform");

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
            m_switcherId,
            m_transformButtonId);

        // Handler
        auto handlerFunc = [this](ViewportUi::ButtonId buttonId)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                m_switcherId,
                buttonId);

            ActivateComponentMode(buttonId);
        };

        m_handler = AZ::Event<ViewportUi::ButtonId>::Handler(handlerFunc);
        // Registers the SwitcherEventHandler which is defined in ComponentModeSwitcher
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::RegisterSwitcherEventHandler,
            m_switcherId,
            m_handler);
    }

    ComponentModeSwitcher::~ComponentModeSwitcher()
    {
        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        EntityCompositionNotificationBus::Handler::BusDisconnect();
        ToolsApplicationNotificationBus::Handler::BusDisconnect();

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveSwitcher, m_switcherId);
    }

    void ComponentModeSwitcher::UpdateSwitcherOnEntitySelectionChange(
        [[maybe_unused]] const EntityIdList newlySelectedEntityIds, [[maybe_unused]] const EntityIdList newlyDeselectedEntityIds)
    {
        AZ::Entity* entity = nullptr;

        // Currently only handling one new entity selection at a time
        if (!newlySelectedEntityIds.empty() && newlySelectedEntityIds.size() == 1)
        {
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(newlySelectedEntityIds.front()));

            if (entity)
            {
                EntityIdList selectedEntityIds;
                ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequestBus::Events::GetSelectedEntities);
        
                if (selectedEntityIds.size() > 1)
                {
                    RemoveExclusiveComponents(*entity);
                    return;
                }

                for (const auto& entityComponent : entity->GetComponents())
                {
                    // Find if the component has a component mode using GetNumOfEventHandlers
                    const auto entityComponentIdPair = AZ::EntityComponentIdPair(entity->GetId(), entityComponent->GetId());
                    AddComponentButton(entityComponentIdPair);
                }
            }
        }
        else if (!newlyDeselectedEntityIds.empty())
        {
            AZStd::vector<AZ::EntityComponentIdPair> pairIdsToRemove;
            for (auto componentDataIt : m_addedComponents)
            {
                pairIdsToRemove.push_back(componentDataIt.m_pairId);
            }
            for (auto pairId : pairIdsToRemove)
            {
                RemoveComponentButton(pairId);
            }
            //v.erase(std::remove_if(v.begin(), v.end(), is_odd), v.end());
        }
    }

    void ComponentModeSwitcher::AddComponentButton(const AZ::EntityComponentIdPair pairId)
    {
        // Check if the component has a component mode.
        const auto numHandlers = AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequestBus::GetNumOfEventHandlers(pairId);
        if (numHandlers < 1)
        {
            return;
        }

        ComponentData newComponentData = ComponentData(pairId);

        // If the component has not already been added as a button, add the button
        if (auto componentDataIt = AZStd::find_if(
                m_addedComponents.begin(),
                m_addedComponents.end(),
                [newComponentData](const ComponentData& componentInfo)
                {
                    return componentInfo.m_component->RTTI_GetType() == newComponentData.m_component->RTTI_GetType();
                });
            componentDataIt == m_addedComponents.end())
        {
            AddSwitcherButton(newComponentData, newComponentData.m_componentName.c_str(), newComponentData.m_iconPath.c_str());
            m_addedComponents.push_back(newComponentData);
        }
    }

    void ComponentModeSwitcher::AddSwitcherButton(ComponentData& componentData, const char* componentName, const char* iconPath)
    {
        ViewportUi::ButtonId buttonId;
        ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateSwitcherButton,
            m_switcherId,
            iconPath,
            componentName);

        componentData.m_buttonId = buttonId;

        AZStd::string buttonTooltip = AZStd::string::format(" Enter %s component mode", componentData.m_componentName.c_str());

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherButtonTooltip,
            m_switcherId,
            buttonId,
            buttonTooltip);
    }

    void ComponentModeSwitcher::RemoveComponentButton(const AZ::EntityComponentIdPair pairId)
    {
        // find pairId in m_whatever, call bus to remove
        if (auto componentDataIt = AZStd::find_if(
                m_addedComponents.begin(),
                m_addedComponents.end(),
                [pairId](const ComponentData& componentInfo)
                {
                    return componentInfo.m_pairId == pairId;
                });
            componentDataIt != m_addedComponents.end())
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::RemoveSwitcherButton,
                m_switcherId,
                componentDataIt->m_buttonId);

            m_addedComponents.erase(componentDataIt);
        }
    }

    void ComponentModeSwitcher::ActivateComponentMode(const ViewportUi::ButtonId buttonId)
    {
        bool inComponentMode;
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::BroadcastResult(
            inComponentMode, &ComponentModeSystemRequests::InComponentMode);
        ComponentData* component = nullptr;

        // find component associated with button Id
        if (auto componentDataIt = AZStd::find_if(
                m_addedComponents.begin(),
                m_addedComponents.end(),
                [buttonId](const ComponentData& componentData)
                {
                    return componentData.m_buttonId == buttonId;
                });
            componentDataIt != m_addedComponents.end())
        {
            component = componentDataIt;
        }

        // if already in component mode, end current mode and switch active button to the selected component
        if (inComponentMode)
        {
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &ComponentModeSystemRequests::EndComponentMode);

            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                m_switcherId,
                buttonId);
        }

        // if the newly active button is not the transform button (no component mode associated), emter component mode 
        if (buttonId != m_transformButtonId)
        {
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &ComponentModeSystemRequests::AddSelectedComponentModesOfType, component->m_component->GetUnderlyingComponentType());
        }
    }

    void ComponentModeSwitcher::OnEditorModeDeactivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode)
    {
        if (mode == ViewportEditorMode::Component)
        {
            // always want to return to the default transform button upon exiting component mode
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                m_switcherId,
                m_transformButtonId);

            m_activeSwitcherComponent = nullptr;
        }
    }

    void ComponentModeSwitcher::OnEditorModeActivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode)
    {

        // if the user enters component mode not using the switcher, the switcher should still switch buttons
        if (mode == ViewportEditorMode::Component)
        {
            for (auto componentData : m_addedComponents)
            {
                bool isComponentActive;
                ComponentModeSystemRequestBus::BroadcastResult(
                    isComponentActive,
                    &ComponentModeSystemRequestBus::Events::AddedToComponentMode,
                    componentData.m_pairId,
                    componentData.m_component->GetUnderlyingComponentType());

                if (isComponentActive)
                {
                    ViewportUi::ViewportUiRequestBus::Event(
                        ViewportUi::DefaultViewportId,
                        &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                        m_switcherId,
                        componentData.m_buttonId);
                    m_activeSwitcherComponent = componentData.m_component;
                }
            }
        }
    }

    void ComponentModeSwitcher::RemoveExclusiveComponents(const AZ::Entity& entity)
    {
        // Get all component Ids associated with entity, identify which ones to remove and then remove them
        AZStd::set<AZ::TypeId> componentIds;
        for (const auto& entityComponent : entity.GetComponents())
        {
            componentIds.insert(entityComponent->RTTI_GetType());
        }

        AZStd::vector<AZ::EntityComponentIdPair> pairsToRemove;
        for (auto switcherComponent: m_addedComponents)
        {
            if (!componentIds.contains(switcherComponent.m_component->RTTI_GetType()))
            {
                pairsToRemove.push_back(switcherComponent.m_pairId);
            }
        }
        for (auto pairId : pairsToRemove)
        {
            RemoveComponentButton(pairId);
        }
    }

    void ComponentModeSwitcher::OnEntityComponentDisabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        auto pairId = AZ::EntityComponentIdPair(entityId, componentId);
        RemoveComponentButton(pairId);
    }

    void ComponentModeSwitcher::OnEntityComponentEnabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        auto pairId = AZ::EntityComponentIdPair(entityId, componentId);
        AddComponentButton(pairId);
    }

    void ComponentModeSwitcher::OnEntityComponentAdded(
        [[maybe_unused]] const AZ::EntityId& entity, [[maybe_unused]] const AZ::ComponentId& component)
    {
        // when a component is added to an entity set a staging member variable to the entityComponentIdPair
        m_componentModePair = AZ::EntityComponentIdPair(entity, component);
        m_addOrRemove = AddOrRemoveComponent::Add;
    }

    void ComponentModeSwitcher::OnEntityComponentRemoved(
        [[maybe_unused]] const AZ::EntityId& entityId, [[maybe_unused]] const AZ::ComponentId& componentId)
    {
        m_componentModePair = AZ::EntityComponentIdPair(entityId, componentId);
        m_addOrRemove = AddOrRemoveComponent::Remove;
    }

    // this is called twice when a component is added, once while the component is pending and
    // once when it has been added fully to the entity. This is handled in UpdateComponentButton
    void ComponentModeSwitcher::OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& entityIdList)
    {
        if (AZStd::find(entityIdList.begin(), entityIdList.end(), m_componentModePair.GetEntityId()) != entityIdList.end())
        {
            if (m_addOrRemove == AddOrRemoveComponent::Add)
            {
                AddComponentButton(m_componentModePair);
            }
            else
            {
                RemoveComponentButton(m_componentModePair);
            }
        }
    }

    void ComponentModeSwitcher::AfterEntitySelectionChanged(
        [[maybe_unused]] const EntityIdList& newlySelectedEntities, [[maybe_unused]] const EntityIdList& newlyDeselectedEntities)
    {
 
        // when the selected entity switches, switch back to the transform button
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
            m_switcherId,
            m_transformButtonId);

        // Send a list of selected and deselected entities to the switcher to deal with updating the switcher view
        UpdateSwitcherOnEntitySelectionChange(newlySelectedEntities, newlyDeselectedEntities);

    }

} // namespace AzToolsFramework::ComponentModeFramework
#pragma optimize("", on)
#pragma inline_depth()
