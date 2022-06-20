/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Framework/INetworking.h>
#include <AzNetworking/TcpTransport/TcpConnection.h>
#include <AzNetworking/TcpTransport/TcpPacketHeader.h>
#include <AzNetworking/TcpTransport/TcpNetworkInterface.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/ConnectionLayer/IConnectionSet.h>
#include <AzNetworking/Framework/ICompressor.h>
#include <AzNetworking/AutoGen/CorePackets.AutoPackets.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    AZ_CVAR(AZ::CVarFixedString, net_TcpCompressor, "MultiplayerCompressor", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "TCP compressor to use."); // WARN: similar to encryption this needs to be set once and only once before creating the network interface

    TcpConnection::TcpConnection
    (
        ConnectionId connectionId,
        const IpAddress& remoteAddress,
        TcpNetworkInterface& networkInterface,
        TcpSocket& socket
    )
        : IConnection(connectionId, remoteAddress)
        , m_networkInterface(networkInterface)
        , m_socket(socket.CloneAndTakeOwnership())
        , m_state(m_socket->IsOpen() ? ConnectionState::Connecting : ConnectionState::Disconnected)
        , m_connectionRole(ConnectionRole::Acceptor)
        , m_registeredSocketFd(InvalidSocketFd)
    {
        ;
    }

    TcpConnection::TcpConnection
    (
        ConnectionId connectionId,
        const IpAddress& remoteAddress,
        TcpNetworkInterface& networkInterface,
        TrustZone trustZone,
        bool useEncryption
    )
        : IConnection(connectionId, remoteAddress)
        , m_networkInterface(networkInterface)
        , m_socket(nullptr)
        , m_state(ConnectionState::Disconnected)
        , m_connectionRole(ConnectionRole::Connector)
        , m_registeredSocketFd(InvalidSocketFd)
    {
        const AZ::CVarFixedString compressor = static_cast<AZ::CVarFixedString>(net_TcpCompressor);
        m_compressor = AZ::Interface<INetworking>::Get()->CreateCompressor(compressor);

        if (useEncryption)
        {
            m_socket = AZStd::make_unique<TlsSocket>(trustZone);
        }
        else
        {
            m_socket = AZStd::make_unique<TcpSocket>();
        }
    }

    TcpConnection::~TcpConnection()
    {
        if (m_state == ConnectionState::Connected)
        {
            m_networkInterface.GetConnectionListener().OnDisconnect(this, DisconnectReason::ConnectionDeleted, TerminationEndpoint::Local);
        }
    }

    bool TcpConnection::Connect()
    {
        Disconnect(DisconnectReason::TerminatedByClient, TerminationEndpoint::Local);
        if (!m_socket->Connect(GetRemoteAddress()))
        {
            m_networkInterface.GetConnectionListener().OnDisconnect(this, DisconnectReason::ConnectionRejected, TerminationEndpoint::Local);
            return false;
        }
        m_state = ConnectionState::Connecting;
        SendReliablePacket(CorePackets::InitiateConnectionPacket());
        return true;
    }

    void TcpConnection::UpdateSend()
    {
        const uint32_t numSendBytes = m_sendRingbuffer.GetReadBufferSize();
        if (numSendBytes <= 0)
        {
            return;
        }

        uint8_t* sendData = m_sendRingbuffer.GetReadBufferData();
        const int32_t sentBytes = m_socket->Send(sendData, numSendBytes);
        const DisconnectReason disconnectReason = GetDisconnectReasonForSocketResult(sentBytes);
        if (disconnectReason != DisconnectReason::MAX)
        {
            Disconnect(disconnectReason, TerminationEndpoint::Remote);
            return;
        }

        m_sendRingbuffer.AdvanceReadBuffer(sentBytes);
        m_networkInterface.GetMetrics().m_sendBytes += numSendBytes;
        m_networkInterface.GetMetrics().m_sendBytesUncompressed += numSendBytes;

        if (m_socket->IsEncrypted() && sentBytes > 0)
        {
            m_networkInterface.GetMetrics().m_sendBytesEncryptionInflation += (aznumeric_cast<uint32_t>(sentBytes) - numSendBytes);
            m_networkInterface.GetMetrics().m_sendPacketsEncrypted++;
        }
    }

    bool TcpConnection::UpdateRecv()
    {
        const AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();
        GetMetrics().LogPacketRecv(0, startTimeMs);

        // Read new data off the input socket
        {
            uint8_t* srcData = m_recvRingbuffer.ReserveBlockForWrite(MaxPacketSize);
            if (srcData == nullptr)
            {
                AZLOG_ERROR("Receive ringbuffer full, dropped connection");
                Disconnect(DisconnectReason::StreamError, TerminationEndpoint::Local);
                return false;
            }

            const int32_t receivedBytes = m_socket->Receive(srcData, MaxPacketSize);
            if (receivedBytes == 0)
            {
                // No data on the socket, can happen if we're not in select or epoll mode
                return true;
            }

            const DisconnectReason disconnectReason = GetDisconnectReasonForSocketResult(receivedBytes);
            if (disconnectReason != DisconnectReason::MAX)
            {
                Disconnect(disconnectReason, TerminationEndpoint::Remote);
                return true;
            }
            m_recvRingbuffer.AdvanceWriteBuffer(receivedBytes);
            m_networkInterface.GetMetrics().m_recvBytes += receivedBytes;
            m_networkInterface.GetMetrics().m_recvBytesUncompressed += receivedBytes;
        }

        // Process received packets
        for (;;)
        {
            TcpPacketHeader header(PacketType(0), 0);
            TcpPacketEncodingBuffer buffer;

            if (!ReceivePacketInternal(header, buffer, startTimeMs))
            {
                break;
            }

            NetworkOutputSerializer serializer(buffer.GetBuffer(), static_cast<uint32_t>(buffer.GetSize()));
            if (m_state == ConnectionState::Connecting)
            {
                const ConnectResult connectResult = m_networkInterface.GetConnectionListener().ValidateConnect(GetRemoteAddress(), header, serializer);
                if (connectResult == ConnectResult::Rejected)
                {
                    Disconnect(DisconnectReason::ConnectionRejected, TerminationEndpoint::Local);
                }
                else
                {
                    m_state = ConnectionState::Connected;
                }
            }

            if (m_state == ConnectionState::Connected)
            {
                m_networkInterface.GetConnectionListener().OnPacketReceived(this, header, serializer);
            }
        }

        m_networkInterface.GetMetrics().m_recvTimeMs += AZ::GetElapsedTimeMs() - startTimeMs;
        return true;
    }

    bool TcpConnection::SendReliablePacket(const IPacket& packet)
    {
        TcpPacketEncodingBuffer buffer;
        {
            NetworkInputSerializer serializer(buffer.GetBuffer(), static_cast<uint32_t>(buffer.GetCapacity()));
            if (!const_cast<IPacket&>(packet).Serialize(serializer))
            {
                AZ_Assert(false, "SendReliablePacket: Unable to serialize packet [Type: %d]", packet.GetPacketType());
                return false;
            }
            buffer.Resize(serializer.GetSize());
        }

        const AZ::TimeMs currentTimeMs = AZ::GetElapsedTimeMs();
        ++m_lastSentPacketId;
        return SendPacketInternal(packet.GetPacketType(), buffer, currentTimeMs);
    }

    PacketId TcpConnection::SendUnreliablePacket(const IPacket& packet)
    {
        if (SendReliablePacket(packet))
        {
            return m_lastSentPacketId;
        }
        return InvalidPacketId;
    }

    bool TcpConnection::WasPacketAcked(PacketId packetId) const
    {
        // Treat packetId as a sequence value to handle rollover
        // Since this is Tcp, if the packet was sent we implicitly assume it was received
        return !SequenceMoreRecent(packetId, m_lastSentPacketId);
    }

    ConnectionState TcpConnection::GetConnectionState() const
    {
        return m_state;
    }

    ConnectionRole TcpConnection::GetConnectionRole() const
    {
        return m_connectionRole;
    }

    bool TcpConnection::Disconnect(DisconnectReason reason, [[maybe_unused]] TerminationEndpoint endpoint)
    {
        if (m_state == ConnectionState::Disconnected)
        {
            return true;
        }
        m_networkInterface.GetConnectionListener().OnDisconnect(this, reason, endpoint);
        m_networkInterface.RequestDisconnect(this, reason);
        m_state = ConnectionState::Disconnected;
        GetMetrics().Reset();
        return true;
    }

    void TcpConnection::SetConnectionMtu([[maybe_unused]] uint32_t connectionMtu)
    {
        ; // do nothing, unsupported on TCP connections
    }

    uint32_t TcpConnection::GetConnectionMtu() const
    {
        return 0; // do nothing, unsupported on TCP connections
    }

    bool TcpConnection::SendPacketInternal(PacketType packetType, TcpPacketEncodingBuffer& payloadBuffer, AZ::TimeMs currentTimeMs)
    {
        AZ_Assert(payloadBuffer.GetCapacity() < AZStd::numeric_limits<uint16_t>::max(), "Buffer capacity should be representable using 2 bytes or less");
        int32_t payloadSize = aznumeric_cast<int32_t>(payloadBuffer.GetSize());
        bool shouldCompress = m_compressor && packetType != aznumeric_cast<PacketType>(CorePackets::PacketType::InitiateConnectionPacket);

        // Create and serialize header...
        TcpPacketEncodingBuffer headerBuffer;
        {
            TcpPacketHeader header(packetType, aznumeric_cast<uint16_t>(payloadBuffer.GetSize()));
            header.SetPacketFlag(PacketFlag::Compressed, shouldCompress);
            NetworkInputSerializer serializer(headerBuffer.GetBuffer(), static_cast<uint32_t>(headerBuffer.GetCapacity()));
            if (!header.Serialize(serializer))
            {
                return false;
            }
            headerBuffer.Resize(serializer.GetSize());
        }

        const uint16_t headerSize = aznumeric_cast<uint16_t>(headerBuffer.GetSize());
        const uint8_t* srcData = reinterpret_cast<const uint8_t*>(payloadBuffer.GetBuffer());
        uint8_t* dstData = reinterpret_cast<uint8_t*>(m_sendRingbuffer.ReserveBlockForWrite(headerSize + payloadSize));

        if (dstData == nullptr)
        {
            AZLOG_ERROR("Send ringbuffer full, dropped packet");
            return false;
        }

        // Compress send data
        TcpPacketEncodingBuffer writeBuffer;
        if (m_compressor && packetType != aznumeric_cast<PacketType>(CorePackets::PacketType::InitiateConnectionPacket))
        {
            const AZStd::size_t maxSizeNeeded = m_compressor->GetMaxCompressedBufferSize(payloadBuffer.GetSize());
            AZStd::size_t compressionMemBytesUsed = 0;
            CompressorError compErr = m_compressor->Compress(payloadBuffer.GetBuffer(), payloadBuffer.GetSize(), writeBuffer.GetBuffer(), maxSizeNeeded, compressionMemBytesUsed);

            if (compErr != CompressorError::Ok)
            {
                AZLOG_ERROR("Failed to compress packet with error %d", aznumeric_cast<int32_t>(compErr));
                return false;
            }

            if (compressionMemBytesUsed >= payloadSize)
            {
                // Track how many packets are being sent with no compression gain
                m_networkInterface.GetMetrics().m_sendCompressedPacketsNoGain++;
            }
            // Track byte delta caused by compression
            m_networkInterface.GetMetrics().m_sendBytesCompressedDelta += (payloadSize - compressionMemBytesUsed);

            writeBuffer.Resize(aznumeric_cast<int32_t>(compressionMemBytesUsed));
            payloadSize = static_cast<uint32_t>(writeBuffer.GetSize());
            srcData = writeBuffer.GetBuffer();
        }

        // Copy the header data to the ring buffer
        {
            memcpy(dstData, headerBuffer.GetBuffer(), headerSize);
        }

        // Write payload...
        {
            memcpy(dstData + headerSize, srcData, payloadSize);
        }

        m_sendRingbuffer.AdvanceWriteBuffer(headerSize + payloadSize);
        GetMetrics().LogPacketSent(headerSize + payloadSize, currentTimeMs);
        m_networkInterface.GetMetrics().m_sendPackets++;
        UpdateSend();
        return true;
    }

    bool TcpConnection::ReceivePacketInternal(TcpPacketHeader& outHeader, TcpPacketEncodingBuffer& outBuffer, AZ::TimeMs currentTimeMs)
    {
        NetworkOutputSerializer serializer(m_recvRingbuffer.GetReadBufferData(), m_recvRingbuffer.GetReadBufferSize());
        if (!outHeader.Serialize(serializer))
        {
            return false;
        }

        uint16_t packetSize = outHeader.GetPacketSize();
        const uint32_t unreadSize = serializer.GetUnreadSize();
        if (packetSize > unreadSize)
        {
            // We don't have all the data required for this packet yet
            return false;
        }

        if (packetSize > outBuffer.GetCapacity())
        {
            // If we can't fit the packet, do not allow the copy to proceed as that would overwrite invalid memory
            return false;
        }
        outBuffer.Resize(packetSize);

        const uint8_t* srcData = serializer.GetUnreadData();
        if (m_compressor && outHeader.IsPacketFlagSet(PacketFlag::Compressed))
        {
            if (!DecompressPacket(srcData, packetSize, outBuffer))
            {
                AZLOG_WARN("Failed to decompress packet!");
                return false;
            }
            srcData = outBuffer.GetBuffer();
            packetSize = aznumeric_cast<uint16_t>(outBuffer.GetSize());
        }

        uint8_t* dstData = outBuffer.GetBuffer();
        memcpy(dstData, srcData, packetSize);

        m_recvRingbuffer.AdvanceReadBuffer(serializer.GetReadSize() + packetSize);
        GetMetrics().LogPacketRecv(packetSize, currentTimeMs);
        m_networkInterface.GetMetrics().m_recvPackets++;
        return true;
    }

    bool TcpConnection::DecompressPacket(const uint8_t* packetBuffer, AZStd::size_t packetSize, TcpPacketEncodingBuffer& packetBufferOut) const
    {
        if (!m_compressor) // should probably have some compression handshake than relying on existence of compressor
        {
            AZLOG_ERROR("Decompress called without a compressor.");
            return false;
        }

        AZStd::size_t uncompSize = 0;
        AZStd::size_t bytesConsumed = 0;
        const CompressorError compErr = m_compressor->Decompress(packetBuffer, packetSize, packetBufferOut.GetBuffer(), packetBufferOut.GetCapacity(), bytesConsumed, uncompSize);

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

        packetBufferOut.Resize(aznumeric_cast<uint32_t>(uncompSize)); // Decompress will fail if larger than buffer size, so this cast is safe
        return true;
    }
}
