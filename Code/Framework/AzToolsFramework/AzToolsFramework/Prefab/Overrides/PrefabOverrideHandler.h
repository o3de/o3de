/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPath.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabSystemComponentInterface;
        class PrefabOverrideHandler
        {
        public:
            PrefabOverrideHandler();
            ~PrefabOverrideHandler();

            //! Checks whether overrides are present on the link object matching the linkId at the provided path.
            //! @param path The path to check for overrides on the link object.
            //! @param linkId The id of the link object to check for overrides.
            //! @return true if overrides are present at the given path on the link object matching the link id.
            bool AreOverridesPresent(AZ::Dom::Path path, LinkId linkId) const;

            //! Gets the patch type on the link object matching the linkId at the provided path.
            //! @param path The path to get override type on the link object.
            //! @param linkId The id of the link object to get overrides on.
            //! @return a patch type if an override is present at the given path on the link object matching the link id.
            AZStd::optional<PatchType> GetPatchType(AZ::Dom::Path path, LinkId linkId) const;

            //! Revert overrides corresponding to the provided path from the overrides stored in the link matching the link id.
            //! @param path The path at which overrides should be reverted from.
            //! @param linkId The id of the link from which overrides should be reverted.
            //! @return Whether overrides are reverted successfully.
            bool RevertOverrides(AZ::Dom::Path path, LinkId linkId) const;

        private:
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
