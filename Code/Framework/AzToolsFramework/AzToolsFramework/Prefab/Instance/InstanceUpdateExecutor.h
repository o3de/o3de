/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/containers/deque.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipService.h>
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        class PrefabSystemComponentInterface;
        class TemplateInstanceMapperInterface;
        class InstanceDomGeneratorInterface;

        class InstanceUpdateExecutor
            : public InstanceUpdateExecutorInterface
        {
        public:
            AZ_RTTI(InstanceUpdateExecutor, "{E21DB0D4-0478-4DA9-9011-31BC96F55837}", InstanceUpdateExecutorInterface);
            AZ_CLASS_ALLOCATOR(InstanceUpdateExecutor, AZ::SystemAllocator);

            explicit InstanceUpdateExecutor(int instanceCountToUpdateInBatch = 0);

            void AddInstanceToQueue(InstanceOptionalReference instance) override;
            void AddTemplateInstancesToQueue(TemplateId instanceTemplateId, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt) override;

            // Note, this function destroys and re-creates Entity* and Component*, do not assume your pointers are still good after this.
            bool UpdateTemplateInstancesInQueue() override;
            void RemoveTemplateInstanceFromQueue(Instance* instance) override;
            void QueueRootPrefabLoadedNotificationForNextPropagation() override;

            void SetShouldPauseInstancePropagation(bool shouldPausePropagation) override;

            void RegisterInstanceUpdateExecutorInterface();
            void UnregisterInstanceUpdateExecutorInterface();

        private:
            //! Connect the game mode event handler in a lazy fashion rather than at construction of this class.
            //! This is required because the event won't be ready for connection during construction as EditorEntityContextComponent
            //! gets initialized after the PrefabSystemComponent.
            void LazyConnectGameModeEventHandler();

            void AddInstanceToQueue(Instance* instance);

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
            TemplateInstanceMapperInterface* m_templateInstanceMapperInterface = nullptr;
            InstanceDomGeneratorInterface* m_instanceDomGeneratorInterface = nullptr;
            AZ::IO::Path m_rootPrefabInstanceSourcePath;
            AZStd::deque<Instance*> m_instancesUpdateQueue;
            AZStd::unordered_set<Instance*> m_uniqueInstancesForPropagation;

            AZ::Event<GameModeState>::Handler m_GameModeEventHandler;
            int m_instanceCountToUpdateInBatch = 0;
            bool m_isRootPrefabInstanceLoaded = false;
            bool m_shouldPausePropagation = false;
            bool m_updatingTemplateInstancesInQueue { false };
        };
    }
}
