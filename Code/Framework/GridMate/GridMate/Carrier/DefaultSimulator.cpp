/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Carrier/DefaultSimulator.h>
#include <GridMate/Carrier/Driver.h>

#include <AzCore/std/parallel/lock.h>

#include <AzCore/Math/Crc.h>

#include <stdlib.h>

using namespace GridMate;

//=========================================================================
// DefaultSimulator
// [10/15/2010]
//=========================================================================
DefaultSimulator::DefaultSimulator()
{
    // Latency in milliseconds
    m_minOutLatency = 0;
    m_maxOutLatency = 0;
    m_minInLatency = 0;
    m_maxInLatency = 0;

    // Packet loss in %
    m_minOutPacketLoss = 0;
    m_maxOutPacketLoss = 0;
    m_minInPacketLoss = 0;
    m_maxInPacketLoss = 0;
    m_isOutPacketLoss = false;
    m_isInPacketLoss = false;

    m_numOutPacketsTillDrop = 0;
    m_numInPacketsTillDrop = 0;

    // Packet loss in periods
    m_minInPacketDropInterval = 0;
    m_maxInPacketDropInterval = 0;
    m_minInPacketDropPeriod = 0;
    m_maxInPacketDropPeriod = 0;
    m_minOutPacketDropInterval = 0;
    m_maxOutPacketDropInterval = 0;
    m_minOutPacketDropPeriod = 0;
    m_maxOutPacketDropPeriod = 0;

    m_inPacketDropInterval = 0;
    m_inPacketDropPeriod = 0;
    m_outPacketDropInterval = 0;
    m_outPacketDropPeriod = 0;

    // Bandwidth in Kbps
    m_minOutBandwidth = 0;
    m_maxOutBandwidth = 0;
    m_minInBandwidth = 0;
    m_maxInBandwidth = 0;

    m_currentDataOut = 0;
    m_currentDataOutMax = 0;
    m_currentDataIn = 0;
    m_currentDataInMax = 0;

    m_dataLimiterTimeout = 0;

    m_outReorder = false;
    m_inReorder = false;

    m_enable = false;

    m_driver = nullptr;
}

//=========================================================================
// ~DefaultSimulator
// [10/15/2010]
//=========================================================================
DefaultSimulator::~DefaultSimulator()
{
    FreeAllData();
}

void DefaultSimulator::BindDriver(class Driver* driver)
{
    m_driver = driver;
}

void DefaultSimulator::UnbindDriver()
{
    FreeAllData();
    m_driver = nullptr;
}

//=========================================================================
// OnConnect
// [10/15/2010]
//=========================================================================
void
DefaultSimulator::OnConnect(const AZStd::intrusive_ptr<DriverAddress>& address)
{
    (void)address;
}

//=========================================================================
// OnDisconnect
// [10/15/2010]
//=========================================================================
void
DefaultSimulator::OnDisconnect(const AZStd::intrusive_ptr<DriverAddress>& address)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);

    for (PacketListType::iterator pakIter = m_outgoing.begin(); pakIter != m_outgoing.end(); )
    {
        Packet& pak = *pakIter;
        if (pak.m_address == address)
        {
            azfree(pak.m_data, GridMateAllocatorMP);
            pakIter = m_outgoing.erase(pakIter);
        }
        else
        {
            ++pakIter;
        }
    }
    for (PacketListType::iterator pakIter = m_incoming.begin(); pakIter != m_incoming.end(); )
    {
        Packet& pak = *pakIter;
        if (pak.m_address == address)
        {
            azfree(pak.m_data, GridMateAllocatorMP);
            pakIter = m_incoming.erase(pakIter);
        }
        else
        {
            ++pakIter;
        }
    }
}

