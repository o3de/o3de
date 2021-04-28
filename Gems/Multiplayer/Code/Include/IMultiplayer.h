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

#include <AzCore/RTTI/RTTI.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <Include/MultiplayerStats.h>

namespace AzNetworking
{
    class INetworkInterface;
}

namespace Multiplayer
{
    //! Collection of types of Multiplayer Connections
    enum class MultiplayerAgentType
    {
        Uninitialized,   ///< Agent is uninitialized.
        Client,          ///< A Client connected to either a server or host.
        ClientServer,    ///< A Client that also hosts and is the authority of the session
        DedicatedServer  ///< A Dedicated Server which does not locally host any clients
    };

    //! Payload detailing aspects of a Connection other services may be interested in
    struct MultiplayerAgentDatum
    {
        bool m_isInvited;
        MultiplayerAgentType m_agentType;
        AzNetworking::ConnectionId m_id;
        AzNetworking::ByteBuffer<2048> m_userData;
    };

    using ConnectionAcquiredEvent = AZ::Event<MultiplayerAgentDatum>;
    using SessionInitEvent = AZ::Event<AzNetworking::INetworkInterface*>;
    using SessionShutdownEvent = AZ::Event<AzNetworking::INetworkInterface*>;

    //! IMultiplayer provides insight into the Multiplayer session and its Agents
    class IMultiplayer
    {
    public:
        AZ_RTTI(IMultiplayer, "{90A001DD-AD31-46C7-9FBE-1059AFB7F5E9}");

        virtual ~IMultiplayer() = default;

        //! Gets the type of Agent this IMultiplayer impl represents
        //! @return The type of agents represented
        virtual MultiplayerAgentType GetAgentType() = 0;

        //! Sets the type of this Multiplayer connection and calls any related callback
        //! @param state The state of this connection
        virtual void InitializeMultiplayer(MultiplayerAgentType state) = 0;

        //! Adds a ConnectionAcquiredEvent Handler which is invoked when a new endpoint connects to the session
        //! @param handler The SessionInitEvent Handler to add
        virtual void AddConnectionAcquiredHandler(ConnectionAcquiredEvent::Handler& handler) = 0;

        //! Adds a SessionInitEvent Handler which is invoked when a new network session starts
        //! @param handler The SessionInitEvent Handler to add
        virtual void AddSessionInitHandler(SessionInitEvent::Handler& handler) = 0;

        //! Adds a SessionShutdownEvent Handler which is invoked when the current network session ends
        //! @param handler The SessionShutdownEvent handler to add
        virtual void AddSessionShutdownHandler(SessionShutdownEvent::Handler& handler) = 0;

        //! Sends a packet telling if entity update messages can be sent
        //! @param readyForEntityUpdates Ready for entity updates or not
        virtual void SendReadyForEntityUpdates(bool readyForEntityUpdates) = 0;

        //! Returns the gem name associated with the provided component index.
        //! @param  netComponentId the componentId to return the gem name of
        //! @return the name of the gem that contains the requested component
        virtual const char* GetComponentGemName(NetComponentId netComponentId) const = 0;

        //! Returns the component name associated with the provided component index.
        //! @param  netComponentId the componentId to return the component name of
        //! @return the name of the component
        virtual const char* GetComponentName(NetComponentId netComponentId) const = 0;

        //! Returns the property name associated with the provided component index and property index.
        //! @param  netComponentId the component index to return the property name of
        //! @param  propertyIndex  the index of the network property to return the property name of
        //! @return the name of the network property
        virtual const char* GetComponentPropertyName(NetComponentId netComponentId, PropertyIndex propertyIndex) const = 0;

        //! Returns the Rpc name associated with the provided component index and rpc index.
        //! @param  netComponentId the componentId to return the property name of
        //! @param  rpcIndex       the index of the rpc to return the rpc name of
        //! @return the name of the requested rpc
        virtual const char* GetComponentRpcName(NetComponentId netComponentId, RpcIndex rpcIndex) const = 0;

        //! Retrieve the stats object bound to this multiplayer instance.
        //! @return the stats object bound to this multiplayer instance
        MultiplayerStats& GetStats() { return m_stats; }

    private:
        MultiplayerStats m_stats;
    };

    inline const char* GetEnumString(MultiplayerAgentType value)
    {
        switch (value)
        {
        case MultiplayerAgentType::Uninitialized:
            return "Uninitialized";
        case MultiplayerAgentType::Client:
            return "Client";
        case MultiplayerAgentType::ClientServer:
            return "ClientServer";
        case MultiplayerAgentType::DedicatedServer:
            return "DedicatedServer";
        }
        return "INVALID";
    }
}
