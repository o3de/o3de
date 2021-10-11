/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Carrier/DefaultTrafficControl.h>
#include <GridMate/Carrier/Carrier.h>
#include <GridMate/Carrier/Driver.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/string/string.h>

using namespace GridMate;

//#define GRIDMATE_FIXED_RATE_BYTES 1000000
//#define VERBOSE_DISCONNECT_DEBUGGING

//////////////////////////////////////////////////////////////////////////
// TCP Cubic parameters
const double DefaultTrafficControl::ConnectionData::k_CubicBeta     = 0.2;                                  ///< beta (backoff rate) for congestion calculations
const double DefaultTrafficControl::ConnectionData::k_CubicAlpha    = 3 * k_CubicBeta / (2 - k_CubicBeta);  ///< alpha for TCP friendly window estimation
const double DefaultTrafficControl::ConnectionData::k_CubicScaleC   = 0.4;                                  ///< Scaling factor. COTS default (0.4)
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// DefaultTrafficControl
// [10/5/2010]
//=========================================================================
DefaultTrafficControl::DefaultTrafficControl(unsigned int maxSystemPacketSize, float rttConnectionThreshold, float packetLossThreshold, unsigned int maxRecvPackets)
    : m_maxPacketSize(maxSystemPacketSize)
    , m_rttConnectionThreshold(rttConnectionThreshold)  /// in milliseconds
    , m_packetLossThreshold(packetLossThreshold)        /// percent 1.0 is 100%
    , m_lastStatDataReset(0.0f)
    , m_lostPacketTimeoutMS(1000)
    , m_defaultMaxCongestionWindowSize(1000000)
    , m_maxRecvPackets(maxRecvPackets)
{
    AZ_Assert(maxSystemPacketSize >= 256, "Maximum system packet is too small!");
    m_currentTime = AZStd::chrono::system_clock::now();
}

//=========================================================================
// DefaultTrafficControl
// [10/5/2010]
//=========================================================================
DefaultTrafficControl::~DefaultTrafficControl()
{
}

//=========================================================================
// OnConnect
// [10/5/2010]
//=========================================================================
void
DefaultTrafficControl::OnConnect(TrafficControlConnectionId id, const AZStd::intrusive_ptr<DriverAddress>& address)
{
    AZ_Assert(id->m_trafficData == nullptr, "We have already assigned traffic data to this connection!");
    if (id->m_trafficData != nullptr)
    {
        return;
    }

    ConnectionData cd;

    //cd.mode = ConnectionData::Bad;
    //cd.penaltyTime = 4.0f;

    // to avoid measuring stats while handshake is done, get a stamp in the future (after any possible handshake)
    cd.m_handshakeDone = m_currentTime + AZStd::chrono::minutes(60);
    cd.m_isReceivedDataAfterLastSend = false;
    cd.m_address = address->ToAddress();
    cd.m_recvPacketAllowance = m_maxRecvPackets;
    cd.m_canReceiveData = true;

    cd.m_lastAckSend = m_currentTime;

    //////////////////////////////////////////////////////////////////////////
    // Slow start traffic control
    cd.m_lastWindowSizeIncrease = m_currentTime;
    cd.m_lastWindowSizeDecrease = m_currentTime;
    cd.m_inTransfer = 0;
    cd.m_slowStartThreshold = 0;
    cd.m_congestionWindow = m_maxPacketSize;
    cd.m_maxCongestionWindow = m_defaultMaxCongestionWindowSize;
    //////////////////////////////////////////////////////////////////////////

    m_connections.push_back(cd);
    id->m_trafficData = &m_connections.back();
}

//=========================================================================
// OnDisconnect
// [10/12/2010]
//=========================================================================
void
DefaultTrafficControl::OnDisconnect(TrafficControlConnectionId id)
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
    id->m_trafficData = nullptr;
    bool isFound = false;
    for (ConnectionListType::iterator i = m_connections.begin(); i != m_connections.end(); ++i)
    {
        if (&*i == cd)
        {
            m_connections.erase(i);
            isFound = true;
            break;
        }
    }
    (void)isFound;
    AZ_Assert(isFound, "Traffic control data is NOT in the list!");
}

