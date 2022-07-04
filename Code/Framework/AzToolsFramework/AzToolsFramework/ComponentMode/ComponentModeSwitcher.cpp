/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentMode/ComponentModeSwitcher.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelection.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>

#pragma optimize("", off)
#pragma inline_depth(0)
namespace AzToolsFramework::ComponentModeFramework

{
    ComponentInfo::ComponentInfo(AZ::EntityComponentIdPair pairId)
        : m_pairId(pairId)
    {
        AZ::ComponentApplicationBus::BroadcastResult(
            m_entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(pairId.GetEntityId()));

        AZ::ComponentId componentId = pairId.GetComponentId();
        m_component = m_entity->FindComponent(componentId);
        const auto componentTypeId = m_component->GetUnderlyingComponentType();

        AZ::ComponentDescriptorBus::EventResult(m_componentDescriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
    }

    bool ComponentModeSwitcher::Update(EntityIdList selectedEntityIds, [[maybe_unused]] EntityIdList deselectedEntityIds)
    {
        AZ::Entity* entity = nullptr;

        if (!selectedEntityIds.empty() && selectedEntityIds.size() == 1)
        {
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(selectedEntityIds.front()));

            if (entity)
            {
                auto map = GetComponentModeIds(entity);
                for (auto component : map)
                {
                    auto pairId = component.first;
                    AddComponentButton(pairId);
                }
            }
        }
        else if (!deselectedEntityIds.empty() && deselectedEntityIds.size() == 1)
        {
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(deselectedEntityIds.front()));

            if (entity)
            {
                /*auto map = GetComponentModeIds(entity);
                for (auto component : map)
                {
                    auto pairId = component.first;
                    RemoveComponentButton(pairId);
                }*/
                for (auto componentInfoIt : m_addedComponents)
                {
                    RemoveComponentButton(componentInfoIt.m_pairId);
                }
            }
        }
        return true;
    }

    void ComponentModeSwitcher::AddComponentButton(AZ::EntityComponentIdPair pairId)
    {
        const auto numHandlers = AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequestBus::GetNumOfEventHandlers(pairId);
        if (numHandlers < 1)
        {
            return;
        }
        // make a struct which is output of eg queryComponent which returns component, component name if it has a button, then add it to
        // m_addedComponents?
        AZ_Printf("AddComponentButton", "adding button %zu", pairId.GetComponentId());
        ComponentInfo newComponentInfo = ComponentInfo(pairId);

        //AZ_Printf("logger", "the name is: %s", newComponentInfo.m_componentDescriptor->GetName());
        auto componentName = newComponentInfo.m_componentDescriptor->GetName();
        // componentDescriptor->GetName(); figure it out later
        // If the component has not already been added as a button
        if (auto componentInfoIt = AZStd::find_if(
                m_addedComponents.begin(),
                m_addedComponents.end(),
                [pairId](const ComponentInfo& componentInfo)
                {
                    return componentInfo.m_pairId == pairId;
                });
            componentInfoIt == m_addedComponents.end())
        {
            AddSwitcherButton(&newComponentInfo, componentName, "World");
            m_addedComponents.push_back(newComponentInfo);
        }
    }

    void ComponentModeSwitcher::RemoveComponentButton(AZ::EntityComponentIdPair pairId)
    {
        // find pairId in m_whatever, call bus to remove
        if (auto componentInfoIt = AZStd::find_if(
                m_addedComponents.begin(),
                m_addedComponents.end(),
                [pairId](const ComponentInfo& componentInfo)
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
            // componentInfoIt->
            // m_addedComponents.push_back(newComponentInfo);
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

    AZStd::unordered_map<AZ::EntityComponentIdPair, AZ::Component*> ComponentModeSwitcher::GetComponentModeIds(AZ::Entity* entity)
    {
        AZStd::unordered_map<AZ::EntityComponentIdPair, AZ::Component*> newMap;

        for (const auto& entityComponent : entity->GetComponents())
        {
            // Find if the component has a component mode using GetNumOfEventHandlers
            const auto entityComponentIdPair = AZ::EntityComponentIdPair(entity->GetId(), entityComponent->GetId());
            const auto numHandlers =
                AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequestBus::GetNumOfEventHandlers(entityComponentIdPair);
            AZ_Printf("GetComponentModeIds", "checking component %zu", entityComponentIdPair.GetComponentId());
            if (numHandlers >= 1)
            {
                const auto editorComponentDescriptor = azrtti_cast<Components::EditorComponentBase*>(entityComponent);
                const auto componentTypeId = entityComponent->GetUnderlyingComponentType();

                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(
                    componentDescriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);

                if (componentDescriptor && editorComponentDescriptor)
                {
                    newMap.insert({ entityComponentIdPair, entityComponent });
                }
            }
        }

        return newMap;
    }

    void ComponentModeSwitcher::AddSwitcherButton(ComponentInfo* component, const char* buttonName, const char* iconName)
    {
        ViewportUi::ButtonId buttonId;
        ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateSwitcherButton,
            m_switcher->m_switcherId,
            AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName),
            buttonName);
        component->m_buttonId = buttonId;
    }
} // namespace AzToolsFramework::ComponentModeFramework
#pragma optimize("", on)
#pragma inline_depth()
