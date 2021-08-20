/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qpushbutton.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>

#include <EditorAutomationTests/GroupTests.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/CreateElementsStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/EditorViewStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/GraphStates.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/UtilityStates.h>

namespace ScriptCanvasDeveloper
{
    ////////////////////
    // CreateGroupTest
    ////////////////////

    CreateGroupTest::CreateGroupTest(CreateGroupAction::CreationType creationType)
        : EditorAutomationTest(creationType == CreateGroupAction::CreationType::Hotkey ? "Create Group Test" : "Create Group Test With Toolbar")
    {
        AddState(new CreateRuntimeGraphState());
        AddState(new CreateGroupState(ScriptCanvasEditor::AssetEditorId, creationType));
        AddState(new ForceCloseActiveGraphState());
    }

    //////////////////////////
    // GroupManipulationTest
    //////////////////////////

    GroupManipulationTest::OffsetPositionByNodeDimension::OffsetPositionByNodeDimension(float horizontalDimension, float verticalDimension, AutomationStateModelId nodeId, AutomationStateModelId positionId)
        : CustomActionState("OffsetPositionByNodeDimension")
        , m_horizontalDimension(horizontalDimension)
        , m_verticalDimension(verticalDimension)
        , m_nodeId(nodeId)
        , m_positionId(positionId)
    {
        SetStateName(AZStd::string::format("OffsetPosition::%s::ByNodeDimension::%s", m_positionId.c_str(), m_nodeId.c_str()));
    }

    void GroupManipulationTest::OffsetPositionByNodeDimension::OnCustomAction()
    {
        const AZ::EntityId* nodeId = GetStateModel()->GetStateDataAs<AZ::EntityId>(m_nodeId);
        const AZ::Vector2* position = GetStateModel()->GetStateDataAs<AZ::Vector2>(m_positionId);

        if (nodeId && position)
        {
            QGraphicsItem* nodeItem = nullptr;
            GraphCanvas::VisualRequestBus::EventResult(nodeItem, (*nodeId), &GraphCanvas::VisualRequests::AsGraphicsItem);

            if (nodeItem)
            {
                AZ::Vector2 modifiedValue = (*position);

                QRectF sceneBoundingBox = nodeItem->sceneBoundingRect();
                modifiedValue.SetX(position->GetX() + static_cast<float>(sceneBoundingBox.width()) * m_horizontalDimension);
                modifiedValue.SetY(position->GetY() + static_cast<float>(sceneBoundingBox.height()) * m_verticalDimension);

                GetStateModel()->SetStateData(m_positionId, modifiedValue);
            }
        }
        else
        {
            if (nodeId == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid EntityId", m_nodeId.c_str()));
            }

            if (position == nullptr)
            {
                ReportError(AZStd::string::format("%s is not a valid Vector2", m_positionId.c_str()));
            }
        }
    }

    GroupManipulationTest::GroupManipulationTest(GraphCanvas::NodePaletteWidget* nodePaletteWidget)
        : EditorAutomationTest("Group Manipulation Test")
    {
        AddState(new CreateRuntimeGraphState());

        AutomationStateModelId groupId = "GroupId";

        AddState(new CreateGroupState(ScriptCanvasEditor::AssetEditorId, CreateGroupAction::CreationType::Hotkey, groupId));

        AutomationStateModelId groupCenter = "GroupCenter";

        {
            FindPositionOffsets groupOffsets;
            groupOffsets.m_horizontalPosition = 0.5f;
            groupOffsets.m_verticalPosition = 0.5f;

            AddState(new FindGroupPosition(groupId, groupCenter, groupOffsets));
        }

        AutomationStateModelId buildStringNode = "BuildStringNode";
        AddState(new CreateNodeFromContextMenuState("Build String", CreateNodeFromContextMenuState::CreationType::ScenePosition, groupCenter, buildStringNode));

        AddState(new CheckIsInGroup(buildStringNode, groupId, true, "CheckGroupStatus::ContextMenuCreation"));

        AutomationStateModelId moveOutSceneDragStart = "MoveOutSceneDragStart";
        AutomationStateModelId moveOutSceneDragEnd = "MoveOutSceneDragEnd";

        {
            FindPositionOffsets nodeOffsets;
            nodeOffsets.m_horizontalPosition = 0.5f;
            nodeOffsets.m_verticalPosition = 0.0f;
            nodeOffsets.m_verticalOffset = 10;

            AddState(new FindNodePosition(buildStringNode, moveOutSceneDragStart, nodeOffsets));
            AddState(new FindNodePosition(buildStringNode, moveOutSceneDragEnd, nodeOffsets));
        }

        AddState(new OffsetPositionByNodeDimension(1.0f, 0.0f, groupId, moveOutSceneDragEnd));
        AddState(new OffsetPositionByNodeDimension(1.0f, 0.0f, buildStringNode, moveOutSceneDragEnd));

        AddState(new SceneMouseDragState(moveOutSceneDragStart, moveOutSceneDragEnd));
        AddState(new CheckIsInGroup(buildStringNode, groupId, false, "CheckGroupStatus::DragOutOfGroup"));

        AutomationStateModelId envelopDragStart = "EnvelopDragStart";
        AutomationStateModelId envelopDragEnd = "EnvelopDragEnd";

        {
            FindPositionOffsets groupOffsets;
            groupOffsets.m_horizontalPosition = 1.0f;
            groupOffsets.m_horizontalOffset = -5;
            groupOffsets.m_verticalPosition = 0.5f;

            AddState(new FindGroupPosition(groupId, envelopDragStart, groupOffsets));
            AddState(new FindGroupPosition(groupId, envelopDragEnd, groupOffsets));
        }

        AddState(new OffsetPositionByNodeDimension(1.0f, 0.0f, groupId, envelopDragEnd));
        AddState(new OffsetPositionByNodeDimension(1.0f, 0.0f, buildStringNode, envelopDragEnd));

        AddState(new SceneMouseDragState(envelopDragStart, envelopDragEnd));
        AddState(new CheckIsInGroup(buildStringNode, groupId, true, "CheckGroupStatus::ResizeToInclude"));

        AddState(new SceneMouseDragState(envelopDragEnd, envelopDragStart));
        AddState(new CheckIsInGroup(buildStringNode, groupId, false, "CheckGroupStatus::ResizeToExclude"));

        AddState(new SceneMouseDragState(moveOutSceneDragEnd, moveOutSceneDragStart));
        AddState(new CheckIsInGroup(buildStringNode, groupId, true, "CheckGroupStatus::DragIntoGroup"));

        AutomationStateModelId proposalEndpoint = "ProposalEndpoint";
        AddState(new FindEndpointOfTypeState(buildStringNode, proposalEndpoint, GraphCanvas::ConnectionType::CT_Output, GraphCanvas::SlotTypes::ExecutionSlot));

        AutomationStateModelId printNode = "PrintNode";
        AddState(new CreateNodeFromProposalState("Print", proposalEndpoint, envelopDragStart, printNode));

        AddState(new CheckIsInGroup(printNode, groupId, true, "CheckGroupStatus::PoposalCreation"));

        AutomationStateModelId onGraphStartNode = "OnGraphStartNode";
        AddState(new CreateNodeFromPaletteState(nodePaletteWidget, "On Graph Start", CreateNodeFromPaletteState::CreationType::ScenePosition, groupCenter, onGraphStartNode));

        AddState(new CheckIsInGroup(onGraphStartNode, groupId, true, "CheckGroupStatus::PaletteDrop"));

        AddState(new ForceCloseActiveGraphState());
    }
}