//=========================================================================
// OnHandshakeComplete
// [2/22/2011]
//=========================================================================
void
DefaultTrafficControl::OnHandshakeComplete(TrafficControlConnectionId id)
{
    // because the NAT punch can stall the connection, out packed loss
    // can be really high after the connection is established. This can
    // produce an bad connection situation.
    // for now we just reset the stats. Another option will be to just not update stats at all.
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
    cd->m_handshakeDone = m_currentTime;
}

//=========================================================================
// OnSend
// [10/5/2010]
//=========================================================================
void
DefaultTrafficControl::OnSend(TrafficControlConnectionId id, DataGramControlData& info)
{
    info.m_time = m_currentTime;

    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);

    cd->m_sdCurrentSecond.m_dataSend += info.m_size;
    cd->m_sdCurrentSecond.m_packetSend++;
    if (info.m_effectiveSize)
    {
        cd->m_sdEffectiveCurrentSecond.m_dataSend += info.m_effectiveSize;
        cd->m_sdEffectiveCurrentSecond.m_packetSend++;
    }
    cd->m_lastPacketSend = info.m_time;
    cd->m_isReceivedDataAfterLastSend = false;

    cd->m_inTransfer += info.m_size;

#if defined(AZ_DEBUG_BUILD)
    //{
    //    static GridMate::TimeStamp last_time = m_currentTime;
    //    if( (m_currentTime - last_time).count() > 1000000) //1000ms
    //    {
    //        last_time = m_currentTime;
    //        AZ_TracePrintf("GridMate", "Sent: CWND %u sent %u rcvd %u\n",
    //           cd->m_congestionWindow, cd->m_sdLifetime.m_packetSend, cd->m_sdLifetime.m_packetReceived);
    //    }
    //}
#endif
}

//=========================================================================
// OnSendAck
// [8/1/2012]
//=========================================================================
void
DefaultTrafficControl::OnSendAck(TrafficControlConnectionId id)
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
    cd->m_lastAckSend = m_currentTime;
}

//=========================================================================
// OnAck
// [10/5/2010]
//=========================================================================
void
DefaultTrafficControl::OnAck(TrafficControlConnectionId id, DataGramControlData& info, bool& windowChanged)
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
    cd->m_sdCurrentSecond.m_dataAcked += info.m_size;
    cd->m_sdCurrentSecond.m_packetAcked++;
    if (info.m_effectiveSize)
    {
        cd->m_sdEffectiveCurrentSecond.m_dataAcked += info.m_effectiveSize;
        cd->m_sdEffectiveCurrentSecond.m_packetAcked++;
    }

    if (info.m_time >= cd->m_handshakeDone)   // we measure after the handshake is done \ref OnHandshakeComplete
    {
        // compute the packet rtt
        AZStd::chrono::microseconds rtt =  m_currentTime - info.m_time;
        cd->m_sdCurrentSecond.m_rtt = (cd->m_sdCurrentSecond.m_rtt + .001f * rtt.count()) * 0.5f;
        if (info.m_effectiveSize)
        {
            cd->m_sdEffectiveCurrentSecond.m_rtt = (cd->m_sdEffectiveCurrentSecond.m_rtt + .001f * rtt.count()) * 0.5f;
        }
    }

    AZ_Assert(cd->m_inTransfer >= info.m_size, "Invalid data size");
    cd->m_inTransfer -= info.m_size;

    // Traffic control - packets are getting acked, increase rate
    if (m_currentTime != cd->m_lastWindowSizeIncrease)
    {
        cd->m_lastWindowSizeIncrease = m_currentTime;

        bool isSlowStart = cd->m_congestionWindow <= cd->m_slowStartThreshold || cd->m_slowStartThreshold == 0;
        if (isSlowStart)
        {
            cd->m_congestionWindow = AZStd::GetMin(cd->m_congestionWindow * 2, cd->m_maxCongestionWindow);
            if (cd->m_congestionWindow > cd->m_slowStartThreshold && cd->m_slowStartThreshold != 0)
            {
                cd->m_congestionWindow = cd->m_slowStartThreshold;
                if (cd->k_enableCubic)
                {
                    cd->m_congestionWindow += m_maxPacketSize;
                    cd->TCPCubicExitSlowStart(m_currentTime, m_maxPacketSize);
                }
                else
                {
                    cd->m_congestionWindow += m_maxPacketSize * m_maxPacketSize / cd->m_congestionWindow;
                }
            }
        }
        else
        {
            if (cd->k_enableCubic)
            {
                cd->m_congestionWindow = cd->TCPCubicWindow(m_currentTime, m_maxPacketSize);
            }
            else
            {
                cd->m_congestionWindow += m_maxPacketSize * m_maxPacketSize / cd->m_congestionWindow;
            }
        }

        cd->m_congestionWindow = AZStd::GetMin(cd->m_congestionWindow, cd->m_maxCongestionWindow);

#if defined(GRIDMATE_FIXED_RATE_BYTES) && GRIDMATE_FIXED_RATE_BYTES > 0
        //const float rtt = cd->m_sdLifetime.m_rtt > 1.f ? cd->m_sdLifetime.m_rtt : 100.f;
        cd->m_sdLifetime.m_rtt = 100;
        cd->m_congestionWindow = static_cast<unsigned int>(GRIDMATE_FIXED_RATE_BYTES / 10);
#endif
        windowChanged = true;
    }
    else
    {
        windowChanged = false;
    }
}

