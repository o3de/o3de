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
            AZStd::pair<const Instance*, AZStd::vector<InstanceOptionalConstReference>> GetRelativePathBetweenInstances(
                const Instance* startInstance, const Instance* targetInstance)
            {
                if (!startInstance)
                {
                    return AZStd::make_pair(nullptr, AZStd::vector<InstanceOptionalConstReference>());
                }

                // Climb up the instance hierarchy from this instance until you hit the target or the root.
                InstanceOptionalConstReference instance = *startInstance;
                AZStd::vector<InstanceOptionalConstReference> instancePath;

                while (&instance->get() != targetInstance && instance->get().GetParentInstance() != AZStd::nullopt)
                {
                    instancePath.emplace_back(instance);
                    instance = instance->get().GetParentInstance();
                }

                return AZStd::make_pair(&instance->get(), AZStd::move(instancePath));
            }

            AZStd::pair<const Instance*, AZStd::string> GetRelativePathStringBetweenInstances(
                const Instance* startInstance, const Instance* targetInstance)
            {
                if (!startInstance)
                {
                    return AZStd::make_pair(nullptr, "");
                }

                auto getRelativePathResult = GetRelativePathBetweenInstances(startInstance, targetInstance);

                AZStd::string relativePathToStartInstance;
                const auto& instancePath = getRelativePathResult.second;
                for (auto instanceIter = instancePath.crbegin(); instanceIter != instancePath.crend(); ++instanceIter)
                {
                    relativePathToStartInstance.append("/Instances/");
                    relativePathToStartInstance.append((*instanceIter)->get().GetInstanceAlias());
                }

                return AZStd::make_pair(getRelativePathResult.first, AZStd::move(relativePathToStartInstance));
            }
        } // namespace PrefabInstanceUtils
    } // namespace Prefab
} // namespace AzToolsFramework
