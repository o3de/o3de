/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;

        namespace PrefabInstanceUtils
        {
            //! Climbs up the instance hierarchy tree from the given start instance untils it hits
            //! the target instance or the root instance.
            //! @param relativePath The returned relative path to the start instance from target or root.
            //! @param startInstance The instance as the starting point from an instance hierarchy tree.
            //! @param targetInstance The instance to climb up to. If not provided, root instance will be hit.
            //! @return Pointer to the target instance or the root instance.
            const Instance* ClimbUpToTargetOrRootInstance(
                AZStd::string& relativePath, const Instance& startInstance, InstanceOptionalConstReference targetInstance = AZStd::nullopt);

            //! Generates a relative path from a parent instance to its child instance.
            //! @param parentInstance The parent instance that the path points from.
            //! @param childInstance The child instance that the path points to.
            //! @return The relative path string.
            AZStd::string GetRelativePathBetweenInstances(const Instance& parentInstance, const Instance& childInstance);

            //! Checks if the child instance is a descendant of the parent instance (or ancestor vice versa).
            //! @param childInstance The given child instance.
            //! @param parentInstance The given parent instance.
            //! @return bool on whether the relation is valid.
            bool IsHierarchical(const Instance& childInstance, const Instance& parentInstance);

            //! Checks if the child instance is a proper descendant (not inclusive) of the parent instance (or ancestor vice versa).
            //! @param childInstance The given child instance.
            //! @param parentInstance The given parent instance.
            //! @return bool on whether the relation is valid.
            bool IsProperlyHierarchical(const Instance& childInstance, const Instance& parentInstance);

            //! Checks if two instances are identical by address.
            //! @param instanceA The given instance.
            //! @param instanceB The given instance to be compared with.
            //! @return bool on whether the two instances are idendical or both are nullopt.
            bool CompareInstances(InstanceOptionalConstReference instanceA, InstanceOptionalConstReference instanceB);
        } // namespace PrefabInstanceUtils
    } // namespace Prefab
} // namespace AzToolsFramework
