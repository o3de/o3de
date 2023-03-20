/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Types/Types.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/WidgetActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>

namespace ScriptCanvas::Developer
{
    struct FindPositionOffsets
    {
        // 0 is top. 1 is bottom.
        float m_verticalPosition = 0;

        // Offset will be added on to the relative anchor point
        int m_verticalOffset = 0;

        // 0 is left, 1 is right
        float m_horizontalPosition = 0;

        // Offset will be added on to the relative anchor point
        int m_horizontalOffset = 0;
    };

    /**
        EditorAutomationState that will find the node position and store it in the supplied id.
        - By default will return the 'center' of the node. The offset vector will determine
          a direction on the node to offset to the edge of, and the length deterines how much
          extra space to apply in that offset direction

          0 is right/up, 1 is left/down
    */
    class FindNodePosition
        : public CustomActionState
    {        
    public:
        AZ_CLASS_ALLOCATOR(FindNodePosition, AZ::SystemAllocator)
        FindNodePosition(AutomationStateModelId nodeId, AutomationStateModelId outputId, FindPositionOffsets offsets = FindPositionOffsets());
        ~FindNodePosition() override = default;

        void OnCustomAction() override;

    private:

        FindPositionOffsets m_offsets;

        AutomationStateModelId m_nodeId;
        AutomationStateModelId m_outputId;
    };

    /**
        EditorAutomationState that will find the group position and store it in the supplied id.
        - By default will return the 'center' of the node. The offset vector will determine
          a direction on the node to offset to the edge of, and the length deterines how much
          extra space to apply in that offset direction

          0 is right/up, 1 is left/down
    */
    class FindGroupPosition
        : public CustomActionState
    {
    public:
        AZ_CLASS_ALLOCATOR(FindGroupPosition, AZ::SystemAllocator)
        FindGroupPosition(AutomationStateModelId groupId, AutomationStateModelId outputId, FindPositionOffsets offsets = FindPositionOffsets());
        ~FindGroupPosition() override = default;

        void OnCustomAction() override;

    private:

        FindPositionOffsets m_offsets;

        AutomationStateModelId m_groupId;
        AutomationStateModelId m_outputId;
    };

    /**
        EditorAutomationState that will find an endpoint of the specified type on the supplied node.
        Slot number will be the number of the slot that you are looking for(index 0).
    */
    class FindEndpointOfTypeState
        : public CustomActionState
    {
    public:
        AZ_CLASS_ALLOCATOR(FindEndpointOfTypeState, AZ::SystemAllocator)

        FindEndpointOfTypeState(AutomationStateModelId targetNodeId, AutomationStateModelId outputId, GraphCanvas::ConnectionType connectionType, GraphCanvas::SlotType slotType, int slotNumber = 0);
        ~FindEndpointOfTypeState() override = default;

        void OnCustomAction() override;

    private:

        AutomationStateModelId m_targetNodeId;
        AutomationStateModelId m_outputId;

        int m_slotNumber;

        GraphCanvas::ConnectionType m_connectionType = GraphCanvas::ConnectionType::CT_Invalid;
        GraphCanvas::SlotType       m_slotType       = GraphCanvas::SlotTypes::Invalid;
    };

    /**
        EditorAutomationState that will find the last connection for the specified endpoint
    */
    class GetLastConnection
        : public CustomActionState
    {
    public:
        AZ_CLASS_ALLOCATOR(GetLastConnection, AZ::SystemAllocator)
        GetLastConnection(AutomationStateModelId targetEndpoint, AutomationStateModelId outputId);
        ~GetLastConnection() override = default;

        void OnCustomAction() override;

    private:

        AutomationStateModelId m_targetEndpoint;
        AutomationStateModelId m_outputId;
    };

    /**
        EditorAutomationState that will delete the specified row from the GraphVariablesPalette
    */
    class DeleteVariableRowFromPaletteState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(DeleteVariableRowFromPaletteState, AZ::SystemAllocator)

        DeleteVariableRowFromPaletteState(int row);
        ~DeleteVariableRowFromPaletteState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:
        int m_rowCount = 0;

        MoveMouseToViewRowAction* m_mouseToRow = nullptr;

        MouseClickAction            m_clickAction;
        TypeCharAction              m_deleteAction;
        ProcessUserEventsAction     m_processEvents;
    };

    /**
        EditorAutomationState will confirm the supplied group status of the provided scene member to the provided group.
    */
    class CheckIsInGroup
        : public CustomActionState
    {
    public:
        AZ_CLASS_ALLOCATOR(CheckIsInGroup, AZ::SystemAllocator)
        CheckIsInGroup(AutomationStateModelId sceneMemberId, AutomationStateModelId groupId, bool expectResult, AZStd::string stateName = "");
        ~CheckIsInGroup() override = default;

        void OnCustomAction() override;

    private:

        bool m_expectResult = false;

        AutomationStateModelId m_sceneMemberId;
        AutomationStateModelId m_groupId;
    };

    /**
        EditorAutomationState that triggers the specified character with the 'ctrl' key.
    */
    class TriggerHotKey
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(TriggerHotKey, AZ::SystemAllocator)
        TriggerHotKey(QChar hotkey, AZStd::string stateId = "");
        ~TriggerHotKey() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;

    private:

        TypeCharAction m_typeAction;

        ProcessUserEventsAction m_processEvents;

        KeyPressAction m_pressCtrlAction;
        KeyReleaseAction m_releaseCtrlAction;
    };
}
