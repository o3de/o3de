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
}
