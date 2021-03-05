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

        SceneUndoState(AzToolsFramework::UndoSystem::IUndoNotify* undoNotify);
        ~SceneUndoState();

        void BeginUndoBatch(AZStd::string_view label);
        void EndUndoBatch();

        AZStd::unique_ptr<UndoCache> m_undoCache;
        AZStd::unique_ptr<AzToolsFramework::UndoSystem::UndoStack> m_undoStack;

        AzToolsFramework::UndoSystem::URSequencePoint* m_currentUndoBatch = nullptr;
    };
}
