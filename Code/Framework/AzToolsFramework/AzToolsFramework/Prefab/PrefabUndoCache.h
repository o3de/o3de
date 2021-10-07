/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Undo/UndoCacheInterface.h>

namespace AzToolsFramework
{
    namespace UndoSystem
    {
        class URSequencePoint;
    }

    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class InstanceToTemplateInterface;

        class PrefabUndoCache
            : UndoSystem::UndoCacheInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabUndoCache, AZ::SystemAllocator, 0);

            virtual ~PrefabUndoCache() = default;

            void Initialize();
            void Destroy();

            // UndoCacheInterface...
            void UpdateCache(const AZ::EntityId& entityId) override;
            void PurgeCache(const AZ::EntityId& entityId) override;
            void Clear() override;
            void Validate(const AZ::EntityId& entityId) override;

            // Retrieve the last known state for an entity
            bool Retrieve(const AZ::EntityId& entityId, PrefabDom& outDom, AZ::EntityId& parentId);

            // Store dom as the cached state of entityId
            void Store(const AZ::EntityId& entityId, PrefabDom&& dom, const AZ::EntityId& parentId);

        private:
            struct PrefabUndoCacheItem
            {
                PrefabDom dom;
                AZ::EntityId parentId;
            };
            typedef AZStd::unordered_map<AZ::EntityId, PrefabUndoCacheItem> EntityCache;
            EntityCache m_entitySavedStates;

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
        };

    } // namespace Prefab
} // namespace AzToolsFramework
