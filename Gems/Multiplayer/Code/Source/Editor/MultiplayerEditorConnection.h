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

#pragma once

#include <Source/AutoGen/MultiplayerEditor.AutoPacketDispatcher.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/std/string/string.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>

namespace AzNetworking
{
    class INetworkInterface;
}

namespace Multiplayer
{
    //! MultiplayerEditorConnection is a connection listener to synchronize the Editor and a local server it launches
    class MultiplayerEditorConnection final
        : public AzNetworking::IConnectionListener
    {
    public:
        MultiplayerEditorConnection();
        ~MultiplayerEditorConnection() = default;

        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerEditorPackets::EditorServerInit& packet);
        bool HandleRequest(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, MultiplayerEditorPackets::EditorServerReady& packet);
        
        //! IConnectionListener interface
        //! @{
        AzNetworking::ConnectResult ValidateConnect(const AzNetworking::IpAddress& remoteAddress, const AzNetworking::IPacketHeader& packetHeader, AzNetworking::ISerializer& serializer) override;
        void OnConnect(AzNetworking::IConnection* connection) override;
        bool OnPacketReceived(AzNetworking::IConnection* connection, const AzNetworking::IPacketHeader& packetHeader, AzNetworking::ISerializer& serializer) override;
        void OnPacketLost(AzNetworking::IConnection* connection, AzNetworking::PacketId packetId) override;
        void OnDisconnect(AzNetworking::IConnection* connection, AzNetworking::DisconnectReason reason, AzNetworking::TerminationEndpoint endpoint) override;
        //! @}

    private:

        AzNetworking::INetworkInterface* m_networkEditorInterface = nullptr;
        AZStd::vector<uint8_t> m_buffer;
        AZ::IO::ByteContainerStream<AZStd::vector<uint8_t>> m_byteStream;
    };
}
