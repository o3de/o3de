/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/UdpTransport/UdpSocket.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Utilities/Endian.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/EBus/IEventScheduler.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Interface/Interface.h>

namespace AzNetworking
{
    AZ_CVAR(int32_t, net_UdpSendBufferSize, 1 * 1024 * 1024, nullptr, AZ::ConsoleFunctorFlags::Null, "Default UDP socket send buffer size");
    AZ_CVAR(int32_t, net_UdpRecvBufferSize, 1 * 1024 * 1024, nullptr, AZ::ConsoleFunctorFlags::Null, "Default UDP socket receive buffer size");
    AZ_CVAR(bool, net_UdpIgnoreWin10054, true, nullptr, AZ::ConsoleFunctorFlags::Null, "If true, will ignore 10054 socket errors on windows");

    UdpSocket::~UdpSocket()
    {
        Close();
    }

    bool UdpSocket::IsEncrypted() const
    {
        return false;
    }

    DtlsEndpoint::ConnectResult UdpSocket::ConnectDtlsEndpoint(DtlsEndpoint&, const IpAddress&, UdpPacketEncodingBuffer&) const
    {
        // No-op, no encryption wrapper required
        return DtlsEndpoint::ConnectResult::Complete;
    }

    DtlsEndpoint::ConnectResult UdpSocket::AcceptDtlsEndpoint(DtlsEndpoint&, const IpAddress&) const
    {
        // No-op, no encryption wrapper required
        return DtlsEndpoint::ConnectResult::Complete;
    }

    bool UdpSocket::Open(uint16_t port, CanAcceptConnections, TrustZone)
    {
        AZ_Assert(!IsOpen(), "Open called on an active socket");

        if (IsOpen())
        {
            return false;
        }

        // Open the socket
        {
            m_socketFd = static_cast<SocketFd>(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));

            if (!IsOpen())
            {
                const int32_t error = GetLastNetworkError();
                AZLOG_ERROR("Failed to create socket (%d:%s)", error, GetNetworkErrorDesc(error));
                m_socketFd = InvalidSocketFd;
                return false;
            }
        }

        // Handle binding
        {
            sockaddr_in hints;
            hints.sin_family = AF_INET;
            hints.sin_addr.s_addr = INADDR_ANY;
            hints.sin_port = htons(port);

            if (::bind(static_cast<int32_t>(m_socketFd), (const sockaddr *)&hints, sizeof(hints)) != 0)
            {
                const int32_t error = GetLastNetworkError();
                AZLOG_ERROR("Failed to bind UDP socket to port %u (%d:%s)", uint32_t(port), error, GetNetworkErrorDesc(error));
                Close();
                return false;
            }
        }

        if (!SetSocketBufferSizes(m_socketFd, net_UdpSendBufferSize, net_UdpRecvBufferSize)
         || !SetSocketNonBlocking(m_socketFd))
        {
            Close();
            return false;
        }

