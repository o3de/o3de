/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/IMultiplayerTools.h>
#include <Multiplayer/INetworkSpawnableLibrary.h>
#include <Multiplayer/MultiplayerConstants.h>

#include <MultiplayerSystemComponent.h>
#include <PythonEditorEventsBus.h>
#include <Editor/MultiplayerEditorSystemComponent.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>


namespace Multiplayer
{
    using namespace AzNetworking;

    AZ_CVAR(bool, editorsv_enabled, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Whether Editor launching a local server to connect to is supported");
    AZ_CVAR(bool, editorsv_launch, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Whether Editor should launch a server when the server address is localhost");
    AZ_CVAR(AZ::CVarFixedString, editorsv_process, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The server executable that should be run. Empty to use the current project's ServerLauncher");
    AZ_CVAR(bool, editorsv_hidden, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The server executable launches hidden without a window. Best used with editorsv_rhi_override set to null.");
    AZ_CVAR(AZ::CVarFixedString, editorsv_serveraddr, AZ::CVarFixedString(LocalHost), nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The address of the server to connect to");
    AZ_CVAR(AZ::CVarFixedString, editorsv_rhi_override, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Override the default rendering hardware interface (rhi) when launching the Editor server. For example, you may be running an Editor using 'dx12', but want to launch a headless server using 'null'. If empty the server will launch using the same rhi as the Editor.");
    AZ_CVAR_EXTERNED(uint16_t, editorsv_port);
    
    //////////////////////////////////////////////////////////////////////////
    void PyEnterGameMode()
    {
        editorsv_enabled = true;
        editorsv_launch = true;
        AzToolsFramework::EditorLayerPythonRequestBus::Broadcast(&AzToolsFramework::EditorLayerPythonRequestBus::Events::EnterGameMode);
    }

    bool PyIsInGameMode()
    {
        // If the network entity manager is tracking at least 1 entity then the editor has connected and the autonomous player exists and is being replicated.
        if (const INetworkEntityManager* networkEntityManager = AZ::Interface<INetworkEntityManager>::Get())
        {
            return networkEntityManager->GetEntityCount() > 0;
        }

        AZ_Warning("MultiplayerEditorSystemComponent", false, "PyIsInGameMode returning false; NetworkEntityManager has not been created yet.")
        return false;
    }

    void PythonEditorFuncs::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PythonEditorFuncs, AZ::Component>()
                ->Version(0);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // This will create static python methods in the 'azlmbr.multiplayer' module
            // Note: The methods will be prefixed with the class name, PythonEditorFuncs
            // Example Hydra Python: azlmbr.multiplayer.PythonEditorFuncs_enter_game_mode()
            behaviorContext->Class<PythonEditorFuncs>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "multiplayer")
                ->Method("enter_game_mode", PyEnterGameMode, nullptr, "Enters the editor game mode and launches/connects to the server launcher.")
                ->Method("is_in_game_mode", PyIsInGameMode, nullptr, "Queries if it's in the game mode and the server has finished connecting and the default network player has spawned.")
            ;

        }
    }
    
    void MultiplayerEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerEditorSystemComponent, AZ::Component>()
                ->Version(1);
        }

        // Reflect Python Editor Functions
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // This will add the MultiplayerPythonEditorBus into the 'azlmbr.multiplayer' module
            behaviorContext->EBus<MultiplayerEditorLayerPythonRequestBus>("MultiplayerPythonEditorBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "multiplayer")
                ->Event("EnterGameMode", &MultiplayerEditorLayerPythonRequestBus::Events::EnterGameMode)
                ->Event("IsInGameMode", &MultiplayerEditorLayerPythonRequestBus::Events::IsInGameMode)
            ;
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
        : m_serverAcceptanceReceivedHandler([this](){OnServerAcceptanceReceived();})
    {
        ;
    }

    void MultiplayerEditorSystemComponent::Activate()
    {
        AzFramework::GameEntityContextEventBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        MultiplayerEditorServerRequestBus::Handler::BusConnect();
        AZ::Interface<IMultiplayer>::Get()->AddServerAcceptanceReceivedHandler(m_serverAcceptanceReceivedHandler);
    }

    void MultiplayerEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzFramework::GameEntityContextEventBus::Handler::BusDisconnect();
        MultiplayerEditorServerRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
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
            // Kill the configured server if it's active
            AZ::TickBus::Handler::BusDisconnect();
            if (m_serverProcessWatcher)
            {
                m_serverProcessWatcher->TerminateProcess(0);
                if (m_serverProcessTracePrinter)
                {
                    m_serverProcessTracePrinter->Pump();
                    m_serverProcessTracePrinter->WriteCurrentString(true);
                    m_serverProcessTracePrinter->WriteCurrentString(false);
                }
                m_serverProcessWatcher = nullptr;
                m_serverProcessTracePrinter = nullptr;
            }
            
            if (INetworkInterface* editorNetworkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpEditorInterfaceName)))
            {
                editorNetworkInterface->Disconnect(m_editorConnId, AzNetworking::DisconnectReason::TerminatedByClient);
            }
            if (auto console = AZ::Interface<AZ::IConsole>::Get(); console)
            {
                console->PerformCommand("disconnect");
            }

            if (m_connectionEvent.IsScheduled())
            {
                m_connectionEvent.RemoveFromQueue();
            }

            AZ::Interface<INetworkEntityManager>::Get()->ClearAllEntities();

            // Rebuild the library to clear temporary in-memory spawnable assets
            AZ::Interface<INetworkSpawnableLibrary>::Get()->BuildSpawnablesList();

            break;
        }
    }

    bool MultiplayerEditorSystemComponent::FindServerLauncher(AZ::IO::FixedMaxPath& serverPath)
    {
        serverPath.clear();

        // 1. Try the path from `editorsv_process` cvar.
        const AZ::IO::FixedMaxPath serverPathFromCvar{ AZ::CVarFixedString(editorsv_process).c_str()};
        if (AZ::IO::SystemFile::Exists(serverPathFromCvar.c_str()))
        {
            serverPath = serverPathFromCvar;
            return true;
        }

        // 2. Try from the executable folder where the Editor was launched from.
        AZ::IO::FixedMaxPath serverPathFromEditorLocation = AZ::Utils::GetExecutableDirectory();
        serverPathFromEditorLocation /= AZStd::string_view(AZ::Utils::GetProjectName() + ".ServerLauncher" + AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
        if (AZ::IO::SystemFile::Exists(serverPathFromEditorLocation.c_str()))
        {
            serverPath = serverPathFromEditorLocation;
            return true;
        }

        // 3. Try from the project's build folder.
        AZ::IO::FixedMaxPath serverPathFromProjectBin;
        if (const auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            if (AZ::IO::FixedMaxPath projectModulePath;
                settingsRegistry->Get(projectModulePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectConfigurationBinPath))
            {
                serverPathFromProjectBin /= projectModulePath;
                serverPathFromProjectBin /= AZStd::string_view(AZ::Utils::GetProjectName() + ".ServerLauncher" + AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
                if (AZ::IO::SystemFile::Exists(serverPathFromProjectBin.c_str()))
                {
                    serverPath = serverPathFromProjectBin;
                    return true;
                }
            }
        }

        AZ_Error(
            "MultiplayerEditor", false,
            "The ServerLauncher binary is missing! (%s), (%s) and (%s) were tried. Please build server launcher or specify using editorsv_process.",
            serverPathFromCvar.c_str(),
            serverPathFromEditorLocation.c_str(),
            serverPathFromProjectBin.c_str());

        return false;
    }

    void MultiplayerEditorSystemComponent::LaunchEditorServer()
    {
        // Assemble the server's path
        AZ::IO::FixedMaxPath serverPath;
        if (!FindServerLauncher(serverPath))
        {
            return;
        }        

        // Start the configured server if it's available
        AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        // Open the server launcher using the same rhi as the editor (or launch with the override rhi)
        AZ::Name server_rhi = AZ::RPI::RPISystemInterface::Get()->GetRenderApiName();
        if (!static_cast<AZ::CVarFixedString>(editorsv_rhi_override).empty())
        {
            server_rhi = static_cast<AZ::CVarFixedString>(editorsv_rhi_override);
        }

        processLaunchInfo.m_commandlineParameters = AZStd::string::format(
            R"("%s" --project-path "%s" --editorsv_isDedicated true --bg_ConnectToAssetProcessor false --rhi "%s" --editorsv_port %i)",
            serverPath.c_str(),
            AZ::Utils::GetProjectPath().c_str(),
            server_rhi.GetCStr(),
            static_cast<uint16_t>(editorsv_port)
        );
        processLaunchInfo.m_showWindow = !editorsv_hidden;
        processLaunchInfo.m_processPriority = AzFramework::ProcessPriority::PROCESSPRIORITY_NORMAL;

        // Launch the Server
        AzFramework::ProcessWatcher* outProcess = AzFramework::ProcessWatcher::LaunchProcess(
            processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT);

        if (outProcess)
        {
            // Stop the previous server if one exists
            if (m_serverProcessWatcher)
            {
                AZ::TickBus::Handler::BusDisconnect();
                m_serverProcessWatcher->TerminateProcess(0);
            }
            m_serverProcessWatcher.reset(outProcess);
            m_serverProcessTracePrinter = AZStd::make_unique<ProcessCommunicatorTracePrinter>(m_serverProcessWatcher->GetCommunicator(), "EditorServer");
            AZ::TickBus::Handler::BusConnect();
        }
        else
        {
            AZ_Error("MultiplayerEditor", outProcess, "LaunchEditorServer failed! Unable to create AzFramework::ProcessWatcher.");
        }
    }

    void MultiplayerEditorSystemComponent::OnGameEntitiesStarted()
    {
        IMultiplayerTools* mpTools = AZ::Interface<IMultiplayerTools>::Get();
        if (!editorsv_enabled || !mpTools)
        {
            // Early out if Editor server is not enabled.
            // This allows to avoid printing an error about missing PrefabEditorEntityOwnershipInterface for non-prefab levels.
            return;
        }

        auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        if (!prefabEditorEntityOwnershipInterface)
        {
            bool prefabSystemEnabled = false;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(prefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);
            AZ_Error("MultiplayerEditor", !prefabSystemEnabled, "PrefabEditorEntityOwnershipInterface unavailable but prefabs are enabled");
            return;
        }

        // BeginGameMode and Prefab Processing have completed at this point
        const auto& allAssetData = prefabEditorEntityOwnershipInterface->GetPlayInEditorAssetData();
        
        AZStd::vector<uint8_t> buffer;
        AZ::IO::ByteContainerStream byteStream(&buffer);

        // Serialize Asset information and AssetData into a potentially large buffer
        for (auto& [spawnableName, spawnableAssetData] : allAssetData)
        {
            for (auto& asset : spawnableAssetData.m_assets)
            {
                AZ::Data::AssetId assetId = asset.GetId();
                AZStd::string assetHint = asset.GetHint();
                uint32_t hintSize = aznumeric_cast<uint32_t>(assetHint.size());

                byteStream.Write(sizeof(AZ::Data::AssetId), reinterpret_cast<void*>(&assetId));
                byteStream.Write(sizeof(uint32_t), reinterpret_cast<void*>(&hintSize));
                byteStream.Write(assetHint.size(), assetHint.data());
                AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, asset.GetData(), asset.GetData()->GetType());
            }
        }

        if (editorsv_launch)
        {
            const AZ::CVarFixedString remoteAddress = editorsv_serveraddr;
            if (LocalHost != remoteAddress)
            {
                AZ_Warning(
                    "MultiplayerEditor", false,
                    "Launching editor server skipped because of incompatible settings. "
                    "When using editorsv_launch=true editorsv_serveraddr must be set to local address (127.0.0.1) instead %s",
                    remoteAddress.c_str())
                return;
            }

            AZ_Printf("MultiplayerEditor", "Editor is listening for the editor-server...")

            // Launch the editor-server
            LaunchEditorServer();
        }
        
        // Keep trying to connect until the port is finally available.
        m_connectionAttempts = 0;
        constexpr float retrySeconds = 1.0f;
        constexpr bool autoRequeue = true;
        m_connectionEvent.Enqueue(AZ::SecondsToTimeMs(retrySeconds), autoRequeue);
    }

    void MultiplayerEditorSystemComponent::OnGameEntitiesReset()
    {
    }

    void MultiplayerEditorSystemComponent::OnServerAcceptanceReceived()
    {
        // We're now accepting the connection to the EditorServer.
        // In normal game clients SendReadyForEntityUpdates will be enabled once the appropriate level's root spawnable is loaded,
        // but since we're in Editor, we're already in the level.
        AZ::Interface<IMultiplayer>::Get()->SendReadyForEntityUpdates(true);
    }

    void MultiplayerEditorSystemComponent::SendEditorServerLevelDataPacket(AzNetworking::IConnection* connection)
    {
        const auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        if (!prefabEditorEntityOwnershipInterface)
        {
            bool prefabSystemEnabled = false;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(prefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);
            AZ_Error("MultiplayerEditor", !prefabSystemEnabled, "PrefabEditorEntityOwnershipInterface unavailable but prefabs are enabled");
            return;
        }

        AZ_Printf("MultiplayerEditor", "Editor is sending the editor-server the level data packet.")

        const auto& allAssetData = prefabEditorEntityOwnershipInterface->GetPlayInEditorAssetData();

        AZStd::vector<uint8_t> buffer;
        AZ::IO::ByteContainerStream byteStream(&buffer);

        // Serialize Asset information and AssetData into a potentially large buffer
        for (auto& [spawnableName, spawnableAssetData] : allAssetData)
        {
            for (auto& asset : spawnableAssetData.m_assets)
            {
                AZ::Data::AssetId assetId = asset.GetId();
                AZStd::string assetHint = asset.GetHint();
                auto hintSize = aznumeric_cast<uint32_t>(assetHint.size());

                byteStream.Write(sizeof(AZ::Data::AssetId), reinterpret_cast<void*>(&assetId));
                byteStream.Write(sizeof(uint32_t), reinterpret_cast<void*>(&hintSize));
                byteStream.Write(assetHint.size(), assetHint.data());
                AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, asset.GetData(), asset.GetData()->GetType());
            }
        }
        
        // Spawnable library needs to be rebuilt since now we have newly registered in-memory spawnable assets
        AZ::Interface<INetworkSpawnableLibrary>::Get()->BuildSpawnablesList();

        // Read the buffer into EditorServerLevelData packets until we've flushed the whole thing
        byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

        while (byteStream.GetCurPos() < byteStream.GetLength())
        {
            MultiplayerEditorPackets::EditorServerLevelData editorServerLevelDataPacket;
            auto& outBuffer = editorServerLevelDataPacket.ModifyAssetData();

            // Size the packet's buffer appropriately
            size_t readSize = outBuffer.GetCapacity();
            const size_t byteStreamSize = byteStream.GetLength() - byteStream.GetCurPos();
            if (byteStreamSize < readSize)
            {
                readSize = byteStreamSize;
            }

            outBuffer.Resize(readSize);
            byteStream.Read(readSize, outBuffer.GetBuffer());

            // If we've run out of buffer, mark that we're done
            if (byteStream.GetCurPos() == byteStream.GetLength())
            {
                editorServerLevelDataPacket.SetLastUpdate(true);
            }

            connection->SendReliablePacket(editorServerLevelDataPacket);
        }
    }

    void MultiplayerEditorSystemComponent::EnterGameMode()
    {
        PyEnterGameMode();
    }
    
    bool MultiplayerEditorSystemComponent::IsInGameMode()
    {
        return PyIsInGameMode();
    }

    void MultiplayerEditorSystemComponent::OnTick(float, AZ::ScriptTimePoint)
    {
        if (m_serverProcessTracePrinter)
        {
            m_serverProcessTracePrinter->Pump();
        }
        else
        {
            AZ::TickBus::Handler::BusDisconnect();
            AZ_Warning(
                "MultiplayerEditorSystemComponent", false,
                "The server process trace printer is NULL so we won't be able to pipe server logs to the editor. Please update the code to call AZ::TickBus::Handler::BusDisconnect whenever the editor-server is terminated.")
        }
    }

    void MultiplayerEditorSystemComponent::Connect()
    {
        ++m_connectionAttempts;
        AZ_Printf("MultiplayerEditor", "Editor tcp connection attempt #%i.", m_connectionAttempts)

        INetworkInterface* editorNetworkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpEditorInterfaceName));
        AZ_Assert(editorNetworkInterface, "MP Editor Network Interface was unregistered before Editor could connect.")

        const AZ::CVarFixedString remoteAddress = editorsv_serveraddr;
        m_editorConnId = editorNetworkInterface->Connect(AzNetworking::IpAddress(remoteAddress.c_str(), editorsv_port, AzNetworking::ProtocolType::Tcp));

        if (m_editorConnId != AzNetworking::InvalidConnectionId)
        {
            AZ_Printf("MultiplayerEditor", "Editor has connected to the editor-server.")
            m_connectionEvent.RemoveFromQueue();
            SendEditorServerLevelDataPacket(editorNetworkInterface->GetConnectionSet().GetConnection(m_editorConnId));
        }
    }

}
