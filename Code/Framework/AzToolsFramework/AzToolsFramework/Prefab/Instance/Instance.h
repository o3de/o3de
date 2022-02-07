/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AZ
{
    class Entity;
    class ReflectContext;
}

namespace AzToolsFramework
{
    class PrefabEditorEntityOwnershipService;

    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class TemplateInstanceMapperInterface;

        using AliasPath = AZ::IO::Path;
        using AliasPathView = AZ::IO::PathView;
        using EntityAlias = AZStd::string;
        using InstanceAlias = AZStd::string;

        class Instance;
        using EntityAliasOptionalReference = AZStd::optional<AZStd::reference_wrapper<EntityAlias>>;
        using InstanceOptionalReference = AZStd::optional<AZStd::reference_wrapper<Instance>>;
        using InstanceOptionalConstReference = AZStd::optional<AZStd::reference_wrapper<const Instance>>;

        using InstanceSet = AZStd::unordered_set<Instance*>;
        using InstanceSetConstReference = AZStd::optional<AZStd::reference_wrapper<const InstanceSet>>;
        using EntityOptionalReference = AZStd::optional<AZStd::reference_wrapper<AZ::Entity>>;
        using EntityOptionalConstReference = AZStd::optional<AZStd::reference_wrapper<const AZ::Entity>>;

        // A prefab instance is the container for when a Prefab Template is Instantiated.
        class Instance
        {
        public:
            AZ_CLASS_ALLOCATOR(Instance, AZ::SystemAllocator, 0);
            AZ_RTTI(Instance, "{D4219332-A648-4285-9CA6-B7F095987CD3}");

            friend class AzToolsFramework::PrefabEditorEntityOwnershipService;

            using AliasToInstanceMap = AZStd::unordered_map<InstanceAlias, AZStd::unique_ptr<Instance>>;
            using AliasToEntityMap = AZStd::unordered_map<EntityAlias, AZStd::unique_ptr<AZ::Entity>>;
            using EntityList = AZStd::vector<AZ::Entity*>;

            Instance();
            explicit Instance(AZStd::unique_ptr<AZ::Entity> containerEntity);
            explicit Instance(InstanceOptionalReference parent);
            explicit Instance(AZStd::unique_ptr<AZ::Entity> containerEntity, InstanceOptionalReference parent);
            explicit Instance(InstanceAlias alias);
            virtual ~Instance();

            Instance(const Instance& rhs) = delete;
            Instance& operator=(const Instance& rhs) = delete;

            static void Reflect(AZ::ReflectContext* context);

            TemplateId GetTemplateId() const;
            void SetTemplateId(TemplateId templateId);

            const AZ::IO::Path& GetTemplateSourcePath() const;
            void SetTemplateSourcePath(AZ::IO::PathView sourcePath);
            void SetContainerEntityName(AZStd::string_view containerName);

            bool AddEntity(AZ::Entity& entity);
            bool AddEntity(AZ::Entity& entity, EntityAlias entityAlias);
            AZStd::unique_ptr<AZ::Entity> DetachEntity(const AZ::EntityId& entityId);
            void DetachEntities(const AZStd::function<void(AZStd::unique_ptr<AZ::Entity>)>& callback);

            /**
            * Detaches all entities in the instance hierarchy.
            * Includes all direct entities, all nested entities, and all container entities.
            * Note that without container entities the hierarchy that remains cannot be used further without restoring new ones.
            * @param callback A user provided callback that can be used to capture ownership and manipulate the detached entities.
            */
            void DetachAllEntitiesInHierarchy(const AZStd::function<void(AZStd::unique_ptr<AZ::Entity>)>& callback);

            void RemoveNestedEntities(const AZStd::function<bool(const AZStd::unique_ptr<AZ::Entity>&)>& filter);

            void Reset();

            Instance& AddInstance(AZStd::unique_ptr<Instance> instance);
            AZStd::unique_ptr<Instance> DetachNestedInstance(const InstanceAlias& instanceAlias);
            void DetachNestedInstances(const AZStd::function<void(AZStd::unique_ptr<Instance>)>& callback);

            /**
            * Gets the aliases for the entities in the Instance DOM.
            * 
            * @return The list of EntityAliases
            */
            AZStd::vector<EntityAlias> GetEntityAliases();

            /**
            * Gets the ids for the entities in the Instance DOM.  Can recursively trace all nested instances.
            */
            void GetNestedEntityIds(const AZStd::function<bool(AZ::EntityId)>& callback);

            void GetEntityIds(const AZStd::function<bool(AZ::EntityId)>& callback);

