/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/UdpConnection.h>
#include <AzNetworking/UdpTransport/UdpPacketTracker.h>
#include <AzNetworking/UdpTransport/UdpNetworkInterface.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/Serialization/TrackChangedSerializer.h>
#include <AzNetworking/AutoGen/CorePackets.AutoPackets.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    AZ_CVAR(uint32_t, net_UdpMaxUnackedPacketCount, 10, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Maximum packets to receive before forcing a heartbeat packet for acking");

    // Track every 8th packet to determine Rtt
    // Only reason we're doing every 8th packet instead of every packet is to reduce per-packet overhead
    static const uint32_t PacketRttMask = 0x07;
    static_assert(AZ::IsPowerOfTwo(PacketRttMask + 1), "Sequence mask should be of the form 2^N - 1");

    static bool IncludePacketInRtt(PacketId packetId)
    {
        return ((static_cast<uint32_t>(packetId) & PacketRttMask) == 0);
    }

    const char* GetEnumString(PacketTimeoutResult value)
    {
        switch (value)
        {
        case PacketTimeoutResult::Acked:
            return "PacketTimeoutResult::Acked";
        case PacketTimeoutResult::Lost:
            return "PacketTimeoutResult::Lost";
        case PacketTimeoutResult::Pending:
            return "PacketTimeoutResult::Pending";
        }
        return "INVALID";
    }

    UdpConnection::UdpConnection(ConnectionId connectionId, const IpAddress& remoteAddress, UdpNetworkInterface& networkInterface, ConnectionRole connectionRole)
        : IConnection(connectionId, remoteAddress)
        , m_networkInterface(networkInterface)
        , m_lastSentPacketMs(AZ::GetElapsedTimeMs())
        , m_connectionRole(connectionRole)
    {
        ;
    }

    UdpConnection::~UdpConnection()
    {
        if (m_state == ConnectionState::Connected)
        {
            m_state = ConnectionState::Disconnecting;
            m_networkInterface.GetConnectionListener().OnDisconnect(this, DisconnectReason::ConnectionDeleted, TerminationEndpoint::Local);
            m_state = ConnectionState::Disconnected;
        }
    }

    DtlsEndpoint::ConnectResult UdpConnection::ProcessHandshakeData(const UdpPacketEncodingBuffer& dtlsData)
    {
        const DtlsEndpoint::ConnectResult result = m_dtlsEndpoint.ProcessHandshakeData(*this, dtlsData);
        if (result == DtlsEndpoint::ConnectResult::Failed)
        {
            Disconnect(DisconnectReason::NetworkError, TerminationEndpoint::Local);
        }
        return result;
    }

    void UdpConnection::UpdateHeartbeat([[maybe_unused]] AZ::TimeMs currentTimeMs)
    {
        if (m_unackedPacketCount >= net_UdpMaxUnackedPacketCount)
        {
            AZLOG(NET_Acks, "Unacked packet count exceeded, sending client heartbeat (curr %u : max %u)", m_unackedPacketCount, static_cast<uint32_t>(net_UdpMaxUnackedPacketCount));
            // This simply times out unreliable chunks that haven't completed within our timeout delay
            m_fragmentQueue.Update();
            // This heartbeat is sent to minimize the time the remote endpoint spends waiting for ack vector replication, we don't require a response
            SendUnreliablePacket(CorePackets::HeartbeatPacket(false));
        }
    }

    bool UdpConnection::SendReliablePacket(const IPacket& packet)
    {
        AZStd::lock_guard lock(m_sendPacketMutex);
        const SequenceId reliableSequenceId = m_reliableQueue.GetNextSequenceId();
        return (m_networkInterface.SendPacket(*this, packet, reliableSequenceId) != InvalidPacketId);
    }

    PacketId UdpConnection::SendUnreliablePacket(const IPacket& packet)
    {
        AZStd::lock_guard lock(m_sendPacketMutex);
        return m_networkInterface.SendPacket(*this, packet, InvalidSequenceId);
    }

    bool UdpConnection::WasPacketAcked(PacketId packetId) const
    {
        return m_packetTracker.GetPacketAckStatus(packetId) == PacketAckState::Acked;
    }

    ConnectionState UdpConnection::GetConnectionState() const
    {
        return m_state;
    }

    ConnectionRole UdpConnection::GetConnectionRole() const
    {
        return m_connectionRole;
    }

    bool UdpConnection::Disconnect(DisconnectReason reason, TerminationEndpoint endpoint)
    {
        if (m_state == ConnectionState::Disconnected)
        {
            return true;
        }
        if (m_state == ConnectionState::Disconnecting)
        {
            AZLOG_WARN("Disconnecting an already disconnecting connection due to %s", ToString(reason).data());
            return false;
        }
        m_state = ConnectionState::Disconnecting;

        if (endpoint == TerminationEndpoint::Local
           && reason != DisconnectReason::NetworkError
           && reason != DisconnectReason::DtlsHandshakeError
           && reason != DisconnectReason::Unknown
           && reason != DisconnectReason::RemoteHostClosedConnection
           && reason != DisconnectReason::TransportError
           && reason != DisconnectReason::SslFailure)
        {
            // If disconnect initiated from Local, inform the remote endpoint
            CorePackets::TerminateConnectionPacket terminationPacket(reason);
            SendUnreliablePacket(terminationPacket);
        }

        m_networkInterface.RequestDisconnect(this, reason, endpoint);
        return true;
    }

    void UdpConnection::SetConnectionMtu(uint32_t connectionMtu)
    {
        m_connectionMtu = connectionMtu;
    }

    uint32_t UdpConnection::GetConnectionMtu() const
    {
        return m_connectionMtu;
    }

    void UdpConnection::ProcessAcked(PacketId packetId, AZ::TimeMs currentTimeMs)
    {
        GetMetrics().LogPacketAcked();
        m_reliableQueue.OnPacketAcked(m_networkInterface, *this, packetId);

        // Compute Rtt adjustments
        if (IncludePacketInRtt(packetId))
        {
            GetMetrics().m_connectionRtt.LogPacketAcked(packetId, currentTimeMs);
        }
    }

    void UdpConnection::ProcessSent(PacketId packetId, [[maybe_unused]] const IPacket& packet, 
        uint32_t packetSize, [[maybe_unused]] ReliabilityType reliability)
    {
        const AZ::TimeMs currentTimeMs = AZ::GetElapsedTimeMs();

        if (IncludePacketInRtt(packetId))
        {
            GetMetrics().m_connectionRtt.LogPacketSent(packetId, currentTimeMs);
        }

        GetMetrics().LogPacketSent(packetSize, currentTimeMs);
        m_lastSentPacketMs = currentTimeMs;
        m_unackedPacketCount = 0;
    }

    PacketTimeoutResult UdpConnection::ProcessTimeout(PacketId packetId, ReliabilityType reliability)
    {
        if (IncludePacketInRtt(packetId))
        {
            GetMetrics().m_connectionRtt.LogPacketTimeout(packetId);
        }

        const PacketAckState ackState = m_packetTracker.GetPacketAckStatus(packetId);

        switch (ackState)
        {
        case PacketAckState::Acked:
            return PacketTimeoutResult::Acked;

        case PacketAckState::Nacked:
            GetMetrics().LogPacketLost();
            if (reliability == ReliabilityType::Reliable)
            {
                m_reliableQueue.OnPacketLost(m_networkInterface, *this, packetId);
            }
            return PacketTimeoutResult::Lost;

        case PacketAckState::Unknown_TooNew:
            return PacketTimeoutResult::Pending;

        case PacketAckState::Unknown_TooOld:
            // TODO: Disconnect?
            AZLOG_ERROR("PacketId %u timeout fell outside the ack history window", static_cast<uint32_t>(packetId));
            break;

        default:
            AZLOG_ERROR("PacketId %u ack state was unhandled (%s)", static_cast<uint32_t>(packetId), GetEnumString(ackState));
            break;
        }

        return PacketTimeoutResult::Lost;
    }

    bool UdpConnection::ProcessReceived(UdpPacketHeader& header, [[maybe_unused]] const NetworkOutputSerializer& serializer, 
        uint32_t packetSize, AZ::TimeMs currentTimeMs)
    {
        if (!m_packetTracker.ProcessReceived(this, header))
        {
            return false;
        }

        GetMetrics().LogPacketRecv(packetSize, currentTimeMs);

        if (header.GetIsReliable() && !m_reliableQueue.OnPacketReceived(header))
        {
            return false;
        }

        m_unackedPacketCount++;
        UpdateHeartbeat(currentTimeMs);

        return true;
    }

    PacketDispatchResult UdpConnection::HandleCorePacket(IConnectionListener& connectionListener, UdpPacketHeader& header, ISerializer& serializer)
    {
        switch (static_cast<CorePackets::PacketType>(header.GetPacketType()))
        {
        case CorePackets::PacketType::InitiateConnectionPacket:
        {
            AZLOG(NET_CorePackets, "Received core packet %s", "InitiateConnection");
            return PacketDispatchResult::Success;
        }
        break;

        case CorePackets::PacketType::ConnectionHandshakePacket:
        {
            AZLOG(NET_CorePackets, "Received core packet %s", "ConnectionHandshakePacket");
            CorePackets::ConnectionHandshakePacket packet;
            if (!serializer.Serialize(packet, "Packet"))
            {
                return PacketDispatchResult::Failure;
            }

            if (m_state != ConnectionState::Connected)
            {
                if (ProcessHandshakeData(packet.GetHandshakeBuffer()) == DtlsEndpoint::ConnectResult::Complete)
                {
                    m_state = ConnectionState::Connected;
                }
            }

            return PacketDispatchResult::Success;
        }
        break;

        case CorePackets::PacketType::TerminateConnectionPacket:
        {
            AZLOG(NET_CorePackets, "Received core packet %s", "TerminateConnection");
            CorePackets::TerminateConnectionPacket packet;
            if (!serializer.Serialize(packet, "Packet"))
            {
                return PacketDispatchResult::Failure;
            }
            Disconnect(packet.GetDisconnectReason(), TerminationEndpoint::Remote);
            return PacketDispatchResult::Success;
        }
        break;

        case CorePackets::PacketType::HeartbeatPacket:
        {
            AZLOG(NET_CorePackets, "Received core packet %s", "Heartbeat");
            CorePackets::HeartbeatPacket packet;
            if (!serializer.Serialize(packet, "Packet"))
            {
                return PacketDispatchResult::Failure;
            }
            if (packet.GetRequestResponse())
            {
                // We're replying to a heartbeat request, we don't want a response
                SendUnreliablePacket(CorePackets::HeartbeatPacket(false));
            }
            return PacketDispatchResult::Success;
        }
        break;

        case CorePackets::PacketType::FragmentedPacket:
            AZLOG(NET_CorePackets, "Received core packet %s", "Fragment");
            return m_fragmentQueue.ProcessReceivedChunk(this, connectionListener, header, serializer);

        default:
            AZ_Assert(false, "Unhandled core packet type!");
        }

        return PacketDispatchResult::Failure;
    }
}
