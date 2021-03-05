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
#include "precompiled.h"

#include <QVBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/Nodes/NodeUtils.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Core/GraphBus.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuActions.h>
#include <GraphCanvas/Utils/NodeNudgingController.h>
#include <GraphCanvas/Utils/ConversionUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Nodes/NodeUtils.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>

#include "ScriptCanvasContextMenus.h"
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Slot.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <ScriptCanvas/Libraries/Core/ExecutionNode.h>
#include <GraphCanvas/Types/Endpoint.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
namespace ScriptCanvasEditor
{
    //////////////////////////////
    // AddSelectedEntitiesAction
    //////////////////////////////

    AddSelectedEntitiesAction::AddSelectedEntitiesAction(QObject* parent)
        : GraphCanvas::ContextMenuAction("", parent)
    {
    }

    GraphCanvas::ActionGroupId AddSelectedEntitiesAction::GetActionGroupId() const
    {
        return AZ_CRC("EntityActionGroup", 0x17e16dfe);
    }

    void AddSelectedEntitiesAction::RefreshAction(const GraphCanvas::GraphId&, const AZ::EntityId&)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        setEnabled(!selectedEntities.empty());

        if (selectedEntities.size() <= 1)
        {
            setText("Reference selected entity");
        }
        else
        {
            setText("Reference selected entities");
        }
    }

    GraphCanvas::ContextMenuAction::SceneReaction AddSelectedEntitiesAction::TriggerAction(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePos)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

        AZ::Vector2 addPosition = scenePos;

        for (const AZ::EntityId& id : selectedEntities)
        {
            NodeIdPair nodePair = Nodes::CreateEntityNode(id, scriptCanvasId);
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, addPosition);
            addPosition += AZ::Vector2(20, 20);
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
    }

    ////////////////////////////
    // EndpointSelectionAction
    ////////////////////////////

    EndpointSelectionAction::EndpointSelectionAction(const GraphCanvas::Endpoint& proposedEndpoint)
        : QAction(nullptr)
        , m_endpoint(proposedEndpoint)
    {
        AZStd::string name;
        GraphCanvas::SlotRequestBus::EventResult(name, proposedEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetName);

        AZStd::string tooltip;
        GraphCanvas::SlotRequestBus::EventResult(tooltip, proposedEndpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetTooltip);

        setText(name.c_str());
        setToolTip(tooltip.c_str());
    }

    const GraphCanvas::Endpoint& EndpointSelectionAction::GetEndpoint() const
    {
        return m_endpoint;
    }

    ////////////////////////////////////
    // RemoveUnusedVariablesMenuAction
    ////////////////////////////////////

    RemoveUnusedVariablesMenuAction::RemoveUnusedVariablesMenuAction(QObject* parent)
        : SceneContextMenuAction("Variables", parent)
    {
        setToolTip("Removes all of the unused variables from the active graph");
    }

    void RemoveUnusedVariablesMenuAction::RefreshAction([[maybe_unused]] const GraphCanvas::GraphId& graphId, [[maybe_unused]] const AZ::EntityId& targetId)
    {
        setEnabled(true);
    }

    bool RemoveUnusedVariablesMenuAction::IsInSubMenu() const
    {
        return true;
    }

    AZStd::string RemoveUnusedVariablesMenuAction::GetSubMenuPath() const
    {
        return "Remove Unused";
    }

    GraphCanvas::ContextMenuAction::SceneReaction RemoveUnusedVariablesMenuAction::TriggerAction(const GraphCanvas::GraphId& graphId, [[maybe_unused]] const AZ::Vector2& scenePos)
    {
        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::RemoveUnusedNodes);
        return SceneReaction::PostUndo;
    }

    ///////////////////////////////
    // SlotManipulationMenuAction
    ///////////////////////////////

    SlotManipulationMenuAction::SlotManipulationMenuAction(AZStd::string_view actionName, QObject* parent)
        : GraphCanvas::ContextMenuAction(actionName, parent)
    {
    }


    ScriptCanvas::Slot* SlotManipulationMenuAction::GetScriptCanvasSlot(const GraphCanvas::Endpoint& endpoint) const
    {
        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, endpoint.GetNodeId(), &GraphCanvas::SceneMemberRequests::GetScene);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        ScriptCanvas::Endpoint scriptCanvasEndpoint;

        {
            AZStd::any* userData = nullptr;

            GraphCanvas::SlotRequestBus::EventResult(userData, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetUserData);
            ScriptCanvas::SlotId scriptCanvasSlotId = (userData && userData->is<ScriptCanvas::SlotId>()) ? *AZStd::any_cast<ScriptCanvas::SlotId>(userData) : ScriptCanvas::SlotId();

            userData = nullptr;

            GraphCanvas::NodeRequestBus::EventResult(userData, endpoint.GetNodeId(), &GraphCanvas::NodeRequests::GetUserData);
            AZ::EntityId scriptCanvasNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();

            scriptCanvasEndpoint = ScriptCanvas::Endpoint(scriptCanvasNodeId, scriptCanvasSlotId);
        }

        ScriptCanvas::Slot* scriptCanvasSlot = nullptr;

        ScriptCanvas::GraphRequestBus::EventResult(scriptCanvasSlot, scriptCanvasId, &ScriptCanvas::GraphRequests::FindSlot, scriptCanvasEndpoint);

        return scriptCanvasSlot;
    }

    /////////////////////////////////////////
    // ConvertVariableNodeToReferenceAction
    /////////////////////////////////////////

    ConvertVariableNodeToReferenceAction::ConvertVariableNodeToReferenceAction(QObject* parent)
        : GraphCanvas::ContextMenuAction("Convert to References", parent)
    {
    }

    GraphCanvas::ActionGroupId ConvertVariableNodeToReferenceAction::GetActionGroupId() const
    {
        return AZ_CRC("VariableConversion", 0x157beab0);
    }

    void ConvertVariableNodeToReferenceAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        bool hasMultipleSelection = false;
        GraphCanvas::SceneRequestBus::EventResult(hasMultipleSelection, graphId, &GraphCanvas::SceneRequests::HasMultipleSelection);

        // This item is added only when it's valid.
        setEnabled(!hasMultipleSelection);

        m_targetId = targetId;
    }

    GraphCanvas::ContextMenuAction::SceneReaction ConvertVariableNodeToReferenceAction::TriggerAction(const GraphCanvas::GraphId& graphId, [[maybe_unused]] const AZ::Vector2& scenePos)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        bool convertedNode = false;
        EditorGraphRequestBus::EventResult(convertedNode, scriptCanvasId, &EditorGraphRequests::ConvertVariableNodeToReference, m_targetId);

        if (convertedNode)
        {
            return SceneReaction::PostUndo;
        }

        return SceneReaction::Nothing;
    }

    /////////////////////////////////////////
    // ConvertReferenceToVariableNodeAction
    /////////////////////////////////////////

    ConvertReferenceToVariableNodeAction::ConvertReferenceToVariableNodeAction(QObject* parent)
        : SlotManipulationMenuAction("Convert to Variable Node", parent)
    {
    }

    GraphCanvas::ActionGroupId ConvertReferenceToVariableNodeAction::GetActionGroupId() const
    {
        return AZ_CRC("VariableConversion", 0x157beab0);
    }


    void ConvertReferenceToVariableNodeAction::RefreshAction([[maybe_unused]] const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_targetId = targetId;

        bool enableAction = false;

        if (GraphCanvas::GraphUtils::IsSlot(m_targetId))
        {
            bool isDataSlot = false;
            GraphCanvas::SlotType slotType = GraphCanvas::SlotTypes::Invalid;
            GraphCanvas::SlotRequestBus::EventResult(slotType, m_targetId, &GraphCanvas::SlotRequests::GetSlotType);

            if (slotType == GraphCanvas::SlotTypes::DataSlot)
            {
                GraphCanvas::DataSlotRequestBus::EventResult(enableAction, m_targetId, &GraphCanvas::DataSlotRequests::CanConvertToValue);

                if (enableAction)
                {
                    GraphCanvas::DataSlotType valueType = GraphCanvas::DataSlotType::Unknown;
                    GraphCanvas::DataSlotRequestBus::EventResult(valueType, m_targetId, &GraphCanvas::DataSlotRequests::GetDataSlotType);

                    enableAction = (valueType == GraphCanvas::DataSlotType::Reference);

                    if (enableAction)
                    {
                        GraphCanvas::Endpoint endpoint;
                        GraphCanvas::SlotRequestBus::EventResult(endpoint, m_targetId, &GraphCanvas::SlotRequests::GetEndpoint);

                        ScriptCanvas::Slot* slot = GetScriptCanvasSlot(endpoint);

                        enableAction = slot->GetVariableReference().IsValid();
                    }
                }
            }
        }

        setEnabled(enableAction);
    }

    GraphCanvas::ContextMenuAction::SceneReaction ConvertReferenceToVariableNodeAction::TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        GraphCanvas::Endpoint endpoint;
        GraphCanvas::SlotRequestBus::EventResult(endpoint, m_targetId, &GraphCanvas::SlotRequests::GetEndpoint);

        GraphCanvas::ConnectionType connectionType = GraphCanvas::CT_Invalid;
        GraphCanvas::SlotRequestBus::EventResult(connectionType, m_targetId, &GraphCanvas::SlotRequests::GetConnectionType);

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        ScriptCanvas::Slot* scriptCanvasSlot = GetScriptCanvasSlot(endpoint);

        if (scriptCanvasSlot == nullptr
            || !scriptCanvasSlot->IsVariableReference())
        {
            return SceneReaction::Nothing;
        }

        // Store the variable then convert the slot to a value for the next step.
        ScriptCanvas::VariableId variableId = scriptCanvasSlot->GetVariableReference();
        GraphCanvas::DataSlotRequestBus::Event(m_targetId, &GraphCanvas::DataSlotRequests::ConvertToValue);

        AZ::EntityId createdNodeId;

        if (connectionType == GraphCanvas::CT_Input)
        {
            CreateGetVariableNodeMimeEvent createMimeEvent(variableId);

            createdNodeId = createMimeEvent.CreateSplicingNode(graphId);
        }
        else if (connectionType == GraphCanvas::CT_Output)
        {
            CreateSetVariableNodeMimeEvent createMimeEvent(variableId);

            createdNodeId = createMimeEvent.CreateSplicingNode(graphId);
        }

        if (!createdNodeId.IsValid())
        {
            return SceneReaction::Nothing;
        }

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::AddNode, createdNodeId, scenePos);

        GraphCanvas::CreateConnectionsBetweenConfig createConnectionBetweenConfig;

        createConnectionBetweenConfig.m_connectionType = GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SingleConnection;
        createConnectionBetweenConfig.m_createModelConnections = true;

        GraphCanvas::GraphUtils::CreateConnectionsBetween({ endpoint }, createdNodeId, createConnectionBetweenConfig);

        if (!createConnectionBetweenConfig.m_createdConnections.empty())
        {
            GraphCanvas::Endpoint otherEndpoint;
            GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, (*createConnectionBetweenConfig.m_createdConnections.begin()), &GraphCanvas::ConnectionRequests::FindOtherEndpoint, endpoint);

            if (otherEndpoint.IsValid())
            {
                GraphCanvas::GraphUtils::AlignSlotForConnection(otherEndpoint, endpoint);
            }
        }

        AZStd::vector<AZ::EntityId> slotIds;

        GraphCanvas::NodeRequestBus::EventResult(slotIds, endpoint.GetNodeId(), &GraphCanvas::NodeRequests::GetSlotIds);        

        GraphCanvas::ConnectionSpliceConfig spliceConfig;
        spliceConfig.m_allowOpportunisticConnections = false;

        bool connectedExecution = false;
        AZStd::vector< GraphCanvas::Endpoint > validInputSlots;

        for (auto slotId : slotIds)
        {
            GraphCanvas::SlotRequests* slotRequests = GraphCanvas::SlotRequestBus::FindFirstHandler(slotId);

            if (slotRequests == nullptr)
            {
                continue;
            }

            GraphCanvas::SlotType slotType = slotRequests->GetSlotType();

            if (slotType == GraphCanvas::SlotTypes::ExecutionSlot)
            {
                GraphCanvas::ConnectionType testConnectionType = slotRequests->GetConnectionType();

                // We only want to connect to things going in the same direction as we are.
                if (testConnectionType == connectionType)
                {
                    validInputSlots.emplace_back(endpoint.GetNodeId(), slotId);

                    AZStd::vector< AZ::EntityId > connectionIds = slotRequests->GetConnections();

                    for (const AZ::EntityId& connectionId : connectionIds)
                    {
                        if (GraphCanvas::GraphUtils::SpliceNodeOntoConnection(createdNodeId, connectionId, spliceConfig))
                        {
                            connectedExecution = true;
                        }
                    }
                }
            }

            if (!connectedExecution)
            {
                GraphCanvas::CreateConnectionsBetweenConfig fallbackConnectionConfig;

                fallbackConnectionConfig.m_connectionType = GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SinglePass;
                fallbackConnectionConfig.m_createModelConnections = true;

                GraphCanvas::GraphUtils::CreateConnectionsBetween(validInputSlots, createdNodeId, fallbackConnectionConfig);
            }
        }

        GraphCanvas::NodeNudgingController nudgingController;

        nudgingController.SetGraphId(graphId);
        nudgingController.StartNudging({ createdNodeId });
        nudgingController.FinalizeNudging();

        GraphCanvas::AnimatedPulseConfiguration animatedPulseConfig;

        animatedPulseConfig.m_enableGradient = true;
        animatedPulseConfig.m_drawColor = QColor(255, 255, 255);
        animatedPulseConfig.m_durationSec = 0.25f;

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::CreatePulseAroundSceneMember, createdNodeId, 4, animatedPulseConfig);

        return SceneReaction::PostUndo;
    }

    //////////////////////////////////
    // ExposeExecutionSlotMenuAction
    //////////////////////////////////

    ExposeSlotMenuAction::ExposeSlotMenuAction(QObject* parent)
        : GraphCanvas::SlotContextMenuAction("Expose", parent)
    {

    }

    void ExposeSlotMenuAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        bool isEnabled = false;
        EditorGraphRequestBus::EventResult(isEnabled, scriptCanvasId, &EditorGraphRequests::IsFunctionGraph);

        GraphCanvas::NodeId nodeId;
        GraphCanvas::SlotRequestBus::EventResult(nodeId, targetId, &GraphCanvas::SlotRequests::GetNode);

        GraphCanvas::SlotType slotType;
        GraphCanvas::SlotRequestBus::EventResult(slotType, targetId, &GraphCanvas::SlotRequests::GetSlotType);

        if (slotType == GraphCanvas::SlotTypes::DataSlot)
        {
            GraphCanvas::DataSlotType dataSlotType = GraphCanvas::DataSlotType::Unknown;
            GraphCanvas::DataSlotRequestBus::EventResult(dataSlotType, targetId, &GraphCanvas::DataSlotRequests::GetDataSlotType);

            if (dataSlotType != GraphCanvas::DataSlotType::Value)
            {
                isEnabled = false;
            }

            bool hasConnections = false;
            GraphCanvas::SlotRequestBus::EventResult(hasConnections, targetId, &GraphCanvas::SlotRequests::HasConnections);

            if (hasConnections)
            {
                isEnabled = false;
            }
        }

        bool isNodeling = false;
        NodeDescriptorRequestBus::EventResult(isNodeling, nodeId, &NodeDescriptorRequests::IsType, NodeDescriptorType::ExecutionNodeling);

        setEnabled(isEnabled && !isNodeling);
    }

    void ExposeSlotMenuAction::CreateNodeling(const GraphCanvas::GraphId& graphId, AZ::EntityId scriptCanvasGraphId, GraphCanvas::GraphId slotId, const AZ::Vector2& scenePos, GraphCanvas::ConnectionType connectionType)
    {
        GraphCanvas::NodeId nodeId;
        GraphCanvas::SlotRequestBus::EventResult(nodeId, slotId, &GraphCanvas::SlotRequests::GetNode);

        NodeIdPair nodePair = ScriptCanvasEditor::Nodes::CreateExecutionNodeling(scriptCanvasGraphId);

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, scenePos);

        GraphCanvas::Endpoint graphCanvasEndpoint;
        GraphCanvas::SlotRequestBus::EventResult(graphCanvasEndpoint, slotId, &GraphCanvas::SlotRequests::GetEndpoint);

        // Find the execution "nodeling"
        ScriptCanvas::Nodes::Core::ExecutionNodeling* nodeling = ScriptCanvasEditor::Nodes::GetNode<ScriptCanvas::Nodes::Core::ExecutionNodeling>(scriptCanvasGraphId, nodePair);

        // Configure the Execution node
        AZStd::string nodeTitle;
        GraphCanvas::NodeTitleRequestBus::EventResult(nodeTitle, nodeId, &GraphCanvas::NodeTitleRequests::GetTitle);

        AZStd::string name;
        GraphCanvas::SlotRequestBus::EventResult(name, slotId, &GraphCanvas::SlotRequests::GetName);

        AZStd::string fullTitle = AZStd::string::format("%s : %s", nodeTitle.c_str(), name.c_str());

        nodeling->SetDisplayName(fullTitle);

        // Set the node title, subtitle, tooltip
        GraphCanvas::NodeTitleRequestBus::Event(nodePair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTitle, fullTitle);
        GraphCanvas::NodeTitleRequestBus::Event(nodePair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetSubTitle, "Function");
        GraphCanvas::NodeRequestBus::Event(nodePair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetTooltip, name);

        // Set the connection type for the node opposite of what it actually is because we're interested in the connection type of the node we're 
        // exposing, not the type of the slot we just created
        ScriptCanvas::ConnectionType scriptCanvasConnectionType = (connectionType == GraphCanvas::CT_Input) ? ScriptCanvas::ConnectionType::Input : ScriptCanvas::ConnectionType::Output;

        ScriptCanvas::SlotDescriptor descriptor;
        descriptor.m_slotType = ScriptCanvas::SlotTypeDescriptor::Execution;
        descriptor.m_connectionType = scriptCanvasConnectionType;

        auto descriptorSlots = nodeling->GetAllSlotsByDescriptor(descriptor);

        // There should only be a single slot
        AZ_Assert(descriptorSlots.size() == 1, "Nodeling should only create one of each execution slot type.");

        const ScriptCanvas::Slot* slot = descriptorSlots.front();

        GraphCanvas::SlotId graphCanvasSlotId;
        SlotMappingRequestBus::EventResult(graphCanvasSlotId, nodePair.m_graphCanvasId, &SlotMappingRequests::MapToGraphCanvasId, slot->GetId());

        GraphCanvas::Endpoint fixedEndpoint(nodeId, slotId);

        // Automatically connect to the slot that was exposed
        AZ::EntityId connectionId;
        GraphCanvas::SlotRequestBus::EventResult(connectionId, graphCanvasSlotId, &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, fixedEndpoint);

        if (connectionId.IsValid())
        {
            GraphCanvas::Endpoint executionEndpoint(nodePair.m_graphCanvasId, graphCanvasSlotId);

            GraphCanvas::GraphUtils::AlignSlotForConnection(executionEndpoint, fixedEndpoint);
        }
        else
        {
            AZStd::unordered_set<AZ::EntityId> deletionSet = { nodePair.m_graphCanvasId };
            GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::Delete, deletionSet);
        }
    }

    GraphCanvas::ContextMenuAction::SceneReaction ExposeSlotMenuAction::TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        // Go to Execution node and allow it to be renamed
        // Make sure this stuff is restored on serialization

        ScriptCanvas::ScriptCanvasId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasId, graphId);

        const GraphCanvas::SlotId& slotId = GetTargetId();

        GraphCanvas::ConnectionType connectionType;
        GraphCanvas::SlotRequestBus::EventResult(connectionType, slotId, &GraphCanvas::SlotRequests::GetConnectionType);

        GraphCanvas::SlotType slotType;
        GraphCanvas::SlotRequestBus::EventResult(slotType, slotId, &GraphCanvas::SlotRequests::GetSlotType);

        // Will create an Execution node and connect it to this slot, nothing if the slot is already connected (the option shouldn't show)
        if (slotType == GraphCanvas::SlotTypes::ExecutionSlot && connectionType == GraphCanvas::CT_Input)
        {
            AZ::Vector2 spawnPosition = scenePos + AZ::Vector2(-200, 0);
            CreateNodeling(graphId, scriptCanvasGraphId, slotId, spawnPosition, GraphCanvas::CT_Output);
        }
        else if (slotType == GraphCanvas::SlotTypes::ExecutionSlot && connectionType == GraphCanvas::CT_Output)
        {
            AZ::Vector2 spawnPosition = scenePos + AZ::Vector2(200, 0);
            CreateNodeling(graphId, scriptCanvasGraphId, slotId, spawnPosition, GraphCanvas::CT_Input);
        }
        else if (slotType == GraphCanvas::SlotTypes::DataSlot)
        {
            const AZ::EntityId& slotId2 = GetTargetId();
            const GraphCanvas::GraphId& graphId = GetGraphId();

            GraphCanvas::Endpoint endpoint;
            GraphCanvas::SlotRequestBus::EventResult(endpoint, slotId2, &GraphCanvas::SlotRequests::GetEndpoint);

            bool promotedElement = false;
            GraphCanvas::GraphModelRequestBus::EventResult(promotedElement, graphId, &GraphCanvas::GraphModelRequests::PromoteToVariableAction, endpoint);

            if (promotedElement)
            {
                ScriptCanvas::Endpoint scEndpoint;
                EditorGraphRequestBus::EventResult(scEndpoint, scriptCanvasGraphId, &EditorGraphRequests::ConvertToScriptCanvasEndpoint, endpoint);

                if (scEndpoint.IsValid())
                {
                    ScriptCanvas::Slot* slot = nullptr;
                    ScriptCanvas::GraphRequestBus::EventResult(slot, scriptCanvasGraphId, &ScriptCanvas::GraphRequests::FindSlot, scEndpoint);

                    if (slot && slot->IsVariableReference())
                    {
                        ScriptCanvas::GraphVariable* variable = slot->GetVariable();

                        if (variable)
                        {
                            if (connectionType == GraphCanvas::CT_Input)
                            {
                                if (!variable->IsInScope(ScriptCanvas::VariableFlags::Scope::Input))
                                {
                                    variable->SetScope(ScriptCanvas::VariableFlags::Scope::Input);
                                }
                            }
                            else if (connectionType == GraphCanvas::CT_Output)
                            {
                                if (!variable->IsInScope(ScriptCanvas::VariableFlags::Scope::Output))
                                {
                                    variable->SetScope(ScriptCanvas::VariableFlags::Scope::Output);
                                }
                            }
                        }
                    }
                }
            }
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
    }
    
    /////////////////////
    // SceneContextMenu
    /////////////////////

    SceneContextMenu::SceneContextMenu(const NodePaletteModel& paletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
        : GraphCanvas::SceneContextMenu(ScriptCanvasEditor::AssetEditorId)
    {
        const bool inContextMenu = true;
        Widget::ScriptCanvasNodePaletteConfig paletteConfig(paletteModel, assetModel, inContextMenu);

        m_addSelectedEntitiesAction = aznew AddSelectedEntitiesAction(this);
        
        AddActionGroup(m_addSelectedEntitiesAction->GetActionGroupId());
        AddMenuAction(m_addSelectedEntitiesAction);

        AddNodePaletteMenuAction(paletteConfig);
    }

    void SceneContextMenu::ResetSourceSlotFilter()
    {
        m_nodePalette->ResetSourceSlotFilter();
    }

    void SceneContextMenu::FilterForSourceSlot(const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& sourceSlotId)
    {
        m_nodePalette->FilterForSourceSlot(scriptCanvasGraphId, sourceSlotId);
    }

    void SceneContextMenu::OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        GraphCanvas::SceneContextMenu::OnRefreshActions(graphId, targetMemberId);

        // Don't want to overly manipulate the state. So we only modify this when we know we want to turn it on.
        if (GraphVariablesTableView::HasCopyVariableData())
        {
            m_editorActionsGroup.SetPasteEnabled(true);
        }
    }

    void SceneContextMenu::SetupDisplayForProposal()
    {
        // Disabling all of the actions here for the proposal.
        // Allows a certain 'visual consistency' in using the same menu while
        // not providing any unusable options.
        m_editorActionsGroup.SetCutEnabled(false);
        m_editorActionsGroup.SetCopyEnabled(false);
        m_editorActionsGroup.SetPasteEnabled(false);
        m_editorActionsGroup.SetDeleteEnabled(false);
        m_editorActionsGroup.SetDuplicateEnabled(false);

        m_graphCanvasConstructGroups.SetAddBookmarkEnabled(false);
        m_graphCanvasConstructGroups.SetCommentsEnabled(false);
        m_nodeGroupPresets.SetEnabled(false);
        m_alignmentActionsGroups.SetEnabled(false);

        m_addSelectedEntitiesAction->setEnabled(false);
    }

    //////////////////////////
    // ConnectionContextMenu
    //////////////////////////

    ConnectionContextMenu::ConnectionContextMenu(const NodePaletteModel& nodePaletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
        : GraphCanvas::ConnectionContextMenu(ScriptCanvasEditor::AssetEditorId)
    {
        const bool inContextMenu = true;
        Widget::ScriptCanvasNodePaletteConfig paletteConfig(nodePaletteModel, assetModel, inContextMenu);

        AddNodePaletteMenuAction(paletteConfig);
    }

    void ConnectionContextMenu::OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        GraphCanvas::ConnectionContextMenu::OnRefreshActions(graphId, targetMemberId);

        m_connectionId = targetMemberId;
        
        // TODO: Filter nodes.
    }

    #include "Editor/View/Windows/moc_ScriptCanvasContextMenus.cpp"
}

