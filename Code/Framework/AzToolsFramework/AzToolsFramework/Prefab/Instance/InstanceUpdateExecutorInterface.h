/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceUpdateExecutorInterface
        {
        public:
            AZ_RTTI(InstanceUpdateExecutorInterface, "{26BB4ACF-6EA5-4668-84E7-AB75B90BA449}");
            virtual ~InstanceUpdateExecutorInterface() = default;

            // Add an instance into the queue so it's updated at the end of the frame.
            virtual void AddInstanceToQueue(InstanceOptionalReference instance) = 0;

            // Add all Instances of Template with given Id into a queue so it's updated at the end of the frame.
            virtual void AddTemplateInstancesToQueue(TemplateId instanceTemplateId, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt) = 0;

            // Update Instances in the waiting queue.
            virtual bool UpdateTemplateInstancesInQueue() = 0;

            // Remove an Instance from the waiting queue.
            virtual void RemoveTemplateInstanceFromQueue(const Instance* instance) = 0;
        };
    }
}
