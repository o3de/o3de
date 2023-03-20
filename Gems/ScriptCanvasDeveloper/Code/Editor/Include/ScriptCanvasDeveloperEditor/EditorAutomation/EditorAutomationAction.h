/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QWindow>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>

#include <AzCore/Outcome/Outcome.h>

class QMainWindow;

namespace ScriptCanvas::Developer
{
    typedef AZ::Outcome<void, AZStd::string> ActionReport;

    /**
        EditorAutomationAction is the base class that all other actions will inherit from. Exposes a setup, and a tick function which returns whether or not the action is complete.
    */
    class EditorAutomationAction
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorAutomationAction, AZ::SystemAllocator);
        AZ_RTTI(EditorAutomationAction, "{133E2ECE-5829-432F-BA37-60C75C5CCC34}")
    
        EditorAutomationAction() = default;
        virtual ~EditorAutomationAction() = default;

        bool IsAtPreconditionLimit() const { return m_preconditionAttempts >= 10; }        
        
        virtual bool IsMissingPrecondition() { return false; }

        void ResetPreconditionAttempts() { m_preconditionAttempts = 0; }

        EditorAutomationAction* GenerationPreconditionActions()
        {
            ++m_preconditionAttempts;
            return GenerateMissingPreconditionAction();
        }

        void SignalActionBegin()
        {
            ResetPreconditionAttempts();
            SetupAction();
        }
        
        virtual bool Tick() = 0;

        virtual ActionReport GenerateReport() const { return AZ::Success(); }

    protected:

        virtual EditorAutomationAction* GenerateMissingPreconditionAction() { return nullptr; }
        virtual void SetupAction() {}

    private:

        int m_preconditionAttempts = 0;
    };
}
