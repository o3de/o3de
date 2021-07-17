/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <ScriptCanvas/Bus/UndoBus.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasMemoryAsset;

    // Helper class that provides the implementation for UndoRequestBus
    class UndoHelper : UndoRequestBus::Handler
    {
    public:

        UndoHelper(ScriptCanvasMemoryAsset& memoryAsset);
        ~UndoHelper();

        UndoCache* GetSceneUndoCache()  override;
        UndoData CreateUndoData() override;

        void BeginUndoBatch(AZStd::string_view label) override;
        void EndUndoBatch() override;
        void AddUndo(AzToolsFramework::UndoSystem::URSequencePoint* seqPoint) override;
        void AddGraphItemChangeUndo(AZStd::string_view undoLabel) override;
        void AddGraphItemAdditionUndo(AZStd::string_view undoLabel) override;
        void AddGraphItemRemovalUndo(AZStd::string_view undoLabel) override;
        void Undo() override;
        void Redo() override;
        void Reset() override;

        bool IsActive() override;
        bool IsIdle() override;

        bool CanUndo() const override;
        bool CanRedo() const override;

    private:

        void UpdateCache();

        enum class Status
        {
            Idle,
            InUndo
        };

        Status m_status = Status::Idle;

        ScriptCanvasMemoryAsset& m_memoryAsset;
    };
}
