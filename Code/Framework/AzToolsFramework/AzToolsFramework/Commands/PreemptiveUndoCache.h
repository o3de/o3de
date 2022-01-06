/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef PREEMPTIVE_UNDO_CACHE_H
#define PREEMPTIVE_UNDO_CACHE_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzToolsFramework/Undo/UndoCacheInterface.h>

// Enable to generate warnings for entity data changes not caught by undo batches.
#if defined(AZ_DEBUG_BUILD) && !defined(ENABLE_UNDOCACHE_CONSISTENCY_CHECKS)
#   define ENABLE_UNDOCACHE_CONSISTENCY_CHECKS
#endif

namespace AzToolsFramework
{
    // The purpose of the pre-emptive Undo Cache is to keep a snapshot of the last known state of any undoable objects
    // so that the user does not have to inform us before they make a change, only after they make a change
    // it also allows us to detect errors with change notification and not have to do multiple snapshots (before and after)
    class PreemptiveUndoCache
        : UndoSystem::UndoCacheInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(PreemptiveUndoCache, AZ::SystemAllocator, 0);

        static PreemptiveUndoCache* Get();
        PreemptiveUndoCache();
        virtual ~PreemptiveUndoCache();

        void RegisterToUndoCacheInterface();

        // UndoCacheInterface...
        void UpdateCache(const AZ::EntityId& entityId) override;
        void PurgeCache(const AZ::EntityId& entityId) override;
        void Clear() override;
        void Validate(const AZ::EntityId& entityId) override;

        // whenever an entity appears in the world, as an atomic operation (ie, after all of its loading is complete, and the entity is active)
        // we snapshot it here. then, whenever an entity changes, we have what it used to be.
        typedef AZStd::vector<AZ::u8> CacheLineType;

        // retrieve the last known state for an entity
        const CacheLineType& Retrieve(const AZ::EntityId& entityId);

    protected:
        typedef AZStd::unordered_map<AZ::EntityId, CacheLineType> EntityStateMap;

        EntityStateMap m_EntityStateMap;

        CacheLineType m_Empty;
    };
} // namespace AzToolsFramework

#pragma once

#endif
