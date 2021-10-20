/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "BehaviorEntity.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////
    // BehaviorComponentId

    void BehaviorComponentId::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BehaviorComponentId>()
                ->Version(1)
                ->Field("ComponentId", &BehaviorComponentId::m_id)
                ;

            serializeContext->RegisterGenericType<AZStd::vector<BehaviorComponentId>>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<BehaviorComponentId>("ComponentId")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constructor()
                ->Method("IsValid", &BehaviorComponentId::IsValid)
                ->Method("Equal", &BehaviorComponentId::operator==)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Method("ToString", &BehaviorComponentId::ToString)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;
        }
    }

    BehaviorComponentId::BehaviorComponentId(AZ::ComponentId id)
        : m_id(id)
    {
    }

    BehaviorComponentId::operator AZ::ComponentId() const
    {
        return m_id;
    }

    bool BehaviorComponentId::operator==(const BehaviorComponentId& rhs) const
    {
        return m_id == rhs.m_id;
    }

    bool BehaviorComponentId::IsValid() const
    {
        return m_id != AZ::InvalidComponentId;
    }

    AZStd::string BehaviorComponentId::ToString() const
    {
        return AZStd::string::format("[%llu]", m_id);
    }

    ////////////////////////////////////////////////////////////////////////////
    // BehaviorEntity

    namespace Internal
    {
        void BehaviorEntityScriptConstructor(BehaviorEntity* self, AZ::ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() == 0)
            {
                *self = BehaviorEntity();
                return;
            }
            else if (dc.GetNumArguments() == 1)
            {
                if (dc.IsClass<AZ::EntityId>(0))
                {
                    AZ::EntityId entityId;
                    dc.ReadArg(0, entityId);
                    new(self) BehaviorEntity(entityId);
                    return;
                }
                else if (dc.IsNil(0))
                {
                    new(self) BehaviorEntity(nullptr);
                    return;
                }
                // Constructor taking AZ::Entity* isn't supported.
                // AZ::Entity is not exposed to BehaviorContext, so we can't detect args of that type.
             }

            dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "Invalid arguments passed to BehaviorEntity().");
            new(self) BehaviorEntity();
        }

        const char* GetComponentName(const AZ::TypeId& componentTypeId)
        {
            AZ::ComponentDescriptor* descriptor = nullptr;
            AZ::ComponentDescriptorBus::EventResult(descriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
            return descriptor ? descriptor->GetName() : "<unknown>";
        }
    }

    void BehaviorEntity::Reflect(AZ::ReflectContext* context)
    {
        BehaviorComponentId::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BehaviorEntity>()
                ->Field("EntityId", &BehaviorEntity::m_entityId)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BehaviorEntity>("Entity", "Entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BehaviorEntity::m_entityId, "EntityId", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<BehaviorEntity>("Entity")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::ConstructorOverride, &Internal::BehaviorEntityScriptConstructor)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "entity")
                ->Constructor()
                ->Constructor<AZ::EntityId>()
                ->Constructor<AZ::Entity*>()
                ->Method("GetName", &BehaviorEntity::GetName)
                ->Method("SetName", &BehaviorEntity::SetName)
                ->Method("GetId", &BehaviorEntity::GetId)
                ->Method("GetOwningContextId", &BehaviorEntity::GetOwningContextId)
                ->Method("IsValid", &BehaviorEntity::IsValid)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)
                ->Method("Exists", &BehaviorEntity::Exists)
                ->Method("IsActivated", &BehaviorEntity::IsActivated)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)
                ->Method("Activate", &BehaviorEntity::Activate)
                ->Method("Deactivate", &BehaviorEntity::Deactivate)
                ->Method("CreateComponent", &BehaviorEntity::CreateComponent, behaviorContext->MakeDefaultValues(static_cast<const AZ::ComponentConfig*>(nullptr)))
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::List)
                ->Method("DestroyComponent", &BehaviorEntity::DestroyComponent)
                ->Method("GetComponents", &BehaviorEntity::GetComponents)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)
                ->Method("FindComponentOfType", &BehaviorEntity::FindComponentOfType)
                ->Method("FindAllComponentsOfType", &BehaviorEntity::FindAllComponentsOfType)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::All)
                ->Method("GetComponentType", &BehaviorEntity::GetComponentType)
                ->Method("GetComponentName", &BehaviorEntity::GetComponentName)
                ->Method("SetComponentConfiguration", &BehaviorEntity::SetComponentConfiguration)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::List)
                ->Method("GetComponentConfiguration", &BehaviorEntity::GetComponentConfiguration)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::List)
                // Allow BehaviorEntity to be passed to functions expecting AZ::Entity*
                ->WrappingMember<AZ::Entity*>(&BehaviorEntity::GetRawEntityPtr)
                ;
        }
    }

    BehaviorEntity::BehaviorEntity(AZ::EntityId entityId)
        : m_entityId(entityId)
    {
    }

    BehaviorEntity::BehaviorEntity(AZ::Entity* entity)
        : m_entityId(entity ? entity->GetId() : AZ::EntityId())
    {
    }

    AZStd::string BehaviorEntity::GetName() const
    {
        AZ::Entity* entity;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, nullptr, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot get entity name. %s", errorMessage.c_str());
            return "";
        }

        return entity->GetName();
    }

    void BehaviorEntity::SetName(const char* name)
    {
        AZ::Entity* entity;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, nullptr, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot set entity name. %s", errorMessage.c_str());
            return;
        }

        entity->SetName(name);
    }

    AzFramework::EntityContextId BehaviorEntity::GetOwningContextId() const
    {
        if (!m_entityId.IsValid())
        {
            AZ_Warning("Entity", false, "Cannot get entity context. Entity ID is invalid.");
            return EntityContextId::CreateNull();
        }

        // no further warnings, iquerying missing entities is a valid use case
        EntityContextId contextId = EntityContextId::CreateNull();
        EntityIdContextQueryBus::EventResult(contextId, m_entityId, &EntityIdContextQueryBus::Events::GetOwningContextId);
        return contextId;
    }

    bool BehaviorEntity::Exists() const
    {
        if (!m_entityId.IsValid())
        {
            AZ_Warning("Entity", false, "Cannot check entity existence. Entity ID is invalid.");
            return false;
        }

        // no further warnings, querying missing entities is a valid use case
        return GetValidEntity(nullptr, nullptr, nullptr);
    }

    bool BehaviorEntity::IsActivated() const
    {
        AZ::Entity* entity;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, nullptr, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot get entity activation. %s", errorMessage.c_str());
            return false;
        }

        AZ::Entity::State state = entity->GetState();
        return (state == AZ::Entity::State::Active || state == AZ::Entity::State::Activating);
    }

    void BehaviorEntity::Activate()
    {
        AZ::Entity* entity;
        EntityContextId contextId;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, &contextId, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot activate entity. %s", errorMessage.c_str());
            return;
        }

        if (entity->GetState() != AZ::Entity::State::Init)
        {
            AZ_Warning("Entity", false, "Cannot activate entity. Entity (id=%s name='%s') must be in the initialized state.", m_entityId.ToString().c_str(), entity->GetName().c_str());
            return;
        }

        EntityContextRequestBus::Event(contextId, &EntityContextRequestBus::Events::ActivateEntity, m_entityId);
        // don't warn if activation fails, Entity::Activate() already issues warnings
     }

    void BehaviorEntity::Deactivate()
    {
        AZ::Entity* entity;
        EntityContextId contextId;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, &contextId, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot deactivate entity. %s", errorMessage.c_str());
            return;
        }

        AZ::Entity::State state = entity->GetState();
        if (state != AZ::Entity::State::Active && state != AZ::Entity::State::Activating)
        {
            AZ_Warning("Entity", false, "Cannot deactivate entity. Entity (id=%s name='%s') must be in the activated state.", m_entityId.ToString().c_str(), entity->GetName().c_str());
            return;
        }

        EntityContextRequestBus::Event(contextId, &EntityContextRequestBus::Events::DeactivateEntity, m_entityId);
    }

    BehaviorComponentId BehaviorEntity::CreateComponent(const AZ::TypeId& componentTypeId, const AZ::ComponentConfig* componentConfig /*=nullptr*/)
    {
        AZ::Entity* entity;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, nullptr, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot create component. %s", errorMessage.c_str());
            return AZ::InvalidComponentId;
        }
        
        // don't create component if an incompatible component exists on the entity
        AZStd::vector<AZ::Component*> incompatibleComponents;
        entity->IsComponentReadyToAdd(componentTypeId, nullptr, &incompatibleComponents);
        if (!incompatibleComponents.empty())
        {
            AZ_Warning("Entity", false, "Cannot create component '%s' because it is incompatible with existing component '%s' on entity (id=%s name='%s').",
                Internal::GetComponentName(componentTypeId), Internal::GetComponentName(azrtti_typeid(incompatibleComponents[0])), m_entityId.ToString().c_str(), entity->GetName().c_str());
            return AZ::InvalidComponentId;
        }

        AZ::Component* component = entity->CreateComponent(componentTypeId);
        if (!component)
        {
            AZ_Warning("Entity", false, "Failed to create component (type=%s) on entity (id=%s name='%s')", componentTypeId.ToString<AZStd::string>().c_str(), m_entityId.ToString().c_str(), entity->GetName().c_str());
            return AZ::InvalidComponentId;
        }

        if (componentConfig)
        {
            component->SetConfiguration(*componentConfig);
            // don't warn if configuration fails. Entity::SetConfiguration() already gives good warnings.
        }

        return component->GetId();
    }

    bool BehaviorEntity::DestroyComponent(BehaviorComponentId componentId)
    {
        AZ::Component* component;
        AZStd::string errorMessage;
        if (!GetValidComponent(componentId, &component, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot destroy component. %s", errorMessage.c_str());
            return false;
        }

        if (!component->GetEntity()->RemoveComponent(component))
        {
            AZ_Warning("Entity", false, "Cannot destroy component. Failed to remove component (id=%llu) from entity (id=%s name='%s').", componentId, m_entityId.ToString().c_str(), component->GetEntity()->GetName().c_str());
            return false;
        }

        delete component;
        return true;
    }

    AZStd::vector<BehaviorComponentId> BehaviorEntity::GetComponents() const
    {
        AZ::Entity* entity;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, nullptr, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot get components. %s", errorMessage.c_str());
            return AZStd::vector<BehaviorComponentId>();
        }

        AZStd::vector<BehaviorComponentId> components;
        for (AZ::Component* component : entity->GetComponents())
        {
            components.emplace_back(component->GetId());
        }
        return components;
    }

    BehaviorComponentId BehaviorEntity::FindComponentOfType(const AZ::TypeId& componentTypeId) const
    {
        AZ::Entity* entity;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, nullptr, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot find component. %s", errorMessage.c_str());
            return AZ::InvalidComponentId;
        }

        BehaviorComponentId componentId = AZ::InvalidComponentId;
        if (const AZ::Component* component = entity->FindComponent(componentTypeId))
        {
            componentId = component->GetId();
        }
        return componentId;
    }

    AZStd::vector<BehaviorComponentId> BehaviorEntity::FindAllComponentsOfType(const AZ::TypeId& componentTypeId) const
    {
        AZ::Entity* entity;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, nullptr, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot find components. %s", errorMessage.c_str());
            return AZStd::vector<BehaviorComponentId>();
        }

        AZStd::vector<BehaviorComponentId> components;
        for (const AZ::Component* component : entity->FindComponents(componentTypeId))
        {
            components.emplace_back(component->GetId());
        }
        return components;
    }

    AZ::TypeId BehaviorEntity::GetComponentType(BehaviorComponentId componentId) const
    {
        AZ::Component* component;
        AZStd::string errorMessage;
        if (!GetValidComponent(componentId, &component, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot get component type. %s", errorMessage.c_str());
            return AZ::TypeId::CreateNull();
        }

        return azrtti_typeid(component);
    }

    AZStd::string BehaviorEntity::GetComponentName(BehaviorComponentId componentId) const
    {
        AZ::Component* component;
        AZStd::string errorMessage;
        if (!GetValidComponent(componentId, &component, &errorMessage))
        {
            AZ_Warning("Entity", false, "Cannot get component name. %s", errorMessage.c_str());
            return "";
        }

        return component->RTTI_GetTypeName();
    }

    bool BehaviorEntity::SetComponentConfiguration(BehaviorComponentId componentId, const AZ::ComponentConfig& componentConfig)
    {
        AZ::Component* component;
        AZStd::string errorMessage;
        if (!GetValidComponent(componentId, &component, &errorMessage))
        {
            AZ_Warning("Entity", false, "Failed to set component configuration. %s", errorMessage.c_str());
            return false;
        }

        bool success = component->SetConfiguration(componentConfig);
        // don't warn if configuration fails. Entity::SetConfiguration() already gives good warnings.

        return success;
    }

    bool BehaviorEntity::GetComponentConfiguration(BehaviorComponentId componentId, AZ::ComponentConfig& outComponentConfig) const
    {
        AZ::Component* component;
        AZStd::string errorMessage;
        if (!GetValidComponent(componentId, &component, &errorMessage))
        {
            AZ_Warning("Entity", false, "Failed to get component configuration. %s", errorMessage.c_str());
            return false;
        }

        bool success = component->GetConfiguration(outComponentConfig);
        // don't warn if configuration fails. Entity::GetConfiguration() already gives good warnings.

        return success;
    }

    AZ::Entity* BehaviorEntity::GetRawEntityPtr()
    {
        AZ::Entity* entity;
        AZStd::string errorMessage;
        if (!GetValidEntity(&entity, nullptr, &errorMessage))
        {
            AZ_Warning("Entity", false, errorMessage.c_str());
            return nullptr;
        }
        return entity;
    }

    bool BehaviorEntity::GetValidEntity(AZ::Entity** outEntity, EntityContextId* outContextId, AZStd::string* outErrorMessage) const
    {
        AZ::Entity* entity = nullptr;
        EntityContextId contextId = EntityContextId::CreateNull();
        AZStd::string errorMessage;
        bool success = false;

        if (m_entityId.IsValid())
        {
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_entityId);
            if (entity)
            {
                    EntityIdContextQueryBus::EventResult(contextId, m_entityId, &EntityIdContextQueryBus::Events::GetOwningContextId);
                    if (!contextId.IsNull())
                    {
                        success = true;
                    }
                    else
                    {
                        errorMessage = AZStd::string::format("Entity has no owning context (id=%s name='%s')", m_entityId.ToString().c_str(), entity->GetName().c_str());
                }
            }
            else
            {
                errorMessage = AZStd::string::format("Entity does not exist (id=%s)", m_entityId.ToString().c_str());
            }
        }
        else
        {
            errorMessage = "Entity ID is invalid";
        }

        if (outEntity)
        {
            *outEntity = success ? entity : nullptr;
        }
        if (outContextId)
        {
            *outContextId = success ? contextId : AZ::TypeId::CreateNull();
        }
        if (outErrorMessage)
        {
            if (success)
            {
                outErrorMessage->clear();
            }
            else
            {
                *outErrorMessage = AZStd::move(errorMessage);
            }
        }
        return success;
    }

    bool BehaviorEntity::GetValidComponent(BehaviorComponentId componentId, AZ::Component** outComponent, AZStd::string* outErrorMessage) const
    {
        AZ::Entity* entity = nullptr;
        AZ::Component* component = nullptr;
        AZStd::string errorMessage;
        bool success = false;

        if (GetValidEntity(&entity, nullptr, &errorMessage))
        {
            component = entity->FindComponent(componentId);
            if (component)
            {
                success = true;
            }
            else
            {
                errorMessage = AZStd::string::format("Component (id=%llu) not found on entity(id=%s name='%s').", static_cast<AZ::u64>(componentId), m_entityId.ToString().c_str(), entity->GetName().c_str());
            }
        }

        if (outComponent)
        {
            *outComponent = success ? component : nullptr;
        }
        if (outErrorMessage)
        {
            if (success)
            {
                outErrorMessage->clear();
            }
            else
            {
                *outErrorMessage = AZStd::move(errorMessage);
            }
        }
        return success;
    }

} // namespace AzFramework
