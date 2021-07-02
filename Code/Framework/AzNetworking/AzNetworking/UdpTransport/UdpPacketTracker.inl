/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline PacketId UdpPacketTracker::GetNextPacketId()
    {
        const PacketId nextPacketId = PacketId(++m_nextPacketId);

        if (ToSequenceId(nextPacketId) == InvalidSequenceId)
        {
            return PacketId(++m_nextPacketId);
        }

        return nextPacketId;
    }

    inline SequenceId UdpPacketTracker::GetLastReceivedSequenceId() const
    {
        return m_receivedWindow.GetHeadSequenceId();
    }

    inline BitsetChunk& UdpPacketTracker::GetSequencedAckHistory(BitsetChunk& outWindow) const
    {
        return m_receivedWindow.GetMostRecentAckState(outWindow);
    }

    inline const UdpPacketIdWindow& UdpPacketTracker::GetReceivedWindow() const
    {
        return m_receivedWindow;
    }

    inline const UdpPacketIdWindow& UdpPacketTracker::GetAcknowledgedWindow() const
    {
        return m_acknowledgedWindow;
    }
}
