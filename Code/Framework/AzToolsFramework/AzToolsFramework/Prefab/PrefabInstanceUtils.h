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
            /**
             * Climbs up the instance hierarchy tree from startInstance.
             * Stops when it hits either the targetInstance or the root.
             */
            AZStd::pair<const Instance*, AZStd::string> ClimbUpToTargetInstance(
                const Instance* startInstance, const Instance* targetInstance);

        } // namespace PrefabInstanceUtils
    } // namespace Prefab
} // namespace AzToolsFramework

