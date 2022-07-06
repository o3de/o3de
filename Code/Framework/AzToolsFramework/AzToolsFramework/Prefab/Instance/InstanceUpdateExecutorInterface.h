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

            // Add all Instances of Template with given Id into a queue for updating them later.
            virtual void AddTemplateInstancesToQueue(TemplateId instanceTemplateId, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt) = 0;

            // Update Instances in the waiting queue.
            virtual bool UpdateTemplateInstancesInQueue() = 0;

            // Remove an Instance from the waiting queue.
            virtual void RemoveTemplateInstanceFromQueue(const Instance* instance) = 0;

            // A notification OnRootPrefabInstanceLoaded will fire during the propagation if root
            // prefab instance is loaded for the first time after this function is called.
            virtual void QueueRootPrefabLoadedNotificationForNextPropagation() = 0;

            //! Sets whether to pause instance propagation or not.
            //! When making property changes in the entity editor, pausing propagation during
            //! editing will prevent the user from losing control of the properties they are editing.
            virtual void SetShouldPauseInstancePropagation(bool shouldPausePropagation) = 0;
        };
    }
}
