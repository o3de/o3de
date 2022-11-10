/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ComponentMode/ComponentModeSwitcher.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelection.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>

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
        AzFramework::ViewportImGuiNotificationBus::Handler::BusConnect();
        AzFramework::ComponentModeDelegateNotificationBus::Handler::BusConnect();

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
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton, m_switcherId, buttonId);

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
        AzFramework::ComponentModeDelegateNotificationBus::Handler::BusDisconnect();
        AzFramework::ViewportImGuiNotificationBus::Handler::BusDisconnect();
        ToolsApplicationNotificationBus::Handler::BusDisconnect();
        EntityCompositionNotificationBus::Handler::BusDisconnect();
        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveSwitcher, m_switcherId);
    }

    static const EntityIdList& GetSelectedEntities()
    {
        auto* toolsApplicationRequests = AzToolsFramework::ToolsApplicationRequestBus::FindFirstHandler();
        return toolsApplicationRequests->GetSelectedEntities();
    }

    void ComponentModeSwitcher::UpdateSwitcher(
        const EntityIdList& newlySelectedEntityIds, const EntityIdList& newlyDeselectedEntityIds)
    {
        const auto& selectedEntityIds = GetSelectedEntities();

        if (!newlyDeselectedEntityIds.empty())
        {
            // clear the switcher then add the components back if entities are still selected
            ClearSwitcher();

            if (!selectedEntityIds.empty())
            {
                UpdateSwitcher(selectedEntityIds, EntityIdList{});
            }
        }

        if (!newlySelectedEntityIds.empty())
        {
            for (auto entityId : newlySelectedEntityIds)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(
                    entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(entityId));

                if (entity)
                {
                    // if two or more entities are selected, ensure the only components
                    // that remain on the switcher are common to all entities
                    if (selectedEntityIds.size() > 1 && !m_addedComponents.empty())
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
                        if (AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequestBus::HasHandlers(entityComponentIdPair))
                        {
                            AddComponentButton(entityComponentIdPair);
                        }
                    }
                }
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
        ComponentData newComponentData = ComponentData(pairId);

        // if the component has not already been added as a button, add the button
        if (auto componentDataIt = AZStd::ranges::find_if(
                m_addedComponents,
                [newComponentData](const ComponentData& componentInfo)
                {
                    return componentInfo.m_component->RTTI_GetType() == newComponentData.m_component->RTTI_GetType();
                });
            componentDataIt == m_addedComponents.end())
        {
            AddSwitcherButton(newComponentData);
        }
    }

    void ComponentModeSwitcher::AddSwitcherButton(ComponentData& componentData)
    {
        ViewportUi::ViewportUiRequestBus::EventResult(
            componentData.m_buttonId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateSwitcherButton,
            m_switcherId,
            componentData.m_iconPath.c_str(),
            componentData.m_componentName.c_str());

        AZStd::string buttonTooltip = AZStd::string::format("Enter %s Edit mode", componentData.m_componentName.c_str());

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherButtonTooltip,
            m_switcherId,
            componentData.m_buttonId,
            buttonTooltip);

        m_addedComponents.push_back(componentData);
    }

    void ComponentModeSwitcher::RemoveComponentButton(const AZ::EntityComponentIdPair pairId)
    {
        // find pairId in m_addedComponents, call ViewportUiRequestBus to remove
        if (auto componentDataIt = AZStd::ranges::find_if(
                m_addedComponents,
                [pairId](const ComponentData& predComponentData)
                {
                    return pairId == predComponentData.m_pairId;
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
        ComponentData* componentData = nullptr;

        // find component associated with button Id
        if (auto componentDataIt = AZStd::ranges::find_if(
                m_addedComponents,
                [buttonId](const ComponentData& componentData)
                {
                    return componentData.m_buttonId == buttonId;
                });
            componentDataIt != m_addedComponents.end())
        {
            componentData = componentDataIt;
        }

        // the transform button does not have an associated component mode,
        // if the user clicks the transform button they must already be in component mode
        // so when they click it, leave component mode
        if (buttonId == m_transformButtonId)
        {
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &ComponentModeSystemRequests::EndComponentMode);

            return;
        }

        bool inComponentMode = false;
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::BroadcastResult(
            inComponentMode, &ComponentModeSystemRequests::InComponentMode);

        // if already in component mode, end current mode and switch active button to the selected component
        if (inComponentMode)
        {
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &ComponentModeSystemRequests::ChangeComponentMode, componentData->m_component->GetUnderlyingComponentType());

            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton, m_switcherId, buttonId);
        }
        else
        {
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &ComponentModeSystemRequests::AddSelectedComponentModesOfType, componentData->m_component->GetUnderlyingComponentType());
        }
    }

    void ComponentModeSwitcher::OnEditorModeDeactivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        if (mode == ViewportEditorMode::Component)
        {
            // when component mode ends, change the active button to transform
            m_activeSwitcherComponent = nullptr;

            bool inComponentMode = false;
            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::BroadcastResult(
                inComponentMode, &ComponentModeSystemRequests::InComponentMode);

            if (!inComponentMode)
            {
                ViewportUi::ViewportUiRequestBus::Event(
                    ViewportUi::DefaultViewportId,
                    &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherActiveButton,
                    m_switcherId,
                    m_transformButtonId);
            }
        }
    }

    void ComponentModeSwitcher::OnEditorModeActivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        // if the user enters component mode not using the switcher, the switcher should still switch buttons
        if (mode == ViewportEditorMode::Component)
        {
            if (auto componentDataIt = AZStd::ranges::find_if(
                    m_addedComponents,
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
            if (!componentIds.contains(it->m_component->RTTI_GetType()))
            {
                RemoveComponentButton(it->m_pairId);
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

        // send a list of selected and deselected entities to the switcher to deal with updating the switcher view
        UpdateSwitcher(newlySelectedEntities, newlyDeselectedEntities);
    }

    void ComponentModeSwitcher::OnImGuiDropDownShown()
    {
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherVisible, m_switcherId, false);
        m_hiddenByImGui = true;
    }

    void ComponentModeSwitcher::OnImGuiDropDownHidden()
    {
        if (m_hiddenByImGui)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherVisible, m_switcherId, true);
            // reset hiddenByImGui variable
            m_hiddenByImGui = false;
        }
    }

    void ComponentModeSwitcher::OnImGuiDeactivated()
    {
        // when the switcher is hidden by the ImGui, if ImGui is deactivated the drop down is still present and the switcher will
        // remain hidden, this makes the switcher visible while ImGui is deactivated
        if (m_hiddenByImGui)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherVisible, m_switcherId, true);
        }
    }

    void ComponentModeSwitcher::OnImGuiActivated()
    {
        // when the ImGui menu bar gets reactivated, hide the  switcher again
        if (m_hiddenByImGui)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherVisible, m_switcherId, false);
        }
    }

    void ComponentModeSwitcher::OnComponentModeDelegateConnect(const AZ::EntityComponentIdPair& pairId)
    {
        const auto& selectedEntityIds = GetSelectedEntities();
        auto entityId = pairId.GetEntityId();

        if (AZStd::ranges::find(selectedEntityIds, entityId) != selectedEntityIds.end())
        {
            AddComponentButton(pairId);
        } 
    }

    void ComponentModeSwitcher::OnComponentModeDelegateDisconnect(const AZ::EntityComponentIdPair& pairId)
    {
        const auto& selectedEntityIds = GetSelectedEntities();
        auto entityId = pairId.GetEntityId();

        if (AZStd::ranges::find(selectedEntityIds, entityId) != selectedEntityIds.end())
        {
            RemoveComponentButton(pairId);
        }
    }
} // namespace AzToolsFramework::ComponentModeFramework
