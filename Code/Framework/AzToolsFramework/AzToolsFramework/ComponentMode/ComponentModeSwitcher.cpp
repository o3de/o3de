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
        AZStd::string iconStr;
        AzToolsFramework::EditorRequestBus::BroadcastResult(
            m_iconStr, &AzToolsFramework::EditorRequestBus::Events::GetComponentEditorIcon, componentTypeId, m_component);
    }

    ComponentModeSwitcher::ComponentModeSwitcher(Switcher* switcher)
        : m_switcher(switcher)
    {
        ComponentModeFramework::EditorComponentModeNotificationBus2::Handler::BusConnect();

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
        ComponentModeFramework::EditorComponentModeNotificationBus2::Handler::BusDisconnect();
    }

    void ComponentModeSwitcher::Update(EntityIdList selectedEntityIds, [[maybe_unused]] EntityIdList deselectedEntityIds)
    {
        AZ::Entity* entity = nullptr;

        if (!selectedEntityIds.empty() && selectedEntityIds.size() == 1)
        {
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(selectedEntityIds.front()));

            if (entity)
            {
                for (const auto& entityComponent : entity->GetComponents())
                {
                    // Find if the component has a component mode using GetNumOfEventHandlers
                    const auto entityComponentIdPair = AZ::EntityComponentIdPair(entity->GetId(), entityComponent->GetId());
                    const auto numHandlers =
                        AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequestBus::GetNumOfEventHandlers(
                            entityComponentIdPair);

                    if (numHandlers >= 1)
                    {
                        AddComponentButton(entityComponentIdPair);
                    }
                }
                /*const auto editorComponentDescriptor = azrtti_cast<Components::EditorComponentBase*>(entityComponent);
                const auto componentTypeId = entityComponent->GetUnderlyingComponentType();

                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(
                    componentDescriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);

                if (componentDescriptor && editorComponentDescriptor)
                {*/
            }
        }
        else if (!deselectedEntityIds.empty() && deselectedEntityIds.size() == 1)
        {
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(deselectedEntityIds.front()));

            if (entity)
            {
                for (auto componentDataIt : m_addedComponents)
                {
                    RemoveComponentButton(componentDataIt.m_pairId);
                }
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
        if (auto componentInfoIt = AZStd::find_if(
                m_addedComponents.begin(),
                m_addedComponents.end(),
                [pairId](const ComponentData& componentInfo)
                {
                    return componentInfo.m_pairId == pairId;
                });
            componentInfoIt == m_addedComponents.end())
        {
            AddSwitcherButton(newComponentData, newComponentData.m_componentName.c_str(), newComponentData.m_iconStr.c_str());
            m_addedComponents.push_back(newComponentData);
        }
    }

    void ComponentModeSwitcher::RemoveComponentButton(AZ::EntityComponentIdPair pairId)
    {
        // find pairId in m_whatever, call bus to remove
        if (auto componentInfoIt = AZStd::find_if(
                m_addedComponents.begin(),
                m_addedComponents.end(),
                [pairId](const ComponentData& componentInfo)
                {
                    return componentInfo.m_pairId == pairId;
                });
            componentInfoIt != m_addedComponents.end())
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::RemoveSwitcherButton,
                m_switcher->m_switcherId,
                componentInfoIt->m_buttonId);

            m_addedComponents.erase(componentInfoIt);
        }
    }

    void ComponentModeSwitcher::UpdateComponentButton(AZ::EntityComponentIdPair pairId, bool addRemove)
    {
        if (addRemove)
        {
            AddComponentButton(pairId);
        }
        else
        {
            RemoveComponentButton(pairId);
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
} // namespace AzToolsFramework::ComponentModeFramework
#pragma optimize("", on)
#pragma inline_depth()
