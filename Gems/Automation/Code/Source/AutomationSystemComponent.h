/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Automation/AutomationBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class ScriptContext;
} // namespace AZ

namespace Automation
{
    //! Manages running lua scripts for test automation.
    //! This initializes a lua context, binds C++ callback functions and does per-frame processing
    //! to execute scripts
    //!
    //! This uses an asynchronous execution model, which is necessary in order to allow scripts to
    //! simply call functions like IdleFrames() or IdleSeconds() to insert delays, making scripts
    //! much easier to write. When a script runs, every callback function adds an entry to an operations
    //! queue, and the Tick() function works its way through this queue every frame.
    //! Note that this means the C++ functions we expose to lua cannot return dynamic data; the only
    //! data we can return are constants like the number of samples available, or stateless utility
    //! functions
    class AutomationSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AutomationRequestBus::Handler
    {
    public:
        AZ_COMPONENT(AutomationSystemComponent, "{755280BF-F227-4048-B323-D5E28EC55D61}", AutomationRequests);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        AutomationSystemComponent();
        ~AutomationSystemComponent();

        void SetIdleFrames(int numFrames);
        void SetIdleSeconds(float numSeconds);

    protected:
        // AZ::Component implementation
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AutomationRequests implementation
        AZ::BehaviorContext* GetAutomationContext() override;
        void PauseAutomation(float timeout = DefaultPauseTimeout) override;
        void ResumeAutomation() override;
        void QueueScriptOperation(AutomationRequests::ScriptOperation&& operation) override;

        void ExecuteScript(const AZStd::string& scriptFilePath);

        AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext; //< Provides the lua scripting system
        AZStd::unique_ptr<AZ::BehaviorContext> m_scriptBehaviorContext; //< Used to bind script callback functions to lua

        AZStd::queue<AutomationRequests::ScriptOperation> m_scriptOperations;

        AZStd::string m_automationScript;

        int m_scriptIdleFrames = 0;
        float m_scriptIdleSeconds = 0.0f;

        float m_scriptPauseTimeout = 0.0f;
        bool m_scriptPaused = false;

        bool m_isStarted = false;
        bool m_exitOnFinish = false;
    };
} // namespace Automation