//=========================================================================
// OnNAck
// [7/30/2012]
//=========================================================================
void
DefaultTrafficControl::OnNAck(TrafficControlConnectionId id, DataGramControlData& info)
{
    (void)id;
    // If we get N number of NAck consider that packet lost. N should be not 1 to allow for packet
    // reordering.
    const unsigned int N = 3;
    info.m_time -= AZStd::chrono::milliseconds(m_lostPacketTimeoutMS / N); // decrease the time for each packet, so after N NACKs we consider the packet lost.
}

//=========================================================================
// OnReceived
// [11/15/2010]
//=========================================================================
void
DefaultTrafficControl::OnReceived(TrafficControlConnectionId id, DataGramControlData& info)
{
    (void)info;
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);

    cd->m_sdCurrentSecond.m_dataReceived += info.m_size;
    cd->m_sdCurrentSecond.m_packetReceived++;
    if (info.m_effectiveSize)
    {
        cd->m_sdEffectiveCurrentSecond.m_dataReceived += info.m_effectiveSize;
        cd->m_sdEffectiveCurrentSecond.m_packetReceived++;
        cd->m_isReceivedDataAfterLastSend = true;
    }

    if (m_maxRecvPackets != 0)
    {
        --cd->m_recvPacketAllowance;
        if (cd->m_recvPacketAllowance == 0) // hit the limit
        {
            cd->m_canReceiveData = false;
        }
    }

#if defined(AZ_DEBUG_BUILD)
    //{
    //    static GridMate::TimeStamp last_time = m_currentTime;
    //    if ((m_currentTime - last_time).count() > 1000000) //1000ms
    //    {
    //        last_time = m_currentTime;
    //        AZ_TracePrintf("GridMate", "Rcvd: CWND %u sent %u rcvd %u\n",
    //            cd->m_congestionWindow, cd->m_sdLifetime.m_packetSend, cd->m_sdLifetime.m_packetReceived);
    //    }
    //}
#endif
}

//=========================================================================
// IsSend
// [10/5/2010]
//=========================================================================
bool
DefaultTrafficControl::IsSend(TrafficControlConnectionId id)
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);

    return cd->m_inTransfer <= cd->m_congestionWindow;
}

//=========================================================================
// IsSendAck
// [8/1/2012]
//=========================================================================
bool
DefaultTrafficControl::IsSendAck(TrafficControlConnectionId id)
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
    if (cd->m_lastAckSend != m_currentTime)
    {
        return true;
    }
    return false;
}

