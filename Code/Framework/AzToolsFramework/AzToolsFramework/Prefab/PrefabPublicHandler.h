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

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/PrefabUndoCache.h>

namespace AzToolsFramework
{
    using EntityList = AZStd::vector<AZ::Entity*>;

    namespace Prefab
    {
        class Instance;

        class InstanceEntityMapperInterface;
        class InstanceToTemplateInterface;
        class PrefabLoaderInterface;
        class PrefabSystemComponentInterface;

        class PrefabPublicHandler final
            : public PrefabPublicInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabPublicHandler, AZ::SystemAllocator, 0);
            AZ_RTTI(PrefabPublicHandler, "{35802943-6B60-430F-9DED-075E3A576A25}", PrefabPublicInterface);

            void RegisterPrefabPublicHandlerInterface();
            void UnregisterPrefabPublicHandlerInterface();

            // PrefabPublicInterface...
            PrefabOperationResult CreatePrefab(const AZStd::vector<AZ::EntityId>& entityIds, AZ::IO::PathView filePath) override;
            PrefabOperationResult InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position) override;
            PrefabOperationResult SavePrefab(AZ::IO::Path filePath) override;
            PrefabEntityResult CreateEntity(AZ::EntityId parentId, const AZ::Vector3& position) override;
            
            void GenerateUndoNodesForEntityChangeAndUpdateCache(AZ::EntityId entityId, UndoSystem::URSequencePoint* parentUndoBatch) override;

            bool IsInstanceContainerEntity(AZ::EntityId entityId) const override;
            bool IsLevelInstanceContainerEntity(AZ::EntityId entityId) const override;
            AZ::EntityId GetInstanceContainerEntityId(AZ::EntityId entityId) const override;
            AZ::EntityId GetLevelInstanceContainerEntityId() const override;
            AZ::IO::Path GetOwningInstancePrefabPath(AZ::EntityId entityId) const override;
            PrefabRequestResult HasUnsavedChanges(AZ::IO::Path prefabFilePath) const override;

            PrefabOperationResult DeleteEntitiesInInstance(const EntityIdList& entityIds) override;
            PrefabOperationResult DeleteEntitiesAndAllDescendantsInInstance(const EntityIdList& entityIds) override;
            PrefabOperationResult DuplicateEntitiesInInstance(const EntityIdList& entityIds) override;

        private:
            PrefabOperationResult DeleteFromInstance(const EntityIdList& entityIds, bool deleteDescendants);
            bool RetrieveAndSortPrefabEntitiesAndInstances(const EntityList& inputEntities, Instance& commonRootEntityOwningInstance,
                EntityList& outEntities, AZStd::vector<Instance*>& outInstances) const;

            InstanceOptionalReference GetOwnerInstanceByEntityId(AZ::EntityId entityId) const;
            bool EntitiesBelongToSameInstance(const EntityIdList& entityIds) const;

            /**
             * Creates a link between the templates of an instance and its parent.
             * 
             * \param topLevelEntities The list of entities that are immediate children to the container entity of the instance.
             * \param sourceInstance The instance that corresponds to the source template of the link.
             * \param targetInstance The id of the target template.
             * \param undoBatch The undo batch to set as parent for this create link action.
             * \param commonRootEntityId The id of the entity that the source instance should be parented under.
             * \param isUndoRedoSupportNeeded The flag indicating whether the link should be created with undo/redo support or not.
             */
            void CreateLink(
                const EntityList& topLevelEntities, Instance& sourceInstance, TemplateId targetTemplateId,
                UndoSystem::URSequencePoint* undoBatch, AZ::EntityId commonRootEntityId, const bool isUndoRedoSupportNeeded = true);

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

            static Instance* GetParentInstance(Instance* instance);
            static Instance* GetAncestorOfInstanceThatIsChildOfRoot(const Instance* ancestor, Instance* descendant);
            static void GenerateContainerEntityTransform(const EntityList& topLevelEntities, AZ::Vector3& translation, AZ::Quaternion& rotation);

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
            PrefabLoaderInterface* m_prefabLoaderInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;

            // Caches entity states for undo/redo purposes
            PrefabUndoCache m_prefabUndoCache;

            uint64_t m_newEntityCounter = 1;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
