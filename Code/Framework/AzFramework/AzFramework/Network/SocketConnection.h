/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/parallel/mutex.h>

namespace AzFramework
{
    /**
    * SocketConnection - an interface for creating a socket connection using AzSocket
    */
    class SocketConnection
    {
    public:
        AZ_RTTI(SocketConnection, "{CF95282C-B514-4AB8-8C72-6063AD03725B}");
        virtual ~SocketConnection() {};

        //! Utility functions to get or set the one global instance of a SocketConnection
        static SocketConnection* GetInstance();
        static void SetInstance(SocketConnection* instance);

        //! Connect to an address with this connection. Takes a null-terminated string address and a port
        //! Currently this blocks until connected
        virtual bool Connect(const char* address, AZ::u16 port) = 0;

        //! Disconnect from connection - Only works if actually connected
        //! completeDisconnect - True if this function should wait for the connection to be fully
        //!                      terminated before returning. False for async behavior.
        virtual bool Disconnect(bool completeDisconnect = false) = 0;

        //! Listen for one connection on a port
        //!  This does not block and will set state to listening until connected
        virtual bool Listen(AZ::u16 port) = 0;

        enum class EConnectionState
        {
            Connected,
            Connecting,
            Disconnected,
            Disconnecting,
            Listening
        };

        //! Get the current connection state
        virtual EConnectionState GetConnectionState() const = 0;
        virtual bool IsConnected() const = 0;
        virtual bool IsDisconnected() const = 0;

        //! Callback parameters: Message ID, Serial, Data Buffer, Data Length
        //! Note that if the message is a request and it failed because of a protocol error (engine is shutting down for example)
        //! It is still guaranteed to be called, but will be called with 0, nullptr
        typedef AZStd::function<void(AZ::u32, AZ::u32, const void*, AZ::u32)> TMessageCallback;
        typedef AZ::u32 TMessageCallbackHandle;
        static const TMessageCallbackHandle s_invalidCallbackHandle = 0;

        //! Send a message across the connection by message id
        virtual bool SendMsg(AZ::u32 typeId, const void* dataBuffer, AZ::u32 dataLength) = 0;

        //! Send a message across the connection by message id
        virtual bool SendMsg(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength) = 0;

        //! Send a message and wait for the response
        virtual bool SendRequest(AZ::u32 typeId, const void* dataBuffer, AZ::u32 dataLength, TMessageCallback handler) = 0;

        //! Add callback for specific typeId (allows multiple callbacks per id)
        //! This will be invoked when a complete message is received from the remote end
        virtual TMessageCallbackHandle AddMessageHandler(AZ::u32 typeId, TMessageCallback callback) = 0;

        //! Remove callback for specific typeId (allows multiple callbacks per id)
        virtual void RemoveMessageHandler(AZ::u32 typeId, TMessageCallbackHandle callbackHandle) = 0;

        //! Get the last socket result code from any socket call 
        virtual AZ::s32 GetLastResult() const { return 0; }

        //! Get the last socket-related error from any socket call
        virtual AZStd::string GetLastErrorMessage() const { return AZStd::string(); };        
    };

    class EngineConnectionEvents
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EngineConnectionEvents>;
        using MutexType = AZStd::recursive_mutex; // this is invoked from different threads!

        EngineConnectionEvents() = default;
        ~EngineConnectionEvents() = default;

        virtual void Connected(SocketConnection*) {}
        virtual void Connecting(SocketConnection*) {}
        virtual void Listening(SocketConnection*) {}
        virtual void Disconnecting(SocketConnection*) {}
        virtual void Disconnected(SocketConnection*) {}
    };
} // namespace AzFramework
