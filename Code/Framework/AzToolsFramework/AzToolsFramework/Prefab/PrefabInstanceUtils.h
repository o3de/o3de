/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;

        //! Defines climb-up result returned by climb-up operation.
        struct InstanceClimbUpResult
        {
            //! Instance pointer to the destination.
            const Instance* m_reachedInstance = nullptr;

            //! Flag to tell if reached instance is the target instance.
            bool m_isTargetInstanceReached = false;

            //! Instance list that contains instances that are climbed up from bottom to top.
            //! The list does not include the reached instance.
            AZStd::vector<const Instance*> m_climbedInstances;
        };

        namespace PrefabInstanceUtils
        {
            //! Climbs up the instance hierarchy tree from the given start instance until it hits
            //! the target instance or the root instance.
            //! @param startInstance The instance as the starting point in an instance hierarchy tree.
            //! @param targetInstance The instance to climb up to. If not provided, root instance will be hit.
            //! @return InstanceClimbUpResult that shows the climb-up info.
            InstanceClimbUpResult ClimbUpToTargetOrRootInstance(const Instance& startInstance, const Instance* targetInstance);

            //! Generates a relative path from a parent instance to its child instance.
            //! @param parentInstance The parent instance that the path points from.
            //! @param childInstance The child instance that the path points to.
            //! @return The relative path string. Or empty string if the parent instance is not a valid parent.
            AZStd::string GetRelativePathBetweenInstances(const Instance& parentInstance, const Instance& childInstance);

            //! Generates a relative path from a list of climbed instances.
            //! @param climbedInstances The list of climbed instances from bottom to top.
            //! @param skipTopClimbedInstance A flag to determine if returned path should include the top instance of climbedInstances.
            //! @return The relative path string.
            AZStd::string GetRelativePathFromClimbedInstances(
                const AZStd::vector<const Instance*>& climbedInstances,
                bool skipTopClimbedInstance = false);

            //! Checks if the child instance is a descendant of the parent instance.
            //! @param childInstance The given child instance.
            //! @param parentInstance The given parent instance.
            //! @return bool on whether the relation is valid. Returns true if two instances are identical.
            bool IsDescendantInstance(const Instance& childInstance, const Instance& parentInstance);
        } // namespace PrefabInstanceUtils
    } // namespace Prefab
} // namespace AzToolsFramework
