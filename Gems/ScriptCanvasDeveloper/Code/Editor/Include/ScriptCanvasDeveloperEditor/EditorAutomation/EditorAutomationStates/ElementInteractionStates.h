/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Editor/EditorTypes.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationState that will select the specified target
    */
    class SelectSceneElementState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(SelectSceneElementState, AZ::SystemAllocator)
        SelectSceneElementState(AutomationStateModelId targetId);
        ~SelectSceneElementState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        SelectSceneElementAction* m_selectSceneElement = nullptr;

        AutomationStateModelId m_targetId;
    };

    /**
        EditorAutomationState that will execute the alt click scene element action
    */
    class AltClickSceneElementState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(AltClickSceneElementState, AZ::SystemAllocator)
        AltClickSceneElementState(AutomationStateModelId targetId);
        ~AltClickSceneElementState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;

    private:

        AltClickSceneElementAction* m_altClickAction = nullptr;

        AutomationStateModelId m_targetId;
    };

    /**
        EditorAutomationState that will execute the mouse to node property editor action
    */
    class MouseToNodePropertyEditorState
        : public NamedAutomationState
    {
    public:
        AZ_CLASS_ALLOCATOR(MouseToNodePropertyEditorState, AZ::SystemAllocator)
        MouseToNodePropertyEditorState(AutomationStateModelId slotId);
        ~MouseToNodePropertyEditorState() override = default;

        void OnSetupStateActions(EditorAutomationActionRunner& actionRunner) override;
        void OnStateActionsComplete() override;
        
    private:

        ProcessUserEventsAction          m_processEvents;
        MouseToNodePropertyEditorAction* m_moveToPropertyAction = nullptr;
    
        AutomationStateModelId m_slotId;
    };
}
