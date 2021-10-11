/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/INetworkSpawnableLibrary.h>
#include <Multiplayer/MultiplayerConstants.h>
#include <Editor/MultiplayerEditorConnection.h>
#include <Source/AutoGen/AutoComponentTypes.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Framework/INetworking.h>

namespace Multiplayer
{
    using namespace AzNetworking;

    AZ_CVAR(bool, editorsv_isDedicated, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether to init as a server expecting data from an Editor. Do not modify unless you're sure of what you're doing.");

    MultiplayerEditorConnection::MultiplayerEditorConnection()
        : m_byteStream(&m_buffer)
    {
        m_networkEditorInterface = AZ::Interface<INetworking>::Get()->CreateNetworkInterface(
            AZ::Name(MpEditorInterfaceName), ProtocolType::Tcp, TrustZone::ExternalClientToServer, *this);
        m_networkEditorInterface->SetTimeoutMs(AZ::TimeMs{ 0 }); // Disable timeouts on this network interface
        if (editorsv_isDedicated)
        {
            uint16_t editorServerPort = DefaultServerEditorPort;
            if (auto console = AZ::Interface<AZ::IConsole>::Get(); console)
            {
                console->GetCvarValue("editorsv_port", editorServerPort);
            }
            AZ_Assert(m_networkEditorInterface, "MP Editor Network Interface was unregistered before Editor Server could start listening.");
            m_networkEditorInterface->Listen(editorServerPort);
        }
    }
  
    bool MultiplayerEditorConnection::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerEditorPackets::EditorServerInit& packet
    )
    {
        // Editor Server Init is intended for non-release targets
        m_byteStream.Write(packet.GetAssetData().GetSize(), reinterpret_cast<void*>(packet.ModifyAssetData().GetBuffer()));

        // In case if this is the last update, process the byteStream buffer. Otherwise more packets are expected
        if (packet.GetLastUpdate())
        {
            // This is the last expected packet
            // Read all assets out of the buffer
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
                AZ::Data::AssetData* assetDatum = AZ::Utils::LoadObjectFromStream<AZ::Data::AssetData>(m_byteStream, nullptr);
                if (!assetDatum)
                {
                    AZLOG_ERROR("EditorServerInit packet contains no asset data. Asset: %s", assetHint.c_str());
                    return false;
                }
                assetSize = m_byteStream.GetCurPos() - assetSize;
                AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::Asset<AZ::Data::AssetData>(assetId, assetDatum, AZ::Data::AssetLoadBehavior::NoLoad);
                asset.SetHint(assetHint);

                AZ::Data::AssetInfo assetInfo;
                assetInfo.m_assetId = asset.GetId();
                assetInfo.m_assetType = asset.GetType();
                assetInfo.m_relativePath = asset.GetHint();
                assetInfo.m_sizeBytes = assetSize;

                // Register Asset to AssetManager
                AZ::Data::AssetManager::Instance().AssignAssetData(asset);
                AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::RegisterAsset, asset.GetId(), assetInfo);

                assetData.push_back(asset);
            }

            // Now that we've deserialized, clear the byte stream
            m_byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            m_byteStream.Truncate();

            // Spawnable library needs to be rebuilt since now we have newly registered in-memory spawnable assets
            AZ::Interface<INetworkSpawnableLibrary>::Get()->BuildSpawnablesList();

            // Load the level via the root spawnable that was registered
            const AZ::CVarFixedString loadLevelString = "LoadLevel Root.spawnable";
            AZ::Interface<AZ::IConsole>::Get()->PerformCommand(loadLevelString.c_str());

            // Setup the normal multiplayer connection
            AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
            INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MpNetworkInterfaceName));

            uint16_t serverPort = DefaultServerPort;
            if (auto console = AZ::Interface<AZ::IConsole>::Get(); console)
            {
                console->GetCvarValue("sv_port", serverPort);
            }
            networkInterface->Listen(serverPort);

            AZLOG_INFO("Editor Server completed asset receive, responding to Editor...");
            return connection->SendReliablePacket(MultiplayerEditorPackets::EditorServerReady());
        }

        return true;
    }

    bool MultiplayerEditorConnection::HandleRequest
    (
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const IPacketHeader& packetHeader,
        [[maybe_unused]] MultiplayerEditorPackets::EditorServerReady& packet
    )
    {
        if (connection->GetConnectionRole() == ConnectionRole::Connector)
        {
            // Receiving this packet means Editor sync is done, disconnect
            connection->Disconnect(AzNetworking::DisconnectReason::TerminatedByClient, AzNetworking::TerminationEndpoint::Local);

            if (auto console = AZ::Interface<AZ::IConsole>::Get(); console)
            {
                AZ::CVarFixedString remoteAddress;
                uint16_t remotePort;
                if (console->GetCvarValue("editorsv_serveraddr", remoteAddress) != AZ::GetValueResult::ConsoleVarNotFound &&
                    console->GetCvarValue("sv_port", remotePort) != AZ::GetValueResult::ConsoleVarNotFound)
                    {
                        // Connect the Editor to the editor server for Multiplayer simulation
                        AZ::Interface<IMultiplayer>::Get()->Connect(remoteAddress.c_str(), remotePort);
                    }
            }
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

    void MultiplayerEditorConnection::OnPacketLost([[maybe_unused]] IConnection* connection, [[maybe_unused]] PacketId packetId)
    {
        ;
    }

    void MultiplayerEditorConnection::OnDisconnect([[maybe_unused]] AzNetworking::IConnection* connection, [[maybe_unused]] DisconnectReason reason, [[maybe_unused]] TerminationEndpoint endpoint)
    {
        bool editorLaunch = false;
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console)
        {
            console->GetCvarValue("editorsv_launch", editorLaunch);
        }

        if (editorsv_isDedicated && editorLaunch && m_networkEditorInterface->GetConnectionSet().GetConnectionCount() == 1)
        {
            if (m_networkEditorInterface->GetPort() != 0)
            {
                m_networkEditorInterface->StopListening();
            }
        }
    }
}
