/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

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
            //! @return true if overrides are present on the given entity id.
            virtual bool AreOverridesPresent(AZ::EntityId entityId) = 0;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