            /**
            * Gets the entities in the Instance DOM.  Can recursively trace all nested instances.
            */
            void GetEntities(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback);
            void GetConstEntities(const AZStd::function<bool(const AZ::Entity&)>& callback) const;
            void GetAllEntitiesInHierarchy(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback);
            void GetAllEntitiesInHierarchyConst(const AZStd::function<bool(const AZ::Entity&)>& callback) const;
            void GetNestedInstances(const AZStd::function<void(AZStd::unique_ptr<Instance>&)>& callback);

            /**
            * Gets the alias for a given EnitityId in the Instance DOM.
            *
            * @return entityAlias via optional
            */
            AZStd::optional<AZStd::reference_wrapper<EntityAlias>> GetEntityAlias(const AZ::EntityId& id);

            /**
            * Gets the id for a given EnitityAlias in the Instance DOM.
            *
            * @return entityId, invalid ID if not found
            */
            AZ::EntityId GetEntityId(const EntityAlias& alias);


            /**
            * Gets the aliases of all the nested instances, which are sourced by the template with the given id.
            * 
            * @param templateId The source template id of the nested instances.
            * @return The list of aliases of the nested instances.
            */
            AZStd::vector<InstanceAlias> GetNestedInstanceAliases(TemplateId templateId) const;

            void ActivateContainerEntity();

            InstanceOptionalReference FindNestedInstance(const InstanceAlias& nestedInstanceAlias);

            InstanceOptionalConstReference FindNestedInstance(const InstanceAlias& nestedInstanceAlias) const;

            void SetLinkId(LinkId linkId);

            LinkId GetLinkId() const;

            InstanceOptionalReference GetParentInstance();

            InstanceOptionalConstReference GetParentInstance() const;

            const InstanceAlias& GetInstanceAlias() const;

            bool IsParentInstance(const Instance& instance) const;

            AZ::EntityId GetContainerEntityId() const;

            bool HasContainerEntity() const;

            EntityOptionalReference GetContainerEntity();
            EntityOptionalConstReference GetContainerEntity() const;

            void SetContainerEntity(AZ::Entity& entity);

            AZStd::unique_ptr<AZ::Entity> DetachContainerEntity();

            static EntityAlias GenerateEntityAlias();
            AliasPath GetAbsoluteInstanceAliasPath() const;

            static InstanceAlias GenerateInstanceAlias();

        private:
            static constexpr const char s_aliasPathSeparator = '/';

            Instance(AZStd::unique_ptr<AZ::Entity> containerEntity, InstanceOptionalReference parent, InstanceAlias alias);

            void ClearEntities();

            void RemoveEntities(const AZStd::function<bool(const AZStd::unique_ptr<AZ::Entity>&)>& filter);

            bool GetEntities_Impl(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback);
            bool GetConstEntities_Impl(const AZStd::function<bool(const AZ::Entity&)>& callback) const;
            bool GetAllEntitiesInHierarchy_Impl(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback);
            bool GetAllEntitiesInHierarchyConst_Impl(const AZStd::function<bool(const AZ::Entity&)>& callback) const;

            bool RegisterEntity(const AZ::EntityId& entityId, const EntityAlias& entityAlias);
            AZStd::unique_ptr<AZ::Entity> DetachEntity(const EntityAlias& entityAlias);

            // Provide access to private data members in the serializer
            friend class JsonInstanceSerializer;
            friend class InstanceEntityIdMapper;

            // A map of loose entities that the prefab instance directly owns.
            AliasToEntityMap m_entities;

            // A map of prefab instance pointers that this prefab instance owns.
            AliasToInstanceMap m_nestedInstances;

            // The entity representing this Instance as a container in the entity hierarchy.
            AZStd::unique_ptr<AZ::Entity> m_containerEntity;
            
            // The id of the link that connects the template of this instance to its source template.
            // This is not unique per instance. It's unique per link. It is invalid for instances that aren't nested under other instances.
            LinkId m_linkId = InvalidLinkId;

            // Maps to translate alias' to ids and back for serialization
            AZStd::unordered_map<EntityAlias, AZ::EntityId> m_templateToInstanceEntityIdMap;
            AZStd::unordered_map<AZ::EntityId, EntityAlias> m_instanceToTemplateEntityIdMap;

            // Alias this instance goes by under its parent if nested
            InstanceAlias m_alias;

            // The source path of the template this instance represents
            AZ::IO::Path m_templateSourcePath;

            // The unique ID of the template this Instance belongs to.
            TemplateId m_templateId = InvalidTemplateId;

            // Pointer to the parent instance if nested
            Instance* m_parent = nullptr;

            // Interface for registering owned entities for external queries
            InstanceEntityMapperInterface* m_instanceEntityMapper = nullptr;

            // Interface for registering the Instance itself for external queries.
            TemplateInstanceMapperInterface* m_templateInstanceMapper = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
