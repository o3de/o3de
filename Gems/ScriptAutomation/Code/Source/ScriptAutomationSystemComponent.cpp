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
                m_isStarted = false;
                m_automationScript = commandLine->GetSwitchValue(automationSuiteSwitch, 0);
                m_exitOnFinish = commandLine->HasSwitch(automationExitSwitch);

                AZ::TickBus::Handler::BusConnect();
            }
        }
    }

    void ScriptAutomationSystemComponent::Deactivate()
    {
        m_scriptContext = nullptr;
        m_scriptBehaviorContext = nullptr;

        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }

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

            // Execute the next operation
            m_scriptOperations.front()();

            m_scriptOperations.pop();

            if (m_scriptOperations.empty())
            {
                ScriptAutomationNotificationBus::Broadcast(&ScriptAutomationNotificationBus::Events::OnAutomationFinished);

                if (m_exitOnFinish)
                {
                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
                }
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

    void ScriptAutomationSystemComponent::ExecuteScript(const char* scriptFilePath)
    {
        AZ::Data::Asset<AZ::ScriptAsset> scriptAsset = LoadScriptAssetFromPath(scriptFilePath, *m_scriptContext.get());
        if (!scriptAsset)
        {
            // Push an error operation on the back of the queue instead of reporting it immediately so it doesn't get lost
            // in front of a bunch of queued m_scriptOperations.
            QueueScriptOperation([scriptFilePath]()
                {
                    AZ_Error("ScriptAutomation", false, "Script: Could not find or load script asset '%s'.", scriptFilePath);
                }
            );
            return;
        }

        QueueScriptOperation([scriptFilePath]()
            {
                AZ_Printf("ScriptAutomation", "Running script '%s'...\n", scriptFilePath);
            }
        );

        if (!m_scriptContext->Execute(scriptAsset->m_data.GetScriptBuffer().data(), scriptFilePath, scriptAsset->m_data.GetScriptBuffer().size()))
        {
            // Push an error operation on the back of the queue instead of reporting it immediately so it doesn't get lost
            // in front of a bunch of queued m_scriptOperations.
            QueueScriptOperation([scriptFilePath]()
                {
                    AZ_Error("ScriptAutomation", false, "Script: Error running script '%s'.", scriptFilePath);
                }
            );
        }
    }
} // namespace ScriptAutomation
