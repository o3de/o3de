/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Types/Types.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/EditorViewActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorMouseActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/EditorKeyActions.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/WidgetActions.h>


namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationState that will move the mouse to a particular point in the scene.
    */
    class SceneMouseMoveState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneMouseMoveState, AZ::SystemAllocator)
        SceneMouseMoveState(AutomationStateModelId targetPoint);
        ~SceneMouseMoveState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        AutomationStateModelId m_targetPoint;

        SceneMouseMoveAction* m_moveAction = nullptr;
    };

    /**
        EditorAutomationState that will execute a Mouse Drag Action inside of the scene
    */
    class SceneMouseDragState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneMouseDragState, AZ::SystemAllocator)
        SceneMouseDragState(AutomationStateModelId startPoint, AutomationStateModelId endPoint, Qt::MouseButton mouseButton = Qt::MouseButton::LeftButton);
        ~SceneMouseDragState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        AutomationStateModelId m_startPoint;
        AutomationStateModelId m_endPoint;

        Qt::MouseButton m_mouseButton;

        SceneMouseDragAction* m_dragAction = nullptr;
    };

    /**
        EditorAutomationState that will find the view center and store it in the supplied id.
    */
    class FindViewCenterState
        : public CustomActionState
    {
    public:
        AZ_CLASS_ALLOCATOR(FindViewCenterState, AZ::SystemAllocator)
        FindViewCenterState(AutomationStateModelId outputId);
        ~FindViewCenterState() override = default;

        void OnCustomAction() override;

    private:

        AutomationStateModelId m_outputId;
    };
}
