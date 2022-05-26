/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ToolingJoinThread.h"

#include <AzNetworking/Framework/INetworking.h>

namespace ToolingConnection
{
    ToolingJoinThread::ToolingJoinThread(int updateRate)
        : TimedThread("ToolingConnection::ToolingJoinThread", AZ::TimeMs(updateRate))
    {
        ;
    }

    ToolingJoinThread::~ToolingJoinThread()
    {
        Stop();
        Join();
    }

    void ToolingJoinThread::OnUpdate([[maybe_unused]] AZ::TimeMs updateRateMs)
    {
        //connect_target(AZ::ConsoleCommandContainer());
        for (auto& networkInterface : AZ::Interface<AzNetworking::INetworking>::Get()->GetNetworkInterfaces())
        {
            if (networkInterface.first == AZ::Name("ToolingConnection") &&
                networkInterface.second->GetConnectionSet().GetActiveConnectionCount() > 0)
            {
                Stop();
            }
        }
    }
} // namespace ToolingConnection
