/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        /*!
         * PrefabEditInterface
         * Internal interface exposing the Prefab Edit API.
         */
        class PrefabEditInterface
        {
        public:
            AZ_RTTI(PrefabEditInterface, "{DABB1D43-3760-420E-9F1E-5104F0AFF167}");

            /**
             * Sets the prefab for the instance owning the entity provided as the prefab being edited.
             * @param entityId The entity whose owning prefab should be edited.
             */
            virtual void EditPrefab(AZ::EntityId entityId) = 0;
        };

    } // namespace Prefab
} // namespace AzToolsFramework

