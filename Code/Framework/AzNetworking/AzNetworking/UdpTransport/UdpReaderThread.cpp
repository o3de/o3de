/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/UdpReaderThread.h>
#include <AzNetworking/UdpTransport/UdpSocket.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    static constexpr AZ::TimeMs ReaderThreadUpdateRateMs{ 10 };

    AZ_CVAR(AZ::TimeMs, net_UdpMaxReadTimeMs, ReaderThreadUpdateRateMs, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The amount of time to allow the reader thread to read data off registered sockets");

    UdpReaderThread::UdpReaderThread()
        : TimedThread("UdpReaderThread", ReaderThreadUpdateRateMs)
    {
        ;
    }

    UdpReaderThread::~UdpReaderThread()
    {
        Stop();
        Join();
    }

    bool UdpReaderThread::RegisterSocket(UdpSocket* socket)
    {
        if (SocketExists(socket))
        {
            AZLOG_ERROR("Attempting to add a duplicate socket to the UdpReaderThread");
            return false;
        }
        m_pendingAdds.push_back(socket);
        if (!IsRunning())
        {
            Start();
        }
        return true;
    }

    void UdpReaderThread::UnregisterSocket(UdpSocket* socket)
    {
        // We need to null out the socket immediately in both the front and back
        // buffers so that the reader thread doesn't try and use a deleted socket
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        {
            const int32_t frontIndex = 1 - m_backIndex;
            ReaderBuffer& front = m_readerBuffers[frontIndex];
            for (auto& socketEntry : front.m_entries)
            {
                if (socketEntry.m_socket == socket)
                {
                    socketEntry.m_socket = nullptr;
                }
            }
        }
        {
            ReaderBuffer& back = m_readerBuffers[m_backIndex];
            for (auto& socketEntry : back.m_entries)
            {
                if (socketEntry.m_socket == socket)
                {
                    socketEntry.m_socket = nullptr;
                }
            }
        }
    }

    const UdpReaderThread::ReceivedPackets* UdpReaderThread::GetReceivedPackets(UdpSocket* socket) const
    {
        const int32_t frontIndex = 1 - m_backIndex;
        const ReaderBuffer& front = m_readerBuffers[frontIndex];
        for (const auto& socketEntry : front.m_entries)
        {
            if (socketEntry.m_socket == socket)
            {
                return &(socketEntry.m_receivedPackets);
            }
        }
        return nullptr;
    }

    void UdpReaderThread::SwapBuffers()
    {
        const int32_t frontIndex = 1 - m_backIndex;
        ReaderBuffer& front = m_readerBuffers[frontIndex];

        // Clear all the packets we've already processed on our main thread
        for (auto& socketEntry : front.m_entries)
        {
            socketEntry.m_receivedPackets.clear();
        }

        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        {
            // This scope is sync-safe between the main and reader threads
            ReaderBuffer& back = m_readerBuffers[m_backIndex];
            for (UdpSocket* socket : m_pendingAdds)
            {
                front.m_entries.emplace_back(SocketEntry{ socket, ReceivedPackets() });
                back.m_entries.emplace_back(SocketEntry{ socket, ReceivedPackets() });
            }
            m_pendingAdds.clear();
            AZStd::remove_if(front.m_entries.begin(), front.m_entries.end(), [](auto& socketEntry) { return socketEntry.m_socket == nullptr; });
            AZStd::remove_if(back.m_entries.begin(), back.m_entries.end(), [](auto& socketEntry) { return socketEntry.m_socket == nullptr; });
            m_backIndex = 1 - m_backIndex;
            m_readerBuffers[m_backIndex].m_receiveBuffer.Resize(0);
        }
    }

    uint32_t UdpReaderThread::GetSocketCount() const
    {
        const int32_t frontIndex = 1 - m_backIndex;
        const ReaderBuffer& front = m_readerBuffers[frontIndex];
        return aznumeric_cast<uint32_t>(front.m_entries.size());
    }

    AZ::TimeMs UdpReaderThread::GetUpdateTimeMs() const
    {
        return m_updateTimeMs;
    }

    bool UdpReaderThread::SocketExists(UdpSocket* socket) const
    {
        const int32_t frontIndex = 1 - m_backIndex;
        const ReaderBuffer& front = m_readerBuffers[frontIndex];
        for (auto& socketEntry : front.m_entries)
        {
            if (socketEntry.m_socket == socket)
            {
                return true;
            }
        }
        return false;
    }

    void UdpReaderThread::OnStart()
    {
        ;
    }

    void UdpReaderThread::OnStop()
    {
        ;
    }

    void UdpReaderThread::OnUpdate(AZ::TimeMs updateRateMs)
    {
        AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();

        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        ReaderBuffer& back = m_readerBuffers[m_backIndex];
        ByteBuffer<MaxUdpReceiveBufferSize>& receiveBuffer = back.m_receiveBuffer;
        for (auto& socketEntry : back.m_entries)
        {
            UdpSocket* socket = socketEntry.m_socket;
            if (socket == nullptr)
            {
                continue;
            }

            ReceivedPackets& receivedPackets = socketEntry.m_receivedPackets;
            for (;;)
            {
                AZ::TimeMs elapsedTimeMs = AZ::GetElapsedTimeMs() - startTimeMs;
                if (elapsedTimeMs > updateRateMs)
                {
                    AZLOG_INFO("ReceivePackets bled %d ms", aznumeric_cast<int32_t>(elapsedTimeMs - updateRateMs));
                    break;
                }

                IpAddress address;
                const uint32_t bufferHead = static_cast<uint32_t>(receiveBuffer.GetSize());
                if (bufferHead + MaxUdpTransmissionUnit >= receiveBuffer.GetCapacity())
                {
                    AZLOG_INFO("Receive buffer full, leaving data on the socket. Size exceeded by %d",
                        aznumeric_cast<int32_t>(bufferHead + MaxUdpTransmissionUnit - receiveBuffer.GetCapacity()));
                    break;
                }

                uint8_t* dstData = receiveBuffer.GetBufferEnd();
                receiveBuffer.Resize(bufferHead + MaxUdpTransmissionUnit);

                const int32_t receivedBytes = socket->Receive(address, dstData, MaxUdpTransmissionUnit);
                if (receivedBytes > 0 && !receivedPackets.full())
                {
                    receivedPackets.push_back(ReceivedPacket(address, dstData, receivedBytes));
                    receiveBuffer.Resize(bufferHead + receivedBytes);
                }
                else
                {
                    receiveBuffer.Resize(bufferHead);
                    break;
                }
            }
        }
        m_updateTimeMs += AZ::GetElapsedTimeMs() - startTimeMs;
    }

    UdpReaderThread::ReceivedPacket::ReceivedPacket(const IpAddress& address, const uint8_t* buffer, int32_t receivedBytes)
        : m_address(address)
        , m_buffer(buffer)
        , m_receivedBytes(receivedBytes)
    {
        ;
    }
}
