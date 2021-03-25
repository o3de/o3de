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
#pragma once

#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Editor/EditorTypes.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/ScriptCanvasActions/ElementInteractions.h>

namespace ScriptCanvasDeveloper
{
    /**
        EditorAutomationState that will select the specified target
    */
    class SelectSceneElementState
        : public NamedAutomationState
    {
    public:
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
