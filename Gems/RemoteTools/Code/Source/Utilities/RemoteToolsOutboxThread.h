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

namespace RemoteTools
{
    typedef AZStd::pair<uint32_t, AZStd::vector<char, AZ::OSStdAllocator>> OutboundToolingDatum;
    struct OutboundRemoteToolsMessage
    {
        AzNetworking::ConnectionId connectionId;
        AzNetworking::INetworkInterface* netInterface;
        OutboundToolingDatum datum;
    };
    typedef AZStd::deque<OutboundRemoteToolsMessage, AZ::OSStdAllocator> ToolingOutbox;

    //! @class ToolingJoinThread
    //! @brief A class for polling a connection to the host target
    class RemoteToolsOutboxThread final : public AzNetworking::TimedThread
    {
    public:
        RemoteToolsOutboxThread(int updateRate);
        ~RemoteToolsOutboxThread() override;

        //! Pushes a Remote Tools message onto the outbox thread for send
        //! @param netInterface The network interface the message will send on
        //! @param connectionId The Id of the connection to send on
        //! @param datum The datum to send
        void PushOutboxMessage(AzNetworking::INetworkInterface* netInterface,
            AzNetworking::ConnectionId connectionId, OutboundToolingDatum&& datum);

        //! Reads the number of pending messages on the outbox thread
        //! @return The number of pending messages
        uint32_t GetPendingMessageCount();

    private:
        AZ_DISABLE_COPY_MOVE(RemoteToolsOutboxThread);

        //! Invoked on thread start
        void OnStart() override{};

        //! Invoked on thread stop
        void OnStop() override;

        //! Invoked on thread update to poll for a target host to join
        //! @param updateRateMs The amount of time the thread can spend in OnUpdate in ms
        void OnUpdate(AZ::TimeMs updateRateMs) override;

        ToolingOutbox m_outbox;
        AZStd::mutex m_outboxMutex;
    };
} // namespace RemoteTools
