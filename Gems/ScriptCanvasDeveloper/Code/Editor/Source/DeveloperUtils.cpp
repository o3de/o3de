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

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>

#include <ScriptCanvas/Bus/RequestBus.h>
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

            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PopPreventUndoStateUpdate);
            ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PostUndoPoint, activeScriptCanvasId);
        }
    }
}
