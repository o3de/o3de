/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabInstanceUtils
        {
            InstanceClimbUpResult ClimbUpToTargetOrRootInstance(const Instance& startInstance, InstanceOptionalConstReference targetInstance)
            {
                InstanceClimbUpResult result;
                
                // Climbs up the instance hierarchy from the start instance until it hits the target or the root instance.
                const Instance* instance = &startInstance;

                while (!CompareInstances(*instance, targetInstance) && instance->HasParentInstance())
                {
                    result.climbedInstances.emplace_back(*instance);
                    instance = &(instance->GetParentInstance()->get());
                }

                result.reachedInstance = *instance;
                return result;
            }

            AZStd::string GetRelativePathBetweenInstances(const Instance& parentInstance, const Instance& childInstance)
            {
                const Instance* instance = &childInstance;
                AZStd::vector<InstanceOptionalConstReference> climbedInstances;

                // Climbs up the instance hierarchy from the child instance until it hits the parent instance.
                while (!CompareInstances(*instance, parentInstance) && instance->HasParentInstance())
                {
                    climbedInstances.emplace_back(*instance);
                    instance = &(instance->GetParentInstance()->get());
                }

                if (!CompareInstances(*instance, parentInstance))
                {
                    AZ_Warning("Prefab", false, "PrefabInstanceUtils::GetRelativePathBetweenInstances() - "
                               "Tried to get relative path but the parent instance is not a valid parent. Returns empty string instead.");
                    return "";
                }

                // Generates relative path.
                return GetRelativePath(climbedInstances);
            }

            AZStd::string GetRelativePath(const AZStd::vector<InstanceOptionalConstReference>& climbedInstances)
            {
                AZStd::string relativePath = "";

                for (auto instanceIter = climbedInstances.rbegin(); instanceIter != climbedInstances.rend(); ++instanceIter)
                {
                    relativePath.append(PrefabDomUtils::PathStartingWithInstances);
                    const Instance& instance = (*instanceIter)->get();
                    relativePath.append(instance.GetInstanceAlias());
                }

                return relativePath;
            }

            bool IsHierarchical(const Instance& childInstance, const Instance& parentInstance)
            {
                const Instance* instance = &childInstance;

                while (instance)
                {
                    if (CompareInstances(*instance, parentInstance))
                    {
                        return true;
                    }
                    instance = instance->HasParentInstance() ? &(instance->GetParentInstance()->get()) : nullptr;
                }

                return false;
            }

            bool IsProperlyHierarchical(const Instance& childInstance, const Instance& parentInstance)
            {
                if (!childInstance.HasParentInstance())
                {
                    return false;
                }

                return IsHierarchical(childInstance.GetParentInstance()->get(), parentInstance);
            }

            bool CompareInstances(InstanceOptionalConstReference instanceA, InstanceOptionalConstReference instanceB)
            {
                if (!instanceA.has_value())
                {
                    return !instanceB.has_value();
                }

                return instanceB.has_value() && &(instanceA->get()) == &(instanceB->get());
            }
        } // namespace PrefabInstanceUtils
    } // namespace Prefab
} // namespace AzToolsFramework
