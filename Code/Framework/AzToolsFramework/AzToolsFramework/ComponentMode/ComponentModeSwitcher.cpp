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
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace AzToolsFramework::ComponentModeFramework
{
    // ComponentData constructor to fill in component info
    ComponentData::ComponentData(AZ::EntityComponentIdPair pairId)
        : m_pairId(pairId)
    {
        AZ::ComponentApplicationBus::BroadcastResult(
            m_entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(pairId.GetEntityId()));

        m_component = m_entity->FindComponent(pairId.GetComponentId());
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

        // create the switcher
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_switcherId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateSwitcher,
            ViewportUi::Alignment::TopLeft);

        // initial transform button
        AzToolsFramework::Components::TransformComponent transformComponent;
        AZStd::string transformIconPath;

        AzToolsFramework::EditorRequestBus::BroadcastResult(
            transformIconPath,
            &AzToolsFramework::EditorRequestBus::Events::GetComponentTypeEditorIcon,
            transformComponent.GetUnderlyingComponentType());

        ViewportUi::ViewportUiRequestBus::EventResult(
            m_transformButtonId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateSwitcherButton,
            m_switcherId,
            transformIconPath.c_str(),
            "Transform");

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
            m_switcherId,
            m_transformButtonId);

        // create handler for switcher buttons to activate component mode on click
        auto handlerFunc = [this](ViewportUi::ButtonId buttonId)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                m_switcherId,
                buttonId);

            ActivateComponentMode(buttonId);
        };

        // register the SwitcherEventHandler which is defined in ComponentModeSwitcher
        m_handler = AZ::Event<ViewportUi::ButtonId>::Handler(handlerFunc);
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::RegisterSwitcherEventHandler,
            m_switcherId,
            m_handler);
    }

    ComponentModeSwitcher::~ComponentModeSwitcher()
    {
        ToolsApplicationNotificationBus::Handler::BusDisconnect();
        EntityCompositionNotificationBus::Handler::BusDisconnect();
        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveSwitcher, m_switcherId);
    }

    void ComponentModeSwitcher::UpdateSwitcherOnEntitySelectionChange(
        const EntityIdList& newlySelectedEntityIds, const EntityIdList& newlyDeselectedEntityIds)
    {

        auto* toolsApplicationRequests = AzToolsFramework::ToolsApplicationRequestBus::FindFirstHandler();
        const auto& selectedEntityIds = toolsApplicationRequests->GetSelectedEntities();

        
        if (!newlySelectedEntityIds.empty())
        {
            
            for (auto entityId : newlySelectedEntityIds)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(
                    entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(entityId));

                if (entity)
                {
                    /* if two or more entities are selected, ensure the only components
                    that remain on the switcher are common to all entities*/
                    if (selectedEntityIds.size() > 1 && m_addedComponents.size() != 0)
                    {
                        RemoveNonCommonComponents(*entity);
                        // if components have been removed from the switcher and there is nothing left on the switcher
                        // then there are no common components in the selection
                        if (m_addedComponents.size() == 0)
                        {
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }

                    for (const auto& entityComponent : entity->GetComponents())
                    {
                        const auto entityComponentIdPair = AZ::EntityComponentIdPair(entity->GetId(), entityComponent->GetId());
                        AddComponentButton(entityComponentIdPair);
                    }
                }
            }
        }
        else if (!newlyDeselectedEntityIds.empty())
        {
            //CLear the switcher then add the components back if entities are still selected
            ClearSwitcher();

            if (selectedEntityIds.size() >= 1)
            {
                UpdateSwitcherOnEntitySelectionChange( selectedEntityIds, EntityIdList{});
            }
        }
    }

    void ComponentModeSwitcher::ClearSwitcher()
    {
        for (AZStd::vector<ComponentData>::reverse_iterator it = m_addedComponents.rbegin(); it != m_addedComponents.rend(); ++it)
        {
            RemoveComponentButton(it->m_pairId);
        }
    }

    void ComponentModeSwitcher::AddComponentButton(const AZ::EntityComponentIdPair pairId)
    {
        // check if the component has a component mode
        const auto handlerCount = AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequestBus::GetNumOfEventHandlers(pairId);
        if (handlerCount < 1)
        {
            return;
        }

        ComponentData newComponentData = ComponentData(pairId);

        // if the component has not already been added as a button, add the button
        if (auto componentDataIt = AZStd::find_if(
                m_addedComponents.begin(),
                m_addedComponents.end(),
                [newComponentData](const ComponentData& componentInfo)
                {
                    return componentInfo.m_component->RTTI_GetType() == newComponentData.m_component->RTTI_GetType();
                });
            componentDataIt == m_addedComponents.end())
        {
            AddSwitcherButton(newComponentData);
            m_addedComponents.push_back(newComponentData);
        }
    }

    void ComponentModeSwitcher::AddSwitcherButton(ComponentData& componentData)
    {
        ViewportUi::ButtonId buttonId;
        ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateSwitcherButton,
            m_switcherId,
            componentData.m_iconPath.c_str(),
            componentData.m_componentName.c_str());

        componentData.m_buttonId = buttonId;

        AZStd::string buttonTooltip = AZStd::string::format("Enter %s Edit mode", componentData.m_componentName.c_str());

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherButtonTooltip,
            m_switcherId,
            buttonId,
            buttonTooltip);
    }

    void ComponentModeSwitcher::RemoveComponentButton(const AZ::EntityComponentIdPair pairId)
    {
        // find pairId in m_addedComponents, call ViewportUiRequestBus to remove
        if (auto componentDataIt = AZStd::find_if(
                m_addedComponents.begin(),
                m_addedComponents.end(),
                [pairId](const ComponentData& componentData)
                {
                    return componentData.m_pairId == pairId;
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

        bool inComponentMode;
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::BroadcastResult(
            inComponentMode, &ComponentModeSystemRequests::InComponentMode);

        // if already in component mode, end current mode and switch active button to the selected component
        if (inComponentMode)
        {
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &ComponentModeSystemRequests::EndComponentMode);

            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton, m_switcherId, buttonId);
        }

        // if the newly active button is not the transform button (no component mode associated), emter component mode
        if (buttonId != m_transformButtonId)
        {
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &ComponentModeSystemRequests::AddSelectedComponentModesOfType, component->m_component->GetUnderlyingComponentType());
        }
    }

    void ComponentModeSwitcher::OnEditorModeDeactivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        if (mode == ViewportEditorMode::Component)
        {
            // always want to return to the default transform button upon exiting component mode
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                m_switcherId,
                m_transformButtonId);
        }
    }

    void ComponentModeSwitcher::OnEditorModeActivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        // if the user enters component mode not using the switcher, the switcher should still switch buttons
        if (mode == ViewportEditorMode::Component)
        {
             if (auto componentDataIt = AZStd::find_if(
                    m_addedComponents.begin(),
                    m_addedComponents.end(),
                    [](const ComponentData& componentData)
                    {
                        bool isComponentActive;
                        ComponentModeSystemRequestBus::BroadcastResult(
                            isComponentActive,
                            &ComponentModeSystemRequestBus::Events::AddedToComponentMode,
                            componentData.m_pairId,
                            componentData.m_component->GetUnderlyingComponentType());
                        return isComponentActive;
                    });
                componentDataIt != m_addedComponents.end())
            {
                 ViewportUi::ViewportUiRequestBus::Event(
                     ViewportUi::DefaultViewportId,
                     &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                     m_switcherId,
                     componentDataIt->m_buttonId);
                 m_activeSwitcherComponent = componentDataIt->m_component;
            }
        }
    }

    void ComponentModeSwitcher::RemoveNonCommonComponents(const AZ::Entity& entity)
    {
        // get all component Ids associated with entity, identify which ones to remove and then remove them
        AZStd::set<AZ::TypeId> componentIds;
        for (const auto& entityComponent : entity.GetComponents())
        {
            componentIds.insert(entityComponent->RTTI_GetType());
        }

        for (AZStd::vector<ComponentData>::reverse_iterator it = m_addedComponents.rbegin(); it != m_addedComponents.rend(); ++it)
        {
            if(!componentIds.contains(it->m_component->RTTI_GetType()))
            {
                RemoveComponentButton(it->m_pairId);
            }
        }
    }

    void ComponentModeSwitcher::OnEntityComponentDisabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        RemoveComponentButton(AZ::EntityComponentIdPair(entityId, componentId));
    }

    void ComponentModeSwitcher::OnEntityComponentEnabled(const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
    {
        AddComponentButton(AZ::EntityComponentIdPair(entityId, componentId));
    }

    void ComponentModeSwitcher::OnEntityComponentAdded(
        const AZ::EntityId& entity, const AZ::ComponentId& component)
    {
        // when a component is added to an entity set a staging member variable to the entityComponentIdPair
        m_componentModePair = AZ::EntityComponentIdPair(entity, component);
        m_addOrRemove = AddOrRemoveComponent::Add;
    }

    void ComponentModeSwitcher::OnEntityComponentRemoved(
        const AZ::EntityId& entityId, const AZ::ComponentId& componentId)
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
        const EntityIdList& newlySelectedEntities, const EntityIdList& newlyDeselectedEntities)
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
