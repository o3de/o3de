/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzFramework/Spawnable/InMemorySpawnableAssetContainer.h"

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/INetworkSpawnableLibrary.h>
#include <Multiplayer/MultiplayerConstants.h>
#include <Multiplayer/MultiplayerEditorServerBus.h>
#include <Editor/MultiplayerEditorConnection.h>
#include <Source/AutoGen/AutoComponentTypes.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzFramework/Spawnable/InMemorySpawnableAssetContainer.h>

namespace Multiplayer
{
    using namespace AzNetworking;

    AZ_CVAR(bool, editorsv_isDedicated, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether to init as a server expecting data from an Editor. Do not modify unless you're sure of what you're doing.");
    AZ_CVAR(uint16_t, editorsv_port, DefaultServerEditorPort, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The port that the multiplayer editor gem will bind to for traffic.");

    MultiplayerEditorConnection::MultiplayerEditorConnection()
        : m_byteStream(&m_buffer)
    {
        m_networkEditorInterface = AZ::Interface<INetworking>::Get()->CreateNetworkInterface(
            AZ::Name(MpEditorInterfaceName), ProtocolType::Tcp, TrustZone::ExternalClientToServer, *this);
        m_networkEditorInterface->SetTimeoutMs(AZ::Time::ZeroTimeMs); // Disable timeouts on this network interface

        // Wait to activate the editor-server until LegacySystemInterfaceCreated so that the logging system is ready
        // Automated testing listens for these logs
        if (editorsv_isDedicated)
        {
            // Server logs will be piped to the editor so turn off buffering,
            // otherwise it'll take a lot of logs to fill up the buffer before stdout is finally flushed.
            // This isn't optimal, but will only affect editor-servers (used when testing multiplayer levels in Editor gameplay mode) and not production servers.
            // Note: _IOLBF (flush on newlines) won't work for Automated Testing which uses a headless server app and will fall back to _IOFBF (full buffering)
            setvbuf(stdout, NULL, _IONBF, 0);

            // If the settings registry is not available at this point,
            // then something catastrophic has happened in the application startup.
            // That should have been caught and messaged out earlier in startup.
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                AZ::ComponentApplicationLifecycle::RegisterHandler(
                    *settingsRegistry, m_componentApplicationLifecycleHandler,
                    [this](AZStd::string_view /*path*/, AZ::SettingsRegistryInterface::Type /*type*/)
                    {
                        ActivateDedicatedEditorServer();
                    },
                    "CriticalAssetsCompiled");
            }
        }
    }

    MultiplayerEditorConnection::~MultiplayerEditorConnection()
    {
        if (m_inMemorySpawnableAssetContainer != nullptr)
        {
            m_inMemorySpawnableAssetContainer->ClearAllInMemorySpawnableAssets();
        }
    }

    void MultiplayerEditorConnection::ActivateDedicatedEditorServer() const
    {
        if (m_isActivated || !editorsv_isDedicated)
        {
            return;
        }
        m_isActivated = true;
        
        AZ_Assert(m_networkEditorInterface, "MP Editor Network Interface was unregistered before Editor Server could start listening.")

        m_networkEditorInterface->Listen(editorsv_port);
    }

    bool MultiplayerEditorConnection::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        MultiplayerEditorPackets::EditorServerLevelData& packet
    )
    {
        // Editor Server Init is intended for non-release targets
        m_byteStream.Write(packet.GetAssetData().GetSize(), reinterpret_cast<void*>(packet.ModifyAssetData().GetBuffer()));

        // In case if this is the last update, process the byteStream buffer. Otherwise more packets are expected
        if (!packet.GetLastUpdate())
        {
            return true;
        }
        
        // This is the last expected packet
        // Read all assets out of the buffer
        // Create in-memory spawnables for the level, root.spawnable and root.network.spawnable (if level contains network entities)
        if (m_inMemorySpawnableAssetContainer != nullptr)
        {
            m_inMemorySpawnableAssetContainer->ClearAllInMemorySpawnableAssets();
        }
        m_inMemorySpawnableAssetContainer = AZStd::make_unique<AzFramework::InMemorySpawnableAssetContainer>();
        AzFramework::InMemorySpawnableAssetContainer::AssetDataInfoContainer rootSpawnableAssetDataInfoContainer;

        m_byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> assetData;
        while (m_byteStream.GetCurPos() < m_byteStream.GetLength())
        {
            AZ::Data::AssetId assetId;
            uint32_t hintSize;
            AZStd::string assetHint;
            m_byteStream.Read(sizeof(AZ::Data::AssetId), reinterpret_cast<void*>(&assetId));
            m_byteStream.Read(sizeof(uint32_t), reinterpret_cast<void*>(&hintSize));
            assetHint.resize(hintSize);
            m_byteStream.Read(hintSize, assetHint.data());
            size_t assetSize = m_byteStream.GetCurPos();

            // Load spawnable from stream without loading any asset references
            AzFramework::Spawnable* spawnable = AZ::Utils::LoadObjectFromStream<AzFramework::Spawnable>(
                m_byteStream, nullptr, AZ::ObjectStream::FilterDescriptor(AZ::Data::AssetFilterNoAssetLoading));
            if (!spawnable)
            {
                AZLOG_ERROR("EditorServerLevelData packet contains no asset data. Asset: %s", assetHint.c_str())
                return false;
            }

            // We only care about Root.spawnable and Root.network.spawnable
            // @todo update editor so it only sends Root spawnables in the first place
            if (assetHint.starts_with("Root"))
            {
                AZ::Data::AssetInfo spawnableAssetInfo;
                spawnableAssetInfo.m_sizeBytes = m_byteStream.GetCurPos() - assetSize;
                spawnableAssetInfo.m_assetId = assetId;
                spawnableAssetInfo.m_assetType = spawnable->GetType();
                spawnableAssetInfo.m_relativePath = assetHint;

                rootSpawnableAssetDataInfoContainer.emplace_back(spawnable, spawnableAssetInfo);
            }
        }

        // Now that we've deserialized, clear the byte stream
        m_byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        m_byteStream.Truncate();

        // Create in-memory spawnables without loading dependent assets. They will be loaded on level load
        constexpr bool loadDependentAssets = false;
        AzFramework::InMemorySpawnableAssetContainer::CreateSpawnableResult createSpawnableResult =
            m_inMemorySpawnableAssetContainer->CreateInMemorySpawnableAsset(rootSpawnableAssetDataInfoContainer, loadDependentAssets, "Root");
        if (!createSpawnableResult.IsSuccess())
        {
            AZ_Assert(false, createSpawnableResult.GetError().c_str())
        }

        // Spawnable library needs to be rebuilt since now we have newly registered in-memory spawnable assets
        AZ::Interface<INetworkSpawnableLibrary>::Get()->BuildSpawnablesList();

        // Load the level via the root spawnable that was registered
        const auto console = AZ::Interface<AZ::IConsole>::Get();
        console->PerformCommand("LoadLevel Root.spawnable");

        // Setup the normal multiplayer connection
        AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpNetworkInterfaceName));

        uint16_t sv_port = DefaultServerPort;
        if (console->GetCvarValue("sv_port", sv_port) != AZ::GetValueResult::Success)
        {
            AZ_Assert(false,
                "MultiplayerEditorConnection::HandleRequest for EditorServerLevelData failed! Could not find the sv_port cvar; we won't be able to listen on the correct port for incoming network messages! Please update this code to use a valid cvar!")
        }
        
        networkInterface->Listen(sv_port);

        AZLOG_INFO("Editor Server completed receiving the editor's level assets, responding to Editor...");
        return connection->SendReliablePacket(MultiplayerEditorPackets::EditorServerReady());
    }

    bool MultiplayerEditorConnection::HandleRequest(
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerEditorPackets::EditorServerReadyForLevelData& packet)
    {
        MultiplayerEditorServerRequestBus::Broadcast(&MultiplayerEditorServerRequestBus::Events::SendEditorServerLevelDataPacket, connection);
        return true;
    }

    bool MultiplayerEditorConnection::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerEditorPackets::EditorServerReady& packet
    )
    {
        // Receiving this packet means Editor sync is done, disconnect
        connection->Disconnect(AzNetworking::DisconnectReason::TerminatedByClient, AzNetworking::TerminationEndpoint::Local);
        const auto console = AZ::Interface<AZ::IConsole>::Get();
        AZ::CVarFixedString editorsv_serveraddr = AZ::CVarFixedString(LocalHost);
        uint16_t sv_port = DefaultServerEditorPort;

        if (console->GetCvarValue("sv_port", sv_port) != AZ::GetValueResult::Success)
        {
            AZ_Assert(false,
                "MultiplayerEditorConnection::HandleRequest for EditorServerReady failed! Could not find the sv_port cvar; we may not be able to "
                "connect to the correct port for incoming network messages! Please update this code to use a valid cvar!")
        }

        if (console->GetCvarValue("editorsv_serveraddr", editorsv_serveraddr) != AZ::GetValueResult::Success)
        {
            AZ_Assert(false,
                "MultiplayerEditorConnection::HandleRequest for EditorServerReady failed! Could not find the editorsv_serveraddr cvar; we may not be able to "
                "connect to the correct port for incoming network messages! Please update this code to use a valid cvar!")
        }
        
        // Connect the Editor to the editor server for Multiplayer simulation
        if (AZ::Interface<IMultiplayer>::Get()->Connect(editorsv_serveraddr.c_str(), sv_port))
        {
            AZ_Printf("MultiplayerEditorConnection", "Editor-server ready. Editor has successfully connected to the editor-server's network simulation.")
        }
        else
        {
            AZ_Warning("MultiplayerEditorConnection", false, "MultiplayerEditorConnection::HandleRequest for EditorServerReady failed! Connecting to the editor-server's network simulation failed.")
        }
        return true;
    }

    ConnectResult MultiplayerEditorConnection::ValidateConnect
    (
        [[maybe_unused]] const IpAddress& remoteAddress,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] ISerializer& serializer
    )
    {
        return ConnectResult::Accepted;
    }

    void MultiplayerEditorConnection::OnConnect([[maybe_unused]] AzNetworking::IConnection* connection)
    {
        ;
    }

    AzNetworking::PacketDispatchResult MultiplayerEditorConnection::OnPacketReceived(AzNetworking::IConnection* connection, const IPacketHeader& packetHeader, ISerializer& serializer)
    {
        return MultiplayerEditorPackets::DispatchPacket(connection, packetHeader, serializer, *this);
    }
} // namespace Multiplayer
