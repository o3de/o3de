/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AutomationSystemComponent.h>

#include <AutomationScriptBindings.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemBus.h>

namespace Automation
{
    namespace
    {
        AZ::Data::Asset<AZ::ScriptAsset> LoadScriptAssetFromPath(const char* productPath)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId,
                &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                productPath,
                AZ::Data::s_invalidAssetType,
                false
            );

            if (assetId.IsValid())
            {
                AZ::Data::Asset<AZ::ScriptAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::ScriptAsset>(
                    assetId, AZ::Data::AssetLoadBehavior::PreLoad
                );
                asset.BlockUntilLoadComplete();

                if (!asset.IsReady())
                {
                    AZ_Assert(false, "Could not load '%s'", productPath);
                    return {};
                }

                return asset;
            }
            else
            {
                AZ_Assert(false, "Unable to find product asset '%s'. Has the source asset finished building?", productPath);
                return {};
            }
        }
    } // namespace

    void AutomationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AutomationSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AutomationSystemComponent>("Automation", "Provides a mechanism for automating various tasks through Lua scripting in the game launchers")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void AutomationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AutomationServiceCrc);
    }

    void AutomationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AutomationServiceCrc);
    }

    void AutomationSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void AutomationSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    AutomationSystemComponent::AutomationSystemComponent()
    {
        if (AutomationInterface::Get() == nullptr)
        {
            AutomationInterface::Register(this);
        }
    }

    AutomationSystemComponent::~AutomationSystemComponent()
    {
        if (AutomationInterface::Get() == this)
        {
            AutomationInterface::Unregister(this);
        }
    }

    void AutomationSystemComponent::SetIdleFrames(int numFrames)
    {
        AZ_Assert(m_scriptIdleSeconds == 0, "m_scriptIdleFrames is being stomped");
        m_scriptIdleFrames = numFrames;
    }

    void AutomationSystemComponent::SetIdleSeconds(float numSeconds)
    {
        m_scriptIdleSeconds = numSeconds;
    }

    void AutomationSystemComponent::Activate()
    {
        AutomationRequestBus::Handler::BusConnect();

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

    void AutomationSystemComponent::Deactivate()
    {
        m_scriptContext = nullptr;
        m_scriptBehaviorContext = nullptr;

        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }

        AutomationRequestBus::Handler::BusDisconnect();
    }

    void AutomationSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_isStarted)
        {
            m_isStarted = true;
            ExecuteScript(m_automationScript);

            AutomationNotificationBus::Broadcast(&AutomationNotificationBus::Events::OnAutomationStarted);
        }

        while (!m_scriptOperations.empty())
        {
            if (m_scriptPaused)
            {
                m_scriptPauseTimeout -= deltaTime;
                if (m_scriptPauseTimeout < 0)
                {
                    AZ_Error("Automation", false, "Script pause timed out. Continuing...");
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
                AutomationNotificationBus::Broadcast(&AutomationNotificationBus::Events::OnAutomationFinished);

                if (m_exitOnFinish)
                {
                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
                }
            }
        }
    }

    AZ::BehaviorContext* AutomationSystemComponent::GetAutomationContext()
    {
        return m_scriptBehaviorContext.get();
    }

    void AutomationSystemComponent::PauseAutomation(float timeout)
    {
        m_scriptPaused = true;
        m_scriptPauseTimeout = AZ::GetMax(timeout, m_scriptPauseTimeout);
    }

    void AutomationSystemComponent::ResumeAutomation()
    {
        AZ_Warning("Automation", m_scriptPaused, "Script is not paused");
        m_scriptPaused = false;
    }

    void AutomationSystemComponent::QueueScriptOperation(AutomationRequests::ScriptOperation&& operation)
    {
        m_scriptOperations.push(AZStd::move(operation));
    }

    void AutomationSystemComponent::ExecuteScript(const AZStd::string& scriptFilePath)
    {
        AZ::Data::Asset<AZ::ScriptAsset> scriptAsset = LoadScriptAssetFromPath(scriptFilePath.c_str());
        if (!scriptAsset)
        {
            // Push an error operation on the back of the queue instead of reporting it immediately so it doesn't get lost
            // in front of a bunch of queued m_scriptOperations.
            QueueScriptOperation([scriptFilePath]()
                {
                    AZ_Error("Automation", false, "Script: Could not find or load script asset '%s'.", scriptFilePath.c_str());
                }
            );
            return;
        }

        QueueScriptOperation([scriptFilePath]()
            {
                AZ_Printf("Automation", "Running script '%s'...\n", scriptFilePath.c_str());
            }
        );

        if (!m_scriptContext->Execute(scriptAsset->GetScriptBuffer().data(), scriptFilePath.c_str(), scriptAsset->GetScriptBuffer().size()))
        {
            // Push an error operation on the back of the queue instead of reporting it immediately so it doesn't get lost
            // in front of a bunch of queued m_scriptOperations.
            QueueScriptOperation([scriptFilePath]()
                {
                    AZ_Error("Automation", false, "Script: Error running script '%s'.", scriptFilePath.c_str());
                }
            );
        }
    }
} // namespace Automation
