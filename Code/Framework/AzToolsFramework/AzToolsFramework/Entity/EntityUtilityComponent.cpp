/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <sstream>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzToolsFramework/Entity/EntityUtilityComponent.h>
#include <Entity/EditorEntityContextBus.h>
#include <rapidjson/document.h>

namespace AzToolsFramework
{
    void ComponentDetails::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ComponentDetails>()
                ->Field("TypeInfo", &ComponentDetails::m_typeInfo)
                ->Field("BaseClasses", &ComponentDetails::m_baseClasses);

            serializeContext->RegisterGenericType<AZStd::vector<ComponentDetails>>();
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ComponentDetails>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "entity")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("TypeInfo", BehaviorValueProperty(&ComponentDetails::m_typeInfo))
                ->Property("BaseClasses", BehaviorValueProperty(&ComponentDetails::m_baseClasses))
                ->Method("__repr__", [](const ComponentDetails& obj)
                {
                    std::ostringstream result;
                    bool first = true;

                    for (const auto& baseClass : obj.m_baseClasses)
                    {
                        if (!first)
                        {
                            result << ", ";
                        }

                        first = false;
                        result << baseClass.c_str();
                    }
                    
                    return AZStd::string::format("%s, Base Classes: <%s>", obj.m_typeInfo.c_str(), result.str().c_str());
                })
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString);
        }
    }

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
        auto newEntityId = newEntity->GetId();

        m_createdEntities.emplace_back(newEntityId);

        return newEntityId;
    }

    AZ::TypeId GetComponentTypeIdFromName(const AZStd::string& typeName)
    {
        // Try to create a TypeId first.  We won't show any warnings if this fails as the input might be a class name instead
        AZ::TypeId typeId = AZ::TypeId::CreateStringPermissive(typeName.data());

        // If the typeId is null, try a lookup by class name
        if (typeId.IsNull())
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            auto typeNameCrc = AZ::Crc32(typeName.data());
            auto typeUuidList = serializeContext->FindClassId(typeNameCrc);

            // TypeId is invalid or class name is invalid
            if (typeUuidList.empty())
            {
                AZ_Error("EntityUtilityComponent", false, "Provided type %s is either an invalid TypeId or does not match any class names", typeName.c_str());
                return AZ::TypeId::CreateNull();
            }

            typeId = typeUuidList[0];
        }

        return typeId;
    }

    AZ::Component* FindComponentHelper(AZ::EntityId entityId, const AZ::TypeId& typeId, AZ::ComponentId componentId, bool createComponent = false)
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

        AZ::Component* component = FindComponentHelper(entityId, typeId, AZ::InvalidComponentId, true);

        return component ? AzFramework::BehaviorComponentId(component->GetId()) : 
            AzFramework::BehaviorComponentId(AZ::InvalidComponentId);
    }

    bool EntityUtilityComponent::UpdateComponentForEntity(AZ::EntityId entityId, AzFramework::BehaviorComponentId componentId, const AZStd::string& json)
    {
        if (!componentId.IsValid())
        {
            AZ_Error("EntityUtilityComponent", false, "Invalid componentId passed to UpdateComponentForEntity");
            return false;
        }

        AZ::Component* component = FindComponentHelper(entityId, AZ::TypeId::CreateNull(), componentId);

        if (!component)
        {
            return false;
        }

        using namespace AZ::JsonSerializationResult;

        AZ::JsonDeserializerSettings settings = AZ::JsonDeserializerSettings{};
        settings.m_reporting = []([[maybe_unused]] AZStd::string_view message, ResultCode result, AZStd::string_view) -> auto
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

    AZStd::string EntityUtilityComponent::GetComponentDefaultJson(const AZStd::string& typeName)
    {
        AZ::TypeId typeId = GetComponentTypeIdFromName(typeName);

        if (typeId.IsNull())
        {
            // GetComponentTypeIdFromName already does error handling
            return "";
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId);

        if (!classData)
        {
            AZ_Error("EntityUtilityComponent", false, "Failed to find ClassData for typeId %s (%s)", typeId.ToString<AZStd::string>().c_str(), typeName.c_str());
            return "";
        }

        void* component = classData->m_factory->Create("Component");
        rapidjson::Document document;
        AZ::JsonSerializerSettings settings;
        settings.m_keepDefaults = true;

        auto resultCode = AZ::JsonSerialization::Store(document, document.GetAllocator(), component, nullptr, typeId, settings);

        // Clean up the allocated component ASAP, we don't need it anymore
        classData->m_factory->Destroy(component);

        if (resultCode.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
        {
            AZ_Error("EntityUtilityComponent", false, "Failed to serialize component to json (%s): %s",
                typeName.c_str(), resultCode.ToString(typeName).c_str())
            return "";
        }

        AZStd::string jsonString;
        AZ::Outcome<void, AZStd::string> outcome = AZ::JsonSerializationUtils::WriteJsonString(document, jsonString);

        if (!outcome.IsSuccess())
        {
            AZ_Error("EntityUtilityComponent", false, "Failed to write component json to string: %s", outcome.GetError().c_str());
            return "";
        }

        return jsonString;
    }

    AZStd::vector<ComponentDetails> EntityUtilityComponent::FindMatchingComponents(const AZStd::string& searchTerm)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        if (m_typeInfo.empty())
        {
            serializeContext->EnumerateDerived<AZ::Component>(
                [this, serializeContext](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& /*typeId*/)
                {
                    auto& typeInfo = m_typeInfo.emplace_back(classData->m_typeId, classData->m_name, AZStd::vector<AZStd::string>{});

                    serializeContext->EnumerateBase(
                        [&typeInfo](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&)
                        {
                            if (classData)
                            {
                                AZStd::get<2>(typeInfo).emplace_back(classData->m_name);
                            }
                            return true;
                        },
                        classData->m_typeId);
                    
                    return true;
                });
        }
        
        AZStd::vector<ComponentDetails> matches;

        for (const auto& [typeId, typeName, baseClasses] : m_typeInfo)
        {
            if (AZStd::wildcard_match(searchTerm, typeName))
            {
                ComponentDetails details;
                details.m_typeInfo = AZStd::string::format("%s %s", typeId.ToString<AZStd::string>().c_str(), typeName.c_str());
                details.m_baseClasses = baseClasses;
                
                matches.emplace_back(AZStd::move(details));
            }
        }

        return matches;
    }

    void EntityUtilityComponent::ResetEntityContext()
    {
        for (AZ::EntityId entityId : m_createdEntities)
        {
            m_entityContext->DestroyEntityById(entityId);
        }

        m_createdEntities.clear();
        m_entityContext->ResetContext();
    }

    void EntityUtilityComponent::Reflect(AZ::ReflectContext* context)
    {
        ComponentDetails::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EntityUtilityComponent, AZ::Component>();
        }

        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("InvalidComponentId", BehaviorConstant(AZ::InvalidComponentId))
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Entity")
                ->Attribute(AZ::Script::Attributes::Module, "entity");

            behaviorContext->EBus<EntityUtilityBus>("EntityUtilityBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(AZ::Script::Attributes::Category, "Entity")
                ->Attribute(AZ::Script::Attributes::Module, "entity")
                ->Event("CreateEditorReadyEntity", &EntityUtilityBus::Events::CreateEditorReadyEntity)
                ->Event("GetOrAddComponentByTypeName", &EntityUtilityBus::Events::GetOrAddComponentByTypeName)
                ->Event("UpdateComponentForEntity", &EntityUtilityBus::Events::UpdateComponentForEntity)
                ->Event("FindMatchingComponents", &EntityUtilityBus::Events::FindMatchingComponents)
                ->Event("GetComponentDefaultJson", &EntityUtilityBus::Events::GetComponentDefaultJson)
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
