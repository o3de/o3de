/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Editor/Include/ScriptCanvas/Components/GraphUpgrade.h>
#include <Editor/Include/ScriptCanvas/Components/EditorGraph.h>

#include <Editor/Nodes/NodeDisplayUtils.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <ScriptCanvas/Core/ConnectionBus.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

namespace ScriptCanvasEditor
{

    namespace Helpers
    {
        static AZStd::string ConnectionToText(ScriptCanvasEditor::Graph* graph, ScriptCanvas::Endpoint& from, ScriptCanvas::Endpoint& to)
        {
            AZ_Assert(graph, "A valid graph must be provided");

            ScriptCanvas::Node* fromNode = graph->FindNode(from.GetNodeId());
            ScriptCanvas::Node* toNode = graph->FindNode(to.GetNodeId());

            ScriptCanvas::Slot* fromSlot = fromNode ? fromNode->GetSlot(from.GetSlotId()) : nullptr;
            ScriptCanvas::Slot* toSlot = toNode ? toNode->GetSlot(to.GetSlotId()) : nullptr;

            AZStd::string fromNodeName = fromNode ? fromNode->GetNodeName() : "Unknown Node";
            AZStd::string toNodeName = toNode ? toNode->GetNodeName() : "Unknown Node";

            AZStd::string fromSlotName = fromSlot ? fromSlot->GetName() : "Unknown Slot";
            AZStd::string toSlotName = toSlot ? toSlot->GetName() : "Unknown Slot";

            return AZStd::string::format("%s:%s to %s:%s", fromNodeName.c_str(), fromSlotName.c_str(), toNodeName.c_str(), toSlotName.c_str());
        }
    }

