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
            namespace Internal
            {
                static AZStd::string JoinInstanceAliases(const AZStd::vector<InstanceAlias>& instanceAliases)
                {
                    AZStd::string relativePath = "";
                    for (auto instanceAliasIter = instanceAliases.rbegin(); instanceAliasIter != instanceAliases.rend();
                         ++instanceAliasIter)
                    {
                        relativePath.append(PrefabDomUtils::PathStartingWithInstances);
                        relativePath.append(*instanceAliasIter);
                    }

                    return relativePath;
                }
            } // namespace Internal

            const Instance* ClimbUpToTargetOrRootInstance(
                AZStd::string& relativePath, const Instance& startInstance, InstanceOptionalConstReference targetInstance)
            {
                // Climbs up the instance hierarchy from start instance until hitting the target or the root instance.
                const Instance* instance = &startInstance;
                AZStd::vector<InstanceAlias> instanceAliases;
                while (!CompareInstances(*instance, targetInstance) && instance->HasParentInstance())
                {
                    instanceAliases.emplace_back(instance->GetInstanceAlias());
                    instance = &(instance->GetParentInstance()->get());
                }

                // Generates relative path.
                relativePath = Internal::JoinInstanceAliases(instanceAliases);

                return instance;
            }

            AZStd::string GetRelativePathBetweenInstances(const Instance& fromInstance, const Instance& toInstance)
            {
                const Instance* instance = &toInstance;
                AZStd::vector<InstanceAlias> instanceAliases;
                while (!CompareInstances(*instance, fromInstance) && instance->HasParentInstance())
                {
                    instanceAliases.emplace_back(instance->GetInstanceAlias());
                    instance = &(instance->GetParentInstance()->get());
                }

                // Generates relative path.
                return Internal::JoinInstanceAliases(instanceAliases);
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
