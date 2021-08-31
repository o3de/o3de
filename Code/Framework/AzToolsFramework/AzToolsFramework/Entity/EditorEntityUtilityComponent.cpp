/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/Entity/EditorEntityUtilityComponent.h>
#include <Entity/EditorEntityContextBus.h>

namespace AzToolsFramework
{
    AZ::EntityId EntityUtilityComponent::CreateEditorReadyEntity(const AZStd::string& entityName)
    {
        auto* newEntity = m_entityContext->CreateEntity(entityName.c_str());

        if (!newEntity)
        {
            AZ_Error("EditorEntityUtility", false, "Failed to create new entity %s", entityName.c_str());
            return AZ::EntityId();
        }

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, *newEntity);

        newEntity->Init();

        return newEntity->GetId();
    }
    
    void EntityUtilityComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EntityUtilityComponent, AZ::Component>();
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EntityUtilityBus>("EntityUtilityBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Entity")
                ->Attribute(AZ::Script::Attributes::Module, "entity")
                ->Event("CreateEditorReadyEntity", &EntityUtilityBus::Events::CreateEditorReadyEntity);
        }
    }

    void EntityUtilityComponent::Activate()
    {
        m_entityContext = AZStd::make_unique<AzFramework::EntityContext>(UtilityEntityContextId);
        m_entityContext->InitContext();
        EntityUtilityBus::Handler::BusConnect();
    }

    void EntityUtilityComponent::Deactivate()
    {
        EntityUtilityBus::Handler::BusConnect();
        m_entityContext = nullptr;
    }
}
