/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Carrier/StreamSocketDriver.h>
#include <AzCore/Socket/AzSocket.h>
#include <AzCore/std/typetraits/typetraits.h>

namespace GridMate
{
    // defined in SocketDriver.cpp
    namespace Platform
    {
        bool IsSocketError(AZ::s64 result);
        int GetSocketError();
        SocketDriverCommon::SocketType GetInvalidSocket();
        bool IsValidSocket(SocketDriverCommon::SocketType s);
    }
}

#define STREAM_PACKET_LOG 0
#if STREAM_PACKET_LOG
#define LOG_BYTES_STORED(headerSize,dataSize) AZ_TracePrintf("GridMate", "Storing_%d bytes %d:%d\n", __LINE__, headerSize, dataSize)
#define LOG_BYTES_SENT(headerSize,dataSize) AZ_TracePrintf("GridMate", "Sent_%d bytes %d:%d\n", __LINE__, headerSize, dataSize)
#define LOG_BYTES_RECV(dataSize) AZ_TracePrintf("GridMate", "Recv_%d bytes %d\n", __LINE__, dataSize)
#define LOG_BYTES_GOT(headerSize,dataSize) AZ_TracePrintf("GridMate", "GotPacket_%d bytes %d:%d\n", __LINE__, headerSize, dataSize)
#else
#define LOG_BYTES_STORED(headerSize,dataSize)  //
#define LOG_BYTES_SENT(headerSize,dataSize)    //
#define LOG_BYTES_RECV(dataSize)               //
#define LOG_BYTES_GOT(headerSize,dataSize)     //
#endif

//////////////////////////////////////////////////////////////////////////
// StreamSocketDriver::Hasher
//////////////////////////////////////////////////////////////////////////
namespace GridMate
{
    AZStd::size_t StreamSocketDriver::SocketPtrHasher::operator()(const SocketDriverAddressPtr& v) const
    {
        return SocketDriverAddress::Hasher()(*v);
    }
}

//////////////////////////////////////////////////////////////////////////
// StreamSocketDriver::RingBuffer
//////////////////////////////////////////////////////////////////////////
namespace GridMate
{

    StreamSocketDriver::RingBuffer::RingBuffer(AZ::u32 capacity)
        : m_data(nullptr)
        , m_capacity(capacity)
        , m_bytesStored(0)
        , m_indexStart(0)
        , m_indexEnd(0)
    {
    }

    StreamSocketDriver::RingBuffer::~RingBuffer()
    {
        Release();
    }

    void StreamSocketDriver::RingBuffer::Release()
    {
        m_bytesStored = 0;
        m_indexStart = 0;
        m_indexEnd = 0;
        delete[] m_data;
        m_data = nullptr;
    }

    char* StreamSocketDriver::RingBuffer::ReserveForWrite(AZ::u32& inOutSize)
    {
        // do lazy allocation
        Prepare();
        const AZ::u32 spaceUntilMarker = GetSpaceUntilMarker();
        if (spaceUntilMarker == 0)
        {
            inOutSize = 0;
            return nullptr;
        }
        AZ_Assert((0x80000000 & spaceUntilMarker) == 0, "this should never be a negative number");
        inOutSize = spaceUntilMarker;
        return m_data + m_indexEnd;
    }

    bool StreamSocketDriver::RingBuffer::CommitAsWrote(AZ::u32 dataSize, bool& bWroteToMarker)
    {
        bWroteToMarker = false;
        if (dataSize > GetSpaceUntilMarker())
        {
            return false;
        }
        m_bytesStored += dataSize;
        m_indexEnd += dataSize;
        AZ_Assert(m_indexEnd <= m_capacity, "end-index should never be larger than capacity");
        if (m_indexEnd == m_capacity)
        {
            m_indexEnd = 0;
            bWroteToMarker = true;
        }
        return true;
    }

    bool StreamSocketDriver::RingBuffer::Store(const char* data, AZ::u32 dataSize)
    {
        // do lazy allocation
        Prepare();
        if (dataSize > GetSpaceToWrite())
        {
            // would overflow ring buffer
            return false;
        }

        if (dataSize <= (m_capacity - m_indexEnd))
        {
            InternalWrite(data, dataSize);
        }
        else
        {
            // span the write
            AZ::u32 bytesToEnd = m_capacity - m_indexEnd;
            AZ::u32 bytesAfterWrap = dataSize - bytesToEnd;
            InternalWrite(data, bytesToEnd);
            AZ_Assert(m_indexEnd == 0, "Wrapping did not happen!");
            InternalWrite(data + bytesToEnd, bytesAfterWrap);
        }

        m_bytesStored += dataSize;
        return true;
    }

    template <typename T>
    bool StreamSocketDriver::RingBuffer::Store(const T data)
    {
        static_assert(AZStd::is_fundamental<T>::value, "Should only be used for fundamental primitive types");
        return Store(reinterpret_cast<const char*>(&data), sizeof(T));
    }

    bool StreamSocketDriver::RingBuffer::Peek(char* data, AZ::u32 dataSize)
    {
        AZ::u32 oldBytesUsed = m_bytesStored;
        AZ::u32 oldIndexStart = m_indexStart;

        bool ret = Fetch(data, dataSize);

        m_bytesStored = oldBytesUsed;
        m_indexStart = oldIndexStart;
        return ret;
    }

