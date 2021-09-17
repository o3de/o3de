/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BarrierInputClient.h>
#include <BarrierInput/RawInputNotificationBus_Barrier.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

#include <AzCore/Console/ILogger.h>
#include <AzCore/Socket/AzSocket.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// The majority of this file was resurrected from legacy code, and could use some love, but it works.
namespace BarrierInput
{
    struct Stream
    {
        explicit Stream(int size)
        {
            buffer = (AZ::u8*)malloc(size);
            end = data = buffer;
            bufferSize = size;
            packet = nullptr;
        }

        ~Stream()
        {
            free(buffer);
        }

        AZ::u8* data;
        AZ::u8* end;
        AZ::u8* buffer;
        AZ::u8* packet;
        int bufferSize;

        void Rewind() { data = buffer; }
        int GetBufferSize() { return bufferSize; }

        char* GetBuffer() { return (char*)buffer; }
        char* GetData() { return (char*)data; }

        void SetLength(int len) { end = data + len; }
        int GetLength() { return (int)(end - data); }

        int ReadU32() { int ret = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]; data += 4; return ret; }
        int ReadU16() { int ret = (data[0] << 8) | data[1]; data += 2; return ret; }
        int ReadU8() { int ret = data[0]; data += 1; return ret; }
        void Eat(int len) { data += len; }

