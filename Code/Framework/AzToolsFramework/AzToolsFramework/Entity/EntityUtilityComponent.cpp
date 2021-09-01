/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/Entity/EntityUtilityComponent.h>
#include <Entity/EditorEntityContextBus.h>
#include <rapidjson/document.h>

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

    AZ::Component* FindComponent(AZ::EntityId entityId, const AZ::TypeId& typeId, AZ::ComponentId componentId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

        if (!entity)
        {
            AZ_Error("EntityUtilityComponent", false, "Invalid entityId");
            return nullptr;
        }

        AZ::Component* component = nullptr;
        if (componentId != AZ::InvalidComponentId)
        {
            component = entity->FindComponent(componentId);
        }
        else
        {
            component = entity->FindComponent(typeId);
        }

        if (!component)
        {
            AZ_Error(
                "EntityUtilityComponent", false, "Failed to find component (%s) on entity (%s)",
                componentId != AZ::InvalidComponentId ? AZStd::to_string(componentId).c_str() : typeId.ToString<AZStd::string>().c_str(),
                entity->GetName().c_str());
            return nullptr;
        }

        return component;
    }

    AzFramework::BehaviorComponentId EntityUtilityComponent::FindComponentByTypeName(AZ::EntityId entityId, const AZStd::string& typeName)
    {
        AZ::TypeId typeId;
        if (typeName[0] == '{')
        {
            typeId = AZ::TypeId::CreateString(typeName.data(), sizeof("{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}") - 1);
        }
        else
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            auto typeNameCrc = AZ::Crc32(typeName.data());
            auto typeUuidList = serializeContext->FindClassId(typeNameCrc);
            typeId = typeUuidList[0];
        }

        AZ::Component* component = FindComponent(entityId, typeId, AZ::InvalidComponentId);

        return component ? component->GetId() : AzFramework::BehaviorComponentId(AZ::InvalidComponentId);
    }

    bool EntityUtilityComponent::UpdateComponentForEntity(AZ::EntityId entityId, AzFramework::BehaviorComponentId componentId, const AZStd::string& json)
    {
        if (!componentId.IsValid())
        {
            AZ_Error("EntityUtilityComponent", false, "Invalid componentId passed to UpdateComponentForEntity");
            return false;
        }

        AZ::Component* component = FindComponent(entityId, AZ::TypeId::CreateNull(), componentId);

        if (!component)
        {
            return false;
        }

        using namespace AZ::JsonSerializationResult;

        AZ::JsonDeserializerSettings settings = AZ::JsonDeserializerSettings{};
        settings.m_reporting = [](AZStd::string_view message, ResultCode result, AZStd::string_view path) -> auto
        {
            if (result.GetProcessing() == Processing::Halted)
            {
                AZ_Warning("EntityUtilityComponent", false, "JSON %s\n", message.data());
            }
            return result;
        };
        
        rapidjson::Document doc;
        doc.Parse<rapidjson::kParseCommentsFlag>(json.data(), json.size());
        ResultCode resultCode = AZ::JsonSerialization::Load(*component, doc, settings);

        return resultCode.GetProcessing() != Processing::Halted;
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
                ->Event("CreateEditorReadyEntity", &EntityUtilityBus::Events::CreateEditorReadyEntity)
                ->Event("FindComponentByTypeName", &EntityUtilityBus::Events::FindComponentByTypeName)
                ->Event("UpdateComponentForEntity", &EntityUtilityBus::Events::UpdateComponentForEntity);
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
        EntityUtilityBus::Handler::BusDisconnect();
        m_entityContext = nullptr;
    }
}
