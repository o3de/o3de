/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string_view.h>

namespace AzToolsFramework
{
    using EntityIdList = AZStd::vector<AZ::EntityId>;

    namespace Prefab
    {
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
             */
            virtual bool CreatePrefabInMemory(
                const EntityIdList& entityIds, AZStd::string_view filePath) = 0;

            /**
             * Instantiate a prefab from a prefab file.
             */
            virtual AZ::EntityId InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position) = 0;

            /**
             * Deletes all entities and their descendants from the owning instance. Bails if the entities don't
             * all belong to the same instance.
             */
            virtual bool DeleteEntitiesAndAllDescendantsInInstance(const EntityIdList& entityIds) = 0;

        };

        using PrefabPublicRequestBus = AZ::EBus<PrefabPublicRequests>;

    } // namespace Prefab
} // namespace AzToolsFramework