//=========================================================================
// OnSend
// [10/15/2010]
//=========================================================================
bool
DefaultSimulator::OnSend(const AZStd::intrusive_ptr<DriverAddress>& to, const void* data, unsigned int dataSize)
{
    if (!m_enable)
    {
        return false;
    }

    {
        AZStd::lock_guard<AZStd::mutex> l(m_lock);

        if (m_isOutPacketLoss)
        {
            if (m_numOutPacketsTillDrop == 0)
            {
                m_numOutPacketsTillDrop = static_cast<unsigned int>((double)rand() / ((double)RAND_MAX + 1) * (m_maxOutPacketLoss - m_minOutPacketLoss) + m_minOutPacketLoss);
                return true; // we handle the packet and do nothing ;)
            }
            else
            {
                m_numOutPacketsTillDrop--;
            }
        }

        if (m_outPacketDropInterval != 0)
        {
            return true; // If we are dropping outbound packets due to time restriction just drop the packet
        }

        if (m_currentDataOutMax != 0)
        {
            m_currentDataOut += dataSize;
            if (m_currentDataOut > m_currentDataOutMax)
            {
                return true; // we are over the limit drop the packet
            }
        }

        if (m_maxOutLatency > 0)
        {
            int latency = static_cast<int>(static_cast<double>(rand()) / (static_cast<double>(RAND_MAX) + 1) * (m_maxOutLatency - m_minOutLatency) + m_minOutLatency);
            if (latency > 0)
            {
                Packet pak;
                pak.m_data = reinterpret_cast<char*>(azmalloc(dataSize, 1, GridMateAllocatorMP));
                pak.m_dataSize = dataSize;
                memcpy(pak.m_data, data, dataSize);
                pak.m_latency = latency;
                pak.m_address = to;
                pak.m_startTime = AZStd::chrono::system_clock::now();

                if (m_outReorder)
                {
                    // insert at random place
                    PacketListType::iterator pakIter = m_outgoing.begin();
                    int pos = rand() % (m_outgoing.size() + 1);
                    for (; pos > 0; --pos)
                    {
                        ++pakIter;
                    }
                    m_outgoing.insert(pakIter, pak);
                }
                else
                {
                    m_outgoing.push_back(pak);
                }
                return true;
            }
        }
    }

    return false;
}

//=========================================================================
// OnReceive
// [10/15/2010]
//=========================================================================
bool
DefaultSimulator::OnReceive(const AZStd::intrusive_ptr<DriverAddress>& from, const void* data, unsigned int dataSize)
{
    if (!m_enable)
    {
        return false;
    }

    {
        AZStd::lock_guard<AZStd::mutex> l(m_lock);
        if (m_isInPacketLoss)
        {
            if (m_numInPacketsTillDrop == 0)
            {
                m_numInPacketsTillDrop = static_cast<unsigned int>((double)rand() / ((double)RAND_MAX + 1) * (m_maxInPacketLoss - m_minInPacketLoss)    + m_minInPacketLoss);
                return true; // we handle the packet and do nothing ;)
            }
            else
            {
                m_numInPacketsTillDrop--;
            }
        }

        if (m_inPacketDropInterval != 0)
        {
            return true; // If we are dropping outbound packets due to time restriction just drop the packet
        }

        if (m_currentDataInMax != 0)
        {
            m_currentDataIn += dataSize;
            if (m_currentDataIn > m_currentDataInMax)
            {
                return true; // we are over the limit drop the packet
            }
        }

        if (m_maxInLatency > 0)
        {
            int latency = static_cast<int>(static_cast<double>(rand()) / (static_cast<double>(RAND_MAX) + 1) * (m_maxInLatency - m_minInLatency)    + m_minInLatency);
            if (latency > 0)
            {
                Packet pak;
                pak.m_data =  reinterpret_cast<char*>(azmalloc(dataSize, 1, GridMateAllocatorMP));
                pak.m_dataSize = dataSize;
                memcpy(pak.m_data, data, dataSize);
                pak.m_latency = latency;
                pak.m_address = from;
                pak.m_startTime = AZStd::chrono::system_clock::now();

                if (m_inReorder)
                {
                    // insert at random place
                    PacketListType::iterator pakIter = m_incoming.begin();
                    int pos = rand() % (m_incoming.size() + 1);
                    for (; pos > 0; --pos)
                    {
                        ++pakIter;
                    }
                    m_incoming.insert(pakIter, pak);
                }
                else
                {
                    m_incoming.push_back(pak);
                }
                return true;
            }
        }

        if (m_maxOutBandwidth > 0)
        {
        }
    }

    return false;
}

