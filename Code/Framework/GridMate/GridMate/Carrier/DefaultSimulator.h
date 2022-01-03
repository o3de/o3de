/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_DEFAULT_SIMULATOR_H
#define GM_DEFAULT_SIMULATOR_H

#include <GridMate/Carrier/Simulator.h>
#include <GridMate/Containers/list.h>
#include <AzCore/std/parallel/mutex.h>

namespace GridMate
{
    /**
    * Simulator default implementation. It will run on the network thread so all user functions are thread safe.
    */
    class DefaultSimulator
        : public Simulator
    {
        friend class CarrierThread;

        /// Called from Carrier, so simulator can use the low level driver directly.
        void BindDriver(Driver* driver) override;
        /// Called from Carrier when driver can no longer be used(ie. will be destroyed)
        void UnbindDriver() override;
        /// Called when Carrier has established a new connection.
        void OnConnect(const AZStd::intrusive_ptr<DriverAddress>& address) override;
        /// Called when Carrier has lost a connection.
        void OnDisconnect(const AZStd::intrusive_ptr<DriverAddress>& address) override;
        /// Called when Carrier has send a package.
        bool OnSend(const AZStd::intrusive_ptr<DriverAddress>& to, const void* data, unsigned int dataSize) override;
        /// Called when Carrier receives package receive.
        bool OnReceive(const AZStd::intrusive_ptr<DriverAddress>& from, const void* data, unsigned int dataSize) override;
        /// Called from Carrier when no more data has arrived and you can supply you data (with latency, out of order, etc.
        unsigned int ReceiveDataFrom(AZStd::intrusive_ptr<DriverAddress>& from, char* data, unsigned int maxDataSize) override;

        void Update() override;
    public:
        GM_CLASS_ALLOCATOR(DefaultSimulator);

        DefaultSimulator();
        virtual ~DefaultSimulator();

        void Enable()           { m_enable = true; }
        void Disable()          { m_enable = false; }
        bool IsEnabled() const  { return m_enable; }

        void SetOutgoingLatency(unsigned int minDelayMS, unsigned int maxDelayMS);
        void SetIncomingLatency(unsigned int minDelayMS, unsigned int maxDelayMS);

        void GetOutgoingLatency(unsigned int& minDelayMS, unsigned int& maxDelayMS);
        void GetIncomingLatency(unsigned int& minDelayMS, unsigned int& maxDelayMS);

        /// Lose one packet every interval
        void SetOutgoingPacketLoss(unsigned int minInterval, unsigned int maxInterval);
        void SetIncomingPacketLoss(unsigned int minInterval, unsigned int maxInterval);

        void GetOutgoingPacketLoss(unsigned int& minInterval, unsigned int& maxInterval);
        void GetIncomingPacketLoss(unsigned int& minInterval, unsigned int& maxInterval);

        void SetOutgoingBandwidth(unsigned int minBandwidthKbps, unsigned int maxBandwidthKbps);
        void SetIncomingBandwidth(unsigned int minBandwidthKbps, unsigned int maxBandwidthKbps);

        void GetOutgoingBandwidth(unsigned int& minBandwidthKbps, unsigned int& maxBandwidthKbps);
        void GetIncomingBandwidth(unsigned int& minBandwidthKbps, unsigned int& maxBandwidthKbps);

        void SetOutgoingPacketDrop(unsigned int minDropInterval, unsigned int maxDropInterval, unsigned int minDropPeriod, unsigned int maxDropPeriod);
        void SetIncomingPacketDrop(unsigned int minDropInterval, unsigned int maxDropInterval, unsigned int minDropPeriod, unsigned int maxDropPeriod);

        void GetOutgoingPacketDrop(unsigned int& minDropInterval, unsigned int& maxDropInterval, unsigned int& minDropPeriod, unsigned int& maxDropPeriod);
        void GetIncomingPacketDrop(unsigned int& minDropInterval, unsigned int& maxDropInterval, unsigned int& minDropPeriod, unsigned int& maxDropPeriod);

        // Enable packet reordering you need to enable latency to reorder packets.
        void SetOutgoingReorder(bool enable)    { m_outReorder = enable; }
        void SetIncomingReorder(bool enable)    { m_inReorder = enable; }
        bool IsOutgoingReorder() const          { return m_outReorder; }
        bool IsIncomingReorder() const          { return m_inReorder; }

    protected:
        struct Packet
        {
            char*                   m_data;
            unsigned int            m_dataSize;

            AZStd::intrusive_ptr<DriverAddress> m_address;  // To or From address
            TimeStamp               m_startTime;
            unsigned int            m_latency;  /// time to action from startTime mark in milliseconds.
        };

        AZStd::mutex    m_lock;

        /// Free all internal data
        void FreeAllData();

        volatile bool   m_enable;

        // Latency in milliseconds
        unsigned int    m_minOutLatency;
        unsigned int    m_maxOutLatency;
        unsigned int    m_minInLatency;
        unsigned int    m_maxInLatency;

        // Packet loss 1 every X packets
        unsigned int    m_minOutPacketLoss;
        unsigned int    m_maxOutPacketLoss;
        volatile bool   m_isOutPacketLoss;
        unsigned int    m_minInPacketLoss;
        unsigned int    m_maxInPacketLoss;
        volatile bool   m_isInPacketLoss;

        unsigned int    m_numOutPacketsTillDrop;
        unsigned int    m_numInPacketsTillDrop;

        // Packet loss drop for X ms every Y ms
        unsigned int    m_minInPacketDropInterval;  ///< Min interval to drop packets in milliseconds for inbound packets.
        unsigned int    m_maxInPacketDropInterval;  ///< Max interval to drop packets in milliseconds for inbound packets.
        unsigned int    m_minInPacketDropPeriod;    ///< Min period for packet drop in milliseconds for inbound packets. We drop packets for DropInterval every DropPeriod.
        unsigned int    m_maxInPacketDropPeriod;    ///< Max -- " --
        unsigned int    m_minOutPacketDropInterval; ///< Min interval to drop packets in milliseconds for outbound packets.
        unsigned int    m_maxOutPacketDropInterval; ///< Max -- " --
        unsigned int    m_minOutPacketDropPeriod;   ///< Min period for packet drop in milliseconds for inbound packets. We drop packets for DropInterval every DropPeriod.
        unsigned int    m_maxOutPacketDropPeriod;   ///< Max -- " --

        unsigned int    m_inPacketDropInterval;     ///< If interval is != 0 we are currently dropping packets. Interval value is in milliseconds.
        unsigned int    m_inPacketDropPeriod;       ///< Milliseconds left until net simulation interval in milliseconds inbound.
        unsigned int    m_outPacketDropInterval;    ///< If interval is != 0 we are currently dropping packets. Interval value is in milliseconds.
        unsigned int    m_outPacketDropPeriod;      ///< Milliseconds left until net simulation interval in milliseconds outbound.

        // Bandwidth in Kbps
        unsigned int    m_minOutBandwidth;
        unsigned int    m_maxOutBandwidth;
        unsigned int    m_minInBandwidth;
        unsigned int    m_maxInBandwidth;

        unsigned int    m_currentDataOut;           ///< How much data have we send since m_dataLimiterTimeout was reset in bytes.
        unsigned int    m_currentDataOutMax;        ///< What is the current output data limit till m_dataLimiterTimeout is reset in bytes.
        unsigned int    m_currentDataIn;            ///< How much data have we received since m_dataLimiterTimeout was reset in bytes.
        unsigned int    m_currentDataInMax;         ///< What is the current incoming data limit till m_dataLimiterTimeout is reset in bytes.

        unsigned int    m_dataLimiterTimeout;       ///< Time since we counting currentDataXXX limits.

        volatile bool   m_outReorder;
        volatile bool   m_inReorder;

        typedef list<Packet> PacketListType;

        PacketListType  m_outgoing;
        PacketListType  m_incoming;

        TimeStamp       m_currentTime;              ///< Current time. (replace this with a global clock when possible.)

        class Driver*   m_driver;
    };
}

#endif // GM_DEFAULT_SIMULATOR_H
