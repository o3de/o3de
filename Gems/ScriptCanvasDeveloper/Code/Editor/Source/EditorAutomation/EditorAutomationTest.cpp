/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationTest.h>

namespace ScriptCanvasDeveloper
{
    /////////////////////////////////
    // EditorAutomationActionRunner
    /////////////////////////////////

    EditorAutomationActionRunner::~EditorAutomationActionRunner()
    {
        Reset();
    }

    void EditorAutomationActionRunner::Reset()
    {
        for (EditorAutomationAction* deleteAction : m_actionsToDelete)
        {
            delete deleteAction;
        }

        m_actionsToDelete.clear();

        while (!m_executionStack.empty())
        {
            m_executionStack.erase(m_executionStack.begin());
        }

        m_errorReports.clear();
        m_currentAction = nullptr;
    }

    bool EditorAutomationActionRunner::Tick()
    {
        if (m_currentAction == nullptr)
        {
            if (m_executionStack.empty())
            {
                return true;
            }
            else
            {
                m_currentAction = m_executionStack.front();

                while (m_currentAction->IsMissingPrecondition())
                {
                    if (m_currentAction->IsAtPreconditionLimit())
                    {
                        ActionReport failureReport;
                        failureReport = AZ::Failure<AZStd::string>("Action failed to setup its preconditions in a reasonable amount of iterations. Exiting test.");
                        m_errorReports.emplace_back(failureReport);

                        m_currentAction->ResetPreconditionAttempts();

                        // Leak elements, then we'll just exit through normal paths next tick.
                        m_executionStack.clear();

                        return false;
                    }
                    else
                    {
                        EditorAutomationAction* newAction = m_currentAction->GenerationPreconditionActions();

                        m_actionsToDelete.emplace(newAction);

                        if (newAction)
                        {
                            m_currentAction = newAction;
                            m_executionStack.insert(m_executionStack.begin(), newAction);
                        }
                    }
                }

                // Erase our current front of stack, as that is our current action.
                m_executionStack.erase(m_executionStack.begin());

                AZ_Assert(m_currentAction, "Current Action should not be null at this point.");

                if (m_currentAction == nullptr)
                {
                    // Leak all the memory. We'll just exit through the normal path next tick.
                    m_executionStack.clear();

                    return true;
                }
                else
                {
                    m_currentAction->SignalActionBegin();
                }
            }
        }

        if (m_currentAction)
        {
            if (m_currentAction->Tick())
            {
                ActionReport errorReport = m_currentAction->GenerateReport();

                if (!errorReport.IsSuccess())
                {
                    m_errorReports.emplace_back(errorReport);
                }

                m_currentAction = nullptr;
            }
        }

        return false;
    }

    void EditorAutomationActionRunner::AddAction(EditorAutomationAction* actionToRun)
    {
        m_executionStack.emplace_back(actionToRun);
    }

    bool EditorAutomationActionRunner::HasActions() const
    {
        return !m_executionStack.empty();
    }

    bool EditorAutomationActionRunner::HasErrors() const
    {
        return !m_errorReports.empty();
    }

    const AZStd::vector< ActionReport >& EditorAutomationActionRunner::GetErrors() const
    {
        return m_errorReports;
    }

    ///////////////
    // StateModel
    ///////////////

    const AZStd::any* StateModel::FindStateData(const DataKey& dataId) const
    {
        auto dataIter = m_stateData.find(dataId);

        if (dataIter != m_stateData.end())
        {
            return &dataIter->second;
        }

        return nullptr;
    }

    void StateModel::ClearModelData()
    {
        m_stateData.clear();
    }

    /////////////////////////
    // EditorAutomationTest
    /////////////////////////

    EditorAutomationTest::EditorAutomationTest(QString testName)
        : m_testName(testName)
    {
    }

    EditorAutomationTest::~EditorAutomationTest()
    {
        for (const auto& statePair : m_states)
        {
            delete statePair.second;
        }

        m_states.clear();
    }