    void Start::OnEnter()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graph->m_graphCanvasSceneEntity, &GraphCanvas::GraphCanvasRequests::CreateSceneAndActivate);

        if (!graph->m_graphCanvasSceneEntity)
        {
            // TODO: Need to shut it all down.
        }

        GraphCanvas::SceneRequestBus::Event(sm->m_graphCanvasGraphId, &GraphCanvas::SceneRequests::SetEditorId, ScriptCanvasEditor::AssetEditorId);
    }

    void PreventUndo::OnEnter()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
    }
    
    void CollectData::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        sm->m_scriptCanvasId = graph->GetScriptCanvasId();

        // Remove nodes that do not have components, these could be versioning artifacts
        // or nodes that are missing due to a missing gem
        AZStd::vector<AZ::Entity*> nodesToRemove;
        for (auto* node : graph->GetGraphData()->m_nodes)
        {
            if (node->GetComponents().empty())
            {
                AZ_TracePrintf(ScriptCanvas::k_VersionExplorerWindow.data(), "Removing node due to missing components: %s\nVerify that all gems that this script relies on are enabled\n", node->GetName().c_str());

                nodesToRemove.push_back(node);
            }
        }

        for (AZ::Entity* nodeEntity : nodesToRemove)
        {
            graph->GetGraphData()->m_nodes.erase(nodeEntity);
        }

        for (const AZ::EntityId& scriptCanvasNodeId : graph->GetNodes())
        {
            sm->m_assetSanitizationSet.insert(scriptCanvasNodeId);

            ScriptCanvas::Node* scriptCanvasNode = graph->FindNode(scriptCanvasNodeId);

            sm->m_graphCanvasGraphId = graph->GetGraphCanvasGraphId();

            if (scriptCanvasNode)
            {
                sm->m_allNodes.insert(scriptCanvasNode);

                if (scriptCanvasNode->IsDeprecated())
                {
                    sm->m_deprecatedNodes.insert(scriptCanvasNode);
                }

                if (scriptCanvasNode->IsOutOfDate(graph->GetVersion()))
                {
                    sm->m_outOfDateNodes.insert(scriptCanvasNode);
                }

                if (scriptCanvasNode->IsSanityCheckRequired())
                {
                    sm->m_sanityCheckRequiredNodes.insert(scriptCanvasNode);
                }
            }
        }
    }

    IState::ExitStatus CollectData::OnExit()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();

        Log("---------------------------------------------------------------------\n");
        Log("Upgrading\n");
        Log("%d Nodes\n", sm->m_allNodes.size());

        if (sm->m_deprecatedNodes.size() > 0)
        {
            Log("%d Deprecated\n", sm->m_deprecatedNodes.size());
        }

        if (sm->m_outOfDateNodes.size() > 0)
        {
            Log("%d Out of Date\n", sm->m_outOfDateNodes.size());
        }

        if (sm->m_sanityCheckRequiredNodes.size() > 0)
        {
            Log("%d Require Additional Checks\n", sm->m_sanityCheckRequiredNodes.size());
        }
        Log("---------------------------------------------------------------------\n");

        return ExitStatus::Default;
    }

    void PreRequisites::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        graph->m_variableDataModel.Activate(sm->m_scriptCanvasId);

        graph->ConnectGraphCanvasBuses();

        GraphCanvas::SceneRequestBus::Event(sm->m_graphCanvasGraphId, &GraphCanvas::SceneRequests::SignalLoadStart);

        auto saveDataIter = graph->m_graphCanvasSaveData.find(graph->GetEntityId());

        if (saveDataIter != graph->m_graphCanvasSaveData.end())
        {
            GraphCanvas::EntitySaveDataRequestBus::Event(sm->m_graphCanvasGraphId, &GraphCanvas::EntitySaveDataRequests::ReadSaveData, (*saveDataIter->second));
        }
    }

    void UpgradeConnections::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        AZStd::vector< AZ::EntityId > connectionIds = graph->GetConnections();

        for (const AZ::EntityId& connectionId : connectionIds)
        {
            ScriptCanvas::Endpoint scriptCanvasSourceEndpoint;
            ScriptCanvas::Endpoint scriptCanvasTargetEndpoint;

            ScriptCanvas::ConnectionRequestBus::EventResult(scriptCanvasSourceEndpoint, connectionId, &ScriptCanvas::ConnectionRequests::GetSourceEndpoint);
            ScriptCanvas::ConnectionRequestBus::EventResult(scriptCanvasTargetEndpoint, connectionId, &ScriptCanvas::ConnectionRequests::GetTargetEndpoint);

            Log("Upgrade Connection: %s\n", Helpers::ConnectionToText(graph, scriptCanvasSourceEndpoint, scriptCanvasTargetEndpoint).c_str());

            AZ::EntityId graphCanvasSourceNode;

            auto scriptCanvasIter = sm->m_scriptCanvasToGraphCanvasMapping.find(scriptCanvasSourceEndpoint.GetNodeId());

            if (scriptCanvasIter != sm->m_scriptCanvasToGraphCanvasMapping.end())
            {
                graphCanvasSourceNode = scriptCanvasIter->second;
            }
            else
            {
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Could not find ScriptCanvas Node with id %llu", static_cast<AZ::u64>(scriptCanvasSourceEndpoint.GetNodeId()));
            }

            AZ::EntityId graphCanvasSourceSlotId;
            SlotMappingRequestBus::EventResult(graphCanvasSourceSlotId, graphCanvasSourceNode, &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasSourceEndpoint.GetSlotId());

            if (!graphCanvasSourceSlotId.IsValid())
            {
                // For the EBusHandler's I need to remap these to a different visual node.
                // Since multiple GraphCanvas nodes depict a single ScriptCanvas EBus node.
                if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasSourceNode) != nullptr)
                {
                    GraphCanvas::Endpoint graphCanvasEventEndpoint;
                    EBusHandlerNodeDescriptorRequestBus::EventResult(graphCanvasEventEndpoint, graphCanvasSourceNode, &EBusHandlerNodeDescriptorRequests::MapSlotToGraphCanvasEndpoint, scriptCanvasSourceEndpoint.GetSlotId());

                    graphCanvasSourceSlotId = graphCanvasEventEndpoint.GetSlotId();
                }

                if (!graphCanvasSourceSlotId.IsValid())
                {
                    AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), sm->m_deletedNodes.count(scriptCanvasSourceEndpoint.GetNodeId()) > 0, "Could not create connection(%s) for Node(%s).", connectionId.ToString().c_str(), scriptCanvasSourceEndpoint.GetNodeId().ToString().c_str());
                    graph->DisconnectById(connectionId);
                    continue;
                }
            }

            GraphCanvas::Endpoint graphCanvasTargetEndpoint;

            scriptCanvasIter = sm->m_scriptCanvasToGraphCanvasMapping.find(scriptCanvasTargetEndpoint.GetNodeId());

            if (scriptCanvasIter != sm->m_scriptCanvasToGraphCanvasMapping.end())
            {
                graphCanvasTargetEndpoint.m_nodeId = scriptCanvasIter->second;
            }
            else
            {
                AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false, "Could not find ScriptCanvas Node with id %llu", static_cast<AZ::u64>(scriptCanvasSourceEndpoint.GetNodeId()));
            }

            SlotMappingRequestBus::EventResult(graphCanvasTargetEndpoint.m_slotId, graphCanvasTargetEndpoint.GetNodeId(), &SlotMappingRequests::MapToGraphCanvasId, scriptCanvasTargetEndpoint.GetSlotId());

            if (!graphCanvasTargetEndpoint.IsValid())
            {
                // For the EBusHandler's I need to remap these to a different visual node.
                // Since multiple GraphCanvas nodes depict a single ScriptCanvas EBus node.
                if (EBusHandlerNodeDescriptorRequestBus::FindFirstHandler(graphCanvasTargetEndpoint.GetNodeId()) != nullptr)
                {
                    EBusHandlerNodeDescriptorRequestBus::EventResult(graphCanvasTargetEndpoint, graphCanvasTargetEndpoint.GetNodeId(), &EBusHandlerNodeDescriptorRequests::MapSlotToGraphCanvasEndpoint, scriptCanvasTargetEndpoint.GetSlotId());
                }

                if (!graphCanvasTargetEndpoint.IsValid())
                {
                    AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), sm->m_deletedNodes.count(scriptCanvasTargetEndpoint.GetNodeId()) > 0, "Could not create connection(%s) for Node(%s).", connectionId.ToString().c_str(), scriptCanvasTargetEndpoint.GetNodeId().ToString().c_str());
                    graph->DisconnectById(connectionId);
                    continue;
                }
            }

            AZ::EntityId graphCanvasConnectionId;
            GraphCanvas::SlotRequestBus::EventResult(graphCanvasConnectionId, graphCanvasSourceSlotId, &GraphCanvas::SlotRequests::DisplayConnectionWithEndpoint, graphCanvasTargetEndpoint);

            if (graphCanvasConnectionId.IsValid())
            {
                AZStd::any* userData = nullptr;
                GraphCanvas::ConnectionRequestBus::EventResult(userData, graphCanvasConnectionId, &GraphCanvas::ConnectionRequests::GetUserData);

                if (userData)
                {
                    (*userData) = connectionId;

                    SceneMemberMappingConfigurationRequestBus::Event(graphCanvasConnectionId, &SceneMemberMappingConfigurationRequests::ConfigureMapping, connectionId);
                }
            }
        }
    }

    void VerifySaveDataVersion::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        GraphCanvas::SceneRequestBus::Event(graph->GetGraphCanvasGraphId(), &GraphCanvas::SceneRequests::ProcessEnableDisableQueue);

        if (graph->m_graphCanvasSaveVersion != GraphCanvas::EntitySaveDataContainer::CurrentVersion)
        {
            for (auto saveDataPair : graph->m_graphCanvasSaveData)
            {
                auto graphCanvasIter = sm->m_scriptCanvasToGraphCanvasMapping.find(saveDataPair.first);
                graph->OnSaveDataDirtied(graphCanvasIter->second);
            }

            graph->m_graphCanvasSaveVersion = GraphCanvas::EntitySaveDataContainer::CurrentVersion;
            sm->m_graphNeedsDirtying = true;
        }
    }

    void SanityChecks::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        if (!sm->m_sanityCheckRequiredNodes.empty())
        {
            sm->m_graphNeedsDirtying = true;
        }

        for (auto node : sm->m_sanityCheckRequiredNodes)
        {
            if (node)
            {
                node->SanityCheckDynamicDisplay();
            }
        }

    }

    IState::ExitStatus SanityChecks::OnExit()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();

        GraphCanvas::SceneRequestBus::Event(sm->m_graphCanvasGraphId, &GraphCanvas::SceneRequests::SignalLoadEnd);
        EditorGraphNotificationBus::Event(sm->m_scriptCanvasId, &EditorGraphNotifications::OnGraphCanvasSceneDisplayed);

        return IState::ExitStatus::Default;
    }

    void UpgradeScriptEvents::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        auto currentIter = graph->GetGraphData()->m_scriptEventAssets.begin();

        while (currentIter != graph->GetGraphData()->m_scriptEventAssets.end())
        {
            if (sm->m_assetSanitizationSet.find(currentIter->first) == sm->m_assetSanitizationSet.end())
            {
                currentIter->second = {};
                currentIter = graph->GetGraphData()->m_scriptEventAssets.erase(currentIter);
                sm->m_graphNeedsDirtying = true;
            }
            else
            {
                ++currentIter;
            }
        }
    }

    void FixLeakedData::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        // Fix up leaked data elements
        auto mapIter = graph->m_graphCanvasSaveData.begin();

        while (mapIter != graph->m_graphCanvasSaveData.end())
        {
            // Deleted using the wrong id, which orphaned the SaveData. For now we want to go through and sanitize our save data to avoid keeping around a bunch
            // of old save data for no reason.
            //
            // Need to bypass our internal save data for graph canvas information
            if (sm->m_scriptCanvasToGraphCanvasMapping.find(mapIter->first) == sm->m_scriptCanvasToGraphCanvasMapping.end()
                && mapIter->first != graph->GetEntityId())
            {
                delete mapIter->second;
                mapIter = graph->m_graphCanvasSaveData.erase(mapIter);
            }
            else
            {
                ++mapIter;
            }
        }
    }

    void UpdateOutOfDateNodes::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        AZStd::unordered_set< AZ::EntityId > graphCanvasNodesToDelete;

        for (auto scriptCanvasNode : sm->m_outOfDateNodes)
        {
            graph->OnVersionConversionBegin((*scriptCanvasNode));

            auto graphCanvasNodeId = sm->m_scriptCanvasToGraphCanvasMapping[scriptCanvasNode->GetEntityId()];
            ScriptCanvas::UpdateResult updateResult = scriptCanvasNode->UpdateNode();

            graph->OnVersionConversionEnd((*scriptCanvasNode));

            switch (updateResult)
            {
            case ScriptCanvas::UpdateResult::DeleteNode:
            {
                sm->m_graphNeedsDirtying = true;
                sm->m_deletedNodes.insert(scriptCanvasNode->GetEntityId());
                graphCanvasNodesToDelete.insert(graphCanvasNodeId);
                break;
            }
            default:
            {
                sm->m_graphNeedsDirtying = true;
                break;
            }
            }
        }

        if (!graphCanvasNodesToDelete.empty())
        {
            for (auto& nodeId : sm->m_deletedNodes)
            {
                ScriptCanvas::Node* node = graph->FindNode(nodeId);
                if (node)
                {
                    Log("Deleted: %s\n", node->GetNodeName().c_str());
                }
            }

            GraphCanvas::SceneRequestBus::Event(sm->m_graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, graphCanvasNodesToDelete);
        }
    }

    void ReplaceDeprecatedConnections::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        if (!sm->m_updateReport.IsEmpty())
        {
            // currently, it is expected that there are no deleted old slots, those need manual correction
            AZ_Error(ScriptCanvas::k_VersionExplorerWindow.data(), sm->m_updateReport.m_deletedOldSlots.empty(), "Graph upgrade path: If old slots are deleted, manual upgrading is required");
            UpdateConnectionStatus(*graph, sm->m_updateReport);
        }
    }

    void ReplaceDeprecatedNodes::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        for (auto& node : sm->m_deprecatedNodes)
        {
            ScriptCanvas::NodeConfiguration nodeConfig = node->GetReplacementNodeConfiguration();
            if (nodeConfig.IsValid())
            {
                ScriptCanvas::NodeUpdateSlotReport nodeUpdateSlotReport;
                auto nodeEntity = node->GetEntityId();
                auto nodeOutcome = graph->ReplaceNodeByConfig(node, nodeConfig, nodeUpdateSlotReport);
                if (nodeOutcome.IsSuccess())
                {
                    ScriptCanvas::MergeUpdateSlotReport(nodeEntity, sm->m_updateReport, nodeUpdateSlotReport);

                    sm->m_allNodes.erase(node);
                    sm->m_outOfDateNodes.erase(node);
                    sm->m_sanityCheckRequiredNodes.erase(node);
                    sm->m_graphNeedsDirtying = true;

                    auto replacedNode = nodeOutcome.GetValue();
                    sm->m_allNodes.insert(replacedNode);

                    if (replacedNode->IsOutOfDate(graph->GetVersion()))
                    {
                        sm->m_outOfDateNodes.insert(replacedNode);
                    }

                    if (replacedNode->IsSanityCheckRequired())
                    {
                        sm->m_sanityCheckRequiredNodes.insert(replacedNode);
                    }

                    Log("Replaced node (%s)\n", replacedNode->GetNodeName().c_str());
                }
            }
        }
    }

    void BuildGraphCanvasMapping::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        for (auto& scriptCanvasNode : sm->m_allNodes)
        {
            auto scriptCanvasNodeId = scriptCanvasNode->GetEntityId();

            AZ::EntityId graphCanvasNodeId = Nodes::DisplayScriptCanvasNode(sm->m_graphCanvasGraphId, scriptCanvasNode);
            sm->m_scriptCanvasToGraphCanvasMapping[scriptCanvasNodeId] = graphCanvasNodeId;

            auto saveDataIter = graph->m_graphCanvasSaveData.find(scriptCanvasNodeId);

            if (saveDataIter != graph->m_graphCanvasSaveData.end())
            {
                GraphCanvas::EntitySaveDataRequestBus::Event(graphCanvasNodeId, &GraphCanvas::EntitySaveDataRequests::ReadSaveData, (*saveDataIter->second));
            }

            AZ::Vector2 position;
            GraphCanvas::GeometryRequestBus::EventResult(position, graphCanvasNodeId, &GraphCanvas::GeometryRequests::GetPosition);

            GraphCanvas::SceneRequestBus::Event(sm->m_graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, graphCanvasNodeId, position, false);

            // If the node is deprecated, we want to stomp whatever style it had saved and apply the deprecated style
            if (scriptCanvasNode->IsDeprecated())
            {
                Log("Marking node deprecated: %s\n", scriptCanvasNode->GetNodeName().c_str());
                GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "DeprecatedNodeTitlePalette");
            }
        }
    }

    void ParseGraph::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        ScriptCanvas::ValidationResults validationResults;

        // Disable the g_saveRawTranslationOuputToFile CVar during Parse (not needed for upgrade) and restore to its set value after
        bool saveRawTranslationOuputToFile = ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile;
        ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = false;


        // save parsing status before after, just because it didn't parse after doesn't mean it didn't before
        graph->Parse(validationResults);

        ScriptCanvas::Grammar::g_saveRawTranslationOuputToFile = saveRawTranslationOuputToFile;

        if (validationResults.HasErrors())
        {
            sm->MarkError("Failed to Parse");

            for (auto& err : validationResults.GetEvents())
            {
                // Register this graph as needing manual updates
                Log("%s: %s\n", err->GetIdentifier().c_str(), err->GetDescription().data());
            }
        }
    }

    void RestoreUndo::Run()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
    }

    void Finalize::Run()
    {
        EditorGraphUpgradeMachine* sm = GetStateMachine<EditorGraphUpgradeMachine>();
        auto* graph = sm->m_graph;

        if (sm->m_graphNeedsDirtying)
        {
            graph->SignalDirty();
        }

        graph->MarkVersion();
    }

    void DisplayReport::Run()
    {
        Log("Upgrade Complete\n\n\n");
    }

    void Skip::Run()
    {
        Log("Up to date (skipped)\n");
    }

    // Transitions
    int Start::EvaluateTransition()
    {
        return PreventUndo::StateID();
    }

    int PreventUndo::EvaluateTransition()
    {
        return PreRequisites::StateID();
    }

    int PreRequisites::EvaluateTransition()
    {
        return CollectData::StateID();
    }

    int CollectData::EvaluateTransition()
    {
        return ReplaceDeprecatedNodes::StateID();
    }

    int ReplaceDeprecatedNodes::EvaluateTransition()
    {
        return BuildGraphCanvasMapping::StateID();
    }

    int BuildGraphCanvasMapping::EvaluateTransition()
    {
        return ReplaceDeprecatedConnections::StateID();
    }

    int ReplaceDeprecatedConnections::EvaluateTransition()
    {
        return UpdateOutOfDateNodes::StateID();
    }

    int UpdateOutOfDateNodes::EvaluateTransition()
    {
        return UpgradeConnections::StateID();
    }

    int UpgradeConnections::EvaluateTransition()
    {
        return FixLeakedData::StateID();
    }

    int FixLeakedData::EvaluateTransition()
    {
        return UpgradeScriptEvents::StateID();
    }

    int UpgradeScriptEvents::EvaluateTransition()
    {
        return SanityChecks::StateID();
    }

    int SanityChecks::EvaluateTransition()
    {
        return VerifySaveDataVersion::StateID();
    }

    int VerifySaveDataVersion::EvaluateTransition()
    {
        return RestoreUndo::StateID();
    }

    int RestoreUndo::EvaluateTransition()
    {
        return Finalize::StateID();
    }

    int Finalize::EvaluateTransition()
    {
        return ParseGraph::StateID();
    }

    int ParseGraph::EvaluateTransition()
    {
        return DisplayReport::StateID();
    }

    int DisplayReport::EvaluateTransition()
    {
        return EXIT_STATE_ID;
    }

    int Skip::EvaluateTransition()
    {
        return EXIT_STATE_ID;
    }
    //


