/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RemoteToolsJoinThread.h"

#include <AzCore/IO/SystemFile.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/ranges/elements_view.h>
#include <AzCore/std/string/conversions.h>
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

    AZStd::string GetPersistentName()
    {
        AZStd::string persistentName = "O3DE";

        char procPath[AZ::IO::MaxPathLength];
        AZ::Utils::GetExecutablePathReturnType ret = AZ::Utils::GetExecutablePath(AZStd::data(procPath), AZStd::size(procPath));
        if (ret.m_pathStored == AZ::Utils::ExecutablePathResult::Success && ret.m_pathIncludesFilename)
        {
            persistentName = AZ::IO::PathView(procPath).Filename().Native();
        }

        return persistentName;
    }

    void RemoteToolsJoinThread::OnUpdate([[maybe_unused]] AZ::TimeMs updateRateMs)
    {
        bool isRequestingConnection = false;
        auto& networkInterfaces = AZ::Interface<AzNetworking::INetworking>::Get()->GetNetworkInterfaces();
        for (const auto& [persistentId, toolsRegistryEntry] : m_remoteToolsComponent->m_entryRegistry)
        {
            AZ::Name serviceName = toolsRegistryEntry.m_name;
            if (!toolsRegistryEntry.m_isHost && networkInterfaces.contains(serviceName))
            {
                auto networkInterface = networkInterfaces.find(serviceName);
                if (networkInterface->second->GetConnectionSet().GetActiveConnectionCount() == 0)
                {
                    AzNetworking::ConnectionId connId = networkInterface->second->Connect(toolsRegistryEntry.m_ip);
                    if (connId != AzNetworking::InvalidConnectionId)
                    {
                        RemoteToolsPackets::RemoteToolsConnect initPacket;
                        initPacket.SetPersistentId(persistentId);
                        initPacket.SetDisplayName(GetPersistentName());
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
        }
    }
} // namespace RemoteTools
