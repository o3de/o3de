/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Component/EditorLevelComponentAPIComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>


namespace AzToolsFramework
{
    namespace Components
    {
        void EditorLevelComponentAPIComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorLevelComponentAPIComponent, AZ::Component>();
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<EditorLevelComponentAPIBus>("EditorLevelComponentAPIBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Components")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Event("AddComponentsOfType", &EditorLevelComponentAPIRequests::AddComponentsOfType)
                    ->Event("HasComponentOfType", &EditorLevelComponentAPIRequests::HasComponentOfType)
                    ->Event("CountComponentsOfType", &EditorLevelComponentAPIRequests::CountComponentsOfType)
                    ->Event("GetComponentOfType", &EditorLevelComponentAPIRequests::GetComponentOfType)
                    ->Event("GetComponentsOfType", &EditorLevelComponentAPIRequests::GetComponentsOfType)
                    ;
            }
        }

        void EditorLevelComponentAPIComponent::Activate()
        {
            EditorLevelComponentAPIBus::Handler::BusConnect();
        }

        void EditorLevelComponentAPIComponent::Deactivate()
        {
            EditorLevelComponentAPIBus::Handler::BusDisconnect();
        }

        // Returns an Outcome object with the Component Id if successful, and the cause of the failure otherwise
        EditorComponentAPIRequests::AddComponentsOutcome EditorLevelComponentAPIComponent::AddComponentsOfType(const AZ::ComponentTypeList& componentTypeIds)
        {
            //Always get the EntityId of the Level.
            AZ::EntityId levelEntityId;
            ToolsApplicationRequestBus::BroadcastResult(levelEntityId, &ToolsApplicationRequests::GetCurrentLevelEntityId);
            if (!levelEntityId.IsValid())
            {
                return EditorComponentAPIRequests::AddComponentsOutcome(AZStd::unexpect, AZStd::string("Invalid Level EntityId. Most likely there's no level loaded in the Editor."));
            }

            EditorComponentAPIRequests::AddComponentsOutcome outcome;
            EditorComponentAPIBus::BroadcastResult(outcome, &EditorComponentAPIRequests::AddComponentsOfType, levelEntityId, componentTypeIds);
            return outcome;
        }

        bool EditorLevelComponentAPIComponent::HasComponentOfType(AZ::Uuid componentTypeId)
        {
            //Always get the EntityId of the Level.
            AZ::EntityId levelEntityId;
            ToolsApplicationRequestBus::BroadcastResult(levelEntityId, &ToolsApplicationRequests::GetCurrentLevelEntityId);
            if (!levelEntityId.IsValid())
            {
                return false;
            }

            EditorComponentAPIRequests::GetComponentOutcome outcome;
            EditorComponentAPIBus::BroadcastResult(outcome, &EditorComponentAPIRequests::GetComponentOfType, levelEntityId, componentTypeId);
            return outcome.IsSuccess() && outcome.GetValue().GetComponentId() != AZ::InvalidComponentId;
        }

        size_t EditorLevelComponentAPIComponent::CountComponentsOfType(AZ::Uuid componentTypeId)
        {
            //Always get the EntityId of the Level.
            AZ::EntityId levelEntityId;
            ToolsApplicationRequestBus::BroadcastResult(levelEntityId, &ToolsApplicationRequests::GetCurrentLevelEntityId);
            if (!levelEntityId.IsValid())
            {
                return 0;
            }

            size_t count;
            EditorComponentAPIBus::BroadcastResult(count, &EditorComponentAPIRequests::CountComponentsOfType, levelEntityId, componentTypeId);
            return count;
        }

        EditorComponentAPIRequests::GetComponentOutcome EditorLevelComponentAPIComponent::GetComponentOfType(AZ::Uuid componentTypeId)
        {
            //Always get the EntityId of the Level.
            AZ::EntityId levelEntityId;
            ToolsApplicationRequestBus::BroadcastResult(levelEntityId, &ToolsApplicationRequests::GetCurrentLevelEntityId);
            if (!levelEntityId.IsValid())
            {
                return EditorComponentAPIRequests::GetComponentOutcome(AZStd::unexpect, AZStd::string("GetComponentOfType - Component type of id ") + componentTypeId.ToString<AZStd::string>() + " not found on Level Entity");
            }

            EditorComponentAPIRequests::GetComponentOutcome outcome;
            EditorComponentAPIBus::BroadcastResult(outcome, &EditorComponentAPIRequests::GetComponentOfType, levelEntityId, componentTypeId);
            return outcome;
        }

        EditorComponentAPIRequests::GetComponentsOutcome EditorLevelComponentAPIComponent::GetComponentsOfType(AZ::Uuid componentTypeId)
        {
            //Always get the EntityId of the Level.
            AZ::EntityId levelEntityId;
            ToolsApplicationRequestBus::BroadcastResult(levelEntityId, &ToolsApplicationRequests::GetCurrentLevelEntityId);
            if (!levelEntityId.IsValid())
            {
                return EditorComponentAPIRequests::GetComponentsOutcome(AZStd::unexpect, AZStd::string("GetComponentsOfType - Component type of id ") + componentTypeId.ToString<AZStd::string>() + " not found on Level Entity");
            }

            EditorComponentAPIRequests::GetComponentsOutcome outcome;
            EditorComponentAPIBus::BroadcastResult(outcome, &EditorComponentAPIRequests::GetComponentsOfType, levelEntityId, componentTypeId);
            return outcome;
        }

    } // Components
} // AzToolsFramework
