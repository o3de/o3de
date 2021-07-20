/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <QTableView>

#include <GraphCanvas/Components/Nodes/Group/NodeGroupBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>

#include <GraphCanvas/Utils/ConversionUtils.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/UtilityStates.h>

#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasDeveloper
{
    /////////////////////
    // FindNodePosition
    /////////////////////

    FindNodePosition::FindNodePosition(AutomationStateModelId nodeId, AutomationStateModelId outputId, FindPositionOffsets offsets)
        : CustomActionState("FindNodePosition")
        , m_offsets(offsets)
        , m_nodeId(nodeId)
        , m_outputId(outputId)
    {
        SetStateName(AZStd::string::format("FindNodePosition::%s::%s", m_nodeId.c_str(), m_outputId.c_str()));
    }

    void FindNodePosition::OnCustomAction()
    {
        const GraphCanvas::NodeId* nodeId = GetStateModel()->GetStateDataAs<GraphCanvas::NodeId>(m_nodeId);

        if (nodeId)
        {
            QGraphicsItem* graphicsItem = nullptr;
            GraphCanvas::VisualRequestBus::EventResult(graphicsItem, (*nodeId), &GraphCanvas::VisualRequests::AsGraphicsItem);

            if (graphicsItem)
            {
                QRectF boundingRect = graphicsItem->sceneBoundingRect();

                qreal horizontalPoint = boundingRect.left() + boundingRect.width() * m_offsets.m_horizontalPosition;
                horizontalPoint += m_offsets.m_horizontalOffset;

                qreal verticalPoint = boundingRect.top() + boundingRect.height() * m_offsets.m_verticalPosition;
                verticalPoint += m_offsets.m_verticalOffset;

                AZ::Vector2 scenePoint(horizontalPoint, verticalPoint);
                GetStateModel()->SetStateData(m_outputId, scenePoint);
            }
        }
        else
        {
            ReportError(AZStd::string::format("%s is not a valid EntityId", m_nodeId.c_str()));
        }
    }

    //////////////////////
    // FindGroupPosition
    //////////////////////

    FindGroupPosition::FindGroupPosition(AutomationStateModelId groupId, AutomationStateModelId outputId, FindPositionOffsets offsets)
        : CustomActionState("FindGroupPosition")
        , m_offsets(offsets)
        , m_groupId(groupId)
        , m_outputId(outputId)
    {
        SetStateName(AZStd::string::format("FindGroupPosition::%s::%s", m_groupId.c_str(), m_outputId.c_str()));
    }

    void FindGroupPosition::OnCustomAction()
    {
        const AZ::EntityId* groupId = GetStateModel()->GetStateDataAs<AZ::EntityId>(m_groupId);

        if (groupId)
        {
            QRectF groupBoundingBox;
            GraphCanvas::NodeGroupRequestBus::EventResult(groupBoundingBox, (*groupId), &GraphCanvas::NodeGroupRequests::GetGroupBoundingBox);

            qreal horizontalPoint = groupBoundingBox.left() + groupBoundingBox.width() * m_offsets.m_horizontalPosition;
            horizontalPoint += m_offsets.m_horizontalOffset;

            qreal verticalPoint = groupBoundingBox.top() + groupBoundingBox.height() * m_offsets.m_verticalPosition;
            verticalPoint += m_offsets.m_verticalPosition;

            AZ::Vector2 scenePoint(horizontalPoint, verticalPoint);
            GetStateModel()->SetStateData(m_outputId, scenePoint);
        }
        else
        {
            ReportError(AZStd::string::format("%s is not a valid EntityId", m_groupId.c_str()));
        }
    }

    ////////////////////////////
    // FindEndpointOfTypeState
    ////////////////////////////

    FindEndpointOfTypeState::FindEndpointOfTypeState(AutomationStateModelId targetNodeId, AutomationStateModelId outputId, GraphCanvas::ConnectionType connectionType, GraphCanvas::SlotType slotType, int slotNumber)
        : CustomActionState("FindEndpointOfTypeState")
        , m_targetNodeId(targetNodeId)
        , m_outputId(outputId)
        , m_slotNumber(slotNumber)
        , m_connectionType(connectionType)
        , m_slotType(slotType)
    {
        SetStateName(AZStd::string::format("FindEndpointOfType::%s::%s", targetNodeId.c_str(), outputId.c_str()));
    }

    void FindEndpointOfTypeState::OnCustomAction()
    {
        const GraphCanvas::NodeId* nodeId = GetStateModel()->GetStateDataAs<GraphCanvas::NodeId>(m_targetNodeId);

        if (nodeId)
        {
            AZStd::vector< GraphCanvas::SlotId > slotIds;
            GraphCanvas::NodeRequestBus::EventResult(slotIds, (*nodeId), &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, m_connectionType, m_slotType);

            if (slotIds.size() <= m_slotNumber)
            {
                ReportError(AZStd::string::format("Slot Number %i is out of scope for the current node.", m_slotNumber));
                return;
            }

            GraphCanvas::SlotId slotId = slotIds[m_slotNumber];

            GraphCanvas::Endpoint endpoint = GraphCanvas::Endpoint((*nodeId), slotId);
            GetStateModel()->SetStateData(m_outputId, endpoint);
        }
        else
        {
            ReportError(AZStd::string::format("%s is not a valid EntityId", m_targetNodeId.c_str()));
        }
    }

    //////////////////////
    // GetLastConnection
    //////////////////////

    GetLastConnection::GetLastConnection(AutomationStateModelId targetEndpoint, AutomationStateModelId outputId)
        : CustomActionState("GetLastConnection")
        , m_targetEndpoint(targetEndpoint)
        , m_outputId(outputId)
    {
        SetStateName(AZStd::string::format("GetLastConnection::%s", targetEndpoint.c_str()));
    }

    void GetLastConnection::OnCustomAction()
    {
        const GraphCanvas::Endpoint* targetEndpoint = GetStateModel()->GetStateDataAs<GraphCanvas::Endpoint>(m_targetEndpoint);

        if (targetEndpoint)
        {
            GraphCanvas::ConnectionId connectionId;
            GraphCanvas::SlotRequestBus::EventResult(connectionId, targetEndpoint->GetSlotId(), &GraphCanvas::SlotRequests::GetLastConnection);

            GetStateModel()->SetStateData(m_outputId, connectionId);
        }
        else
        {
            ReportError(AZStd::string::format("%s is not a valid GraphCanvas::Endpoint", m_targetEndpoint.c_str()));
        }
    }

    //////////////////////////////////////
    // DeleteVariableRowFromPaletteState
    //////////////////////////////////////

#if !defined(AZ_COMPILER_MSVC)
#ifndef VK_DELETE
#define VK_DELETE 0x2E
#endif

#ifndef VK_CONTROL 
#define VK_CONTROL 0x11
#endif 
#endif

    DeleteVariableRowFromPaletteState::DeleteVariableRowFromPaletteState(int row)
        : NamedAutomationState("DeleteVariableRowFromPaletteState")
        , m_row(row)
        , m_clickAction(Qt::MouseButton::LeftButton)
        , m_deleteAction(VK_DELETE)
    {
        SetStateName(AZStd::string::format("DeleteVariableRowState::%i", row));
    }

    void DeleteVariableRowFromPaletteState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        QTableView* graphPalette;
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(graphPalette, &ScriptCanvasEditor::VariableAutomationRequests::GetGraphPaletteTableView);

        m_rowCount = graphPalette->model()->rowCount();

        m_mouseToRow = aznew MoveMouseToViewRowAction(graphPalette, 0);

        actionRunner.AddAction(m_mouseToRow);
        actionRunner.AddAction(&m_processEvents);
        actionRunner.AddAction(&m_clickAction);
        actionRunner.AddAction(&m_processEvents);
        actionRunner.AddAction(&m_deleteAction);
    }

    void DeleteVariableRowFromPaletteState::OnStateActionsComplete()
    {
        delete m_mouseToRow;
        m_mouseToRow = nullptr;

        QTableView* graphPalette;
        ScriptCanvasEditor::VariableAutomationRequestBus::BroadcastResult(graphPalette, &ScriptCanvasEditor::VariableAutomationRequests::GetGraphPaletteTableView);

        if (graphPalette->model()->rowCount() >= m_rowCount)
        {
            ReportError("Failed to delete variable row from table.");
        }
    }

    ///////////////////
    // CheckIsInGroup
    ///////////////////

    CheckIsInGroup::CheckIsInGroup(AutomationStateModelId sceneMemberId, AutomationStateModelId groupId, bool expectResult, AZStd::string stateName)
        : CustomActionState("CheckIsInGroup")
        , m_sceneMemberId(sceneMemberId)
        , m_groupId(groupId)
        , m_expectResult(expectResult)
    {
        if (stateName.empty())
        {
            SetStateName(AZStd::string::format("CheckGroupStatus::%s::%s", sceneMemberId.c_str(), groupId.c_str()));
        }
        else
        {
            SetStateName(stateName);
        }
    }

    void CheckIsInGroup::OnCustomAction()
    {
        const AZ::EntityId* sceneMemberTarget = GetStateModel()->GetStateDataAs<AZ::EntityId>(m_sceneMemberId);
        const AZ::EntityId* targetGroupId = GetStateModel()->GetStateDataAs<AZ::EntityId>(m_groupId);

        if (sceneMemberTarget && targetGroupId)
        {
            AZ::EntityId groupId;
            GraphCanvas::GroupableSceneMemberRequestBus::EventResult(groupId, (*sceneMemberTarget), &GraphCanvas::GroupableSceneMemberRequests::GetGroupId);

            bool groupMatch = groupId == (*targetGroupId);

            if (groupMatch != m_expectResult)
            {
                ReportError(AZStd::string::format("Group Status of %s not in expected state.", m_sceneMemberId.c_str()));
            }
        }
        else
        {
            AZStd::string errorString;

            if (sceneMemberTarget == nullptr)
            {
                errorString = AZStd::string::format("%s is not a valid EntityId", m_sceneMemberId.c_str());
            }

            if (targetGroupId == nullptr)
            {
                if (!errorString.empty())
                {
                    errorString += ", ";
                }

                errorString += AZStd::string::format("%s is not a valid EntityId", m_groupId.c_str());
            }

            ReportError(errorString);
        }
    }

    //////////////////
    // TriggerHotKey
    //////////////////

    TriggerHotKey::TriggerHotKey(QChar hotKey, AZStd::string stateId)
        : NamedAutomationState("TriggerHotKey")
        , m_typeAction(hotKey)
        , m_pressCtrlAction(VK_CONTROL)
        , m_releaseCtrlAction(VK_CONTROL)
    {
        if (stateId.empty())
        {
            QString name = hotKey;
            SetStateName(AZStd::string::format("TriggerHotKey::%s", name.toUtf8().data()));
        }
        else
        {
            SetStateName(stateId);
        }
    }

    void TriggerHotKey::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        actionRunner.AddAction(&m_pressCtrlAction);
        actionRunner.AddAction(&m_processEvents);
        actionRunner.AddAction(&m_typeAction);
        actionRunner.AddAction(&m_processEvents);
        actionRunner.AddAction(&m_releaseCtrlAction);
        actionRunner.AddAction(&m_processEvents);
    }
}
