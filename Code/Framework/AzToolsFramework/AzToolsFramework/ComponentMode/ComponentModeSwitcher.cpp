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

        const auto componentTypeId = m_component->GetUnderlyingComponentType();
        //AZ_Printf("Constructor", "underyling component %s type: %zu", m_componentName.c_str(), GetComponentTypeId(m_component));
        AZStd::string iconStr;
        AzToolsFramework::EditorRequestBus::BroadcastResult(
            m_iconStr, &AzToolsFramework::EditorRequestBus::Events::GetComponentEditorIcon, componentTypeId, m_component);
    }

    ComponentModeSwitcher::ComponentModeSwitcher(Switcher* switcher)
        : m_switcher(switcher)
    {
        AzFramework::EntityContextId editorEntityContextId = GetEntityContextId();
        ViewportEditorModeNotificationsBus::Handler::BusConnect(editorEntityContextId);
        EntityCompositionNotificationBus::Handler::BusConnect();
        
        auto handlerFunc = [this](ViewportUi::ButtonId buttonId)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                m_switcher->m_switcherId,
                buttonId);

            ActivateComponentMode(buttonId);
        };
        m_handler = AZ::Event<ViewportUi::ButtonId>::Handler(handlerFunc);
    }

    ComponentModeSwitcher::~ComponentModeSwitcher()
    {
        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        EntityCompositionNotificationBus::Handler::BusDisconnect();
    }

    void ComponentModeSwitcher::Update(EntityIdList newlySelectedEntityIds, [[maybe_unused]] EntityIdList newlyDeselectedEntityIds)
    {
        AZ::Entity* entity = nullptr;

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
                    // if multiple components, we add nothing
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
            for (auto componentDataIt : m_addedComponents)
            {
                RemoveComponentButton(componentDataIt.m_pairId);
            }
        }
    }

    void ComponentModeSwitcher::AddSwitcherButton(ComponentData& component, const char* componentName, const char* iconStr)
    {
        ViewportUi::ButtonId buttonId;
        ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateSwitcherButton,
            m_switcher->m_switcherId,
            iconStr,
            componentName);

        component.m_buttonId = buttonId;
    }

    void ComponentModeSwitcher::AddComponentButton(AZ::EntityComponentIdPair pairId)
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
                    return componentInfo.m_componentName == newComponentData.m_componentName;
                });
            componentDataIt == m_addedComponents.end())
        {
            if (!m_addedComponentNames.contains(newComponentData.m_componentName))
            {
                AddSwitcherButton(newComponentData, newComponentData.m_componentName.c_str(), newComponentData.m_iconStr.c_str());
                m_addedComponents.push_back(newComponentData);
            }
            
            
        }
    }

    void ComponentModeSwitcher::RemoveComponentButton(AZ::EntityComponentIdPair pairId)
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
                m_switcher->m_switcherId,
                componentDataIt->m_buttonId);

            m_addedComponents.erase(componentDataIt);
        }
    }

    void ComponentModeSwitcher::ActivateComponentMode(ViewportUi::ButtonId buttonId)
    {
        bool inComponentMode;
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::BroadcastResult(
            inComponentMode, &ComponentModeSystemRequests::InComponentMode);
        ComponentData* component = nullptr;

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

        // if already in component mode, end current mode and switch active button to the selected component mode
        if (inComponentMode)
        {
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &ComponentModeSystemRequests::EndComponentMode);

            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                m_switcher->m_switcherId,
                buttonId);
        }

        if (buttonId != m_switcher->m_transformButtonId)
        {
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &ComponentModeSystemRequests::AddSelectedComponentModesOfType, component->m_component->GetUnderlyingComponentType());
        }
    }

    void ComponentModeSwitcher::OnComponentModeExit()
    {
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
            m_switcher->m_switcherId,
            m_switcher->m_transformButtonId);
    }

    void ComponentModeSwitcher::OnEditorModeDeactivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode)
    {
        if (mode == ViewportEditorMode::Component)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                m_switcher->m_switcherId,
                m_switcher->m_transformButtonId);
        }
    }

    void ComponentModeSwitcher::OnEditorModeActivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode)
    {
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
                        m_switcher->m_switcherId,
                        componentData.m_buttonId);
                }
            }
        }
    }

    void ComponentModeSwitcher::RemoveExclusiveComponents(const AZ::Entity& entity)
    {
        AZStd::set<AZStd::string> componentNames;
        for (const auto& entityComponent : entity.GetComponents())
        {
            componentNames.insert(AzToolsFramework::GetFriendlyComponentName(entityComponent));
            //for m_addecomponents - check if name is there
        }

        AZStd::vector<AZ::EntityComponentIdPair> pairsToRemove;
        for (auto switcherComponent: m_addedComponents)
        {
            if (!componentNames.contains(switcherComponent.m_componentName))
            {
                pairsToRemove.push_back(switcherComponent.m_pairId);
                //AZ_Printf("RemoveExclusiveComponents", "removing component: %s", switcherComponent.m_componentName.c_str());
                //RemoveComponentButton(switcherComponent.m_pairId);

            }
        }

        for (auto pairId : pairsToRemove)
        {
            RemoveComponentButton(pairId);
        } // why doesn't it work like in ComponentModeSwitcher::Update
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
        m_AddRemove = true; // find a better way to do this
    }

    void ComponentModeSwitcher::OnEntityComponentRemoved(
        [[maybe_unused]] const AZ::EntityId& entity, [[maybe_unused]] const AZ::ComponentId& component)
    {
        m_componentModePair = AZ::EntityComponentIdPair(entity, component);
        m_AddRemove = false;
    }

    // this is called twice when a component is added, once while the component is pending and
    // once when it has been added fully to the entity. This is handled in UpdateComponentButton
    void ComponentModeSwitcher::OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& entityIdList)
    {
        if (AZStd::find(entityIdList.begin(), entityIdList.end(), m_componentModePair.GetEntityId()) != entityIdList.end())
        {
            if (m_AddRemove)
            {
                AddComponentButton(m_componentModePair);
            }
            else
            {
                RemoveComponentButton(m_componentModePair);
            }
        }
    }

} // namespace AzToolsFramework::ComponentModeFramework
#pragma optimize("", on)
#pragma inline_depth()
