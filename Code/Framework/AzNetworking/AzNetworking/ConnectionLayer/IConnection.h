/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Time/ITime.h>
#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/ConnectionLayer/ConnectionEnums.h>
#include <AzNetworking/ConnectionLayer/ConnectionMetrics.h>

namespace AzNetworking
{
    // Forwards
    class IPacket;

    //! This is a strong typedef for representing a remote host that may be triggering time changes for backward reconciliation.
    AZ_TYPE_SAFE_INTEGRAL(ConnectionId, uint32_t);
    static constexpr ConnectionId InvalidConnectionId = ConnectionId{ 0xFFFFFFFF };

    struct ConnectionQuality
    {
        ConnectionQuality() = default;
        ConnectionQuality(int32_t lossPercentage, AZ::TimeMs latencyMs, AZ::TimeMs varianceMs);

        int32_t m_lossPercentage = 0;
        AZ::TimeMs m_latencyMs = AZ::Time::ZeroTimeMs;
        AZ::TimeMs m_varianceMs = AZ::Time::ZeroTimeMs;
    };

    enum class TrustZone
    {
        ExternalClientToServer // This connection is potentially opened to external and untrusted machines
    ,   InternalServerToServer // This connection is only ever used for trusted server to server communication
    };

    //! @class IConnection
    //! @brief interface class for network connections.
    //!
    //! IConnection provides a pure-virtual interface for all network connection types. IConnections provide access to
    //! a ConnectionMetrics object which provides a variety of metrics on the connection itself such as data rate, RTT and
    //! packet statistics. 

    class IConnection
    {
    public:

        //! Construct with a specific connectionId and remoteAddress.
        //! @param connectionId the connection identifier to use for this connection
        //! @param address      the remote address this connection
        IConnection(ConnectionId connectionId, const IpAddress& address);

        virtual ~IConnection() = default;

        //! A helper function that transmits a packet on this connection reliably.
        //! @param packet packet to transmit
        //! @return boolean true if the packet was transmitted (not an indication of delivery)
        virtual bool SendReliablePacket(const IPacket& packet) = 0;

        //! A helper function that transmits a packet on this connection unreliably.
        //! @param packet packet to transmit
        //! @return the unreliable packet identifier of the transmitted packet
        virtual PacketId SendUnreliablePacket(const IPacket& packet) = 0;

        //! Returns true if the given packet id was confirmed acknowledged by the remote endpoint, false otherwise.
        //! @param packetId the packet id of the packet to confirm acknowledgment of
        //! @return boolean true if the packet is confirmed acknowledged, false if the packet number is out of range, lost, or still pending acknowledgment
        virtual bool WasPacketAcked(PacketId packetId) const = 0;

        //! Retrieves the connection state for this IConnection instance.
        //! @return the current connection state for this IConnection instance
        virtual ConnectionState GetConnectionState() const = 0;

        //! Retrieves the connection role of this connection instance, whether it was initiated or accepted.
        //! @return whether this connection was initiated or accepted
        virtual ConnectionRole GetConnectionRole() const = 0;

        //! Disconnects the connection with the provided termination reason
        //! @param reason   reason for the disconnect
        //! @param endpoint which endpoint initiated the disconnect, local or remote
        //! @return boolean true on success
        virtual bool Disconnect(DisconnectReason reason, TerminationEndpoint endpoint) = 0;

        //! Sets connection maximum transmission unit for this connection.
        //! Currently unsupported on TcpConnections
        //! @param connectionMtu the max transmission unit for this connection
        virtual void SetConnectionMtu(uint32_t connectionMtu) = 0;

        //! Returns the connection maximum transmission unit.
        //! Currently unsupported on TcpConnections
        //! @return the max transmission unit for this connection
        virtual uint32_t GetConnectionMtu() const = 0;

        //! Returns the connection identifier for this connection instance.
        //! @return the connection identifier for this connection instance
        ConnectionId GetConnectionId() const;

        //! Sets connection user data to the provided value.
        //! @param userData the user data value to bind to this connection
        void SetUserData(void* userData);

        //! Retrieves the connection user data bound to this instance.
        //! @return the connection user data bound to this instance
        void* GetUserData() const;

        //! Sets the remote address for this connection instance.
        //! @param address the remote address to use for this connection instance
        void SetRemoteAddress(const IpAddress& address);

        //! Returns the remote address for this connection instance.
        //! @return the remote address for this connection instance
        const IpAddress& GetRemoteAddress() const;

        //! Retrieves connection metric info.
        //! @return reference to the connection metric info
        const ConnectionMetrics& GetMetrics() const;

        //! Retrieves connection metric info, non-const.
        //! @return reference to the connection metric info
        ConnectionMetrics& GetMetrics();

        //! Retrieves debug connection quality settings.
        //! Currently unsupported on TcpConnections
        //! @return connection quality structure for this connection
        const ConnectionQuality& GetConnectionQuality() const;

        //! Retrieves debug connection quality settings, non-const.
        //! Currently unsupported on TcpConnections
        //! @return connection quality structure for this connection
        ConnectionQuality& GetConnectionQuality();

    private:

        // The following data members are here in the interface for performance reasons
        ConnectionId      m_connectionId = InvalidConnectionId;
        IpAddress         m_remoteAddress;
        ConnectionMetrics m_connectionMetrics;
        ConnectionQuality m_connectionQuality;
        void*             m_userData = nullptr;
    };
}

#include <AzNetworking/ConnectionLayer/IConnection.inl>
