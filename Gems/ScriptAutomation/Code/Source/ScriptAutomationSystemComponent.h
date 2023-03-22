/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Atom/Component/DebugCamera/CameraControllerBus.h>
#include <Atom/Feature/Utils/ProfilingCaptureBus.h>

#include <ScriptAutomation/ScriptAutomationBus.h>

#include <AssetStatusTracker.h>
#include <ImageComparisonOptions.h>
#include <ImGui/ImGuiAssetBrowser.h>
#include <ScriptReporter.h>

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
        , public AZ::Debug::CameraControllerNotificationBus::Handler
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
        void StartFrameCapture(const AZStd::string& imageName) override;
        void StopFrameCapture() override;
        void SetImageComparisonToleranceLevel(const AZStd::string& presetName) override;

        void StartProfilingCapture() override;

        void ActivateScript(const char* scriptPath) override;
        void DeactivateScripts() override;

        void SetCameraEntity(AZ::Entity* cameraEntity) override;
        const AZ::Entity* GetCameraEntity() const override;

        void StartAssetTracking() override;
        void StopAssetTracking() override;
        void ExpectAssets(const AZStd::string& sourceAssetPath, uint32_t expectedCount) override;
        void WaitForExpectAssetsFinish(float timeout) override;

        void ExecuteScript(const AZStd::string& scriptFilePath) override;

        // show/hide imgui
        void SetShowImGui(bool show) override;
        void RestoreShowImGui() override;
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

        // CameraControllerNotificationBus overrides...
        void OnCameraMoveEnded(AZ::TypeId controllerTypeId, uint32_t channels) override;

        void PrepareAndExecuteScript(const AZStd::string& scriptFilePath);
        void AbortScripts(const AZStd::string& reason);

        void RenderImGui();
        void OpenScriptRunnerDialog();
        void RenderScriptRunnerDialog();

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

        bool m_isCapturePending = false;

    private:

        ImGuiAssetBrowser m_scriptBrowser;
        ScriptReporter m_scriptReporter;
        //< Tracks when an executing script just finished so we know when to call ScriptReporter::PopScript().
        bool m_shouldPopScript = false;

        ImageComparisonOptions m_imageComparisonOptions;

        AZ::Entity* m_cameraEntity = nullptr;

        bool m_waitForAssetTracker = false;
        float m_assetTrackingTimeout = 0.0f;
        AssetStatusTracker m_assetStatusTracker;

        //< Tracks which lua scripts are currently being executed. Used to prevent infinite recursion.
        AZStd::unordered_set<AZ::Data::AssetId> m_executingScripts;

        bool m_prevShowImGui = true;
        bool m_showImGui = true;

        bool m_showScriptRunnerDialog = false;

        bool m_shouldRestoreViewportSize = false;
        int m_savedViewportWidth = 0;
        int m_savedViewportHeight = 0;

        bool m_doFinalScriptCleanup = false;
    };
} // namespace ScriptAutomation
