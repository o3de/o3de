/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RemoteToolsOutboxThread.h"

#include <Source/AutoGen/RemoteTools.AutoPackets.h>
#include <RemoteToolsSystemComponent.h>
#include <AzFramework/Network/IRemoteTools.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzNetworking/Framework/INetworkInterface.h>

namespace RemoteTools
{
    RemoteToolsOutboxThread::RemoteToolsOutboxThread(int updateRate)
        : TimedThread("RemoteTools::RemoteToolsOutboxThread", AZ::TimeMs(updateRate))
    {
        ;
    }

    RemoteToolsOutboxThread::~RemoteToolsOutboxThread()
    {
        Stop();
        Join();
    }

    void RemoteToolsOutboxThread::PushOutboxMessage(
        AzNetworking::INetworkInterface* netInterface,
        AzNetworking::ConnectionId connectionId, OutboundToolingDatum&& datum)
    { 
        m_outboxMutex.lock();
        auto& message = m_outbox.emplace_back();
        message.netInterface = netInterface;
        message.connectionId = connectionId;
        message.datum = datum;
        m_outboxMutex.unlock();
    }

    uint32_t RemoteToolsOutboxThread::GetPendingMessageCount()
    {
        return aznumeric_cast<uint32_t>(m_outbox.size());
    }

    void RemoteToolsOutboxThread::OnStop()
    {
        m_outboxMutex.lock();
        m_outbox.clear();
        m_outboxMutex.unlock();
    }

    void RemoteToolsOutboxThread::OnUpdate([[maybe_unused]] AZ::TimeMs updateRateMs)
    {
        // Send outbound data
        size_t maxMsgsToSend = m_outbox.size();
        if (maxMsgsToSend > 0)
        {
            for (size_t iSend = 0; iSend < maxMsgsToSend; ++iSend)
            {
                // Lock outbox to prevent a read/write race
                m_outboxMutex.lock();

                auto& outBoxElem = m_outbox.front();
                auto& outBoxDatum = outBoxElem.datum.second;
                uint8_t* outBuffer = reinterpret_cast<uint8_t*>(outBoxDatum.data());
                const size_t totalSize = outBoxDatum.size();
                size_t outSize = outBoxDatum.size();

                while (outSize > 0 && outBoxElem.netInterface != nullptr)
                {
                    // Fragment the message into RemoteToolsMessageBuffer packet sized chunks and send
                    size_t bufferSize = AZStd::min(outSize, aznumeric_cast<size_t>(RemoteToolsBufferSize));
                    RemoteToolsPackets::RemoteToolsMessage tmPacket;
                    tmPacket.SetPersistentId(outBoxElem.datum.first);
                    tmPacket.SetSize(aznumeric_cast<uint32_t>(totalSize));
                    RemoteToolsMessageBuffer encodingBuffer;
                    encodingBuffer.CopyValues(outBuffer + (totalSize - outSize), bufferSize);
                    tmPacket.SetMessageBuffer(encodingBuffer);
                    outSize -= bufferSize;

                    outBoxElem.netInterface->SendReliablePacket(outBoxElem.connectionId, tmPacket);
                }

                m_outbox.pop_front();
                m_outboxMutex.unlock();
            }
        }
    }
} // namespace RemoteTools
