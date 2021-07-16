/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/NetworkEntity/EntityReplication/ReplicationRecord.h>

namespace Multiplayer
{
    ReplicationRecordStats::ReplicationRecordStats
    (
        uint32_t authorityToClientCount,
        uint32_t authorityToServerCount,
        uint32_t authorityToAutonomousCount,
        uint32_t autonomousToAuthorityCount
    )
        : m_authorityToClientCount(authorityToClientCount)
        , m_authorityToServerCount(authorityToServerCount)
        , m_authorityToAutonomousCount(authorityToAutonomousCount)
        , m_autonomousToAuthorityCount(autonomousToAuthorityCount)
    {
        ;
    }

    bool ReplicationRecordStats::operator ==(const ReplicationRecordStats& rhs) const
    {
        return (m_authorityToClientCount == rhs.m_authorityToClientCount)
            && (m_authorityToServerCount == rhs.m_authorityToServerCount)
            && (m_authorityToAutonomousCount == rhs.m_authorityToAutonomousCount)
            && (m_autonomousToAuthorityCount == rhs.m_autonomousToAuthorityCount);
    }

    ReplicationRecordStats ReplicationRecordStats::operator-(const ReplicationRecordStats& rhs) const
    {
        return ReplicationRecordStats
        {
            (m_authorityToClientCount - rhs.m_authorityToClientCount),
            (m_authorityToServerCount - rhs.m_authorityToServerCount),
            (m_authorityToAutonomousCount - rhs.m_authorityToAutonomousCount),
            (m_autonomousToAuthorityCount - rhs.m_autonomousToAuthorityCount)
        };
    }

    ReplicationRecord::ReplicationRecord(NetEntityRole netEntityRole)
        : m_remoteNetEntityRole(netEntityRole)
    {
        ;
    }

    void ReplicationRecord::SetRemoteNetworkRole(NetEntityRole remoteNetEntityRole)
    {
        m_remoteNetEntityRole = remoteNetEntityRole;
    }

    NetEntityRole ReplicationRecord::GetRemoteNetworkRole() const
    {
        return m_remoteNetEntityRole;
    }

    bool ReplicationRecord::AreAllBitsConsumed() const
    {
        bool ret = true;
        ret &= m_authorityToClientConsumedBits == m_authorityToClient.GetSize();
        ret &= m_authorityToServerConsumedBits == m_authorityToServer.GetSize();
        ret &= m_authorityToAutonomousConsumedBits == m_authorityToAutonomous.GetSize();
        ret &= m_autonomousToAuthorityConsumedBits == m_autonomousToAuthority.GetSize();
        return ret;
    }

    void ReplicationRecord::ResetConsumedBits()
    {
        m_authorityToClientConsumedBits = 0;
        m_authorityToServerConsumedBits = 0;
        m_authorityToAutonomousConsumedBits = 0;
        m_autonomousToAuthorityConsumedBits = 0;
    }

    void ReplicationRecord::Clear()
    {
        ResetConsumedBits();

        uint32_t recordSize = m_authorityToClient.GetSize();
        m_authorityToClient.Clear();
        m_authorityToClient.Resize(recordSize);

        recordSize = m_authorityToServer.GetSize();
        m_authorityToServer.Clear();
        m_authorityToServer.Resize(recordSize);

        recordSize = m_authorityToAutonomous.GetSize();
        m_authorityToAutonomous.Clear();
        m_authorityToAutonomous.Resize(recordSize);

        recordSize = m_autonomousToAuthority.GetSize();
        m_autonomousToAuthority.Clear();
        m_autonomousToAuthority.Resize(recordSize);
    }

    void ReplicationRecord::Append(const ReplicationRecord &rhs)
    {
        m_authorityToClient |= rhs.m_authorityToClient;
        m_authorityToServer |= rhs.m_authorityToServer;
        m_authorityToAutonomous |= rhs.m_authorityToAutonomous;
        m_autonomousToAuthority |= rhs.m_autonomousToAuthority;
    }

    void ReplicationRecord::Subtract(const ReplicationRecord &rhs)
    {
        m_authorityToClient.Subtract(rhs.m_authorityToClient);
        m_authorityToServer.Subtract(rhs.m_authorityToServer);
        m_authorityToAutonomous.Subtract(rhs.m_authorityToAutonomous);
        m_autonomousToAuthority.Subtract(rhs.m_autonomousToAuthority);
    }

    bool ReplicationRecord::HasChanges() const
    {
        bool hasChanges(false);
        if (ContainsAuthorityToClientBits())
        {
            hasChanges = hasChanges ? hasChanges : m_authorityToClient.AnySet();
        }
        if (ContainsAuthorityToServerBits())
        {
            hasChanges = hasChanges ? hasChanges : m_authorityToServer.AnySet();
        }
        if (ContainsAuthorityToAutonomousBits())
        {
            hasChanges = hasChanges ? hasChanges : m_authorityToAutonomous.AnySet();
        }
        if (ContainsAutonomousToAuthorityBits())
        {
            hasChanges = hasChanges ? hasChanges : m_autonomousToAuthority.AnySet();
        }
        return hasChanges;
    }

