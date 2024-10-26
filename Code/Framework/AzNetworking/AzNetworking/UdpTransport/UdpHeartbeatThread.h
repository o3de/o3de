/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Utilities/TimedThread.h>

namespace AzNetworking
{
    class UdpNetworkInterface;

    //! @class UdpHeartbeatThread
    //! @brief Sends a heartbeat packet if the main-thread system tick has been blocked and unable to process packets in a while.
    class UdpHeartbeatThread
        : public TimedThread
    {
    public:

        UdpHeartbeatThread();
        ~UdpHeartbeatThread() override;
        void RegisterNetworkInterface(UdpNetworkInterface* udpNetworkInterface);
        void UnregisterNetworkInterface(UdpNetworkInterface* udpNetworkInterface);

    private:

        void OnStart() override;
        void OnStop() override;
        void OnUpdate(AZ::TimeMs updateRateMs) override;

        AZ_DISABLE_COPY_MOVE(UdpHeartbeatThread);

        AZStd::vector<UdpNetworkInterface*> m_udpNetworkInterfaces;
        AZStd::mutex m_mutex;
    };
}