//=========================================================================
// GetAvailableWindowSize
// [10/6/2010]
//=========================================================================
unsigned int
DefaultTrafficControl::GetAvailableWindowSize(TrafficControlConnectionId id) const
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
    return cd->m_congestionWindow - cd->m_inTransfer;
}

TimeStamp
DefaultTrafficControl::GetResendTime(TrafficControlConnectionId id, const DataGramControlData& info)
{
    (void)id;
    return info.m_time + AZStd::chrono::milliseconds(m_lostPacketTimeoutMS + 1);
}
//=========================================================================
// IsResend
// [10/5/2010]
//=========================================================================
bool
DefaultTrafficControl::IsResend(TrafficControlConnectionId id, const DataGramControlData& info, unsigned int resendDataSize)
{
    AZStd::chrono::milliseconds timeElapsed = m_currentTime - info.m_time;

    // Consider a packet lost after certain amount of time.
    if (timeElapsed.count() > m_lostPacketTimeoutMS)
    {
        ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
        if (info.m_time >= cd->m_handshakeDone)   // we measure after the handshake is done \ref OnHandshakeComplete
        {
            cd->m_sdCurrentSecond.m_packetLost++;
            // how should we affect the rtt when a packet is lost ?
            //cd->sdCurrentSecond.rtt = (cd->sdCurrentSecond.rtt + static_cast<float>(timeElapsed.count())) * 0.5f;
            if (resendDataSize)
            {
                cd->m_sdEffectiveCurrentSecond.m_packetLost++;
            }

            if (cd->k_enableCubic)
            {
                cd->TCPCubicPacketLost(m_currentTime, m_maxPacketSize);
            }
            else
            {
                // Traffic control - we lost a packet, send less data
                if (m_currentTime != cd->m_lastWindowSizeDecrease)
                {
                    cd->m_lastWindowSizeDecrease = m_currentTime;
                    //cd->m_lastWindowSizeIncrease = m_currentTime; // don't allow increase
                    //cd->m_slowStartThreshold = AZStd::GetMax(cd->m_congestionWindow/2,m_maxPacketSize);
                    //cd->m_congestionWindow = m_maxPacketSize;
                }
            }
        }
        cd->m_inTransfer -= info.m_size;

#if defined(GRIDMATE_FIXED_RATE_BYTES) && GRIDMATE_FIXED_RATE_BYTES > 0
        //const float rtt = cd->m_sdLifetime.m_rtt > 1.f ? cd->m_sdLifetime.m_rtt : 100.f;
        cd->m_sdLifetime.m_rtt = 100;
        cd->m_congestionWindow = static_cast<unsigned int>(GRIDMATE_FIXED_RATE_BYTES / 10);
#endif

        return true;
    }

    return false;
}

//=========================================================================
// OnResend
// [10/5/2010]
//=========================================================================
void
DefaultTrafficControl::OnReSend(TrafficControlConnectionId id, DataGramControlData& info, unsigned int resendDataSize)
{
    (void)info;
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
    cd->m_sdCurrentSecond.m_dataResend += resendDataSize;
    cd->m_sdEffectiveCurrentSecond.m_dataResend += resendDataSize;
}

//=========================================================================
// IsDisconnect
// [11/15/2010]
//=========================================================================
bool
DefaultTrafficControl::IsDisconnect(TrafficControlConnectionId id, float conditionThreshold)
{
    AZ_Assert(conditionThreshold >= 0.0f && conditionThreshold <= 1.0f, "Invalid condition threshold!");
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);

    if (cd->m_sdLifetime.m_connectionFactor >= conditionThreshold)
    {
#ifdef VERBOSE_DISCONNECT_DEBUGGING
        AZ_TracePrintf("GridMate", "Connection %p rtt %.2f ms (max. %.2f) and packetLoss %.2f (max. %.2f).\n", id, cd->m_sdLifetime.m_rtt, m_rttConnectionThreshold * conditionThreshold, cd->m_sdLifetime.m_packetLoss, m_packetLossThreshold * conditionThreshold);
#endif
        return true;
    }

    return false;
}

