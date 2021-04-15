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
#include <AzCore/Name/Name.h>

namespace Multiplayer
{
    //! The default number of rewindable samples for us to store.
    static constexpr uint32_t RewindHistorySize = 128;

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
        AuthorityToClient,     // Invoked from Authority, handled on Client
        AuthorityToAutonomous, // Invoked from Authority, handled on Autonomous
        AutonomousToAuthority, // Invoked from Autonomous, handled on Authority
        ServerToAuthority      // Invoked from Server, handled on Authority
    };

    enum class NetEntityRole : uint8_t
    {
        InvalidRole, // No role
        Client,      // A simulated proxy on a client
        Autonomous,  // An autonomous proxy on a client (can execute local prediction)
        Server,      // A simulated proxy on a server
        Authority    // An authoritative proxy on a server (full authority)
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

    template<typename TYPE>
    bool Serialize(TYPE& value, const char* name);

    inline NetEntityId MakeEntityId(uint8_t a_ServerId, int32_t a_NextId)
    {
        constexpr int32_t MAX_ENTITYID = 0x00FFFFFF;

        AZ_Assert((a_NextId < MAX_ENTITYID) && (a_NextId > 0), "Requested Id out of range");

        NetEntityId ret = NetEntityId(((static_cast<int32_t>(a_ServerId) << 24) & 0xFF000000) | (a_NextId & MAX_ENTITYID));
        return ret;
    }

    // This is just a placeholder
    // The level/prefab cooking will devise the actual solution for identifying a dynamically spawnable entity within a prefab
    struct PrefabEntityId
    {
        AZ_TYPE_INFO(PrefabEntityId, "{EFD37465-CCAC-4E87-A825-41B4010A2C75}");

        static constexpr uint32_t AllIndices = AZStd::numeric_limits<uint32_t>::max();

        AZ::Name m_prefabName;
        uint32_t m_entityOffset = AllIndices;

        PrefabEntityId() = default;
        
        explicit PrefabEntityId(AZ::Name name, uint32_t entityOffset = AllIndices)
            : m_prefabName(name)
            , m_entityOffset(entityOffset)
        {
        }

        bool operator==(const PrefabEntityId& rhs) const
        {
            return m_prefabName == rhs.m_prefabName && m_entityOffset == rhs.m_entityOffset;
        }

        bool operator!=(const PrefabEntityId& rhs) const
        {
            return !(*this == rhs);
        }

        bool Serialize(AzNetworking::ISerializer& serializer)
        {
            serializer.Serialize(m_prefabName, "prefabName");
            serializer.Serialize(m_entityOffset, "entityOffset");
            return serializer.IsValid();
        }
    };
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::HostId);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::NetEntityId);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::NetComponentId);