//=========================================================================
// ReceiveDataFrom
// [10/15/2010]
//=========================================================================
unsigned int
DefaultSimulator::ReceiveDataFrom(AZStd::intrusive_ptr<DriverAddress>& from, char* data, unsigned int maxDataSize)
{
    (void)maxDataSize;
    if (!m_enable && m_incoming.empty())
    {
        return 0;
    }

    unsigned int dataSize = 0;
    {
        AZStd::lock_guard<AZStd::mutex> l(m_lock);

        if (!m_incoming.empty())
        {
            Packet& pak = m_incoming.front();
            AZStd::chrono::milliseconds elapsed = AZStd::chrono::system_clock::now() - pak.m_startTime;
            if (!m_enable || elapsed.count() >= pak.m_latency)
            {
                from = pak.m_address;
                AZ_Assert(maxDataSize >= pak.m_dataSize, "Buffer to receive data is too small");
                memcpy(data, pak.m_data, pak.m_dataSize);
                azfree(pak.m_data, GridMateAllocatorMP);

                dataSize = pak.m_dataSize;

                m_incoming.pop_front();
            }
        }
    }
    return dataSize;
}

//=========================================================================
// Update
// [10/15/2010]
//=========================================================================
void
DefaultSimulator::Update()
{
    if (!m_enable && m_outgoing.empty())
    {
        return;
    }
    {
        //
        TimeStamp now = AZStd::chrono::system_clock::now();
        //////////////////////////////////////////////////////////////////////////
        // Or we can deliver this from the engine
        unsigned int deltaTime = static_cast<unsigned int>(AZStd::chrono::milliseconds(now - m_currentTime).count());
        deltaTime = AZStd::GetMin<unsigned int>(deltaTime, 100);
        //////////////////////////////////////////////////////////////////////////
        m_currentTime = now;

        AZStd::lock_guard<AZStd::mutex> l(m_lock);
        for (PacketListType::iterator pakIter = m_outgoing.begin(); pakIter != m_outgoing.end(); )
        {
            Packet& pak = *pakIter;
            AZStd::chrono::milliseconds elapsed = now - pak.m_startTime;
            if (!m_enable || elapsed.count() >= pak.m_latency)
            {
                if (m_driver->Send(pak.m_address, pak.m_data, pak.m_dataSize) == Driver::EC_OK)
                {
                    azfree(pak.m_data, GridMateAllocatorMP);
                    pakIter = m_outgoing.erase(pakIter);
                    continue;
                }
            }
            // we don't want to send the next packet (or anything after it),
            // so break out of the loop.
            break;
        }

        if (m_maxInPacketDropPeriod != 0 || m_maxOutPacketDropPeriod != 0)
        {
            if (m_inPacketDropPeriod > deltaTime)
            {
                m_inPacketDropPeriod -= deltaTime;
                if (m_inPacketDropInterval > deltaTime)
                {
                    m_inPacketDropInterval -= deltaTime;
                }
                else
                {
                    m_inPacketDropInterval = 0;
                }
            }
            else if (m_maxInPacketDropPeriod != 0 && m_maxInPacketDropInterval != 0)
            {
                // period has expired compute a new period
                m_inPacketDropPeriod = static_cast<unsigned int>((double)rand() / ((double)RAND_MAX + 1) * (m_maxInPacketDropPeriod - m_minInPacketDropPeriod) + m_minInPacketDropPeriod);
                m_inPacketDropInterval = static_cast<unsigned int>((double)rand() / ((double)RAND_MAX + 1) * (m_maxInPacketDropInterval - m_minInPacketDropInterval) + m_minInPacketDropInterval);
                if (m_inPacketDropInterval > m_inPacketDropPeriod)
                {
                    m_inPacketDropInterval = m_inPacketDropPeriod; // at worst we can drop all the packets for the period
                }
            }

            if (m_outPacketDropPeriod > deltaTime)
            {
                m_outPacketDropPeriod -= deltaTime;
                if (m_outPacketDropInterval > deltaTime)
                {
                    m_outPacketDropInterval -= deltaTime;
                }
                else
                {
                    m_outPacketDropInterval = 0;
                }
            }
            else if (m_maxOutPacketDropPeriod != 0 && m_maxOutPacketDropInterval != 0)
            {
                // period has expired compute a new period
                m_outPacketDropPeriod = static_cast<unsigned int>((double)rand() / ((double)RAND_MAX + 1) * (m_maxOutPacketDropPeriod - m_minOutPacketDropPeriod) + m_minOutPacketDropPeriod);
                m_outPacketDropInterval = static_cast<unsigned int>((double)rand() / ((double)RAND_MAX + 1) * (m_maxOutPacketDropInterval - m_minOutPacketDropInterval) + m_minOutPacketDropInterval);
                if (m_outPacketDropInterval > m_outPacketDropPeriod)
                {
                    m_outPacketDropInterval = m_outPacketDropPeriod; // at worst we can drop all the packets for the period
                }
            }
        }

        if (m_maxOutBandwidth != 0 || m_maxInBandwidth != 0)
        {
            m_dataLimiterTimeout += deltaTime;

            if (m_dataLimiterTimeout > 1000)  // every second
            {
                m_dataLimiterTimeout -= 1000;

                m_currentDataOut = 0;
                m_currentDataIn = 0;
                if (m_maxOutBandwidth != 0)
                {
                    m_currentDataOutMax = static_cast<unsigned int>((double)rand() / ((double)RAND_MAX + 1) * (m_maxOutBandwidth - m_minOutBandwidth) + m_minOutBandwidth);
                    m_currentDataOutMax = m_currentDataOutMax / 8 * 1024; // Convert to bytes per second
                }
                if (m_maxInBandwidth)
                {
                    m_currentDataInMax = static_cast<unsigned int>((double)rand() / ((double)RAND_MAX + 1) * (m_maxInBandwidth - m_minInBandwidth) + m_minInBandwidth);
                    m_currentDataInMax = m_currentDataInMax / 8 * 1024; // Convert to bytes per second
                }
            }
        }
    }
}

