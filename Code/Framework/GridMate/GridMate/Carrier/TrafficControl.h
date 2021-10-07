/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_TRAFFIC_CONTROL_H
#define GM_TRAFFIC_CONTROL_H

#include <GridMate/Types.h>

#include <AzCore/std/smart_ptr/intrusive_ptr.h>

namespace GridMate
{
    class DriverAddress;

    // u16 sequence counters (max SequenceNumberHalfSpan-1 packets in flight)
    typedef AZ::u16 SequenceNumber;
    static const SequenceNumber SequenceNumberMax = 0xffff;

    // u32 sequence counters
    //    typedef unsigned int SequenceNumber;
    //    static const SequenceNumber SequenceNumberMax = 0xffffffff;

    static const SequenceNumber SequenceNumberHalfSpan = SequenceNumberMax / 2;

    /**
     * Traffic control interface implements the traffic flow to all connections.
     * It should handle issues like congestion, etc.
     * \note All the code is executed in a thread context! Any interaction with
     * the outside code should be made thread safe.
     */
    class TrafficControl
    {
    public:
        struct DataGramControlData
        {
            SequenceNumber          m_sequenceNumber;
            TimeStamp               m_time;
            unsigned short          m_size;             ///< Datagram size in bytes.
            unsigned short          m_effectiveSize;    ///< Datagram effective byte size (no headers just user data)
        };
        struct TrafficControlConnection
        {
            TrafficControlConnection()
                : m_trafficData(NULL)
            {}
            void*   m_trafficData;      ///< Specialized traffic control implementations can store user data here.
        };
        /// Carrier thread connection identifier.
        typedef TrafficControlConnection* TrafficControlConnectionId;

        virtual ~TrafficControl()  {}

        /// Called when Carrier has established a new connection.
        virtual void OnConnect(TrafficControlConnectionId id, const AZStd::intrusive_ptr<DriverAddress>& address) = 0;
        /// Called when Carrier has lost a connection.
        virtual void OnDisconnect(TrafficControlConnectionId id) = 0;
        /// Called when Carrier completed successful handshake. Usually NAT punch happens during the handshake, which can result is high packet loss.
        virtual void OnHandshakeComplete(TrafficControlConnectionId id) = 0;
        /// Called when Carrier has send a package.
        virtual void OnSend(TrafficControlConnectionId id, DataGramControlData& info) = 0;
        /// Called when Carrier has send an ACK/NACK data with the packet.
        virtual void OnSendAck(TrafficControlConnectionId id) = 0;
        /// Called when Carrier has resend a package.
        virtual void OnReSend(TrafficControlConnectionId id, DataGramControlData& info, unsigned int resendDataSize) = 0;
        /// Called when Carrier confirmed a package delivery.
        virtual void OnAck(TrafficControlConnectionId id, DataGramControlData& info, bool& windowChanged) = 0;
        /// Called when we receive a NAck for a package delivery.
        virtual void OnNAck(TrafficControlConnectionId id, DataGramControlData& info) = 0;
        /// Called when Carrier receives a package.
        virtual void OnReceived(TrafficControlConnectionId id, DataGramControlData& info) = 0;
        /// Return true if we can send a package. Otherwise false.
        virtual bool IsSend(TrafficControlConnectionId id) = 0;
        /// Return true if you should send ACK/NACK data at this time.
        virtual bool IsSendAck(TrafficControlConnectionId id) = 0;
        /// Return number of bytes we are allowed to send at the moment. The size can/will vary over time.
        virtual unsigned int GetAvailableWindowSize(TrafficControlConnectionId id) const = 0;
        /**
        * Called for every package waiting for Ack. If this function returns true the packet will be considered lost.
        * You should resend it and call OnReSend function ASAP.
        */
        virtual bool IsResend(TrafficControlConnectionId id, const DataGramControlData& info, unsigned int resendDataSize) = 0;

        /**
        * Returns the timestamp for retransmission
        */
        virtual TimeStamp GetResendTime(TrafficControlConnectionId id, const DataGramControlData& info) = 0;

        /// Verify traffic conditions and disconnect if needed. This usually happens when we have bad conditions. Too much latency or high packet loss.
        virtual bool IsDisconnect(TrafficControlConnectionId id, float conditionThreshold) = 0;

        /// Verify we are able to receive data from a given address
        virtual bool IsCanReceiveData(TrafficControlConnectionId id) const = 0;