    template <typename T>
    bool StreamSocketDriver::RingBuffer::Peek(T* data)
    {
        static_assert(AZStd::is_fundamental<T>::value, "Should only be used for fundamental primitive types");
        return Peek(reinterpret_cast<char*>(data), sizeof(T));
    }

    bool StreamSocketDriver::RingBuffer::Fetch(char* data, AZ::u32 dataSize)
    {
        if (data == nullptr)
        {
            // invalid parameter(s)
            return false;
        }
        if (dataSize > m_bytesStored)
        {
            // would read beyond the end of the buffer
            return false;
        }

        if (dataSize <= m_capacity - m_indexStart)
        {
            InternalRead(data, dataSize);
        }
        else
        {
            // span the read
            AZ::u32 bytesLeft = dataSize - (m_capacity - m_indexStart);
            InternalRead(data, m_capacity - m_indexStart);
            InternalRead(data + (dataSize - bytesLeft), bytesLeft);
        }
        m_bytesStored -= dataSize;
        return true;
    }

    template <typename T>
    bool StreamSocketDriver::RingBuffer::Fetch(T* data)
    {
        static_assert(AZStd::is_fundamental<T>::value, "Should only be used for fundamental primitive types");
        return Fetch(reinterpret_cast<char*>(data), sizeof(T));
    }

    void StreamSocketDriver::RingBuffer::CommitAsRead(AZ::u32 size)
    {
        if (size > m_bytesStored)
        {
            return; // would set read beyond the end of the buffer
        }
        m_bytesStored -= size;
        m_indexStart += size;
        m_indexStart = m_indexStart % m_capacity;
    }

};

//////////////////////////////////////////////////////////////////////////
// StreamSocketDriver::Connection
//////////////////////////////////////////////////////////////////////////
namespace GridMate
{
    const AZ::u32 kMaxPacketSendSize = (1 << (sizeof(AZ::u16) * 8)) - 1;

    StreamSocketDriver::Connection::Connection(AZ::u32 inboundBufferSize, AZ::u32 outputBufferSize)
        : m_remoteAddress(nullptr)
        , m_initialized(false)
        , m_socket(Platform::GetInvalidSocket())
        , m_socketErrors()
        , m_sm()
        , m_inboundBuffer(inboundBufferSize)
        , m_outboundBuffer(outputBufferSize)
    {
    }

    StreamSocketDriver::Connection::~Connection()
    {
        Shutdown();
    }

    bool StreamSocketDriver::Connection::Initialize(ConnectionState startState, SocketType s, SocketDriverAddressPtr remoteAddress)
    {
        if (m_initialized)
        {
            return false;
        }
        m_socket = s;
        m_remoteAddress = remoteAddress;

        AZ::HSM::StateId startStateId = static_cast<AZ::HSM::StateId>(startState);
        if (m_sm.GetCurrentState() != startStateId)
        {
#define AZ_SS_HSM_STATE_NAME(_STATE) static_cast<AZ::HSM::StateId>(ConnectionState::_STATE),#_STATE
            const AZ::HSM::StateId topStateId = static_cast<AZ::HSM::StateId>(ConnectionState::TOP);
            m_sm.SetStateHandler(AZ_SS_HSM_STATE_NAME(TOP), AZ::HSM::StateHandler(this, &StreamSocketDriver::Connection::OnStateTop), AZ::HSM::InvalidStateId, startStateId);
            m_sm.SetStateHandler(AZ_SS_HSM_STATE_NAME(ACCEPT), AZ::HSM::StateHandler(this, &StreamSocketDriver::Connection::OnStateAccept), topStateId);
            m_sm.SetStateHandler(AZ_SS_HSM_STATE_NAME(CONNECTING), AZ::HSM::StateHandler(this, &StreamSocketDriver::Connection::OnStateConnecting), topStateId);
            m_sm.SetStateHandler(AZ_SS_HSM_STATE_NAME(CONNECT), AZ::HSM::StateHandler(this, &StreamSocketDriver::Connection::OnStateConnect), topStateId);
            m_sm.SetStateHandler(AZ_SS_HSM_STATE_NAME(ESTABLISHED), AZ::HSM::StateHandler(this, &StreamSocketDriver::Connection::OnStateEstablished), topStateId);
            m_sm.SetStateHandler(AZ_SS_HSM_STATE_NAME(DISCONNECTED), AZ::HSM::StateHandler(this, &StreamSocketDriver::Connection::OnStateDisconnected), topStateId);
            m_sm.SetStateHandler(AZ_SS_HSM_STATE_NAME(IN_ERROR), AZ::HSM::StateHandler(this, &StreamSocketDriver::Connection::OnStateError), topStateId);
            m_sm.Start();
#undef AZ_SS_HSM_STATE_NAME
        }
        m_initialized = true;
        return true;
    }

    void StreamSocketDriver::Connection::Shutdown()
    {
        if (m_initialized)
        {
            SocketOperations::CloseSocket(m_socket);
            m_inboundBuffer.Release();
            m_remoteAddress = nullptr;
            m_socket = Platform::GetInvalidSocket();
            m_initialized = false;
        }
    }

    void StreamSocketDriver::Connection::Update()
    {
        if (!m_sm.IsDispatching())
        {
            m_sm.Dispatch(ConnectionEvents::CE_UPDATE);
        }
    }

