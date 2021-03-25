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

#include <AzCore/EBus/Event.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/ConnectionLayer/ConnectionEnums.h>

namespace Multiplayer
{
    AZ_TYPE_SAFE_INTEGRAL(HostId, uint32_t);
    static constexpr HostId InvalidHostId = static_cast<HostId>(-1);

    AZ_TYPE_SAFE_INTEGRAL(NetEntityId, uint32_t);
    static constexpr NetEntityId InvalidNetEntityId = static_cast<NetEntityId>(-1);

    AZ_TYPE_SAFE_INTEGRAL(NetComponentId, uint16_t);
    static constexpr NetComponentId InvalidNetComponentId = static_cast<NetComponentId>(-1);

    using LongNetworkString = AZ::CVarFixedString;
    using ReliabilityType = AzNetworking::ReliabilityType;

    class NetworkEntityRpcMessage;
    using RpcSendEvent = AZ::Event<NetworkEntityRpcMessage&>;

    // Note that we explicitly set storage classes so that sizeof() is accurate for serialized size
    enum class RpcDeliveryType : uint8_t
    {
        None,
        ServerAuthorityToClientSimulation, // Invoked from ServerAuthority, handled on ClientSimulation
        ServerAuthorityToClientAutonomous, // Invoked from ServerAuthority, handled on ClientAutonomous
        ClientAutonomousToServerAuthority, // Invoked from ClientAutonomous, handled on ServerAuthority
        ServerSimulationToServerAuthority  // Invoked from ServerSimulation, handled on ServerAuthority
    };

    enum class NetEntityRole : uint8_t
    {
        InvalidRole,      // No role
        ClientSimulation, // A simulated proxy on a client
        ClientAutonomous, // An autonomous proxy on a client (can execute local prediction)
        ServerSimulation, // A simulated proxy on a server
        ServerAuthority   // An authoritative proxy on a server (full authority)
    };

    enum class ComponentSerializationType : uint8_t
    {
        Properties,
        Correction
    };

    enum class EntityIsMigrating : uint8_t
    {
        False,
        True
    };

    // This is just a placeholder
    // The level/prefab cooking will devise the actual solution for identifying a dynamically spawnable entity within a prefab
    struct PrefabEntityId
    {
        AZ_TYPE_INFO(PrefabEntityId, "{EFD37465-CCAC-4E87-A825-41B4010A2C75}");
        bool operator==(const PrefabEntityId&) const { return true; }
        bool operator!=(const PrefabEntityId& rhs) const { return !(*this == rhs); }
        bool Serialize(AzNetworking::ISerializer&) { return true; }
    };
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::HostId);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::NetEntityId);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::NetComponentId);
