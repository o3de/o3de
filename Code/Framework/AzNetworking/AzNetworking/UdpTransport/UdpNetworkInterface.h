/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    static const uint32_t UdpPacketHeaderSize = 20 + 8; //!< 20 byte IPv4 header + 8 byte UDP header
    static const uint32_t DtlsPacketHeaderSize = 13; //!< DTLS1_RT_HEADER_LENGTH

    //! @class UdpNetworkInterface
    //! @brief This class implements a UDP network interface.
    //!
    //! UdpNetworkInterface is an implementation of AzNetworking::INetworkInterface. Since UDP is a very bare bones protocol,
    //! the Open 3D Engine implementation has to provide significantly more than its TCP counterpart (since TCP implements a
    //! significant number of reliability features.)
    //! 
    //! When sent through UDP, a packet can have additional actions performed on it depending on which features are enabled and
    //! configured. Each feature listed in this description is in the order a packet will see them on Send.
    //! 
    //! ### Packet structure
    //! 
    //! The general structure of a UDP packet is:
    //! 
    //! * Flags - A bitfield a receiving endpoint can quickly inspect to learn about configuration of a packet
    //! * Header - Details the type of packet and other information related to reliability
    //! * Payload - The actual serialized content of the packet
    //! 
    //! For more information, read [Networking Packets](http://o3de.org/docs/user-guide/networking/packets) in the O3DE documentation.
    //! 
    //! ### Reliability
    //! 
    //! UDP packets can be sent reliably or unreliably. Reliably sent packets are registered for tracking first. This causes the
    //! reliable packet to be resent if a timeout on the packet is reached. Once the packet is acknowledged, the packet is
    //! unregistered.
    //! 
    //! ### Fragmentation
    //! 
    //! If the raw packet size exceeds the configured maximum transmission unit (MTU) then the packet is broken into
    //! multiple reliable fragments to avoid fragmentation at the routing level. Fragments are always reliable so the original
    //! packet can be reconstructed. Operations that alter the payload generally follow this step so that they can be
    //! separately applied to the Fragments in addition to not being applied to both the original and Fragments.
    //! 
    //! ### Compression
    //! 
    //! Compression here refers to content insensitive compression using libraries like LZ4. If enabled, the target payload is
    //! run through the compressor and replaces the original payload if it's in fact smaller. To tell if compression is enabled
    //! on a given packet, we operate on a bit in the packet's Flags. The Sender writes this bit while the Receiver checks it to
    //! see if a packet needs to be decompressed.
    //! 
    //! O3DE could potentially move from over MTU to under with compression, and the UDP interface doesn't check for this. Detecting a change
    //! that would reduce the number of fragmented packets would require pre-emptively compressing payloads to tell if that change happened,
    //! which could potentially lead to a lot of unnecessary calls to the compressor.
    //! 
    //! ### Encryption
    //! 
    //! AzNetworking uses the [OpenSSL](https://www.openssl.org/) library to implement Datagram Layer Transport Security (DTLS) encryption
    //! on UDP traffic. Encryption operates as described in [O3DE Networking Encryption](http://o3de.org/docs/user-guide/networking/encryption)
    //! on the documentation website. Once both endpoints have completed their handshake, all traffic is expected to be fully encrypted.
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
        bool StopListening() override;
        bool Disconnect(ConnectionId connectionId, DisconnectReason reason) override;
        void SetTimeoutMs(AZ::TimeMs timeoutMs) override;
        AZ::TimeMs GetTimeoutMs() const override;
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

        //! Internal helper to check if a packet's type is for connection handshake.
        //! @param  endpoint   DTLS endpoint participating in the handshake
        //! @param  packetType type of the packet
        //! @return if the packet is for handshake
        bool IsHandshakePacket(const DtlsEndpoint& endpoint, AzNetworking::PacketType packetType) const;

        //! Internal helper to manage connection timeout behaviour.
        //! @param item the timeout item corresponding to the timed out connection
        //! @return whether to delete or persist the timeout item
        TimeoutResult HandleConnectionTimeout(TimeoutQueue::TimeoutItem& item);

        //! Internal helper to manage packet timeout behaviour.
        //! @param item the timeout item corresponding to the timed out packet
        //! @return whether to delete or persist the timeout item
        TimeoutResult HandlePacketTimeout(TimeoutQueue::TimeoutItem& item);

        AZ_DISABLE_COPY_MOVE(UdpNetworkInterface);

        AZ::Name m_name;
        TrustZone m_trustZone;
        uint16_t m_port = 0;
        bool m_allowIncomingConnections = false;
        AZ::TimeMs m_timeoutMs = AZ::Time::ZeroTimeMs;
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