    void EditorAutomationTest::StartTest()
    {
        m_hasRun = true;

        m_testErrors.clear();

        OnTestStarting();

        m_stateId = m_initialStateId;
        m_actionRunner.Reset();

        if (SetupState(m_stateId))
        {
            AZ::SystemTickBus::Handler::BusConnect();
        }
        else
        {
            OnTestComplete();
        }
    }

    void EditorAutomationTest::OnSystemTick()
    {
        if (m_actionRunner.Tick())
        {
            if (!HasErrors())
            {
                if (m_actionRunner.HasErrors())
                {
                    const AZStd::vector<ActionReport>& actionErrors = m_actionRunner.GetErrors();

                    for (const ActionReport& actionErrorReport : actionErrors)
                    {
                        AddError(actionErrorReport.GetError());
                    }
                }
                else
                {
                    m_currentState->StateActionsComplete();

                    if (m_currentState->HasErrors())
                    {
                        AddError(m_currentState->GetError());
                    }

                    m_currentState = nullptr;
                }

                //++m_state;

                m_actionRunner.Reset();

                if (!HasErrors())
                {
                    OnStateComplete(m_stateId);

                    int nextStateId = FindNextState(m_stateId);
                    if (!SetupState(nextStateId))
                    {
                        AZ::SystemTickBus::Handler::BusDisconnect();
                        OnTestComplete();
                    }
                }
                else
                {
                    AZ::SystemTickBus::Handler::BusDisconnect();
                    OnTestComplete();
                }
            }
            else
            {
                AZ::SystemTickBus::Handler::BusDisconnect();
                OnTestComplete();
            }
        }
    }

    void EditorAutomationTest::AddState(EditorAutomationState* newState)
    {
        int stateId = newState->GetStateId();

        if (stateId == EditorAutomationState::EXIT_STATE_ID)
        {
            AZ_Error("EditorAutomationTest", false, "Trying to use reserved exit state id");
            delete newState;
            return;
        }

        auto stateIter = m_states.find(stateId);

        if (stateIter != m_states.end())
        {
            AZ_Error("EditorAutomationTest", false, "Collision on StateId %i found. Maintaining first state with id", stateId);
            delete newState;
            return;
        }

        m_registrationOrder.emplace_back(stateId);
        m_states[stateId] = newState;
        newState->SetStateModel(this);

        if (m_initialStateId == EditorAutomationState::EXIT_STATE_ID)
        {
            m_initialStateId = stateId;
        }
    }

    void EditorAutomationTest::SetHasCustomTransitions(bool hasCustomTransition)
    {
        m_hasCustomTransitions = hasCustomTransition;
    }

    bool EditorAutomationTest::HasRun() const
    {
        return m_hasRun;
    }

    bool EditorAutomationTest::IsRunning() const
    {
        return AZ::SystemTickBus::Handler::BusIsConnected();
    }

    bool EditorAutomationTest::SetupState(int stateId)
    {
        m_stateId = EditorAutomationState::EXIT_STATE_ID;
        m_currentState = nullptr;

        auto stateIter = m_states.find(stateId);

        if (stateIter != m_states.end())
        {
            m_currentState = stateIter->second;
        }

        if (m_currentState)
        {
            m_stateId = stateId;

            m_actionRunner.Reset();
            m_currentState->SetupStateActions(m_actionRunner);
        }

        return m_currentState != nullptr;
    }

    int EditorAutomationTest::FindNextState(int stateId)
    {
        int nextStateId = EditorAutomationState::EXIT_STATE_ID;

        if (m_hasCustomTransitions)
        {
            nextStateId = EvaluateTransition(stateId);
        }
        else
        {
            // Do a minus one here, so we can just blindly add 1 instead of needing to safety check it.
            // We default to EXIT so the result is the same.
            for (int i = 0; i < m_registrationOrder.size() - 1; ++i)
            {
                if (m_registrationOrder[i] == stateId)
                {
                    nextStateId = m_registrationOrder[i + 1];
                }
            }
        }

        return nextStateId;
    }

    void EditorAutomationTest::AddError(AZStd::string error)
    {
        AZ_TracePrintf("EditorAutomationTestDialog", "Error in %s :: %s", m_testName.toUtf8().data(), error.c_str());
        m_testErrors.emplace_back(error);
    }
}
