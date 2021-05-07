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
         * PrefabEditPublicInterface
         * Interface to expose the Public API to Edit Prefabs in the Editor.
         * Functions in this API support undo/redo and are meant to be called in UI and Editor Scripting.
         */
        class PrefabEditPublicInterface
        {
        public:
            AZ_RTTI(PrefabEditInterface, "{7152D92B-4E14-450C-8190-08BDD8FB32DA}");

            /**
             * Sets the prefab for the instance owning the entity provided as the prefab being edited.
             * @param entityId The entity whose owning prefab should be edited. An invalid id defaults to editing the level.
             */
            virtual void EditOwningPrefab(AZ::EntityId entityId) = 0;
            
            /**
             * Queries the Edit Manager to know if the provided entity is part of the prefab currently being edited.
             * @param entityId The entity whose prefab editing state we want to query.
             * @return True if the prefab owning this entity is being edited, false otherwise.
             */
            virtual bool IsOwningPrefabBeingEdited(AZ::EntityId entityId) = 0;
            
            /**
             * Queries the Edit Manager to know if the provided entity is part of a prefab that is currently in the edit stack.
             * @param entityId The entity whose prefab editing state we want to query.
             * @return True if the prefab owning this entity is in the edit stack, false otherwise.
             */
            virtual bool IsOwningPrefabInEditStack(AZ::EntityId entityId) = 0;
        };

    } // namespace Prefab
} // namespace AzToolsFramework

