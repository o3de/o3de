/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/threadbus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Socket/AzSocket_fwd.h>

#include <AzFramework/Network/SocketConnection.h>
#include <AzFramework/Asset/AssetSystemTypes.h>

namespace AzFramework
{
    namespace AssetSystem
    {
        enum EConnectionThread
        {
            Listen,
            Send,
            Recv
        };

        typedef AZStd::function < void(EConnectionThread, AZ::AzSock::AzSockError) > TErrorCallback;
        typedef AZStd::function < void(AZSOCKADDR_IN&, AZSOCKET&) > TConnectionCallback;

        /**
        * AssetProcessorConnection - 
        * an implementation of the SocketConnection interface using AzSocket
        * It is primarily used for the AssetProcessor to connect to the game
        * and stream files across the network
        */
        class AssetProcessorConnection
            : public SocketConnection
        {
            struct ThreadState
            {
                AZStd::function<void()> m_main;
                AZStd::thread_desc m_desc;
                AZStd::thread m_thread;
                AZStd::atomic_bool m_join;
                AZStd::recursive_mutex m_mutex;

                ThreadState()
                {
                    m_join = false;
                }
            };

            using TMessageCallbacks = AZStd::multimap<AZ::u32, AZStd::pair<SocketConnection::TMessageCallbackHandle, SocketConnection::TMessageCallback>, AZStd::less<AZ::u32>, AZ::OSStdAllocator>;
            using TResponseCallbacks = AZStd::map<AZStd::pair<AZ::u32, AZ::u32>, SocketConnection::TMessageCallback, AZStd::less<AZStd::pair<AZ::u32, AZ::u32> >, AZ::OSStdAllocator>;

        public:
            
            AZ_RTTI(AssetProcessorConnection, "{C205009B-74EF-4C47-8623-917883756330}", SocketConnection);
            AssetProcessorConnection();
            ~AssetProcessorConnection();

            //////////////////////////////////////////////////////////////////////////
            // AzFramework::SocketConnection overrides

            //! disconnects any current connection and starts to reconnect
            bool Connect(const char* address, AZ::u16 port) override; 

            //! disconnects any current connection and stop any retrying to connect.
            //! @param completeDisconnect if true, guarantees that by the time Disconnect returns, we have disconnected and all threads are stopped.
            bool Disconnect(bool completeDisconnect = false) override; 

            //! disconnects any current connection and starts to listen for a connection
            bool Listen(AZ::u16 port) override;

            EConnectionState GetConnectionState() const override;
            bool IsConnected() const override;
            bool IsDisconnected() const override;
            bool SendMsg(AZ::u32 typeId, const void* dataBuffer, AZ::u32 dataLength) override;
            bool SendMsg(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength) override;
            bool SendRequest(AZ::u32 typeId, const void* dataBuffer, AZ::u32 dataLength, TMessageCallback handler) override;

            //! Will call callback whenever a message of type typeId arrives.
            //! @return returns a value that can be passed to /ref RemoveMessageHandler .
            SocketConnection::TMessageCallbackHandle AddMessageHandler(AZ::u32 typeId, TMessageCallback callback) override;
            void RemoveMessageHandler(AZ::u32 typeId, TMessageCallbackHandle callbackHandle) override;
            AZ::s32 GetLastResult() const override;
            AZStd::string GetLastErrorMessage() const override;
            //////////////////////////////////////////////////////////////////////////

            bool NegotiationFailed() { return m_negotiationFailed; } //hold whether the last connection attempt failed negotiation or not

            // Setup Connection Options
            void Configure(const char* branchToken, const char* platform, const char* identifier, const char* projectName);

            bool m_unitTesting; //set to true when unit testing

        protected:
            //negotiation
            bool NegotiateWithServer(); //note: StartSendRecvThreads() must be called before this call
            bool NegotiateWithClient(); //note: Calls StartSendRecvThreads() internally

            //thread helpers
            void StartThread(ThreadState& thread);
            void JoinThread(ThreadState& thread, AZStd::condition_variable* wakeUpCondition = nullptr);
            
            //Send/Recv threads
            void StartSendRecvThreads(); //starts up the threads that handle sending and recvieving messages
            void JoinSendRecvThreads(); //joins send recv threads
            void SendThread();
            void RecvThread();
            
            //connect
            void ConnectThread();
            
            //disconnect
            void DisconnectThread();

            //listen
            void ListenThread();

            //send recv messages
            bool SendRequest(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength, TMessageCallback handler);
            void QueueMessageForSend(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength);
            void InvokeMessageHandler(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength);
            void InvokeResponseHandler(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength);
            void AddResponseHandler(AZ::u32 typeId, AZ::u32 serial, SocketConnection::TMessageCallback callback);
            void RemoveResponseHandler(AZ::u32 typeId, AZ::u32 serial);

            AZ::u32 GetNextSerial();
            SocketConnection::TMessageCallbackHandle GetNewCallbackHandle();

            void DebugMessage(const char* format, ...); //shim to intercept debug messages

            void FlushResponseHandlers();


        private:

            AZSOCKET m_socket;//the socket for our connection
            AZStd::mutex m_sendQueueMutex;
            AZStd::recursive_mutex m_responseHandlerMutex;
            AZStd::atomic<EConnectionState> m_connectionState;
            AZStd::atomic_uint m_requestSerial;
            AZStd::atomic<SocketConnection::TMessageCallbackHandle> m_nextHandlerId;

            AZ::OSString m_connectAddr;//if initiating the connection, holds the address
            AZ::u16 m_port;//the port to connect to or listen on
            AZStd::string m_branchToken;//transmitted to ensure the ap is the one for our branch
            AZStd::string m_projectName;//transmitted to ensure the ap is the one for our project
            AZStd::string m_assetPlatform;//which platforms assets we want
            AZStd::string m_identifier;
            bool m_negotiationFailed = false;//holds if the last negotiation attempt failed or not
            AZStd::atomic_bool m_retryAfterDisconnect;//if true, it will constantly try to reconnect or listen after a disconnect
            bool m_listening = false;//controls whether we want to connect or listen after the disconnect

            // RecvThread
            bool m_shouldQueueHandlerChanges;
            TMessageCallbacks m_messageHandlers;
            AZStd::vector<SocketConnection::TMessageCallbackHandle, AZ::OSStdAllocator> m_messageHandlersToRemove;
            AZStd::vector<AZStd::pair<AZ::u32, AZStd::pair<SocketConnection::TMessageCallbackHandle, SocketConnection::TMessageCallback> >, AZ::OSStdAllocator> m_messageHandlersToAdd;
            TResponseCallbacks m_responseHandlers; // typed/nonced callbacks
            AZStd::vector<AZStd::pair<AZStd::pair<AZ::u32, AZ::u32>, SocketConnection::TMessageCallback>, AZ::OSStdAllocator> m_responseHandlersToAdd;
            AZStd::vector<AZStd::pair<AZ::u32, AZ::u32>, AZ::OSStdAllocator> m_responseHandlersToRemove;

            // SendThread
            typedef AZStd::vector<char, AZ::OSStdAllocator> MessageBuffer;
            typedef AZStd::deque<MessageBuffer, AZ::OSStdAllocator> SendQueueType;
            SendQueueType m_sendQueue;
            AZStd::condition_variable m_sendEventNotEmpty;

            // In general, this class operates by starting a cleanup thread (called the disconnect thread) to begin with.
            // this disconnect thread's job is to stop every other thread and clean all other state variables into some sane initial state, with no other threads active.
            // Then, the disconnect thread starts the connect/listen thread (the disconnect thread then exits)
            // the connect/listen thread waits to connect or for an incoming connection, and then starts a send/recv thread once connection is established (the connect/listen thread
            // then exits).
            // if any socket error or other problem happens, any of the service threads will start the disconnect/cleanup thread, which will shut everything down
            // and wait for all other threads to join, before it then restarts the entire sequence again.
            // This system thus has no need to 'pump' a main thread or have a long-lived management thread be active at any time, only ever having the thread(s) active that it requires to actually
            // be waiting on reads/writes/connects/listens.
            ThreadState m_sendThread;
            ThreadState m_recvThread;
            ThreadState m_listenThread;
            ThreadState m_connectThread;

            // the send, recv, listen, and connect threads should all call this if they need to terminate and retry.  It is safe to call from any
            // of these service threads at any time.
            void StartDisconnecting();

            // disconnect thread is special in that its the "tear down thread" for the entire system and is not
            // a persistent thread like send or recv.  Its job is to clean up all resources and then restart connection/listen
            // once everything has been reset into a zeroed out state and no other threads are running.
            // Thus, any time any thread encounters any error and the system needs to start over (such as a socket error),
            // they all start the disconnect thread and then exit, leaving it up to the disconnect thread to actually clean everything up.
            // Because ONLY the disconnect thread can start the connect/listen thread, and ONLY the connect/listen thread can start
            // the recv/send thread, the disconnect thread is the only thread that actually needs to protect itself from being started
            // twice or running in parallel. 
            ThreadState m_disconnectThread;

            // Whenever this value is True, it is not necessary to start another disconnection sequence, because one is already running and
            // has yet to finish cleaning everything up.
            AZStd::atomic_bool m_isBusyDisconnecting;

            AZ::s32 m_lastErrorResult = 0;
            AZStd::string m_lastErrorMessage;
        };

