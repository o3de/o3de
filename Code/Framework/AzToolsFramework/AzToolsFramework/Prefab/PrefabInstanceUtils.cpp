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
            InstanceClimbUpResult ClimbUpToTargetOrRootInstance(const Instance& startInstance, const Instance* targetInstance)
            {
                InstanceClimbUpResult result;
                
                // Climbs up the instance hierarchy from the start instance until it hits the target or the root instance.
                const Instance* instancePtr = &startInstance;

                if (targetInstance)
                {
                    while (instancePtr != targetInstance && instancePtr->HasParentInstance())
                    {
                        result.m_climbedInstances.emplace_back(instancePtr);
                        instancePtr = &(instancePtr->GetParentInstance()->get());
                    }

                    if (instancePtr == targetInstance)
                    {
                        result.m_isTargetInstanceReached = true;
                    }
                }
                else
                {
                    // Returns the root.
                    while (instancePtr->HasParentInstance())
                    {
                        result.m_climbedInstances.emplace_back(instancePtr);
                        instancePtr = &(instancePtr->GetParentInstance()->get());
                    }
                }

                result.m_reachedInstance = instancePtr;
                return result;
            }

            AZStd::string GetRelativePathBetweenInstances(const Instance& parentInstance, const Instance& childInstance)
            {
                const Instance* instancePtr = &childInstance;
                AZStd::vector<const Instance*> climbedInstances;

                // Climbs up the instance hierarchy from the child instance until it hits the parent instance.
                while (instancePtr != &parentInstance && instancePtr->HasParentInstance())
                {
                    climbedInstances.emplace_back(instancePtr);
                    instancePtr = &(instancePtr->GetParentInstance()->get());
                }

                if (instancePtr != &parentInstance)
                {
                    AZ_Warning("Prefab", false, "PrefabInstanceUtils::GetRelativePathBetweenInstances() - "
                               "Tried to get relative path but the parent instance is not a valid parent. Returns empty string instead.");
                    return "";
                }

                return GetRelativePathFromClimbedInstances(climbedInstances);
            }

            AZStd::string GetRelativePathFromClimbedInstances(const AZStd::vector<const Instance*>& climbedInstances,
                bool skipTopClimbedInstance)
            {
                AZStd::string relativePath = "";
                if (climbedInstances.empty() && skipTopClimbedInstance)
                {
                    return relativePath;
                }

                auto climbedInstancesBeginIter = skipTopClimbedInstance ?
                    ++climbedInstances.crbegin() : climbedInstances.crbegin();
                for (auto instanceIter = climbedInstancesBeginIter; instanceIter != climbedInstances.crend(); ++instanceIter)
                {
                    relativePath.append(PrefabDomUtils::PathStartingWithInstances);
                    relativePath.append((*instanceIter)->GetInstanceAlias());
                }

                return AZStd::move(relativePath);
            }

            bool IsDescendantInstance(const Instance& childInstance, const Instance& parentInstance)
            {
                const Instance* instancePtr = &childInstance;

                while (instancePtr)
                {
                    if (instancePtr == &parentInstance)
                    {
                        return true;
                    }
                    instancePtr = instancePtr->HasParentInstance() ? &(instancePtr->GetParentInstance()->get()) : nullptr;
                }

                return false;
            }
        } // namespace PrefabInstanceUtils
    } // namespace Prefab
} // namespace AzToolsFramework