    bool ReplicationRecord::Serialize(AzNetworking::ISerializer& serializer)
    {
        if (ContainsAuthorityToClientBits())
        {
            serializer.Serialize(m_authorityToClient, "AuthorityToClientRecord");
        }
        if (ContainsAuthorityToServerBits())
        {
            serializer.Serialize(m_authorityToServer, "AuthorityToServerRecord");
        }
        if (ContainsAuthorityToAutonomousBits())
        {
            serializer.Serialize(m_authorityToAutonomous, "AuthorityToAutonomousRecord");
        }
        if (ContainsAutonomousToAuthorityBits())
        {
            serializer.Serialize(m_autonomousToAuthority, "AutonomousToAuthorityRecord");
        }
        return serializer.IsValid();
    }

    void ReplicationRecord::ConsumeAuthorityToClientBits(uint32_t consumedBits)
    {
        if (ContainsAuthorityToClientBits())
        {
            m_authorityToClientConsumedBits += consumedBits;
        }
    }

    void ReplicationRecord::ConsumeAuthorityToServerBits(uint32_t consumedBits)
    {
        if (ContainsAuthorityToServerBits())
        {
            m_authorityToServerConsumedBits += consumedBits;
        }
    }

    void ReplicationRecord::ConsumeAuthorityToAutonomousBits(uint32_t consumedBits)
    {
        if (ContainsAuthorityToAutonomousBits())
        {
            m_authorityToAutonomousConsumedBits += consumedBits;
        }
    }

    void ReplicationRecord::ConsumeAutonomousToAuthorityBits(uint32_t consumedBits)
    {
        if (ContainsAutonomousToAuthorityBits())
        {
            m_autonomousToAuthorityConsumedBits += consumedBits;
        }
    }

    bool ReplicationRecord::ContainsAuthorityToClientBits() const
    {
        // Check != Authority here since several modes require information about client updates
        // (i.e. Autonomous when performing corrections)
        return (m_remoteNetEntityRole != NetEntityRole::Authority)
            || (m_remoteNetEntityRole == NetEntityRole::InvalidRole);
    }

    bool ReplicationRecord::ContainsAuthorityToServerBits() const
    {
        return (m_remoteNetEntityRole == NetEntityRole::Server)
            || (m_remoteNetEntityRole == NetEntityRole::InvalidRole);
    }

    bool ReplicationRecord::ContainsAuthorityToAutonomousBits() const
    {
        return (m_remoteNetEntityRole == NetEntityRole::Autonomous || m_remoteNetEntityRole == NetEntityRole::Server)
            || (m_remoteNetEntityRole == NetEntityRole::InvalidRole);
    }

    bool ReplicationRecord::ContainsAutonomousToAuthorityBits() const
    {
        return (m_remoteNetEntityRole == NetEntityRole::Authority)
            || (m_remoteNetEntityRole == NetEntityRole::InvalidRole);
    }

    uint32_t ReplicationRecord::GetRemainingAuthorityToClientBits() const
    {
        return m_authorityToClientConsumedBits < m_authorityToClient.GetValidBitCount() ? m_authorityToClient.GetValidBitCount() - m_authorityToClientConsumedBits : 0;
    }

    uint32_t ReplicationRecord::GetRemainingAuthorityToServerBits() const
    {
        return m_authorityToServerConsumedBits < m_authorityToServer.GetValidBitCount() ? m_authorityToServer.GetValidBitCount() - m_authorityToServerConsumedBits : 0;
    }

    uint32_t ReplicationRecord::GetRemainingAuthorityToAutonomousBits() const
    {
        return m_authorityToAutonomousConsumedBits < m_authorityToAutonomous.GetValidBitCount() ? m_authorityToAutonomous.GetValidBitCount() - m_authorityToAutonomousConsumedBits : 0;
    }

    uint32_t ReplicationRecord::GetRemainingAutonomousToAuthorityBits() const
    {
        return m_autonomousToAuthorityConsumedBits < m_autonomousToAuthority.GetValidBitCount() ? m_autonomousToAuthority.GetValidBitCount() - m_autonomousToAuthorityConsumedBits : 0;
    }

    ReplicationRecordStats ReplicationRecord::GetStats() const
    {
        return ReplicationRecordStats
        {
            m_authorityToClientConsumedBits,
            m_authorityToServerConsumedBits,
            m_authorityToAutonomousConsumedBits,
            m_autonomousToAuthorityConsumedBits
        };
    }
}
