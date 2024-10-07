/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/Entity.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapperInterface.h>

#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        Instance::Instance()
            : Instance(AZStd::make_unique<AZ::Entity>())
        {
        }

        Instance::Instance(AZStd::unique_ptr<AZ::Entity> containerEntity)
            : Instance(AZStd::move(containerEntity), AZStd::nullopt, GenerateInstanceAlias())
        {
        }

        Instance::Instance(InstanceOptionalReference parent)
            : Instance(nullptr, parent, GenerateInstanceAlias())
        {
        }

        Instance::Instance(InstanceOptionalReference parent, InstanceAlias alias)
            : Instance(nullptr, parent, alias)
        {
        }

        Instance::Instance(InstanceOptionalReference parent, InstanceAlias alias, EntityIdInstanceRelationship entityIdInstanceRelationship)
            : Instance(nullptr, parent, alias, entityIdInstanceRelationship)
        {
        }

        Instance::Instance(InstanceAlias alias)
            : Instance(nullptr, AZStd::nullopt, AZStd::move(alias))
        {
        }

        Instance::Instance(EntityIdInstanceRelationship entityIdInstanceRelationship)
            : Instance(nullptr, AZStd::nullopt, GenerateInstanceAlias(), entityIdInstanceRelationship)
        {
        }

        Instance::Instance(InstanceAlias alias, EntityIdInstanceRelationship entityIdInstanceRelationship)
            : Instance(nullptr, AZStd::nullopt, AZStd::move(alias), entityIdInstanceRelationship)
        {
        }

        Instance::Instance(AZStd::unique_ptr<AZ::Entity> containerEntity, InstanceOptionalReference parent)
            : Instance(AZStd::move(containerEntity), parent, GenerateInstanceAlias())
        {
        }

        Instance::Instance(AZStd::unique_ptr<AZ::Entity> containerEntity, InstanceOptionalReference parent, InstanceAlias alias, EntityIdInstanceRelationship entityIdInstanceRelationship)
            : m_parent(parent.has_value() ? &parent->get() : nullptr)
            , m_alias(AZStd::move(alias))
            , m_containerEntity(containerEntity ? AZStd::move(containerEntity) : AZStd::make_unique<AZ::Entity>())
            , m_entityIdInstanceRelationship(entityIdInstanceRelationship)
            , m_instanceEntityMapper(AZ::Interface<InstanceEntityMapperInterface>::Get())
            , m_templateInstanceMapper(AZ::Interface<TemplateInstanceMapperInterface>::Get())
        {
            AZ_Assert(m_instanceEntityMapper,
                "Instance Entity Mapper Interface could not be found. "
                "It is a requirement for the Prefab Instance class. "
                "Check that it is being correctly initialized.");

            AZ_Assert(m_templateInstanceMapper,
                "Template Instance Mapper Interface could not be found. "
                "It is a requirement for the Prefab Instance class. "
                "Check that it is being correctly initialized.");

            m_isDomCachingEnabled = s_DomCachingEnabledDefault;

            if (parent)
            {
                AliasPath absoluteInstancePath = m_parent->GetAbsoluteInstanceAliasPath();
                absoluteInstancePath.Append(m_alias);
                absoluteInstancePath.Append(PrefabDomUtils::ContainerEntityName);

                AZ::EntityId newContainerEntityId = InstanceEntityIdMapper::GenerateEntityIdForAliasPath(absoluteInstancePath);
                m_containerEntity->SetId(newContainerEntityId);
            }

            RegisterEntity(m_containerEntity->GetId(), PrefabDomUtils::ContainerEntityName);
        }

        Instance::~Instance()
        {
            // when destroying an instance in its destructor, do not re-create any 
            // new entities or instances (Avoids a leak)
            Reset(false);
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

        TemplateId Instance::GetTemplateId() const
        {
            return m_templateId;
        }

        void Instance::SetTemplateId(TemplateId templateId)
        {
            // If we aren't changing the template Id, there's no need to unregister / re-register
            if (templateId == m_templateId)
            {
                return;
            }

            m_templateInstanceMapper->UnregisterInstance(*this);

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
            m_linkId = linkId;
        }

        LinkId Instance::GetLinkId() const
        {
            return m_linkId;
        }

        const AZ::IO::Path& Instance::GetTemplateSourcePath() const
        {
            return m_templateSourcePath;
        }

        void Instance::SetTemplateSourcePath(AZ::IO::Path sourcePath)
        {
            m_templateSourcePath = AZStd::move(sourcePath);
        }

        void Instance::SetContainerEntityName(AZStd::string containerName)
        {
            m_containerEntity->SetName(AZStd::move(containerName));
        }

        bool Instance::AddEntity(AZ::Entity& entity)
        {
            return AddEntity(entity, GenerateEntityAlias());
        }

        bool Instance::AddEntity(AZStd::unique_ptr<AZ::Entity>&& entity)
        {
            return AddEntity(AZStd::move(entity), GenerateEntityAlias());
        }

        bool Instance::AddEntity(AZ::Entity& entity, EntityAlias entityAlias)
        {
            return
                RegisterEntity(entity.GetId(), entityAlias) && 
                m_entities.emplace(AZStd::move(entityAlias), &entity).second;
        }

        bool Instance::AddEntity(AZStd::unique_ptr<AZ::Entity>&& entity, EntityAlias entityAlias)
        {
            return
                RegisterEntity(entity->GetId(), entityAlias) &&
                m_entities.emplace(AZStd::move(entityAlias), AZStd::move(entity)).second;
        }

        AZStd::unique_ptr<AZ::Entity> Instance::DetachEntity(const AZ::EntityId& entityId)
        {
            EntityAlias entityAliasToRemove;
            if (auto instanceToTemplateEntityIdIterator = m_instanceToTemplateEntityIdMap.find(entityId);
                instanceToTemplateEntityIdIterator != m_instanceToTemplateEntityIdMap.end())
            {
                entityAliasToRemove = instanceToTemplateEntityIdIterator->second;
                if (m_entityIdInstanceRelationship == EntityIdInstanceRelationship::OneToOne)
                {
                    if (!UnregisterEntity(entityId))
                    {
                        AZ_Error(
                            "Prefab",
                            false,
                            "An owning instance couldn't be found corresponding to the entity requested to be detached");
                        return nullptr;
                    }
                }
                else
                {
                    [[maybe_unused]] bool isEntityRemoved =
                        (m_templateToInstanceEntityIdMap.erase(entityAliasToRemove) > 0) && (m_instanceToTemplateEntityIdMap.erase(entityId) > 0);
                    AZ_Assert(
                        isEntityRemoved,
                        "Prefab - Failed to remove entity with id %s with a Prefab Instance derived from source asset %s "
                        "This happens when the entity is not correctly removed from all the prefab system entity maps.",
                        entityId.ToString().c_str(),
                        m_templateSourcePath.c_str());
                }
            }

            return DetachEntityHelper(entityAliasToRemove);
        }

        AZStd::unique_ptr<AZ::Entity> Instance::DetachEntityHelper(const EntityAlias& entityAlias)
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

        void Instance::DetachAllEntitiesInHierarchy(const AZStd::function<void(AZStd::unique_ptr<AZ::Entity>)>& callback)
        {
            callback(AZStd::move(DetachContainerEntity()));
            DetachEntities(callback);

            for (const auto& [instanceAlias, instance] : m_nestedInstances)
            {
                instance->DetachAllEntitiesInHierarchy(callback);
            }
        }

        void Instance::DetachEntities(const AZStd::function<void(AZStd::unique_ptr<AZ::Entity>)>& callback)
        {
            for (auto&& [entityAlias, entity] : m_entities)
            {
                if (m_entityIdInstanceRelationship == EntityIdInstanceRelationship::OneToOne)
                {
                    m_instanceEntityMapper->UnregisterEntity(entity->GetId());
                }
                m_templateToInstanceEntityIdMap.erase(entityAlias);
                m_instanceToTemplateEntityIdMap.erase(entity->GetId());

                callback(AZStd::move(entity));
            }

            m_entities.clear();
        }

        bool Instance::DestroyEntity(const AZ::EntityId& entityId)
        {
            AZStd::unique_ptr<AZ::Entity> detachedEntity = DetachEntity(entityId);
            if (detachedEntity)
            {
                detachedEntity.reset();
                return true;
            }
            return false;
        }

        AZStd::unique_ptr<AZ::Entity> Instance::ReplaceEntity(AZStd::unique_ptr<AZ::Entity>&& entity, EntityAliasView alias)
        {
            AZStd::unique_ptr<AZ::Entity> result;
            auto it = m_entities.find(alias);
            if (it != m_entities.end())
            {
                // Swap entity ids as these need to remain stable
                AZ::EntityId originalId = it->second->GetId();
                it->second->SetId(entity->GetId());
                entity->SetId(originalId);

                result = AZStd::move(it->second);
                it->second = AZStd::move(entity);
            }
            return result;
        }

        void Instance::RemoveEntitiesInHierarchy(
            const AZStd::function<bool(const AZStd::unique_ptr<AZ::Entity>&)>& filter)
        {
            RemoveEntities(filter);

            for (const auto& [instanceAlias, instance] : m_nestedInstances)
            {
                instance->RemoveEntitiesInHierarchy(filter);
            }
        }

        void Instance::Reset(bool forReuse)
        {
            m_templateInstanceMapper->UnregisterInstance(*this);

            ClearEntities();

            m_nestedInstances.clear();
            m_cachedInstanceDom = PrefabDom();

            // forReuse will be true unless we're in destructor
            // If we're in a destructor, there is no point in creating a new entity for the container
            // or registering a new one in the map, otherwise we would leak elements in the map.
            if (forReuse)
            {
                m_containerEntity.reset(aznew AZ::Entity());
                RegisterEntity(m_containerEntity->GetId(), GenerateEntityAlias());
            }
            else
            {
                m_containerEntity.reset();
            }
        }

        void Instance::RemoveEntities(
            const AZStd::function<bool(const AZStd::unique_ptr<AZ::Entity>&)>& filter)
        {
            // It is possible for the container entity to be the one being asked to be removed
            // This can happen if the prefab is being exported to a spawnable, and the container
            // entity is set to "editor only" or even "not exported".  This is what allows
            // prefab container entities to be set to Editor Only without causing a crash in the Editor.
            if ((m_containerEntity)&&(m_entityIdInstanceRelationship == EntityIdInstanceRelationship::OneToOne))
            {
                if (filter(m_containerEntity)) // asked to filter out the container
                {
                    DestroyContainerEntity();
                }
            }
            AZStd::erase_if(m_entities,
                [this, &filter](const auto& item)
                {
                    const auto& [entityAlias, entity] = item;
                    const bool shouldRemove = filter(entity);
                    if (shouldRemove)
                    {
                        if (m_entityIdInstanceRelationship == EntityIdInstanceRelationship::OneToOne)
                        {
                            m_instanceEntityMapper->UnregisterEntity(entity->GetId());
                        }
                        m_templateToInstanceEntityIdMap.erase(entityAlias);
                        m_instanceToTemplateEntityIdMap.erase(entity->GetId());
                    }
                    return shouldRemove;
                }
            );
        }

        void Instance::ClearEntities()
        {
            if (m_entityIdInstanceRelationship == EntityIdInstanceRelationship::OneToOne)
            {
                if (m_containerEntity)
                {
                    m_instanceEntityMapper->UnregisterEntity(m_containerEntity->GetId());
                    m_containerEntity.reset();
                }

                for (const auto& [entityAlias, entity] : m_entities)
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
            }

            // Destroy the entities *before* clearing the lookup maps so that any lookups triggered during an entity's destructor
            // are still valid.
            m_entities.clear();
            m_instanceToTemplateEntityIdMap.clear();
            m_templateToInstanceEntityIdMap.clear();
        }

        bool Instance::RegisterEntity(const AZ::EntityId& entityId, const EntityAlias& entityAlias)
        {
            
            if ((m_entityIdInstanceRelationship ==
                    EntityIdInstanceRelationship::OneToOne) && !(m_instanceEntityMapper->RegisterEntityToInstance(entityId, *this)))
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

        bool Instance::UnregisterEntity(AZ::EntityId entityId)
        {
            EntityAlias entityAliasToRemove;
            auto instanceToTemplateEntityIdIterator = m_instanceToTemplateEntityIdMap.find(entityId);
            if (instanceToTemplateEntityIdIterator != m_instanceToTemplateEntityIdMap.end())
            {
                entityAliasToRemove = instanceToTemplateEntityIdIterator->second;
                auto templateToInstanceIterator = m_templateToInstanceEntityIdMap.find(entityAliasToRemove);
                InstanceOptionalReference owningInstance = m_instanceEntityMapper->FindOwningInstance(entityId);
                if (templateToInstanceIterator != m_templateToInstanceEntityIdMap.end() && owningInstance.has_value() && &(owningInstance->get()) == this)
                {
                    return (
                        m_instanceEntityMapper->UnregisterEntity(entityId) && m_templateToInstanceEntityIdMap.erase(entityAliasToRemove) &&
                        m_instanceToTemplateEntityIdMap.erase(entityId));
                }
            }
            AZ_Warning(
                "Prefab", false,
                "Failed to remove entity with id %s with a Prefab Instance derived from source asset %s "
                "This happens when the entity cannot be found in the relevant prefab system entity maps.",
                entityId.ToString().c_str(), m_templateSourcePath.c_str());
            return false;
        }

        Instance& Instance::AddInstance(AZStd::unique_ptr<Instance> instance)
        {
            AZ_Assert(instance.get(), "instance argument is nullptr");

            if (instance->GetInstanceAlias().empty())
            {
                instance->m_alias = GenerateInstanceAlias();
            }

            AZ_Assert(
                m_nestedInstances.find(instance->GetInstanceAlias()) == m_nestedInstances.end(),
                "InstanceAlias' unique id collision, this should never happen.");

            instance->m_parent = this;
            auto& alias = instance->GetInstanceAlias();
            return *(m_nestedInstances[alias] = AZStd::move(instance));
        }

        void Instance::DetachNestedInstances(const AZStd::function<void(AZStd::unique_ptr<Instance>)>& callback)
        {
            for (auto&& [instanceAlias, instance] : m_nestedInstances)
            {
                instance->m_parent = nullptr;
                callback(AZStd::move(instance));
            }
            m_nestedInstances.clear();
        }

        AZStd::unique_ptr<Instance> Instance::DetachNestedInstance(const InstanceAlias& instanceAlias)
        {
            AZStd::unique_ptr<Instance> removedNestedInstance;
            auto&& nestedInstanceIterator = m_nestedInstances.find(instanceAlias);
            if (nestedInstanceIterator != m_nestedInstances.end())
            {
                removedNestedInstance = AZStd::move(nestedInstanceIterator->second);

                removedNestedInstance->m_parent = nullptr;

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

        size_t Instance::GetEntityAliasCount() const
        {
            return m_entities.size();
        }

        void Instance::GetEntityIds(const AZStd::function<bool(AZ::EntityId)>& callback) const
        {
            for (auto&&[entityAlias, entityId] : m_templateToInstanceEntityIdMap)
            {
                if (!callback(entityId))
                {
                    break;
                }
            }
        }

        void Instance::GetEntityIdToAlias(const AZStd::function<bool(AZ::EntityId, EntityAliasView)>& callback) const
        {
            for (auto&& [entityAlias, entityId] : m_templateToInstanceEntityIdMap)
            {
                if (!callback(entityId, entityAlias))
                {
                    break;
                }
            }
        }

        bool Instance::GetEntities_Impl(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback)
        {
            for (auto& [entityAlias, entity] : m_entities)
            {
                if (!entity)
                {
                    continue;
                }

                if (!callback(entity))
                {
                    return false;
                }
            }

            return true;
        }

        bool Instance::GetConstEntities_Impl(const AZStd::function<bool(const AZ::Entity&)>& callback) const
        {
            for (const auto& [entityAlias, entity] : m_entities)
            {
                if (!entity)
                {
                    continue;
                }

                if (!callback(*entity))
                {
                    return false;
                }
            }

            return true;
        }

        bool Instance::GetAllEntitiesInHierarchy_Impl(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback)
        {
            if (HasContainerEntity())
            {
                if (!callback(m_containerEntity))
                {
                    return false;
                }
            }

            if (!GetEntities_Impl(callback))
            {
                return false;
            }

            for (auto& [instanceAlias, instance] : m_nestedInstances)
            {
                if (!instance->GetAllEntitiesInHierarchy_Impl(callback))
                {
                    return false;
                }
            }

            return true;
        }

        bool Instance::GetAllEntitiesInHierarchyConst_Impl(const AZStd::function<bool(const AZ::Entity&)>& callback) const
        {
            if (HasContainerEntity())
            {
                if (!callback(*m_containerEntity))
                {
                    return false;
                }
            }

            if (!GetConstEntities_Impl(callback))
            {
                return false;
            }

            for (const auto& [instanceAlias, instance] : m_nestedInstances)
            {
                if (!instance->GetAllEntitiesInHierarchyConst_Impl(callback))
                {
                    return false;
                }
            }

            return true;
        }

        void Instance::GetEntities(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback)
        {
            GetEntities_Impl(callback);
        }

        void Instance::GetConstEntities(const AZStd::function<bool(const AZ::Entity&)>& callback) const
        {
            GetConstEntities_Impl(callback);
        }

        void Instance::GetAllEntityIdsInHierarchy(const AZStd::function<bool(AZ::EntityId)>& callback) const
        {
            GetEntityIds(callback);

            for (auto&& [instanceAlias, instance] : m_nestedInstances)
            {
                instance->GetAllEntityIdsInHierarchy(callback);
            }
        }

        void Instance::GetAllEntitiesInHierarchy(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback)
        {
            GetAllEntitiesInHierarchy_Impl(callback);
        }

        void Instance::GetAllEntitiesInHierarchyConst(const AZStd::function<bool(const AZ::Entity&)>& callback) const
        {
            GetAllEntitiesInHierarchyConst_Impl(callback);
        }

        void Instance::GetNestedInstances(const AZStd::function<void(AZStd::unique_ptr<Instance>&)>& callback)
        {
            for (auto& [instanceAlias, instance] : m_nestedInstances)
            {
                callback(instance);
            }
        }

        EntityAliasOptionalReference Instance::GetEntityAlias(AZ::EntityId id)
        {
            auto it = m_instanceToTemplateEntityIdMap.find(id);
            return it != m_instanceToTemplateEntityIdMap.end() ? EntityAliasOptionalReference(it->second)
                                                               : EntityAliasOptionalReference(AZStd::nullopt);
        }

        EntityAliasView Instance::GetEntityAlias(AZ::EntityId id) const
        {
            auto it = m_instanceToTemplateEntityIdMap.find(id);
            return it != m_instanceToTemplateEntityIdMap.end() ? EntityAliasView(it->second) : EntityAliasView();
        }

        AZStd::pair<Instance*, EntityAliasView> Instance::FindInstanceAndAlias(AZ::EntityId entity)
        {
            auto it = m_instanceToTemplateEntityIdMap.find(entity);
            if (it != m_instanceToTemplateEntityIdMap.end())
            {
                return AZStd::pair<Instance*, EntityAliasView>(this, it->second);
            }
            else
            {
                for (auto&& [_, instance] : m_nestedInstances)
                {
                    AZStd::pair<Instance*, EntityAliasView> next = instance->FindInstanceAndAlias(entity);
                    if (next.first != nullptr)
                    {
                        return next;
                    }
                }
            }
            return AZStd::pair<Instance*, EntityAliasView>(nullptr, "");
        }

        AZStd::pair<const Instance*, EntityAliasView> Instance::FindInstanceAndAlias(AZ::EntityId entity) const
        {
            return const_cast<Instance*>(this)->FindInstanceAndAlias(entity);
        }

        EntityOptionalReference Instance::GetEntity(const EntityAlias& alias)
        {
            auto it = m_entities.find(alias);
            return it != m_entities.end() ? EntityOptionalReference(*it->second) : EntityOptionalReference(AZStd::nullopt);
        }

        EntityOptionalConstReference Instance::GetEntity(const EntityAlias& alias) const
        {
            auto it = m_entities.find(alias);
            return it != m_entities.end() ? EntityOptionalConstReference(*it->second) : EntityOptionalConstReference(AZStd::nullopt);
        }

        AZ::EntityId Instance::GetEntityId(const EntityAlias& alias) const
        {
            auto it = m_templateToInstanceEntityIdMap.find(alias);
            return it != m_templateToInstanceEntityIdMap.end() ? it->second : AZ::EntityId();
        }

        AZ::EntityId Instance::GetEntityIdFromAliasPath(AliasPathView relativeAliasPath) const
        {
            return GetInstanceAndEntityIdFromAliasPath(relativeAliasPath).second;
        }

        AZStd::pair<Instance*, AZ::EntityId> Instance::GetInstanceAndEntityIdFromAliasPath(AliasPathView relativeAliasPath)
        {
            Instance* instance = this;
            AliasPathView path = relativeAliasPath.ParentPath();
            for (auto it : path)
            {
                InstanceOptionalReference child = instance->FindNestedInstance(it.Native());
                if (child.has_value())
                {
                    instance = &(child->get());
                }
                else
                {
                    return AZStd::pair<Instance*, AZ::EntityId>(nullptr, AZ::EntityId());
                }
            }

            return AZStd::pair<Instance*, AZ::EntityId>(instance, instance->GetEntityId(relativeAliasPath.Filename().Native()));
        }

        AZStd::pair<const Instance*, AZ::EntityId> Instance::GetInstanceAndEntityIdFromAliasPath(AliasPathView relativeAliasPath) const
        {
            const Instance* instance = this;
            AliasPathView path = relativeAliasPath.ParentPath();
            for (auto it : path)
            {
                InstanceOptionalConstReference child = instance->FindNestedInstance(it.Native());
                if (child.has_value())
                {
                    instance = &(child->get());
                }
                else
                {
                    return AZStd::pair<const Instance*, AZ::EntityId>(nullptr, AZ::EntityId());
                }
            }

            return AZStd::pair<const Instance*, AZ::EntityId>(instance, instance->GetEntityId(relativeAliasPath.Filename().Native()));
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

        const InstanceAlias& Instance::GetInstanceAlias() const
        {
            return m_alias;
        }

        AliasPath Instance::GetAbsoluteInstanceAliasPath() const
        {
            // Reset the path using our preferred separator
            AliasPath aliasPathResult = AliasPath(s_aliasPathSeparator);
            const Instance* currentInstance = this;

            // If no parent instance we are a root instance and our absolute path is empty
            AZStd::vector<const Instance*> pathOfInstances;

            while (currentInstance->m_parent)
            {
                pathOfInstances.emplace_back(currentInstance);
                currentInstance = currentInstance->m_parent;
            }

            pathOfInstances.emplace_back(currentInstance);

            for (auto instanceIter = pathOfInstances.rbegin(); instanceIter != pathOfInstances.rend(); ++instanceIter)
            {
                aliasPathResult.Append((*instanceIter)->m_alias);
            }

            return aliasPathResult;
        }

        AliasPath Instance::GetAliasPathRelativeToInstance(const AZ::EntityId& entity) const
        {
            AliasPath result = AliasPath(s_aliasPathSeparator);
            auto&& [instance, alias] = FindInstanceAndAlias(entity);
            if (instance)
            {
                AZStd::vector<const Instance*> instanceChain;
                
                while (instance && instance != this)
                {
                    instanceChain.push_back(instance);
                    instance = instance->m_parent;
                }

                for (auto it = instanceChain.rbegin(); it != instanceChain.rend(); ++it)
                {
                    result.Append((*it)->m_alias);
                }
                return result.Append(alias);
            }
            else
            {
                return result;
            }
        }

        EntityAlias Instance::GenerateEntityAlias()
        {
            return AZStd::string::format("Entity_%s", AZ::Entity::MakeId().ToString().c_str());
        }

        InstanceAlias Instance::GenerateInstanceAlias()
        {
            return AZStd::string::format("Instance_%s", AZ::Entity::MakeId().ToString().c_str());
        }

        void Instance::ActivateContainerEntity()
        {
            AZ_Assert(m_containerEntity, "Container entity of instance is null.");

            if (AZ::Entity::State::Constructed == m_containerEntity->GetState())
            {
                m_containerEntity->Init();
            }

            if (AZ::Entity::State::Init == m_containerEntity->GetState())
            {
                m_containerEntity->Activate();
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

        bool Instance::HasParentInstance() const
        {
            return m_parent != nullptr;
        }

        bool Instance::IsParentInstance(const Instance& instance) const
        {
            return &instance == m_parent;
        }

        AZ::EntityId Instance::GetContainerEntityId() const
        {
            return m_containerEntity ? m_containerEntity->GetId() : AZ::EntityId();
        }

        bool Instance::HasContainerEntity() const
        {
            return m_containerEntity.get() != nullptr;
        }

        EntityOptionalReference Instance::GetContainerEntity()
        {
            if (m_containerEntity)
            {
                return *m_containerEntity;
            }
            return AZStd::nullopt;
        }

        EntityOptionalConstReference Instance::GetContainerEntity() const
        {
            if (m_containerEntity)
            {
                return *m_containerEntity;
            }
            return AZStd::nullopt;
        }

        void Instance::SetContainerEntity(AZ::Entity& entity)
        {
            m_containerEntity.reset(&entity);
        }

        AZStd::unique_ptr<AZ::Entity> Instance::DetachContainerEntity()
        {
            if (m_entityIdInstanceRelationship == EntityIdInstanceRelationship::OneToOne && m_containerEntity)
            {
                UnregisterEntity(m_containerEntity->GetId());
            }
            return AZStd::move(m_containerEntity);
        }

        bool Instance::DestroyContainerEntity()
        {
            AZStd::unique_ptr<AZ::Entity> detachedEntity = DetachContainerEntity();
            if (detachedEntity)
            {
                detachedEntity.reset();
                return true;
            }
            return false;
        }

        PrefabDomConstReference Instance::GetCachedInstanceDom() const
        {
            if (m_cachedInstanceDom.IsNull())
            {
                return AZStd::nullopt;
            }
            return m_cachedInstanceDom;
        }

        PrefabDomReference Instance::GetCachedInstanceDom()
        {
            if (m_cachedInstanceDom.IsNull())
            {
                return AZStd::nullopt;
            }
            return m_cachedInstanceDom;
        }

        void Instance::SetCachedInstanceDom(PrefabDomValueConstReference instanceDom)
        {
            // force a flush of memory by clearing first if cache isn't empty.
            if (!m_cachedInstanceDom.IsNull())
            {
                m_cachedInstanceDom = PrefabDom(); 
            }

            if (m_isDomCachingEnabled)
            {
                m_cachedInstanceDom.CopyFrom(instanceDom->get(), m_cachedInstanceDom.GetAllocator());
            }
        }

        void Instance::EnableDomCaching(bool enableDomCaching)
        {
            m_isDomCachingEnabled = enableDomCaching;
        }
    }
} // namespace AzToolsFramework
