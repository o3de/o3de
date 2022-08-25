/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabInstanceUtils
        {
            AZStd::pair<const Instance*, AZStd::vector<InstanceOptionalConstReference>> ClimbUpToTargetOrRootInstance(
                const Instance* startInstance, const Instance* targetInstance)
            {
                if (!startInstance)
                {
                    return AZStd::make_pair(nullptr, AZStd::vector<InstanceOptionalConstReference>());
                }

                // Climb up the instance hierarchy from this instance until you hit the target or the root.
                InstanceOptionalConstReference instance = *startInstance;
                AZStd::vector<InstanceOptionalConstReference> instancesInHierarchy;

                while (&instance->get() != targetInstance && instance->get().GetParentInstance() != AZStd::nullopt)
                {
                    instancesInHierarchy.emplace_back(instance);
                    instance = instance->get().GetParentInstance();
                }

                return AZStd::make_pair(&instance->get(), AZStd::move(instancesInHierarchy));
            }

            AZStd::pair<const Instance*, AZStd::string> GetRelativePathBetweenInstances(
                const Instance* startInstance, const Instance* targetInstance)
            {
                if (!startInstance)
                {
                    return AZStd::make_pair(nullptr, "");
                }

                auto climbUpToTargetOrRootInstanceResult = ClimbUpToTargetOrRootInstance(startInstance, targetInstance);

                AZStd::string relativePathToStartInstance;
                const auto& instancePath = climbUpToTargetOrRootInstanceResult.second;
                for (auto instanceIter = instancePath.crbegin(); instanceIter != instancePath.crend(); ++instanceIter)
                {
                    relativePathToStartInstance.append("/Instances/");
                    relativePathToStartInstance.append((*instanceIter)->get().GetInstanceAlias());
                }

                return AZStd::make_pair(climbUpToTargetOrRootInstanceResult.first, AZStd::move(relativePathToStartInstance));
            }
        } // namespace PrefabInstanceUtils
    } // namespace Prefab
} // namespace AzToolsFramework
