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
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        class PrefabFocusInterface;
        class PrefabSystemComponentInterface;
        class TemplateInstanceMapperInterface;

        class InstanceUpdateExecutor
            : public InstanceUpdateExecutorInterface
        {
        public:
            AZ_RTTI(InstanceUpdateExecutor, "{E21DB0D4-0478-4DA9-9011-31BC96F55837}", InstanceUpdateExecutorInterface);
            AZ_CLASS_ALLOCATOR(InstanceUpdateExecutor, AZ::SystemAllocator, 0);

            explicit InstanceUpdateExecutor(int instanceCountToUpdateInBatch = 0);

            void AddInstanceToQueue(InstanceOptionalReference instance) override;
            void AddTemplateInstancesToQueue(TemplateId instanceTemplateId, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt) override;
            bool UpdateTemplateInstancesInQueue() override;
            void RemoveTemplateInstanceFromQueue(const Instance* instance) override;

            void RegisterInstanceUpdateExecutorInterface();
            void UnregisterInstanceUpdateExecutorInterface();

        private:
            PrefabFocusInterface* m_prefabFocusInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
            TemplateInstanceMapperInterface* m_templateInstanceMapperInterface = nullptr;

            static AzFramework::EntityContextId s_editorEntityContextId;

            // TODO - finish these comments

            /**
             * Climbs up the instance hierarchy tree from startInstance.
             * Stops when it hits either the targetInstance or the root.
             * @param startInstance The position the new entity will be created at.
             * @param parentId The id of the parent of the newly created entity.
             */
            const Instance* ClimbUpToTargetInstance(const Instance* startInstance, const Instance* targetInstance, AZStd::string& aliasPath);

            /**
            * 
             */
            bool GenerateInstanceDomAccordingToCurrentFocus(const Instance* instance, PrefabDom& instanceDom);

            int m_instanceCountToUpdateInBatch = 0;
            AZStd::deque<Instance*> m_instancesUpdateQueue;
            bool m_updatingTemplateInstancesInQueue { false };
        };
    }
}
