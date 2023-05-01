/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceToTemplateInterface;
        class PrefabFocusInterface;
        class PrefabSystemComponentInterface;

        class PrefabOverridePublicHandler : public PrefabOverridePublicRequestBus::Handler
        {
        public:
            PrefabOverridePublicHandler();
            virtual ~PrefabOverridePublicHandler();

            static void Reflect(AZ::ReflectContext* context);

        private:
            //! Checks whether overrides are present on the given entity id. Overrides can come from any ancestor prefab but
            //! this function specifically checks for overrides from the focused prefab.
            //! @param entityId The id of the entity to check for overrides.
            //! @param relativePathFromEntity The relative path from the entity. This can be used to query about overrides on components
            //!        and their properties
            //! @return true if overrides are present on the given entity id from the focused prefab.
            bool AreOverridesPresent(AZ::EntityId entityId, AZStd::string_view relativePathFromEntity = {}) override;

            //! Gets the entity override type on the given entity id. Overrides can come from any ancestor prefab but
            //! this function specifically checks for overrides from the focused prefab.
            //! @param entityId The id of the entity for which to get the override type.
            //! @return an override type if an override exists on the given entity id.
            AZStd::optional<OverrideType> GetEntityOverrideType(AZ::EntityId entityId) override;

            //! Gets the component override type on the given component. Overrides can come from any ancestor prefab but
            //! this function specifically checks for overrides from the focused prefab.
            //! @param entityComponentIdPair The entityId and componentId on which overrides should be reverted.
            //! @return an override type if an override exists on the given component.
            AZStd::optional<OverrideType> GetComponentOverrideType(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

            //! Revert overrides on the entity matching the given id from the focused prefab. Returns false if no overrides are present.
            //! @param entityId The id of the entity on which overrides should be reverted.
            //! @param relativePathFromEntity The relative path from the entity. This can be used to revert overrides on properties.
            //! @return Whether overrides are successfully reverted on the entity.
            bool RevertOverrides(AZ::EntityId entityId, AZStd::string_view relativePathFromEntity = {}) override;

            //! Revert overrides on the given component from the focused prefab. Returns false if no overrides are present.
            //! @param entityComponentIdPair The entityId and componentId on which overrides should be reverted.
            //! @return Whether overrides are successfully reverted on the component.
            bool RevertComponentOverrides(const AZ::EntityComponentIdPair& entityComponentIdPair) override;

            //! Fetches the path to the entity matching the id and the linkId corresponding to the topmost prefab in the hierarchy.
            //! @param entityId The id of the entity to use to fetch the path.
            //! @return The path and link id pair.
            AZStd::pair<AZ::Dom::Path, LinkId> GetEntityPathAndLinkIdFromFocusedPrefab(AZ::EntityId entityId);

            //! Fetches the path to the component matching the id and the linkId corresponding to the topmost prefab in the hierarchy.
            //! @param entityComponentIdPair The entityId and componentId on which overrides should be reverted.
            //! @return The path and link id pair.
            AZStd::pair<AZ::Dom::Path, LinkId> GetComponentPathAndLinkIdFromFocusedPrefab(const AZ::EntityComponentIdPair& entityComponentIdPair);

            PrefabOverrideHandler m_prefabOverrideHandler;

            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
            PrefabFocusInterface* m_prefabFocusInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
