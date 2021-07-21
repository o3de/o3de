/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef GM_STREAM_SOCKET_DRIVER_H
#define GM_STREAM_SOCKET_DRIVER_H

#include <GridMate/Carrier/Driver.h>
#include <GridMate/Carrier/SocketDriver.h>
#include <GridMate/Serialize/Buffer.h>
#include <GridMate/EBus.h>

#include <AzCore/State/HSM.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/functional.h>

namespace GridMate
{
    class StreamSocketDriverEventsInterface
        : public GridMateEBusTraits
    {
    public:
        virtual void OnConnectionEstablished(const SocketDriverAddress& address) = 0;
        virtual void OnConnectionDisconnected(const SocketDriverAddress& address) = 0;
    };
    typedef AZ::EBus<StreamSocketDriverEventsInterface> StreamSocketDriverEventsBus;

    /**
     * Handles TCP socket streaming protocol
     */
    class StreamSocketDriver
        : public SocketDriver
    {
    public:
        GM_CLASS_ALLOCATOR(StreamSocketDriver);

        using SocketDriverAddressPtr = AZStd::intrusive_ptr<SocketDriverAddress>;

        StreamSocketDriver(AZ::u32 maxConnections = 32, AZ::u32 maxPacketSize = 1024 * 64, AZ::u32 inboundBufferSize = 1024 * 64, AZ::u32 outboundBufferSize = 1024 * 64);
        virtual ~StreamSocketDriver();

        // Driver
        void Update() override;

        // SocketDriver
        AZ::u32 GetMaxNumConnections() const override;
        AZ::u32 GetMaxSendSize() const override;
        AZ::u32 GetPacketOverheadSize() const override;
        ResultCode Initialize(AZ::s32 familyType = BSD_AF_INET, const char* address = nullptr, AZ::u32 port = 0, bool isBroadcast = false, AZ::u32 receiveBufferSize = 0, AZ::u32 sendBufferSize = 0) override;
        AZ::u32 GetPort() const override;
        ResultCode Send(const AZStd::intrusive_ptr<DriverAddress>& to, const char* data, AZ::u32 dataSize) override;
        AZ::u32 Receive(char* data, AZ::u32 maxDataSize, AZStd::intrusive_ptr<DriverAddress>& from, ResultCode* resultCode = nullptr) override;

        // Stream operations
        ResultCode ConnectTo(const SocketDriverAddressPtr& addr);
        ResultCode DisconnectFrom(const SocketDriverAddressPtr& addr);
        ResultCode StartListen(AZ::s32 backlog);
        ResultCode StopListen();
        AZ::u32 GetNumberOfConnections() const;
        bool IsConnectedTo(const SocketDriverAddressPtr& to) const;
        bool IsListening() const { return m_isListening; }

    protected:
        ResultCode SetSocketOptions(bool isBroadcast, AZ::u32 receiveBufferSize, AZ::u32 sendBufferSize) override;
        void CloseSocket();
        ResultCode PrepareSocket(AZ::u16 desiredPort, SocketAddressInfo& socketAddressInfo, SocketType& socket);

        // state machine
        enum class ConnectionState
        {
            TOP,
            ACCEPT,          // accepts incoming connections
            CONNECTING,      // attempts to start connection
            CONNECT,         // polls an established connection
            ESTABLISHED,     // stream connection has been made
            DISCONNECTED,    // normal disconnect
            IN_ERROR,        // error
            MAX
        };

        struct Packet
        {
            Packet()
                : m_size(0)
                , m_data(nullptr)
            {
            }
            AZ::u16 m_size;
            char* m_data;
        };

        class RingBuffer
        {
        public:
            RingBuffer(AZ::u32 capacity);
            ~RingBuffer();
            RingBuffer(const RingBuffer& other) = delete;
            RingBuffer& operator=(const RingBuffer& other) = delete;

            AZ_FORCE_INLINE AZ::u32 GetCapacity() const { return m_capacity; }
            AZ_FORCE_INLINE AZ::u32 GetSpaceToWrite() const { return m_capacity - m_bytesStored; }
            AZ_FORCE_INLINE AZ::u32 GetSpaceToRead() const { return m_bytesStored; }

            char* ReserveForWrite(AZ::u32& inOutSize);
            bool CommitAsWrote(AZ::u32 dataSize, bool& bWroteToMarker);
            void CommitAsRead(AZ::u32 size);

            bool Store(const char* data, AZ::u32 dataSize);

            template <typename T>
            bool Store(const T data);

            bool Peek(char* data, AZ::u32 dataSize);

            template <typename T>
            bool Peek(T* data);

            bool Fetch(char* data, AZ::u32 dataSize);

            template <typename T>
            bool Fetch(T* data);

            void Release();

        protected:
            AZ_FORCE_INLINE void Prepare()
            {
                if (!m_data)
                {
                    m_data = new char[m_capacity];
                }
            }

            AZ_FORCE_INLINE void InternalWrite(const char* data, AZ::u32 dataSize)
            {
                memcpy(m_data + m_indexEnd, data, dataSize);
                m_indexEnd += dataSize;
                if (m_indexEnd == m_capacity)
                {
                    m_indexEnd = 0;
                }
            }

            AZ_FORCE_INLINE void InternalRead(char* data, AZ::u32 dataSize)
            {
                memcpy(data, m_data + m_indexStart, dataSize);
                m_indexStart += dataSize;
                if (m_indexStart == m_capacity)
                {
                    m_indexStart = 0;
                }
            }

            AZ_FORCE_INLINE AZ::u32 GetSpaceUntilMarker() const
            {
                if (m_data == nullptr)
                {
                    return 0;
                }
                AZ_Assert(m_bytesStored <= m_capacity, "m_bytesUsed exceeds m_capacity");
                if (m_bytesStored == m_capacity)
                {
                    return 0;
                }
                if (m_indexEnd >= m_indexStart)
                {
                    return m_capacity - m_indexEnd;
                }
                return m_indexStart - m_indexEnd;
            }

        private:
            char* m_data;
            AZ::u32 m_capacity;
            AZ::u32 m_bytesStored;
            AZ::u32 m_indexStart;
            AZ::u32 m_indexEnd;
        };

        class Connection
        {
        public:
            GM_CLASS_ALLOCATOR(Connection);
            
            Connection(AZ::u32 inboundBufferSize, AZ::u32 outputBufferSize);
            virtual ~Connection();

            virtual bool Initialize(ConnectionState startState, SocketType s, SocketDriverAddressPtr remoteAddress);
            virtual void Shutdown();
            virtual void Update();
            virtual void Close();
            ConnectionState GetConnectionState() const;

            virtual bool SendPacket(const char* data, AZ::u32 dataSize);
            virtual bool GetPacket(Packet& packet, char* data, AZ::u32 maxDataSize);

        protected:
            virtual bool OnStateTop(AZ::HSM& sm, const AZ::HSM::Event& e);
            virtual bool OnStateAccept(AZ::HSM& sm, const AZ::HSM::Event& e);
            virtual bool OnStateConnecting(AZ::HSM& sm, const AZ::HSM::Event& e);
            virtual bool OnStateConnect(AZ::HSM& sm, const AZ::HSM::Event& e);
            virtual bool OnStateEstablished(AZ::HSM& sm, const AZ::HSM::Event& e);
            virtual bool OnStateDisconnected(AZ::HSM& sm, const AZ::HSM::Event& e);
            virtual bool OnStateError(AZ::HSM& sm, const AZ::HSM::Event& e);

            void StoreLastSocketError();

            void ProcessInbound(bool& switchState, RingBuffer& inboundBuffer);
            void ProcessOutbound(bool& switchState, RingBuffer& outboundBuffer);

            AZ_FORCE_INLINE bool IsValidPacketDataSize(AZ::u32 dataSize, const AZ::u32 maxDataSize) const
            {
                if (dataSize == 0)
                {
                    // is an empty buffer?
                    AZ_Assert(false, "inOutDataSize should be a non-zero value");
                    return false;
                }
                else if ((0x80000000 & dataSize) != 0)
                {
                    // is negative?
                    AZ_Assert(false, "dataSize should be a positive value");
                    return false;
                }
                else if (dataSize > maxDataSize)
                {
                    // is negative?
                    AZ_Assert(false, "dataSize can not exceed the max send byte size of %d", maxDataSize);
                    return false;
                }

                return true;
            }

            enum ConnectionEvents
            {
                CE_UPDATE = 1,
                CE_CLOSE  = 2,
            };

            SocketDriverAddressPtr m_remoteAddress;
            bool m_initialized;
            SocketType m_socket;
            AZStd::vector<AZ::s64> m_socketErrors;
            AZ::HSM m_sm;
            RingBuffer m_inboundBuffer;
            RingBuffer m_outboundBuffer;
        };

        struct SocketPtrHasher
        {
            AZStd::size_t operator()(const SocketDriverAddressPtr& v) const;
        };

        using ConnectionMap = AZStd::unordered_map<SocketDriverAddressPtr, Connection*, SocketPtrHasher>;
        using ConnectionFactory = AZStd::function<Connection*(AZ::u32 inboundBufferSize, AZ::u32 outputBufferSize)>;

        AZ::u32 m_maxConnections;                   // max connections for the driver
        AZ::u32 m_incomingBufferSize;               // the size of the inbound ring buffer per connection
        AZ::u32 m_outgoingBufferSize;               // the size of the outbound ring buffer per connection
        AZ::u32 m_maxPacketSize;                    // the max packet size expected to be sent through the driver
        AZ::u32 m_maxSendSize;                      // used to set up the TCP socket option SO_SNDBUF
        AZ::u32 m_maxReceiveSize;                   // used to set up the TCP socket option SO_RCVBUF
        bool m_isListening;                         // listening for new connections
        AZStd::string m_boundAddress;               // the bound address name for the family
        BSDSocketFamilyType m_boundSocketFamily;    // socket family type to make sockets with: either IPv4 or IPv6

        // connection storage for both accepted sockets and direct connect sockets
        ConnectionFactory m_connectionFactory;
        ConnectionMap m_connections;
    };
}

#endif // GM_STREAM_SOCKET_DRIVER_H