bool DefaultTrafficControl::IsCanReceiveData(TrafficControlConnectionId id) const
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
    return cd->m_canReceiveData;
}

//=========================================================================
// IsSendACKOnly
// [6/5/2012]
//=========================================================================
bool
DefaultTrafficControl::IsSendACKOnly(TrafficControlConnectionId id) const
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);

    // if we have received any data after the last send (which contains an ACK)
    // we need to send an ACK only packet to confirm receiving the data.
    if (cd->m_isReceivedDataAfterLastSend || m_currentTime - cd->m_lastAckSend > AZStd::chrono::milliseconds(m_lostPacketTimeoutMS / 10))
    {
        return true;
    }

    return false;
}


//=========================================================================
// Update
// [10/12/2010]
//=========================================================================
bool
DefaultTrafficControl::Update()
{
    TimeStamp now = AZStd::chrono::system_clock::now();
    //////////////////////////////////////////////////////////////////////////
    // Or we can deliver this from the engine
    float deltaTime = AZStd::chrono::duration<float>(now - m_currentTime).count();
    //////////////////////////////////////////////////////////////////////////
    m_currentTime = now;

    m_lastStatDataReset += deltaTime;

    if (m_lastStatDataReset >= 1.0f)
    {
        m_lastStatDataReset -= 1.0f;
    }
    else
    {
        return false; ///< We update our mode once per second.
    }

    for (ConnectionListType::iterator iConn = m_connections.begin(); iConn != m_connections.end(); ++iConn)
    {
        ConnectionData& cd = *iConn;

        cd.m_recvPacketAllowance = m_maxRecvPackets; // adding new allowance of recv'ed packets

        //////////////////////////////////////////////////////////////////////////
        // all data
        cd.m_sdLastSecond = cd.m_sdCurrentSecond;

        // update the lifetime stats
        cd.m_sdLifetime.m_dataSend += cd.m_sdLastSecond.m_dataSend;
        cd.m_sdLifetime.m_dataReceived += cd.m_sdLastSecond.m_dataReceived;

        cd.m_sdLifetime.m_dataAcked += cd.m_sdLastSecond.m_dataAcked;
        cd.m_sdLifetime.m_dataResend += cd.m_sdLastSecond.m_dataResend;

        cd.m_sdLifetime.m_packetSend += cd.m_sdLastSecond.m_packetSend;
        cd.m_sdLifetime.m_packetReceived += cd.m_sdLastSecond.m_packetReceived;

        cd.m_sdLifetime.m_packetAcked += cd.m_sdLastSecond.m_packetAcked;
        cd.m_sdLifetime.m_packetLost += cd.m_sdLastSecond.m_packetLost;

        // smooth the packet loss
        cd.m_sdLifetime.m_avgPacketSend += (static_cast<float>(cd.m_sdLastSecond.m_packetSend) - cd.m_sdLifetime.m_avgPacketSend) * 0.1f;
        cd.m_sdLifetime.m_avgPacketLost += (static_cast<float>(cd.m_sdLastSecond.m_packetLost) - cd.m_sdLifetime.m_avgPacketLost) * 0.1f;
        cd.m_sdLastSecond.m_packetLoss = cd.m_sdLifetime.m_avgPacketLost / (cd.m_sdLifetime.m_avgPacketSend + 0.00001f /*eps*/);
        cd.m_sdLastSecond.m_packetLoss = AZStd::min AZ_PREVENT_MACRO_SUBSTITUTION (cd.m_sdLastSecond.m_packetLoss, 1.0f);

        // RTT value will be incorrect (from the previous interval) if we did not received any acks.
        if (cd.m_sdLastSecond.m_packetAcked == 0)
        {
            if (cd.m_sdLastSecond.m_packetLost == 0)
            {
                cd.m_sdLastSecond.m_rtt = 0.0f;  // if we did not lost any packets assume we just did not send anything
            }
            else
            {
                // we don't really know what the RTT is, it's technically infinity. But that's why we have packetLoss too, so keep it the same
                cd.m_sdLastSecond.m_rtt = cd.m_sdLifetime.m_rtt /*blockedConnectionRTT*/;
                //AZ_TracePrintf("GridMate", "Traffic control: We did not received any packets for the last second %llu!\n\n", AZStd::GetTimeUTCMilliSecond());
            }
        }

        // smooth out the average rtt
        cd.m_sdLifetime.m_rtt += (cd.m_sdLastSecond.m_rtt - cd.m_sdLifetime.m_rtt) * 0.1f;
        // We already smooth the m_packetLost over the last 10 seconds cd.m_sdLifetime.m_packetLoss += (cd.m_sdLastSecond.m_packetLoss - cd.m_sdLifetime.m_packetLoss) * 0.1f;
        cd.m_sdLifetime.m_packetLoss = cd.m_sdLastSecond.m_packetLoss;
        //cd.m_sdLifetime.m_flow += (cd.m_sdLastSecond.m_flow - cd.m_sdLifetime.m_flow) * 0.1f;
        cd.m_sdLifetime.m_connectionFactor = AZStd::max AZ_PREVENT_MACRO_SUBSTITUTION (cd.m_sdLifetime.m_rtt / m_rttConnectionThreshold, cd.m_sdLifetime.m_packetLoss / m_packetLossThreshold);

        //////////////////////////////////////////////////////////////////////////
        // effective data
        cd.m_sdEffectiveLastSecond = cd.m_sdEffectiveCurrentSecond;

        // update the lifetime stats
        cd.m_sdEffectiveLifetime.m_dataSend += cd.m_sdEffectiveLastSecond.m_dataSend;
        cd.m_sdEffectiveLifetime.m_dataReceived += cd.m_sdEffectiveLastSecond.m_dataReceived;

        cd.m_sdEffectiveLifetime.m_dataAcked += cd.m_sdEffectiveLastSecond.m_dataAcked;
        cd.m_sdEffectiveLifetime.m_dataResend += cd.m_sdEffectiveLastSecond.m_dataResend;

        cd.m_sdEffectiveLifetime.m_packetSend += cd.m_sdEffectiveLastSecond.m_packetSend;
        cd.m_sdEffectiveLifetime.m_packetReceived += cd.m_sdEffectiveLastSecond.m_packetReceived;

        cd.m_sdEffectiveLifetime.m_packetAcked += cd.m_sdEffectiveLastSecond.m_packetAcked;
        cd.m_sdEffectiveLifetime.m_packetLost += cd.m_sdEffectiveLastSecond.m_packetLost;

        // smooth the packet loss
        cd.m_sdEffectiveLifetime.m_avgPacketSend += (static_cast<float>(cd.m_sdEffectiveLastSecond.m_packetSend) - cd.m_sdEffectiveLifetime.m_avgPacketSend) * 0.1f;
        cd.m_sdEffectiveLifetime.m_avgPacketLost += (static_cast<float>(cd.m_sdEffectiveLastSecond.m_packetLost) - cd.m_sdEffectiveLifetime.m_avgPacketLost) * 0.1f;
        cd.m_sdEffectiveLastSecond.m_packetLoss = cd.m_sdEffectiveLifetime.m_avgPacketLost / (cd.m_sdEffectiveLifetime.m_avgPacketSend + 0.00001f /*eps*/);
        cd.m_sdEffectiveLastSecond.m_packetLoss = AZStd::min AZ_PREVENT_MACRO_SUBSTITUTION (cd.m_sdEffectiveLastSecond.m_packetLoss, 1.0f);

        // RTT value will be incorrect (from the previous interval) if we did not received any acks.
        if (cd.m_sdEffectiveLastSecond.m_packetAcked == 0)
        {
            if (cd.m_sdEffectiveLastSecond.m_packetLost == 0)
            {
                cd.m_sdEffectiveLastSecond.m_rtt = 0.0f;  // if we did not lost any packets assume we just did not send anything
            }
            else
            {
                // we don't really know what the RTT is, it's technically infinity. But that's why we have packetLoss too, so keep it the same
                cd.m_sdEffectiveLastSecond.m_rtt = cd.m_sdEffectiveLifetime.m_rtt /*blockedConnectionRTT*/;
            }
        }

        // smooth out the average rtt
        cd.m_sdEffectiveLifetime.m_rtt += (cd.m_sdEffectiveLastSecond.m_rtt - cd.m_sdEffectiveLifetime.m_rtt) * 0.1f;
        // We already smooth the m_packetLost over the last 10 seconds cd.m_sdEffectiveLifetime.m_packetLoss += (cd.m_sdEffectiveLastSecond.m_packetLoss - cd.m_sdEffectiveLifetime.m_packetLoss) * 0.1f;
        cd.m_sdEffectiveLifetime.m_packetLoss = cd.m_sdEffectiveLastSecond.m_packetLoss;
        //////////////////////////////////////////////////////////////////////////

        //AZ_TracePrintf("GridMate","Traffic control: Connection %s LifeTime(rtt %.2f packetLoss %.2f) LastSecond(rtt %.2f packetLoss %.2f)\n",
        //  cd.m_address.c_str(),cd.m_sdLifetime.m_rtt,cd.m_sdLifetime.m_packetLoss,cd.m_sdLastSecond.m_rtt,cd.m_sdLastSecond.m_packetLoss);

        // send new statistics event
        EBUS_EVENT(Debug::CarrierDrillerBus, OnUpdateStatistics, cd.m_address, cd.m_sdLastSecond, cd.m_sdLifetime, cd.m_sdEffectiveLastSecond, cd.m_sdEffectiveLifetime);

        cd.m_sdCurrentSecond.Reset();
        cd.m_sdCurrentSecond.m_rtt = cd.m_sdLastSecond.m_rtt;
        //cd.sdCurrentSecond.flow = 1.0f; // Good

        cd.m_sdEffectiveCurrentSecond.Reset();
        cd.m_sdEffectiveCurrentSecond.m_rtt = cd.m_sdEffectiveLastSecond.m_rtt;
    }

    return true;
}

