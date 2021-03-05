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

            // Add all Instances of Template with given Id into a queue for updating them later.
            virtual void AddTemplateInstancesToQueue(TemplateId instanceTemplateId) = 0;

            // Update Instances in the waiting queue.
            virtual bool UpdateTemplateInstancesInQueue() = 0;
        };
    }
}
