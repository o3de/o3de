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
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/containers/queue.h>
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

            void AddTemplateInstancesToQueue(TemplateId instanceTemplateId) override;
            bool UpdateTemplateInstancesInQueue() override;

            void RegisterInstanceUpdateExecutorInterface();
            void UnregisterInstanceUpdateExecutorInterface();

        private:
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
            TemplateInstanceMapperInterface* m_templateInstanceMapperInterface = nullptr;
            int m_instanceCountToUpdateInBatch = 0;
            AZStd::queue<Instance*> m_instancesUpdateQueue;
            bool m_updatingTemplateInstancesInQueue { false };
        };
    }
}
