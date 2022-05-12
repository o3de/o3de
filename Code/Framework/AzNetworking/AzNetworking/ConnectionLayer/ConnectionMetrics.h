/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/Time/ITime.h>

namespace AzNetworking
{
    //! @struct DatarateAtom
    //! @brief basic unit for measuring socket datarate with connection to time.
    struct DatarateAtom
    {
        DatarateAtom() = default;

        AZ::TimeMs m_timeAccumulatorMs = AZ::Time::ZeroTimeMs;
        uint32_t m_bytesTransmitted = 0;
        uint32_t m_packetsSent = 0;
        uint32_t m_packetsLost = 0;
    };

    //! @class DatarateMetrics
    //! @brief used to track datarate related metrics for a given connection with respect to time.
    class DatarateMetrics
    {
    public:

        DatarateMetrics() = default;

        //! Constructor.
        //! @param maxSampleTimeMs the period of time in milliseconds to attempt to smooth datarate over
        DatarateMetrics(AZ::TimeMs maxSampleTimeMs);

        //! Invoked whenever traffic is handled by the connection this instance is responsible for.
        //! @param byteCount     number of bytes sent through the connection
        //! @param currentTimeMs current process time in milliseconds
        void LogPacket(uint32_t byteCount, AZ::TimeMs currentTimeMs);

        //! Invoked whenever a packet has determined to be lost.
        void LogPacketLost();

        //! Retrieve a sample of the datarate being incurred by this connection in bytes per second.
        //! @return datarate for traffic sent to or from the connection in bytes per second
        float GetBytesPerSecond() const;

        //! Returns the estimated packet loss rate as a percentage of packets.
        //! @return the estimated percentage loss rate
        float GetLossRatePercent() const;

    private:

        //! Used internally to swap buffers used for metric gathering.
        void SwapBuffers();

        static constexpr AZ::TimeMs MaxSampleTimeMs = AZ::TimeMs{ 2000 };

        AZ::TimeMs   m_maxSampleTimeMs = MaxSampleTimeMs;
        AZ::TimeMs   m_lastLoggedTimeMs = MaxSampleTimeMs;
        uint32_t     m_activeAtom = 0;
        DatarateAtom m_atoms[2];
    };

    //! @struct ConnectionPacketEntry
    //! @brief basic data structure used to timestamp packet sequences.
    struct ConnectionPacketEntry
    {
        ConnectionPacketEntry() = default;

        //! Constructor.
        //! @param packetId   packet id of the packet this entry is tracking
        //! @param sendTimeMs logged send time for the tracked packet in milliseconds
        ConnectionPacketEntry(PacketId packetId, AZ::TimeMs sendTimeMs);

        PacketId m_packetId = InvalidPacketId;
        AZ::TimeMs m_sendTimeMs = AZ::Time::ZeroTimeMs;
    };

    //! @class ConnectionComputeRtt
    //! @brief helper class used to compute round trip time to an connection.
    class ConnectionComputeRtt
    {
    public:

        ConnectionComputeRtt() = default;

        //! Invoked whenever traffic is sent through the connection this instance is responsible for.
        //! @param packetId      identifier of the packet being sent
        //! @param currentTimeMs current process time in milliseconds
        void LogPacketSent(PacketId packetId, AZ::TimeMs currentTimeMs);

        //! Invoked whenever traffic is acknowledged from the connection this instance is responsible for.
        //! @param packetId      identifier of the packet being acked
        //! @param currentTimeMs current process time in milliseconds
        void LogPacketAcked(PacketId packetId, AZ::TimeMs currentTimeMs);

        //! Invoked whenever traffic times out from the connection this instance is responsible for.
        //! @param packetId identifier of the packet timing out
        void LogPacketTimeout(PacketId packetId);

        //! Retrieve a sample of the computed round trip time for this connection.
        //! @return estimated round trip time (ping) for the given connection in seconds
        float GetRoundTripTimeSeconds() const;

    private:

        static constexpr uint32_t MaxTrackableEntries = 8;
        static constexpr float InitialRoundTripTime = 0.1f; //< Start off with a 100 millisecond estimate for Rtt

        float m_roundTripTime = InitialRoundTripTime;
        ConnectionPacketEntry m_entries[MaxTrackableEntries];
    };

    //! @struct ConnectionMetrics
    //! @brief used to track general performance metrics for a given connection with respect to time.
    struct ConnectionMetrics
    {
        ConnectionMetrics() = default;
        ConnectionMetrics& operator=(const ConnectionMetrics& rhs) = default;

        //! Resets all internal metrics to defaults.
        void Reset();

        void LogPacketSent(uint32_t byteCount, AZ::TimeMs currentTimeMs);
        void LogPacketRecv(uint32_t byteCount, AZ::TimeMs currentTimeMs);
        void LogPacketLost();
        void LogPacketAcked();

        uint32_t m_packetsSent  = 0;
        uint32_t m_packetsRecv  = 0;
        uint32_t m_packetsLost  = 0;
        uint32_t m_packetsAcked = 0;

        DatarateMetrics      m_sendDatarate;
        DatarateMetrics      m_recvDatarate;
        ConnectionComputeRtt m_connectionRtt;
    };
}

#include <AzNetworking/ConnectionLayer/ConnectionMetrics.inl>
