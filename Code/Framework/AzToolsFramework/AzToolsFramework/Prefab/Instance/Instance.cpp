/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Component/Entity.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapperInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        Instance::Instance()
        {
            m_instanceEntityMapper = AZ::Interface<InstanceEntityMapperInterface>::Get();

            AZ_Assert(m_instanceEntityMapper,
                "Instance Entity Mapper Interface could not be found. "
                "It is a requirement for the Prefab Instance class. "
                "Check that it is being correctly initialized.");

            m_templateInstanceMapper = AZ::Interface<TemplateInstanceMapperInterface>::Get();

            AZ_Assert(m_templateInstanceMapper,
                "Template Instance Mapper Interface could not be found. "
                "It is a requirement for the Prefab Instance class. "
                "Check that it is being correctly initialized.");

            m_containerEntity = AZStd::make_unique<AZ::Entity>();
            EntityAlias containerEntityAlias = GenerateEntityAlias();
            RegisterEntity(m_containerEntity->GetId(), containerEntityAlias);
        }

        Instance::~Instance()
        {
            // Clean up Instance associations.
            if (m_templateId != InvalidTemplateId &&
                !m_templateInstanceMapper->UnregisterInstance(*this))
            {
                AZ_Assert(false,
                    "Prefab - Attempted to unregister Instance from Template on file path '%s' with Id '%u'.  "
                    "Instance may never have been registered or was unregistered early.",
                    m_templateSourcePath.c_str(),
                    m_templateId);
            }
            
            ClearEntities();

            m_nestedInstances.clear();
        }

        void Instance::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

            if (serialize)
            {
                serialize->Class<Instance>()
                    ->Field("m_containerEntity", &Instance::m_containerEntity)
                    ->Field("m_entities", &Instance::m_entities)
                    ->Field("m_nestedInstances", &Instance::m_nestedInstances)
                    ->Field("m_templateSourcePath", &Instance::m_templateSourcePath)
                    ;
            }
        }

        const TemplateId& Instance::GetTemplateId() const
        {
            return m_templateId;
        }

        void Instance::SetTemplateId(const TemplateId& templateId)
        {
            // If this instance's templateId is valid, we should be able to unregister this instance from 
            // Template to Instance mapping successfully.
            if (m_templateId != InvalidTemplateId &&
                !m_templateInstanceMapper->UnregisterInstance(*this))
            {
                AZ_Assert(false,
                    "Prefab - Attempted to Unregister Instance from Template with Id '%u'.  "
                    "Instance may never have been registered or was unregistered early.",
                    m_templateId);
            }

            m_templateId = templateId;

            // If this instance's templateId is valid, we should be able to register this instance to 
            // Template to Instance mapping successfully.
            if (m_templateId != InvalidTemplateId &&
                !m_templateInstanceMapper->RegisterInstanceToTemplate(*this))
            {
                AZ_Assert(false,
                    "Prefab - Failed to register Instance to its Template with Id '%u'. "
                    "This Instance is likely already registered.",
                    m_templateId);
            }
        }

        void Instance::SetLinkId(LinkId linkId)
        {
            m_linkId = AZStd::move(linkId);
        }

        LinkId Instance::GetLinkId() const
        {
            return m_linkId;
        }

        const AZStd::string& Instance::GetTemplateSourcePath() const
        {
            return m_templateSourcePath;
        }

        void Instance::SetTemplateSourcePath(AZStd::string sourcePath)
        {
            m_templateSourcePath = AZStd::move(sourcePath);

            AZStd::string filename;
            if (AZ::StringFunc::Path::GetFileName(m_templateSourcePath.c_str(), filename))
            {
                m_containerEntity->SetName(filename);
            }
        }

        bool Instance::AddEntity(AZ::Entity& entity)
        {
            EntityAlias newEntityAlias = GenerateEntityAlias();
            if (!RegisterEntity(entity.GetId(), newEntityAlias))
            {
                return false;
            }

            if (!m_entities.emplace(AZStd::make_pair(newEntityAlias, &entity)).second)
            {
                return false;
            }

            return true;
        }

        AZStd::unique_ptr<AZ::Entity> Instance::DetachEntity(const AZ::EntityId& entityId)
        {
            EntityAlias entityAliasToRemove;
            auto instanceToTemplateEntityIdIterator = m_instanceToTemplateEntityIdMap.find(entityId);
            if (instanceToTemplateEntityIdIterator != m_instanceToTemplateEntityIdMap.end())
            {
                entityAliasToRemove = instanceToTemplateEntityIdIterator->second;
                bool isEntityRemoved = m_instanceEntityMapper->UnregisterEntity(entityId) &&
                    m_templateToInstanceEntityIdMap.erase(entityAliasToRemove) && m_instanceToTemplateEntityIdMap.erase(entityId);
                AZ_Assert(isEntityRemoved,
                    "Prefab - Failed to remove entity with id %s with a Prefab Instance derived from source asset %s "
                    "This happens when the entity is not correctly removed from all the prefab system entity maps.",
                    entityId.ToString().c_str(), m_templateSourcePath.c_str());
            }
            return DetachEntity(entityAliasToRemove);
        }

        AZStd::unique_ptr<AZ::Entity> Instance::DetachEntity(const EntityAlias& entityAlias)
        {
            AZStd::unique_ptr<AZ::Entity> removedEntity;
            auto&& entityIterator = m_entities.find(entityAlias);
            if (entityIterator != m_entities.end())
            {
                removedEntity = AZStd::move(entityIterator->second);
                m_entities.erase(entityAlias);
            }
            return removedEntity;
        }

        void Instance::DetachNestedEntities(const AZStd::function<void(AZStd::unique_ptr<AZ::Entity>)>& callback)
        {
            DetachEntities(callback);

            for (const auto& [instanceAlias, instance] : m_nestedInstances)
            {
                instance->DetachNestedEntities(callback);
            }
        }

        void Instance::DetachEntities(const AZStd::function<void(AZStd::unique_ptr<AZ::Entity>)>& callback)
        {
            for (auto&& [entityAlias, entity] : m_entities)
            {
                m_instanceEntityMapper->UnregisterEntity(entity->GetId());
                m_templateToInstanceEntityIdMap.erase(entityAlias);
                m_instanceToTemplateEntityIdMap.erase(entity->GetId());

                callback(AZStd::move(entity));
            }

            m_entities.clear();
        }

        void Instance::RemoveNestedEntities(
            const AZStd::function<bool(const AZStd::unique_ptr<AZ::Entity>&)>& filter)
        {
            RemoveEntities(filter);

            for (const auto& [instanceAlias, instance] : m_nestedInstances)
            {
                instance->RemoveNestedEntities(filter);
            }
        }

        void Instance::RemoveEntities(
            const AZStd::function<bool(const AZStd::unique_ptr<AZ::Entity>&)>& filter)
        {
            AZStd::erase_if(m_entities,
                [this, &filter](const auto& item)
                {
                    const auto& [entityAlias, entity] = item;
                    const bool shouldRemove = filter(entity);
                    if (shouldRemove)
                    {
                        m_instanceEntityMapper->UnregisterEntity(entity->GetId());
                        m_templateToInstanceEntityIdMap.erase(entityAlias);
                        m_instanceToTemplateEntityIdMap.erase(entity->GetId());
                    }
                    return shouldRemove;
                }
            );
        }

        void Instance::ClearEntities()
        {
            for (const auto&[entityAlias, entity] : m_entities)
            {
                if (entity)
                {
                    // Clean up our entity associations
                    if (!m_instanceEntityMapper->UnregisterEntity(entity->GetId()))
                    {
                        AZ_Assert(false,
                            "Prefab - Attempted to Unregister entity with id '%s' from Prefab Instance derived from source asset '%s' "
                            "Entity may never have been registered or was Unregistered early.",
                            entity->GetId().ToString().c_str(),
                            m_templateSourcePath.c_str());
                    }
                }
            }

            m_instanceToTemplateEntityIdMap.clear();
            m_templateToInstanceEntityIdMap.clear();
            m_entities.clear();
        }

        bool Instance::RegisterEntity(const AZ::EntityId& entityId, const EntityAlias& entityAlias)
        {
            if (!m_instanceEntityMapper->RegisterEntityToInstance(entityId, *this))
            {
                AZ_Assert(false,
                    "Prefab - Failed to register entity with id %s with a Prefab Instance derived from source asset %s "
                    "This entity is likely already registered. Check for a double add.",
                    entityId.ToString().c_str(),
                    m_templateSourcePath.c_str());

                return false;
            }

            m_instanceToTemplateEntityIdMap.emplace(AZStd::make_pair(entityId, entityAlias));
            m_templateToInstanceEntityIdMap.emplace(AZStd::make_pair(entityAlias, entityId));

            return true;
        }

        Instance& Instance::AddInstance(AZStd::unique_ptr<Instance> instance)
        {
            InstanceAlias newInstanceAlias = GenerateInstanceAlias();
            AZ_Assert(instance.get(), "instance argument is nullptr");
            AZ_Assert(m_nestedInstances.find(newInstanceAlias) == m_nestedInstances.end(), "InstanceAlias' unique id collision, this should never happen.");
            instance->m_parent = this;
            instance->m_alias = newInstanceAlias;
            return *(m_nestedInstances[newInstanceAlias] = std::move(instance));
        }

        AZStd::unique_ptr<Instance> Instance::DetachNestedInstance(const InstanceAlias& instanceAlias)
        {
            AZStd::unique_ptr<Instance> removedNestedInstance;
            auto&& nestedInstanceIterator = m_nestedInstances.find(instanceAlias);
            if (nestedInstanceIterator != m_nestedInstances.end())
            {
                removedNestedInstance = AZStd::move(nestedInstanceIterator->second);

                removedNestedInstance->m_parent = nullptr;
                removedNestedInstance->m_alias = InstanceAlias();

                m_nestedInstances.erase(instanceAlias);
            }
            return removedNestedInstance;
        }

        AZStd::vector<EntityAlias> Instance::GetEntityAliases()
        {
            AZStd::vector<EntityAlias> entityAliases;
            for (const auto& [entityAlias, entity] : m_entities)
            {
                entityAliases.push_back(entityAlias);
            }

            return entityAliases;
        }

        void Instance::GetNestedEntityIds(const AZStd::function<bool(AZ::EntityId)>& callback)
        {
            GetEntityIds(callback);

            for (auto&&[instanceAlias, instance] : m_nestedInstances)
            {
                instance->GetNestedEntityIds(callback);
            }
        }

        void Instance::GetEntityIds(const AZStd::function<bool(AZ::EntityId)>& callback)
        {
            for (auto&&[entityAlias, entityId] : m_templateToInstanceEntityIdMap)
            {
                if (!callback(entityId))
                {
                    break;
                }
            }
        }

        void Instance::GetConstNestedEntities(const AZStd::function<bool(const AZ::Entity&)>& callback)
        {
            GetConstEntities(callback);

            for (const auto& [instanceAlias, instance] : m_nestedInstances)
            {
                instance->GetConstNestedEntities(callback);
            }
        }

        void Instance::GetConstEntities(const AZStd::function<bool(const AZ::Entity&)>& callback)
        {
            for (const auto& [entityAlias, entity] : m_entities)
            {
                if (!entity)
                {
                    continue;
                }

                if (!callback(*entity))
                {
                    break;
                }
            }
        }

        void Instance::GetNestedEntities(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback)
        {
            GetEntities(callback);

            for (auto& [instanceAlias, instance] : m_nestedInstances)
            {
                instance->GetNestedEntities(callback);
            }
        }

        void Instance::GetEntities(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback)
        {
            for (auto& [entityAlias, entity] : m_entities)
            {
                if (!callback(entity))
                {
                    break;
                }
            }
        }

        void Instance::GetEntities(EntityList& entities, bool includeNestedEntities)
        {
            // Non-recursive traversal of instances
            AZStd::vector<Instance*> instancesToTraverse = { this };
            while (!instancesToTraverse.empty())
            {
                Instance* currentInstance = instancesToTraverse.back();
                instancesToTraverse.pop_back();
                if (includeNestedEntities)
                {
                    instancesToTraverse.reserve(instancesToTraverse.size() + currentInstance->m_nestedInstances.size());
                    for (const auto& instanceByAlias : currentInstance->m_nestedInstances)
                    {
                        instancesToTraverse.push_back(instanceByAlias.second.get());
                    }
                }

                entities.reserve(entities.size() + currentInstance->m_entities.size());
                for (const auto& entityByAlias : currentInstance->m_entities)
                {
                    entities.push_back(entityByAlias.second.get());
                }
            }
        }

        EntityAliasOptionalReference Instance::GetEntityAlias(const AZ::EntityId& id)
        {
            if (m_instanceToTemplateEntityIdMap.count(id))
            {
                return m_instanceToTemplateEntityIdMap[id];
            }
                
            return AZStd::nullopt;
        }

        AZ::EntityId Instance::GetEntityId(const EntityAlias& alias)
        {
            if (m_templateToInstanceEntityIdMap.count(alias))
            {
                return m_templateToInstanceEntityIdMap[alias];
            }
                
            return AZ::EntityId();
        }

        AZStd::vector<InstanceAlias> Instance::GetNestedInstanceAliases(TemplateId templateId) const
        {
            AZStd::vector<InstanceAlias> nestedInstanceAliases;
            for (const auto& [nestedInstanceAlias, nestedInstance] : m_nestedInstances)
            {
                if (nestedInstance->GetTemplateId() == templateId)
                {
                    nestedInstanceAliases.push_back(nestedInstanceAlias);
                }
            }
            return nestedInstanceAliases;
        }

        EntityAlias Instance::GenerateEntityAlias()
        {
            return "Entity_" + AZ::Entity::MakeId().ToString();
        }

        InstanceAlias Instance::GenerateInstanceAlias()
        {
            return "Instance_" + AZ::Entity::MakeId().ToString();
        }

        void Instance::InitializeNestedEntities()
        {
            InitializeEntities();

            for (const auto&[instanceAlias, instance] : m_nestedInstances)
            {
                instance->InitializeNestedEntities();
            }
        }

        void Instance::InitializeEntities()
        {
            for (const auto&[entityAlias, entity] : m_entities)
            {
                if (AZ::Entity::State::Constructed == entity->GetState())
                {
                    entity->Init();
                }
            }
        }

        void Instance::ActivateNestedEntities()
        {
            ActivateEntities();

            for (const auto&[instanceAlias, instance] : m_nestedInstances)
            {
                instance->ActivateNestedEntities();
            }
        }

        void Instance::ActivateEntities()
        {
            for (const auto&[entityAlias, entity] : m_entities)
            {
                if(AZ::Entity::State::Init == entity->GetState())
                {
                    entity->Activate();
                }
            }
        }

        InstanceOptionalReference Instance::FindNestedInstance(const InstanceAlias& nestedInstanceAlias)
        {
            auto nestedInstanceIterator = m_nestedInstances.find(nestedInstanceAlias);
            if (nestedInstanceIterator != m_nestedInstances.end())
            {
                return *nestedInstanceIterator->second;
            }
            return AZStd::nullopt;
        }

        InstanceOptionalConstReference Instance::FindNestedInstance(const InstanceAlias& nestedInstanceAlias) const
        {
            auto nestedInstanceIterator = m_nestedInstances.find(nestedInstanceAlias);
            if (nestedInstanceIterator != m_nestedInstances.end())
            {
                return *nestedInstanceIterator->second;
            }
            return AZStd::nullopt;
        }

        InstanceOptionalReference Instance::GetParentInstance()
        {
            if (m_parent != nullptr)
            {
                return *m_parent;
            }
            return AZStd::nullopt;
        }

        InstanceOptionalConstReference Instance::GetParentInstance() const
        {
            if (m_parent != nullptr)
            {
                return *m_parent;
            }
            return AZStd::nullopt;
        }

        const InstanceAlias& Instance::GetInstanceAlias() const
        {
            return m_alias;
        }

        bool Instance::IsParentInstance(const Instance& instance) const
        {
            return &instance == m_parent;
        }

        AZ::EntityId Instance::GetContainerEntityId() const
        {
            return m_containerEntity->GetId();
        }
    }
}
