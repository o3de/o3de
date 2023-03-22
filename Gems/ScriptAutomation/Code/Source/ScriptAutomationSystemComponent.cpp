/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Script/ScriptComponent.h>

#include <Atom/Component/DebugCamera/CameraComponent.h>
#include <Atom/Component/DebugCamera/NoClipControllerComponent.h>
#include <Atom/Component/DebugCamera/ArcBallControllerComponent.h>
#include <Atom/Feature/ImGui/SystemBus.h>

#include <ScriptableImGui.h>
#include <ScriptAutomationScriptBindings.h>
#include <ScriptAutomationSystemComponent.h>

namespace ScriptAutomation
{
    namespace
    {
        AZ::Data::Asset<AZ::ScriptAsset> LoadScriptAssetFromPath(const char* productPath, AZ::ScriptContext& context)
        {
            AZ::IO::FixedMaxPath resolvedPath;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, productPath);

            AZ::IO::FileIOStream inputStream;
            if (inputStream.Open(resolvedPath.c_str(), AZ::IO::OpenMode::ModeRead))
            {
                AzFramework::ScriptCompileRequest compileRequest;
                compileRequest.m_sourceFile = resolvedPath.c_str();
                compileRequest.m_input = &inputStream;

                auto outcome = AzFramework::CompileScript(compileRequest, context);
                inputStream.Close();
                if (outcome.IsSuccess())
                {
                    AZ::Uuid id = AZ::Uuid::CreateName(productPath);
                    AZ::Data::Asset<AZ::ScriptAsset> scriptAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::ScriptAsset>(AZ::Data::AssetId(id)
                        , AZ::Data::AssetLoadBehavior::Default);
                    scriptAsset.Get()->m_data = compileRequest.m_luaScriptDataOut;

                    return scriptAsset;
                }
                else
                {
                    AZ_Assert(false, "Failed to compile script asset '%s'. Reason: '%s'", resolvedPath.c_str(), outcome.GetError().c_str());
                    return {};
                }
            }
            else
            {
                AZ_Assert(false, "Unable to find product asset '%s'. Has the source asset finished building?", resolvedPath.c_str());
                return {};
            }
        }
    } // namespace

    void ExecuteLuaScript(const AZ::ConsoleCommandContainer& arguments)
    {
        auto scriptAuto = ScriptAutomationInterface::Get();

        if (!scriptAuto)
        {
            AZ_Error("ScriptAutomation", false, "There is no ScriptAutomation instance registered to the interface.");
            return;
        }

        const char* scriptPath = arguments[0].data();

        scriptAuto->ActivateScript(scriptPath);
    }

    AZ_CONSOLEFREEFUNC(ExecuteLuaScript, AZ::ConsoleFunctorFlags::Null, "Execute a Lua script");

    void ScriptAutomationSystemComponent::ActivateScript(const char* scriptPath)
    {
        m_isStarted = false;
        m_automationScript = scriptPath;

        AZ::TickBus::Handler::BusConnect();
    }

    void ScriptAutomationSystemComponent::DeactivateScripts()
    {
        m_isStarted = false;
        m_automationScript = "";
        AZ::TickBus::Handler::BusDisconnect();
    }

    void ScriptAutomationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptAutomationSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ScriptAutomationSystemComponent>("ScriptAutomation", "Provides a mechanism for automating various tasks through Lua scripting in the game launchers")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void ScriptAutomationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AutomationServiceCrc);
    }

    void ScriptAutomationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AutomationServiceCrc);
    }

    void ScriptAutomationSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ScriptAutomationSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ScriptAutomationSystemComponent::ScriptAutomationSystemComponent()
        : m_scriptBrowser("@user@/lua_script_browser.xml")
    {
        if (ScriptAutomationInterface::Get() == nullptr)
        {
            ScriptAutomationInterface::Register(this);
        }
    }

    ScriptAutomationSystemComponent::~ScriptAutomationSystemComponent()
    {
        if (ScriptAutomationInterface::Get() == this)
        {
            ScriptAutomationInterface::Unregister(this);
        }
    }

    void ScriptAutomationSystemComponent::SetIdleFrames(int numFrames)
    {
        AZ_Assert(m_scriptIdleSeconds == 0, "m_scriptIdleFrames is being stomped");
        m_scriptIdleFrames = numFrames;
    }

    void ScriptAutomationSystemComponent::SetIdleSeconds(float numSeconds)
    {
        m_scriptIdleSeconds = numSeconds;
    }

    void ScriptAutomationSystemComponent::SetFrameCaptureId(AZ::Render::FrameCaptureId frameCaptureId)
    {
         // FrameCapture system supports multiple active frame captures, Script Automation would need changes to support more than 1 active at a time.
        AZ_Assert(m_scriptFrameCaptureId == AZ::Render::InvalidFrameCaptureId, "Attempting to start a frame capture while one is in progress");
        m_scriptFrameCaptureId = frameCaptureId;
        AZ::Render::FrameCaptureNotificationBus::Handler::BusConnect(frameCaptureId);
    }

    void ScriptAutomationSystemComponent::StartFrameCapture(const AZStd::string& imageName)
    {
        AZ_Assert(m_scriptFrameCaptureId == AZ::Render::InvalidFrameCaptureId,
            "Attempting to start a capture while one is in progress");
        m_scriptReporter.AddScreenshotTest(imageName);
        m_isCapturePending = true;
    }

    void ScriptAutomationSystemComponent::StopFrameCapture()
    {
        m_isCapturePending = false;
        m_scriptFrameCaptureId = AZ::Render::InvalidFrameCaptureId;
    }

    void ScriptAutomationSystemComponent::SetImageComparisonToleranceLevel(const AZStd::string& presetName)
    {
        m_imageComparisonOptions.SelectToleranceLevel(presetName);
    }

    void ScriptAutomationSystemComponent::StartProfilingCapture()
    {
        m_isCapturePending = true;
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusConnect();
    }

    void ScriptAutomationSystemComponent::Activate()
    {
        ScriptAutomationRequestBus::Handler::BusConnect();
        ScriptableImGui::Create();

        m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
        m_scriptBehaviorContext = AZStd::make_unique<AZ::BehaviorContext>();

        ReflectScriptBindings(m_scriptBehaviorContext.get());
        m_scriptContext->BindTo(m_scriptBehaviorContext.get());

        m_scriptBrowser.SetFilter([](const AZ::Data::AssetInfo& assetInfo)
        {
            return AzFramework::StringFunc::EndsWith(assetInfo.m_relativePath, ".bv.luac");
        });

        m_scriptBrowser.Activate();
        m_imageComparisonOptions.Activate();

        AZ::ComponentApplication* application = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(application, &AZ::ComponentApplicationBus::Events::GetApplication);
        if (application)
        {
            constexpr const char* automationSuiteSwitch = "run-automation-suite";
            constexpr const char* automationExitSwitch = "exit-on-automation-end";

            auto commandLine = application->GetAzCommandLine();
            if (commandLine->HasSwitch(automationSuiteSwitch))
            {
                m_exitOnFinish = commandLine->HasSwitch(automationExitSwitch);
                ActivateScript(commandLine->GetSwitchValue(automationSuiteSwitch, 0).c_str());
            }
        }
    }

    void ScriptAutomationSystemComponent::Deactivate()
    {
        DeactivateScripts();

        m_scriptContext = nullptr;
        m_scriptBehaviorContext = nullptr;

        m_scriptBrowser.Deactivate();
        m_imageComparisonOptions.Deactivate();

        ScriptableImGui::Destory();

        ScriptAutomationRequestBus::Handler::BusDisconnect();
    }

    void ScriptAutomationSystemComponent::SetCameraEntity(AZ::Entity* cameraEntity)
    {
        AZ::Debug::CameraControllerNotificationBus::Handler::BusDisconnect();
        m_cameraEntity = cameraEntity;
        AZ::Debug::CameraControllerNotificationBus::Handler::BusConnect(m_cameraEntity->GetId());
    }

    const AZ::Entity* ScriptAutomationSystemComponent::GetCameraEntity() const
    {
        return m_cameraEntity;
    }

    void ScriptAutomationSystemComponent::StartAssetTracking()
    {
        m_assetStatusTracker.StartTracking();
    }

    void ScriptAutomationSystemComponent::StopAssetTracking()
    {
        m_assetStatusTracker.StopTracking();
    }

    void ScriptAutomationSystemComponent::ExpectAssets(const AZStd::string& sourceAssetPath, uint32_t expectedCount)
    {
        m_assetStatusTracker.ExpectAsset(sourceAssetPath, expectedCount);
    }

    void ScriptAutomationSystemComponent::WaitForExpectAssetsFinish(float timeout)
    {
        AZ_Assert(m_waitForAssetTracker, "It shouldn't be possible to run the next command until m_waitForAssetTracker is false");

        m_waitForAssetTracker = true;
        m_assetTrackingTimeout = timeout;
    }

    void ScriptAutomationSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // All actions must be consumed each frame. Otherwise, this indicates that a script is
        // scheduling ScriptableImGui actions for fields that don't exist.
        ScriptableImGui::CheckAllActionsConsumed();
        ScriptableImGui::ClearActions();

        // We delayed PopScript() until after the above CheckAllActionsConsumed(), so that any errors
        // reported by that function will be associated with the proper script.
        if (m_shouldPopScript)
        {
            m_scriptReporter.PopScript();
            m_shouldPopScript = false;
        }

        if (!m_isStarted)
        {
            m_isStarted = true;
            ExecuteScript(m_automationScript.c_str());

            ScriptAutomationNotificationBus::Broadcast(&ScriptAutomationNotificationBus::Events::OnAutomationStarted);
        }

        while (!m_scriptOperations.empty())
        {
            if (m_scriptPaused)
            {
                m_scriptPauseTimeout -= deltaTime;
                if (m_scriptPauseTimeout < 0)
                {
                    AZ_Error("ScriptAutomation", false, "Script pause timed out. Continuing...");
                    m_scriptPaused = false;
                }
                else
                {
                    break;
                }
            }

            if (m_waitForAssetTracker)
            {
                m_assetTrackingTimeout -= deltaTime;
                if (m_assetTrackingTimeout < 0)
                {
                    auto incomplateAssetList = m_assetStatusTracker.GetIncompleteAssetList();
                    AZStd::string incompleteAssetListString;
                    AzFramework::StringFunc::Join(incompleteAssetListString, incomplateAssetList.begin(), incomplateAssetList.end(), "\n    ");
                    AZ_Error("Automation", false, "Script asset tracking timed out waiting for:\n    %s \n Continuing...", incompleteAssetListString.c_str());
                    m_waitForAssetTracker = false;
                }
                else if (m_assetStatusTracker.DidExpectedAssetsFinish())
                {
                    AZ_Printf("Automation", "Asset Tracker finished with %f seconds remaining.", m_assetTrackingTimeout);
                    m_waitForAssetTracker = false;
                }
                else
                {
                    break;
                }
            }

            if (m_scriptIdleFrames > 0)
            {
                m_scriptIdleFrames--;
                break;
            }

            if (m_scriptIdleSeconds > 0)
            {
                m_scriptIdleSeconds -= deltaTime;
                break;
            }

            if (!m_scriptOperations.empty()) // may be looping waiting for final pause to finish
            {
                // Execute the next operation
                m_scriptOperations.front()();

                m_scriptOperations.pop();
            }

            if (m_scriptOperations.empty())
            {
                if(!m_scriptPaused) // final operation may have paused, wait for it to complete or time out
                {
                    DeactivateScripts();

                    ScriptAutomationNotificationBus::Broadcast(&ScriptAutomationNotificationBus::Events::OnAutomationFinished);

                    if (m_exitOnFinish)
                    {
                        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
                    }
                }
                break;
            }
        }

        if (m_shouldPopScript)
        {
            // We need to proceed for one more frame to do the last PopScript() before final cleanup
            return;
        }

        if (m_doFinalScriptCleanup)
        {
            bool frameCapturePending = false;
            //SampleComponentManagerRequestBus::BroadcastResult(frameCapturePending, &SampleComponentManagerRequests::IsFrameCapturePending);
            if (!frameCapturePending && !m_isCapturePending)
            {
                AZ_Assert(m_scriptPaused == false, "Script manager is in an unexpected state.");
                AZ_Assert(m_scriptIdleFrames == 0, "Script manager is in an unexpected state.");
                AZ_Assert(m_scriptIdleSeconds <= 0.0f, "Script manager is in an unexpected state.");
                AZ_Assert(m_waitForAssetTracker == false, "Script manager is in an unexpected state.");
                AZ_Assert(!m_scriptReporter.HasActiveScript(), "Script manager is in an unexpected state.");
                AZ_Assert(m_executingScripts.size() == 0, "Script manager is in an unexpected state");

                m_assetStatusTracker.StopTracking();

                if (m_shouldRestoreViewportSize)
                {
                    Utils::ResizeClientArea(m_savedViewportWidth, m_savedViewportHeight, AzFramework::WindowPosOptions());
                    m_shouldRestoreViewportSize = false;
                }

                // In case scripts were aborted while ImGui was temporarily hidden, show it again.
                SetShowImGui(true);

                m_scriptReporter.SortScriptReports();
                m_scriptReporter.OpenReportDialog();

                m_shouldPopScript = false;
                m_doFinalScriptCleanup = false;
            }
        }
    }

    AZ::BehaviorContext* ScriptAutomationSystemComponent::GetAutomationContext()
    {
        return m_scriptBehaviorContext.get();
    }

    void ScriptAutomationSystemComponent::PauseAutomation(float timeout)
    {
        m_scriptPaused = true;
        m_scriptPauseTimeout = AZ::GetMax(timeout, m_scriptPauseTimeout);
    }

    void ScriptAutomationSystemComponent::ResumeAutomation()
    {
        AZ_Warning("ScriptAutomation", m_scriptPaused, "Script is not paused.");
        m_scriptPaused = false;
    }

    void ScriptAutomationSystemComponent::QueueScriptOperation(ScriptAutomationRequests::ScriptOperation&& operation)
    {
        m_scriptOperations.push(AZStd::move(operation));
    }

    void ScriptAutomationSystemComponent::PrepareAndExecuteScript(const AZStd::string& scriptFilePath)
    {
        // Save the window size so we can restore it after running the script, in case the script calls ResizeViewport
        AzFramework::NativeWindowHandle defaultWindowHandle;
        AzFramework::WindowSize windowSize;
        AzFramework::WindowSystemRequestBus::BroadcastResult(
            defaultWindowHandle, &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);
        AzFramework::WindowRequestBus::EventResult(windowSize, defaultWindowHandle, &AzFramework::WindowRequests::GetClientAreaSize);
        m_savedViewportWidth = windowSize.m_width;
        m_savedViewportHeight = windowSize.m_height;
        if (m_savedViewportWidth == 0 || m_savedViewportHeight == 0)
        {
            AZ_Assert(false, "Could not get current window size");
        }
        else
        {
            m_shouldRestoreViewportSize = true;
        }

        // Setup the ScriptReporter to track and report the results
        m_scriptReporter.Reset();
        m_scriptReporter.SetAvailableToleranceLevels(m_imageComparisonOptions.GetAvailableToleranceLevels());
        if (m_imageComparisonOptions.IsLevelAdjusted())
        {
            m_scriptReporter.SetInvalidationMessage("Results are invalid because the tolerance level has been adjusted.");
        }
        else if (!m_imageComparisonOptions.IsScriptControlled())
        {
            m_scriptReporter.SetInvalidationMessage("Results are invalid because the tolerance level has been overridden.");
        }
        else
        {
            m_scriptReporter.SetInvalidationMessage("");
        }

        AZ_Assert(m_executingScripts.empty(), "There should be no active scripts at this point");

        ExecuteScript(scriptFilePath.c_str());
    }

    void ScriptAutomationSystemComponent::ExecuteScript(const AZStd::string& scriptFilePath)
    {
        AZ::Data::Asset<AZ::ScriptAsset> scriptAsset = LoadScriptAssetFromPath(scriptFilePath.c_str(), *m_scriptContext.get());
        if (!scriptAsset)
        {
            QueueScriptOperation(
                [scriptFilePath]()
                {
                    AZ_Error("ScriptAutomation", false, "Script: Could not find or load script asset '%s'.", scriptFilePath.c_str());
                });
            return;
        }

        if (m_executingScripts.find(scriptAsset.GetId()) != m_executingScripts.end())
        {
            QueueScriptOperation(
                [scriptFilePath]()
                {
                    AZ_Error(
                        "ScriptAutomation",
                        false,
                        "Calling script '%s' would likely cause an infinite loop and crash. Skipping.",
                        scriptFilePath.c_str());
                });
            return;
        }

        if (m_imageComparisonOptions.IsScriptControlled())
        {
            // Clear the preset before each script to make sure the script is selecting it.
            m_imageComparisonOptions.SelectToleranceLevel(nullptr);
        }

        // Execute(script) will add commands to the m_scriptOperations. These should be considered part of their own test script, for
        // reporting purposes.
        QueueScriptOperation(
            [scriptFilePath, this]()
            {
                m_scriptReporter.PushScript(scriptFilePath);
            });

        QueueScriptOperation(
            [scriptFilePath]()
            {
                AZ_Printf("ScriptAutomation", "Running script '%s'...\n", scriptFilePath.c_str());
            });

        m_executingScripts.insert(scriptAsset.GetId());

        auto& scriptData = scriptAsset->m_data;
        if (!m_scriptContext->Execute(scriptData.GetScriptBuffer().data(), scriptFilePath.c_str(), scriptData.GetScriptBuffer().size()))
        {
            // Push an error operation on the back of the queue instead of reporting it immediately so it doesn't get lost
            // in front of a bunch of queued m_scriptOperations.
            AZ_Error("ScriptAutomation", false, "Error running script '%s'.", scriptAsset.ToString<AZStd::string>().c_str());
        }

        m_executingScripts.erase(scriptAsset.GetId());

        // Execute(script) will have added commands to the m_scriptOperations. When they finish, consider this test as completed, for
        // reporting purposes.
        QueueScriptOperation(
            [this]()
            {
                // We don't call m_scriptReporter.PopScript() yet because some cleanup needs to happen in TickScript() on the next frame.
                AZ_Assert(!m_shouldPopScript, "m_shouldPopScript is already true");
                m_shouldPopScript = true;
            });
    }

    void ScriptAutomationSystemComponent::AbortScripts(const AZStd::string& reason)
    {
        m_scriptReporter.SetInvalidationMessage(reason);

        m_scriptOperations = {};
        m_executingScripts.clear();
        m_scriptPaused = false;
        m_scriptIdleFrames = 0;
        m_scriptIdleSeconds = 0.0f;
        m_waitForAssetTracker = false;
        while (m_scriptReporter.HasActiveScript())
        {
            m_scriptReporter.PopScript();
        }

        m_doFinalScriptCleanup = true;
    }

    void ScriptAutomationSystemComponent::OnCaptureQueryTimestampFinished([[maybe_unused]] bool result, [[maybe_unused]] const AZStd::string& info)
    {
        m_isCapturePending = false;
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();
    }

    void ScriptAutomationSystemComponent::OnCaptureCpuFrameTimeFinished([[maybe_unused]] bool result, [[maybe_unused]] const AZStd::string& info)
    {
        m_isCapturePending = false;
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();
    }

    void ScriptAutomationSystemComponent::OnCaptureQueryPipelineStatisticsFinished([[maybe_unused]] bool result, [[maybe_unused]] const AZStd::string& info)
    {
        m_isCapturePending = false;
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();
    }

    void ScriptAutomationSystemComponent::OnCaptureBenchmarkMetadataFinished([[maybe_unused]] bool result, [[maybe_unused]] const AZStd::string& info)
    {
        m_isCapturePending = false;
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();
    }

    void ScriptAutomationSystemComponent::OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string &info)
    {
        StopFrameCapture();
        AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();

        // This is checking for the exact scenario that results from an HDR setup. The goal is to add a very specific and prominent message that will
        // alert users to a common issue and what action to take. Any other Format issues will be reported by FrameCaptureSystemComponent with a
        // "Can't save image with format %s to a ppm file" message.
        if (result == AZ::Render::FrameCaptureResult::UnsupportedFormat
            && info.find(AZ::RHI::ToString(AZ::RHI::Format::R10G10B10A2_UNORM)) != AZStd::string::npos)
        {
            AZ_Assert(false,
                "ScriptAutomation Screen Capture - HDR Not Supported, "
                "Screen capture to image is not supported from RGB10A2 display format. "
                "Please change the system configuration to disable the HDR display feature.");
        }
    }

    void ScriptAutomationSystemComponent::OnCameraMoveEnded(AZ::TypeId controllerTypeId, uint32_t channels)
    {
        if (controllerTypeId == azrtti_typeid<AZ::Debug::ArcBallControllerComponent>())
        {
            if (channels & AZ::Debug::ArcBallControllerChannel_Center)
            {
                AZ::Vector3 center = AZ::Vector3::CreateZero();
                AZ::Debug::ArcBallControllerRequestBus::EventResult(
                    center, m_cameraEntity->GetId(), &AZ::Debug::ArcBallControllerRequests::GetCenter);

                AZ_TracePrintf(
                    "ScriptAutomation",
                    "ArcBallCameraController_SetCenter(Vector3(%f, %f, %f))\n",
                    (float)center.GetX(),
                    (float)center.GetY(),
                    (float)center.GetZ());
            }

            if (channels & AZ::Debug::ArcBallControllerChannel_Pan)
            {
                AZ::Vector3 pan = AZ::Vector3::CreateZero();
                AZ::Debug::ArcBallControllerRequestBus::EventResult(
                    pan, m_cameraEntity->GetId(), &AZ::Debug::ArcBallControllerRequests::GetPan);

                AZ_TracePrintf(
                    "ScriptAutomation",
                    "ArcBallCameraController_SetPan(Vector3(%f, %f, %f))",
                    (float)pan.GetX(),
                    (float)pan.GetY(),
                    (float)pan.GetZ());
            }

            if (channels & AZ::Debug::ArcBallControllerChannel_Heading)
            {
                float heading = 0.0;
                AZ::Debug::ArcBallControllerRequestBus::EventResult(
                    heading, m_cameraEntity->GetId(), &AZ::Debug::ArcBallControllerRequests::GetHeading);

                AZ_TracePrintf("ScriptAutomation", "ArcBallCameraController_SetHeading(DegToRad(%f))", AZ::RadToDeg(heading));
            }

            if (channels & AZ::Debug::ArcBallControllerChannel_Pitch)
            {
                float pitch = 0.0;
                AZ::Debug::ArcBallControllerRequestBus::EventResult(
                    pitch, m_cameraEntity->GetId(), &AZ::Debug::ArcBallControllerRequests::GetPitch);

                AZ_TracePrintf("ScriptAutomation", "ArcBallCameraController_SetPitch(DegToRad(%f))", AZ::RadToDeg(pitch));
            }

            if (channels & AZ::Debug::ArcBallControllerChannel_Distance)
            {
                float distance = 0.0;
                AZ::Debug::ArcBallControllerRequestBus::EventResult(
                    distance, m_cameraEntity->GetId(), &AZ::Debug::ArcBallControllerRequests::GetDistance);

                AZ_TracePrintf("ScriptAutomation", "ArcBallCameraController_SetDistance(%f)", distance);
            }
        }

        if (controllerTypeId == azrtti_typeid<AZ::Debug::NoClipControllerComponent>())
        {
            if (channels & AZ::Debug::NoClipControllerChannel_Position)
            {
                AZ::Vector3 position = AZ::Vector3::CreateZero();
                AZ::Debug::NoClipControllerRequestBus::EventResult(
                    position, m_cameraEntity->GetId(), &AZ::Debug::NoClipControllerRequests::GetPosition);

                AZ_TracePrintf(
                    "ScriptAutomation",
                    "NoClipCameraController_SetPosition(Vector3(%f, %f, %f))",
                    (float)position.GetX(),
                    (float)position.GetY(),
                    (float)position.GetZ());
            }

            if (channels & AZ::Debug::NoClipControllerChannel_Orientation)
            {
                float heading = 0.0;
                AZ::Debug::NoClipControllerRequestBus::EventResult(
                    heading, m_cameraEntity->GetId(), &AZ::Debug::NoClipControllerRequests::GetHeading);
                AZ_TracePrintf("ScriptAutomation", "NoClipCameraController_SetHeading(DegToRad(%f))", AZ::RadToDeg(heading));

                float pitch = 0.0;
                AZ::Debug::NoClipControllerRequestBus::EventResult(
                    pitch, m_cameraEntity->GetId(), &AZ::Debug::NoClipControllerRequests::GetPitch);
                AZ_TracePrintf("ScriptAutomation", "NoClipCameraController_SetPitch(DegToRad(%f))", AZ::RadToDeg(pitch));
            }

            if (channels & AZ::Debug::NoClipControllerChannel_Fov)
            {
                float fov = 0.0;
                AZ::Debug::NoClipControllerRequestBus::EventResult(
                    fov, m_cameraEntity->GetId(), &AZ::Debug::NoClipControllerRequests::GetFov);
                 AZ_TracePrintf("ScriptAutomation", "NoClipCameraController_SetFov(DegToRad(%f))", AZ::RadToDeg(fov));
            }
        }
    }

    void ScriptAutomationSystemComponent::SetShowImGui(bool show)
    {
        m_prevShowImGui = m_showImGui;
        if (show)
        {
            AZ::Render::ImGuiSystemRequestBus::Broadcast(&AZ::Render::ImGuiSystemRequestBus::Events::ShowAllImGuiPasses);
        }
        else
        {
            AZ::Render::ImGuiSystemRequestBus::Broadcast(&AZ::Render::ImGuiSystemRequestBus::Events::HideAllImGuiPasses);
        }
        m_showImGui = show;
    }

    void ScriptAutomationSystemComponent::RestoreShowImGui()
    {
        SetShowImGui(m_prevShowImGui);
    }

    void ScriptAutomationSystemComponent::OpenScriptRunnerDialog()
    {
        m_showScriptRunnerDialog = true;
    }

    void ScriptAutomationSystemComponent::RenderScriptRunnerDialog()
    {
        if (ImGui::Begin("Script Runner", &m_showScriptRunnerDialog))
        {
            auto drawAbortButton = [this](const char* uniqueId)
            {
                ImGui::PushID(uniqueId);

                if (ImGui::Button("Abort"))
                {
                    AbortScripts("Script(s) manually aborted.");
                }

                ImGui::PopID();
            };

            // The main buttons are at the bottom, but show the Abort button at the top too, in case the window size is small.
            if (!m_scriptOperations.empty())
            {
                drawAbortButton("Button1");
            }

            ImGuiAssetBrowser::WidgetSettings assetBrowserSettings;
            assetBrowserSettings.m_labels.m_root = "Lua Scripts";
            m_scriptBrowser.Tick(assetBrowserSettings);

            AZStd::string selectedFileName = "<none>";
            AzFramework::StringFunc::Path::GetFullFileName(m_scriptBrowser.GetSelectedAssetPath().c_str(), selectedFileName);
            ImGui::LabelText("##SelectedScript", "Selected: %s", selectedFileName.c_str());

            ImGui::Separator();

            ImGui::Text("Settings");
            ImGui::Indent();

            m_imageComparisonOptions.DrawImGuiSettings();
            if (ImGui::Button("Reset"))
            {
                m_imageComparisonOptions.ResetImGuiSettings();
            }

            ImGui::Unindent();

            ImGui::Separator();

            if (ImGui::Button("Run"))
            {
                auto scriptAsset = m_scriptBrowser.GetSelectedAsset<AZ::ScriptAsset>();
                if (scriptAsset.GetId().IsValid())
                {
                    PrepareAndExecuteScript(m_scriptBrowser.GetSelectedAssetPath());
                }
            }

            if (ImGui::Button("View Latest Results"))
            {
                m_scriptReporter.OpenReportDialog();
            }

            if (m_scriptOperations.size() > 0)
            {
                ImGui::LabelText("##RunningScript", "Running %zu operations...", m_scriptOperations.size());

                drawAbortButton("Button2");
            }
        }

        ImGui::End();
    }

    void ScriptAutomationSystemComponent::RenderImGui()
    {
        if (m_showScriptRunnerDialog)
        {
            RenderScriptRunnerDialog();
        }

        m_scriptReporter.TickImGui();
    }
} // namespace ScriptAutomation
