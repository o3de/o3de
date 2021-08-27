/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/DataStructures/FixedSizeVectorBitset.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <Multiplayer/MultiplayerTypes.h>

namespace Multiplayer
{
    struct ReplicationRecordStats
    {
        ReplicationRecordStats() = default;
        ReplicationRecordStats
        (
            uint32_t authorityToClientCount,
            uint32_t authorityToServerCount,
            uint32_t authorityToAutonomousCount,
            uint32_t autonomousToAuthorityCount
        );

        uint32_t m_authorityToClientCount = 0;
        uint32_t m_authorityToServerCount = 0;
        uint32_t m_authorityToAutonomousCount = 0;
        uint32_t m_autonomousToAuthorityCount = 0;

        bool operator ==(const ReplicationRecordStats& rhs) const;
        ReplicationRecordStats operator-(const ReplicationRecordStats& rhs) const;
    };

    class ReplicationRecord
    {
    public:
        static constexpr uint32_t MaxRecordBits = 2048;

        ReplicationRecord() = default;
        ReplicationRecord(NetEntityRole remoteNetEntityRole);

        void SetRemoteNetworkRole(NetEntityRole remoteNetEntityRole);
        NetEntityRole GetRemoteNetworkRole() const;

        bool AreAllBitsConsumed() const;
        void ResetConsumedBits();

        void Clear();

        void Append(const ReplicationRecord &rhs);
        void Subtract(const ReplicationRecord &rhs);
        bool HasChanges() const;

        bool Serialize(AzNetworking::ISerializer& serializer);

        void ConsumeAuthorityToClientBits(uint32_t consumedBits);
        void ConsumeAuthorityToServerBits(uint32_t consumedBits);
        void ConsumeAuthorityToAutonomousBits(uint32_t consumedBits);
        void ConsumeAutonomousToAuthorityBits(uint32_t consumedBits);

        bool ContainsAuthorityToClientBits() const;
        bool ContainsAuthorityToServerBits() const;
        bool ContainsAuthorityToAutonomousBits() const;
        bool ContainsAutonomousToAuthorityBits() const;

        uint32_t GetRemainingAuthorityToClientBits() const;
        uint32_t GetRemainingAuthorityToServerBits() const;
        uint32_t GetRemainingAuthorityToAutonomousBits() const;
        uint32_t GetRemainingAutonomousToAuthorityBits() const;

        ReplicationRecordStats GetStats() const;

        using RecordBitset = AzNetworking::FixedSizeVectorBitset<MaxRecordBits>;
        RecordBitset m_authorityToClient;
        RecordBitset m_authorityToServer;
        RecordBitset m_authorityToAutonomous;
        RecordBitset m_autonomousToAuthority;

        uint32_t m_authorityToClientConsumedBits = 0;
        uint32_t m_authorityToServerConsumedBits = 0;
        uint32_t m_authorityToAutonomousConsumedBits = 0;
        uint32_t m_autonomousToAuthorityConsumedBits = 0;

        // Sequence number this ReplicationRecord was sent on
        AzNetworking::PacketId m_sentPacketId = AzNetworking::InvalidPacketId;

        NetEntityRole m_remoteNetEntityRole = NetEntityRole::InvalidRole;;
    };
}
