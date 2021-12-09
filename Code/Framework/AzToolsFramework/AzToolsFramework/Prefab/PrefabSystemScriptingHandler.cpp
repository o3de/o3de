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
                "PrefabSystemComponent", entity, "EntityId %s was not found and will not be added to the prefab",
                entityId.ToString().c_str());

            if (entity)
            {
                entities.push_back(entity);
            }
        }

        bool result = false;
        [[maybe_unused]] AZ::EntityId commonRoot;
        EntityList topLevelEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(result, &AzToolsFramework::ToolsApplicationRequestBus::Events::FindCommonRootInactive,
            entities, commonRoot, &topLevelEntities);


        auto containerEntity = AZStd::make_unique<AZ::Entity>();

        containerEntity->CreateComponent<Components::EditorLockComponent>();
        containerEntity->CreateComponent<Components::EditorVisibilityComponent>();
        containerEntity->CreateComponent<Prefab::EditorPrefabComponent>();

        {
            auto transformComponent = containerEntity->CreateComponent<Components::TransformComponent>();
            // Because procedural prefabs need to be deterministic we need to set the component ID to something unique and non-random
            // The prefab that references the proc prefab will store a patch that references the transform component by it's component ID
            // If this ID is not stable, the proc prefab will lose its position, parenting, etc data next time it is regenerated
            auto hash = TypeHash64(reinterpret_cast<const uint8_t*>(filePath.data()), filePath.length(), AZ::HashValue64{0});

            transformComponent->SetId(static_cast<AZ::ComponentId>(hash));

        }

        for (AZ::Entity* entity : topLevelEntities)
        {
            AzToolsFramework::Components::TransformComponent* transformComponent =
                entity->FindComponent<AzToolsFramework::Components::TransformComponent>();

            if (transformComponent)
            {
                transformComponent->SetParent(containerEntity->GetId());
            }
        }

        auto prefab = m_prefabSystemComponentInterface->CreatePrefab(
            entities, {}, AZ::IO::PathView(AZStd::string_view(filePath)), AZStd::move(containerEntity));

        if (!prefab)
        {
            AZ_Error("PrefabSystemComponenent", false, "Failed to create prefab %s", filePath.c_str());
            return InvalidTemplateId;
        }

        return prefab->GetTemplateId();
    }
}
