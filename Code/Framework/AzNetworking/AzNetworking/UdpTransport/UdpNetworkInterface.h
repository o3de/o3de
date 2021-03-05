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

#include <AzNetworking/UdpTransport/UdpPacketHeader.h>
#include <AzNetworking/UdpTransport/UdpConnectionSet.h>
#include <AzNetworking/UdpTransport/UdpReaderThread.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/ConnectionLayer/ConnectionEnums.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <AzNetworking/DataStructures/TimeoutQueue.h>
#include <AzCore/Threading/ThreadSafeDeque.h>
#include <AzCore/std/containers/vector.h>

namespace AzNetworking
{
    class IConnectionListener;
    class ICompressor;

    // 20 byte IPv4 header + 8 byte UDP header
    static const uint32_t UdpPacketHeaderSize = 20 + 8;
    static const uint32_t DtlsPacketHeaderSize = 13; // DTLS1_RT_HEADER_LENGTH

    //! @class UdpNetworkInterface
    //! @brief This class implements a UDP network interface.
    class UdpNetworkInterface final
        : public INetworkInterface
    {
    public:

        //! Constructor.
        //! @param name               the name of this network interface instance.
        //! @param connectionListener reference to the connection listener responsible for handling all connection events
        //! @param trustZone          the trust level assigned to this network interface, server to server or client to server
        //! @param readerThread       pointer to the reader thread to be bound to this network interface
        UdpNetworkInterface(AZ::Name name, IConnectionListener& connectionListener, TrustZone trustZone, UdpReaderThread& readerThread);
        ~UdpNetworkInterface() override;

        //! INetworkInterface interface.
        //! @{
        AZ::Name GetName() const override;
        ProtocolType GetType() const override;
        TrustZone GetTrustZone() const override;
        uint16_t GetPort() const override;
        IConnectionSet& GetConnectionSet() override;
        IConnectionListener& GetConnectionListener() override;
        bool Listen(uint16_t port) override;
        ConnectionId Connect(const IpAddress& remoteAddress) override;
        void Update(AZ::TimeMs deltaTimeMs) override;
        bool SendReliablePacket(ConnectionId connectionId, const IPacket& packet) override;
        PacketId SendUnreliablePacket(ConnectionId connectionId, const IPacket& packet) override;
        bool WasPacketAcked(ConnectionId connectionId, PacketId packetId) override;
        bool Disconnect(ConnectionId connectionId, DisconnectReason reason) override;
        //! @}

        //! Returns true if this is an encrypted socket, false if not.
        //! @return boolean true if this is an encrypted socket, false if not
        bool IsEncrypted() const;

        //! Returns true if this connection instance is in an open state, and is capable of actively sending and receiving packets.
        //! @return boolean true if this connection instance is in an open state
        bool IsOpen() const;

    private:

        //! Registers a packet with a timeout queue on the provided connection.
        //! @param connectionId identifier of the connection to register
        //! @param packetId     packet id of the packet to register for the given connection
        //! @param reliability  whether or not to guarantee delivery
        //! @param metrics      reference to the connections metrics instance
        void RegisterWithTimeoutQueue(ConnectionId connectionId, PacketId packetId, ReliabilityType reliability, const ConnectionMetrics& metrics);

        //! Decompresses an incoming packet data buffer.
        //! @param packetBuffer    the compressed packet buffer to decode
        //! @param packetSize      the size of the compressed packet buffer
        //! @param packetBufferOut the decoded data
        //! @return boolean true on success, false on failure
        bool DecompressPacket(const uint8_t* packetBuffer, size_t packetSize, UdpPacketEncodingBuffer& packetBufferOut) const;

        //! Sends a packet to the remote connection.
        //! @param connection         the UdpConnection instance to send the packet on
        //! @param packet             serializable object to transmit
        //! @param reliableSequence   the reliable sequence number to use for this packet, providing InvalidSequenceId will cause the packet to be sent unreliably
        //! @return packet id for the transmitted packet
        PacketId SendPacket(UdpConnection& connection, const IPacket& packet, SequenceId reliableSequence);

        //! Accepts an incoming udp connection.
        //! @param connectPacket the initial connectPacket
        void AcceptConnection(const UdpReaderThread::ReceivedPacket& connectPacket);

        //! Internal helper to cleanly remove a connection from the network interface.
        //! @param connection pointer to the connection to disconnect
        //! @param reason     reason for the disconnect
        //! @param endpoint   whether the disconnection was initiated locally or remotely
        void RequestDisconnect(UdpConnection* connection, DisconnectReason reason, TerminationEndpoint endpoint);

        AZ_DISABLE_COPY_MOVE(UdpNetworkInterface);

        struct ConnectionTimeoutFunctor final
            : public ITimeoutHandler
        {
            ConnectionTimeoutFunctor(UdpNetworkInterface& networkInterface);
            TimeoutResult HandleTimeout(TimeoutQueue::TimeoutItem& item) override;
        private:
            AZ_DISABLE_COPY_MOVE(ConnectionTimeoutFunctor);
            UdpNetworkInterface& m_networkInterface;
        };

        struct PacketTimeoutFunctor final
            : public ITimeoutHandler
        {
            PacketTimeoutFunctor(UdpNetworkInterface& networkInterface);
            TimeoutResult HandleTimeout(TimeoutQueue::TimeoutItem& item) override;
        private:
            AZ_DISABLE_COPY_MOVE(PacketTimeoutFunctor);
            UdpNetworkInterface& m_networkInterface;
        };

        AZ::Name m_name;
        TrustZone m_trustZone;
        uint16_t m_port = 0;
        bool m_allowIncomingConnections = false;
        IConnectionListener& m_connectionListener;
        UdpConnectionSet m_connectionSet;
        TimeoutQueue m_connectionTimeoutQueue;
        TimeoutQueue m_packetTimeoutQueue;
        AZStd::unique_ptr<UdpSocket> m_socket;
        AZStd::unique_ptr<ICompressor> m_compressor;
        UdpReaderThread& m_readerThread;

        struct RemovedConnection
        {
            UdpConnection* m_connection;
            DisconnectReason m_reason;
            TerminationEndpoint m_endpoint;
        };
        AZStd::vector<RemovedConnection> m_removedConnections;

        UdpPacketEncodingBuffer m_decryptBuffer;
        UdpPacketEncodingBuffer m_decompressBuffer;

        friend class UdpReliableQueue;
        friend class UdpConnection; // For access to private RequestDisconnect() method
    };
}
