/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Bus/UndoBus.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

namespace ScriptCanvasEditor
{
    class ScopedUndoBatch
    {
    public:
        ScopedUndoBatch(AZStd::string_view undoLabel);
        ~ScopedUndoBatch();

    private:
        ScopedUndoBatch(const ScopedUndoBatch&) = delete;
        ScopedUndoBatch& operator=(const ScopedUndoBatch&) = delete;
    };

    // This cache maintains the previous state of the Script Canvas graph items recorded for Undo
    class UndoCache
    {
    public:
        AZ_CLASS_ALLOCATOR(UndoCache, AZ::SystemAllocator, 0);

        UndoCache() = default;
        ~UndoCache() = default;

        // Update the graph item within the cache
        void UpdateCache(ScriptCanvas::ScriptCanvasId);

        // remove the graph item from the cache
        void PurgeCache(ScriptCanvas::ScriptCanvasId);

        // retrieve the last known state for the graph item
        const AZStd::vector<AZ::u8>& Retrieve(ScriptCanvas::ScriptCanvasId);

        // Populate the cache from a ScriptCanvas Entity graph entity
        void PopulateCache(ScriptCanvas::ScriptCanvasId);

        // clear the entire cache:
        void Clear();

    protected:

        AZStd::unordered_map <ScriptCanvas::ScriptCanvasId, AZStd::vector<AZ::u8>> m_dataMap; ///< Stores an Entity Id to serialized graph data(Node/Connection) map

        AZStd::vector<AZ::u8> m_emptyData;
    };

    class SceneUndoState
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneUndoState, AZ::SystemAllocator, 0);

        SceneUndoState() = default;
        SceneUndoState(AzToolsFramework::UndoSystem::IUndoNotify* undoNotify);
        ~SceneUndoState();

        void BeginUndoBatch(AZStd::string_view label);
        void EndUndoBatch();

        AZStd::unique_ptr<UndoCache> m_undoCache;
        AZStd::unique_ptr<AzToolsFramework::UndoSystem::UndoStack> m_undoStack;

        AzToolsFramework::UndoSystem::URSequencePoint* m_currentUndoBatch = nullptr;
    };
}
