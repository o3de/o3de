/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QPoint>
#include <QWidget>

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Types/Types.h>

#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationAction.h>
#include <ScriptCanvasDeveloperEditor/EditorAutomation/EditorAutomationTest.h>

namespace ScriptCanvas::Developer
{
    /**
        EditorAutomationAction that will be composed of a series EditorAutomationActions that will be executed in order.
        The CompoundAction will take ownership of the actions added to it.
    */
    class CompoundAction
        : public EditorAutomationAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CompoundAction, AZ::SystemAllocator);
        AZ_RTTI(CompoundAction, "{3F9A5736-111C-4D49-A3D5-BA3484D74F4D}", EditorAutomationAction);

        CompoundAction();
        ~CompoundAction() override;

        void SetupAction() override;
        bool Tick() override;

        void AddAction(EditorAutomationAction* action);

        ActionReport GenerateReport() const override;

    protected:

        void ClearActionQueue();

        virtual void OnActionsComplete() {}

    private:

        AZStd::vector< ActionReport >       m_errorReports;
        AZStd::vector< EditorAutomationAction* > m_actionQueue;

        EditorAutomationActionRunner m_actionRunner;
    };

    /**
        EditorAutomationAction that will delay for the specified time
    */
    class DelayAction
        : public EditorAutomationAction
    {
    public:
        AZ_CLASS_ALLOCATOR(DelayAction, AZ::SystemAllocator);
        AZ_RTTI(DelayAction, "{44927F65-58DF-4142-8C04-6EC7A10050FB}", EditorAutomationAction);

        DelayAction(AZStd::chrono::milliseconds delayTime = AZStd::chrono::milliseconds(250));

        void SetupAction() override;
        bool Tick() override;

    private:

        AZStd::chrono::steady_clock::time_point m_startPoint;

        AZStd::chrono::milliseconds m_delay;
    };

    /**
        EditorAutomationAction that will delay so the OS can process the faked events, then pump the Qt application to process the events.
    */
    class ProcessUserEventsAction
        : public DelayAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ProcessUserEventsAction, AZ::SystemAllocator);
        AZ_RTTI(ProcessUserEventsAction, "{CB6D47D6-2E63-4277-BCD0-F9014D74CC48}", DelayAction);

        ProcessUserEventsAction(AZStd::chrono::milliseconds delayTime = AZStd::chrono::milliseconds(250));

        void SetupAction() override;

        bool Tick() override;
    private:

        bool                m_delayComplete = false;
        bool                m_processingComplete = false;
        AZStd::atomic<bool> m_processingEventsSwitch = false;
    };

    /**
        EditorAutomationAction that will print out the specified string during execution
    */
    class TraceEvent
        : public EditorAutomationAction
    {
    public:
        AZ_CLASS_ALLOCATOR(TraceEvent, AZ::SystemAllocator);
        AZ_RTTI(TraceEvent, "{A2C0B4B1-9FD7-4441-86EB-B63C8DDD2C97}", EditorAutomationAction);

        TraceEvent(AZStd::string traceName);

        bool Tick() override;

    private:

        AZStd::string m_traceName;
    };
}
