/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ToolingOutboxThread.h"

#include <Source/AutoGen/ToolingConnection.AutoPackets.h>
#include <AzFramework/Network/IToolingConnection.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzNetworking/Framework/INetworkInterface.h>

namespace ToolingConnection
{
    ToolingOutboxThread::ToolingOutboxThread(int updateRate)
        : TimedThread("ToolingConnection::ToolingOutboxThread", AZ::TimeMs(updateRate))
    {
        ;
    }

    ToolingOutboxThread::~ToolingOutboxThread()
    {
        Stop();
        Join();
    }

    void ToolingOutboxThread::PushOutboxMessage(AzNetworking::INetworkInterface* netInterface,
        AzNetworking::ConnectionId connectionId, OutboundToolingDatum&& datum)
    {
        OutboundToolingMessage message;
        message.netInterface = netInterface;
        message.connectionId = connectionId;
        message.datum = datum;
        m_outbox.push_back(message);
    }

    void ToolingOutboxThread::OnUpdate([[maybe_unused]] AZ::TimeMs updateRateMs)
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

                const AzFramework::ToolingEndpointInfo ti = AZ::Interface<AzFramework::IToolingConnection>::Get()->GetEndpointInfo(static_cast<uint32_t>(outBoxElem.connectionId));

                while (outSize > 0)
                {
                    // Fragment the message into NeighborMessageBuffer packet sized chunks and send
                    size_t bufferSize = AZStd::min(outSize, aznumeric_cast<size_t>(Neighborhood::NeighborBufferSize));
                    ToolingConnectionPackets::ToolingPacket tmPacket;
                    tmPacket.SetPersistentId(ti.GetPersistentId());
                    tmPacket.SetSize(aznumeric_cast<uint32_t>(totalSize));
                    Neighborhood::NeighborMessageBuffer encodingBuffer;
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
} // namespace ToolingConnection
