/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string_view.h>

#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabFocusHandler.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabUndoCache.h>
#include <AzCore/Math/Transform.h>

class QString;

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        class InstanceEntityMapperInterface;
        class InstanceToTemplateInterface;
        class InstanceDomGeneratorInterface;
        class PrefabLoaderInterface;
        class PrefabSystemComponentInterface;

        class PrefabPublicHandler final
            : public PrefabPublicInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabPublicHandler, AZ::SystemAllocator);
            AZ_RTTI(PrefabPublicHandler, "{35802943-6B60-430F-9DED-075E3A576A25}", PrefabPublicInterface);

            PrefabPublicHandler();

            void RegisterPrefabPublicHandlerInterface();
            void UnregisterPrefabPublicHandlerInterface();

            // PrefabPublicInterface...
            CreatePrefabResult CreatePrefabInDisk(const EntityIdList& entityIds, AZ::IO::PathView filePath) override;
            CreatePrefabResult CreatePrefabAndSaveToDisk(const EntityIdList& entityIds, AZ::IO::PathView filePath) override;
            CreatePrefabResult CreatePrefabInMemory(const EntityIdList& entityIds, AZ::IO::PathView filePath) override;
            InstantiatePrefabResult InstantiatePrefab(
                AZStd::string_view filePath, AZ::EntityId parentId, const AZ::Transform& transform) override;
            InstantiatePrefabResult InstantiatePrefab(
                AZStd::string_view filePath, AZ::EntityId parentId, const AZ::Vector3& position) override;

            PrefabOperationResult SavePrefab(AZ::IO::Path filePath) override;
            PrefabEntityResult CreateEntity(AZ::EntityId parentId, const AZ::Vector3& position) override;
            
            PrefabOperationResult GenerateUndoNodesForEntityChangeAndUpdateCache(AZ::EntityId entityId, UndoSystem::URSequencePoint* parentUndoBatch) override;

            bool IsOwnedByPrefabInstance(AZ::EntityId entityId) const override;
            bool IsOwnedByProceduralPrefabInstance(AZ::EntityId entityId) const override;
            bool IsInstanceContainerEntity(AZ::EntityId entityId) const override;
            bool IsLevelInstanceContainerEntity(AZ::EntityId entityId) const override;
            bool EntitiesBelongToSameInstance(const EntityIdList& entityIds) const;
            AZ::EntityId GetInstanceContainerEntityId(AZ::EntityId entityId) const override;
            AZ::EntityId GetLevelInstanceContainerEntityId() const override;
            AZ::IO::Path GetOwningInstancePrefabPath(AZ::EntityId entityId) const override;
            PrefabRequestResult HasUnsavedChanges(AZ::IO::Path prefabFilePath) const override;

            PrefabOperationResult DeleteEntitiesAndAllDescendantsInInstance(const EntityIdList& entityIds) override;
            DuplicatePrefabResult DuplicateEntitiesInInstance(const EntityIdList& entityIds) override;

            PrefabOperationResult DetachPrefab(const AZ::EntityId& containerEntityId) override;
            PrefabOperationResult DetachPrefabAndRemoveContainerEntity(const AZ::EntityId& containerEntityId) override;

        private:
            PrefabOperationResult DetachPrefabImpl(const AZ::EntityId& containerEntityId, bool keepContainerEntity);

            PrefabOperationResult DeleteFromInstance(const EntityIdList& entityIds);
            PrefabOperationResult RetrieveAndSortPrefabEntitiesAndInstances(
                const EntityList& inputEntities,
                Instance& commonRootEntityOwningInstance,
                EntityList& outEntities,
                AZStd::vector<Instance*>& outInstances) const;

            //! Sanitizes an EntityIdList to remove entities that should not be affected by prefab operations.
            //! It will identify and exclude the container entity of the root prefab instance, and all read-only entities.
            EntityIdList SanitizeEntityIdList(const EntityIdList& entityIds) const;

            InstanceOptionalReference GetOwnerInstanceByEntityId(AZ::EntityId entityId) const;
            void AddNewEntityToSortOrder(Instance& owningInstance, PrefabDom& domToAddEntityUnder,
                const EntityAlias& parentEntityAlias, const EntityAlias& entityToAddAlias);

            /**
             * Duplicate a list of entities owned by a common owning instance by directly
             * copying/modifying their entries in the instance DOM
             *
             * \param commonOwningInstance The common owning instance of all the entities being duplicated.
             * \param entities The list of Entities that will be duplicated.
             * \param domToAddDuplicatedEntitiesUnder The DOM of the common owning instance where the duplicated
             *      entity DOM values will be added to.
             * \param duplicatedEntityIds A list of EntityIds corresponding to the entities that were duplicated.
             */
            void DuplicateNestedEntitiesInInstance(Instance& commonOwningInstance,
                const AZStd::vector<AZ::Entity*>& entities, PrefabDom& domToAddDuplicatedEntitiesUnder,
                EntityIdList& duplicatedEntityIds, AZStd::unordered_map<EntityAlias, EntityAlias>& oldAliasToNewAliasMap);
            /**
             * Duplicate a list of instances owned by a common owning instance by directly
             * copying/modifying their entries in the instance DOM
             *
             * \param commonOwningInstance The common owning instance of all the instances being duplicated.
             * \param entities The list of Instances that will be duplicated.
             * \param domToAddDuplicatedInstancesUnder The DOM of the common owning instance where the duplicated
             *      instance DOM values will be added to.
             * \param duplicatedEntityIds A list of EntityIds corresponding to the instances that were duplicated.
             */
            void DuplicateNestedInstancesInInstance(Instance& commonOwningInstance,
                const AZStd::vector<Instance*>& instances, PrefabDom& domToAddDuplicatedInstancesUnder,
                EntityIdList& duplicatedEntityIds, AZStd::unordered_map<InstanceAlias, Instance*>& newInstanceAliasToOldInstanceMap);

            /**
             * Applies the correct transform to the container entity, and returns an appropriate patch.
             * This helper function won't support undo/redo, update the templates or create any links.
             * All that needs to be done by the caller.
             *
             * \param containerEntityId The container to apply the changes to.
             * \param parentEntityId The id of the entity the container should be parented to.
             * \param translation New translation for the container entity.
             * \param rotation New rotation for the container entity.
             * \return The PrefabDom containing the patches that should be stored in the parent link.
             */
            PrefabDom ApplyContainerTransformAndGeneratePatch(AZ::EntityId containerEntityId, AZ::EntityId parentEntityId,
                const AZ::Vector3& translation, const AZ::Quaternion& rotation);

            /**
             * Creates a link between the templates of an instance and its parent.
             * 
             * \param sourceInstance The instance that corresponds to the source template of the link (child).
             * \param targetInstance The id of the target template (parent).
             * \param undoBatch The undo batch to set as parent for this create link action.
             * \param patch The patch to store in the newly created link dom.
             * \param isUndoRedoSupportNeeded The flag indicating whether the link should be created with undo/redo support or not.
             */
            void CreateLink(
                Instance& sourceInstance, TemplateId targetTemplateId, UndoSystem::URSequencePoint* undoBatch,
                PrefabDom patch, const bool isUndoRedoSupportNeeded = true);

            /**
             * Removes the link between template of the sourceInstance and the template corresponding to targetTemplateId.
             *
             * \param sourceInstance The instance corresponding to the source template of the link to be removed.
             * \param targetTemplateId The id of the target template of the link to be removed.
             * \param undoBatch The undo batch to set as parent for this remove link action.
             */
            void RemoveLink(
                AZStd::unique_ptr<Instance>& sourceInstance, TemplateId targetTemplateId, UndoSystem::URSequencePoint* undoBatch);

            /**
             * Given a list of entityIds, finds the prefab instance that owns the common root entity of the entityIds.
             * 
             * \param entityIds The list of entity ids.
             * \param inputEntityList The list of entities corresponding to the entity ids.
             * \param topLevelEntities The list of entities that are immediate children of the common root entity.
             * \param commonRootEntityId The entity id of the common root entity of all the entityIds.
             * \param commonRootEntityOwningInstance The owning instance of the common root entity.
             * \return PrefabOperationResult indicating whether the action was successful or not.
             */
            PrefabOperationResult FindCommonRootOwningInstance(
                const AZStd::vector<AZ::EntityId>& entityIds, EntityList& inputEntityList, EntityList& topLevelEntities,
                AZ::EntityId& commonRootEntityId, InstanceOptionalReference& commonRootEntityOwningInstance);

            /* Checks whether the template source path of any of the ancestors in the instance hierarchy matches with one of the
             * paths provided in a set.
             *
             * \param instance The instance whose ancestor hierarchy the provided set of template source paths will be tested against.
             * \param templateSourcePaths The template source paths provided to be checked against the instance ancestor hierarchy.
             * \return true if any of the template source paths could be found in the ancestor hierarchy of instance, false otherwise.
             */
            bool IsCyclicalDependencyFound(
                InstanceOptionalConstReference instance, const AZStd::unordered_set<AZ::IO::Path>& templateSourcePaths);

            static void Internal_HandleContainerOverride(
                UndoSystem::URSequencePoint* undoBatch, AZ::EntityId entityId, const PrefabDom& patch,
                const LinkId linkId);

            // if a non-nullopt const reference is sent to HandleEntityChange, it essentially means that the instance in question
            // will be fully updated by this function (as in, the template it came from will be updated, as well as the instance in question)
            // and thus, it can skip the instance update queue that happens later.
            static void Internal_HandleEntityChange(
                UndoSystem::URSequencePoint* undoBatch, AZ::EntityId entityId, PrefabDom& beforeState,
                PrefabDom& afterState, InstanceOptionalConstReference instanceToSkipUpdateQueue = AZStd::nullopt);

            void Internal_HandleInstanceChange(UndoSystem::URSequencePoint* undoBatch, AZ::Entity* entity, AZ::EntityId beforeParentId, AZ::EntityId afterParentId);

            void UpdateLinkPatchesWithNewEntityAliases(
                PrefabDom& linkPatch,
                const AZStd::unordered_map<AZ::EntityId, AZStd::string>& oldEntityAliases,
                Instance& newParent);

            static void ReplaceOldAliases(QString& stringToReplace, AZStd::string_view oldAlias, AZStd::string_view newAlias);

            static Instance* GetParentInstance(Instance* instance);
            static Instance* GetAncestorOfInstanceThatIsChildOfRoot(const Instance* ancestor, Instance* descendant);

            //! Generates the transform based on the input list of entities.
            //! The transform will be based on the average transform of the direct children.
            //! \param topLevelEntities The direct children entities provided to calculate the average transform.
            //! \param[out] translation The output translation.
            //! \param[out] rotation The output rotation.
            static void GenerateContainerEntityTransform(const EntityList& topLevelEntities, AZ::Vector3& translation, AZ::Quaternion& rotation);

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
            InstanceDomGeneratorInterface* m_instanceDomGeneratorInterface = nullptr;
            PrefabFocusInterface* m_prefabFocusInterface = nullptr;
            PrefabFocusPublicInterface* m_prefabFocusPublicInterface = nullptr;
            PrefabLoaderInterface* m_prefabLoaderInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;

            //! Caches entity states for undo/redo purposes.
            PrefabUndoCache m_prefabUndoCache;

            //! Handles the Prefab Focus API that determines what prefab is being edited.
            PrefabFocusHandler m_prefabFocusHandler;

            uint64_t m_newEntityCounter = 1;

            bool m_isRunningInEditor = true;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
