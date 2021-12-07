/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/ConnectionLayer/ConnectionMetrics.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    AZ_CVAR(float, net_rttIncreaseOnPacketLoss, 1.2f, nullptr, AZ::ConsoleFunctorFlags::Null, "Scalar amount to increase round trip time estimates by on packet loss");
    AZ_CVAR(AZ::TimeMs, net_maxPacketTrackTimeMs, AZ::TimeMs{2000}, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum time to track any particular packetid before giving up");

    void DatarateMetrics::LogPacket(uint32_t byteCount, AZ::TimeMs currentTimeMs)
    {
        const AZ::TimeMs deltaTimeMs = currentTimeMs - m_lastLoggedTimeMs;

        m_atoms[m_activeAtom].m_bytesTransmitted += byteCount;
        m_atoms[m_activeAtom].m_packetsSent++;
        m_atoms[m_activeAtom].m_timeAccumulatorMs += deltaTimeMs;

        if (m_atoms[m_activeAtom].m_timeAccumulatorMs >= m_maxSampleTimeMs)
        {
            SwapBuffers();
        }

        m_lastLoggedTimeMs = currentTimeMs;
    }

    void DatarateMetrics::LogPacketLost()
    {
        m_atoms[m_activeAtom].m_packetsLost++;
    }

    float DatarateMetrics::GetBytesPerSecond() const
    {
        const uint32_t sampleAtom = 1 - m_activeAtom;

        if (m_atoms[sampleAtom].m_timeAccumulatorMs == AZ::Time::ZeroTimeMs)
        {
            return 0.0f;
        }

        const float bytesLogged = float(m_atoms[sampleAtom].m_bytesTransmitted);
        const float sampleTime = float(m_atoms[sampleAtom].m_timeAccumulatorMs);

        return (bytesLogged * 1000.0f) / sampleTime; // (* 1000) to convert from bytes per millisecond to bytes per second
    }

    float DatarateMetrics::GetLossRatePercent() const
    {
        const uint32_t sampleAtom = 1 - m_activeAtom;

        if (m_atoms[sampleAtom].m_packetsSent == 0)
        {
            return 0.0f;
        }

        return float(m_atoms[sampleAtom].m_packetsLost) / float(m_atoms[sampleAtom].m_packetsSent);
    }

    void ConnectionComputeRtt::LogPacketSent(PacketId packetId, AZ::TimeMs currentTimeMs)
    {
        for (uint32_t i = 0; i < MaxTrackableEntries; i++)
        {
            // Locate first unused entry, if one exists
            if (m_entries[i].m_packetId == InvalidPacketId)
            {
                m_entries[i].m_packetId = packetId;
                m_entries[i].m_sendTimeMs = currentTimeMs;
                return;
            }
        }
    }

    void ConnectionComputeRtt::LogPacketAcked(PacketId packetId, AZ::TimeMs currentTimeMs)
    {
        for (uint32_t i = 0; i < MaxTrackableEntries; i++)
        {
            if (m_entries[i].m_packetId == packetId)
            {
                const AZ::TimeMs milliseconds(currentTimeMs - m_entries[i].m_sendTimeMs);
                const float timeToAck = static_cast<float>(milliseconds) * 0.001f;
                m_roundTripTime = (timeToAck * 0.1f) + (m_roundTripTime * 0.9f);
                m_entries[i].m_packetId = InvalidPacketId;
                AZLOG(NET_Rtt, "Packet id %d acked after %d milliseconds, new latency %f seconds", (int)packetId, (int)milliseconds, m_roundTripTime);
                return;
            }
            else if ((m_entries[i].m_packetId != InvalidPacketId) && (currentTimeMs - m_entries[i].m_sendTimeMs > net_maxPacketTrackTimeMs))
            {
                AZLOG(NET_Rtt, "Giving up on tracking packetid %d, timeout exceeded", (int)m_entries[i].m_packetId);
                m_entries[i].m_packetId = InvalidPacketId;
            }
        }
    }

    void ConnectionComputeRtt::LogPacketTimeout(PacketId packetId)
    {
        for (uint32_t i = 0; i < MaxTrackableEntries; i++)
        {
            if (m_entries[i].m_packetId == packetId)
            {
                m_roundTripTime *= net_rttIncreaseOnPacketLoss;
                m_entries[i].m_packetId = InvalidPacketId;
                return;
            }
        }
    }
}
