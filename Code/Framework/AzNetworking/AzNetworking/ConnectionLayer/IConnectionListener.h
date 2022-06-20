/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/PacketLayer/IPacket.h>
#include <AzNetworking/PacketLayer/IPacketHeader.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/ConnectionLayer/ConnectionEnums.h>

namespace AzNetworking
{
    //! @class IConnectionListener
    //! @brief interface class for application layer dealing with connection level events.
    //!
    //! IConnectionListener defines an abstract interface that the user of AzNetworking is expected to implement to react and
    //! handle all IConnection related events, including the handling of any received IPacket derived packets.  The AzNetworking
    //! user should derive a handler class from IConnectionListener, and provide an instance of that handler to any
    //! INetworkInterface the user instantiates.  The lifetime of the IConnectionListener must outlive the lifetime of the
    //! INetworkInterface.
    class IConnectionListener
    {
    public:

        virtual ~IConnectionListener() = default;

        //! Invoked to validate any new incoming connection from a new endpoint.
        //! @param remoteAddress the address of the remote endpoint initiating a connection
        //! @param packetHeader  packet header of the associated payload
        //! @param serializer    serializer instance containing the transmitted payload
        //! @return the result of the application layers validation of the connect message
        virtual ConnectResult ValidateConnect(const IpAddress& remoteAddress, const IPacketHeader& packetHeader, ISerializer& serializer) = 0;

        //! Invoked when a new connection is successfully established.
        //! @param connection pointer to the new connection instance
        virtual void OnConnect(IConnection* connection) = 0;

        //! Called on receipt of a packet from a connected connection.
        //! @param connection   pointer to the connection instance generating the event
        //! @param packetHeader packet header of the associated payload
        //! @param serializer   serializer instance containing the transmitted payload
        //! @return PacketDispatchResult result of the packet handling attempt
        virtual PacketDispatchResult OnPacketReceived(IConnection* connection, const IPacketHeader& packetHeader, ISerializer& serializer) = 0;

        //! Called when a packet is deemed lost by the remote connection.
        //! @param connection pointer to the connection instance generating the event
        //! @param packetId   identifier of the lost packet
        virtual void OnPacketLost(IConnection* connection, PacketId packetId) = 0;

        //! Called on disconnection from an connection.
        //! @param connection pointer to the connection instance generating the event
        //! @param reason     reason for the disconnect
        //! @param endpoint   whether the disconnection was initiated locally or remotely
        virtual void OnDisconnect(IConnection* connection, DisconnectReason reason, TerminationEndpoint endpoint) = 0;

    };
}
