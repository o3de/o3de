/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/UdpHeartbeatThread.h>
#include <AzNetworking/UdpTransport/UdpNetworkInterface.h>
#include <AzNetworking/AutoGen/CorePackets.AutoPackets.h>


namespace AzNetworking
{
    // HeartbeatThreadUpdateRateMs is how often the timed thread's OnUpdate call will be executed.
    // This controls how often heartbeats are sent on every active UDP connection.
    // The heartbeat sends are offloaded to this heartbeat thread to ensure that they keep getting sent even
    // while the main thread is blocked on time-consuming tasks like loading a level.
    static constexpr AZ::TimeMs HeartbeatThreadUpdateRateMs{ 500 };

    UdpHeartbeatThread::UdpHeartbeatThread()
        : TimedThread("UdpHeartbeatThread", HeartbeatThreadUpdateRateMs)
    {
    }

    UdpHeartbeatThread::~UdpHeartbeatThread()
    {
        Stop();
        Join();
    }

    void UdpHeartbeatThread::OnStart()
    {
    }

    void UdpHeartbeatThread::OnStop()
    {
    }

    void UdpHeartbeatThread::OnUpdate([[maybe_unused]]AZ::TimeMs updateRateMs)
    {
        AZ::TimeMs currentTime = AZ::GetElapsedTimeMs();

        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        for (auto udpInterface : m_udpNetworkInterfaces)
        {
            AZ::TimeMs timeSinceSystemTick = currentTime - udpInterface->GetLastSystemTickUpdate();
            if (timeSinceSystemTick > HeartbeatThreadUpdateRateMs)
            {
                // Main-thread is blocked; send a heartbeat from this thread.
                auto sendNetworkUpdates = [](AzNetworking::IConnection& connection)
                {
                    if (connection.GetConnectionRole() == ConnectionRole::Connector)
                    {
                        connection.SendUnreliablePacket(CorePackets::HeartbeatPacket(false));
                    }
                };

                udpInterface->GetConnectionSet().VisitConnections(sendNetworkUpdates);
            }
        }
    }

    void UdpHeartbeatThread::RegisterNetworkInterface(UdpNetworkInterface* udpNetworkInterface)
    {
        {
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            m_udpNetworkInterfaces.push_back(udpNetworkInterface);
        }

        if (!IsRunning())
        {
            Start();
        }
    }

    void UdpHeartbeatThread::UnregisterNetworkInterface(UdpNetworkInterface* udpNetworkInterface)
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);

        auto it = AZStd::find(m_udpNetworkInterfaces.begin(), m_udpNetworkInterfaces.end(), udpNetworkInterface);

        if (it != m_udpNetworkInterfaces.end())
        {
            m_udpNetworkInterfaces.erase(it);
        }

        if (IsRunning() && m_udpNetworkInterfaces.empty())
        {
            Stop();
        }
    }
} // namespace AzNetworking
