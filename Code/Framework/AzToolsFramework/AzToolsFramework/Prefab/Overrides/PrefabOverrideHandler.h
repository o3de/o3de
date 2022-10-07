/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPath.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabOverrideHandler
        {
        public:
            //! Checks whether overrides are present on the link object matching the linkId at the provided path.
            //! @param path The path to check for overrides on the link object.
            //! @param linkId The id of the link object to check for overrides
            //! @return true if overrides are present at the given path on the link object matching the link id.
            bool AreOverridesPresent(AZ::Dom::Path path, LinkId linkId);
        };
    } // namespace Prefab
} // namespace AzToolsFramework
