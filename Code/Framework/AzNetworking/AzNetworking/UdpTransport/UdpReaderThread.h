/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <AzNetworking/Utilities/TimedThread.h>
#include <AzNetworking/UdpTransport/DtlsEndpoint.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzNetworking
{
    // Forwards
    class UdpSocket;

    //! @class UdpSocketReader
    //! @brief reads lots of data off a UDP socket for deferred processing.
    class UdpReaderThread
        : public TimedThread
    {
    public:

        static constexpr uint32_t MaxUdpReceivePacketCount = 1024;
        static constexpr uint32_t MaxUdpReceiveBufferSize = MaxUdpReceivePacketCount * MaxUdpTransmissionUnit;

        struct ReceivedPacket
        {
            ReceivedPacket() = default;
            ReceivedPacket(const IpAddress& address, const uint8_t* buffer, int32_t receivedBytes);
            IpAddress      m_address;
            const uint8_t* m_buffer = nullptr;
            int32_t        m_receivedBytes = 0;
        };

        using ReceivedPackets = AZStd::fixed_vector<ReceivedPacket, MaxUdpReceivePacketCount>;

        UdpReaderThread();
        ~UdpReaderThread();

        //! Adds the provided socket to the socket reader for processing.
        //! @param socket pointer to the UdpSocket to read incoming data from
        //! @return boolean true on success, false for failure
        bool RegisterSocket(UdpSocket* socket);

        //! Removes the provided socket from the socket reader for processing.
        //! @param socket pointer to the UdpSocket to read incoming data from
        void UnregisterSocket(UdpSocket* socket);

        //! Returns the set of all packets consumed off the socket during the last call to ReadDataFromSocket().
        //! @return all packets consumed off the socket during the last call to ReadDataFromSocket()
        const ReceivedPackets* GetReceivedPackets(UdpSocket* socket) const;

        //! Should be called immediately before any registered sockets have processed their received packets.
        void SwapBuffers();

        //! Returns the number of active sockets bound to this thread.
        //! @return the number of active sockets bound to this thread
        uint32_t GetSocketCount() const;

        //! Gets the total elapsed time spent updating the background thread in milliseconds
        //! @return the total elapsed time spent updating the background thread in milliseconds
        AZ::TimeMs GetUpdateTimeMs() const;

    private:

        //! Helper to determine if a given socket is monitored by this reader thread instance
        bool SocketExists(UdpSocket* socket) const;

        void OnStart() override;
        void OnStop() override;
        void OnUpdate(AZ::TimeMs updateRateMs) override;

        AZ_DISABLE_COPY_MOVE(UdpReaderThread);

        struct SocketEntry
        {
            UdpSocket* m_socket;
            ReceivedPackets m_receivedPackets;
        };

        struct ReaderBuffer
        {
            AZStd::vector<SocketEntry> m_entries;
            ByteBuffer<MaxUdpReceiveBufferSize> m_receiveBuffer;
        };

        AZStd::recursive_mutex m_mutex;
        int32_t m_backIndex = 0;
        AZStd::array<ReaderBuffer, 2> m_readerBuffers;
        AZStd::vector<UdpSocket*> m_pendingAdds;
        AZ::TimeMs m_updateTimeMs = AZ::Time::ZeroTimeMs;
    };
}
