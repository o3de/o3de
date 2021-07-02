/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <ScriptCanvas/Variable/VariableBus.h>

#include <ScriptCanvasDeveloperEditor/DeveloperUtils.h>

namespace ScriptCanvasDeveloperEditor
{
    ///////////////////
    // DeveloperUtils
    ///////////////////
    
    ScriptCanvasEditor::NodeIdPair DeveloperUtils::HandleMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, GraphCanvas::GraphId graphCanvasGraphId, const QRectF& viewportRectangle, int& widthOffset, int& heightOffset, int& maxRowHeight, AZ::Vector2 spacing)
    {
        AZ::Vector2 position(aznumeric_cast<float>(viewportRectangle.x()) + widthOffset, aznumeric_cast<float>(viewportRectangle.y()) + heightOffset);

        ScriptCanvasEditor::NodeIdPair createdPair;
        ScriptCanvasEditor::AutomationRequestBus::BroadcastResult(createdPair, &ScriptCanvasEditor::AutomationRequests::ProcessCreateNodeMimeEvent, mimeEvent, graphCanvasGraphId, position);

        UpdateViewportPositionOffsetForNode(createdPair.m_graphCanvasId, viewportRectangle, widthOffset, heightOffset, maxRowHeight, spacing);        

        GraphCanvas::SceneNotificationBus::Event(graphCanvasGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);

        return createdPair;
    }

    void DeveloperUtils::UpdateViewportPositionOffsetForNode(GraphCanvas::NodeId nodeId, const QRectF& viewportRectangle, int& widthOffset, int& heightOffset, int& maxRowHeight, AZ::Vector2 spacing)
    {
        QGraphicsItem* graphicsWidget = nullptr;
        GraphCanvas::SceneMemberUIRequestBus::EventResult(graphicsWidget, nodeId, &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);

        GraphCanvas::NodeUIRequestBus::Event(nodeId, &GraphCanvas::NodeUIRequests::AdjustSize);

        if (graphicsWidget)
        {
            int height = aznumeric_cast<int>(graphicsWidget->boundingRect().height());

            if (height > maxRowHeight)
            {
                maxRowHeight = height;
            }

            widthOffset += aznumeric_cast<int>(graphicsWidget->boundingRect().width() + spacing.GetX());

            if (widthOffset >= viewportRectangle.width())
            {
                widthOffset = 0;
                heightOffset += maxRowHeight + aznumeric_cast<int>(spacing.GetY());
                maxRowHeight = 0;
            }
        }
    }

    bool DeveloperUtils::CreateConnectedChain(const ScriptCanvasEditor::NodeIdPair& nodeIdPair, DeveloperUtils::CreateConnectedChainConfig& connectionConfig)
    {
        if (connectionConfig.m_connectionStyle == ConnectionStyle::NoConnections)
        {
            return true;
        }

        if (!nodeIdPair.m_graphCanvasId.IsValid())
        {
            return false;
        }

        // Crappy work around to ensure we know our 'seeded' element for a random connection
        if (!connectionConfig.m_fallbackNode.m_graphCanvasId.IsValid())
        {
            connectionConfig.m_fallbackNode = nodeIdPair;
        }

        if (connectionConfig.m_skipHandlers)
        {
            AZ::EntityId outermostNode = GraphCanvas::GraphUtils::FindOutermostNode(nodeIdPair.m_graphCanvasId);

            bool isHandler = false;
            ScriptCanvasEditor::NodeDescriptorRequestBus::EventResult(isHandler, outermostNode, &ScriptCanvasEditor::NodeDescriptorRequests::IsType, ScriptCanvasEditor::NodeDescriptorType::EBusHandler);

            if (isHandler)
            {
                return false;
            }

            // Hacky way to deal with finding the InputHandler node.
            // And the On Graph Start node that we will stumble into in the middle of the node palette as well
            AZStd::vector< GraphCanvas::SlotId > executionIns;
            GraphCanvas::NodeRequestBus::EventResult(executionIns, outermostNode, &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, GraphCanvas::CT_Input, GraphCanvas::SlotTypes::ExecutionSlot);

            if (executionIns.empty() && connectionConfig.m_fallbackNode.m_graphCanvasId != outermostNode)
            {
                return false;
            }
        }

        AZStd::vector< GraphCanvas::SlotId > executionIns;
        GraphCanvas::NodeRequestBus::EventResult(executionIns, nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, GraphCanvas::CT_Input, GraphCanvas::SlotTypes::ExecutionSlot);

        if (!executionIns.empty())
        {
            GraphCanvas::Endpoint executionIn = GraphCanvas::Endpoint(nodeIdPair.m_graphCanvasId, executionIns.front());

            if (connectionConfig.m_previousEndpoint.IsValid())
            {
                if (connectionConfig.m_connectionStyle == ConnectionStyle::SingleExecutionConnection)
                {
                    GraphCanvas::SlotRequestBus::Event(executionIn.GetSlotId(), &GraphCanvas::SlotRequests::CreateConnectionWithEndpoint, connectionConfig.m_previousEndpoint);
                }
            }
        }

        AZStd::vector< GraphCanvas::SlotId > executionOuts;
        GraphCanvas::NodeRequestBus::EventResult(executionOuts, nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, GraphCanvas::CT_Output, GraphCanvas::SlotTypes::ExecutionSlot);

        if (!executionOuts.empty())
        {
            connectionConfig.m_previousEndpoint = GraphCanvas::Endpoint(nodeIdPair.m_graphCanvasId, executionOuts.front());
        }

        return true;
    }

    void DeveloperUtils::ProcessNodePalette(ProcessNodePaletteInterface& processNodePaletteInterface)
    {   
        AZ::EntityId activeGraphCanvasGraphId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeGraphCanvasGraphId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

        ScriptCanvas::ScriptCanvasId activeScriptCanvasId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeScriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveScriptCanvasId);        
        
        if (activeGraphCanvasGraphId.IsValid())
        {
            processNodePaletteInterface.SetupInterface(activeGraphCanvasGraphId, activeScriptCanvasId);

            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PushPreventUndoStateUpdate);

            const GraphCanvas::GraphCanvasTreeItem* treeItem = nullptr;
            ScriptCanvasEditor::AutomationRequestBus::BroadcastResult(treeItem, &ScriptCanvasEditor::AutomationRequests::GetNodePaletteRoot);

            AZStd::list< const GraphCanvas::GraphCanvasTreeItem* > expandedItems;
            expandedItems.emplace_back(treeItem);

            while (!expandedItems.empty())
            {
                const GraphCanvas::GraphCanvasTreeItem* treeItem2 = expandedItems.front();
                expandedItems.pop_front();

                for (int i = 0; i < treeItem2->GetChildCount(); ++i)
                {
                    expandedItems.emplace_back(treeItem2->FindChildByRow(i));
                }

                if (const GraphCanvas::NodePaletteTreeItem* nodePaletteTreeItem = azrtti_cast<const GraphCanvas::NodePaletteTreeItem*>(treeItem2))
                {
                    if (processNodePaletteInterface.ShouldProcessItem(nodePaletteTreeItem))
                    {
                        AZ_TracePrintf("ScriptCanvas", "%s", nodePaletteTreeItem->GetName().toUtf8().data());
                        processNodePaletteInterface.ProcessItem(nodePaletteTreeItem);
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    continue;
                }
            }

            processNodePaletteInterface.OnProcessingComplete();

            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PopPreventUndoStateUpdate);
            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PostUndoPoint, activeScriptCanvasId);
        }
    }

    void DeveloperUtils::ProcessVariablePalette(ProcessVariablePaletteInterface& variablePaletteInterface)
    {
        AZ::EntityId activeGraphCanvasGraphId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeGraphCanvasGraphId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

        ScriptCanvas::ScriptCanvasId activeScriptCanvasId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(activeScriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveScriptCanvasId);

        if (activeScriptCanvasId.IsValid())
        {
            variablePaletteInterface.SetupInterface(activeGraphCanvasGraphId, activeScriptCanvasId);

            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PushPreventUndoStateUpdate);

            AZStd::vector<ScriptCanvas::Data::Type> variableTypes;
            ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(variableTypes, &ScriptCanvasEditor::VariableAutomationRequests::GetVariableTypes);

            for (auto variableType : variableTypes)
            {
                if (variablePaletteInterface.ShouldProcessVariableType(variableType))
                {
                    AZ_TracePrintf("ScriptCanvas", "Processing Variable Type - %s", ScriptCanvas::Data::GetName(variableType).c_str());
                    variablePaletteInterface.ProcessVariableType(variableType);
                }
            }

            variablePaletteInterface.OnProcessingComplete();

            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PopPreventUndoStateUpdate);
            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PostUndoPoint, activeScriptCanvasId);
        }
    }
}
