/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Variable/VariableBus.h>

#include <GraphCanvas/Components/SceneBus.h>

#include <Editor/Undo/ScriptCanvasGraphCommand.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>

namespace ScriptCanvasEditor
{
    GraphItemCommand::GraphItemCommand(AZStd::string_view friendlyName)
        : AzToolsFramework::UndoSystem::URSequencePoint(friendlyName, 0)
    {
    }

    void GraphItemCommand::Undo()
    {
    }

    void GraphItemCommand::Redo()
    {
    }

    void GraphItemCommand::Capture(Graph*, bool)
    {
    }

    bool GraphItemCommand::Changed() const
    {
        return true;
    }

    void GraphItemCommand::RestoreItem(const AZStd::vector<AZ::u8>&)
    {
    }

    void GraphItemChangeCommand::DeleteOldGraphData(const UndoData& oldData)
    {
        EditorGraphRequestBus::Event(m_scriptCanvasId, &EditorGraphRequests::ClearGraphCanvasScene);
        ScriptCanvas::GraphVariableManagerRequestBus::Event(m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::DeleteVariableData, oldData.m_variableData);
    }

    void GraphItemChangeCommand::ActivateRestoredGraphData(const UndoData& restoredData)
    {
        // Reset VariableData on the GraphVariableManager before re-init node entities, as the GetVariableNode/SetVariable Node
        // queries the VariableRequestBus
        ScriptCanvas::GraphVariableManagerRequestBus::Event(m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::SetVariableData, restoredData.m_variableData);

        // Init Script Canvas Graph Data
        for (auto entityContainer : { restoredData.m_graphData.m_nodes })
        {
            for (AZ::Entity* entity : entityContainer)
            {
                if (entity->GetState() == AZ::Entity::State::Constructed)
                {
                    entity->Init();
                }
            }
        }

        for (auto entityContainer : { restoredData.m_graphData.m_connections })
        {
            for (AZ::Entity* entity : entityContainer)
            {
                if (entity->GetState() == AZ::Entity::State::Constructed)
                {
                    entity->Init();
                }
            }
        }

        ScriptCanvas::GraphRequestBus::Event(m_scriptCanvasId, &ScriptCanvas::GraphRequests::AddGraphData, restoredData.m_graphData);

        EditorGraphRequestBus::Event(m_scriptCanvasId, &EditorGraphRequests::UpdateGraphCanvasSaveData, restoredData.m_visualSaveData);
    }

    //// Graph Item Change Command
    GraphItemChangeCommand::GraphItemChangeCommand(AZStd::string_view friendlyName)
        : GraphItemCommand(friendlyName)
    {
    }

    void GraphItemChangeCommand::Undo()
    {
        RestoreItem(m_undoState);
    }

    void GraphItemChangeCommand::Redo()
    {
        RestoreItem(m_redoState);
    }

    void GraphItemChangeCommand::Capture(Graph* graph, bool captureUndo)
    {
        m_scriptCanvasId = graph->GetScriptCanvasId();
        m_graphCanvasGraphId = graph->GetGraphCanvasGraphId();

        UndoCache* undoCache = nullptr;
        UndoRequestBus::EventResult(undoCache, m_scriptCanvasId, &UndoRequests::GetSceneUndoCache);
        if (!undoCache)
        {
            AZ_Error("Script Canvas", false, "Unable to find ScriptCanvas Undo Cache. Most likely the Undo Manager has no active scene");
            return;
        }

        if (captureUndo)
        {
            AZ_Assert(m_undoState.empty(), "Attempting to capture undo twice");
            m_undoState = undoCache->Retrieve(m_scriptCanvasId);
            if (m_undoState.empty())
            {
                undoCache->UpdateCache(m_scriptCanvasId);
                m_undoState = undoCache->Retrieve(m_scriptCanvasId);
            }

            undoCache->UpdateCache(m_scriptCanvasId);
        }
        else
        {
            UndoData undoData;
            UndoRequestBus::EventResult(undoData, m_scriptCanvasId , &UndoRequests::CreateUndoData);

            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            m_redoState.clear();
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> byteStream(&m_redoState);
            AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&byteStream, *serializeContext, AZ::DataStream::ST_BINARY);
            if (!objStream->WriteClass(&undoData))
            {
                AZ_Assert(false, "Unable to serialize Script Canvas scene and graph data for undo/redo");
                return;
            }

            objStream->Finalize();
        }
    }

    void GraphItemChangeCommand::RestoreItem(const AZStd::vector<AZ::u8>& restoreBuffer)
    {
        if (restoreBuffer.empty())
        {
            return;
        }

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ::IO::MemoryStream byteStream(restoreBuffer.data(), restoreBuffer.size());

        UndoData oldData;
        UndoRequestBus::EventResult(oldData, m_scriptCanvasId , &UndoRequests::CreateUndoData);

        // Remove old Script Canvas data
        GraphItemCommandNotificationBus::Event(m_scriptCanvasId, &GraphItemCommandNotificationBus::Events::PreRestore, oldData);
        DeleteOldGraphData(oldData);

        UndoData restoreData;
        bool restoreSuccess = AZ::Utils::LoadObjectFromStreamInPlace(byteStream, restoreData, serializeContext, AZ::ObjectStream::FilterDescriptor(AZ::Data::AssetFilterNoAssetLoading));
        if (restoreSuccess)
        {
            ActivateRestoredGraphData(restoreData);

            UndoCache* undoCache = nullptr;
            UndoRequestBus::EventResult(undoCache, m_scriptCanvasId, &UndoRequests::GetSceneUndoCache);
            if (!undoCache)
            {
                AZ_Assert(false, "Unable to find ScriptCanvas Undo Cache. Most likely the ScriptCanvasEditor Undo Manager has not been created");
                return;
            }
            undoCache->UpdateCache(m_scriptCanvasEntityId);

            GraphItemCommandNotificationBus::Event(m_scriptCanvasId, &GraphItemCommandNotifications::PostRestore, restoreData);
        }
    }

    //// Graph Item Add Command
    GraphItemAddCommand::GraphItemAddCommand(AZStd::string_view friendlyName)
        : GraphItemChangeCommand(friendlyName)
    {
    }

    void GraphItemAddCommand::Undo()
    {
        RestoreItem(m_undoState);
    }

    void GraphItemAddCommand::Redo()
    {
        RestoreItem(m_redoState);
    }

    void GraphItemAddCommand::Capture(Graph* graph, bool)
    {
        GraphItemChangeCommand::Capture(graph, false);
    }

    //// Graph Item Removal Command
    GraphItemRemovalCommand::GraphItemRemovalCommand(AZStd::string_view friendlyName)
        : GraphItemChangeCommand(friendlyName)
    {
    }

    void GraphItemRemovalCommand::Undo()
    {
        RestoreItem(m_undoState);
    }

    void GraphItemRemovalCommand::Redo()
    {
        RestoreItem(m_redoState);
    }

    void GraphItemRemovalCommand::Capture(Graph* graph, bool)
    {
        GraphItemChangeCommand::Capture(graph, true);
    }
}
