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
            AZ_CLASS_ALLOCATOR(PrefabUndoCache, AZ::SystemAllocator);

            virtual ~PrefabUndoCache() = default;

            void Initialize();
            void Destroy();

            // UndoCacheInterface...
            void UpdateCache(const AZ::EntityId& entityId) override;
            void PurgeCache(const AZ::EntityId& entityId) override;
            void Clear() override;
            void Validate(const AZ::EntityId& entityId) override;

            // Retrieve the last known parent state for an entity
            bool Retrieve(const AZ::EntityId& entityId, AZ::EntityId& parentId);

            // Store the parent state of an entity
            void Store(const AZ::EntityId& entityId, const AZ::EntityId& parentId);

        private:
            AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_parentEntityCache;

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
        };

    } // namespace Prefab
} // namespace AzToolsFramework
