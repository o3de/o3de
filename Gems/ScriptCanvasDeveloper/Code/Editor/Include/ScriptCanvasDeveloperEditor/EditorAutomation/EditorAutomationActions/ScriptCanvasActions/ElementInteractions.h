/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Types/Types.h>


#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvasDeveloper
{
    /**
        EditorAutomationAction that will select the specified entity inside of the active Graph scene
    */
    class SelectSceneElementAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(SelectSceneElementAction, AZ::SystemAllocator, 0);
        AZ_RTTI(SelectSceneElementAction, "{D1EA3B23-D7FD-411A-87BC-4D9D88D35A03}", CompoundAction);

        SelectSceneElementAction(AZ::EntityId sceneMemberId);
        ~SelectSceneElementAction() override = default;

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        void SetupAction() override;

    private:

        QPointF m_scenePoint;

        AZ::EntityId m_sceneMemberId;
        GraphCanvas::GraphId m_graphId;
        GraphCanvas::ViewId m_viewId;
    };

    /**
        EditorAutomationAction that will alt click the specified entity inside of the active Graph scene in order
        Should delete the element that is clicked on
    */
    class AltClickSceneElementAction
        : public CompoundAction
        , public GraphCanvas::SceneNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AltClickSceneElementAction, AZ::SystemAllocator, 0);
        AZ_RTTI(AltClickSceneElementAction, "{FF99EC14-53B3-474E-A7A1-6D30800B9583}", CompoundAction);

        AltClickSceneElementAction(AZ::EntityId sceneMemberId);
        ~AltClickSceneElementAction() override = default;

        bool IsMissingPrecondition() override;
        EditorAutomationAction* GenerateMissingPreconditionAction() override;

        void SetupAction() override;

        ActionReport GenerateReport() const override;

        // SceneNotificaitonBus
        void OnNodeRemoved(const AZ::EntityId& nodeId) override;

        void OnConnectionRemoved(const AZ::EntityId& connectionId) override;
        ////

    protected:

        void OnActionsComplete() override;

    private:

        QPointF m_scenePoint;

        bool         m_sceneMemberRemoved = false;
        AZ::EntityId m_sceneMemberId;
        GraphCanvas::GraphId m_graphId;
        GraphCanvas::ViewId m_viewId;
    };

    /**
        EditorAutomationAction that will move the mouse to the NodePropertyDisplay for the specified GraphCanvas SlotId
    */
    class MouseToNodePropertyEditorAction
        : public CompoundAction
    {
    public:
        AZ_CLASS_ALLOCATOR(MouseToNodePropertyEditorAction, AZ::SystemAllocator, 0);
        AZ_RTTI(MouseToNodePropertyEditorAction, "{03BE0BBE-B977-4103-BC6D-0357B7CEA46E}", CompoundAction);

        MouseToNodePropertyEditorAction(GraphCanvas::SlotId slotId);
        ~MouseToNodePropertyEditorAction() override = default;

        void SetupAction() override;

    private:

        GraphCanvas::SlotId m_slotId;
    };
}