        /**
         * Returns true if you need to send a ACK only (empty datagram) due to time and/or number of received datagrams.
         * If you already have data to send ACK will be included in the datagram anyway. This function should be checked only
         * if you have no data to send.
         */
        virtual bool IsSendACKOnly(TrafficControlConnectionId id) const = 0;

        /// Update/Tick returns true if we have updated the statistics (which we can read by \ref QueryStatistics)
        virtual bool Update() = 0;

        /**
         *
         */
        struct Statistics
        {
            unsigned int    m_dataSend;         ///< Data send in bytes.
            unsigned int    m_dataReceived;     ///< Data received in bytes.

            unsigned int    m_dataResend;           ///< Data resend in bytes.
            unsigned int    m_dataAcked;            ///< Data received in bytes.

            unsigned int    m_packetSend;           ///< Number of packets/datagrams send.
            unsigned int    m_packetReceived;       ///< Number of packets/datagrams received.

            unsigned int    m_packetLost;           ///< Number of packets/datagrams lost.
            unsigned int    m_packetAcked;      ///< Number of packets/datagrams acked/confirmed received.

            float           m_rtt;              ///< Round trip time in milliseconds.
            float           m_packetLoss;           ///< Packet loss percentage (smooth average) [0.0,1.0]

            float           m_connectionFactor; ///< [0.0,1.0] 0 is good connection when 1.0 is reached a bad connection will be reported (unless disconnect detection is off)

            //float         m_flow;             ///< A value reporting the state of the flow control. 1.0 if unrestricted flow (full capacity), 0.0 is max restricted (send at minimal rate - otherwise disconnect will occur).
        };

        /**
         * Stores connection statistics, it's ok to pass NULL for any of the statistics
         * \param id Connection ID
         * \param lastSecond last second statistics for all data
         * \param lifetime lifetime statistics for all data
         * \param effectiveLastSecond last second statistics for effective data (actual data - carrier overhead excluded)
         * \param effectiveLifetime lifetime statistics for effective data (actual data - carrier overhead excluded)
         */
        virtual void        QueryStatistics(TrafficControlConnectionId id, Statistics* lastSecond = nullptr, Statistics* lifetime = nullptr, Statistics* effectiveLastSecond = nullptr, Statistics* effectiveLifetime = nullptr) const = 0;

        struct CongestionState
        {
            unsigned int    m_dataInTransfer;   ///< Data in progess (out of the toSend queue)
            unsigned int    m_congestionWindow; ///< If the traffic controller uses congestionWindow it's size will be set >0
        };

        /**
         * Stores current congestion state into the provided block.
         */
        virtual void        QueryCongestionState(TrafficControlConnectionId id, CongestionState* congestionState) const = 0;
    };


    //////////////////////////////////////////////////////////////////////////
    // Utility functions
    inline bool SequenceNumberIsSequential(SequenceNumber a, SequenceNumber b)
    {
        return ((b > a) && (b - a <= SequenceNumberHalfSpan)) || ((b < a) && (a - b > SequenceNumberHalfSpan));
    }
    inline SequenceNumber SequenceNumberSequentialDistance(SequenceNumber a, SequenceNumber b)
    {
        SequenceNumber dist;
        if (b > a)
        {
            dist = b - a;
            if (dist <= SequenceNumberHalfSpan)
            {
                return dist;
            }
        }
        else if (b < a)
        {
            dist = (SequenceNumberMax - a) + b + 1;
            if (dist <= SequenceNumberHalfSpan)
            {
                return dist;
            }
        }
        return 0; // invalid distance if a != b, otherwise correct
    }

    // Checks if a > b
    inline bool SequenceNumberGreaterThan(SequenceNumber a, SequenceNumber b)
    {
        SequenceNumber off = b - a;
        return (b != a) && (off > SequenceNumberHalfSpan);
    }

    // Check if a >= b
    inline bool SequenceNumberGreaterEqualThan(SequenceNumber a, SequenceNumber b)
    {
        if (a == b)
        {
            return true;
        }
        return SequenceNumberGreaterThan(a, b);
    }

    // Checks if a < b
    inline bool SequenceNumberLessThan(SequenceNumber a, SequenceNumber b)
    {
        SequenceNumber off = b - a;
        return (b != a) && (off < SequenceNumberHalfSpan);
    }
    //////////////////////////////////////////////////////////////////////////
}

#endif // GM_TRAFFIC_CONTROL_H

