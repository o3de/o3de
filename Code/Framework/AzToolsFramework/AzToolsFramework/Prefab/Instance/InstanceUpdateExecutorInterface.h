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
        //! Interface to handle instance update in template propagation.
        class InstanceUpdateExecutorInterface
        {
        public:
            AZ_RTTI(InstanceUpdateExecutorInterface, "{26BB4ACF-6EA5-4668-84E7-AB75B90BA449}");
            virtual ~InstanceUpdateExecutorInterface() = default;

            //! Adds an instance into queue for updating it later.
            //! @param instance The instance to be added to queue.
            virtual void AddInstanceToQueue(InstanceOptionalReference instance) = 0;

            //! Adds all instances of template with given id into queue for updating them later.
            //! @param instanceTemplateId The template id used to retrieve all instances.
            //! @param instanceToExclude The instance to be excluded. It is nullopt by default.
            virtual void AddTemplateInstancesToQueue(TemplateId instanceTemplateId, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt) = 0;

            //! Updates instances in the waiting queue.
            //! @return bool on whether the operation succeeds.
            virtual bool UpdateTemplateInstancesInQueue() = 0;

            //! Removes an instance from the waiting queue.
            //! @param instance The instance to be removed from queue.
            virtual void RemoveTemplateInstanceFromQueue(Instance* instance) = 0;

            //! Sets the flag that tells whether root prefab instance is loaded to false.
            //! A notification OnRootPrefabInstanceLoaded will fire during the propagation if root
            //! prefab instance is loaded for the first time after this function is called.
            virtual void QueueRootPrefabLoadedNotificationForNextPropagation() = 0;

            //! Sets whether to pause instance propagation or not.
            //! When making property changes in the entity editor, pausing propagation during
            //! editing will prevent the user from losing control of the properties they are editing.
            //! @param shouldPausePropagation Flag that decides whether to pause propagation.
            virtual void SetShouldPauseInstancePropagation(bool shouldPausePropagation) = 0;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
