/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RemoteToolsJoinThread.h"

#include <AzCore/Name/Name.h>
#include <AzNetworking/Framework/INetworking.h>

#include <RemoteToolsSystemComponent.h>
#include <Source/AutoGen/RemoteTools.AutoPackets.h>

namespace RemoteTools
{
    RemoteToolsJoinThread::RemoteToolsJoinThread(int updateRate, RemoteToolsSystemComponent* component)
        : TimedThread("RemoteTools::RemoteToolsJoinThread", AZ::TimeMs(updateRate))
    {
        m_remoteToolsComponent = component;
    }

    RemoteToolsJoinThread::~RemoteToolsJoinThread()
    {
        Stop();
        Join();
    }

    void RemoteToolsJoinThread::OnUpdate([[maybe_unused]] AZ::TimeMs updateRateMs)
    {
        bool isRequestingConnection = false;
        auto& networkInterfaces = AZ::Interface<AzNetworking::INetworking>::Get()->GetNetworkInterfaces();
        for (auto registryIt = m_remoteToolsComponent->m_entryRegistry.begin();
            registryIt != m_remoteToolsComponent->m_entryRegistry.end();
             ++registryIt)
        {
            AZ::Name serviceName = registryIt->second.m_name;
            if (!registryIt->second.m_isHost && networkInterfaces.contains(serviceName))
            {
                auto networkInterface = networkInterfaces.find(serviceName);
                if (networkInterface->second->GetConnectionSet().GetActiveConnectionCount() == 0)
                {
                    AzNetworking::ConnectionId connId = networkInterface->second->Connect(registryIt->second.m_ip);
                    if (connId != AzNetworking::InvalidConnectionId)
                    {
                        RemoteToolsPackets::RemoteToolsConnect initPacket;
                        initPacket.SetPersistentId(registryIt->first);
                        initPacket.SetDisplayName(serviceName.GetCStr());
                        networkInterface->second->SendReliablePacket(connId, initPacket);
                    }
                    else
                    {
                        isRequestingConnection = true;
                    }
                }
            }
        }

        if (!isRequestingConnection)
        {
            Stop();
            Join();
        }
    }

    void RemoteToolsJoinThread::UpdateStatus()
    {
        if (!IsRunning())
        {
            bool isRequestingConnection = false;
            auto& networkInterfaces = AZ::Interface<AzNetworking::INetworking>::Get()->GetNetworkInterfaces();
            for (auto registryIt = m_remoteToolsComponent->m_entryRegistry.begin();
                 registryIt != m_remoteToolsComponent->m_entryRegistry.end();
                 ++registryIt)
            {
                AZ::Name serviceName = registryIt->second.m_name;
                if (networkInterfaces.contains(serviceName))
                {
                    auto networkInterface = networkInterfaces.find(serviceName);
                    if (networkInterface->second->GetConnectionSet().GetActiveConnectionCount() == 0)
                    {
                        isRequestingConnection = true;
                        break;
                    }
                }
            }

            if (isRequestingConnection)
            {
                Start();
            }
        }
    }

} // namespace RemoteTools
