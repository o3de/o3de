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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Vector3.h>

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
            PrefabOperationResult InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, AZ::Vector3 position) override;
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

        private:
            PrefabOperationResult DeleteFromInstance(const EntityIdList& entityIds, bool deleteDescendants);
            bool RetrieveAndSortPrefabEntitiesAndInstances(const EntityList& inputEntities, Instance& commonRootEntityOwningInstance,
                EntityList& outEntities, AZStd::vector<AZStd::unique_ptr<Instance>>& outInstances) const;

            InstanceOptionalReference GetOwnerInstanceByEntityId(AZ::EntityId entityId) const;
            bool EntitiesBelongToSameInstance(const EntityIdList& entityIds) const;

            static Instance* GetParentInstance(Instance* instance);
            static Instance* GetAncestorOfInstanceThatIsChildOfRoot(const Instance* ancestor, Instance* descendant);
            static void GenerateContainerEntityTransform(const EntityList& topLevelEntities, AZ::Vector3& translation, AZ::Quaternion& rotation);
            static void EntityIdListToEntityList(const EntityIdList& inputEntityIds, EntityList& outEntities);

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;

            // Caches entity states for undo/redo purposes
            PrefabUndoCache m_prefabUndoCache;

            uint64_t m_newEntityCounter = 1;
        };
    }
}
