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

#include <Multiplayer/IMultiplayer.h>
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
            AZ::Name(MPEditorInterfaceName), ProtocolType::Tcp, TrustZone::ExternalClientToServer, *this);
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
        if (!packet.GetLastUpdate())
        {
            // More packets are expected, flush this to the buffer
            m_byteStream.Write(TcpPacketEncodingBuffer::GetCapacity(), reinterpret_cast<void*>(packet.ModifyAssetData().GetBuffer()));
        }
        else
        {
            // This is the last expected packet, flush it to the buffer
            m_byteStream.Write(packet.GetAssetData().GetSize(), reinterpret_cast<void*>(packet.ModifyAssetData().GetBuffer()));

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

            // Load the level via the root spawnable that was registered
            const AZ::CVarFixedString loadLevelString = "LoadLevel Root.spawnable";
            AZ::Interface<AZ::IConsole>::Get()->PerformCommand(loadLevelString.c_str());

            // Setup the normal multiplayer connection
            AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
            INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MPNetworkInterfaceName));

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
                        AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(MultiplayerAgentType::Client);
                        INetworkInterface* networkInterface =
                            AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(MPNetworkInterfaceName));

                        const IpAddress ipAddress(remoteAddress.c_str(), remotePort, networkInterface->GetType());
                        networkInterface->Connect(ipAddress);

                        AZ::Interface<IMultiplayer>::Get()->SendReadyForEntityUpdates(true);
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

    bool MultiplayerEditorConnection::OnPacketReceived(AzNetworking::IConnection* connection, const IPacketHeader& packetHeader, ISerializer& serializer)
    {
        return MultiplayerEditorPackets::DispatchPacket(connection, packetHeader, serializer, *this);
    }

    void MultiplayerEditorConnection::OnPacketLost([[maybe_unused]] IConnection* connection, [[maybe_unused]] PacketId packetId)
    {
        ;
    }

    void MultiplayerEditorConnection::OnDisconnect([[maybe_unused]] AzNetworking::IConnection* connection, [[maybe_unused]] DisconnectReason reason, [[maybe_unused]] TerminationEndpoint endpoint)
    {
        ;
    }
}