//=========================================================================
// FreeAllData
// [10/15/2010]
//=========================================================================
void
DefaultSimulator::FreeAllData()
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    while (!m_outgoing.empty())
    {
        Packet& pak = m_outgoing.front();
        azfree(pak.m_data, GridMateAllocatorMP);
        m_outgoing.pop_front();
    }
    while (!m_incoming.empty())
    {
        Packet& pak = m_incoming.front();
        azfree(pak.m_data, GridMateAllocatorMP);
        m_incoming.pop_front();
    }
}

//=========================================================================
// SetOutgoingLatency
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::SetOutgoingLatency(unsigned int minDelayMS, unsigned int maxDelayMS)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    m_minOutLatency = minDelayMS;
    m_maxOutLatency = maxDelayMS;
}
//=========================================================================
// SetIncomingLatency
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::SetIncomingLatency(unsigned int minDelayMS, unsigned int maxDelayMS)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    m_minInLatency = minDelayMS;
    m_maxInLatency = maxDelayMS;
}

//=========================================================================
// GetOutgoingLatency
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::GetOutgoingLatency(unsigned int& minDelayMS, unsigned int& maxDelayMS)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    minDelayMS = m_minOutLatency;
    maxDelayMS = m_maxOutLatency;
}
//=========================================================================
// GetIncomingLatency
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::GetIncomingLatency(unsigned int& minDelayMS, unsigned int& maxDelayMS)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    minDelayMS = m_minInLatency;
    maxDelayMS = m_maxInLatency;
}

//=========================================================================
// SetOutgoingPacketLoss
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::SetOutgoingPacketLoss(unsigned int minInterval, unsigned int maxInterval)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);

    if (minInterval > 0)
    {
        m_minOutPacketLoss = minInterval - 1;
    }

    if (maxInterval > 0)
    {
        m_maxOutPacketLoss = maxInterval - 1;
        m_isOutPacketLoss = true;
    }
    else
    {
        m_isOutPacketLoss = false;
    }
}

//=========================================================================
// SetIncomingPacketLoss
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::SetIncomingPacketLoss(unsigned int minInterval, unsigned int maxInterval)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);

    if (minInterval > 0)
    {
        m_minInPacketLoss = minInterval - 1;
    }

    if (maxInterval > 0)
    {
        m_maxInPacketLoss = maxInterval - 1;
        m_isInPacketLoss = true;
    }
    else
    {
        m_isInPacketLoss = false;
    }
}

//=========================================================================
// GetOutgoingPacketLoss
// [5/25/2011]
//=========================================================================
void
DefaultSimulator::GetOutgoingPacketLoss(unsigned int& minInterval, unsigned int& maxInterval)
{
    if (m_isOutPacketLoss)
    {
        AZStd::lock_guard<AZStd::mutex> l(m_lock);
        minInterval = m_minOutPacketLoss + 1;
        maxInterval = m_maxOutPacketLoss + 1;
    }
    else
    {
        minInterval = 0;
        maxInterval = 0;
    }
}

