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

#include <Include/IMultiplayer.h>
#include <Source/Editor/MultiplayerEditorConnection.h>
#include <Source/AutoGen/AutoComponentTypes.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

namespace Multiplayer
{
    using namespace AzNetworking;

    static const AZStd::string_view s_networkInterfaceName("MultiplayerNetworkInterface");
    static const AZStd::string_view s_networkEditorInterfaceName("MultiplayerEditorNetworkInterface");
    static constexpr AZStd::string_view DefaultEditorIp = "127.0.0.1";
    static constexpr uint16_t DefaultServerPort = 30090;
    static constexpr uint16_t DefaultServerEditorPort = 30091;

    static AZStd::vector<uint8_t> buffer;
    static AZ::IO::ByteContainerStream<AZStd::vector<uint8_t>> s_byteStream(&buffer);

    AZ_CVAR(bool, editorsv_isDedicated, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether to init as a server expecting data from an Editor. Do not modify unless you're sure of what you're doing.");

    MultiplayerEditorConnection::MultiplayerEditorConnection()
    {
        m_networkEditorInterface = AZ::Interface<INetworking>::Get()->CreateNetworkInterface(
            AZ::Name(s_networkEditorInterfaceName), ProtocolType::Tcp, TrustZone::ExternalClientToServer, *this);
        if (editorsv_isDedicated)
        {
            m_networkEditorInterface->Listen(DefaultServerEditorPort);
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
            s_byteStream.Write(TcpPacketEncodingBuffer::GetCapacity(), reinterpret_cast<void*>(packet.ModifyAssetData().GetBuffer()));
        }
        else
        {
            // This is the last expected packet, flush it to the buffer
            s_byteStream.Write(packet.GetAssetData().GetSize(), reinterpret_cast<void*>(packet.ModifyAssetData().GetBuffer()));

            // Read all assets out of the buffer
            s_byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> assetData;
            while (s_byteStream.GetCurPos() < s_byteStream.GetLength())
            {
                AZ::Data::AssetLoadBehavior assetLoadBehavior;
                s_byteStream.Read(sizeof(AZ::Data::AssetLoadBehavior), reinterpret_cast<void*>(&assetLoadBehavior));

                AZ::Data::AssetData* assetDatum = AZ::Utils::LoadObjectFromStream<AZ::Data::AssetData>(s_byteStream, nullptr);             
                AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::Asset<AZ::Data::AssetData>(assetDatum, assetLoadBehavior);
                
				/*
				// Register Asset to AssetManager
                */
				
				assetData.push_back(asset);
            }

            // Now that we've deserialized, clear the byte stream
            s_byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            s_byteStream.Truncate();

            /*
            // Hand-off our resultant assets
			*/

            AZLOG_INFO("Editor Server completed asset receive, responding to Editor...");
            if (connection->SendReliablePacket(MultiplayerEditorPackets::EditorServerReady()))
            {
                // Setup the normal multiplayer connection
                AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
                INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(s_networkInterfaceName));
                networkInterface->Listen(DefaultServerPort);

                return true;
            }
            else
            {
                return false;
            }
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

            // Connect the Editor to the local server for Multiplayer simulation
            AZ::Interface<IMultiplayer>::Get()->InitializeMultiplayer(MultiplayerAgentType::Client);
            INetworkInterface* networkInterface = AZ::Interface<INetworking>::Get()->RetrieveNetworkInterface(AZ::Name(s_networkInterfaceName));
            const IpAddress ipAddress(DefaultEditorIp.data(), DefaultServerEditorPort, networkInterface->GetType());
            networkInterface->Connect(ipAddress);
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