//=========================================================================
// QueryStatistics
// [11/11/2010]
//=========================================================================
void
DefaultTrafficControl::QueryStatistics(TrafficControlConnectionId id, Statistics* lastSecond, Statistics* lifetime, Statistics* effectiveLastSecond, Statistics* effectiveLifetime) const
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);

    if (lastSecond)
    {
        *lastSecond = cd->m_sdLastSecond;
    }
    if (lifetime)
    {
        *lifetime = cd->m_sdLifetime;
    }
    if (effectiveLastSecond)
    {
        *effectiveLastSecond = cd->m_sdEffectiveLastSecond;
    }
    if (effectiveLifetime)
    {
        *effectiveLifetime = cd->m_sdEffectiveLifetime;
    }
}

//=========================================================================
// QueryStatistics
//
//=========================================================================
void
DefaultTrafficControl::QueryCongestionState(TrafficControlConnectionId id, CongestionState* congestionState) const
{
    ConnectionData* cd = reinterpret_cast<ConnectionData*>(id->m_trafficData);
    AZ_Warning("GridMate", congestionState != nullptr, "Invalid congestion state, data will not be set!");
    if (congestionState)
    {
        congestionState->m_dataInTransfer = cd->m_inTransfer;
        congestionState->m_congestionWindow = cd->m_congestionWindow;
    }
}

