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
#include <AzToolsFramework/Prefab/Instance/Instance.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabInstanceUtils
        {
            /**
             * Climbs up the instance hierarchy tree from startInstance.
             * Stops when it hits either the targetInstance or the root.
             * Then return relative path between targetInstance and startInstance.
             * @param startInstance An instance as starting point from an instance hierarchy tree.
             * @param targetInstance Target instance to climb up to from startInstance.
             * @return Pointer to the targetInstance or the root (if targetInstance is not an ancestor
             * of startInstance), and targetInstance's relative path to startInstance.
             */
            AZStd::pair<const Instance*, AZStd::vector<InstanceOptionalConstReference>> ClimbUpToTargetOrRootInstance(
                const Instance* startInstance, const Instance* targetInstance);

            /**
             * Climbs up the instance hierarchy tree from startInstance.
             * Stops when it hits either the targetInstance or the root.
             * Then return a string of relative path between startInstance and targetInstance.
             * @param startInstance An instance as starting point from an instance hierarchy tree.
             * @param targetInstance Target instance to climb up to from startInstance.
             * @return Pointer to the targetInstance or the root (if targetInstance is not an ancestor
             * of startInstance), and a string of startInstance's relative path to targetInstance.
             */
            AZStd::pair<const Instance*, AZStd::string> GetRelativePathBetweenInstances(
                const Instance* startInstance, const Instance* targetInstance);

        } // namespace PrefabInstanceUtils
    } // namespace Prefab
} // namespace AzToolsFramework

