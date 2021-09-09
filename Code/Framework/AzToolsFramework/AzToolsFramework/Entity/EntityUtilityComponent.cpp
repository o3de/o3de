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
#include <AzFramework/FileFunc/FileFunc.h>
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

    AZ::TypeId GetComponentTypeIdFromName(const AZStd::string& typeName)
    {
        AZ::TypeId typeId;
        if (typeName[0] == '{')
        {
            typeId = AZ::TypeId::CreateStringPermissive(typeName.data(), AZ::TypeId::MaxStringBuffer, false);

            if (typeId.IsNull())
            {
                AZ_Error("EntityUtilityComponent", false, "Invalid type id %s", typeName.c_str());
                return AZ::TypeId::CreateNull();
            }
        }
        else
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            auto typeNameCrc = AZ::Crc32(typeName.data());
            auto typeUuidList = serializeContext->FindClassId(typeNameCrc);

            if (typeUuidList.empty())
            {
                AZ_Error("EntityUtilityComponent", false, "Failed to find ClassId for component type name %s", typeName.c_str());
                return AZ::TypeId::CreateNull();
            }

            typeId = typeUuidList[0];
        }

        return typeId;
    }

    AZ::Component* FindComponent(AZ::EntityId entityId, const AZ::TypeId& typeId, AZ::ComponentId componentId, bool createComponent = false)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

        if (!entity)
        {
            AZ_Error("EntityUtilityComponent", false, "Invalid entityId %s", entityId.ToString().c_str());
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

        if (!component && createComponent)
        {
            component = entity->CreateComponent(typeId);
        }

        if (!component)
        {
            AZ_Error(
                "EntityUtilityComponent", false, "Failed to find component (%s) on entity %s (%s)",
                componentId != AZ::InvalidComponentId ? AZStd::to_string(componentId).c_str()
                                                      : typeId.ToString<AZStd::string>().c_str(),
                entityId.ToString().c_str(),
                entity->GetName().c_str());
            return nullptr;
        }

        return component;
    }

    AzFramework::BehaviorComponentId EntityUtilityComponent::GetOrAddComponentByTypeName(AZ::EntityId entityId, const AZStd::string& typeName)
    {
        AZ::TypeId typeId = GetComponentTypeIdFromName(typeName);

        if (typeId.IsNull())
        {
            return AzFramework::BehaviorComponentId(AZ::InvalidComponentId);
        }

        AZ::Component* component = FindComponent(entityId, typeId, AZ::InvalidComponentId, true);

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
        settings.m_reporting = [](AZStd::string_view message, ResultCode result, AZStd::string_view) -> auto
        {
            if (result.GetProcessing() == Processing::Halted)
            {
                AZ_Error("EntityUtilityComponent", false, "JSON %s\n", message.data());
            }
            else if (result.GetOutcome() > Outcomes::PartialDefaults)
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

    AZStd::string EntityUtilityComponent::GetComponentJson(const AZStd::string& typeName)
    {
        AZ::TypeId typeId = GetComponentTypeIdFromName(typeName);

        if (typeId.IsNull())
        {
            return "";
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId);

        if (!classData)
        {
            return "";
        }

        void* component = classData->m_factory->Create("Component");
        rapidjson::Document document;
        AZ::JsonSerializerSettings settings;
        settings.m_keepDefaults = true;

        auto resultCode = AZ::JsonSerialization::Store(document, document.GetAllocator(), component, nullptr, typeId, settings);

        if (resultCode.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
        {
            return "";
        }

        AZStd::string jsonString;
        AzFramework::FileFunc::WriteJsonToString(document, jsonString);

        return jsonString;
    }

    AZStd::vector<AZStd::string> EntityUtilityComponent::SearchComponents(const AZStd::string& searchTerm)
    {
        if (m_typeNames.empty())
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            serializeContext->EnumerateDerived<AZ::Component>(
                [this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& /*typeId*/)
                {
                    m_typeNames.emplace_back(classData->m_typeId, classData->m_name);

                    return true;
                });
        }
        
        AZStd::vector<AZStd::string> matches;

        for (const auto& [typeId, typeName] : m_typeNames)
        {
            if (AZStd::wildcard_match(searchTerm, typeName))
            {
                matches.emplace_back(
                    AZStd::string::format("%s %s", typeId.ToString<AZStd::string>().c_str(), typeName.c_str()));
            }
        }

        return matches;
    }

    void EntityUtilityComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EntityUtilityComponent, AZ::Component>();
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("InvalidComponentId", BehaviorConstant(AZ::InvalidComponentId));

            behaviorContext->EBus<EntityUtilityBus>("EntityUtilityBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Entity")
                ->Attribute(AZ::Script::Attributes::Module, "entity")
                ->Event("CreateEditorReadyEntity", &EntityUtilityBus::Events::CreateEditorReadyEntity)
                ->Event("GetOrAddComponentByTypeName", &EntityUtilityBus::Events::GetOrAddComponentByTypeName)
                ->Event("UpdateComponentForEntity", &EntityUtilityBus::Events::UpdateComponentForEntity)
                ->Event("SearchComponents", &EntityUtilityBus::Events::SearchComponents)
                ->Event("GetComponentJson", &EntityUtilityBus::Events::GetComponentJson)
            ;
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
