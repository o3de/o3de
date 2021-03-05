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
