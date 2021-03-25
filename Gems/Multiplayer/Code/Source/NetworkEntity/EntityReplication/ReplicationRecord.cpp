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

#include <Source/NetworkEntity/EntityReplication/ReplicationRecord.h>

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
        : m_netEntityRole(netEntityRole)
    {
        ;
    }

    void ReplicationRecord::SetNetworkRole(NetEntityRole netEntityRole)
    {
        m_netEntityRole = netEntityRole;
    }

    NetEntityRole ReplicationRecord::GetNetworkRole() const
    {
        return m_netEntityRole;
    }

    bool ReplicationRecord::AreAllBitsConsumed() const
    {
        bool ret = true;
        ret &= m_authorityToClientConsumedBits == m_authorityToClientRecord.GetSize();
        ret &= m_authorityToServerConsumedBits == m_authorityToServerRecord.GetSize();
        ret &= m_authorityToAutonomousConsumedBits == m_authorityToAutonomousRecord.GetSize();
        ret &= m_autonomousToAuthorityConsumedBits == m_autonomousToAuthorityRecord.GetSize();
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

        uint32_t recordSize = m_authorityToClientRecord.GetSize();
        m_authorityToClientRecord.Clear();
        m_authorityToClientRecord.Resize(recordSize);

        recordSize = m_authorityToServerRecord.GetSize();
        m_authorityToServerRecord.Clear();
        m_authorityToServerRecord.Resize(recordSize);

        recordSize = m_authorityToAutonomousRecord.GetSize();
        m_authorityToAutonomousRecord.Clear();
        m_authorityToAutonomousRecord.Resize(recordSize);

        recordSize = m_autonomousToAuthorityRecord.GetSize();
        m_autonomousToAuthorityRecord.Clear();
        m_autonomousToAuthorityRecord.Resize(recordSize);
    }

    void ReplicationRecord::Append(const ReplicationRecord &rhs)
    {
        m_authorityToClientRecord |= rhs.m_authorityToClientRecord;
        m_authorityToServerRecord |= rhs.m_authorityToServerRecord;
        m_authorityToAutonomousRecord |= rhs.m_authorityToAutonomousRecord;
        m_autonomousToAuthorityRecord |= rhs.m_autonomousToAuthorityRecord;
    }

    void ReplicationRecord::Subtract(const ReplicationRecord &rhs)
    {
        m_authorityToClientRecord.Subtract(rhs.m_authorityToClientRecord);
        m_authorityToServerRecord.Subtract(rhs.m_authorityToServerRecord);
        m_authorityToAutonomousRecord.Subtract(rhs.m_authorityToAutonomousRecord);
        m_autonomousToAuthorityRecord.Subtract(rhs.m_autonomousToAuthorityRecord);
    }

    bool ReplicationRecord::HasChanges() const
    {
        bool hasChanges(false);
        if (ContainsAuthorityToClientBits())
        {
            hasChanges = hasChanges ? hasChanges : m_authorityToClientRecord.AnySet();
        }
        if (ContainsAuthorityToServerBits())
        {
            hasChanges = hasChanges ? hasChanges : m_authorityToServerRecord.AnySet();
        }
        if (ContainsAuthorityToAutonomousBits())
        {
            hasChanges = hasChanges ? hasChanges : m_authorityToAutonomousRecord.AnySet();
        }
        if (ContainsAutonomousToAuthorityBits())
        {
            hasChanges = hasChanges ? hasChanges : m_autonomousToAuthorityRecord.AnySet();
        }
        return hasChanges;
    }

    bool ReplicationRecord::Serialize(AzNetworking::ISerializer& a_Serializer)
    {
        if (ContainsAuthorityToClientBits())
        {
            a_Serializer.Serialize(m_authorityToClientRecord, "ServerToClientsRecord");
        }
        if (ContainsAuthorityToServerBits())
        {
            a_Serializer.Serialize(m_authorityToServerRecord, "ServerToServersRecord");
        }
        if (ContainsAuthorityToAutonomousBits())
        {
            a_Serializer.Serialize(m_authorityToAutonomousRecord, "ServerToClientAutonomousRecord");
        }
        if (ContainsAutonomousToAuthorityBits())
        {
            a_Serializer.Serialize(m_autonomousToAuthorityRecord, "ClientToServersRecord");
        }
        return a_Serializer.IsValid();
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
        return (m_netEntityRole != NetEntityRole::ServerAuthority)
            || (m_netEntityRole == NetEntityRole::InvalidRole);
    }

    bool ReplicationRecord::ContainsAuthorityToServerBits() const
    {
        return (m_netEntityRole == NetEntityRole::ServerSimulation)
            || (m_netEntityRole == NetEntityRole::InvalidRole);
    }

    bool ReplicationRecord::ContainsAuthorityToAutonomousBits() const
    {
        return (m_netEntityRole == NetEntityRole::ClientAutonomous || m_netEntityRole == NetEntityRole::ServerSimulation)
            || (m_netEntityRole == NetEntityRole::InvalidRole);
    }

    bool ReplicationRecord::ContainsAutonomousToAuthorityBits() const
    {
        return (m_netEntityRole == NetEntityRole::ServerAuthority)
            || (m_netEntityRole == NetEntityRole::InvalidRole);
    }

    uint32_t ReplicationRecord::GetRemainingAuthorityToClientBits() const
    {
        return m_authorityToClientConsumedBits < m_authorityToClientRecord.GetValidBitCount() ? m_authorityToClientRecord.GetValidBitCount() - m_authorityToClientConsumedBits : 0;
    }

    uint32_t ReplicationRecord::GetRemainingAuthorityToServerBits() const
    {
        return m_authorityToServerConsumedBits < m_authorityToServerRecord.GetValidBitCount() ? m_authorityToServerRecord.GetValidBitCount() - m_authorityToServerConsumedBits : 0;
    }

    uint32_t ReplicationRecord::GetRemainingAuthorityToAutonomousBits() const
    {
        return m_authorityToAutonomousConsumedBits < m_authorityToAutonomousRecord.GetValidBitCount() ? m_authorityToAutonomousRecord.GetValidBitCount() - m_authorityToAutonomousConsumedBits : 0;
    }

    uint32_t ReplicationRecord::GetRemainingAutonomousToAuthorityBits() const
    {
        return m_autonomousToAuthorityConsumedBits < m_autonomousToAuthorityRecord.GetValidBitCount() ? m_autonomousToAuthorityRecord.GetValidBitCount() - m_autonomousToAuthorityConsumedBits : 0;
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
