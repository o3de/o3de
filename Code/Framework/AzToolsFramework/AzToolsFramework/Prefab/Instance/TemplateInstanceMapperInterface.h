/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class TemplateInstanceMapperInterface
        {
        public:
            AZ_RTTI(TemplateInstanceMapperInterface, "{5DCCCDAA-3441-4266-9670-B349386E0129}");

            virtual ~TemplateInstanceMapperInterface() = default;
            virtual InstanceSetConstReference FindInstancesOwnedByTemplate(TemplateId templateId) const = 0;

        protected:
            // Only the Instance class is allowed to register and unregister Instances.
            friend class Instance;

            virtual bool RegisterInstanceToTemplate(Instance& instance) = 0;
            virtual bool UnregisterInstance(Instance& instance) = 0;
        };
    }
}
