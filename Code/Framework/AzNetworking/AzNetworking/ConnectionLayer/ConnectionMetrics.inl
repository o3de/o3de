/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline DatarateMetrics::DatarateMetrics(AZ::TimeMs maxSampleTimeMs)
        : m_maxSampleTimeMs(maxSampleTimeMs)
        , m_lastLoggedTimeMs{0}
        , m_activeAtom(0)
    {
        ;
    }

    inline void DatarateMetrics::SwapBuffers()
    {
        m_activeAtom = 1 - m_activeAtom;
        m_atoms[m_activeAtom] = DatarateAtom();
    }

    inline ConnectionPacketEntry::ConnectionPacketEntry(PacketId packetId, AZ::TimeMs sendTimeMs)
        : m_packetId(packetId)
        , m_sendTimeMs(sendTimeMs)
    {
        ;
    }

    inline float ConnectionComputeRtt::GetRoundTripTimeSeconds() const
    {
        return m_roundTripTime;
    }

    inline void ConnectionMetrics::Reset()
    {
        *this = ConnectionMetrics();
    }

    inline void ConnectionMetrics::LogPacketSent(uint32_t byteCount, AZ::TimeMs currentTimeMs)
    {
        if (byteCount > 0)
        {
            m_packetsSent++;
        }
        m_sendDatarate.LogPacket(byteCount, currentTimeMs);
    }

    inline void ConnectionMetrics::LogPacketRecv(uint32_t byteCount, AZ::TimeMs currentTimeMs)
    {
        if (byteCount > 0)
        {
            m_packetsRecv++;
        }
        m_recvDatarate.LogPacket(byteCount, currentTimeMs);
    }

    inline void ConnectionMetrics::LogPacketLost()
    {
        m_packetsLost++;
        m_sendDatarate.LogPacketLost();
    }

    inline void ConnectionMetrics::LogPacketAcked()
    {
        m_packetsAcked++;
    }
}