unsigned int
DefaultTrafficControl::ConnectionData::TCPCubicWindow(TimeStamp& now, unsigned int packetSize) const
{
    // Ref: "CUBIC: a new TCP-friendly high-speed TCP variant" http://dl.acm.org/citation.cfm?id=1400105
    const double seconds = (now - m_lastWindowSizeDecrease).count() / 1000000.0;                    // Seconds since last backoff

    // For very short RTT's TCP-Reno is more aggressive, so we use TCP-Reno's window as a floor
    const unsigned int friendlyWindow = TCPRenoWindow(seconds);

    // Calculate window using TCP-Cubic formula
    // With a floor of MAX( TCP-Reno, our configured minimum )
    const unsigned int window = AZStd::GetMax(static_cast<unsigned int>(k_CubicScaleC*pow((seconds - m_CubicK), 3)*packetSize + m_preBackoffCongestionWindow),
    AZStd::GetMax(friendlyWindow, k_MinCongestionWindowPackets * packetSize));

    return window;
}
void
DefaultTrafficControl::ConnectionData::TCPCubicCalcK(unsigned int packetSize)
{
    m_CubicKcube = m_preBackoffCongestionWindow / ((k_CubicScaleC / k_CubicBeta)*packetSize);   // Use window size, scaling factor and backoff rate to calc m_CubicK^3
    m_CubicK = pow(m_CubicKcube, 1.0 / 3);                                                           // Cubic inflection point, m_CubicK, in seconds
}
void
DefaultTrafficControl::ConnectionData::TCPCubicPacketLost(TimeStamp& now, unsigned int packetSize)
{
    using AZStd::chrono::milliseconds;
    static const unsigned int backoffRate100 = static_cast<unsigned int>((1 - k_CubicBeta) * 100);     // Backoff Rate * 100 for integer calculations

    // Wait 1 RTT before allowing another backoff, plus 10ms buffer to cover jitter
    if (now > (m_lastWindowSizeDecrease + milliseconds(static_cast<int>(m_sdLifetime.m_rtt + 10))))
    {
        TCPCubicCalcK(packetSize);
        m_lastWindowSizeDecrease = now;
        m_lastWindowSizeIncrease = now;
        m_preBackoffCongestionWindow = m_congestionWindow;
        m_congestionWindow = AZStd::GetMax(backoffRate100 * m_congestionWindow / 100, k_MinCongestionWindowPackets * packetSize);   // Back off but not less than minimum
        m_slowStartThreshold = m_congestionWindow - packetSize;                                                                     // Store for idle recovery
        //AZ_TracePrintf("GridMate", "lost CWND old %u time %f w_max %u K %f avgPER %f\n",
        //    m_congestionWindow / packetSize, ((now - m_lastWindowSizeDecrease).count() / 1000000.0), m_preBackoffCongestionWindow / packetSize, m_CubicK, m_sdLifetime.m_avgPacketLost);
    }
}