        typedef AZStd::vector<AZ::u8, AZ::OSStdAllocator> MessageBuffer;

        template <class Response>
        static bool SendResponse(const Response& response, AZ::u32 requestSerial)
        {
            requestSerial |= AzFramework::AssetSystem::RESPONSE_SERIAL_FLAG; //Set response bit
            return SendRequest(response, nullptr, requestSerial);
        }

        template <class Request>
        static bool SendRequest(const Request& request, SocketConnection* connection = nullptr, AZ::u32 serial = AzFramework::AssetSystem::DEFAULT_SERIAL)
        {
            if (!connection)
            {
                connection = SocketConnection::GetInstance();
            }

            AZ_Assert(connection, "SendRequest requires a valid SocketConnection");
            if (!connection)
            {
                return false;
            }

            return SendRequestToConnection(request, connection, serial);
        }

        template <class Request>
        static bool SendRequestToConnection(const Request& request, SocketConnection* connection, AZ::u32 serial = AzFramework::AssetSystem::DEFAULT_SERIAL)
        {
            AZ_Assert(connection, "SendRequest requires a valid SocketConnection");
            if (!connection)
            {
                return false;
            }

            MessageBuffer requestBuffer;
            bool serialized = PackMessage(request, requestBuffer);
            AZ_Assert(serialized, "AssetProcessor::SendRequest: failed to serialize");
            if (serialized)
            {
                return connection->SendMsg(request.GetMessageType(), serial, requestBuffer.data(), static_cast<AZ::u32>(requestBuffer.size()));
            }
            return false;
        }

