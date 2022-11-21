/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptAutomation/ScriptAutomationBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Atom/Feature/Utils/ProfilingCaptureBus.h>
#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/Feature/Utils/FrameCaptureTestBus.h>

namespace AZ
{
    class ScriptContext;
} // namespace AZ

namespace ScriptAutomation
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
    class ScriptAutomationSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public ScriptAutomationRequestBus::Handler
        , public AZ::Render::ProfilingCaptureNotificationBus::Handler
        , public AZ::Render::FrameCaptureNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(ScriptAutomationSystemComponent, "{755280BF-F227-4048-B323-D5E28EC55D61}", ScriptAutomationRequests);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ScriptAutomationSystemComponent();
        ~ScriptAutomationSystemComponent();

        void SetIdleFrames(int numFrames) override;
        void SetIdleSeconds(float numSeconds) override;
        void SetFrameCaptureId(AZ::Render::FrameCaptureId frameCaptureId) override;
        void StartProfilingCapture() override;

        void ActivateScript(const char* scriptPath) override;
        void DeactivateScripts() override;

    protected:
        // AZ::Component implementation
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // ScriptAutomationRequests implementation
        AZ::BehaviorContext* GetAutomationContext() override;
        void PauseAutomation(float timeout = DefaultPauseTimeout) override;
        void ResumeAutomation() override;
        void QueueScriptOperation(ScriptAutomationRequests::ScriptOperation&& operation) override;

        // FrameCaptureNotificationBus implementation
        void OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info) override;

        // ProfilingCaptureNotificationBus implementation
        void OnCaptureQueryTimestampFinished(bool result, const AZStd::string& info) override;
        void OnCaptureCpuFrameTimeFinished(bool result, const AZStd::string& info) override;
        void OnCaptureQueryPipelineStatisticsFinished(bool result, const AZStd::string& info) override;
        void OnCaptureBenchmarkMetadataFinished(bool result, const AZStd::string& info) override;


        void ExecuteScript(const char* scriptFilePath);

        AZStd::unique_ptr<AZ::ScriptContext> m_scriptContext; //< Provides the lua scripting system
        AZStd::unique_ptr<AZ::BehaviorContext> m_scriptBehaviorContext; //< Used to bind script callback functions to lua

        AZStd::queue<ScriptAutomationRequests::ScriptOperation> m_scriptOperations;

        AZStd::string m_automationScript;

        int m_scriptIdleFrames = 0;
        float m_scriptIdleSeconds = 0.0f;

        float m_scriptPauseTimeout = 0.0f;
        bool m_scriptPaused = false;
        AZ::Render::FrameCaptureId m_scriptFrameCaptureId = AZ::Render::InvalidFrameCaptureId;

        bool m_isStarted = false;
        bool m_exitOnFinish = false;
        bool m_doFinalCleanup = false;
    };
} // namespace ScriptAutomation
