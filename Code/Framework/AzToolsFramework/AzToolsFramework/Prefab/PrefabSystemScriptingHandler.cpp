/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <API/ToolsApplicationAPI.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <Prefab/PrefabSystemComponentInterface.h>
#include <Prefab/PrefabSystemScriptingHandler.h>
#include <Prefab/EditorPrefabComponent.h>
#include <ToolsComponents/TransformComponent.h>

namespace AzToolsFramework::Prefab
{
    static AZStd::unique_ptr<AZ::Entity> CreateContainerEntityAndParentEntities(const AZStd::vector<AZ::Entity*> entities)
    {
        bool result = false;
        AZ::EntityId commonRoot;
        EntityList topLevelEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            result, &AzToolsFramework::ToolsApplicationRequestBus::Events::FindCommonRootInactive, entities, commonRoot, &topLevelEntities);

        auto containerEntity = AZStd::make_unique<AZ::Entity>();

        containerEntity->CreateComponent<Components::EditorLockComponent>();
        containerEntity->CreateComponent<Components::EditorVisibilityComponent>();
        containerEntity->CreateComponent<Prefab::EditorPrefabComponent>();
        containerEntity->CreateComponent<Components::TransformComponent>();

        for (AZ::Entity* entity : topLevelEntities)
        {
            if (auto* transformComponent = entity->FindComponent<Components::TransformComponent>(); transformComponent)
            {
                transformComponent->SetParent(containerEntity->GetId());
            }
        }
        return containerEntity;
    }

    void PrefabSystemScriptingHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("InvalidTemplateId", BehaviorConstant(InvalidTemplateId))
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "prefab")
                ->Attribute(AZ::Script::Attributes::Category, "Prefab");

            behaviorContext->EBus<PrefabSystemScriptingBus>("PrefabSystemScriptingBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "prefab")
                ->Attribute(AZ::Script::Attributes::Category, "Prefab")
                ->Event("CreatePrefab", &PrefabSystemScriptingBus::Events::CreatePrefabTemplate);
        }
    }

    void PrefabSystemScriptingHandler::Connect(PrefabSystemComponentInterface* prefabSystemComponentInterface)
    {
        AZ_Assert(prefabSystemComponentInterface != nullptr, "prefabSystemComponentInterface must not be null");
        m_prefabSystemComponentInterface = prefabSystemComponentInterface;
        PrefabSystemScriptingBus::Handler::BusConnect();
    }

    void PrefabSystemScriptingHandler::Disconnect()
    {
        PrefabSystemScriptingBus::Handler::BusDisconnect();
    }

    TemplateId PrefabSystemScriptingHandler::CreatePrefabTemplate(const AZStd::vector<AZ::EntityId>& entityIds, const AZStd::string& filePath)
    {
        AZStd::vector<AZ::Entity*> entities;

        for (const auto& entityId : entityIds)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

            AZ_Warning(
                "PrefabSystemScriptingHandler", entity, "EntityId %s was not found and will not be added to the prefab",
                entityId.ToString().c_str());

            if (entity)
            {
                entities.push_back(entity);
            }
        }

        AZStd::unique_ptr<AZ::Entity> containerEntity = CreateContainerEntityAndParentEntities(entities);

        auto prefab = m_prefabSystemComponentInterface->CreatePrefab(
            entities, {}, AZ::IO::PathView(AZStd::string_view(filePath)), AZStd::move(containerEntity));

        if (!prefab)
        {
            AZ_Error("PrefabSystemComponenent", false, "Failed to create prefab %s", filePath.c_str());
            return InvalidTemplateId;
        }

        return prefab->GetTemplateId();
    }

    TemplateId PrefabSystemScriptingHandler::CreatePrefabTemplateWithCustomEntityAliases(
        const AZStd::unordered_map<AZ::EntityId, AZStd::string>& entityIds, const AZStd::string& filePath)
    {
        AZStd::map<AZStd::string, AZ::Entity*> entities;
        AZStd::vector<AZ::Entity*> entitiesVector;
        for (const auto& [entityId, entityAlias] : entityIds)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

            AZ_Warning(
                "PrefabSystemScriptingHandler",
                entity,
                "EntityId %s was not found and will not be added to the prefab",
                entityId.ToString().c_str());

            if (entity)
            {
                entities.emplace(entityAlias, entity);
                entitiesVector.push_back(entity);
            }
        }
        
        AZStd::unique_ptr<AZ::Entity> containerEntity = CreateContainerEntityAndParentEntities(entitiesVector);

        auto prefab = m_prefabSystemComponentInterface->CreatePrefabWithCustomEntityAliases(
            entities, {}, AZ::IO::PathView(AZStd::string_view(filePath)), AZStd::move(containerEntity));

        if (!prefab)
        {
            AZ_Error("PrefabSystemScriptingHandler", false, "Failed to create prefab %s", filePath.c_str());
            return InvalidTemplateId;
        }

        return prefab->GetTemplateId();
    }
}