        template <class Request, class Response>
        static bool SendRequest(const Request& request, Response& response, SocketConnection* connection = nullptr)
        {
            if (!connection)
            {
                connection = SocketConnection::GetInstance();
            }

            AZ_Assert(connection, "SendRequest requires a valid SocketConnection");
            if (!connection)
            {
                return false;
            }

            return SendRequestToConnection(request, response, connection);
        }

        template <class Request, class Response>
        static bool SendRequestToConnection(const Request& request, Response& response, SocketConnection* connection)
        {
            AZ_Assert(connection, "SendRequest requires a valid SocketConnection");
            if (!connection)
            {
                return false;
            }

            MessageBuffer requestBuffer;
            bool emptyResponseMessage = true;
            bool serialized = PackMessage(request, requestBuffer);
            AZ_Assert(serialized, "AssetProcessor::SendRequest: failed to serialize");
            if (serialized)
            {
                MessageBuffer responseBuffer;

                auto readResponseFunction = [&responseBuffer, &emptyResponseMessage](AZ::u32 connId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength)
                    {
                        (void)connId;
                        if (dataLength != 0)
                        {
                            responseBuffer.resize_no_construct(dataLength);
                            memcpy(responseBuffer.data(), data, dataLength);
                            emptyResponseMessage = false;
                        }
                    };

                if (connection->SendRequest(request.GetMessageType(), requestBuffer.data(), static_cast<AZ::u32>(requestBuffer.size()), readResponseFunction))
                {
                    // Return false if it is an empty response message
                    if (emptyResponseMessage)
                    {
                        return false;
                    }

                    bool deserialized = UnpackMessage(responseBuffer, response);
                    // because the response comes from a potentially unknown source, we are not going to assert simply becuase it was not unpackable.
                    AZ_Warning("AssetProcessorConnection", deserialized, "Unable to deserialize the response from the remote connection (for request/response %s, %s)", request.RTTI_GetTypeName(), response.RTTI_GetTypeName());
                    return deserialized;
                }
            }

            return false;
        }
    } // namespace AssetSystem
} // namespace AzFramework
