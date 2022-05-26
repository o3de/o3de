/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Utilities/TimedThread.h>

namespace AzNetworking
{
    class INetworkInterface;
}

namespace ToolingConnection
{
    typedef AZStd::pair<uint32_t, AZStd::vector<char, AZ::OSStdAllocator>> OutboundToolingDatum;
    struct OutboundToolingMessage
    {
        AzNetworking::ConnectionId connectionId;
        AzNetworking::INetworkInterface* netInterface;
        OutboundToolingDatum datum;
    };
    typedef AZStd::deque<OutboundToolingMessage, AZ::OSStdAllocator> ToolingOutbox;


    //! @class ToolingJoinThread
    //! @brief A class for polling a connection to the host target
    class ToolingOutboxThread final : public AzNetworking::TimedThread
    {
    public:
        ToolingOutboxThread(int updateRate);
        ~ToolingOutboxThread() override;

        

        void PushOutboxMessage(AzNetworking::INetworkInterface* netInterface,
            AzNetworking::ConnectionId connectionId, OutboundToolingDatum&& datum);
    private:
        AZ_DISABLE_COPY_MOVE(ToolingOutboxThread);

        //! Invoked on thread start
        void OnStart() override{};

        //! Invoked on thread stop
        void OnStop() override{};

        //! Invoked on thread update to poll for a Target host to join
        //! @param updateRateMs The amount of time the thread can spend in OnUpdate in ms
        void OnUpdate(AZ::TimeMs updateRateMs) override;

        ToolingOutbox m_outbox;
        AZStd::mutex m_outboxMutex;
    };
} // namespace ToolingConnection
