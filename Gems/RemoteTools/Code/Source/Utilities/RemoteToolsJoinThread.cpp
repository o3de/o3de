/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RemoteToolsJoinThread.h"

#include <AzNetworking/Framework/INetworking.h>

namespace RemoteTools
{
    RemoteToolsJoinThread::RemoteToolsJoinThread(int updateRate)
        : TimedThread("RemoteTools::RemoteToolsJoinThread", AZ::TimeMs(updateRate))
    {
        ;
    }

    RemoteToolsJoinThread::~RemoteToolsJoinThread()
    {
        Stop();
        Join();
    }

    void RemoteToolsJoinThread::OnUpdate([[maybe_unused]] AZ::TimeMs updateRateMs)
    {
        //connect_target(AZ::ConsoleCommandContainer());
        for (auto& networkInterface : AZ::Interface<AzNetworking::INetworking>::Get()->GetNetworkInterfaces())
        {
            if (networkInterface.first == AZ::Name("RemoteTools") &&
                networkInterface.second->GetConnectionSet().GetActiveConnectionCount() > 0)
            {
                Stop();
            }
        }
    }
} // namespace RemoteTools
