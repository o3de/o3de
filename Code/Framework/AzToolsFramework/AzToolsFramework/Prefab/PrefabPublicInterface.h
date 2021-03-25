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
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        typedef AZ::Outcome<void, AZStd::string> PrefabOperationResult;

        /*!
         * PrefabPublicInterface
         * Interface to expose Prefab functionality directly to UI and Scripting.
         * Functions will correctly call the Undo/Redo system under the hood.
         */
        class PrefabPublicInterface
        {
        public:
            AZ_RTTI(PrefabPublicInterface, "{931AAE9D-C775-4818-9070-A2DA69489CBE}");

            /**
             * Create a prefab out of the entities provided, at the path provided.
             * Automatically detects descendants of entities, and discerns between entities and child instances.
             * @param entityIds The entities that should form the new prefab (along with their descendants).
             * @param filePath The path for the new prefab file.
             * @return An outcome object; on failure, it comes with an error message detailing the cause of the error.
             */
            virtual PrefabOperationResult CreatePrefab(const AZStd::vector<AZ::EntityId>& entityIds, AZStd::string_view filePath) = 0;

            /**
             * Instantiate a prefab from a prefab file.
             * @param filePath The path to the prefab file to instantiate.
             * @param parent The entity the prefab should be a child of in the transform hierarchy.
             * @param position The position in world space the prefab should be instantiated in.
             * @return An outcome object; on failure, it comes with an error message detailing the cause of the error.
             */
            virtual PrefabOperationResult InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, AZ::Vector3 position) = 0;
            
            /**
             * Detects if an entity is the container entity for its owning prefab instance.
             * @param entityId The entity to query.
             * @return True if the entity is the container entity for its owning prefab instance, false otherwise.
             */
            virtual bool IsInstanceContainerEntity(AZ::EntityId entityId) = 0;
            
            /**
             * Gets the entity id for the instance container of the owning instance.
             * @param entityId The id of the entity to query.
             * @return The entity id of the instance container owning the queried entity.
             */
            virtual AZ::EntityId GetInstanceContainerEntityId(AZ::EntityId entityId) = 0;
        };


    } // namespace Prefab
} // namespace AzToolsFramework

