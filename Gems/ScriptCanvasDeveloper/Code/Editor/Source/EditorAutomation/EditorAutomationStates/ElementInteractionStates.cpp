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

#include <platform.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationStates/ElementInteractionStates.h>

namespace ScriptCanvasDeveloper
{
    ////////////////////////////
    // SelectSceneElementState
    ////////////////////////////

    SelectSceneElementState::SelectSceneElementState(AutomationStateModelId targetId)
        : NamedAutomationState("SelectSceneElementState")
        , m_targetId(targetId)
    {
        SetStateName(AZStd::string::format("SelectSceneElementState::%s", targetId.c_str()));
    }

    void SelectSceneElementState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const AZ::EntityId* targetId = GetStateModel()->GetStateDataAs<AZ::EntityId>(m_targetId);

        if (targetId)
        {
            m_selectSceneElement = aznew SelectSceneElementAction((*targetId));
            actionRunner.AddAction(m_selectSceneElement);
        }
        else
        {
            ReportError(AZStd::string::format("%s is not a valid EntityId", m_targetId.c_str()));
        }
    }

    void SelectSceneElementState::OnStateActionsComplete()
    {
        if (m_selectSceneElement)
        {
            delete m_selectSceneElement;
            m_selectSceneElement = nullptr;
        }
    }

    //////////////////////////////
    // AltClickSceneElementState
    //////////////////////////////

    AltClickSceneElementState::AltClickSceneElementState(AutomationStateModelId targetId)
        : NamedAutomationState("AltClickSceneElementState")
        , m_targetId(targetId)
    {
        SetStateName(AZStd::string::format("AltClickSceneElementState::%s", targetId.c_str()));
    }

    void AltClickSceneElementState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const AZ::EntityId* targetId = GetStateModel()->GetStateDataAs<AZ::EntityId>(m_targetId);

        if (targetId)
        {
            m_altClickAction = aznew AltClickSceneElementAction((*targetId));
            actionRunner.AddAction(m_altClickAction);
        }
        else
        {
            ReportError(AZStd::string::format("%s is not a valid EntityId", m_targetId.c_str()));
        }
    }

    void AltClickSceneElementState::OnStateActionsComplete()
    {
        if (m_altClickAction)
        {
            delete m_altClickAction;
            m_altClickAction = nullptr;
        }
    }

    ///////////////////////////////////
    // MouseToNodePropertyEditorState
    ///////////////////////////////////

    MouseToNodePropertyEditorState::MouseToNodePropertyEditorState(AutomationStateModelId slotId)
        : NamedAutomationState("MouseToNodePropertyEditorState")
        , m_slotId(slotId)
    {
        SetStateName(AZStd::string::format("MouseToNodePropertyEditorState::%s", slotId.c_str()));
    }

    void MouseToNodePropertyEditorState::OnSetupStateActions(EditorAutomationActionRunner& actionRunner)
    {
        const GraphCanvas::SlotId* slotId = GetStateModel()->GetStateDataAs<GraphCanvas::SlotId>(m_slotId);

        if (slotId)
        {
            m_moveToPropertyAction = aznew MouseToNodePropertyEditorAction((*slotId));
            actionRunner.AddAction(m_moveToPropertyAction);
            actionRunner.AddAction(&m_processEvents);
        }
        else
        {
            ReportError(AZStd::string::format("%s is not a valid SlotId", m_slotId.c_str()));
        }
    }

    void MouseToNodePropertyEditorState::OnStateActionsComplete()
    {
        delete m_moveToPropertyAction;
        m_moveToPropertyAction = nullptr;
    }
}
