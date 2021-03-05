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

#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/PacketLayer/IPacket.h>
#include <AzNetworking/PacketLayer/IPacketHeader.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/ConnectionLayer/ConnectionEnums.h>

namespace AzNetworking
{
    //! @class IConnectionListener
    //! @brief interface class for application layer dealing with connection level events.
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
        //! @return boolean true to signal success, false to disconnect with a transport error
        virtual bool OnPacketReceived(IConnection* connection, const IPacketHeader& packetHeader, ISerializer& serializer) = 0;

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
