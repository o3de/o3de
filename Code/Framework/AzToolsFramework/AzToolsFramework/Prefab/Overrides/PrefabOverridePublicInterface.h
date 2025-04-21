/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/DOM/DomPath.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabOverridePublicInterface
        {
        public:
            AZ_RTTI(PrefabOverridePublicInterface, "{19F080A2-BDD7-476F-AA50-C1581401FC81}");

            //! Checks whether overrides are present on the given entity id. The prefab that creates the overrides is identified
            //! by the class implmenting this interface based on certain selections in the editor. eg: the prefab currently being edited.
            //! @param entityId The id of the entity to check for overrides.
            //! @param relativePathFromEntity The relative path from the entity. This can be used to query about overrides on components
            //!        and their properties
            //! @return true if overrides are present on the given entity id.
            virtual bool AreOverridesPresent(AZ::EntityId entityId, AZStd::string_view relativePathFromEntity = {}) = 0;

            //! Checks whether overrides are present on the given component. The prefab that creates the overrides is identified
            //! by the class implmenting this interface based on certain selections in the editor. eg: the prefab currently being edited.
            //! @param entityComponentIdPair The entity id - component id pair of the component to check for overrides.
            //! @return true if overrides are present on the given entityComponentIdPair.
            virtual bool AreComponentOverridesPresent(AZ::EntityComponentIdPair entityComponentIdPair) = 0;

            //! Gets the override type on the given entity id. The prefab that creates the overrides is identified
            //! by the class implmenting this interface based on certain selections in the editor. eg: the prefab currently being edited.
            //! @param entityId The id of the entity for which to get the override type.
            //! @return an override type if an override exists on the given entity id.
            virtual AZStd::optional<OverrideType> GetEntityOverrideType(AZ::EntityId entityId) = 0;

            //! Gets the override type on the given component. The prefab that creates the overrides is identified
            //! by the class implmenting this interface based on certain selections in the editor. eg: the prefab currently being edited.
            //! @param entityComponentIdPair The entityId and componentId for which to get the override type.
            //! @return an override type if an override exists on the given component.
            virtual AZStd::optional<OverrideType> GetComponentOverrideType(const AZ::EntityComponentIdPair& entityComponentIdPair) = 0;

            //! Revert overrides on the entity matching the entity id. Returns false if no overrides are present on the entity.
            //! @param entityId The id of the entity on which overrides should be reverted.
            //! @param relativePathFromEntity The relative path from the entity. This can be used to revert overrides on properties.
            //! @return Whether overrides are successfully reverted on the entity.
            virtual bool RevertOverrides(AZ::EntityId entityId, AZStd::string_view relativePathFromEntity = {}) = 0;

            //! Revert overrides on the given component. Returns false if no overrides are present on the component.
            //! @param entityComponentIdPair The entityId and componentId on which overrides should be reverted.
            //! @return Whether overrides are successfully reverted on the component.
            virtual bool RevertComponentOverrides(const AZ::EntityComponentIdPair& entityComponentIdPair) = 0;

            //! Apply overrides on the given component. Returns false if no overrides are present on the component.
            //! @param entityComponentIdPair The entityId and componentId on which overrides should be applied.
            //! @return Whether overrides are successfully reverted on the component.
            virtual bool ApplyComponentOverrides(const AZ::EntityComponentIdPair& entityComponentIdPair) = 0;
        };

        class PrefabOverridePublicRequests : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        };

        using PrefabOverridePublicRequestBus = AZ::EBus<PrefabOverridePublicInterface, PrefabOverridePublicRequests>;
    } // namespace Prefab
} // namespace AzToolsFramework
