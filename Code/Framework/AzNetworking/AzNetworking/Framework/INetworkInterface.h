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

#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzNetworking/Framework/NetworkInterfaceMetrics.h>
#include <AzNetworking/ConnectionLayer/IConnectionSet.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzCore/std/containers/vector.h>

namespace AzNetworking
{
    //! @class INetworkInterface
    //! @brief pure virtual network interface class to abstract client/server and tcp/udp concerns from application code.
    class INetworkInterface
    {
    public:

        AZ_RTTI(INetworkInterface, "{ECDA6FA2-4AA0-435E-881F-214C4B179A31}");

        virtual ~INetworkInterface() = default;

        //! Retrieves the name of this network interface instance.
        //! @return the name of this network interface instance
        virtual AZ::Name GetName() const = 0;

        //! Retrieves the type of this network interface instance.
        //! @return the type of this network interface instance
        virtual ProtocolType GetType() const = 0;

        //! Retrieves the trust zone for this network interface instance.
        //! @return the trust zone for this network interface instance
        virtual TrustZone GetTrustZone() const = 0;

        //! Returns the port number this network interface is bound to.
        //! @return the port number this network interface is bound to
        virtual uint16_t GetPort() const = 0;

        //! Returns the connection set for this network interface.
        //! @return the connection set for this network interface
        virtual IConnectionSet& GetConnectionSet() = 0;

        //! Returns a reference to the connection listener for this network interface.
        //! @return reference to the connection listener for this network interface
        virtual IConnectionListener& GetConnectionListener() = 0;

        //! Opens the network interface to allow it to accept new incoming connections.
        //! @param port the listen port number this network interface will potentially bind to, 0 if it's a don't care
        //! @return boolean true if the operation was successful, false if it failed
        virtual bool Listen(uint16_t port) = 0;

        //! Opens a new connection to the provided address.
        //! @param remoteAddress the IpAddress of the remote process to open a connection to
        //! @return the connectionId of the new connection, or InvalidConnectionId if the operation failed
        virtual ConnectionId Connect(const IpAddress& remoteAddress) = 0;

        //! Updates the INetworkInterface.
        //! @param deltaTimeMs milliseconds since update was last invoked
        virtual void Update(AZ::TimeMs deltaTimeMs) = 0;

        //! A helper function that transmits a packet on this connection reliably.
        //! Note that a packetId is not returned here, since retransmits may cause the packetId to change
        //! @param connectionId identifier of the connection to send to
        //! @param packet       packet to transmit
        //! @return boolean true if the packet was transmitted (not an indication of delivery)
        virtual bool SendReliablePacket(ConnectionId connectionId, const IPacket& packet) = 0;

        //! A helper function that transmits a packet on this connection unreliably.
        //! @param connectionId identifier of the connection to send to
        //! @param packet       packet to transmit
        //! @return the unreliable packet identifier of the transmitted packet
        virtual PacketId SendUnreliablePacket(ConnectionId connectionId, const IPacket& packet) = 0;

        //! Returns true if the given packet id was confirmed acknowledged by the remote endpoint, false otherwise.
        //! @param connectionId identifier of the connection to send to
        //! @param packetId   the packet id of the packet to confirm acknowledgment of
        //! @return boolean true if the packet is confirmed acknowledged, false if the packet number is out of range, lost, or still pending acknowledgment
        virtual bool WasPacketAcked(ConnectionId connectionId, PacketId packetId) = 0;

        //! Disconnects the specified connection.
        //! @param connectionId identifier of the connection to terminate
        //! @param reason      reason for the disconnect
        //! @return boolean true on success
        virtual bool Disconnect(ConnectionId connectionId, DisconnectReason reason) = 0;

        //! Const access to the metrics tracked by this network interface.
        //! @return const reference to the metrics tracked by this network interface
        const NetworkInterfaceMetrics& GetMetrics() const;

        //! Non-const access to the metrics tracked by this network interface.
        //! @return reference to the metrics tracked by this network interface
        NetworkInterfaceMetrics& GetMetrics();

    private:

        NetworkInterfaceMetrics m_metrics;
    };

    inline const NetworkInterfaceMetrics& INetworkInterface::GetMetrics() const
    {
        return m_metrics;
    }

    inline NetworkInterfaceMetrics& INetworkInterface::GetMetrics()
    {
        return m_metrics;
    }
}
