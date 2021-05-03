/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <Include/IMultiplayerTools.h>
#include <Source/Editor/MultiplayerEditorSystemComponent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>

namespace Multiplayer
{
    using namespace AzNetworking;

    AZ_CVAR(bool, editorsv_enabled, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Whether Editor launching a local server to connect to is supported");
    AZ_CVAR(AZ::CVarFixedString, editorsv_process, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The server executable that should be run. Empty to use the current project's ServerLauncher");

    void MultiplayerEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerEditorSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void MultiplayerEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("MultiplayerService"));
    }

    void MultiplayerEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerEditorService"));
    }

    void MultiplayerEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MultiplayerEditorService"));
    }

    MultiplayerEditorSystemComponent::MultiplayerEditorSystemComponent()
    {
        ;
    }

    void MultiplayerEditorSystemComponent::Activate()
    {
        AzFramework::GameEntityContextEventBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void MultiplayerEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzFramework::GameEntityContextEventBus::Handler::BusDisconnect();
    }

    void MultiplayerEditorSystemComponent::NotifyRegisterViews()
    {
        AZ_Assert(m_editor == nullptr, "NotifyRegisterViews occurred twice!");
        m_editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(m_editor, &AzToolsFramework::EditorRequests::GetEditor);
        m_editor->RegisterNotifyListener(this);
    }

    void MultiplayerEditorSystemComponent::OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        switch (event)
        {
        case eNotify_OnQuit:
            AZ_Warning("Multiplayer Editor", m_editor != nullptr, "Multiplayer Editor received On Quit without an Editor pointer.");
            if (m_editor)
            {
                m_editor->UnregisterNotifyListener(this);
                m_editor = nullptr;
            }
            [[fallthrough]];
        case eNotify_OnEndGameMode:
            AZ::TickBus::Handler::BusDisconnect();
            // Kill the configured server if it's active
            if (m_serverProcess)
            {
                m_serverProcess->TerminateProcess(0);
                m_serverProcess = nullptr;
            }
            break;
        }
    }

    void MultiplayerEditorSystemComponent::OnGameEntitiesStarted()
    {
        // BeginGameMode and Prefab Processing have completed at this point
        IMultiplayerTools* mpTools = AZ::Interface<IMultiplayerTools>::Get();
        if (editorsv_enabled && mpTools != nullptr && mpTools->DidProcessNetworkPrefabs())
        {
            AZ::TickBus::Handler::BusConnect();

            auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
            if (!prefabEditorEntityOwnershipInterface)
            {
                AZ_Error("MultiplayerEditor", prefabEditorEntityOwnershipInterface != nullptr, "PrefabEditorEntityOwnershipInterface unavailable");
            }
            const AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& assetData = prefabEditorEntityOwnershipInterface->GetPlayInEditorAssetData();

            if (assetData.size() > 0)
            {
                // Assemble the server's path
                AZ::CVarFixedString serverProcess = editorsv_process;
                if (serverProcess.empty())
                {
                    // If enabled but no process name is supplied, try this project's ServerLauncher
                    serverProcess = AZ::Utils::GetProjectName() + ".ServerLauncher";
                }

                AZ::IO::FixedMaxPathString serverPath = AZ::Utils::GetExecutableDirectory();
                if (!serverProcess.contains(AZ_TRAIT_OS_PATH_SEPARATOR))
                {
                    // If only the process name is specified, append that as well
                    serverPath.append(AZ_TRAIT_OS_PATH_SEPARATOR + serverProcess);
                }
                else
                {
                    // If any path was already specified, then simply assign
                    serverPath = serverProcess;
                }

                if (!serverProcess.ends_with(AZ_TRAIT_OS_EXECUTABLE_EXTENSION))
                {
                    // Add this platform's exe extension if it's not specified
                    serverPath.append(AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
                }

                // Start the configured server if it's available
                AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
                processLaunchInfo.m_commandlineParameters = AZStd::string::format("\"%s\"", serverPath.c_str());
                processLaunchInfo.m_showWindow = true;
                processLaunchInfo.m_processPriority = AzFramework::ProcessPriority::PROCESSPRIORITY_NORMAL;

                m_serverProcess = AzFramework::ProcessWatcher::LaunchProcess(
                    processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE);
            }
        }
    }

    void MultiplayerEditorSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {

    }

    int MultiplayerEditorSystemComponent::GetTickOrder()
    {
        // Tick immediately after the network system component
        return AZ::TICK_PLACEMENT + 1;
    }
}