    void StreamSocketDriver::Connection::Close()
    {
        if (!m_sm.IsDispatching())
        {
            m_sm.Dispatch(ConnectionEvents::CE_CLOSE);
        }
    }

    StreamSocketDriver::ConnectionState StreamSocketDriver::Connection::GetConnectionState() const
    {
        return static_cast<StreamSocketDriver::ConnectionState>(m_sm.GetCurrentState());
    }

    bool StreamSocketDriver::Connection::SendPacket(const char* data, AZ::u32 dataSize)
    {
        if (!Platform::IsValidSocket(m_socket))
        {
            return false;
        }

        if (dataSize == 0)
        {
            // is an empty buffer?
            AZ_Assert(false, "inOutDataSize should be a non-zero value");
            return false;
        }
        else if ((0x80000000 & dataSize) != 0)
        {
            // is negative?
            AZ_Assert(false, "inOutDataSize should be a positive value");
            return false;
        }
        else if (dataSize > kMaxPacketSendSize)
        {
            // is negative?
            AZ_Assert(false, "inOutDataSize can not exceed the max send byte size of %d", kMaxPacketSendSize);
            return false;
        }

        const AZ::s32 kPacketDelimeterSize = sizeof(decltype(Packet::m_size));
        AZ::u16 packetSize = static_cast<AZ::u16>(dataSize);
        packetSize = SocketOperations::HostToNetShort(packetSize);

        // has over filled packet data to send?
        if (m_outboundBuffer.GetSpaceToRead() > 0 || m_sm.GetCurrentState() != static_cast<AZ::HSM::StateId>(ConnectionState::ESTABLISHED))
        {
            // have enough room to store another packet?
            if (m_outboundBuffer.GetSpaceToWrite() >= (dataSize + kPacketDelimeterSize))
            {
                LOG_BYTES_STORED(2, dataSize);
                m_outboundBuffer.Store(packetSize);
                m_outboundBuffer.Store(data, dataSize);   // store the entire packet to continue the packet stream
                return true;
            }
            AZ_TracePrintf("GridMate", "out bound network byte stream is full\n");
            return false;
        }

        // send the size of the packet
        AZ::u32 bytesSent = 0;
        if (SocketOperations::Send(m_socket, reinterpret_cast<char*>(&packetSize), kPacketDelimeterSize, bytesSent) == EC_OK)
        {
            if (kPacketDelimeterSize == bytesSent)
            {
                // send the size of the rest of the packet
                bytesSent = 0;
                if (SocketOperations::Send(m_socket, data, dataSize, bytesSent) == EC_OK)
                {
                    LOG_BYTES_SENT(2, bytesSent);
                    // did not send the entire packet?
                    if (bytesSent < dataSize)
                    {
                        LOG_BYTES_STORED(0, dataSize);
                        m_outboundBuffer.Store(data + bytesSent, dataSize - bytesSent);
                    }
                    return true;
                }
                else
                {
                    AZ_TracePrintf("GridMate", "Send body failed with:\n", Platform::GetSocketError());
                    StoreLastSocketError();
                }
            }
            else if (bytesSent == 1)
            {
                LOG_BYTES_SENT(1, 0);
                LOG_BYTES_STORED(1, dataSize);
                AZ::u8 nextPacketLengthByte = static_cast<AZ::u8>((packetSize & 0xFF00) >> 8);
                m_outboundBuffer.Store(nextPacketLengthByte); // store the other byte of the packet length
                m_outboundBuffer.Store(data, dataSize);       // store the entire packet to continue the packet stream
                return true;
            }
            else if (bytesSent == 0)
            {
                LOG_BYTES_SENT(0, 0);
                LOG_BYTES_STORED(2, dataSize);
                m_outboundBuffer.Store(packetSize);        // store the packet size in network byte order
                m_outboundBuffer.Store(data, dataSize);    // store the entire packet to continue the packet stream
                return true;
            }
        }
        else
        {
            AZ_TracePrintf("GridMate", "Send header failed with:\n", Platform::GetSocketError());
            StoreLastSocketError();
        }
        return false;
    }

    bool StreamSocketDriver::Connection::GetPacket(Packet& packet, char* data, AZ::u32 maxDataSize)
    {
        if (!Platform::IsValidSocket(m_socket))
        {
            return false;
        }

        packet.m_size = 0;
        packet.m_data = nullptr;

        if (m_inboundBuffer.GetSpaceToRead() > sizeof(packet.m_size))
        {
            AZ::u16 packetSize = 0;
            m_inboundBuffer.Peek(&packetSize);
            packetSize = SocketOperations::NetToHostShort(packetSize);

            // this is really bad since the stream is now out of sync!
            if (packetSize == 0)
            {
                StoreLastSocketError();
                return false;
            }

            // have enough room in the data buffer to store the packet?
            if (packetSize > maxDataSize)
            {
                return false;
            }

            // have enough bytes come in from the network to read in the entire packet?
            if (packetSize <= m_inboundBuffer.GetSpaceToRead())
            {
                LOG_BYTES_GOT(2, packetSize);
                m_inboundBuffer.Fetch(&packet.m_size);
                m_inboundBuffer.Fetch(data, packetSize);
                packet.m_data = data;
                packet.m_size = packetSize;
                return true;
            }
        }

        // no errors happened
        return true;
    }

