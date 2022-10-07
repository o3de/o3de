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

        class PrefabOverridePublicHandler : private PrefabOverridePublicInterface
        {
        public:
            PrefabOverridePublicHandler();
            virtual ~PrefabOverridePublicHandler();

        private:
            //! Checks whether overrides are present on the given entity id. Overrides can come from any ancestor prefab but
            //! this function specifically checks for overrides from the focused prefab.
            //! @param entityId The id of the entity to check for overrides.
            //! @return true if overrides are present on the given entity id from the focused prefab.
            bool AreOverridesPresent(AZ::EntityId entityId) override;

            PrefabOverrideHandler m_prefabOverrideHandler;

            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
            PrefabFocusInterface* m_prefabFocusInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
