/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPath.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
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

            //! Pushes overrides corresponding to the provided path to the prefab template of the prefab instance.
            //! @param path The path at which overrides should be applied.
            //! @param relativePath The relative path from the instance currently holding the overrides to the one that will receive them.
            //! @param targetInstance The instance whose prefab template the overrides will be pushed to.
            //! @return Whether overrides are applied successfully.
            bool PushOverridesToPrefab(const AZ::Dom::Path& path, AZStd::string_view relativePath, InstanceOptionalReference targetInstance) const;

            //! Pushes overrides corresponding to the provided path from one link to another.
            //! For this to work, overrides should target a descendant of both source and target links,
            //! and targetLink should be a descendant of sourceLink.
            //! @param path The path at which overrides should be applied.
            //! @param relativePath The relative path from the instance currently holding the overrides to the one that will receive them.
            //! @param sourceLinkId The id for the link currently holding the overrides.
            //! @param targetLinkId The id for the link that will be receive the overrides.
            //! @return Whether overrides are applied successfully.
            bool PushOverridesToLink(const AZ::Dom::Path& path, AZStd::string_view relativePath, LinkId sourceLinkId, LinkId targetLinkId) const;
            
        private:
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
