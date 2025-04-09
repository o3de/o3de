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
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>

#include <MultiplayerSystemComponent.h>
#include <PythonEditorEventsBus.h>
#include <Editor/MultiplayerEditorAutomation.h>
#include <Editor/MultiplayerEditorSystemComponent.h>
#include <Editor/EditorViewportSettings.h>
#include <Editor/Viewport.h>
#include <Editor/ViewManager.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Process/ProcessUtils.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorActionUpdaterIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <QMenu>
#include <QAction>

namespace Multiplayer
{
    using namespace AzNetworking;

    AZ_CVAR(bool, editorsv_enabled, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Whether Editor launching a local server to connect to is supported");
    AZ_CVAR(bool, editorsv_clientserver, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "If true, the editor will act as both the server and a client. No dedicated server will be launched.");
    AZ_CVAR(bool, editorsv_launch, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Whether Editor should launch a server when the server address is localhost");
    AZ_CVAR(AZ::CVarFixedString, editorsv_process, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The server executable that should be run. Empty to use the current project's ServerLauncher");
    AZ_CVAR(bool, editorsv_hidden, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The server executable launches hidden without a window. Best used with editorsv_rhi_override set to null.");
    AZ_CVAR(AZ::CVarFixedString, editorsv_serveraddr, AZ::CVarFixedString(LocalHost), nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The address of the server to connect to");
    AZ_CVAR(AZ::CVarFixedString, editorsv_rhi_override, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Override the default rendering hardware interface (rhi) when launching the Editor server. For example, you may be running an Editor using 'dx12', but want to launch a headless server using 'null'. If empty the server will launch using the same rhi as the Editor.");
    AZ_CVAR(uint16_t, editorsv_max_connection_attempts, 5, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The maximum times the editor will attempt to connect to the server. Time between attempts is increased based on the number of failed attempts.");

    AZ_CVAR(bool, editorsv_print_server_logs, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Whether Editor should print its server's logs to the Editor console. Useful for seeing server prints, warnings, and errors without having to open up the server console or server.log file. Note: Must be set before entering the editor play mode.");

    AZ_CVAR_EXTERNED(uint16_t, editorsv_port);
    AZ_CVAR_EXTERNED(bool, bg_enableNetworkingMetrics);

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
        Automation::MultiplayerEditorAutomationHandler::Reflect(context);

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
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        MultiplayerEditorServerRequestBus::Handler::BusConnect();
        AZ::Interface<IMultiplayer>::Get()->AddServerAcceptanceReceivedHandler(m_serverAcceptanceReceivedHandler);
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void MultiplayerEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        MultiplayerEditorServerRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AzToolsFramework::Prefab::PrefabToInMemorySpawnableNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();

        ResetLevelSendData();
    }

    void MultiplayerEditorSystemComponent::NotifyRegisterViews()
    {
        AZ_Assert(m_editor == nullptr, "NotifyRegisterViews occurred twice!");
        m_editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(m_editor, &AzToolsFramework::EditorRequests::GetEditor);
        if(m_editor)
        {
            m_editor->RegisterNotifyListener(this);
        }
    }

    void MultiplayerEditorSystemComponent::ResetLevelSendData()
    {
        // Clear out the temporary buffer so that it doesn't consume any memory when not in use.
        m_levelSendData = {};
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
            m_connectionEvent.RemoveFromQueue();
            ResetLevelSendData();

            if (m_serverProcessWatcher)
            {
                m_serverProcessWatcher->TerminateProcess(0);

                // The TracePrinter hangs onto a pointer to an object that is owned by
                // the ProcessWatcher.  Make sure to destroy the TracePrinter first, before ProcessWatcher.
                m_serverProcessTracePrinter = nullptr;
                m_serverProcessWatcher = nullptr;
            }

            const AZ::Name editorInterfaceName = AZ::Name(MpEditorInterfaceName);
            if (INetworkInterface* editorNetworkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(editorInterfaceName))
            {
                editorNetworkInterface->Disconnect(m_editorConnId, AzNetworking::DisconnectReason::TerminatedByClient);
            }
            if (const auto console = AZ::Interface<AZ::IConsole>::Get())
            {
                console->PerformCommand("disconnect");
            }

            // SpawnableAssetEventsBus would already be disconnected once OnStartPlayInEditor happens, but it's possible to
            // exit gamemode before the OnStartPlayInEditor is called if the user hits CTRL+G and then ESC really fast.
            AzToolsFramework::Prefab::PrefabToInMemorySpawnableNotificationBus::Handler::BusDisconnect();

            // Rebuild the library to clear temporary in-memory spawnable assets
            AZ::Interface<INetworkSpawnableLibrary>::Get()->BuildSpawnablesList();

            // Delete the spawnables we've stored for the server
            m_preAliasedSpawnablesForServer.clear();

            // Turn off debug messaging: we've exiting playmode and intentionally disconnected from the server.
            MultiplayerEditorServerNotificationBus::Broadcast(&MultiplayerEditorServerNotificationBus::Events::OnPlayModeEnd);
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
            "The ServerLauncher binary is missing! Attempted to find ServerLauncher in the editorsv_process path:\"%s\", relative to "
            "editor:\"%s\" and relative to the current project:\"%s\". Please build ServerLauncher or specify its location using editorsv_process.",
            serverPathFromCvar.c_str(),
            serverPathFromEditorLocation.c_str(),
            serverPathFromProjectBin.c_str());

        return false;
    }

    bool MultiplayerEditorSystemComponent::LaunchEditorServer()
    {
        // Assemble the server's path
        AZ::IO::FixedMaxPath serverPath;
        if (!FindServerLauncher(serverPath))
        {
            return false;
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
            R"("%s" --project-path "%s" --editorsv_isDedicated true --bg_ConnectToAssetProcessor false --rhi "%s" --editorsv_port %i --bg_enableNetworkingMetrics %i --sv_dedicated_host_onstartup false)",
            serverPath.c_str(),
            AZ::Utils::GetProjectPath().c_str(),
            server_rhi.GetCStr(),
            static_cast<uint16_t>(editorsv_port),
            (bg_enableNetworkingMetrics ? 1 : 0)
        );
        processLaunchInfo.m_showWindow = !editorsv_hidden;
        processLaunchInfo.m_processPriority = AzFramework::ProcessPriority::PROCESSPRIORITY_NORMAL;
        processLaunchInfo.m_tetherLifetime = true;

        // Launch the Server
        const AzFramework::ProcessCommunicationType communicationType = editorsv_print_server_logs
            ? AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT
            : AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE;

        AzFramework::ProcessWatcher* outProcess = AzFramework::ProcessWatcher::LaunchProcess(processLaunchInfo, communicationType);

        if (outProcess)
        {
            MultiplayerEditorServerNotificationBus::Broadcast(&MultiplayerEditorServerNotificationBus::Events::OnServerLaunched);

            // Stop the previous server if one exists
            if (m_serverProcessWatcher)
            {
                AZ::TickBus::Handler::BusDisconnect();
                m_serverProcessWatcher->TerminateProcess(0);
            }
            m_serverProcessWatcher.reset(outProcess);

            if (editorsv_print_server_logs)
            {
                // Create a threaded trace printer so that it will keep the output pipes flowing smoothly even while sending the
                // editor data over to the server.
                m_serverProcessTracePrinter = AZStd::make_unique<ProcessCommunicatorTracePrinter>(
                    m_serverProcessWatcher->GetCommunicator(), "EditorServer", ProcessCommunicatorTracePrinter::TraceProcessing::Threaded);
            }

            // Connect to the tick bus to listen for unexpected server process disconnections
            AZ::TickBus::Handler::BusConnect();
        }
        else
        {
            AZ_Error("MultiplayerEditor", outProcess, "LaunchEditorServer failed! Unable to create AzFramework::ProcessWatcher.");
            return false;
        }
        return true;
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
            AZ_Error("MultiplayerEditor", false, "PrefabEditorEntityOwnershipInterface could not find PrefabEditorEntityOwnershipInterface!");
            return;
        }

        AZ_TracePrintf("MultiplayerEditor", "Editor is sending the editor-server the level data packet.")

        m_levelSendData.m_sendConnection = connection;
        m_levelSendData.m_byteStream =
            AZStd::make_unique<AZ::IO::ByteContainerStream<AZStd::vector<uint8_t>>>(&m_levelSendData.m_sendBuffer);

        // Serialize Asset information and AssetData into a potentially large buffer
        for (const auto& preAliasedSpawnableData : m_preAliasedSpawnablesForServer)
        {
            // This is an un-aliased level spawnable (example: Root.spawnable and Root.network.spawnable) which we'll send to the server
            auto hintSize = aznumeric_cast<uint32_t>(preAliasedSpawnableData.assetHint.size());

            m_levelSendData.m_byteStream->Write(sizeof(AZ::Data::AssetId), reinterpret_cast<const void*>(&preAliasedSpawnableData.assetId));
            m_levelSendData.m_byteStream->Write(sizeof(uint32_t), reinterpret_cast<void*>(&hintSize));
            m_levelSendData.m_byteStream->Write(preAliasedSpawnableData.assetHint.size(), preAliasedSpawnableData.assetHint.data());
            AZ::Utils::SaveObjectToStream(
                *m_levelSendData.m_byteStream,
                AZ::DataStream::ST_BINARY,
                preAliasedSpawnableData.spawnable.get(),
                preAliasedSpawnableData.spawnable->GetType());
        }

        // Spawnable library needs to be rebuilt since now we have newly registered in-memory spawnable assets
        AZ::Interface<INetworkSpawnableLibrary>::Get()->BuildSpawnablesList();

        // Read the buffer into EditorServerLevelData packets until we've flushed the whole thing
        m_levelSendData.m_byteStream->Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

        // Send an initial notification showing how much data will be sent.
        MultiplayerEditorServerNotificationBus::Broadcast(
            &MultiplayerEditorServerNotificationBus::Events::OnEditorSendingLevelData,
            0,
            aznumeric_cast<uint32_t>(m_levelSendData.m_byteStream->GetLength()));

        // The actual data will get sent "asynchronously" during the OnTick callback over multiple frames.
    }

    void MultiplayerEditorSystemComponent::SendLevelDataToServer()
    {
        // This controls the maximum time slice to use for sending packets. Lower numbers will make the total send time take longer,
        // but will give the Editor more time to do other work. Larger numbers will make the total send time faster, but will starve
        // the Editor. The current value attempts to balance between the two.
        static constexpr AZ::TimeMs MaxSendTimeMs = AZ::TimeMs{ 5 };

        // These control how many retries and how to space them out for packet send failures.
        static constexpr int MaxRetries = 20;
        static constexpr AZ::TimeMs InitialMsDelayPerRetry = AZ::TimeMs { 10 };
        static constexpr AZ::TimeMs MaxMsDelayPerRetry = AZ::TimeMs { 1000 };

        // If there's no data left to send, exit.
        if (!m_levelSendData.m_byteStream)
        {
            return;
        }

        bool updateFinished = false;
        bool updateSuccessful = true;

        AZ::TimeMs startTime = AZ::GetElapsedTimeMs();

        // Loop and send packets until we've reached our max send time slice for this frame.
        while (!updateFinished && ((AZ::GetElapsedTimeMs() - startTime) < MaxSendTimeMs))
        {
            MultiplayerEditorPackets::EditorServerLevelData editorServerLevelDataPacket;
            auto& outBuffer = editorServerLevelDataPacket.ModifyAssetData();

            // Size the packet's buffer appropriately
            size_t readSize = outBuffer.GetCapacity();
            const size_t byteStreamSize = m_levelSendData.m_byteStream->GetLength() - m_levelSendData.m_byteStream->GetCurPos();
            if (byteStreamSize < readSize)
            {
                readSize = byteStreamSize;
            }

            outBuffer.Resize(readSize);
            m_levelSendData.m_byteStream->Read(readSize, outBuffer.GetBuffer());

            // If we've run out of buffer, mark that we're done
            if (m_levelSendData.m_byteStream->GetCurPos() == m_levelSendData.m_byteStream->GetLength())
            {
                editorServerLevelDataPacket.SetLastUpdate(true);
                updateFinished = true;
            }

            // Try to send the packet to the Editor server. Retry if necessary.
            bool packetSent = false;
            AZ::TimeMs millisecondDelayPerRetry = InitialMsDelayPerRetry;
            int numRetries = 0;
            while (!packetSent && (numRetries < MaxRetries))
            {
                packetSent = m_levelSendData.m_sendConnection->SendReliablePacket(editorServerLevelDataPacket);
                if (!packetSent)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(aznumeric_cast<int>(millisecondDelayPerRetry)));
                    numRetries++;

                    // Keep doubling the time between retries up to the max amount, then clamp it there.
                    millisecondDelayPerRetry = AZStd::min(millisecondDelayPerRetry * AZ::TimeMs{ 2 }, MaxMsDelayPerRetry);

                    // Force the networking buffers to try and flush before sending the packet again.
                    AZ::Interface<AzNetworking::INetworking>::Get()->ForceUpdate();
                }
            }

            if (packetSent)
            {
                // Update our information to track the current amount of data sent.
                MultiplayerEditorServerNotificationBus::Broadcast(
                    &MultiplayerEditorServerNotificationBus::Events::OnEditorSendingLevelData,
                    aznumeric_cast<uint32_t>(m_levelSendData.m_byteStream->GetCurPos()),
                    aznumeric_cast<uint32_t>(m_levelSendData.m_byteStream->GetLength()));
            }
            else
            {
                updateFinished = true;
                updateSuccessful = false;
            }
        }

        if (updateFinished)
        {
            // After we're done sending the level data, clear out our temporary buffer.
            ResetLevelSendData();

            if (updateSuccessful)
            {
                // Notify that the level has successfully been sent.
                MultiplayerEditorServerNotificationBus::Broadcast(
                    &MultiplayerEditorServerNotificationBus::Events::OnEditorSendingLevelDataSuccess);
            }
            else
            {
                // Notify that the level send failed.
                MultiplayerEditorServerNotificationBus::Broadcast(
                    &MultiplayerEditorServerNotificationBus::Events::OnEditorSendingLevelDataFailed);
            }
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
        if (m_serverProcessWatcher && !m_serverProcessWatcher->IsProcessRunning())
        {
            AZ::TickBus::Handler::BusDisconnect();
            MultiplayerEditorServerNotificationBus::Broadcast(&MultiplayerEditorServerNotificationBus::Events::OnEditorServerProcessStoppedUnexpectedly);
            AZ_Warning("MultiplayerEditorSystemComponent", false, "The editor server process has unexpectedly stopped running. Did it crash or get accidentally closed?")
        }

        // Continue sending the level data to the server if any more data exists that needs to be sent.
        SendLevelDataToServer();
    }

    void MultiplayerEditorSystemComponent::Connect()
    {
        ++m_connectionAttempts;
        if (m_connectionAttempts > editorsv_max_connection_attempts)
        {
            m_connectionEvent.RemoveFromQueue();
            MultiplayerEditorServerNotificationBus::Broadcast(&MultiplayerEditorServerNotificationBus::Events::OnEditorConnectionAttemptsFailed, editorsv_max_connection_attempts);
            return;
        }

        MultiplayerEditorServerNotificationBus::Broadcast(&MultiplayerEditorServerNotificationBus::Events::OnEditorConnectionAttempt, m_connectionAttempts, editorsv_max_connection_attempts);
        AZ_TracePrintf("MultiplayerEditor", "Editor TCP connection attempt #%i.", m_connectionAttempts)

        const AZ::Name editorInterfaceName = AZ::Name(MpEditorInterfaceName);
        INetworkInterface* editorNetworkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(editorInterfaceName);
        AZ_Assert(editorNetworkInterface, "MP Editor Network Interface was unregistered before Editor could connect.")

        const AZ::CVarFixedString remoteAddress = editorsv_serveraddr;
        m_editorConnId = editorNetworkInterface->Connect(AzNetworking::IpAddress(remoteAddress.c_str(), editorsv_port, AzNetworking::ProtocolType::Tcp));
        if (m_editorConnId != AzNetworking::InvalidConnectionId)
        {
            AZ_TracePrintf("MultiplayerEditor", "Editor has connected to the editor-server.")
            m_connectionEvent.RemoveFromQueue();
            SendEditorServerLevelDataPacket(editorNetworkInterface->GetConnectionSet().GetConnection(m_editorConnId));
        }
        else
        {
            // Increase the wait time based on the number of connection attempts.
            const double retrySeconds = m_connectionAttempts;
            constexpr bool autoRequeue = false;
            m_connectionEvent.Enqueue(AZ::SecondsToTimeMs(retrySeconds), autoRequeue);
        }
    }

    void MultiplayerEditorSystemComponent::OnPreparingInMemorySpawnableFromPrefab(
        const AzFramework::Spawnable& spawnable, const AZStd::string& assetHint)
    {
        // Only grab the level (Root.spawnable or Root.network.spawnable)
        // We'll receive OnPreparingSpawnable for other spawnables that are referenced by components in the level,
        // but these spawnables are already available for the server inside the asset cache.
        if (!assetHint.starts_with(AzFramework::Spawnable::DefaultMainSpawnableName))
        {
            return;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve application serialization context.")
        AZ_Assert(!assetHint.empty(), "Asset hint is empty!")

        // Store a clone of this spawnable for the server; we make a clone now before the spawnable is modified by aliasing.
        // Aliasing for this editor (client) is different from aliasing that will happen on the server.
        // For example, resolving alias on the client disables auto-spawning of network entities, and will instead wait for a message from the server before updating the net-entities.
        AZStd::unique_ptr<AzFramework::Spawnable> preAliasedSpawnableClone(serializeContext->CloneObject(&spawnable));
        m_preAliasedSpawnablesForServer.push_back({ AZStd::move(preAliasedSpawnableClone), assetHint, spawnable.GetId() });
    }

    void MultiplayerEditorSystemComponent::OnStartPlayInEditorBegin()
    {
        IMultiplayerTools* mpTools = AZ::Interface<IMultiplayerTools>::Get();
        if (!editorsv_enabled || !mpTools)
        {
            // Early out if Editor server is not enabled.
            return;
        }

        if (editorsv_clientserver)
        {
            // Start hosting as a client-server
            const bool isDedicated = false;
            AZ::Interface<IMultiplayer>::Get()->StartHosting(editorsv_port, isDedicated);
            return;
        }

        AZ_Assert(m_preAliasedSpawnablesForServer.empty(), "MultiplayerEditorSystemComponent already has pre-aliased spawnables! Please update code to clean-up the table between entering and existing play mode.")
        AzToolsFramework::Prefab::PrefabToInMemorySpawnableNotificationBus::Handler::BusConnect();
    }

    void MultiplayerEditorSystemComponent::OnStartPlayInEditor()
    {
        IMultiplayerTools* mpTools = AZ::Interface<IMultiplayerTools>::Get();
        if (!editorsv_enabled || !mpTools)
        {
            // Early out if Editor server is not enabled.
            return;
        }

        if (editorsv_clientserver)
        {
            return;
        }

        AzToolsFramework::Prefab::PrefabToInMemorySpawnableNotificationBus::Handler::BusDisconnect();

        if (editorsv_launch)
        {
            const AZ::CVarFixedString remoteAddress = editorsv_serveraddr;
            if (LocalHost != remoteAddress)
            {
                AZ_Warning(
                    "MultiplayerEditor", false,
                    "Launching editor server skipped because of incompatible settings. "
                    "When using editorsv_launch=true editorsv_serveraddr must be set to local address (127.0.0.1) instead %s",
                    remoteAddress.c_str()) return;
            }

            // Find any existing server launchers before launching a new one.
            // It's possible for a rogue server launcher to exist if the Editor shutdown unexpectedly while running a previous multiplayer session.
            // It's also common to open ServerLaunchers by hand for testing, but then to forget to shut it down before starting the editor play mode.
            const AZStd::string serverExeFilename(AZ::Utils::GetProjectName() + ".ServerLauncher" + AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
            int existingServers = AzFramework::ProcessUtils::ProcessCount(serverExeFilename);
            if (existingServers > 0)
            {
                AZ_Warning("MultiplayerEditorSystemComponent", false,
                    "There are already existing servers opened (x%i: %s); please terminate as your Editor may connect to the wrong server! "
                    "If your intention was to connect to this server instead of automatically launching one from the Editor set editorsv_launch = false.",
                    existingServers, serverExeFilename.c_str());
            }

            AZ_Printf("MultiplayerEditor", "Editor is listening for the editor-server...\n");

            // Launch the editor-server
            if (!LaunchEditorServer())
            {
                MultiplayerEditorServerNotificationBus::Broadcast(&MultiplayerEditorServerNotificationBus::Events::OnServerLaunchFail);
                return;
            }
        }

        // Keep trying to connect until the port is finally available.
        m_connectionAttempts = 0;
        constexpr double retrySeconds = 1.0;
        constexpr bool autoRequeue = false;
        m_connectionEvent.Enqueue(AZ::SecondsToTimeMs(retrySeconds), autoRequeue);
    }


    void MultiplayerEditorSystemComponent::OnActionRegistrationHook()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        auto readOnlyEntityPublicInterface = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
        if (!actionManagerInterface || !hotKeyManagerInterface || !readOnlyEntityPublicInterface)
        {
            return;
        }

        // Create Multiplayer Entity
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.multiplayer.createMultiplayerEntity";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Create multiplayer entity";
            actionProperties.m_description = "Create a multiplayer entity.";
            actionProperties.m_category = "Entity";

            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [this, readOnlyEntityPublicInterface]
                {
                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    // when nothing is selected, entity is created at root level.
                    if (selectedEntities.empty())
                    {
                        ContextMenu_NewMultiplayerEntity(AZ::EntityId(), AZ::Vector3::CreateZero());
                    }
                    // when a single entity is selected, entity is created as its child.
                    else if (selectedEntities.size() == 1)
                    {
                        AZ::EntityId selectedEntityId = selectedEntities.front();
                        bool selectedEntityIsReadOnly = readOnlyEntityPublicInterface->IsReadOnly(selectedEntityId);
                        auto containerEntityInterface = AZ::Interface<AzToolsFramework::ContainerEntityInterface>::Get();

                        if (containerEntityInterface && containerEntityInterface->IsContainerOpen(selectedEntityId) && !selectedEntityIsReadOnly)
                        {
                            ContextMenu_NewMultiplayerEntity(selectedEntityId, AZ::Vector3::CreateZero());
                        }
                    }
                }
            );

            actionManagerInterface->InstallEnabledStateCallback(
                actionIdentifier,
                [readOnlyEntityPublicInterface]()
                {
                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    if (selectedEntities.size() == 0)
                    {
                        return true;
                    }
                    else if (selectedEntities.size() == 1)
                    {
                        AZ::EntityId selectedEntityId = selectedEntities.front();
                        bool selectedEntityIsReadOnly = readOnlyEntityPublicInterface->IsReadOnly(selectedEntityId);
                        auto containerEntityInterface = AZ::Interface<AzToolsFramework::ContainerEntityInterface>::Get();

                        return (containerEntityInterface && containerEntityInterface->IsContainerOpen(selectedEntityId) && !selectedEntityIsReadOnly);
                    }

                    return false;
                }
            );

            actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Ctrl+Alt+M");
        }
    }

    void MultiplayerEditorSystemComponent::OnMenuBindingHook()
    {
        auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        if (!menuManagerInterface)
        {
            return;
        }

        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityCreationMenuIdentifier, "o3de.action.multiplayer.createMultiplayerEntity", 1000);
    }

    void MultiplayerEditorSystemComponent::ContextMenu_NewMultiplayerEntity(AZ::EntityId parentEntityId, const AZ::Vector3& worldPosition)
    {
        auto prefabIntegrationInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabIntegrationInterface>::Get();
        AZ::EntityId newEntityId = prefabIntegrationInterface->CreateNewEntityAtPosition(worldPosition, parentEntityId);

        AzToolsFramework::EntityCompositionRequestBus::Broadcast
        (
            &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
            AzToolsFramework::EntityIdList{ newEntityId },
            AZ::ComponentTypeList{ azrtti_typeid<NetBindComponent>(), azrtti_typeid<NetworkTransformComponent>() }
        );
    }
    void MultiplayerEditorSystemComponent::OnStopPlayInEditorBegin()
    {
        if (GetMultiplayer()->GetAgentType() != MultiplayerAgentType::ClientServer || !editorsv_clientserver)
        {
            return;
        }

        // Make sure the client-server stops before the editor leaves play mode.
        // Otherwise network entities will be left hanging around.
        AZ::Interface<IMultiplayer>::Get()->Terminate(DisconnectReason::TerminatedByUser);
    }

} // namespace Multiplayer