#define RegisterState(stateName) m_states.emplace_back(new stateName(this));

    EditorGraphUpgradeMachine::EditorGraphUpgradeMachine(Graph* graph)
        : m_graph(graph)
    {
        RegisterState(Start);
        RegisterState(PreRequisites);
        RegisterState(PreventUndo);
        RegisterState(CollectData);
        RegisterState(ReplaceDeprecatedNodes);
        RegisterState(ReplaceDeprecatedConnections);
        RegisterState(VerifySaveDataVersion);
        RegisterState(SanityChecks);
        RegisterState(UpgradeScriptEvents);
        RegisterState(UpdateOutOfDateNodes);
        RegisterState(UpgradeConnections);
        RegisterState(BuildGraphCanvasMapping);
        RegisterState(FixLeakedData);
        RegisterState(RestoreUndo);
        RegisterState(Finalize);
        RegisterState(DisplayReport);
        RegisterState(Skip);
        RegisterState(ParseGraph);
    }

    void EditorGraphUpgradeMachine::SetAsset(SourceHandle& asset)
    {
        if (m_asset != asset)
        {
            m_asset = asset;
            SetDebugPrefix(asset.Path().c_str());
        }
    }

    void EditorGraphUpgradeMachine::OnComplete(IState::ExitStatus exitStatus)
    {
        UpgradeNotificationsBus::Broadcast(&UpgradeNotifications::OnGraphUpgradeComplete, m_asset, exitStatus == IState::ExitStatus::Skipped);
        // releasing the asset at this stage of the system tick causes a memory crash
        // m_asset = {};
    }

    //////////////////////////////////////////////////////////////////////
    // State Machine Internals
    bool StateMachine::GetVerbose() const
    {
        return m_isVerbose;
    }

    void StateMachine::OnSystemTick()
    {
        IState::ExitStatus exitStatus = IState::ExitStatus::Default;

        if (m_currentState)
        {
            m_currentState->Run();

            int targetState = m_currentState->EvaluateTransition();
            if (targetState != IState::EXIT_STATE_ID)
            {
                exitStatus = m_currentState->Exit();

                auto state = AZStd::find_if(m_states.begin(), m_states.end(), [targetState](AZStd::shared_ptr<IState>& state) { if (state->GetStateId() == targetState) { return true; } return false; });

                if (state != m_states.end())
                {
                    m_currentState = *state;
                    m_currentState->Enter();
                }
                else
                {
                    AZ_Assert(false, "Target State ID: %d Not Registered (From: %s)", targetState, m_currentState->GetName());
                    m_currentState = nullptr;
                }
            }
            else
            {
                exitStatus = m_currentState->Exit();
                m_currentState = nullptr;
            }
        }

        if (!m_currentState)
        {
            AZ::SystemTickBus::Handler::BusDisconnect();

            OnComplete(m_error.empty() ? exitStatus : IState::ExitStatus::Skipped);
        }
    }

    void StateMachine::Run(int startStateID)
    {
        auto startState = AZStd::find_if(m_states.begin(), m_states.end(), [startStateID](AZStd::shared_ptr<IState>& state) { if (state->GetStateId() == startStateID) { return true; } return false; });
        if (startState != m_states.end())
        {
            m_currentState = *startState;

            m_currentState->Enter();

            AZ::SystemTickBus::Handler::BusConnect();
        }
    }

    void StateMachine::SetVerbose(bool isVerbose)
    {
        m_isVerbose = isVerbose;
    }

    const AZStd::string& StateMachine::GetDebugPrefix() const
    {
        return m_debugPrefix;
    }

    void StateMachine::SetDebugPrefix(AZStd::string_view prefix)
    {
        m_debugPrefix = prefix;
    }


}
