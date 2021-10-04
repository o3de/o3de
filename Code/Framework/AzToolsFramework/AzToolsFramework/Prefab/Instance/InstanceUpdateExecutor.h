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
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutorInterface.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        class PrefabSystemComponentInterface;
        class TemplateInstanceMapperInterface;

        class InstanceUpdateExecutor
            : public InstanceUpdateExecutorInterface
        {
        public:
            AZ_RTTI(InstanceUpdateExecutor, "{E21DB0D4-0478-4DA9-9011-31BC96F55837}", InstanceUpdateExecutorInterface);
            AZ_CLASS_ALLOCATOR(InstanceUpdateExecutor, AZ::SystemAllocator, 0);

            explicit InstanceUpdateExecutor(int instanceCountToUpdateInBatch = 0);

            void AddTemplateInstancesToQueue(TemplateId instanceTemplateId, bool immediate = false, InstanceOptionalReference instanceToExclude = AZStd::nullopt) override;
            bool UpdateTemplateInstancesInQueue() override;
            virtual void RemoveTemplateInstanceFromQueue(const Instance* instance) override;

            void RegisterInstanceUpdateExecutorInterface();
            void UnregisterInstanceUpdateExecutorInterface();

        private:
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
            TemplateInstanceMapperInterface* m_templateInstanceMapperInterface = nullptr;
            int m_instanceCountToUpdateInBatch = 0;
            AZStd::deque<Instance*> m_instancesUpdateQueue;
            bool m_updatingTemplateInstancesInQueue { false };
        };
    }
}