        return true;
    }

    void UdpSocket::Close()
    {
        CloseSocket(m_socketFd);
        m_socketFd = InvalidSocketFd;
    }

    int32_t UdpSocket::Send
    (
        const IpAddress& address,
        const uint8_t* data,
        uint32_t size,
        bool encrypt,
        DtlsEndpoint& dtlsEndpoint,
        [[maybe_unused]] const ConnectionQuality& connectionQuality
    ) const
    {
        AZ_Assert(size > 0, "Invalid data size for send");
        AZ_Assert(data != nullptr, "NULL data pointer passed to send");

        AZ_Assert(address.GetAddress(ByteOrder::Host) != 0, "Invalid address");
        AZ_Assert(address.GetPort(ByteOrder::Host) != 0, "Invalid address");

        if (!IsOpen())
        {
            return 0;
        }

#ifdef ENABLE_LATENCY_DEBUG
        if (connectionQuality.m_lossPercentage > 0)
        {
            if (int32_t(m_random.GetRandom() % 100) < (connectionQuality.m_lossPercentage))
            {
                // Pretend we sent, but don't actually send
                return true;
            }
        }
#endif

        int32_t sentBytes = size;

#ifdef ENABLE_LATENCY_DEBUG
        if (connectionQuality.m_latencyMs <= AZ::TimeMs{ 0 })
#endif
        {
            sentBytes = SendInternal(address, data, size, encrypt, dtlsEndpoint);

            if (sentBytes < 0)
            {
                const int32_t error = GetLastNetworkError();

                if (ErrorIsWouldBlock(error)) // Filter would block messages
                {
                    return SocketOpResultSuccess;
                }

                AZLOG_ERROR("Failed to write to socket (%d:%s)", error, GetNetworkErrorDesc(error));
            }
        }
#ifdef ENABLE_LATENCY_DEBUG
        else if ((connectionQuality.m_latencyMs > AZ::TimeMs{ 0 }) || (connectionQuality.m_varianceMs > AZ::TimeMs{ 0 }))
        {
            const AZ::TimeMs jitterMs = aznumeric_cast<AZ::TimeMs>(m_random.GetRandom()) % (connectionQuality.m_varianceMs > AZ::TimeMs{ 0 }
                                      ? connectionQuality.m_varianceMs
                                      : AZ::TimeMs{ 1 });
            const AZ::TimeMs deferTimeMs = (connectionQuality.m_latencyMs) + jitterMs;

            DeferredData deferred = DeferredData(address, data, size, encrypt, dtlsEndpoint);
            AZ::Interface<AZ::IEventScheduler>::Get()->AddCallback([&, deferredData = deferred]
                    { SendInternalDeferred(deferredData); }, AZ::Name("Deferred packet"), deferTimeMs);
        }
#endif

        m_sentPackets++;
        m_sentBytes += sentBytes;

        return sentBytes;
    }

    int32_t UdpSocket::Receive(IpAddress& outAddress, uint8_t* outData, uint32_t size) const
    {
        AZ_Assert(size > 0, "Invalid data size for send");
        AZ_Assert(outData != nullptr, "NULL data pointer passed to send");

        if (!IsOpen())
        {
            return 0;
        }

        sockaddr_in from;
        socklen_t   fromLen = sizeof(from);

        const int32_t receivedBytes = recvfrom(static_cast<int32_t>(m_socketFd), reinterpret_cast<char*>(outData), static_cast<int32_t>(size), 0, (sockaddr*)&from, &fromLen);

        outAddress = IpAddress(ByteOrder::Network, from.sin_addr.s_addr, from.sin_port);

        if (receivedBytes < 0)
        {
            const int32_t error = GetLastNetworkError();

            if (ErrorIsWouldBlock(error)) // Filter would block messages
            {
                return 0;
            }

            bool ignoreForciblyClosedError = false;
            if (ErrorIsForciblyClosed(error, ignoreForciblyClosedError))
            {
                if (ignoreForciblyClosedError)
                {
                    return 0;
                }
                else
                {
                    return SocketOpResultError;
                }
            }

            AZLOG_ERROR("Failed to read from socket (%d:%s)", error, GetNetworkErrorDesc(error));
        }

        if (receivedBytes <= 0)
        {
            return 0;
        }

        m_recvPackets++;
        m_recvBytes += receivedBytes;
        return receivedBytes;
    }

    int32_t UdpSocket::SendInternal(const IpAddress& address, const uint8_t* data, uint32_t size,
        [[maybe_unused]] bool encrypt, [[maybe_unused]] DtlsEndpoint& dtlsEndpoint) const
    {
        sockaddr_in destAddr;
        memset(&destAddr, 0, sizeof(destAddr));
        destAddr.sin_family = AF_INET;
        destAddr.sin_addr.s_addr = address.GetAddress(ByteOrder::Network);
        destAddr.sin_port = address.GetPort(ByteOrder::Network);
        return sendto(static_cast<int32_t>(m_socketFd), reinterpret_cast<const char*>(data), size, 0, (sockaddr*)&destAddr, sizeof(destAddr));
    }

#ifdef ENABLE_LATENCY_DEBUG
    int32_t UdpSocket::SendInternalDeferred(const DeferredData& data) const
    {
        return SendInternal(data.m_address, data.m_dataBuffer.GetBuffer(), static_cast<uint32_t>(data.m_dataBuffer.GetSize()), data.m_encrypt, *data.m_dtlsEndpoint);
    }
#endif
}
