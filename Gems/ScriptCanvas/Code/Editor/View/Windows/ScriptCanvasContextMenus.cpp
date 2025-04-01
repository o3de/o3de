/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QVBoxLayout>

#include <AzCore/UserSettings/UserSettings.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Nodes/NodeCreateUtils.h>
#include <Editor/Nodes/NodeUtils.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/ScriptCanvasNodePaletteDockWidget.h>
#include <Editor/View/Widgets/VariablePanel/GraphVariablesTableView.h>
#include <Editor/View/Widgets/VariablePanel/VariableConfigurationWidget.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/NodeDescriptors/FunctionDefinitionNodeDescriptorComponent.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Utils/NodeNudgingController.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuActions.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/GraphSerialization.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Grammar/ParsingUtilitiesScriptEventExtension.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

#include <ScriptEvents/ScriptEventsBus.h>

#include "ScriptCanvasContextMenus.h"
#include "Settings.h"

namespace ScriptCanvasEditor
{
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


    ScriptCanvas::Slot* SlotManipulationMenuAction::GetScriptCanvasSlot(const GraphCanvas::Endpoint& endpoint) 
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
        return AZ_CRC_CE("VariableConversion");
    }

    void ConvertVariableNodeToReferenceAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        bool hasMultipleSelection = false;
        GraphCanvas::SceneRequestBus::EventResult(hasMultipleSelection, graphId, &GraphCanvas::SceneRequests::HasMultipleSelection);        

        m_targetId = targetId;

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        bool canConvertNode = false;
        EditorGraphRequestBus::EventResult(canConvertNode, scriptCanvasId, &EditorGraphRequests::CanConvertVariableNodeToReference, m_targetId);

        // This item is added only when it's valid
        setEnabled(canConvertNode && !hasMultipleSelection);
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
        return AZ_CRC_CE("VariableConversion");
    }


    void ConvertReferenceToVariableNodeAction::RefreshAction([[maybe_unused]] const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_targetId = targetId;

        bool enableAction = false;

        if (GraphCanvas::GraphUtils::IsSlot(m_targetId))
        {
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

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::AddNode, createdNodeId, scenePos, false);

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

        GraphCanvas::NodeId nodeId;
        GraphCanvas::SlotRequestBus::EventResult(nodeId, targetId, &GraphCanvas::SlotRequests::GetNode);

        bool canExposeSlot = false;
        EditorGraphRequestBus::EventResult(canExposeSlot, scriptCanvasId, &EditorGraphRequests::CanExposeEndpoint, GraphCanvas::Endpoint(nodeId, targetId ));        

        setEnabled(canExposeSlot);
    }

    void ExposeSlotMenuAction::CreateNodeling(const GraphCanvas::GraphId& graphId, AZ::EntityId scriptCanvasGraphId, GraphCanvas::GraphId slotId, const AZ::Vector2& scenePos, GraphCanvas::ConnectionType connectionType)
    {
        GraphCanvas::NodeId nodeId;
        GraphCanvas::SlotRequestBus::EventResult(nodeId, slotId, &GraphCanvas::SlotRequests::GetNode);

        // Set the connection type for the node opposite of what it actually is because we're interested in the connection type of the node we're 
        // exposing, not the type of the slot we just created
        ScriptCanvas::ConnectionType scriptCanvasConnectionType = (connectionType == GraphCanvas::CT_Input) ? ScriptCanvas::ConnectionType::Input : ScriptCanvas::ConnectionType::Output;
        bool isInput = (scriptCanvasConnectionType == ScriptCanvas::ConnectionType::Output);

        NodeIdPair nodePair = ScriptCanvasEditor::Nodes::CreateFunctionDefinitionNode(scriptCanvasGraphId, isInput);

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, scenePos, false);

        GraphCanvas::Endpoint graphCanvasEndpoint;
        GraphCanvas::SlotRequestBus::EventResult(graphCanvasEndpoint, slotId, &GraphCanvas::SlotRequests::GetEndpoint);

        // Find the execution "nodeling"
        ScriptCanvas::Nodes::Core::FunctionDefinitionNode* nodeling = ScriptCanvasEditor::Nodes::GetNode<ScriptCanvas::Nodes::Core::FunctionDefinitionNode>(scriptCanvasGraphId, nodePair);

        // Configure the Execution node
        AZStd::string nodeTitle;
        GraphCanvas::NodeTitleRequestBus::EventResult(nodeTitle, nodeId, &GraphCanvas::NodeTitleRequests::GetTitle);

        AZStd::string name;
        GraphCanvas::SlotRequestBus::EventResult(name, slotId, &GraphCanvas::SlotRequests::GetName);

        AZStd::string fullTitle = AZStd::string::format("%s : %s", nodeTitle.c_str(), name.c_str());

        nodeling->SetDisplayName(fullTitle);

        // Set the node title, subtitle, tooltip
        GraphCanvas::NodeTitleRequestBus::Event(nodePair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetTitle, fullTitle);
        GraphCanvas::NodeRequestBus::Event(nodePair.m_graphCanvasId, &GraphCanvas::NodeRequests::SetTooltip, name);

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
            const GraphCanvas::GraphId& graphId2 = GetGraphId();

            GraphCanvas::Endpoint endpoint;
            GraphCanvas::SlotRequestBus::EventResult(endpoint, slotId2, &GraphCanvas::SlotRequests::GetEndpoint);

            bool promotedElement = false;
            GraphCanvas::GraphModelRequestBus::EventResult(promotedElement, graphId2, &GraphCanvas::GraphModelRequests::PromoteToVariableAction, endpoint, false);

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
                            variable->SetScope(ScriptCanvas::VariableFlags::Scope::Function);
                        }
                    }
                }
            }
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
    }

    //////////////////////////////////
    // SetDataSlotTypeMenuAction
    //////////////////////////////////

    SetDataSlotTypeMenuAction::SetDataSlotTypeMenuAction(QObject* parent)
        : GraphCanvas::SlotContextMenuAction("Set Slot Type", parent)
    {
    }

    void SetDataSlotTypeMenuAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        ScriptCanvas::Slot* slot = GetSlot(graphId, targetId);

        bool isEnabled = slot && slot->IsUserAdded() && slot->GetDescriptor().IsData();

        setEnabled(isEnabled);
    }

    GraphCanvas::ContextMenuAction::SceneReaction SetDataSlotTypeMenuAction::TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        ScriptCanvas::Slot* slot = GetSlot(graphId, GetTargetId());
        if (!slot)
        {
            return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
        }

        auto variable = slot->GetVariable();
        if (!variable)
        {
            return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
        }

        ScriptCanvas::ScriptCanvasId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasId, graphId);
        if (!scriptCanvasGraphId.IsValid())
        {
            return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
        }

        VariablePaletteRequests::VariableConfigurationInput selectedSlotSetup;
        selectedSlotSetup.m_changeVariableType = true;
        selectedSlotSetup.m_graphVariable = variable;
        selectedSlotSetup.m_currentName = slot->GetName();
        selectedSlotSetup.m_currentType = slot->GetDataType();

        QPoint scenePoint(static_cast<int>(scenePos.GetX()), static_cast<int>(scenePos.GetY()));
        VariablePaletteRequests::VariableConfigurationOutput output;
        VariablePaletteRequestBus::BroadcastResult(output, &VariablePaletteRequests::ShowVariableConfigurationWidget
            , selectedSlotSetup, scenePoint);

        bool changed = false;

        if (output.m_actionIsValid)
        {
            if ((output.m_nameChanged && !output.m_name.empty()) || (output.m_typeChanged && output.m_type.IsValid()))
            {
                GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, scriptCanvasGraphId);
                GraphCanvas::ScopedGraphUndoBlocker undoBlocker(graphId);

                if ((output.m_nameChanged && !output.m_name.empty()))
                {
                    variable->SetVariableName(output.m_name);
                }

                if (output.m_typeChanged && output.m_type.IsValid())
                {
                    variable->ModDatum().SetType(output.m_type, ScriptCanvas::Datum::TypeChange::Forced);
                    ScriptCanvas::GraphRequestBus::Event(scriptCanvasGraphId, &ScriptCanvas::GraphRequests::RefreshVariableReferences
                        , variable->GetVariableId());
                }

                changed = true;
            }
        }

        return changed ? GraphCanvas::ContextMenuAction::SceneReaction::PostUndo : GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }
    
    bool SetDataSlotTypeMenuAction::IsSupportedSlotType(const AZ::EntityId& slotId)
    {
        GraphCanvas::Endpoint endpoint;
        GraphCanvas::SlotRequestBus::EventResult(endpoint, slotId, &GraphCanvas::SlotRequests::GetEndpoint);

        const ScriptCanvas::Slot* slot = SlotManipulationMenuAction::GetScriptCanvasSlot(endpoint);
        if (slot)
        {
            if (slot->GetDescriptor().IsData())
            {
                return true;
            }
        }

        return false;
    }

    ScriptCanvas::Slot* SetDataSlotTypeMenuAction::GetSlot(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasId, graphId);

        GraphCanvas::Endpoint endpoint;
        GraphCanvas::SlotRequestBus::EventResult(endpoint, targetId, &GraphCanvas::SlotRequests::GetEndpoint);

        ScriptCanvas::Endpoint scEndpoint;
        EditorGraphRequestBus::EventResult(scEndpoint, scriptCanvasGraphId, &EditorGraphRequests::ConvertToScriptCanvasEndpoint, endpoint);

        ScriptCanvas::Slot* slot = nullptr;
        ScriptCanvas::NodeRequestBus::EventResult(slot, scEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlot, scEndpoint.GetSlotId());

        return slot;

    }

    //////////////////////////////////
    // CreateAzEventHandlerSlotMenuAction
    //////////////////////////////////

    CreateAzEventHandlerSlotMenuAction::CreateAzEventHandlerSlotMenuAction(QObject* parent)
        : SlotContextMenuAction("Create event handler", parent)
    {
    }

    void CreateAzEventHandlerSlotMenuAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetId)
    {
        m_methodWithAzEventReturn = FindBehaviorMethodWithAzEventReturn(graphId, targetId);
        if (m_methodWithAzEventReturn)
        {
            // Store the GraphCanvas endpoint corresponding to the supplied slot
            GraphCanvas::SlotRequestBus::Event(targetId, [this](GraphCanvas::SlotRequests* slotRequests)
            {
                m_methodNodeAzEventEndpoint = slotRequests->GetEndpoint();
            });
            setEnabled(true);
            return;
        }

        setEnabled(false);
    }

    GraphCanvas::ContextMenuAction::SceneReaction CreateAzEventHandlerSlotMenuAction::TriggerAction(const GraphCanvas::GraphId& graphId, const AZ::Vector2& scenePos)
    {
        if (m_methodWithAzEventReturn)
        {
            ScriptCanvas::ScriptCanvasId scriptCanvasGraphId;
            GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasId, graphId);

            // Retrieve the MethodNodeScriptCanvasId and pass that in to the CreateAzEventHandlerNode function to enforce
            // the restricted node contract
            AZ::EntityId methodNodeScriptCanvasId;
            auto GetScriptCanvasNodeId = [&methodNodeScriptCanvasId](GraphCanvas::NodeRequests* nodeRequests)
            {
                if (auto scNodeId = AZStd::any_cast<AZ::EntityId>(nodeRequests->GetUserData());
                    scNodeId != nullptr)
                {
                    methodNodeScriptCanvasId = *scNodeId;
                }
            };
            GraphCanvas::NodeRequestBus::Event(m_methodNodeAzEventEndpoint.GetNodeId(), GetScriptCanvasNodeId);
            NodeIdPair nodePair = Nodes::CreateAzEventHandlerNode(*m_methodWithAzEventReturn, scriptCanvasGraphId,
                methodNodeScriptCanvasId);

            if (nodePair.m_graphCanvasId.IsValid())
            {
                // Add newly created GraphCanvas node to the GraphCanvas scene
                GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, scenePos, false);

                // Connect the AZ::Event<Params...> data output from the MethodNode to newly created AzEventHandler node data input slot of the same type
                GraphCanvas::CreateConnectionsBetweenConfig createConnectionBetweenConfig;
                createConnectionBetweenConfig.m_connectionType = GraphCanvas::CreateConnectionsBetweenConfig::CreationType::SingleConnection;
                createConnectionBetweenConfig.m_createModelConnections = true;
                GraphCanvas::GraphUtils::CreateConnectionsBetween({ m_methodNodeAzEventEndpoint }, nodePair.m_graphCanvasId, createConnectionBetweenConfig);

                if (!createConnectionBetweenConfig.m_createdConnections.empty())
                {
                    GraphCanvas::Endpoint otherEndpoint;
                    GraphCanvas::ConnectionRequestBus::EventResult(otherEndpoint, *createConnectionBetweenConfig.m_createdConnections.begin(),
                        &GraphCanvas::ConnectionRequests::FindOtherEndpoint, m_methodNodeAzEventEndpoint);

                    if (otherEndpoint.IsValid())
                    {
                        // This will connect the execution output slot from the MethodNode to the Connect input slot
                        // on our newly created AzEventHandler node
                        auto opportunisticConnections = GraphCanvas::GraphUtils::CreateOpportunisticConnectionsBetween(m_methodNodeAzEventEndpoint, otherEndpoint);
                        createConnectionBetweenConfig.m_createdConnections.insert(opportunisticConnections.begin(), opportunisticConnections.end());

                        // Update the AzEventHandler node position to not overlap over the MethodNode
                        GraphCanvas::GraphUtils::AlignSlotForConnection(otherEndpoint, m_methodNodeAzEventEndpoint);
                    }

                    // Disable the flags on the connections we created with the AZ::Event Handler node (e.g. selectable/movable)
                    // so that the user can't delete them
                    for (auto connectionId : createConnectionBetweenConfig.m_createdConnections)
                    {
                        GraphCanvas::ConnectionUIRequestBus::Event(connectionId, &GraphCanvas::ConnectionUIRequests::SetGraphicsItemFlags, QGraphicsItem::GraphicsItemFlags());
                    }
                }

                return GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
            }
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }

    const AZ::BehaviorMethod* CreateAzEventHandlerSlotMenuAction::FindBehaviorMethodWithAzEventReturn(const GraphCanvas::GraphId& graphId, AZ::EntityId targetId)
    {
        const AZ::BehaviorMethod* methodWithAzEventReturn{};

        if (GraphCanvas::GraphUtils::IsSlot(targetId))
        {
            // Extract the GraphCanvas Slot Type and complete endpoint from using the targetId
            GraphCanvas::SlotType slotType = GraphCanvas::SlotTypes::Invalid;
            GraphCanvas::Endpoint endpoint;
            auto GetGraphCanvasEndpoint = [&slotType, &endpoint](GraphCanvas::SlotRequests* slotRequests)
            {
                slotType = slotRequests->GetSlotType();
                endpoint = slotRequests->GetEndpoint();
            };
            GraphCanvas::SlotRequestBus::Event(targetId, GetGraphCanvasEndpoint);

            // A slot that exposes the context menu to create an AZ::Event must be a data slot
            if (slotType != GraphCanvas::SlotTypes::DataSlot)
            {
                return nullptr;
            }

            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

            ScriptCanvas::SlotId scriptCanvasSlotId;
            auto GetScriptCanvasSlotId = [&scriptCanvasSlotId](GraphCanvas::SlotRequests* slotRequests)
            {
                if (auto scSlotId = AZStd::any_cast<ScriptCanvas::SlotId>(slotRequests->GetUserData());
                    scSlotId != nullptr)
                {
                    scriptCanvasSlotId = *scSlotId;
                }
            };
            GraphCanvas::SlotRequestBus::Event(endpoint.GetSlotId(), GetScriptCanvasSlotId);

            AZ::EntityId scriptCanvasNodeId;
            auto GetScriptCanvasNodeId = [&scriptCanvasNodeId](GraphCanvas::NodeRequests* nodeRequests)
            {
                if (auto scNodeId = AZStd::any_cast<AZ::EntityId>(nodeRequests->GetUserData());
                    scNodeId != nullptr)
                {
                    scriptCanvasNodeId = *scNodeId;
                }
            };
            GraphCanvas::NodeRequestBus::Event(endpoint.GetNodeId(), GetScriptCanvasNodeId);

            const AZ::BehaviorMethod* candidateMethod{};
            auto GetMethodFromNode = [&candidateMethod, &scriptCanvasNodeId, scriptCanvasSlotId](ScriptCanvas::GraphRequests* graphRequests)
            {
                ScriptCanvas::Slot* scriptCanvasSlot = graphRequests->FindSlot(ScriptCanvas::Endpoint{ scriptCanvasNodeId, scriptCanvasSlotId });
                ScriptCanvas::Node* scriptCanvasNode = scriptCanvasSlot ? scriptCanvasSlot->GetNode() : nullptr;
                if (auto methodNode = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(scriptCanvasNode); methodNode != nullptr)
                {
                    candidateMethod = methodNode->GetMethod();
                }
            };
            ScriptCanvas::GraphRequestBus::Event(scriptCanvasId, GetMethodFromNode);

            if (candidateMethod)
            {
                auto MethodReturnsValidAzEvent = [&methodWithAzEventReturn, &candidateMethod](AZ::ComponentApplicationRequests* requests)
                {
                    if (AZ::BehaviorContext* behaviorContext = requests->GetBehaviorContext(); behaviorContext != nullptr)
                    {
                        if (AZ::ValidateAzEventDescription(*behaviorContext, *candidateMethod))
                        {
                            methodWithAzEventReturn = candidateMethod;
                        }
                    }
                };
                AZ::ComponentApplicationBus::Broadcast(MethodReturnsValidAzEvent);
            }
        }

        return methodWithAzEventReturn;
    }

    /////////////////////
    // SceneContextMenu
    /////////////////////

    SceneContextMenu::SceneContextMenu(const NodePaletteModel& paletteModel, AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* assetModel)
        : GraphCanvas::SceneContextMenu(ScriptCanvasEditor::AssetEditorId)
    {

        auto userSettings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC_CE("ScriptCanvasPreviewSettings"), AZ::UserSettings::CT_LOCAL);
        if (userSettings)
        {
            m_userNodePaletteWidth = userSettings->m_sceneContextMenuNodePaletteWidth;
        }

        const bool inContextMenu = true;
        Widget::ScriptCanvasNodePaletteConfig paletteConfig(paletteModel, assetModel, inContextMenu);
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

    //////////////////////////
    // RenameFunctionDefinitionNodeAction
    //////////////////////////

    RenameFunctionDefinitionNodeAction::RenameFunctionDefinitionNodeAction(NodeDescriptorComponent* descriptor, QObject* parent)
        : NodeContextMenuAction("Rename Function", parent)
        , m_descriptor(descriptor)
    {}

    void RenameFunctionDefinitionNodeAction::RefreshAction(const GraphCanvas::GraphId& graphId, const AZ::EntityId& /*targetId*/)
    {
        AZStd::vector<AZ::EntityId> selectedNodes;
        GraphCanvas::SceneRequestBus::EventResult(selectedNodes, graphId, &GraphCanvas::SceneRequests::GetSelectedNodes);

        setEnabled(selectedNodes.size() == 1);
    }

    GraphCanvas::ContextMenuAction::SceneReaction RenameFunctionDefinitionNodeAction::TriggerAction(const GraphCanvas::GraphId&, const AZ::Vector2&)
    {
        if (FunctionDefinitionNodeDescriptorComponent::RenameDialog(azrtti_cast<FunctionDefinitionNodeDescriptorComponent*>(m_descriptor)))
        {
            return GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }

    #include "Editor/View/Windows/moc_ScriptCanvasContextMenus.cpp"
}