    bool StreamSocketDriver::Connection::OnStateTop(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        if (e.id == AZ::HSM::EnterEventId)
        {
            return true;
        }
        else if (e.id == AZ::HSM::ExitEventId)
        {
            return true;
        }
        (void)sm;
        return false;
    }

    bool StreamSocketDriver::Connection::OnStateAccept(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        if (e.id == AZ::HSM::EnterEventId)
        {
            AZ_TracePrintf("GridMate", "Accepting a new connection for %s\n", m_remoteAddress->ToString().c_str());
            return true;
        }
        else if (e.id == AZ::HSM::ExitEventId)
        {
            return true;
        }
        else if (e.id == ConnectionEvents::CE_CLOSE)
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }

        if (Platform::IsValidSocket(m_socket))
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::ESTABLISHED));
        }
        else
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
        }
        return true;
    }

    bool StreamSocketDriver::Connection::OnStateConnecting(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        if (e.id == AZ::HSM::EnterEventId)
        {
            AZ_TracePrintf("GridMate", "Attempting to connect to %s\n", m_remoteAddress->ToString().c_str());
            return true;
        }
        else if (e.id == AZ::HSM::ExitEventId)
        {
            return true;
        }
        else if (e.id == ConnectionEvents::CE_CLOSE)
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }

        if (!Platform::IsValidSocket(m_socket))
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }

        SocketOperations::ConnectionResult connectionResult;
        if (SocketOperations::Connect(m_socket, *m_remoteAddress, connectionResult) != Driver::EC_OK)
        {
            StoreLastSocketError();
            AZ_TracePrintf("GridMate", "Connect attempt failed result: %d to %s\n", connectionResult, m_remoteAddress->ToString().c_str());
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
        }
        else
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::CONNECT));
        }
        return true;
    }

    bool StreamSocketDriver::Connection::OnStateConnect(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        if (e.id == AZ::HSM::EnterEventId)
        {
            return true;
        }
        else if (e.id == AZ::HSM::ExitEventId)
        {
            return true;
        }
        else if (e.id == ConnectionEvents::CE_CLOSE)
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }
        else if (!m_socketErrors.empty())
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }

        if (!Platform::IsValidSocket(m_socket))
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }

        if (SocketOperations::IsWritable(m_socket, AZStd::chrono::milliseconds(1)))
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::ESTABLISHED));
            return true;
        }

        return false;
    }

    bool StreamSocketDriver::Connection::OnStateEstablished(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        if (e.id == AZ::HSM::EnterEventId)
        {
            AZ_TracePrintf("GridMate", "Successfully established connection to %s\n", m_remoteAddress->ToString().c_str());
            EBUS_EVENT(StreamSocketDriverEventsBus, OnConnectionEstablished, *m_remoteAddress.get());
            return true;
        }
        else if (e.id == AZ::HSM::ExitEventId)
        {
            return true;
        }
        else if (e.id == ConnectionEvents::CE_CLOSE)
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
            return true;
        }
        else if (!m_socketErrors.empty())
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
            return true;
        }
        
        if (SocketOperations::IsReceivePending(m_socket, AZStd::chrono::microseconds(1)))
        {
            AZ::u32 inOutBytesSize = 0;
            char* bytesBuffer = m_inboundBuffer.ReserveForWrite(inOutBytesSize);
            if (bytesBuffer == nullptr || inOutBytesSize == 0)
            {
                AZ_TracePrintf("GridMate", "Connection read buffer is full for %s\n", m_remoteAddress->ToString().c_str());
                return false;
            }

            if (SocketOperations::Receive(m_socket, bytesBuffer, inOutBytesSize) != Driver::EC_OK)
            {
                StoreLastSocketError();
                sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
                return true;
            }
            LOG_BYTES_RECV(inOutBytesSize);

            // TCP socket closed?
            if (inOutBytesSize == 0)
            {
                sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
                return true;
            }

            bool bWroteToMarker = false;
            m_inboundBuffer.CommitAsWrote(inOutBytesSize, bWroteToMarker);
            if (bWroteToMarker && inOutBytesSize > 0)
            {
                // attempt to fill out the other side of the ring buffer
                inOutBytesSize = 0;
                bytesBuffer = m_inboundBuffer.ReserveForWrite(inOutBytesSize);
                if (bytesBuffer == nullptr || inOutBytesSize == 0)
                {
                    AZ_TracePrintf("GridMate", "Connection read buffer is full for %s\n", m_remoteAddress->ToString().c_str());
                    return false;
                }
                if (SocketOperations::Receive(m_socket, bytesBuffer, inOutBytesSize) != Driver::EC_OK)
                {
                    StoreLastSocketError();
                    sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
                    return true;
                }
                LOG_BYTES_RECV(inOutBytesSize);
                m_inboundBuffer.CommitAsWrote(inOutBytesSize, bWroteToMarker);
            }
        }

        // still sending data from the send buffer?
        while (m_outboundBuffer.GetSpaceToRead() > 0)
        {
            if (SocketOperations::IsWritable(m_socket, AZStd::chrono::microseconds(1)) == false)
            {
                break;
            }

            const AZ::u32 kDrainSize = 256;
            char buffer[kDrainSize];

            AZ::u32 fetchSize = AZStd::GetMin(kDrainSize, m_outboundBuffer.GetSpaceToRead());
            m_outboundBuffer.Peek(buffer, fetchSize);

            AZ::u32 bytesSent = 0;
            if (SocketOperations::Send(m_socket, &buffer[0], fetchSize, bytesSent) == EC_OK)
            {
                LOG_BYTES_SENT(0, bytesSent);

                m_outboundBuffer.CommitAsRead(bytesSent);
                
                if (m_outboundBuffer.GetSpaceToRead() == 0)
                {
                    return false;
                }
            }
            else
            {
                StoreLastSocketError();
                sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
                return true;
            }
        }
        return false;
    }

    bool StreamSocketDriver::Connection::OnStateDisconnected(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        (void)sm;
        if (e.id == AZ::HSM::EnterEventId)
        {
            AZ_TracePrintf("GridMate", "Lost connection to %s.\n", m_remoteAddress->ToString().c_str());
            if (m_remoteAddress)
            {
                EBUS_EVENT(StreamSocketDriverEventsBus, OnConnectionDisconnected, *m_remoteAddress.get());
            }
            Shutdown();
            return true;
        }
        else if (e.id == AZ::HSM::ExitEventId)
        {
            return true;
        }
        return false;
    }

    bool StreamSocketDriver::Connection::OnStateError(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        if (e.id == AZ::HSM::EnterEventId)
        {
            for (auto err : m_socketErrors)
            {
                (void)err;
                AZ_TracePrintf("GridMate", "Stream socketError:%d with remote:%s\n", err, m_remoteAddress->ToString().c_str());
            }
            return true;
        }
        else if (e.id == AZ::HSM::ExitEventId)
        {
            return true;
        }
        else if (e.id == CE_UPDATE)
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
            return true;
        }
        else if (e.id == ConnectionEvents::CE_CLOSE)
        {
            sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
            return true;
        }
        return false;
    }

    void StreamSocketDriver::Connection::StoreLastSocketError()
    {
        m_socketErrors.push_back(Platform::GetSocketError());
    }

    void StreamSocketDriver::Connection::ProcessInbound(bool& switchState, RingBuffer& inboundBuffer)
    {
        switchState = false;

        if (SocketOperations::IsReceivePending(m_socket, AZStd::chrono::microseconds(1)))
        {
            AZ::u32 inOutBytesSize = 0;
            char* bytesBuffer = inboundBuffer.ReserveForWrite(inOutBytesSize);
            if (bytesBuffer == nullptr || inOutBytesSize == 0)
            {
                AZ_TracePrintf("GridMate", "Connection read buffer is full for %s\n", m_remoteAddress->ToString().c_str());
                return;
            }

            if (SocketOperations::Receive(m_socket, bytesBuffer, inOutBytesSize) != Driver::EC_OK)
            {
                StoreLastSocketError();
                m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
                switchState = true;
                return;
            }
            LOG_BYTES_RECV(inOutBytesSize);

            // TCP socket closed?
            if (inOutBytesSize == 0)
            {
                m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
                switchState = true;
                return;
            }

            bool bWroteToMarker = false;
            inboundBuffer.CommitAsWrote(inOutBytesSize, bWroteToMarker);
            if (bWroteToMarker && inOutBytesSize > 0)
            {
                // attempt to fill out the other side of the ring buffer
                inOutBytesSize = 0;
                bytesBuffer = inboundBuffer.ReserveForWrite(inOutBytesSize);
                if (bytesBuffer == nullptr || inOutBytesSize == 0)
                {
                    AZ_TracePrintf("GridMate", "Connection read buffer is full for %s\n", m_remoteAddress->ToString().c_str());
                    return;
                }
                if (SocketOperations::Receive(m_socket, bytesBuffer, inOutBytesSize) != Driver::EC_OK)
                {
                    StoreLastSocketError();
                    m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::DISCONNECTED));
                    switchState = true;
                    return;
                }
                LOG_BYTES_RECV(inOutBytesSize);
                inboundBuffer.CommitAsWrote(inOutBytesSize, bWroteToMarker);
            }
        }
    }

    void StreamSocketDriver::Connection::ProcessOutbound(bool& switchState, RingBuffer& outboundBuffer)
    {
        switchState = false;

        // still sending data from the send buffer?
        while (outboundBuffer.GetSpaceToRead() > 0)
        {
            if (SocketOperations::IsWritable(m_socket, AZStd::chrono::microseconds(1)) == false)
            {
                break;
            }

            const AZ::u32 kDrainSize = 256;
            char buffer[kDrainSize];

            AZ::u32 fetchSize = AZStd::GetMin(kDrainSize, outboundBuffer.GetSpaceToRead());
            outboundBuffer.Peek(buffer, fetchSize);

            AZ::u32 bytesSent = 0;
            if (SocketOperations::Send(m_socket, &buffer[0], fetchSize, bytesSent) == EC_OK)
            {
                LOG_BYTES_SENT(0, bytesSent);

                outboundBuffer.CommitAsRead(bytesSent);
                if (outboundBuffer.GetSpaceToRead() == 0)
                {
                    return;
                }
            }
            else
            {
                StoreLastSocketError();
                m_sm.Transition(static_cast<AZ::HSM::StateId>(ConnectionState::IN_ERROR));
                switchState = true;
                return;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// StreamSocketDriver
//////////////////////////////////////////////////////////////////////////
namespace GridMate
{
    StreamSocketDriver::StreamSocketDriver(AZ::u32 maxConnections /*= 32*/, AZ::u32 maxPacketSize /*= 1024 * 64*/, AZ::u32 incomingBufferSize /*= 1024 * 64*/, AZ::u32 outgoingBufferSize /*= 1024 * 64*/)
        : SocketDriver(false, false)
        , m_maxConnections(maxConnections)
        , m_incomingBufferSize(incomingBufferSize)
        , m_outgoingBufferSize(outgoingBufferSize)
        , m_maxPacketSize(maxPacketSize)
        , m_maxSendSize(0)
        , m_maxReceiveSize(0)
        , m_isListening(false)
        , m_boundAddress()
        , m_boundSocketFamily(BSD_AF_INET)
        , m_connectionFactory()
        , m_connections()
    {
        AZ::AzSock::Startup();
        m_socket = Platform::GetInvalidSocket();
        m_isDatagram = false;
    }

    StreamSocketDriver::~StreamSocketDriver()
    {
        if (m_isListening)
        {
            StopListen();
        }
        CloseSocket();
        AZ::AzSock::Cleanup();
    }

    void StreamSocketDriver::Update()
    {
        // accept logic
        if (m_isListening)
        {
            while(m_connections.size() < m_maxConnections)
            {
                // prepare for 'any' socket address
                union
                {
                    sockaddr_in  sockAddrIn;
                    sockaddr_in6 sockAddrIn6;
                };
                sockaddr* outAddr = reinterpret_cast<sockaddr*>(&sockAddrIn6);
                socklen_t outAddrSize = sizeof(sockAddrIn6);
                SocketType outSocket;

                // check to see if there is a connection ready on the back log
                if (SocketOperations::Accept(m_socket, outAddr, outAddrSize, outSocket) == EC_OK)
                {
                    auto addr = AZStd::static_pointer_cast<SocketDriverAddress>(CreateDriverAddress(outAddr));

                    // make sure the exact same address+port is not already being processed
                    if (m_connections.find(addr) != m_connections.end())
                    {
                        AZ_Warning("GridMate", false, "Already have a connection to %s", addr->ToString().c_str());
                        continue;
                    }

                    // if the accpet() would block then EC_OK could come back, thus when the socket in valid AND accept succeeds then make a new Connection
                    if (Platform::IsValidSocket(outSocket))
                    {
                        Connection* c = m_connectionFactory(m_incomingBufferSize, m_outgoingBufferSize);
                        if (c->Initialize(ConnectionState::ACCEPT, outSocket, addr))
                        {
                            m_connections[addr] = c;
                        }
                        else
                        {
                            AZ_Warning("GridMate", false, "Could not initialize connection");
                            delete c;
                        }
                    }
                    else
                    {
                        // accept() worked but there is no new socket to connect
                        break;
                    }
                }
                else
                {
                    AZ_Warning("GridMate", false, "Accept() a connection failed with error:%d", Platform::GetSocketError());
                    break;
                }
            }

        }

        // update or remove connections
        auto itConn = m_connections.begin();
        while (itConn != m_connections.end())
        {
            if (itConn->second->GetConnectionState() == ConnectionState::DISCONNECTED)
            {
                delete itConn->second;
                itConn = m_connections.erase(itConn);
            }
            else
            {
                itConn->second->Update();
                ++itConn;
            }
        }
    }

    void StreamSocketDriver::CloseSocket()
    {
        if (Platform::IsValidSocket(m_socket))
        {
            SocketOperations::CloseSocket(m_socket);
        }
        m_port = 0;
        m_socket = Platform::GetInvalidSocket();
        for (auto& addrConn : m_connections)
        {
            delete addrConn.second;
        }
        m_connections.clear();
    }

    Driver::ResultCode StreamSocketDriver::PrepareSocket(AZ::u16 desiredPort, SocketAddressInfo& socketAddressInfo, SocketType& socket)
    {
        socket = Platform::GetInvalidSocket();
        if (!socketAddressInfo.Resolve(m_boundAddress.empty() ? nullptr : m_boundAddress.c_str(), desiredPort, m_boundSocketFamily, false, SocketAddressInfo::AdditionalOptionFlags::Passive))
        {
            return EC_SOCKET_CONNECT;
        }

        SocketType s = CreateSocket(socketAddressInfo.GetAddressInfo()->ai_family, socketAddressInfo.GetAddressInfo()->ai_socktype, socketAddressInfo.GetAddressInfo()->ai_protocol);
        if (!Platform::IsValidSocket(s))
        {
            return EC_SOCKET_CONNECT;
        }

        if (SocketOperations::SetSocketBlockingMode(s, false) != EC_OK)
        {
            AZ_TracePrintf("GridMate", "Socket error SetSocketBlockingMode:%d\n", Platform::GetSocketError());
            SocketOperations::CloseSocket(s);
            return EC_SOCKET_SOCK_OPT;
        }

        if (SocketOperations::EnableTCPNoDelay(s, false) != EC_OK)
        {
            AZ_TracePrintf("GridMate", "Socket error EnableTCPNoDelay:%d\n", Platform::GetSocketError());
            SocketOperations::CloseSocket(s);
            return EC_SOCKET_SOCK_OPT;
        }

        AZ::s32 sockOptVal = m_maxReceiveSize;
        if (SocketOperations::SetSocketOptionValue(s, SocketOperations::SocketOption::ReceiveBuffer, reinterpret_cast<char*>(&sockOptVal), sizeof(sockOptVal)) != EC_OK)
        {
            SocketOperations::CloseSocket(s);
            return EC_SOCKET_SOCK_OPT;
        }

        sockOptVal = m_maxSendSize;
        if (SocketOperations::SetSocketOptionValue(s, SocketOperations::SocketOption::SendBuffer, reinterpret_cast<char*>(&sockOptVal), sizeof(sockOptVal)) != EC_OK)
        {
            SocketOperations::CloseSocket(s);
            return EC_SOCKET_SOCK_OPT;
        }

        if (SocketOperations::SetSocketLingerTime(s, false, 0) != EC_OK)
        {
            SocketOperations::CloseSocket(s);
            return EC_SOCKET_SOCK_OPT;
        }

        socket = s;
        return EC_OK;
    }


    AZ::u32 StreamSocketDriver::GetMaxNumConnections() const
    {
        return m_maxConnections;
    }

    AZ::u32 StreamSocketDriver::GetMaxSendSize() const
    {
        return m_maxPacketSize;
    }

    AZ::u32 StreamSocketDriver::GetPacketOverheadSize() const
    {
        return sizeof(decltype(Packet::m_size));
    }

    Driver::ResultCode StreamSocketDriver::Initialize(AZ::s32 familyType /*= BSD_AF_INET*/, const char* address /*= nullptr*/, AZ::u32 port /*= 0*/, bool isBroadcast /*= false*/, AZ::u32 receiveBufferSize /*= 0*/, AZ::u32 sendBufferSize /*= 0*/)
    {
        (void)isBroadcast;
        AZ_Assert(BSD_AF_INET == familyType || BSD_AF_INET6 == familyType, "familyType can be IPV4 or IPV6 only! (see also BSDSocketFamilyType)");
        if (BSD_AF_INET != familyType && BSD_AF_INET6 != familyType)
        {
            return EC_SOCKET_CREATE;
        }
        m_isIpv6 = (familyType == BSD_AF_INET6);
        m_maxSendSize = sendBufferSize == 0 ? kMaxPacketSendSize : sendBufferSize;
        m_maxReceiveSize = receiveBufferSize == 0 ? (1024 * 256) : receiveBufferSize;
        m_port = AZ::AzSock::HostToNetShort(static_cast<AZ::u16>(port));
        m_boundSocketFamily = m_isIpv6 ? BSD_AF_INET6 : BSD_AF_INET;
        if (address != nullptr)
        {
            m_boundAddress.assign(address);
        }
        m_connectionFactory = [](AZ::u32 inboundBufferSize, AZ::u32 outputBufferSize)
        {
            return aznew Connection(inboundBufferSize, outputBufferSize);
        };
        return EC_OK;
    }

    AZ::u32 StreamSocketDriver::GetPort() const
    {
        return SocketDriverCommon::GetPort();
    }

    Driver::ResultCode StreamSocketDriver::Send(const AZStd::intrusive_ptr<DriverAddress>& to, const char* data, AZ::u32 dataSize)
    {
        if (dataSize > kMaxPacketSendSize)
        {
            AZ_TracePrintf("GridMate", "Tried to send dataSize:%d beyond the limit of:%d\n", dataSize, kMaxPacketSendSize);
            return EC_SEND;
        }

        const SocketDriverAddressPtr connKey = AZStd::static_pointer_cast<SocketDriverAddress>(to);
        auto conn = m_connections.find(connKey);
        if (conn == m_connections.end())
        {
            return EC_SEND_ADDRESS_NOT_BOUND;
        }

        if (conn->second->SendPacket(data, dataSize))
        {
            return EC_OK;
        }
        return EC_SEND;
    }

    AZ::u32 StreamSocketDriver::Receive(char* data, AZ::u32 maxDataSize, AZStd::intrusive_ptr<DriverAddress>& from, ResultCode* resultCode /*= 0*/)
    {
        const auto fnReturnResult = [resultCode](ResultCode rc, AZ::u32 bytesReceived)
        {
            if (resultCode != nullptr)
            {
                *resultCode = rc;
            }
            return bytesReceived;
        };

        for (auto& conn : m_connections)
        {
            Packet p;
            if (conn.second->GetPacket(p, data, maxDataSize))
            {
                if (p.m_size > 0)
                {
                    from = conn.first;
                    return fnReturnResult(EC_OK, p.m_size);
                }
            }
            else
            {
                return fnReturnResult(EC_RECEIVE, 0);
            }
        }
        return fnReturnResult(EC_OK, 0);
    }

    Driver::ResultCode StreamSocketDriver::ConnectTo(const SocketDriverAddressPtr& addr)
    {
        // already connected there?
        if (m_connections.find(addr) != m_connections.end())
        {
            return EC_SOCKET_CONNECT;
        }

        SocketAddressInfo socketAddressInfo;
        SocketType socket;
        if (PrepareSocket(0, socketAddressInfo, socket) != EC_OK)
        {
            return EC_SOCKET_CONNECT;
        }
        
        AZ_Error("GridMate", Platform::IsValidSocket(socket), "Made an invalid socket and still returned EC_OK");
        if (!Platform::IsValidSocket(socket))
        {
            return EC_SOCKET_CONNECT;
        }

        if (SocketOperations::Bind(socket, socketAddressInfo.GetAddressInfo()->ai_addr, socketAddressInfo.GetAddressInfo()->ai_addrlen) != EC_OK)
        {
            AZ_TracePrintf("GridMate", "StreamSocketDriver::Initialize - bind failed with code %d\n", Platform::GetSocketError());
            SocketOperations::CloseSocket(socket);
            return EC_SOCKET_BIND;
        }

        Connection* c = m_connectionFactory(m_incomingBufferSize, m_outgoingBufferSize);
        if (!c->Initialize(ConnectionState::CONNECTING, socket, addr.get()) )
        {
            delete c;
            return EC_SOCKET_CONNECT;
        }

        // all good, start connecting
        m_connections[addr] = c;
        return EC_OK;
    }

    Driver::ResultCode StreamSocketDriver::DisconnectFrom(const SocketDriverAddressPtr& addr)
    {
        // if connecting or established 
        auto connItr = m_connections.find(addr);
        if (connItr != m_connections.end())
        {
            // gracefully start the disconnect process
            connItr->second->Close();
            return EC_OK;
        }
        // not connecting, established, or even disconnected
        return EC_SOCKET_CLOSE;
    }

    Driver::ResultCode StreamSocketDriver::StartListen(AZ::s32 backlog)
    {
        AZ::u16 port = AZ::AzSock::NetToHostShort(static_cast<AZ::u16>(m_port));

        SocketAddressInfo socketAddressInfo;
        ResultCode res = PrepareSocket(static_cast<AZ::u16>(port), socketAddressInfo, m_socket);
        if (res != EC_OK)
        {
            return EC_SOCKET_CREATE;
        }

        res = SetSocketOptions(false, m_maxReceiveSize, m_maxSendSize);
        if (res != EC_OK)
        {
            CloseSocket();
            return res;
        }

        AZ::s32 bindResult = BindSocket(socketAddressInfo.GetAddressInfo()->ai_addr, socketAddressInfo.GetAddressInfo()->ai_addrlen);
        if (Platform::IsSocketError(bindResult))
        {
            AZ_TracePrintf("GridMate", "StreamSocketDriver::Initialize - bind failed with code %d at port %d\n", Platform::GetSocketError(), port);
            CloseSocket();
            return EC_SOCKET_BIND;
        }

        // if used implicit bind, retrieve the system assigned port
        if (port == 0)
        {
            m_port = socketAddressInfo.RetrieveSystemAssignedPort(m_socket);
            if (m_port == 0)
            {
                AZ_TracePrintf("GridMate", "StreamSocketDriver::Initialize - RetrieveSystemAssignedPort() failed with code %d at port %d\n", Platform::GetSocketError(), port);
                CloseSocket();
                return EC_SOCKET_BIND;
            }
        }
        else
        {
            m_port = AZ::AzSock::NetToHostShort(static_cast<AZ::u16>(port));
        }

        if (Platform::IsValidSocket(m_socket))
        {
            if (!Platform::IsSocketError(SocketOperations::Listen(m_socket, backlog)))
            {
                m_isListening = true;
                return EC_OK;
            }
        }
        return EC_SOCKET_LISTEN;
    }

    Driver::ResultCode StreamSocketDriver::StopListen()
    {
        if (Platform::IsValidSocket(m_socket))
        {
            // signal that all existing Connections to close
            for (auto& conn : m_connections)
            {
                conn.second->Close();
            }
            m_isListening = false;
            return EC_OK;
        }
        return EC_SOCKET_LISTEN;
    }

    AZ::u32 StreamSocketDriver::GetNumberOfConnections() const
    {
        AZ::u32 count = 0;
        for (const auto& c : m_connections)
        {
            if (c.second->GetConnectionState() == ConnectionState::ESTABLISHED)
            {
                ++count;
            }
        }
        return count;
    }

    bool StreamSocketDriver::IsConnectedTo(const SocketDriverAddressPtr& to) const
    {
        const auto conn = m_connections.find(to);
        if (conn != m_connections.end())
        {
            if (conn->second->GetConnectionState() == ConnectionState::ESTABLISHED)
            {
                return true;
            }
        }
        return false;
    }

    Driver::ResultCode StreamSocketDriver::SetSocketOptions(bool isBroadcast, AZ::u32 receiveBufferSize, AZ::u32 sendBufferSize)
    {
        (void)isBroadcast; // never true for TCP streaming driver

        ResultCode ret = SocketOperations::SetSocketBlockingMode(m_socket, false);
        if (Platform::IsSocketError(ret))
        {
            AZ_TracePrintf("GridMate", "Socket error SetSocketBlockingMode:%d\n", Platform::GetSocketError());
            return EC_SOCKET_MAKE_NONBLOCK;
        }

        ret = SocketOperations::EnableTCPNoDelay(m_socket, false);
        if (Platform::IsSocketError(ret))
        {
            AZ_TracePrintf("GridMate", "Socket error EnableTCPNoDelay:%d\n", Platform::GetSocketError());
            return EC_SOCKET_SOCK_OPT;
        }

        return SocketDriverCommon::SetSocketOptions(false, receiveBufferSize, sendBufferSize);
    }

} // namespace GridMate