//=========================================================================
// GetIncomingPacketLoss
// [5/25/2011]
//=========================================================================
void
DefaultSimulator::GetIncomingPacketLoss(unsigned int& minInterval, unsigned int& maxInterval)
{
    if (m_isInPacketLoss)
    {
        AZStd::lock_guard<AZStd::mutex> l(m_lock);
        minInterval = m_minInPacketLoss + 1;
        maxInterval = m_maxInPacketLoss + 1;
    }
    else
    {
        minInterval = 0;
        maxInterval = 0;
    }
}

//=========================================================================
// SetOutgoingBandwidth
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::SetOutgoingBandwidth(unsigned int minBandwidthKbps, unsigned int maxBandwidthKbps)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    m_minOutBandwidth = minBandwidthKbps;
    m_maxOutBandwidth = maxBandwidthKbps;
}
//=========================================================================
// SetIncomingBandwidth
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::SetIncomingBandwidth(unsigned int minBandwidthKbps, unsigned int maxBandwidthKbps)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    m_minInBandwidth = minBandwidthKbps;
    m_maxInBandwidth = maxBandwidthKbps;
}

//=========================================================================
// GetOutgoingBandwidth
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::GetOutgoingBandwidth(unsigned int& minBandwidthKbps, unsigned int& maxBandwidthKbps)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    minBandwidthKbps = m_minOutBandwidth;
    maxBandwidthKbps = m_maxOutBandwidth;
}
//=========================================================================
// GetIncomingBandwidth
// [10/18/2010]
//=========================================================================
void
DefaultSimulator::GetIncomingBandwidth(unsigned int& minBandwidthKbps, unsigned int& maxBandwidthKbps)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    minBandwidthKbps = m_minInBandwidth;
    maxBandwidthKbps = m_maxInBandwidth;
}

//=========================================================================
// SetOutgoingPacketDrop
// [10/21/2013]
//=========================================================================
void
DefaultSimulator::SetOutgoingPacketDrop(unsigned int minDropInterval, unsigned int maxDropInterval, unsigned int minDropPeriod, unsigned int maxDropPeriod)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    m_minOutPacketDropInterval = minDropInterval;
    m_maxOutPacketDropInterval = maxDropInterval;
    m_minOutPacketDropPeriod = minDropPeriod;
    m_maxOutPacketDropPeriod = maxDropPeriod;

    m_outPacketDropInterval = 0;
    m_outPacketDropPeriod = 0;
}

//=========================================================================
// SetIncomingPacketDrop
// [10/21/2013]
//=========================================================================
void
DefaultSimulator::SetIncomingPacketDrop(unsigned int minDropInterval, unsigned int maxDropInterval, unsigned int minDropPeriod, unsigned int maxDropPeriod)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    m_minInPacketDropInterval = minDropInterval;
    m_maxInPacketDropInterval = maxDropInterval;
    m_minInPacketDropPeriod = minDropPeriod;
    m_maxInPacketDropPeriod = maxDropPeriod;

    m_inPacketDropInterval = 0;
    m_inPacketDropPeriod = 0;
}

//=========================================================================
// GetOutgoingPacketDrop
// [10/21/2013]
//=========================================================================
void DefaultSimulator::GetOutgoingPacketDrop(unsigned int& minDropInterval, unsigned int& maxDropInterval, unsigned int& minDropPeriod, unsigned int& maxDropPeriod)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    minDropInterval = m_minOutPacketDropInterval;
    maxDropInterval = m_maxOutPacketDropInterval;
    minDropPeriod = m_minOutPacketDropPeriod;
    maxDropPeriod = m_maxOutPacketDropPeriod;
}

//=========================================================================
// GetIncomingPacketDrop
// [10/21/2013]
//=========================================================================
void DefaultSimulator::GetIncomingPacketDrop(unsigned int& minDropInterval, unsigned int& maxDropInterval, unsigned int& minDropPeriod, unsigned int& maxDropPeriod)
{
    AZStd::lock_guard<AZStd::mutex> l(m_lock);
    minDropInterval = m_minInPacketDropInterval;
    maxDropInterval = m_maxInPacketDropInterval;
    minDropPeriod = m_minInPacketDropPeriod;
    maxDropPeriod = m_maxInPacketDropPeriod;
}