void
DefaultTrafficControl::ConnectionData::TCPCubicExitSlowStart(TimeStamp& now, unsigned int packetSize)
{
    TCPCubicCalcK(packetSize);
    m_lastWindowSizeDecrease = now;
    m_lastWindowSizeIncrease = now;
    m_lastWindowSizeDecrease -= AZStd::chrono::milliseconds(static_cast<unsigned int>(m_CubicK * 1000));    // start at the inflection point
    m_preBackoffCongestionWindow = m_congestionWindow;
    m_slowStartThreshold = m_congestionWindow - packetSize;                                                 // Store for idle recovery
    //AZ_TracePrintf("GridMate", " exSS cwnd %u time %f w_max %u K %f avgPER %f\n",
    //    m_congestionWindow / packetSize, ((now - m_lastWindowSizeDecrease).count() / 1000000.0), m_preBackoffCongestionWindow / packetSize, m_CubicK, m_sdLifetime.m_avgPacketLost);
}

unsigned int
DefaultTrafficControl::ConnectionData::TCPRenoWindow(double seconds) const
{
    const double rtt = m_sdLifetime.m_rtt;
    const double backoffWindow = (1 - k_CubicBeta)*m_preBackoffCongestionWindow;
    if ( ! AZ::IsNormalDouble(rtt) )
    {
        return static_cast<unsigned int>(backoffWindow); // Unable to predict with non-normal RTT; shouldn't happen
    }
    const double cwnd = (backoffWindow + k_CubicAlpha * seconds / rtt);
    return static_cast<unsigned int>(cwnd);
}
