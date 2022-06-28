/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportUi/ViewportUiSwitcherManager.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelection.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>

namespace AzToolsFramework::ViewportUi

{


    void ViewportSwitcherManager::OnTick(float, AZ::ScriptTimePoint)
    {

        EntityIdList entityIds;
        AZ::Entity* entity = nullptr;
        // Get the selected entities so we can check what components they have
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            entityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
        // Currently only handling selections of one entity *********
        if (!entityIds.empty() && entityIds.size() == 1) {
           
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(entityIds.front()));
            
            /*if (entity) {
                AZ_Printf("viewportUiManager", "Entity name: %s", entity->GetName().c_str());
                 
                for (const auto& entityComponent : entity->GetComponents())
                {
                    const auto editorComponentDescriptor = azrtti_cast<Components::EditorComponentBase*>(entityComponent);

                    const auto componentTypeId = entityComponent->GetUnderlyingComponentType();
                    const auto entityComponentIdPair = AZ::EntityComponentIdPair(entity->GetId(), entityComponent->GetId());

                    const auto numHandlers = AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequestBus::GetNumOfEventHandlers(entityComponentIdPair);


                    AZ::ComponentDescriptor* componentDescriptor = nullptr;
                    AZ::ComponentDescriptorBus::EventResult(
                        componentDescriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                    if (componentDescriptor && editorComponentDescriptor && numHandlers >= 1) {

                        
                        AddSwitcherButton(entityComponentIdPair, componentDescriptor->GetName());
                        const auto switcherIt = m_buttons.find(entityComponentIdPair);
                        if (switcherIt != m_buttons.end())
                        {
                            AZ_Printf("ViewportUiSwitcherTest", "buttonId %s", switcherIt->second.c_str());
                            
                        }
                    }

                }
            }*/
            if (entity) {
                auto map = GetComponentModeIds(entity);
                for (auto component : map) {
                    AddSwitcherButton(component.first, "World");
                    m_buttons.insert({ component.first, "World"});
                }
            }

        }
        
    }
    
    //got the entity, then check what components have component mode, then check if theyre added already, then add
    //but 
    int ViewportSwitcherManager::GetTickOrder()
    {
        return AZ::TICK_UI;
    }

    AZStd::unordered_map<AZ::EntityComponentIdPair, AZ::Component*> ViewportSwitcherManager::GetComponentModeIds(AZ::Entity* entity)
    {
        AZStd::unordered_map<AZ::EntityComponentIdPair, AZ::Component*> newMap;

        for (const auto& entityComponent : entity->GetComponents())
        {

            // Find if the component has a component mode using GetNumOfEventHandlers
            const auto entityComponentIdPair = AZ::EntityComponentIdPair(entity->GetId(), entityComponent->GetId());
            const auto numHandlers = AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequestBus::GetNumOfEventHandlers(entityComponentIdPair);

            //
            const auto editorComponentDescriptor = azrtti_cast<Components::EditorComponentBase*>(entityComponent);
            const auto componentTypeId = entityComponent->GetUnderlyingComponentType();

            AZ::ComponentDescriptor* componentDescriptor = nullptr;
            AZ::ComponentDescriptorBus::EventResult(
                componentDescriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);

            if (componentDescriptor && editorComponentDescriptor && numHandlers >= 1) {
                if (auto button = m_buttons.find(entityComponentIdPair); button == m_buttons.end())
                {
                    newMap.insert({ entityComponentIdPair, entityComponent });
                    //m_buttons.insert({ entityComponentIdPair, componentDescriptor->GetName() });
                }
                
            }

        }

        return newMap;
    }

    void ViewportSwitcherManager::AddSwitcherButton(AZ::EntityComponentIdPair pairId, const char* buttonName)
    {
        AZ_Printf("ViewportUiSwitcherTest", "checking for new buttons")
        if (auto button = m_buttons.find(pairId); button == m_buttons.end())
        {
            AzToolsFramework::RegisterSwitcherButton(m_switcherCluster->m_switcherId, buttonName, "World");
            /*m_buttons.insert({ pairId, buttonName});*/

        }
    }
} // namespace AzToolsFramework::ViewportUi
