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
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
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
        using EntityAliasView = AZStd::string_view;
        using InstanceAlias = AZStd::string;

        class Instance;
        using EntityAliasOptionalReference = AZStd::optional<AZStd::reference_wrapper<EntityAlias>>;
        using InstanceOptionalReference = AZStd::optional<AZStd::reference_wrapper<Instance>>;
        using InstanceOptionalConstReference = AZStd::optional<AZStd::reference_wrapper<const Instance>>;

        using InstanceSet = AZStd::unordered_set<Instance*>;
        using InstanceSetConstReference = AZStd::optional<AZStd::reference_wrapper<const InstanceSet>>;
        using EntityOptionalReference = AZStd::optional<AZStd::reference_wrapper<AZ::Entity>>;
        using EntityOptionalConstReference = AZStd::optional<AZStd::reference_wrapper<const AZ::Entity>>;

        //! Enum class that represents entity id and instance relationship.
        enum class EntityIdInstanceRelationship : uint8_t
        {
            OneToOne,
            OneToMany
        };

        //! Class that represents a prefab instance instantiated from a prefab template.
        class Instance
        {
        public:
            AZ_CLASS_ALLOCATOR(Instance, AZ::SystemAllocator, 0);
            AZ_RTTI(Instance, "{D4219332-A648-4285-9CA6-B7F095987CD3}");

            friend class AzToolsFramework::PrefabEditorEntityOwnershipService;

            using AliasToInstanceMap = AZStd::unordered_map<InstanceAlias, AZStd::unique_ptr<Instance>>;
            using AliasToEntityMap = AZStd::unordered_map<EntityAlias, AZStd::unique_ptr<AZ::Entity>>;

            //! Construtors.
            //! @{
            Instance();
            explicit Instance(AZStd::unique_ptr<AZ::Entity> containerEntity);
            explicit Instance(InstanceOptionalReference parent);
            Instance(InstanceOptionalReference parent, InstanceAlias alias);
            Instance(InstanceOptionalReference parent, InstanceAlias alias, EntityIdInstanceRelationship entityIdInstanceRelationship);
            Instance(AZStd::unique_ptr<AZ::Entity> containerEntity, InstanceOptionalReference parent);
            explicit Instance(InstanceAlias alias);
            explicit Instance(EntityIdInstanceRelationship entityIdInstanceRelationship);
            Instance(InstanceAlias alias, EntityIdInstanceRelationship entityIdInstanceRelationship);
            //! @}

            //! Destructor.
            virtual ~Instance();

            //! Disables copy constructor and copy assignment.
            //! @{
            Instance(const Instance& rhs) = delete;
            Instance& operator=(const Instance& rhs) = delete;
            //! @}

            //! AzCore Reflection.
            //! @param context reflection context.
            static void Reflect(AZ::ReflectContext* context);

            //! Gets template id.
            //! @return Template id of the instance.
            TemplateId GetTemplateId() const;

            //! Sets template id.
            //! @param templateId Template id to be set.
            void SetTemplateId(TemplateId templateId);

            //! Gets template source path.
            //! @return Template source path of the instance.
            const AZ::IO::Path& GetTemplateSourcePath() const;

            //! Sets template source path.
            //! @param sourcePath Template source path to be set.
            void SetTemplateSourcePath(AZ::IO::Path sourcePath);

            //! Sets container entity name.
            //! @param containerName New container entity name to be set.
            void SetContainerEntityName(AZStd::string containerName);

            //! Adds entity to the instance.
            //! It registers the entity id and moves the entity ownership to the instance.
            //! @{
            bool AddEntity(AZ::Entity& entity);
            bool AddEntity(AZStd::unique_ptr<AZ::Entity>&& entity);
            bool AddEntity(AZ::Entity& entity, EntityAlias entityAlias);
            bool AddEntity(AZStd::unique_ptr<AZ::Entity>&& entity, EntityAlias entityAlias);
            //! @}

            //! Detaches an entity from the instance.
            //! It unregisters the entity id and moves the entity ownership to the caller.
            //! @param entityId The entity id to be detached.
            //! @return Unique pointer to the entity.
            AZStd::unique_ptr<AZ::Entity> DetachEntity(const AZ::EntityId& entityId);

            //! Detaches entities from the instance.
            //! It unregisters the entity ids and moves the entity ownerships to the callback provided by the caller.
            //! @param callback A user provided callback that can be used to capture ownership and manipulate the detached entities.
            void DetachEntities(const AZStd::function<void(AZStd::unique_ptr<AZ::Entity>)>& callback);

            //! Detaches all entities in the instance hierarchy.
            //! Includes all direct entities, all nested entities, and all container entities.
            //! Note that without container entities the hierarchy that remains cannot be used further without restoring new ones.
            //! @param callback A user provided callback that can be used to capture ownership and manipulate the detached entities.
            void DetachAllEntitiesInHierarchy(const AZStd::function<void(AZStd::unique_ptr<AZ::Entity>)>& callback);

            //! Detaches and deletes an entity from the instance.
            //! @param entityId The entity id to be detached and destroyed.
            //! @return True if the entity was detached and destroyed, false otherwise.
            bool DestroyEntity(const AZ::EntityId& entityId);

            //! Replaces the entity stored under the provided alias with a new one.
            //! @param entity The new entity.
            //! @param alias The alias of the entity that will be replaced.
            //! @return The original entity or a nullptr if not found.
            AZStd::unique_ptr<AZ::Entity> ReplaceEntity(AZStd::unique_ptr<AZ::Entity>&& entity, EntityAliasView alias);

            //! Detaches entities in the instance hierarchy based on the provided filter.
            //! @param filter A user provided filter used to decide whether an entity should be detached. Returns true to detach.
            void RemoveEntitiesInHierarchy(const AZStd::function<bool(const AZStd::unique_ptr<AZ::Entity>&)>& filter);

            //! Adds a new nested instance.
            //! @param instance The new nested instance.
            //! @return Reference to the new nested instance.
            Instance& AddInstance(AZStd::unique_ptr<Instance> instance);

            //! Detaches a nested instance from the instance.
            //! It removes the nested instance and moves the instance ownership to the caller.
            //! @param entityId The entity id to be detached.
            //! @return Unique pointer to the entity.
            AZStd::unique_ptr<Instance> DetachNestedInstance(const InstanceAlias& instanceAlias);

            //! Detaches entities from the instance.
            //! It removes nested instances and moves the instance ownerships to the callback provided by the caller.
            //! @param callback A user provided callback that can be used to capture ownership and manipulate the detached instances.
            void DetachNestedInstances(const AZStd::function<void(AZStd::unique_ptr<Instance>)>& callback);

            //! Resets the instance to an initial state.
            //! It unregisters the instance, entities and nested instances.
            void Reset();

            //! Gets the aliases for the entities in the Instance DOM.
            //! @return The list of EntityAliases.
            AZStd::vector<EntityAlias> GetEntityAliases();

            //! Gets the entity alias count in the Instance DOM.
            //! @return The entity alias count.
            size_t GetEntityAliasCount() const;

            //! Gets the entities or entity ids in the Instance DOM.
            //! A callback can be provided to manipulates the entities.
            //! Note: These are non-recursive operations.
            //! @{
            void GetEntityIds(const AZStd::function<bool(AZ::EntityId)>& callback) const;
            void GetEntityIdToAlias(const AZStd::function<bool(AZ::EntityId, EntityAliasView)>& callback) const;
            void GetEntities(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback);
            void GetConstEntities(const AZStd::function<bool(const AZ::Entity&)>& callback) const;
            //! @}

            //! Gets the entity ids for all entities in hierarchy in the Instance DOM.
            //! A callback can be provided to manipulates the entities.
            //! Note: These are recursive operations that trace all nested instances.
            //! @{
            void GetAllEntityIdsInHierarchy(const AZStd::function<bool(AZ::EntityId)>& callback) const;
            void GetAllEntitiesInHierarchy(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback);
            void GetAllEntitiesInHierarchyConst(const AZStd::function<bool(const AZ::Entity&)>& callback) const;
            //! @}

            //! Gets the nested instances in the instance DOM.
            //! Note: This is a non-recursive operation.
            //! @param callback A user provided callback that manipulates the nested instances.
            void GetNestedInstances(const AZStd::function<void(AZStd::unique_ptr<Instance>&)>& callback);

            //! Gets the entity alias for a given entity id in the Instance DOM.
            //! @{
            EntityAliasOptionalReference GetEntityAlias(AZ::EntityId id);
            EntityAliasView GetEntityAlias(AZ::EntityId id) const;
            //! @}

            //! Searches for the entity in this instance and its nested instances.
            //! If the entity isn't found then the returned instance will be null and the alias will be empty.
            //! @{
            AZStd::pair<Instance*, EntityAliasView> FindInstanceAndAlias(AZ::EntityId entity);
            AZStd::pair<const Instance*, EntityAliasView> FindInstanceAndAlias(AZ::EntityId entity) const;
            //! @}

            //! Gets entity by a user provided entity alias.
            //! @{
            EntityOptionalReference GetEntity(const EntityAlias& alias);
            EntityOptionalConstReference GetEntity(const EntityAlias& alias) const;
            //! @}

            //! Gets the id for a given EnitityAlias in the Instance DOM.
            //! @param alias Entity alias used to retrieve the entity id.
            //! @return entityId, invalid ID if not found.
            AZ::EntityId GetEntityId(const EntityAlias& alias) const;

            //! Retrieves the entity id from an alias path that's relative to this instance.
            //! @param relativeAliasPath Entity alias path relative to this instance.
            //! @return entityId, invalid ID if not found.
            AZ::EntityId GetEntityIdFromAliasPath(AliasPathView relativeAliasPath) const;

            //! Retrieves the instance pointer and entity id from an alias path that's relative to this instance.
            //! It returns a pair with the Instance and entity id. The Instance is set to null and entityId is set to invalid if not found.
            //! @{
            AZStd::pair<Instance*, AZ::EntityId> GetInstanceAndEntityIdFromAliasPath(AliasPathView relativeAliasPath);
            AZStd::pair<const Instance*, AZ::EntityId> GetInstanceAndEntityIdFromAliasPath(AliasPathView relativeAliasPath) const;
            //! @}

            //! Gets the aliases of all the nested instances, which are sourced by the template with the given id.
            //! @param templateId The source template id of the nested instances.
            //! @return The list of aliases of the nested instances.
            AZStd::vector<InstanceAlias> GetNestedInstanceAliases(TemplateId templateId) const;

            //! Activates the container entity.
            void ActivateContainerEntity();

            //! Finds a nested instance non-recursively by instance alias.
            //! @{
            InstanceOptionalReference FindNestedInstance(const InstanceAlias& nestedInstanceAlias);
            InstanceOptionalConstReference FindNestedInstance(const InstanceAlias& nestedInstanceAlias) const;
            //! @}

            //! Sets link id.
            //! @param linkId Link id to be set.
            void SetLinkId(LinkId linkId);
            
            //! Gets link id.
            //! @return Link id of the instance.
            LinkId GetLinkId() const;

            //! Query functions for parent instance.
            //! @{
            InstanceOptionalReference GetParentInstance();
            InstanceOptionalConstReference GetParentInstance() const;
            bool HasParentInstance() const;
            bool IsParentInstance(const Instance& instance) const;
            //! @}

            //! Query, getter and setter functions for container entity.
            //! @{
            AZ::EntityId GetContainerEntityId() const;
            bool HasContainerEntity() const;
            EntityOptionalReference GetContainerEntity();
            EntityOptionalConstReference GetContainerEntity() const;
            void SetContainerEntity(AZ::Entity& entity);
            //! @}

            //! Detaches the container entity from the instance.
            //! It unregisters the entity id and moves the entity ownership to the caller.
            //! @return Unique pointer to the container entity.
            AZStd::unique_ptr<AZ::Entity> DetachContainerEntity();

            //! Detaches and deletes the container entity from the instance.
            //! @return True if the container entity was detached and destroyed, false otherwise.
            bool DestroyContainerEntity();

            //! Getter for instance alias.
            //! @return Instance alias string.
            const InstanceAlias& GetInstanceAlias() const;

            //! Gets absolute instance alias path seen from root instance.
            //! @return Alias path seen from root instance.
            AliasPath GetAbsoluteInstanceAliasPath() const;

            //! Gets relative alias path to entity.
            //! @param entity The entity that the relative path points to.
            //! @return Alias path to the provided entity.
            AliasPath GetAliasPathRelativeToInstance(const AZ::EntityId& entity) const;

            //! Entity and instance alias generator functions.
            //! @{
            static EntityAlias GenerateEntityAlias();
            static InstanceAlias GenerateInstanceAlias();
            //! @}

            //! Getter and setter for cached instance DOM.
            //! @{
            PrefabDomConstReference GetCachedInstanceDom() const;
            PrefabDomReference GetCachedInstanceDom();
            void SetCachedInstanceDom(PrefabDomValueConstReference instanceDom);
            //! @}

        private:
            static constexpr const char s_aliasPathSeparator = '/';
            static constexpr EntityIdInstanceRelationship DefaultEntityIdInstanceRelationship = EntityIdInstanceRelationship::OneToOne;

            //! Private constructor called by public constructor.
            Instance(AZStd::unique_ptr<AZ::Entity> containerEntity, InstanceOptionalReference parent, InstanceAlias alias,
                EntityIdInstanceRelationship entityIdInstanceRelationship = DefaultEntityIdInstanceRelationship);

            //! Unregisters and erases all entities owned by this instance.
            void ClearEntities();

            //! Detaches entities based on a user provided filter. Returns true to detach.
            void RemoveEntities(const AZStd::function<bool(const AZStd::unique_ptr<AZ::Entity>&)>& filter);

            //! Functions for get-entity implementation.
            //! @{
            bool GetEntities_Impl(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback);
            bool GetConstEntities_Impl(const AZStd::function<bool(const AZ::Entity&)>& callback) const;
            bool GetAllEntitiesInHierarchy_Impl(const AZStd::function<bool(AZStd::unique_ptr<AZ::Entity>&)>& callback);
            bool GetAllEntitiesInHierarchyConst_Impl(const AZStd::function<bool(const AZ::Entity&)>& callback) const;
            //! @}

            //! Registers an entity for the instance.
            //! It generates mapping from entity id to entity alias, and vice versa.
            //! @param entityId The entity id to be registered.
            //! @param entityAlias The entity alias to be registered.
            //! @return Returns true if the registration succeeds.
            bool RegisterEntity(const AZ::EntityId& entityId, const EntityAlias& entityAlias);

            //! Unregisters an entity for the instance.
            //! It erases mapping from entity id to entity alias, and vice versa.
            //! @param entityId The entity id to be unregistered.
            //! @param entityAlias The entity alias to be unregistered.
            //! @return Returns true if the unregistration succeeds.
            bool UnregisterEntity(AZ::EntityId entityId);

            //! Helper function for detaching entity.
            //! It removes an entity from map and returns entity pointer.
            //! @param entityAlias The entity alias to be erased.
            //! @return Unique pointer to the entity.
            AZStd::unique_ptr<AZ::Entity> DetachEntityHelper(const EntityAlias& entityAlias);

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

            // This can be used to set the DOM that was last used for building the instance object through deserialization.
            // This is optional and will only be set when asked explicitly through SetCachedInstanceDom().
            PrefabDom m_cachedInstanceDom;

            // The unique ID of the template this Instance belongs to.
            TemplateId m_templateId = InvalidTemplateId;

            // Pointer to the parent instance if nested
            Instance* m_parent = nullptr;

            // Interface for registering owned entities for external queries
            InstanceEntityMapperInterface* m_instanceEntityMapper = nullptr;

            // Interface for registering the Instance itself for external queries.
            TemplateInstanceMapperInterface* m_templateInstanceMapper = nullptr;

            // Defines entity id and instance relationship. The default relationship is one-to-one.
            EntityIdInstanceRelationship m_entityIdInstanceRelationship = DefaultEntityIdInstanceRelationship;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
