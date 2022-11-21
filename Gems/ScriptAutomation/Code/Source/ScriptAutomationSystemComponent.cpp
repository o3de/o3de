/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptAutomationSystemComponent.h>

#include <ScriptAutomationScriptBindings.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
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
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
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

    void ScriptAutomationSystemComponent::StartProfilingCapture()
    {
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusConnect();
    }

    void ScriptAutomationSystemComponent::Activate()
    {
        ScriptAutomationRequestBus::Handler::BusConnect();

        m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
        m_scriptBehaviorContext = AZStd::make_unique<AZ::BehaviorContext>();

        ReflectScriptBindings(m_scriptBehaviorContext.get());
        m_scriptContext->BindTo(m_scriptBehaviorContext.get());

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
        m_scriptContext = nullptr;
        m_scriptBehaviorContext = nullptr;

        DeactivateScripts();

        ScriptAutomationRequestBus::Handler::BusDisconnect();
    }

    void ScriptAutomationSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_isStarted)
        {
            m_isStarted = true;
            ExecuteScript(m_automationScript.c_str());

            ScriptAutomationNotificationBus::Broadcast(&ScriptAutomationNotificationBus::Events::OnAutomationStarted);
        }

        while (true)
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
        AZ_Warning("ScriptAutomation", m_scriptPaused, "Script is not paused");
        m_scriptPaused = false;
    }

    void ScriptAutomationSystemComponent::QueueScriptOperation(ScriptAutomationRequests::ScriptOperation&& operation)
    {
        m_scriptOperations.push(AZStd::move(operation));
    }

    void ScriptAutomationSystemComponent::ExecuteScript(const char* scriptFilePath [[maybe_unused]])
    {
        AZ::Data::Asset<AZ::ScriptAsset> scriptAsset = LoadScriptAssetFromPath(scriptFilePath, *m_scriptContext.get());
        if (!scriptAsset)
        {
#ifndef _RELEASE // AZ_Error is a no-op in release builds
            // Push an error operation on the back of the queue instead of reporting it immediately so it doesn't get lost
            // in front of a bunch of queued m_scriptOperations.
            QueueScriptOperation([scriptFilePath]()
                {
                    AZ_Error("ScriptAutomation", false, "Script: Could not find or load script asset '%s'.", scriptFilePath);
                }
            );
#endif
            return;
        }

#ifndef _RELEASE // AZ_Error is a no-op in release builds
        QueueScriptOperation([scriptFilePath]()
            {
                AZ_Printf("ScriptAutomation", "Running script '%s'...\n", scriptFilePath);
            }
        );
#endif

        if (!m_scriptContext->Execute(scriptAsset->m_data.GetScriptBuffer().data(), scriptFilePath, scriptAsset->m_data.GetScriptBuffer().size()))
        {
#ifndef _RELEASE // AZ_Error is a no-op in release builds
            // Push an error operation on the back of the queue instead of reporting it immediately so it doesn't get lost
            // in front of a bunch of queued m_scriptOperations.
            QueueScriptOperation([scriptFilePath]()
                {
                    AZ_Error("ScriptAutomation", false, "Script: Error running script '%s'.", scriptFilePath);
                }
            );
#endif
        }
    }

    void ScriptAutomationSystemComponent::OnCaptureQueryTimestampFinished([[maybe_unused]] bool result, [[maybe_unused]] const AZStd::string& info)
    {
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();
    }

    void ScriptAutomationSystemComponent::OnCaptureCpuFrameTimeFinished([[maybe_unused]] bool result, [[maybe_unused]] const AZStd::string& info)
    {
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();
    }

    void ScriptAutomationSystemComponent::OnCaptureQueryPipelineStatisticsFinished([[maybe_unused]] bool result, [[maybe_unused]] const AZStd::string& info)
    {
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();
    }

    void ScriptAutomationSystemComponent::OnCaptureBenchmarkMetadataFinished([[maybe_unused]] bool result, [[maybe_unused]] const AZStd::string& info)
    {
        AZ::Render::ProfilingCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();
    }

    void ScriptAutomationSystemComponent::OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string &info)
    {
        m_scriptFrameCaptureId = AZ::Render::InvalidFrameCaptureId;
        AZ::Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
        ResumeAutomation();

        // This is checking for the exact scenario that results from an HDR setup. The goal is to add a very specific and prominent message that will
        // alert users to a common issue and what action to take. Any other Format issues will be reported by FrameCaptureSystemComponent with a
        // "Can't save image with format %s to a ppm file" message.
        if (result == AZ::Render::FrameCaptureResult::UnsupportedFormat && info.find(AZ::RHI::ToString(AZ::RHI::Format::R10G10B10A2_UNORM)) != AZStd::string::npos)
        {
            AZ_Assert(false, "ScriptAutomation Screen Capture - HDR Not Supported, Screen capture to image is not supported from RGB10A2 display format. Please change the system configuration to disable the HDR display feature.");
        }
    }

} // namespace ScriptAutomation
