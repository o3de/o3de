/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Framework/INetworking.h>
#include <AzNetworking/UdpTransport/UdpNetworkInterface.h>
#include <AzNetworking/UdpTransport/UdpConnection.h>
#include <AzNetworking/UdpTransport/DtlsSocket.h>
#include <AzNetworking/UdpTransport/UdpSocket.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/Framework/ICompressor.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Math/MathUtils.h>

namespace AzNetworking
{
#if AZ_TRAIT_USE_OPENSSL
    AZ_CVAR(bool, net_UdpUseEncryption, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Enable encryption on Udp based connections");
    AZ_CVAR(uint32_t, net_SslInflationOverhead, 32, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "A SSL fudge overhead value to take out of fragmented packet payloads");
#else
    static const bool net_UdpUseEncryption = false;
    static const uint32_t net_SslInflationOverhead = 0;
#endif

    AZ_CVAR(bool, net_UdpTimeoutConnections, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Boolean value on whether we should timeout Udp connections");
    AZ_CVAR(AZ::TimeMs, net_UdpPacketTimeSliceMs, AZ::TimeMs{ 8 }, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The number of milliseconds to allow for packet processing");
    AZ_CVAR(uint32_t, net_UdpUnackedHeartbeats, 5, nullptr, AZ::ConsoleFunctorFlags::Null, "The number of heartbeats to attempt to send to keep a connection alive before giving up");
    AZ_CVAR(AZ::TimeMs, net_UdpDefaultTimeoutMs, AZ::TimeMs{ 10 * 1000 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Time in milliseconds before we timeout an idle Udp connection");
    AZ_CVAR(AZ::TimeMs, net_MinPacketTimeoutMs, AZ::TimeMs{ 200 }, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Minimum time to wait before timing out an unacked packet");
    AZ_CVAR(int32_t, net_MaxTimeoutsPerFrame, 1000, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Maximum number of packet timeouts to allow to process in a single frame");
    AZ_CVAR(float, net_RttFudgeScalar, 2.0f, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Scalar value to multiply computed Rtt by to determine an optimal packet timeout threshold");
    AZ_CVAR(uint32_t, net_FragmentedHeaderOverhead, 32, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "A fudge overhead value to take out of fragmented packet payloads");
    AZ_CVAR(bool, net_FragmentsAlwaysReliable, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Whether fragmented packets should be reliable by default or use their source packet's reliability type");
    AZ_CVAR(AZ::CVarFixedString, net_UdpCompressor, "MultiplayerCompressor", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "UDP compressor to use."); // WARN: similar to encryption this needs to be set once and only once before creating the network interface

    static uint64_t ConstructTimeoutId(ConnectionId connectionId, PacketId packetId, ReliabilityType reliability)
    {
        const uint64_t intConnectionId = aznumeric_cast<uint64_t>(connectionId);
        const uint64_t intPacketId = aznumeric_cast<uint64_t>(packetId);
        const uint64_t intReliability = (reliability == ReliabilityType::Reliable) ? 1 : 0;
        const uint64_t baseTimeoutId = ((intConnectionId << 32) | intPacketId) & 0x7FFFFFFFFFFFFFFF;
        return (intReliability << 63) | baseTimeoutId;
    }

    static void DecodeTimeoutId(uint64_t timeoutId, ConnectionId& outConnectionId, PacketId& outPacketId, ReliabilityType& outReliability)
    {
        outConnectionId = ConnectionId(aznumeric_cast<uint32_t>(timeoutId >> 32) & 0x7FFFFFFF);
        outPacketId = PacketId(aznumeric_cast<uint32_t>(timeoutId >> 0) & 0xFFFFFFFF);
        outReliability = ((timeoutId & 0x8000000000000000) > 0) ? ReliabilityType::Reliable : ReliabilityType::Unreliable;
    }

    UdpNetworkInterface::UdpNetworkInterface(const AZ::Name& name, IConnectionListener& connectionListener, TrustZone trustZone, UdpReaderThread& readerThread, UdpHeartbeatThread& heartbeatThread)
        : m_name(name)
        , m_trustZone(trustZone)
        , m_connectionListener(connectionListener)
        , m_socket(net_UdpUseEncryption ? new DtlsSocket() : new UdpSocket())
        , m_readerThread(readerThread)
        , m_heartbeatThread(heartbeatThread)
        , m_timeoutMs(net_UdpDefaultTimeoutMs)
    {
        const AZ::CVarFixedString compressor = static_cast<AZ::CVarFixedString>(net_UdpCompressor);
        m_compressor = AZ::Interface<INetworking>::Get()->CreateCompressor(compressor);
        m_heartbeatThread.RegisterNetworkInterface(this);
    }

    UdpNetworkInterface::~UdpNetworkInterface()
    {
        m_heartbeatThread.UnregisterNetworkInterface(this);
        m_readerThread.UnregisterSocket(m_socket.get());
    }

    AZ::Name UdpNetworkInterface::GetName() const
    {
        return m_name;
    }

    ProtocolType UdpNetworkInterface::GetType() const
    {
        return ProtocolType::Udp;
    }

    TrustZone UdpNetworkInterface::GetTrustZone() const
    {
        return m_trustZone;
    }

    uint16_t UdpNetworkInterface::GetPort() const
    {
        return m_port;
    }

    IConnectionSet& UdpNetworkInterface::GetConnectionSet()
    {
        return m_connectionSet;
    }

    IConnectionListener& UdpNetworkInterface::GetConnectionListener()
    {
        return m_connectionListener;
    }

    bool UdpNetworkInterface::Listen(uint16_t port)
    {
        if (m_socket->IsOpen())
        {
            AZ_Assert(false, "Listen cannot be invoked on an already opened network interface");
            return false;
        }

        m_port = port;
        m_allowIncomingConnections = true;
        if (m_socket->Open(m_port, UdpSocket::CanAcceptConnections::True, m_trustZone))
        {
            m_readerThread.RegisterSocket(m_socket.get());
            return true;
        }
        else
        {
            return false;
        }
    }

    ConnectionId UdpNetworkInterface::Connect(const IpAddress& remoteAddress, uint16_t localPort)
    {
        if (m_connectionSet.GetConnection(remoteAddress) != nullptr)
        {
            AZLOG_WARN("Attempting to connect to an endpoint (%s) that already has a connection", remoteAddress.GetString().c_str());
            return InvalidConnectionId;
        }

        m_port = localPort;
        if (!m_socket->IsOpen())
        {
            if (m_socket->Open(m_port, UdpSocket::CanAcceptConnections::False, m_trustZone))
            {
                m_readerThread.RegisterSocket(m_socket.get());
            }
            else
            {
                return InvalidConnectionId;
            }
        }

        const ConnectionId connectionId = m_connectionSet.GetNextConnectionId();
        const AZ::TimeMs timeoutTimeMs = m_timeoutMs / static_cast<AZ::TimeMs>(static_cast<int32_t>(net_UdpUnackedHeartbeats));
        const TimeoutId timeoutId = m_connectionTimeoutQueue.RegisterItem(aznumeric_cast<uint64_t>(connectionId), timeoutTimeMs);

        AZStd::unique_ptr<UdpConnection> connection = AZStd::make_unique<UdpConnection>(connectionId, remoteAddress, *this, ConnectionRole::Connector);
        UdpPacketEncodingBuffer dtlsData;
        m_socket->ConnectDtlsEndpoint(connection->GetDtlsEndpoint(), remoteAddress, dtlsData);

        // We're initiating this connection, so go to a connecting state until we receive some kind of response so that we know it's alive and valid
        connection->m_state = ConnectionState::Connecting;
        connection->SetConnectionMtu(MaxUdpTransmissionUnit);
        connection->SetTimeoutId(timeoutId);

        // Signal the connection attempt
        CorePackets::InitiateConnectionPacket connectPacket = CorePackets::InitiateConnectionPacket();
        connectPacket.SetHandshakeBuffer(dtlsData);
        connection->SendReliablePacket(connectPacket);

        m_connectionListener.OnConnect(connection.get());
        m_connectionSet.AddConnection(AZStd::move(connection));
        return connectionId;
    }

    void UdpNetworkInterface::Update()
    {
        m_lastSystemTickUpdate = AZ::GetElapsedTimeMs();

        if (!m_socket->IsOpen())
        {
            return;
        }

        const AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();
        const UdpReaderThread::ReceivedPackets* packets = m_readerThread.GetReceivedPackets(m_socket.get());
        if (packets == nullptr)
        {
            // Socket is not yet registered with the reader thread and is likely still pending, try again later
            return;
        }

        for (uint32_t i = 0; i < packets->size(); ++i)
        {
            const UdpReaderThread::ReceivedPacket& packet = (*packets)[i];
            const AZ::TimeMs currentTimeMs = AZ::GetElapsedTimeMs();

            // Don't exceed our timeslice, even if unprocessed data remains
            if ((currentTimeMs - startTimeMs) > net_UdpPacketTimeSliceMs)
            {
                AZLOG_WARN("Processing time exceeded, discarding %d/%d received packets", aznumeric_cast<int32_t>(packets->size() - i), aznumeric_cast<int32_t>(packets->size()));
                GetMetrics().m_discardedPackets += packets->size() - i;
                break;
            }

            UdpConnection* connection = m_connectionSet.GetConnection(packet.m_address);
            if (connection == nullptr)
            {
                AcceptConnection(packet);
                continue;
            }

            const DisconnectReason disconnectReason = GetDisconnectReasonForSocketResult(packet.m_receivedBytes);
            if (disconnectReason != DisconnectReason::MAX)
            {
                connection->Disconnect(disconnectReason, TerminationEndpoint::Local);
                continue;
            }
            
            const ConnectionState connectionState = connection->GetConnectionState();
            if (connectionState == ConnectionState::Disconnecting || connectionState == ConnectionState::Disconnected)
            {
                // Skip packets from disconnected connections
                continue;
            }

            int32_t decodedPacketSize = 0;
            m_decryptBuffer.Resize(m_decryptBuffer.GetCapacity());
            const uint8_t* decodedPacketData = connection->GetDtlsEndpoint().DecodePacket(*connection, packet.m_buffer, packet.m_receivedBytes, m_decryptBuffer.GetBuffer(), decodedPacketSize);
            m_decryptBuffer.Resize(decodedPacketSize);

            if (decodedPacketSize == 0)
            {
                // OpenSSL may have consumed packets during handshake negotiation
                continue;
            }
            else if (decodedPacketSize < 0)
            {
                // Late unencrypted handshake packets or just random garbage can show up, discard and continue
                continue;
            }

            connection->GetMetrics().LogPacketRecv(packet.m_receivedBytes + UdpPacketHeaderSize, currentTimeMs);

            // Decode the packet flag bitset first since it's always uncompressed
            UdpPacketHeader header;
            {
                NetworkOutputSerializer flagSerializer(decodedPacketData, decodedPacketSize);
                if (!header.SerializePacketFlags(flagSerializer))
                {
                    continue;
                }
                // Adjust decoded tracking to represent the payload now that we've grabbed the flags
                decodedPacketData = flagSerializer.GetUnreadData();
                decodedPacketSize = flagSerializer.GetUnreadSize();
                GetMetrics().m_recvBytesUncompressed += flagSerializer.GetReadSize();
            }

            if (m_compressor && header.IsPacketFlagSet(PacketFlag::Compressed))
            {
                // Only the payload is compressed
                if (!DecompressPacket(decodedPacketData, decodedPacketSize, m_decompressBuffer))
                {
                    AZLOG_WARN("Failed to decompress packet!");
                    continue;
                }
                decodedPacketData = m_decompressBuffer.GetBuffer();
                decodedPacketSize = static_cast<int32_t>(m_decompressBuffer.GetSize());
            }
            GetMetrics().m_recvBytesUncompressed += decodedPacketSize;

            TimeoutQueue::TimeoutItem* timeoutItem = m_connectionTimeoutQueue.RetrieveItem(connection->GetTimeoutId());
            if (timeoutItem == nullptr)
            {
                connection->Disconnect(DisconnectReason::Unknown, TerminationEndpoint::Local);
                continue;
            }
            else
            {
                // Deserialize the packet header
                NetworkOutputSerializer packetSerializer(decodedPacketData, decodedPacketSize);
                ISerializer& serializer = packetSerializer; // To get the default typeinfo parameters in ISerializer
                if (!serializer.Serialize(header, "Header"))
                {
                    continue;
                }

                // Note that the serializer passed in here is unused for UDP
                if (!connection->ProcessReceived(header, packetSerializer, packet.m_receivedBytes + UdpPacketHeaderSize, currentTimeMs))
                {
                    continue;
                }

                timeoutItem->UpdateTimeoutTime(startTimeMs);
                connection->m_timeoutCounter = 0;

                PacketDispatchResult handledPacket = PacketDispatchResult::Failure;
                if (header.GetPacketType() < aznumeric_cast<PacketType>(CorePackets::PacketType::MAX))
                {
                    handledPacket = connection->HandleCorePacket(m_connectionListener, header, packetSerializer);
                }
                else
                {
                    handledPacket = m_connectionListener.OnPacketReceived(connection, header, packetSerializer);
                }

                if (handledPacket == PacketDispatchResult::Success)
                {
                    connection->UpdateHeartbeat(currentTimeMs);
                    if (connection->GetConnectionState() == ConnectionState::Connecting && !connection->GetDtlsEndpoint().IsConnecting())
                    {
                        // Connection is realized once a packet is received and socket handshake is verified complete
                        connection->m_state = ConnectionState::Connected;
                    }
                }
                else if (m_socket->IsEncrypted() && connection->GetDtlsEndpoint().IsConnecting() &&
                    !IsHandshakePacket(connection->GetDtlsEndpoint(), header.GetPacketType()))
                {
                    // It's possible for one side to finish its half of the encryption handshake and start sending encrypted data
                    // This will appear as a SerializationError due to the incomplete encryption handshake
                    // If it's not an expected unencrypted type then skip it for now
                    continue;
                }
                else if (handledPacket == PacketDispatchResult::Skipped)
                {
                    // If the result is marked as skipped then do so (i.e. if a handshake is not yet complete)
                    continue;
                }
                else if (connection->GetConnectionState() != ConnectionState::Disconnecting)
                {
                    connection->Disconnect(DisconnectReason::StreamError, TerminationEndpoint::Local);
                }
            }
        }
        const AZ::TimeMs receiveTimeMs = AZ::GetElapsedTimeMs() - startTimeMs;

        // Time out any stale client connections
        m_connectionTimeoutQueue.UpdateTimeouts([this](TimeoutQueue::TimeoutItem& item) { return HandleConnectionTimeout(item); });

        // Time out any packets that haven't been acked within our timeout window
        m_packetTimeoutQueue.UpdateTimeouts([this](TimeoutQueue::TimeoutItem& item) { return HandlePacketTimeout(item); }, static_cast<int32_t>(net_MaxTimeoutsPerFrame));

        // Delete any connections we've disconnected
        for (RemovedConnection& removedConnection : m_removedConnections)
        {
            m_connectionListener.OnDisconnect(removedConnection.m_connection, removedConnection.m_reason, removedConnection.m_endpoint);
            m_connectionSet.DeleteConnection(removedConnection.m_connection->GetConnectionId()); // Will delete the connection
        }
        m_removedConnections.clear();

        // Update metrics
        GetMetrics().m_sendPackets = m_socket->GetSentPackets();
        GetMetrics().m_sendBytes = m_socket->GetSentBytes();
        GetMetrics().m_sendPacketsEncrypted = m_socket->GetSentPacketsEncrypted();
        GetMetrics().m_sendBytesEncryptionInflation = m_socket->GetSentBytesEncryptionInflation();
        GetMetrics().m_recvTimeMs += receiveTimeMs;
        GetMetrics().m_recvPackets = m_socket->GetRecvPackets();
        GetMetrics().m_recvBytes = m_socket->GetRecvBytes();
        GetMetrics().m_connectionCount = m_connectionSet.GetConnectionCount();
        GetMetrics().m_updateTimeMs += AZ::GetElapsedTimeMs() - startTimeMs;
    }

    bool UdpNetworkInterface::SendReliablePacket(ConnectionId connectionId, const IPacket& packet)
    {
        IConnection* connection = m_connectionSet.GetConnection(connectionId);
        if (connection == nullptr)
        {
            return false;
        }
        return connection->SendReliablePacket(packet);
    }

    PacketId UdpNetworkInterface::SendUnreliablePacket(ConnectionId connectionId, const IPacket& packet)
    {
        IConnection* connection = m_connectionSet.GetConnection(connectionId);
        if (connection == nullptr)
        {
            return InvalidPacketId;
        }
        return connection->SendUnreliablePacket(packet);
    }

    bool UdpNetworkInterface::WasPacketAcked(ConnectionId connectionId, PacketId packetId)
    {
        IConnection* connection = m_connectionSet.GetConnection(connectionId);
        if (connection == nullptr)
        {
            return false;
        }
        return connection->WasPacketAcked(packetId);
    }

    bool UdpNetworkInterface::StopListening()
    {
        if (!m_socket->IsOpen())
        {
            return false;
        }

        m_port = 0;
        m_readerThread.UnregisterSocket(m_socket.get());
        m_allowIncomingConnections = false;
        m_socket->Close();
        return true;
    }

    bool UdpNetworkInterface::Disconnect(ConnectionId connectionId, DisconnectReason reason)
    {
        IConnection* connection = m_connectionSet.GetConnection(connectionId);
        if (connection == nullptr)
        {
            return false;
        }
        return connection->Disconnect(reason, TerminationEndpoint::Local);
    }

    void UdpNetworkInterface::SetTimeoutMs(AZ::TimeMs timeoutMs)
    {
        m_timeoutMs = timeoutMs;
    }

    AZ::TimeMs UdpNetworkInterface::GetTimeoutMs() const
    {
        return m_timeoutMs;
    }

    bool UdpNetworkInterface::IsEncrypted() const
    {
        return m_socket->IsEncrypted();
    }

    bool UdpNetworkInterface::IsOpen() const
    {
        return m_socket->IsOpen();
    }

    void UdpNetworkInterface::RegisterWithTimeoutQueue(ConnectionId connectionId, PacketId packetId, ReliabilityType reliability, const ConnectionMetrics& metrics)
    {
        const float avgRtt = metrics.m_connectionRtt.GetRoundTripTimeSeconds(); // Time is in seconds, timeout times are in milliseconds
        const AZ::TimeMs expectedTimeoutMs = aznumeric_cast<AZ::TimeMs>(aznumeric_cast<int64_t>(avgRtt * 1000.0f * net_RttFudgeScalar));
        const AZ::TimeMs packetTimeoutMs = AZStd::max<AZ::TimeMs>(expectedTimeoutMs, net_MinPacketTimeoutMs); // Consider packets lost after twice the current connection Rtt
        AZLOG(NET_Debug, "Registering packetId %u with timeout %u", aznumeric_cast<uint32_t>(packetId), aznumeric_cast<uint32_t>(packetTimeoutMs));
        m_packetTimeoutQueue.RegisterItem(ConstructTimeoutId(connectionId, packetId, reliability), packetTimeoutMs);
    }

    bool UdpNetworkInterface::DecompressPacket(const uint8_t* packetBuffer, size_t packetSize, UdpPacketEncodingBuffer& packetBufferOut) const
    {
        if (!m_compressor) // should probably have some compression handshake than relying on existence of compressor
        {
            AZLOG_ERROR("Decompress called without a compressor.");
            return false;
        }

        AZStd::size_t uncompSize = 0;
        AZStd::size_t bytesConsumed = 0;

        packetBufferOut.Resize(packetBufferOut.GetCapacity());
        const CompressorError compErr = m_compressor->Decompress(packetBuffer, packetSize, packetBufferOut.GetBuffer(), packetBufferOut.GetCapacity(), bytesConsumed, uncompSize);
        packetBufferOut.Resize(aznumeric_cast<uint32_t>(uncompSize)); // Decompress will fail if larger than buffer size, so this cast is safe

        if (compErr != CompressorError::Ok)
        {
            AZLOG_ERROR("Decompress failed with error %d this will lead to data read errors!", aznumeric_cast<int32_t>(compErr));
            return false;
        }

        if (packetSize != bytesConsumed)
        {
            AZLOG_ERROR("Decompress must consume entire buffer [%zu != %zu]!", bytesConsumed, packetSize);
            return false;
        }
        return true;
    }

    PacketId UdpNetworkInterface::SendPacket(UdpConnection& connection, const IPacket& packet, SequenceId reliableSequence)
    {
        AZLOG(NET_DebugPacketSend, "Sending packet type %u to remote address %s", aznumeric_cast<uint32_t>(packet.GetPacketType()), connection.GetRemoteAddress().GetString().c_str());

        // The ordering inside this function is incredibly important and fragile
        const IpAddress& address = connection.GetRemoteAddress();
        // We don't want to compress the initial InitiateConnectionPacket, ConnectionHandshakePackets or FragmentedPackets of those two
        const bool shouldCompress = packet.GetPacketType() != aznumeric_cast<PacketType>(CorePackets::PacketType::InitiateConnectionPacket);

        if (address.GetAddress(ByteOrder::Host) == 0)
        {
            return InvalidPacketId;
        }

        const ReliabilityType reliabilityType = (reliableSequence == InvalidSequenceId) ? ReliabilityType::Unreliable : ReliabilityType::Reliable;

        // Check if we need to fragment this packet first
        // We don't ack aggregate packets that get fragmented, so we want to get this chunk out of the way before
        // we start throwing PacketId's and SequenceId's into our other tracking data structures below
        UdpPacketHeader header(connection.GetPacketTracker(), packet.GetPacketType(), reliableSequence);
        const PacketId localPacketId = header.GetPacketId();

        // If it's a reliable packet, make sure our reliable queue knows about it now because we might need to drop it if our connection is
        // not set up
        if (reliabilityType == ReliabilityType::Reliable)
        {
            if (!connection.PrepareReliablePacketForSend(localPacketId, reliableSequence, packet))
            {
                connection.Disconnect(DisconnectReason::ReliableQueueFull, TerminationEndpoint::Local);
            }
        }

        // If we're still connecting, only transmit packets related to establishing connection and queue the rest for later
        // This implicitly enforces that the only FragmentedPackets sent here are of ConnectionHandshakePacket
        // Other large packets are simply queued before they are fragmented
        if (connection.GetDtlsEndpoint().IsConnecting() && !IsHandshakePacket(connection.GetDtlsEndpoint(), packet.GetPacketType()))
        {
            // IMPORTANT that we register with the timeout queue here, otherwise we don't have the timer to pop for reliable packets
            RegisterWithTimeoutQueue(connection.GetConnectionId(), localPacketId, reliabilityType, connection.GetMetrics());
            AZLOG(
                NET_DebugDtls, "Connection is still in handshake negotiation, blocking packet send for packet type %d",
                (int)packet.GetPacketType());
            return localPacketId;
        }

        UdpPacketEncodingBuffer buffer;
        {
            buffer.Resize(buffer.GetCapacity());

            NetworkInputSerializer networkSerializer(buffer.GetBuffer(), static_cast<uint32_t>(buffer.GetCapacity()));
            ISerializer& serializer = networkSerializer; // To get the default typeinfo parameters in ISerializer

            if (!header.SerializePacketFlags(serializer))
            {
                AZLOG_ERROR("PacketId %u failed flag serialization and will not be sent", aznumeric_cast<uint32_t>(localPacketId));
                return InvalidPacketId;
            }

            if (!serializer.Serialize(header, "Header"))
            {
                AZLOG_ERROR("PacketId %u failed header serialization and will not be sent", aznumeric_cast<uint32_t>(localPacketId));
                return InvalidPacketId;
            }

            if (!serializer.Serialize(const_cast<IPacket&>(packet), "Payload"))
            {
                AZLOG_ERROR("PacketId %u failed payload serialization and will not be sent", aznumeric_cast<uint32_t>(localPacketId));
                return InvalidPacketId;
            }

            buffer.Resize(serializer.GetSize());
        }
        uint32_t packetSize = static_cast<uint32_t>(buffer.GetSize());
        uint8_t* packetData = buffer.GetBuffer();

        // If the packet doesn't fit within our MTU (minus potential SSL encryption overhead), break it up
        if (packetSize > connection.GetConnectionMtu() - net_SslInflationOverhead)
        {
            // Each fragmented packet we send adds an extra fragmented packet header, need to deduct that from our chunk size, otherwise we infinitely loop
            // SSL encryption can also inflate our payload so we pre-emptively deduct an estimated tax
            const uint32_t chunkSize = connection.GetConnectionMtu() - net_FragmentedHeaderOverhead - net_SslInflationOverhead;
            const uint32_t numChunks = AZ::DivideAndRoundUp(packetSize, chunkSize); // We want to round up on the remainder
            const uint8_t* chunkStart = packetData;
            const SequenceId fragmentedSequence = connection.m_fragmentQueue.GetNextFragmentedSequenceId();
            uint32_t bytesRemaining = packetSize;
            ChunkBuffer chunkBuffer;
            for (uint32_t chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex)
            {
                const uint32_t nextChunkSize = AZStd::min(bytesRemaining, chunkSize);
                chunkBuffer.CopyValues(chunkStart, nextChunkSize);
                CorePackets::FragmentedPacket fragmentedPacket(ToSequenceId(localPacketId), fragmentedSequence, aznumeric_cast<uint8_t>(chunkIndex), aznumeric_cast<uint8_t>(numChunks), chunkBuffer);
                const SequenceId chunkReliableId = (net_FragmentsAlwaysReliable || reliabilityType == ReliabilityType::Reliable)
                    ? connection.m_reliableQueue.GetNextSequenceId()
                    : InvalidSequenceId;
                SendPacket(connection, fragmentedPacket, chunkReliableId);
                bytesRemaining -= nextChunkSize;
                chunkStart += nextChunkSize;
            }
            AZ_Assert(bytesRemaining == 0, "Non-zero bytes remaining (%u) after chunking a packet into fragments", bytesRemaining);

            return localPacketId;
        }

        UdpPacketEncodingBuffer writeBuffer;
        if (m_compressor && shouldCompress)
        {
            NetworkInputSerializer flagSerializer(writeBuffer.GetBuffer(), static_cast<uint32_t>(writeBuffer.GetCapacity()));
            ISerializer& serializer = flagSerializer; // To get the default typeinfo parameters in ISerializer

            header.SetPacketFlag(PacketFlag::Compressed, true);
            if (!header.SerializePacketFlags(serializer))
            {
                AZLOG_ERROR("PacketId %u failed flag serialization for compression and will not be sent", aznumeric_cast<uint32_t>(localPacketId));
                return InvalidPacketId;
            }
            uint32_t flagSize = flagSerializer.GetSize();
            AZ_Assert(flagSize == 1, "Flag bitfield should serialize to one byte");

            // Compress the packet, make sure to offset by the size of the flag which is now serialized
            const uint32_t payloadSize = static_cast<uint32_t>(buffer.GetSize() - flagSize);
            uint8_t* payload = buffer.GetBuffer() + flagSize;
            const AZStd::size_t maxSizeNeeded = m_compressor->GetMaxCompressedBufferSize(payloadSize);
            AZStd::size_t compressionMemBytesUsed = 0;
            CompressorError compErr = m_compressor->Compress(payload, payloadSize, writeBuffer.GetBuffer() + flagSize, maxSizeNeeded, compressionMemBytesUsed);

            if (compErr != CompressorError::Ok)
            {
                AZLOG_ERROR("Failed to compress packet with error %d", aznumeric_cast<int32_t>(compErr));
                return InvalidPacketId;
            }

            // Only use compression if there's actual gain
            if (compressionMemBytesUsed < payloadSize)
            {
                writeBuffer.Resize(aznumeric_cast<int32_t>(flagSize + compressionMemBytesUsed));
                packetSize = static_cast<uint32_t>(writeBuffer.GetSize());
                packetData = writeBuffer.GetBuffer();
                // Track byte delta caused by compression
                GetMetrics().m_sendBytesCompressedDelta += (packetSize - compressionMemBytesUsed);
            }        
        }

        AZLOG(NET_Debug, "Sending local sequence id %d, remote sequence id %d, %s, reliable id: %d, ack vector %x",
            aznumeric_cast<int32_t>(header.GetLocalSequenceId()),
            aznumeric_cast<int32_t>(header.GetRemoteSequenceId()),
            header.GetIsReliable() ? "reliable" : "unreliable",
            aznumeric_cast<int32_t>(header.GetReliableSequenceId()),
            aznumeric_cast<uint32_t>(header.GetSequenceWindow())
        );

        AZLOG(NET_DebugDtls, "Connection is sending packet type %d", aznumeric_cast<int32_t>(packet.GetPacketType()));
        // If we're not connected then we're still handshaking and require packets to be unencrypted
        const bool shouldEncrypt = !IsHandshakePacket(connection.GetDtlsEndpoint(), packet.GetPacketType());
        if (m_socket->Send(address, packetData, packetSize, shouldEncrypt, connection.GetDtlsEndpoint(), connection.GetConnectionQuality()))
        {
            RegisterWithTimeoutQueue(connection.GetConnectionId(), localPacketId, reliabilityType, connection.GetMetrics());
            connection.ProcessSent(localPacketId, packet, packetSize + UdpPacketHeaderSize, reliabilityType);
            GetMetrics().m_sendBytesUncompressed += buffer.GetSize() + UdpPacketHeaderSize + (shouldEncrypt ? DtlsPacketHeaderSize : 0);
            return localPacketId;
        }
        else
        {
            AZLOG_ERROR("PacketId %u failed to send on the socket", aznumeric_cast<uint32_t>(localPacketId));
        }

        return InvalidPacketId;
    }

    void UdpNetworkInterface::AcceptConnection(const UdpReaderThread::ReceivedPacket& connectPacket)
    {
        if (!m_allowIncomingConnections)
        {
            // This network interface is not set to allow incoming connections
            return;
        }

        CorePackets::InitiateConnectionPacket packet;
        {
            NetworkOutputSerializer networkSerializer(connectPacket.m_buffer, connectPacket.m_receivedBytes);

            // First, serialize out the header
            UdpPacketHeader header;
            if (!header.SerializePacketFlags(networkSerializer))
            {
                return;
            }

            if (!static_cast<ISerializer&>(networkSerializer).Serialize(header, "Header"))
            {
                return;
            }

            // Validate that this is really an InitiateConnection packet
            if (header.GetPacketType() != aznumeric_cast<PacketType>(CorePackets::PacketType::InitiateConnectionPacket))
            {
                return;
            }

            // Next serialize the InitiateConnectionPacket itself
            {
                NetworkOutputSerializer tempPacketSerializer(networkSerializer.GetUnreadData(), networkSerializer.GetUnreadSize());
                if (!static_cast<ISerializer&>(tempPacketSerializer).Serialize(packet, "Packet"))
                {
                    return;
                }
            }

            // Retrieve the connection type, and run application layer connection filtering (state checks, CIDR address filtering, etc..)
            const ConnectResult connectResult = m_connectionListener.ValidateConnect(connectPacket.m_address, header, networkSerializer);

            switch (connectResult)
            {
            case ConnectResult::Rejected:
                return; // Failed validation, simply discard the connect message
            case ConnectResult::Accepted:
                break; // This is not actually an expected return from this code path, assume it's a client connection
            }
        }
        // We've passed all our security checks, so now we're free to allocate memory and track the new connection

        // How long should we sit in the timeout queue before heartbeating or disconnecting
        const ConnectionId connectionId = m_connectionSet.GetNextConnectionId();
        const TimeoutId    timeoutId = m_connectionTimeoutQueue.RegisterItem(aznumeric_cast<uint64_t>(connectionId), m_timeoutMs);

        AZLOG(Debug_UdpConnect, "Accepted new Udp Connection");
        AZStd::unique_ptr<UdpConnection> connection = AZStd::make_unique<UdpConnection>(connectionId, connectPacket.m_address, *this, ConnectionRole::Acceptor);
        DtlsEndpoint::ConnectResult result = m_socket->AcceptDtlsEndpoint(connection->GetDtlsEndpoint(), connectPacket.m_address);

        // Transition state based on our how our socket resolved
        connection->m_state = result == DtlsEndpoint::ConnectResult::Complete ? ConnectionState::Connected : ConnectionState::Connecting;
        connection->SetTimeoutId(timeoutId);
        m_connectionListener.OnConnect(connection.get());
        m_connectionSet.AddConnection(AZStd::move(connection));
    }

    void UdpNetworkInterface::RequestDisconnect(UdpConnection* connection, DisconnectReason reason, TerminationEndpoint endpoint)
    {
        if (connection == nullptr)
        {
            return;
        }

        connection->m_state = ConnectionState::Disconnecting;
        m_removedConnections.emplace_back(RemovedConnection{ connection, reason, endpoint });
    }

    bool UdpNetworkInterface::IsHandshakePacket(const DtlsEndpoint& endpoint, PacketType packetType) const
    {
        // Packets involved in handshake are InitiateConnection, ConnectionHandshake and FragmentedPackets of ConnectionHandshake
        return packetType == aznumeric_cast<PacketType>(CorePackets::PacketType::InitiateConnectionPacket) ||
               packetType == aznumeric_cast<PacketType>(CorePackets::PacketType::ConnectionHandshakePacket) ||
              (packetType == aznumeric_cast<PacketType>(CorePackets::PacketType::FragmentedPacket) && endpoint.IsConnecting());
    }

    TimeoutResult UdpNetworkInterface::HandleConnectionTimeout(TimeoutQueue::TimeoutItem& item)
    {
        const ConnectionId connectionId = ConnectionId(aznumeric_cast<uint32_t>(item.m_userData));
        UdpConnection* udpConnection = static_cast<UdpConnection*>(m_connectionSet.GetConnection(connectionId));

        if (udpConnection == nullptr)
        {
            // We've already deleted this connection
            return TimeoutResult::Delete;
        }

        if ((udpConnection->GetConnectionState() == ConnectionState::Connecting)
         && udpConnection->GetDtlsEndpoint().IsConnecting())
        {
            // DTLS prefers we resend data lost over the wire with fresh SSL IDs so account for that here
            UdpPacketEncodingBuffer dtlsData;
            udpConnection->ProcessHandshakeData(dtlsData);
            return TimeoutResult::Refresh;
        }

        if ((udpConnection->GetConnectionRole() == ConnectionRole::Connector)
         && (udpConnection->m_timeoutCounter < net_UdpUnackedHeartbeats))
        {
            // Set the request response flag to true since we want a response to keep the connection alive
            udpConnection->SendUnreliablePacket(CorePackets::HeartbeatPacket(true));
            ++udpConnection->m_timeoutCounter;
        }
        else if (net_UdpTimeoutConnections && (GetTimeoutMs() > AZ::Time::ZeroTimeMs))
        {
            udpConnection->Disconnect(DisconnectReason::Timeout, TerminationEndpoint::Local);
            return TimeoutResult::Delete;
        }

        return TimeoutResult::Refresh;
    }

    TimeoutResult UdpNetworkInterface::HandlePacketTimeout(TimeoutQueue::TimeoutItem& item)
    {
        ConnectionId    connectionId;
        PacketId        packetId;
        ReliabilityType reliability;
        DecodeTimeoutId(item.m_userData, connectionId, packetId, reliability);
        UdpConnection* connection = static_cast<UdpConnection*>(m_connectionSet.GetConnection(connectionId));

        if (connection == nullptr)
        {
            AZLOG(NET_Debug, "Failed to look up connection for timed out packetId %u", aznumeric_cast<uint32_t>(packetId));
            return TimeoutResult::Delete;
        }

        const PacketTimeoutResult result = connection->ProcessTimeout(packetId, reliability);
        AZLOG(NET_Debug, "Timeout triggered for packetId %u with result %s", aznumeric_cast<uint32_t>(packetId), GetEnumString(result));
        switch (result)
        {
        case PacketTimeoutResult::Acked:
            // Packet was already acked, just discard this timeout entry
            return TimeoutResult::Delete;
        case PacketTimeoutResult::Pending:
            // Packet timed out before we received any info about it's sequence from the remote endpoint
            // The connection latency may have increased, and our Rtt metrics may still be adjusting..
            // Just throw it back into the timeout queue
            return TimeoutResult::Refresh;
        case PacketTimeoutResult::Lost:
            // Packet timed out and was not acked, so we consider it lost
            m_connectionListener.OnPacketLost(connection, packetId);
            break;
        }

        return TimeoutResult::Delete;
    }

    AZStd::atomic<AZ::TimeMs> UdpNetworkInterface::GetLastSystemTickUpdate() const
    {
        return m_lastSystemTickUpdate.load();
    }
}
