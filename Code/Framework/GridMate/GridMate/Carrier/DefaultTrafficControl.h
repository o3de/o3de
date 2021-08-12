/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_DEFAULT_TRAFFIC_CONTROL_H
#define GM_DEFAULT_TRAFFIC_CONTROL_H

#include <GridMate/Carrier/TrafficControl.h>

#include <GridMate/Containers/list.h>

#include <AzCore/std/string/string.h>

namespace GridMate
{
    /**
    * Traffic control default implementation.
    */
    class DefaultTrafficControl
        : public TrafficControl
    {
    public:
        GM_CLASS_ALLOCATOR(DefaultTrafficControl);

        /// maxSystemPacketSize in bytes and datagramOverhead in bytes (used for effective statistics)
        DefaultTrafficControl(unsigned int maxSystemPacketSize, float rttConnectionThreshold, float packetLossThreshold, unsigned int maxRecvPackets);
        ~DefaultTrafficControl() override;

        /// Called when Carrier has established a new connection.
        void OnConnect(TrafficControlConnectionId id, const AZStd::intrusive_ptr<DriverAddress>& address) override;
        /// Called when Carrier has lost a connection.
        void OnDisconnect(TrafficControlConnectionId id) override;
        /// Called when Carrier completed successful handshake. Usually NAT punch happens during the handshake, which can result is high packet loss.
        void OnHandshakeComplete(TrafficControlConnectionId id) override;
        /// Called when Carrier has send a package.
        void OnSend(TrafficControlConnectionId id, DataGramControlData& info) override;
        /// Called when Carrier has send an ACK/NACK data with the packet.
        void OnSendAck(TrafficControlConnectionId id) override;
        /// Called when Carrier has resend a package.
        void OnReSend(TrafficControlConnectionId id, DataGramControlData& info, unsigned int resendDataSize) override;
        /// Called when Carrier confirmed a package delivery.
        void OnAck(TrafficControlConnectionId id, DataGramControlData& info, bool& windowChange) override;
        /// Called when we receive a NAck for a package delivery.
        void OnNAck(TrafficControlConnectionId id, DataGramControlData& info) override;
        /// Called when Carrier receives a package.
        void OnReceived(TrafficControlConnectionId id, DataGramControlData& info) override;
        /// Return true if we can send a package. Otherwise false.
        bool IsSend(TrafficControlConnectionId id) override;
        /// Return true if you should send ACK/NACK data at this time.
        bool IsSendAck(TrafficControlConnectionId id) override;
        /// Return number of bytes we are allowed to send at the moment. The size can/will vary over time.
        unsigned int GetAvailableWindowSize(TrafficControlConnectionId id) const override;
        /**
        * Called for every package waiting for Ack. If this function returns true the packet will be considered lost.
        * You should resend it and call OnReSend function ASAP.
        */
        bool IsResend(TrafficControlConnectionId id, const DataGramControlData& info, unsigned int resendDataSize) override;

        TimeStamp GetResendTime(TrafficControlConnectionId id, const DataGramControlData& info) override;

        bool IsDisconnect(TrafficControlConnectionId id, float conditionThreshold) override;

        bool IsCanReceiveData(TrafficControlConnectionId id) const override;

        /**
         * Returns true if you need to send a ACK only (empty datagram) due to time and/or number of received datagrams.
         * If you already have data to send ACK will be included in the datagram anyway. This function should be checked only
         * if you have no data to send.
         */
        bool IsSendACKOnly(TrafficControlConnectionId id) const override;

        /// Update/Tick returns true if we have updated the statistics (which we can read by \ref QueryStatistics)
        bool Update() override;
        /**
         * Stores connection statistics, it's ok to pass NULL for any of the statistics.
         * \param id Connection ID
         * \param lastSecond last second statistics for all data
         * \param lifetime lifetime statistics for all data
         * \param effectiveLastSecond last second statistics for effective data (actual data - carrier overhead excluded)
         * \param effectiveLifetime lifetime statistics for effective data (actual data - carrier overhead excluded)
         */
        void QueryStatistics(TrafficControlConnectionId id, Statistics* lastSecond = nullptr, Statistics* lifetime = nullptr, Statistics* effectiveLastSecond = nullptr, Statistics* effectiveLifetime = nullptr) const override;

        /**
        * Stores current congestion state into the provided block.
        */
        void        QueryCongestionState(TrafficControlConnectionId id, CongestionState* congestionState) const override;

    private:
        unsigned int m_maxPacketSize;       ///< Current max packet size

        float m_rttConnectionThreshold;     ///< in milliseconds
        float m_packetLossThreshold;        ///< percent 1.0 is 100%

        struct StatisticData
            : public TrafficControl::Statistics
        {
            float m_avgPacketSend;  ///< Used to average packet send over the last 10-15 sec. Used for packetLoss
            float m_avgPacketLost;  ///< Used to average packet lost over the last 10-15 sec. Used for packetLoss

            StatisticData() { Reset(); }
            void Reset()
            {
                m_dataSend = 0;
                m_dataReceived = 0;

                m_dataResend = 0;
                m_dataAcked = 0;

                m_packetSend = 0;
                m_packetReceived = 0;

                m_packetLost = 0;
                m_packetAcked = 0;

                m_rtt = 0;
                m_packetLoss = 0;
                m_connectionFactor = 0;
                //m_flow = 0;

                m_avgPacketSend = 0;
                m_avgPacketLost = 0;
            }
        };

        struct ConnectionData
        {
            //////////////////////////////////////////////////////////////////////////
            // TCP Cubic Functions
            // NOTE: Before modifying this section read "CUBIC: a new TCP-friendly high-speed TCP variant" 
            //       http://dl.acm.org/citation.cfm?id=1400105
            void TCPCubicCalcK(unsigned int packetSize);
            void TCPCubicPacketLost(TimeStamp& now, unsigned int packetSize);
            void TCPCubicExitSlowStart(TimeStamp& now, unsigned int packetSize);
            unsigned int TCPCubicWindow(TimeStamp& now, unsigned int packetSize) const;
            unsigned int TCPRenoWindow(double time) const;
            //////////////////////////////////////////////////////////////////////////

            StatisticData   m_sdLifetime;           ///< Statistic data for the lifetime of the connection.
            StatisticData   m_sdLastSecond;     ///< Statistic data for the last second.
            StatisticData   m_sdCurrentSecond;  ///< Current data for the elapsing second.

            StatisticData   m_sdEffectiveLifetime;      ///< Lifetime statistics for effective data.
            StatisticData   m_sdEffectiveLastSecond;        ///< Last second statistics for effective data.
            StatisticData   m_sdEffectiveCurrentSecond; ///< Elapsing second statistics for effective data.

            AZStd::string   m_address;          ///< Full address for this connection. (we need for debug only)
            unsigned int    m_recvPacketAllowance; ///< Current allowance for number of incoming packets
            bool            m_canReceiveData; ///< Able to receive data on this connection

            TimeStamp       m_lastPacketSend;
            TimeStamp       m_lastAckSend;
            TimeStamp       m_handshakeDone;        ///< Stamp when the handshake operation has completed.
            bool            m_isReceivedDataAfterLastSend;  ///< Flag indicating that we have received data (not just ACK) after the last send. This can be used to send instant ACK if needed.

            //////////////////////////////////////////////////////////////////////////
            // Slow start basic TCP based congestion control
            TimeStamp       m_lastWindowSizeIncrease;
            TimeStamp       m_lastWindowSizeDecrease;
            unsigned int    m_inTransfer;               ///< Number of bytes in transfer
            unsigned int    m_slowStartThreshold;       ///< Slow start threshold (SSTresh)
            unsigned int    m_congestionWindow;         ///< Congestion window (cwnd)
            unsigned int    m_maxCongestionWindow;      ///< Max congestion window for this connection (TODO make the other side to advertise this window)
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // TCP Cubic Data
            // NOTE: Before modifying this section read "CUBIC: a new TCP-friendly high-speed TCP variant" 
            //       http://dl.acm.org/citation.cfm?id=1400105
            static const bool   k_enableCubic = false;
            static const double k_CubicAlpha;                                    ///< alpha for TCP friendly window estimation
            static const double k_CubicBeta;                                     ///< beta (backoff rate) for congestion calculations
            static const double k_CubicScaleC;                                   ///< Scaling factor. COTS default (0.4)
            static const unsigned int  k_MinCongestionWindowPackets = 10;        ///< Minimum congestion window
            unsigned int        m_preBackoffCongestionWindow = k_MinCongestionWindowPackets;                   ///< Window before backoff
            double              m_CubicKcube = 0.0;
            double              m_CubicK = 0.0;
            //////////////////////////////////////////////////////////////////////////
        };

        typedef list<ConnectionData> ConnectionListType;
        ConnectionListType m_connections;

        float           m_lastStatDataReset;        ///< The time in seconds since the last time we reset the statistic data.
        TimeStamp       m_currentTime;              ///< Current time. (replace this with a global clock when possible.)
        unsigned int    m_lostPacketTimeoutMS;      ///< Time in milliseconds for a packet to be considered lost.
        unsigned int    m_defaultMaxCongestionWindowSize;

        unsigned int m_maxRecvPackets;
    };
}

#endif // GM_DEFAULT_TRAFFIC_CONTROL_H