        void InsertString(const char* str) { int len = static_cast<int>(strlen(str)); memcpy(end, str, len); end += len; }
        void InsertU32(int a)
        {
            end[0] = static_cast<AZ::u8>(a >> 24);
            end[1] = static_cast<AZ::u8>(a >> 16);
            end[2] = static_cast<AZ::u8>(a >> 8);
            end[3] = static_cast<AZ::u8>(a);
            end += 4;
        }
        void InsertU16(int a)
        {
            end[0] = static_cast<AZ::u8>(a >> 8);
            end[1] = static_cast<AZ::u8>(a);
            end += 2;
        }
        void InsertU8(int a)
        {
            end[0] = static_cast<AZ::u8>(a);
            end += 1;
        }
        void OpenPacket() { packet = end; end += 4; }
        void ClosePacket()
        {
            int len = GetLength() - sizeof(AZ::u32);
            packet[0] = static_cast<AZ::u8>(len >> 24);
            packet[1] = static_cast<AZ::u8>(len >> 16);
            packet[2] = static_cast<AZ::u8>(len >> 8);
            packet[3] = static_cast<AZ::u8>(len);
            packet = nullptr;
        }
    };

    enum ArgType
    {
        ARG_END = 0,
        ARG_UINT8,
        ARG_UINT16,
        ARG_UINT32
    };
    constexpr int MAX_ARGS = 16;

    typedef bool (*packetCallback)(BarrierClient* pContext, int* pArgs, Stream* pStream, int streamLeft);

    struct Packet
    {
        const char* pattern;
        ArgType args[MAX_ARGS + 1];
        packetCallback callback;
    };

    static bool barrierSendFunc(BarrierClient* pContext, const char* buffer, int length)
    {
        int ret = AZ::AzSock::Send(pContext->GetSocket(), buffer, length, 0);
        return (ret == length) ? true : false;
    }

    static bool barrierPacket(BarrierClient* pContext, [[maybe_unused]]int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        Stream stream(256);
        stream.OpenPacket();
        stream.InsertString("Barrier");
        stream.InsertU16(1);
        stream.InsertU16(4);
        stream.InsertU32(static_cast<int>(pContext->GetClientScreenName().length()));
        stream.InsertString(pContext->GetClientScreenName().c_str());
        stream.ClosePacket();
        return barrierSendFunc(pContext, stream.GetBuffer(), stream.GetLength());
    }

    static bool barrierQueryInfo(BarrierClient* pContext, [[maybe_unused]]int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        Stream stream(256);
        stream.OpenPacket();
        stream.InsertString("DINF");
        stream.InsertU16(0);
        stream.InsertU16(0);

        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        AZ::RPI::ViewportContextPtr viewportContext = atomViewportRequests->GetDefaultViewportContext();
        if (viewportContext)
        {
            const AzFramework::WindowSize windowSize = viewportContext->GetViewportSize();
            stream.InsertU16(windowSize.m_width);
            stream.InsertU16(windowSize.m_height);
        }
        else
        {
            stream.InsertU16(1920);
            stream.InsertU16(1080);
        }
        stream.InsertU16(0);
        stream.InsertU16(0);
        stream.InsertU16(0);
        stream.ClosePacket();
        return barrierSendFunc(pContext, stream.GetBuffer(), stream.GetLength());
    }

    static bool barrierKeepAlive(BarrierClient* pContext, [[maybe_unused]]int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        Stream stream(256);
        stream.OpenPacket();
        stream.InsertString("CALV");
        stream.ClosePacket();
        return barrierSendFunc(pContext, stream.GetBuffer(), stream.GetLength());
    }

    static bool barrierEnterScreen([[maybe_unused]]BarrierClient* pContext, int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        const float positionX = static_cast<float>(pArgs[0]);
        const float positionY = static_cast<float>(pArgs[1]);
        RawInputNotificationBusBarrier::Broadcast(&RawInputNotificationsBarrier::OnRawMousePositionEvent,
                                                  positionX,
                                                  positionY);
        return true;
    }

    static bool barrierExitScreen([[maybe_unused]]BarrierClient* pContext, [[maybe_unused]]int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        return true;
    }

    static bool barrierMouseMove([[maybe_unused]]BarrierClient* pContext, int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        const float positionX = static_cast<float>(pArgs[0]);
        const float positionY = static_cast<float>(pArgs[1]);
        RawInputNotificationBusBarrier::Broadcast(&RawInputNotificationsBarrier::OnRawMousePositionEvent,
                                                  positionX,
                                                  positionY);
        return true;
    }

    static bool barrierMouseMoveRelative([[maybe_unused]]BarrierClient* pContext, int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        const float movementX = static_cast<float>(pArgs[0]);
        const float movementY = static_cast<float>(pArgs[1]);
        RawInputNotificationBusBarrier::Broadcast(&RawInputNotificationsBarrier::OnRawMouseMovementEvent,
                                                  movementX,
                                                  movementY);
        return true;
    }

    static bool barrierMouseButtonDown([[maybe_unused]]BarrierClient* pContext, int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        const uint32_t buttonIndex = pArgs[0];
        RawInputNotificationBusBarrier::Broadcast(&RawInputNotificationsBarrier::OnRawMouseButtonDownEvent, buttonIndex);
        return true;
    }

    static bool barrierMouseButtonUp([[maybe_unused]]BarrierClient* pContext, int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        const uint32_t buttonIndex = pArgs[0];
        RawInputNotificationBusBarrier::Broadcast(&RawInputNotificationsBarrier::OnRawMouseButtonUpEvent, buttonIndex);
        return true;
    }

    static bool barrierKeyboardDown([[maybe_unused]]BarrierClient* pContext, int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        const uint32_t scanCode = pArgs[2];
        const ModifierMask activeModifiers = static_cast<ModifierMask>(pArgs[1]);
        RawInputNotificationBusBarrier::Broadcast(&RawInputNotificationsBarrier::OnRawKeyboardKeyDownEvent, scanCode, activeModifiers);
        return true;
    }

    static bool barrierKeyboardUp([[maybe_unused]]BarrierClient* pContext, int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        const uint32_t scanCode = pArgs[2];
        const ModifierMask activeModifiers = static_cast<ModifierMask>(pArgs[1]);
        RawInputNotificationBusBarrier::Broadcast(&RawInputNotificationsBarrier::OnRawKeyboardKeyUpEvent, scanCode, activeModifiers);
        return true;
    }

    static bool barrierKeyboardRepeat([[maybe_unused]]BarrierClient* pContext, int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        const uint32_t scanCode = pArgs[2];
        const ModifierMask activeModifiers = static_cast<ModifierMask>(pArgs[1]);
        RawInputNotificationBusBarrier::Broadcast(&RawInputNotificationsBarrier::OnRawKeyboardKeyRepeatEvent, scanCode, activeModifiers);
        return true;
    }

    static bool barrierClipboard([[maybe_unused]]BarrierClient* pContext, int* pArgs, Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        for (int i = 0; i < pArgs[3]; i++)
        {
            int format = pStream->ReadU32();
            int size = pStream->ReadU32();
            if (format == 0) // Is text
            {
                char* clipboardContents = new char[size];
                memcpy(clipboardContents, pStream->GetData(), size);
                clipboardContents[size] = '\0';
                RawInputNotificationBusBarrier::Broadcast(&RawInputNotificationsBarrier::OnRawClipboardEvent, clipboardContents);
                delete[] clipboardContents;
            }
            pStream->Eat(size);
        }
        return true;
    }

    static bool barrierBye([[maybe_unused]]BarrierClient* pContext, [[maybe_unused]]int* pArgs, [[maybe_unused]]Stream* pStream, [[maybe_unused]]int streamLeft)
    {
        AZLOG_INFO("BarrierClient: Server said bye. Disconnecting\n");
        return false;
    }

    static Packet s_packets[] = {
        { "Barrier", { ARG_UINT16, ARG_UINT16 }, barrierPacket },
        { "QINF", {}, barrierQueryInfo },
        { "CALV", {}, barrierKeepAlive },
        { "CINN", { ARG_UINT16, ARG_UINT16, ARG_UINT32, ARG_UINT16 }, barrierEnterScreen },
        { "COUT", { }, barrierExitScreen },
        { "CBYE", { }, barrierBye },
        { "DMMV", { ARG_UINT16, ARG_UINT16 }, barrierMouseMove },
        { "DMRM", { ARG_UINT16, ARG_UINT16 }, barrierMouseMoveRelative },
        { "DMDN", { ARG_UINT8 }, barrierMouseButtonDown },
        { "DMUP", { ARG_UINT8 }, barrierMouseButtonUp },
        { "DKDN", { ARG_UINT16, ARG_UINT16, ARG_UINT16 }, barrierKeyboardDown },
        { "DKUP", { ARG_UINT16, ARG_UINT16, ARG_UINT16 }, barrierKeyboardUp },
        { "DKRP", { ARG_UINT16, ARG_UINT16, ARG_UINT16, ARG_UINT16 }, barrierKeyboardRepeat },
        { "DCLP", { ARG_UINT8, ARG_UINT32, ARG_UINT32, ARG_UINT32 }, barrierClipboard }
    };

    static bool ProcessPackets(BarrierClient* pContext, Stream& stream)
    {
        while (stream.data < stream.end)
        {
            const int packetLength = stream.ReadU32();
            const int streamLength = stream.GetLength();
            const char* packetStart = stream.GetData();
            if (packetLength > streamLength)
            {
                AZLOG_INFO("BarrierClient: Packet overruns buffer (Packet Length: %d Buffer Length: %d), probably lots of data on clipboard?\n", packetLength, streamLength);
                return false;
            }

            const int numPackets = sizeof(s_packets) / sizeof(s_packets[0]);
            int i;
            for (i = 0; i < numPackets; ++i)
            {
                const int len = static_cast<int>(strlen(s_packets[i].pattern));
                if (packetLength >= len && memcmp(stream.GetData(), s_packets[i].pattern, len) == 0)
                {
                    bool bDone = false;
                    int numArgs = 0;
                    int args[MAX_ARGS];
                    stream.Eat(len);
                    while (!bDone)
                    {
                        switch (s_packets[i].args[numArgs])
                        {
                        case ARG_UINT8:
                            args[numArgs++] = stream.ReadU8();
                            break;
                        case ARG_UINT16:
                            args[numArgs++] = stream.ReadU16();
                            break;
                        case ARG_UINT32:
                            args[numArgs++] = stream.ReadU32();
                            break;
                        case ARG_END:
                            bDone = true;
                            break;
                        }
                    }
                    if (s_packets[i].callback)
                    {
                        if (!s_packets[i].callback(pContext, args, &stream, packetLength - (int)(stream.GetData() - packetStart)))
                        {
                            return false;
                        }
                    }
                    stream.Eat(packetLength - (int)(stream.GetData() - packetStart));
                    break;
                }
            }
            if (i == numPackets)
            {
                stream.Eat(packetLength);
            }
        }
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    BarrierClient::BarrierClient(const char* clientScreenName, const char* serverHostName, AZ::u32 connectionPort)
        : m_clientScreenName(clientScreenName)
        , m_serverHostName(serverHostName)
        , m_connectionPort(connectionPort)
        , m_socket(AZ_SOCKET_INVALID)
        , m_threadHandle()
        , m_threadQuit(false)
    {
        AZStd::thread_desc threadDesc;
        threadDesc.m_name = "BarrierInputClientThread";
        m_threadHandle = AZStd::thread(threadDesc, AZStd::bind(&BarrierClient::Run, this));
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    BarrierClient::~BarrierClient()
    {
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::CloseSocket(m_socket);
        }
        m_threadQuit = true;
        m_threadHandle.join();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void BarrierClient::Run()
    {
        Stream stream(4 * 1024);
        bool connected = false;
        while (!m_threadQuit)
        {
            if (!connected)
            {
                connected = ConnectToServer();
                continue;
            }

            const int lengthReceived = AZ::AzSock::Recv(m_socket, stream.GetBuffer(), stream.GetBufferSize(), 0);
            if (lengthReceived <= 0)
            {
                AZLOG_INFO("BarrierClient: Receive failed, reconnecting.\n");
                connected = false;
                continue;
            }

            stream.Rewind();
            stream.SetLength(lengthReceived);
            if (!ProcessPackets(this, stream))
            {
                AZLOG_INFO("BarrierClient: Packet processing failed, reconnecting.\n");
                connected = false;
                continue;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool BarrierClient::ConnectToServer()
    {
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::CloseSocket(m_socket);
        }

        m_socket = AZ::AzSock::Socket();
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::AzSocketAddress socketAddress;
            if (socketAddress.SetAddress(m_serverHostName.c_str(), static_cast<AZ::u16>(m_connectionPort)))
            {
                const int result = AZ::AzSock::Connect(m_socket, socketAddress);
                if (!AZ::AzSock::SocketErrorOccured(result))
                {
                    return true;
                }
            }
            AZ::AzSock::CloseSocket(m_socket);
            m_socket = AZ_SOCKET_INVALID;
        }

        return false;
    }
} // namespace BarrierInput
