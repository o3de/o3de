/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QApplication>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationActions/GenericActions.h>

namespace ScriptCanvas::Developer
{
    ///////////////////
    // CompoundAction
    ///////////////////

    CompoundAction::CompoundAction()
    {
    }

    CompoundAction::~CompoundAction()
    {
        ClearActionQueue();
    }

    void CompoundAction::SetupAction()
    {
        m_actionRunner.Reset();

        for (EditorAutomationAction* action : m_actionQueue)
        {
            m_actionRunner.AddAction(action);
        }

        m_errorReports.clear();
    }

    bool CompoundAction::Tick()
    {
        if (m_actionRunner.Tick())
        {
            m_errorReports = m_actionRunner.GetErrors();
            OnActionsComplete();
            return true;
        }

        return false;
    }

    void CompoundAction::AddAction(EditorAutomationAction* action)
    {
        m_actionQueue.emplace_back(action);
    }

    ActionReport CompoundAction::GenerateReport() const
    {
        if (!m_errorReports.empty())
        {
            AZStd::string finalErrorString = "Compound Action Error: ";

            bool firstAdd = false;

            for (ActionReport errorReport : m_errorReports)
            {
                if (!firstAdd)
                {
                    finalErrorString.append(", ");
                }

                firstAdd = false;

                finalErrorString.append(errorReport.GetError());
            }

            return AZ::Failure(finalErrorString);
        }

        return AZ::Success();
    }

    void CompoundAction::ClearActionQueue()
    {
        m_actionRunner.Reset();

        for (EditorAutomationAction* action : m_actionQueue)
        {
            delete action;
        }

        m_actionQueue.clear();

        m_errorReports.clear();
    }

    ////////////////
    // DelayAction
    ////////////////

    DelayAction::DelayAction(AZStd::chrono::milliseconds delayTime)
        : m_delay(delayTime)
    {
    }

    void DelayAction::SetupAction()
    {
        m_startPoint = AZStd::chrono::steady_clock::now();
    }

    bool DelayAction::Tick()
    {
        auto currentTime = AZStd::chrono::steady_clock::now();

        auto elapsedTime = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(currentTime - m_startPoint);

        return elapsedTime >= m_delay;
    }

    ////////////////////////////
    // ProcessUserEventsAction
    ////////////////////////////

    ProcessUserEventsAction::ProcessUserEventsAction(AZStd::chrono::milliseconds delayTime)
        : DelayAction(delayTime)
    {
    }

    void ProcessUserEventsAction::SetupAction()
    {
        DelayAction::SetupAction();

        m_delayComplete = false;
        m_processingComplete = false;
        m_processingEventsSwitch = false;
    }

    bool ProcessUserEventsAction::Tick()
    {
        if (!m_delayComplete)
        {
            m_delayComplete = DelayAction::Tick();
        }

        if (m_delayComplete && !m_processingEventsSwitch)
        {
            m_processingEventsSwitch = true;

            if (!m_processingComplete)
            {
                QApplication::processEvents();
                m_processingComplete = true;
            }
        }

        return m_delayComplete && m_processingComplete;
    }

    ///////////////
    // TraceEvent
    ///////////////

    TraceEvent::TraceEvent(AZStd::string traceName)
        : m_traceName(traceName)
    {

    }

    bool TraceEvent::Tick()
    {
        AZ_TracePrintf("Testing", "TraceEvent::%s", m_traceName.c_str());
        return true;
    }
}

