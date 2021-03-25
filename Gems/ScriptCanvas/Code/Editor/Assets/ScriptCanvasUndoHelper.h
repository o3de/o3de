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
