/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <AzToolsFramework/Entity/EntityTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        using CreatePrefabResult = AZ::Outcome<AZ::EntityId, AZStd::string>;
        using InstantiatePrefabResult = AZ::Outcome<AZ::EntityId, AZStd::string>;
        using DuplicatePrefabResult = AZ::Outcome<EntityIdList, AZStd::string>;
        using PrefabOperationResult = AZ::Outcome<void, AZStd::string>;
        using CreateSpawnableResult = AZ::Outcome<AZ::Data::AssetId, AZStd::string>;

        /**
        * The primary purpose of this bus is to facilitate writing automated tests for prefabs.
        * It calls PrefabPublicInterface internally to talk to the prefab system.
        * If you would like to integrate prefabs into your system, please call PrefabPublicInterface
        * directly for better performance.
        */
        class PrefabPublicRequests
            : public AZ::EBusTraits
        {
        public:
            using Bus = AZ::EBus<PrefabPublicRequests>;

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            virtual ~PrefabPublicRequests() = default;

            /**
             * Create a prefab out of the entities provided, at the path provided, and keep it in memory.
             * Automatically detects descendants of entities, and discerns between entities and child instances.
             * Return an outcome object with an container entity id of the prefab created if creation succeeded;
             * on failure, it comes with an error message detailing the cause of the error.
             */
            virtual CreatePrefabResult CreatePrefabInMemory(
                const EntityIdList& entityIds, AZStd::string_view filePath) = 0;

            /**
             * Instantiate a prefab from a prefab file.
             * Return an outcome object with an container entity id of the prefab instantiated if instantiation succeeded; 
             * on failure, it comes with an error message detailing the cause of the error.
             */
            virtual InstantiatePrefabResult InstantiatePrefab(
                AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position) = 0;

            /**
             * Deletes all entities and their descendants from the owning instance. Bails if the entities don't
             * all belong to the same instance.
             * Return an outcome object; on failure, it comes with an error message detailing the cause of the error.
             */
            virtual PrefabOperationResult DeleteEntitiesAndAllDescendantsInInstance(const EntityIdList& entityIds) = 0;

            /**
              * If the entity id is a container entity id, detaches the prefab instance corresponding to it. This includes converting
              * the container entity into a regular entity and putting it under the parent prefab, removing the link between this
              * instance and the parent, removing links between this instance and its nested instances, and adding entities directly
              * owned by this instance under the parent instance.
              * Bails if the entity is not a container entity or belongs to the level prefab instance.
              * Return an outcome object; on failure, it comes with an error message detailing the cause of the error.
              */
            virtual PrefabOperationResult DetachPrefab(const AZ::EntityId& containerEntityId) = 0;

            /**
              * The same operation as DetachPrefab, except it also removes the container entity and reparents its children 
              * to the parent of the container entity.  This operation is the opposite operation of creating a prefab, which
              * creates the container entity and reparents the children to the container entity.
              * Note that the above function was the original API call, which originally kept the 
              * container entities.   In order to ensure the API is stable for callers, the above
              * function's outcome is left unchanged, and the new functionality to remove the container
              * entity is instead added to a new API call.
              */
            virtual PrefabOperationResult DetachPrefabAndRemoveContainerEntity(const AZ::EntityId& containerEntityId) = 0;

            /**
              * Duplicates all entities in the owning instance. Bails if the entities don't all belong to the same instance.
              * Return an outcome object with a list of ids of given entities' duplicates if duplication succeeded;
              * on failure, it comes with an error message detailing the cause of the error.
              */
            virtual DuplicatePrefabResult DuplicateEntitiesInInstance(const EntityIdList& entityIds) = 0;

            /**
             * Get the file path to the prefab file for the prefab instance owning the entity provided.
             * Returns the path to the prefab, or an empty path if the entity is owned by the level.
             */
            virtual AZStd::string GetOwningInstancePrefabPath(AZ::EntityId entityId) const = 0;

            /**
             * Convert a prefab on given file path with given name to in-memory spawnable asset. 
             * Returns the asset id of the produced spawnable if creation succeeded;
             * on failure, it comes with an error message detailing the cause of the error.
             */
            virtual CreateSpawnableResult CreateInMemorySpawnableAsset(AZStd::string_view prefabFilePath, AZStd::string_view spawnableName) = 0;

            /**
             * Remove in-memory spawnable asset with given name.
             * Return an outcome object; on failure, it comes with an error message detailing the cause of the error.
             */
            virtual PrefabOperationResult RemoveInMemorySpawnableAsset(AZStd::string_view spawnableName) = 0;

            /**
             * Return whether an in-memory spawnable with given name exists or not.
             */
            virtual bool HasInMemorySpawnableAsset(AZStd::string_view spawnableName) const = 0;

            /**
             * Return an asset id of a spawnalbe with given name. Invalid asset id will be returned if the spawnable doesn't exist.
             */
            virtual AZ::Data::AssetId GetInMemorySpawnableAssetId(AZStd::string_view spawnableName) const = 0;

            /**
             * Remove all the in-memory spawnable assets.
             */
            virtual void RemoveAllInMemorySpawnableAssets() = 0;
        };

        using PrefabPublicRequestBus = AZ::EBus<PrefabPublicRequests>;

    } // namespace Prefab
} // namespace AzToolsFramework
