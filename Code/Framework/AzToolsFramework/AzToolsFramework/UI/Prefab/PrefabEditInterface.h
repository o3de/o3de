/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
         * Interface to expose the API to Edit Prefabs in the Editor.
         */
        class PrefabEditInterface
        {
        public:
            AZ_RTTI(PrefabEditInterface, "{DABB1D43-3760-420E-9F1E-5104F0AFF167}");

            /**
             * Sets the prefab for the instance owning the entity provided as the prefab being edited.
             * @param entityId The entity whose owning prefab should be edited.
             */
            virtual void EditOwningPrefab(AZ::EntityId entityId) = 0;
            
            /**
             * Queries the Edit Manager to know if the provided entity is part of the prefab currently being edited.
             * @param entityId The entity whose prefab editing state we want to query.
             * @return True if the prefab owning this entity is being edited, false otherwise.
             */
            virtual bool IsOwningPrefabBeingEdited(AZ::EntityId entityId) = 0;
        };

    } // namespace Prefab
} // namespace AzToolsFramework

