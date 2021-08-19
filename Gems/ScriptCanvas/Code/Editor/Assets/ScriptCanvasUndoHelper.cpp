/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasUndoHelper.h"
#include "ScriptCanvasMemoryAsset.h"
#include <Undo/ScriptCanvasGraphCommand.h>

namespace ScriptCanvasEditor
{
    UndoHelper::UndoHelper(ScriptCanvasMemoryAsset& memoryAsset)
        : m_memoryAsset(memoryAsset)
    {
        UndoRequestBus::Handler::BusConnect(memoryAsset.GetScriptCanvasId());
    }

    UndoHelper::~UndoHelper()
    {
        UndoRequestBus::Handler::BusDisconnect();
    }

    ScriptCanvasEditor::UndoCache* UndoHelper::GetSceneUndoCache()
    {
        return m_memoryAsset.GetUndoState()->m_undoCache.get();
    }

    ScriptCanvasEditor::UndoData UndoHelper::CreateUndoData()
    {
        AZ::EntityId graphCanvasGraphId = m_memoryAsset.GetGraphId();
        ScriptCanvas::ScriptCanvasId scriptCanvasId = m_memoryAsset.GetScriptCanvasId();

        GraphCanvas::GraphModelRequestBus::Event(graphCanvasGraphId, &GraphCanvas::GraphModelRequests::OnSaveDataDirtied, graphCanvasGraphId);

        UndoData undoData;

        ScriptCanvas::GraphData* graphData{};
        ScriptCanvas::GraphRequestBus::EventResult(graphData, scriptCanvasId, &ScriptCanvas::GraphRequests::GetGraphData);

        const ScriptCanvas::VariableData* varData{};
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varData, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableDataConst);

        if (graphData && varData)
        {
            undoData.m_graphData = *graphData;
            undoData.m_variableData = *varData;

            EditorGraphRequestBus::EventResult(undoData.m_visualSaveData, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasSaveData);
        }

        return undoData;
    }

    void UndoHelper::BeginUndoBatch(AZStd::string_view label)
    {
        m_memoryAsset.GetUndoState()->BeginUndoBatch(label);
    }

    void UndoHelper::EndUndoBatch()
    {
        m_memoryAsset.GetUndoState()->EndUndoBatch();
    }

    void UndoHelper::AddUndo(AzToolsFramework::UndoSystem::URSequencePoint* sequencePoint)
    {
        if (SceneUndoState* sceneUndoState = m_memoryAsset.GetUndoState())
        {
            if (!sceneUndoState->m_currentUndoBatch)
            {
                sceneUndoState->m_undoStack->Post(sequencePoint);
            }
            else
            {
                sequencePoint->SetParent(sceneUndoState->m_currentUndoBatch);
            }
        }
    }

    void UndoHelper::AddGraphItemChangeUndo(AZStd::string_view undoLabel)
    {
        GraphItemChangeCommand* command = aznew GraphItemChangeCommand(undoLabel);
        command->Capture(m_memoryAsset, true);
        command->Capture(m_memoryAsset, false);
        AddUndo(command);
    }

    void UndoHelper::AddGraphItemAdditionUndo(AZStd::string_view undoLabel)
    {
        GraphItemAddCommand* command = aznew GraphItemAddCommand(undoLabel);
        command->Capture(m_memoryAsset, false);
        AddUndo(command);
    }

    void UndoHelper::AddGraphItemRemovalUndo(AZStd::string_view undoLabel)
    {
        GraphItemRemovalCommand* command = aznew GraphItemRemovalCommand(undoLabel);
        command->Capture(m_memoryAsset, true);
        AddUndo(command);
    }

    void UndoHelper::Undo()
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        SceneUndoState* sceneUndoState = m_memoryAsset.GetUndoState();
        if (sceneUndoState)
        {
            AZ_Warning("Script Canvas", !sceneUndoState->m_currentUndoBatch, "Script Canvas Editor has an open undo batch when performing a redo operation");

            if (sceneUndoState->m_undoStack->CanUndo())
            {
                m_status = Status::InUndo;
                sceneUndoState->m_undoStack->Undo();
                m_status = Status::Idle;

                UpdateCache();
            }
        }
    }

    void UndoHelper::Redo()
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        SceneUndoState* sceneUndoState = m_memoryAsset.GetUndoState();
        if (sceneUndoState)
        {
            AZ_Warning("Script Canvas", !sceneUndoState->m_currentUndoBatch, "Script Canvas Editor has an open undo batch when performing a redo operation");

            if (sceneUndoState->m_undoStack->CanRedo())
            {
                m_status = Status::InUndo;
                sceneUndoState->m_undoStack->Redo();
                m_status = Status::Idle;

                UpdateCache();
            }
        }
    }

    void UndoHelper::Reset()
    {
        if (SceneUndoState* sceneUndoState = m_memoryAsset.GetUndoState())
        {
            AZ_Warning("Script Canvas", !sceneUndoState->m_currentUndoBatch, "Script Canvas Editor has an open undo batch when resetting the undo stack");
            sceneUndoState->m_undoStack->Reset();
        }
    }

    bool UndoHelper::IsIdle()
    {
        return m_status == Status::Idle;
    }

    bool UndoHelper::IsActive()
    {
        return m_status != Status::Idle;
    }

    bool UndoHelper::CanUndo() const
    {
        return m_memoryAsset.GetUndoState()->m_undoStack->CanUndo();
    }

    bool UndoHelper::CanRedo() const
    {
        return m_memoryAsset.GetUndoState()->m_undoStack->CanRedo();
    }

    void UndoHelper::UpdateCache()
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId = m_memoryAsset.GetScriptCanvasId();

        UndoCache* undoCache = nullptr;
        UndoRequestBus::EventResult(undoCache, scriptCanvasId, &UndoRequests::GetSceneUndoCache);

        if (undoCache)
        {
            undoCache->UpdateCache(scriptCanvasId);
        }
    }
}
