/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/ConnectionLayer/ConnectionEnums.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>

namespace Multiplayer
{
    //! The default number of rewindable samples for us to store.
    static constexpr uint32_t RewindHistorySize = 128;

    //! The default blend factor for ScopedAlterTime
    static constexpr float DefaultBlendFactor = 1.f;

    using HostId = AzNetworking::IpAddress;
    static const HostId InvalidHostId = HostId();

    AZ_TYPE_SAFE_INTEGRAL(NetEntityId, uint64_t);
    static constexpr NetEntityId InvalidNetEntityId = static_cast<NetEntityId>(-1);

    AZ_TYPE_SAFE_INTEGRAL(NetComponentId, uint16_t);
    static constexpr NetComponentId InvalidNetComponentId = static_cast<NetComponentId>(-1);

    AZ_TYPE_SAFE_INTEGRAL(PropertyIndex, uint16_t);
    AZ_TYPE_SAFE_INTEGRAL(RpcIndex, uint16_t);

    AZ_TYPE_SAFE_INTEGRAL(ClientInputId, uint16_t);

    //! This is a strong typedef for representing the number of application frames since application start.
    AZ_TYPE_SAFE_INTEGRAL(HostFrameId, uint32_t);
    static constexpr HostFrameId InvalidHostFrameId = HostFrameId{ AzPhysics::SimulatedBody::UndefinedFrameId };

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
    const char* GetEnumString(NetEntityRole value);

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

    enum class AutoActivate : uint8_t
    {
        DoNotActivate,
        Activate
    };

    //! Structure for identifying a specific entity within a spawnable.
    struct PrefabEntityId
    {
        AZ_TYPE_INFO(PrefabEntityId, "{EFD37465-CCAC-4E87-A825-41B4010A2C75}");

        static constexpr uint32_t AllIndices = AZStd::numeric_limits<uint32_t>::max();

        AZ::Name m_prefabName;
        uint32_t m_entityOffset = AllIndices;

        PrefabEntityId() = default;
        explicit PrefabEntityId(AZ::Name name, uint32_t entityOffset = AllIndices);
        bool operator==(const PrefabEntityId& rhs) const;
        bool operator!=(const PrefabEntityId& rhs) const;
        bool Serialize(AzNetworking::ISerializer& serializer);
    };

    struct EntityMigrationMessage
    {
        NetEntityId m_netEntityId;
        PrefabEntityId m_prefabEntityId;
        AzNetworking::PacketEncodingBuffer m_propertyUpdateData;
        bool operator!=(const EntityMigrationMessage& rhs) const;
        bool Serialize(AzNetworking::ISerializer& serializer);
    };

    inline const char* GetEnumString(NetEntityRole value)
    {
        switch (value)
        {
        case NetEntityRole::InvalidRole:
            return "InvalidRole";
        case NetEntityRole::Client:
            return "Client";
        case NetEntityRole::Autonomous:
            return "Autonomous";
        case NetEntityRole::Server:
            return "Server";
        case NetEntityRole::Authority:
            return "Authority";
        }
        return "Unknown";
    }

    inline PrefabEntityId::PrefabEntityId(AZ::Name name, uint32_t entityOffset)
        : m_prefabName(name)
        , m_entityOffset(entityOffset)
    {
        ;
    }

    inline bool PrefabEntityId::operator==(const PrefabEntityId& rhs) const
    {
        return m_prefabName == rhs.m_prefabName && m_entityOffset == rhs.m_entityOffset;
    }

    inline bool PrefabEntityId::operator!=(const PrefabEntityId& rhs) const
    {
        return !(*this == rhs);
    }

    inline bool PrefabEntityId::Serialize(AzNetworking::ISerializer& serializer)
    {
        serializer.Serialize(m_prefabName, "prefabName");
        serializer.Serialize(m_entityOffset, "entityOffset");
        return serializer.IsValid();
    }

    inline bool EntityMigrationMessage::operator!=(const EntityMigrationMessage& rhs) const
    {
        return m_netEntityId        != rhs.m_netEntityId
            || m_prefabEntityId     != rhs.m_prefabEntityId
            || m_propertyUpdateData != rhs.m_propertyUpdateData;
    }

    inline bool EntityMigrationMessage::Serialize(AzNetworking::ISerializer& serializer)
    {
        serializer.Serialize(m_netEntityId, "netEntityId");
        serializer.Serialize(m_prefabEntityId, "prefabEntityId");
        serializer.Serialize(m_propertyUpdateData, "propertyUpdateData");
        return serializer.IsValid();
    }
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::NetEntityId);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::NetComponentId);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::PropertyIndex);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::RpcIndex);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::ClientInputId);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::HostFrameId);

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Multiplayer::NetEntityId, "{05E4C08B-3A1B-4390-8144-3767D8E56A81}");
    AZ_TYPE_INFO_SPECIALIZE(Multiplayer::NetComponentId, "{8AF3B382-F187-4323-9014-B380638767E3}");
    AZ_TYPE_INFO_SPECIALIZE(Multiplayer::PropertyIndex, "{F4460210-024D-4B3B-A10A-04B669C34230}");
    AZ_TYPE_INFO_SPECIALIZE(Multiplayer::RpcIndex, "{EBB1C475-FA03-4111-8C84-985377434B9B}");
    AZ_TYPE_INFO_SPECIALIZE(Multiplayer::ClientInputId, "{35BF3504-CEC9-4406-A275-C633A17FBEFB}");
    AZ_TYPE_INFO_SPECIALIZE(Multiplayer::HostFrameId, "{DF17F6F3-48C6-4B4A-BBD9-37DA03162864}");
} // namespace AZ
