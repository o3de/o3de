/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Network/AssetProcessorConnection.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Platform.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Socket/AzSocket.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <inttypes.h>

// Enable this if you want to see a message for every socket/session event
//#define ASSETPROCESORCONNECTION_VERBOSE_LOGGING

namespace AzFramework
{
    namespace AssetSystem
    {
        namespace Platform
        {
            void DebugOutput(const char* message);
        }

        namespace
        {
            bool GetValidAzSocket(AZSOCKET& outSocket)
            {
                // Socket already valid
                if (AZ::AzSock::IsAzSocketValid(outSocket))
                {
                    return true;
                }

                // Create new socket
                outSocket = AZ::AzSock::Socket();
                // Check new socket for validity
                if (!AZ::AzSock::IsAzSocketValid(outSocket))
                {
                    // Error, something went wrong getting a socket
                    AZ_Error("AssetProcessorConnection", false, "AssetProcessorConnection - GetValidAzSocket - AZ::AzSock::Socket returned %d", outSocket);
                    return false;
                }
                return true;
            }

            AZ::u32 CurrentProcessID()
            {
                return AZ::Platform::GetCurrentProcessId();
            }
        }

        AssetProcessorConnection::AssetProcessorConnection()
            : m_socket(AZ_SOCKET_INVALID)
            , m_connectionState(EConnectionState::Disconnected)
            , m_port(0)
            , m_sendEventNotEmpty()
        {
            m_requestSerial = 1;
            m_unitTesting = false;
            m_shouldQueueHandlerChanges = false;
            m_retryAfterDisconnect = false;
            m_isBusyDisconnecting = false;

            m_sendThread.m_desc.m_name = "APConnectSend";
            m_sendThread.m_main = AZStd::bind(&AssetProcessorConnection::SendThread, this);

            m_recvThread.m_desc.m_name = "APConnectRecv";
            m_recvThread.m_main = AZStd::bind(&AssetProcessorConnection::RecvThread, this);

            // Put the AP send and receive threads on a core not shared by the main thread or render thread.
            // We have found cases where render thread would stall on a Send request, but the main thread would be running on core 0
            // and starve the send and receive threads.  Putting them on a lighter CPU prevents this starvation.
            m_sendThread.m_desc.m_cpuId = AZ_TRAIT_THREAD_AFFINITY_MASK_ASSET_PROCESSOR_CONNECTION_THREAD;
            m_recvThread.m_desc.m_cpuId = AZ_TRAIT_THREAD_AFFINITY_MASK_ASSET_PROCESSOR_CONNECTION_THREAD;

            m_disconnectThread.m_desc.m_name = "APConnection::DisconnectThread";
            m_disconnectThread.m_main = [this]()
            {
                DisconnectThread();
            };

            m_listenThread.m_desc.m_name = "APConnection::ListenThread";
            m_listenThread.m_main = AZStd::bind(&AssetProcessorConnection::ListenThread, this);

            m_connectThread.m_desc.m_name = "APConnection::ConnectThread";
            m_connectThread.m_main = [this]()
            {
                ConnectThread();
            };

            int result = AZ::AzSock::Startup();
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                AZ_Error("AssetProcessorConnection", false, "AZ::AzSock::startup returned an error (%s)", AZ::AzSock::GetStringForError(result));
            }
        }

        AssetProcessorConnection::~AssetProcessorConnection()
        {
            Disconnect(true);

            //cleanup
            int result = AZ::AzSock::Cleanup();
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                DebugMessage("~AssetProcessorConnection cleanup returned an error (%s)", AZ::AzSock::GetStringForError(result));
            }
        }

        void AssetProcessorConnection::DebugMessage(const char* format, ...)
        {
#ifdef ASSETPROCESORCONNECTION_VERBOSE_LOGGING
            constexpr size_t MaxMessageSize = 2048;
            AZStd::array<char, MaxMessageSize> msg;
            va_list va;
            va_start(va, format);
            azvsnprintf(msg.data(), msg.size(), format, va);
            va_end(va);

            static_assert(sizeof(AZStd::thread_id) <= sizeof(uintptr_t), "Thread Id should less than a size of a pointer");
            uintptr_t numericThreadId{};
            *reinterpret_cast<AZStd::thread_id*>(&numericThreadId) = AZStd::this_thread::get_id();

            AZStd::array<char, MaxMessageSize> buffer;
            azsnprintf(buffer.data(), buffer.size(), "(%p/%#" PRIxPTR "): %s\n", this, numericThreadId, msg.data());

            AZ::Debug::Platform::OutputToDebugger("AssetProcessorConnection", buffer.data());
#else // ASSETPROCESORCONNECTION_VERBOSE_LOGGING is not defined
            (void)format;
#endif
        }

        AZ::u32 AssetProcessorConnection::GetNextSerial()
        {
            AZ::u32 nextSerial = m_requestSerial++;

            // Avoid special-case serials
            return (nextSerial == AzFramework::AssetSystem::DEFAULT_SERIAL
                || nextSerial == AssetSystem::NEGOTIATION_SERIAL
                || (nextSerial & AzFramework::AssetSystem::RESPONSE_SERIAL_FLAG))
                ? GetNextSerial() // re-roll, we picked a special serial
                : nextSerial;
        }

        bool AssetProcessorConnection::Connect(const char* address, AZ::u16 port)
        {
            //set the input parameters
            m_connectAddr = address;
            m_port = port;

            //set listening to false, as we are connecting
            m_listening = false;
            m_negotiationFailed = false;

            //set retry after disconnect to true and kick off the disconnect thread
            //the idea here is to disconnect from any connection we may have
            //stop all the associated threads for that connection and try
            //to connect after that is done
            m_retryAfterDisconnect = true;
            StartDisconnecting();

            return true;
        }

        //trying to connect, if any error occurs start the disconnect thread
        void AssetProcessorConnection::ConnectThread()
        {
            [[maybe_unused]] static constexpr char ConnectThreadWindow[] = "AssetProcessorConnection::ConnectThread";

            //check for join before doing an op
            if (m_connectThread.m_join)
            {
                DebugMessage("AssetProcessorConnection::ConnectThread - Join");
                return;
            }

            if (!m_retryAfterDisconnect)
            {
                return;
            }

            DebugMessage("AssetProcessorConnection::ConnectThread - Start to connect to %s:%u...", m_connectAddr.c_str(), m_port);
            AZ_Assert(m_connectionState != EConnectionState::Listening, "");

            //send the connecting event
            DebugMessage("AssetProcessorConnection::ConnectThread - Sending connecting event.");
            m_connectionState = EConnectionState::Connecting;
            EngineConnectionEvents::Bus::Broadcast(&EngineConnectionEvents::Bus::Events::Connecting, this);

            //set the address
            AZ::AzSock::AzSocketAddress socketAddress;
            if (!socketAddress.SetAddress(m_connectAddr.c_str(), m_port))
            {
                DebugMessage("AssetProcessorConnection::ConnectThread - could not obtain numeric address from string (%s)", m_connectAddr.c_str());
                if (m_connectThread.m_join)
                {
                    return;
                }
                StartDisconnecting();
                return;
            }

            //make sure we have a valid socket
            if (!GetValidAzSocket(m_socket))
            {
                DebugMessage("AssetProcessorConnection::ConnectThread - No valid socket available to connect");
                if (m_connectThread.m_join)
                {
                    return;
                }
                StartDisconnecting();
                return;
            }

            //connect, we are non blocking and if there was an error we pick it up on the select
            AZ::s32 result = AZ::AzSock::Connect(m_socket, socketAddress);
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                m_lastErrorResult = result;
                m_lastErrorMessage = AZStd::string::format("Network connection attempt failure, Connect returned error %s", AZ::AzSock::GetStringForError(result));
                DebugMessage("%s", m_lastErrorMessage.c_str());

                if (m_connectThread.m_join)
                {
                    return;
                }

                StartDisconnecting();
                return;
            }

            result = AZ::AzSock::EnableTCPNoDelay(m_socket, true);
            if (result)
            {
                m_lastErrorResult = result;
                m_lastErrorMessage = AZStd::string::format("Network connection attempt failure, EnableTCPNoDelay returned an error %s", AZ::AzSock::GetStringForError(result));
                DebugMessage("%s", m_lastErrorMessage.c_str());
                if (m_connectThread.m_join)
                {
                    return;
                }
                StartDisconnecting();
                return;
            }

            DebugMessage("AssetProcessorConnection::ConnectThread - socket connected to %s:%u, Negotiate...", m_connectAddr.c_str(), m_port);

            m_lastErrorResult = 0;
            m_lastErrorMessage.clear();

            //we connected, start the send recv threads so we can negotiate
            StartSendRecvThreads();

            //send the negotiation, if we fail we have to disconnect
            if (m_unitTesting || NegotiateWithServer())
            {
                DebugMessage("AssetProcessorConnection::ConnectThread - Negotiation with %s:%u, Successful.", m_connectAddr.c_str(), m_port);
                //we successfully negotiated, send the connected event
                m_connectionState = EConnectionState::Connected;
                EngineConnectionEvents::Bus::Broadcast(&EngineConnectionEvents::Bus::Events::Connected, this);
                return;
            }

            if (m_connectThread.m_join)
            {
                return;
            }

            //we failed start the disconnect thread and try again if retry is set
            AZ_Warning(ConnectThreadWindow, false, "Network connection attempt failure, negotiation with %s:%u failed.", m_connectAddr.c_str(), m_port);
            // when we are the one connecting outbound, a negotiation failure is terminal
            // we cannot recover, the thing we're trying to connect to is not an asset processor, so disable automatic retry:
            m_retryAfterDisconnect = false;
            StartDisconnecting();
        }

        //disconnect is public and can be pounded on by an app, so protect out threads
        bool AssetProcessorConnection::Disconnect(bool completeDisconnect)
        {
            // do not call this function from any of the service threads, or you are certain to deadlock immediately.
            // instead, call StartDisconnecting.   Disconnect is the API call for outsiders to call on their own threads.
            m_retryAfterDisconnect = false;

            // now that we have disabled retry, we do an atomic exchange to detect whether the disconnect function is
            // already running.  m_isBusyDisconnecting is true whenever something is either trying to start the disconnect
            // thread, or the thread itself has not yet reached the point at which it has finished tearing everything down
            // and will try to retry (using the above bool).  This means that if we successfully exchange it false to true,
            // it means we need to wait for the thread to finish and then run it once more to clean up.
            // otherwise, its already running and will eventually check the above value, so we only need to wait for it.

            bool wasDisconnecting = false;
            m_isBusyDisconnecting.compare_exchange_strong(wasDisconnecting, true);
            if (!wasDisconnecting)
            {
                DebugMessage("Disconnect(...) is in control of performing the disconnection sequence.");
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_disconnectThread.m_mutex);
                JoinThread(m_disconnectThread);
                m_disconnectThread.m_main();
            }
            else
            {
                DebugMessage("Disconnect(...) is in not in control of performing the disconnection sequence, waiting");
                // this ensures that the disconnection thread is at least started by whoever was in control of it
                // before we continue.  Its vital we do this, since we're about to call join() after this, and until this
                // flips back to false, its not safe to assume the disconnection thread has started yet.
                while (m_isBusyDisconnecting.load())
                {
                    AZStd::this_thread::yield();
                }
            }

            if (completeDisconnect)
            {
                DebugMessage("Disconnect(...) waiting for the disconnect thread to join");
                JoinThread(m_disconnectThread);
            }
            DebugMessage("Disconnect - done");
            return true;
        }

        //The disconnect thread has only 1 function, shutdown all other threads then send the disconnect event,
        //and if retry is set, restart the connect or listen thread, before dying itself
        void AssetProcessorConnection::DisconnectThread()
        {
            DebugMessage("AssetProcessorConnection::DisconnectThread - Disconnecting %d", m_socket);

            //set join on all the other threads so they can exit asap
            m_recvThread.m_join = true;
            m_sendThread.m_join = true;
            m_listenThread.m_join = true;
            m_connectThread.m_join = true;

            //set disconnecting state
            m_connectionState = EConnectionState::Disconnecting;
            EngineConnectionEvents::Bus::Broadcast(&EngineConnectionEvents::Bus::Events::Disconnecting, this);

            FlushResponseHandlers();

            //call all response handlers and clear them out
            DebugMessage("AssetProcessorConnection::DisconnectThread - notifying all response handlers of disconnection.");

            //clean up the socket if it is valid
            if (AZ::AzSock::IsAzSocketValid(m_socket))
            {
                DebugMessage("AssetProcessorConnection::DisconnectThread - Socket is valid %d, clean it up.", m_socket);

                // shutdown the socket, which will kill the Recv/Listen threads
                DebugMessage("AssetProcessorConnection::DisconnectThread - shutdown(%d)", m_socket);
                int result = AZ::AzSock::Shutdown(m_socket, SD_BOTH);
                if (AZ::AzSock::SocketErrorOccured(result))
                {
                    DebugMessage("AssetProcessorConnection::DisconnectThread - shutdown returned an error %s", AZ::AzSock::GetStringForError(result));
                }

                //close the socket
                DebugMessage("AssetProcessorConnection::DisconnectThread - closesocket(%d)", m_socket);
                result = AZ::AzSock::CloseSocket(m_socket);
                if (AZ::AzSock::SocketErrorOccured(result))
                {
                    DebugMessage("AssetProcessorConnection::DisconnectThread - closesocket returned an error %s", AZ::AzSock::GetStringForError(result));
                }

                //set socket to invalid
                DebugMessage("AssetProcessorConnection::DisconnectThread - Socket Set to Invalid %d", m_socket);
                m_socket = AZ_SOCKET_INVALID;
            }

            DebugMessage("AssetProcessorConnection::DisconnectThread - Joining worker threads");
            JoinSendRecvThreads();

            // Ensure that the connect thread dies if this is an intentional disconnect

            JoinThread(m_connectThread);
            JoinThread(m_listenThread);

            FlushResponseHandlers();

            // clear the send queue so there is no stale data!
            {
                SendQueueType emptyQueue;
                m_sendQueue.swap(emptyQueue);
            }

            //set disconnected state
            DebugMessage("AssetProcessorConnection::DisconnectThread - All threads joined, Setting Disconnected state.");
            m_connectionState = EConnectionState::Disconnected;
            EngineConnectionEvents::Bus::Broadcast(&EngineConnectionEvents::Bus::Events::Disconnected, this);

            DebugMessage("AssetProcessorConnection::DisconnectThread - crossing the isBusyDisconnecting line");
            // once we cross this line, we would need to restart the disconnect thread in order to properly disconnect
            // since we're about to potentially start other threads if m_retryAfterDisconnect is true.
            m_isBusyDisconnecting = false;

            // if necessary, start the listening or connect thread to begin another attempt.
            {
                DebugMessage("AssetProcessorConnection::DisconnectThread - m_retryAfterDisconnect = %s", m_retryAfterDisconnect ? "True" : "False");
                if (m_retryAfterDisconnect)
                {
                    // wait for a short while so we don't make 1000s of connects per second
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
                    if ((m_retryAfterDisconnect) && (!m_disconnectThread.m_join))
                    {
                        if (m_listening)
                        {
                            DebugMessage("AssetProcessorConnection::DisconnectThread - Start Listen Thread");
                            StartThread(m_listenThread);
                        }
                        else
                        {
                            DebugMessage("AssetProcessorConnection::DisconnectThread - Start Connect Thread");
                            StartThread(m_connectThread);
                        }
                    }
                }
            }
        }

        bool AssetProcessorConnection::NegotiateWithServer()
        {
            AZ_Assert(!m_unitTesting, "Negotiation is not valid during unit testing");
            NegotiationMessage engineInfo;

            AZ::u32 pID = CurrentProcessID();
            char processID[20];
            azsnprintf(processID, AZ_ARRAY_SIZE(processID), "%d", pID);

            engineInfo.m_identifier = m_identifier;
            engineInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_ProcessId, AZ::OSString(processID)));
            engineInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_Platform, AZ::OSString(m_assetPlatform.c_str())));
            engineInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_BranchIndentifier, AZ::OSString(m_branchToken.c_str())));
            engineInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_ProjectName, AZ::OSString(m_projectName.c_str())));

            if (m_connectThread.m_join)
            {
                return false;
            }

            NegotiationMessage apInfo;
            if (!AssetSystem::SendRequestToConnection(engineInfo, apInfo, this))
            {
                return false;
            }

            if (apInfo.m_apiVersion != engineInfo.m_apiVersion)
            {
                m_negotiationFailed = true;
                DebugMessage("AssetProcessorConnection::NegotateWithServer - negotiation invalid.Version Mismatch");
                AssetSystemConnectionNotificationsBus::Broadcast(&AssetSystemConnectionNotificationsBus::Events::NegotiationFailed);
                return false;
            }

            bool isBranchIdentifierMatch = AzFramework::StringFunc::Equal(apInfo.m_negotiationInfoMap[NegotiationInfo_BranchIndentifier].c_str(), m_branchToken.c_str());
            bool isProjectMatch = AzFramework::StringFunc::Equal(apInfo.m_negotiationInfoMap[NegotiationInfo_ProjectName].c_str(), m_projectName.c_str());
            bool isIdentifierMatch = apInfo.m_identifier == "ASSETPROCESSOR" ? true : false;

            if ((isIdentifierMatch && isBranchIdentifierMatch && isProjectMatch) || (m_unitTesting && apInfo.m_identifier == "GAME"))
            {
                DebugMessage("AssetProcessorConnection::NegotiateWithServer - negotiation with %s succeeded.", apInfo.m_identifier.c_str());
            }
            else
            {
                m_negotiationFailed = true;
                DebugMessage("AssetProcessorConnection::NegotiateWithServer - negotiation invalid");
                AssetSystemConnectionNotificationsBus::Broadcast(&AssetSystemConnectionNotificationsBus::Events::NegotiationFailed);
                return false;
            }

            m_negotiationFailed = false;

            return true;
        }

        bool AssetProcessorConnection::NegotiateWithClient()
        {
            AZ_Assert(!m_unitTesting, "Negotiation is not valid during unit testing");
            NegotiationMessage engineInfo;
            NegotiationMessage apInfo;

            AZ::u32 pID = CurrentProcessID();
            char processID[20];
            azsnprintf(processID, AZ_ARRAY_SIZE(processID), "%d", pID);

            engineInfo.m_identifier = m_identifier;
            engineInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_ProcessId, AZ::OSString(processID)));
            engineInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_Platform, AZ::OSString(m_assetPlatform.c_str())));
            engineInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_BranchIndentifier, AZ::OSString(m_branchToken.c_str())));
            engineInfo.m_negotiationInfoMap.insert(AZStd::make_pair(NegotiationInfo_ProjectName, AZ::OSString(m_projectName.c_str())));

            DebugMessage("AssetProcessorConnection::NegotiateWithClient - Waiting for negotiation.");

            AZStd::binary_semaphore wait;
            // Add callback for negotiation type before starting recv thread
            MessageBuffer apBuffer;
            bool received = false;
            AddResponseHandler(apInfo.GetMessageType(), NEGOTIATION_SERIAL,
                [this, &wait, &received, &apBuffer](AZ::u32 typeId, AZ::u32 /*serial*/, const void* buffer, AZ::u32 bufferSize)
                {
                    (void)typeId;
                    DebugMessage("AssetProcessorConnection::NegotiateWithClient - negotiation callback fired");
                    if (bufferSize)
                    {
                        apBuffer.resize(bufferSize);
                        memcpy(apBuffer.data(), buffer, bufferSize);
                        received = true;
                    }
                    wait.release();
                });

            StartSendRecvThreads();

            wait.acquire(); // wait for negotiation from client

            if (!received)
            {
                // we are terminating or retrying.
                DebugMessage("AssetProcessorConnection", false, "NegotiationWithClient: negotiation aborted");
                return false;
            }

            DebugMessage("AssetProcessorConnection::NegotiateWithClient - negotiation receieved.");

            if (!UnpackMessage(apBuffer, apInfo))
            {
                AZ_Error("AssetProcessorConnection", false, "NegotiationWithClient: Unable to unpack negotiation message");
                return false;
            }

            if (apInfo.m_apiVersion != engineInfo.m_apiVersion)
            {
                m_negotiationFailed = true;
                DebugMessage("AssetProcessorConnection::NegotiateWithServer - negotiation invalid.Version Mismatch");
                AssetSystemConnectionNotificationsBus::Broadcast(&AssetSystemConnectionNotificationsBus::Events::NegotiationFailed);
                return false;
            }

            bool isBranchIdentifierMatch = AzFramework::StringFunc::Equal(apInfo.m_negotiationInfoMap[NegotiationInfo_BranchIndentifier].c_str(), m_branchToken.c_str());
            bool isProjectMatch = AzFramework::StringFunc::Equal(apInfo.m_negotiationInfoMap[NegotiationInfo_ProjectName].c_str(), m_projectName.c_str());

            DebugMessage("AssetProcessorConnection::NegotiateWithClient - Branch token (us) %s vs Branch token (server) %s", m_branchToken.c_str(), apInfo.m_negotiationInfoMap[NegotiationInfo_BranchIndentifier].c_str());
            DebugMessage("AssetProcessorConnection::NegotiateWithClient - Project Name (us) %s vs Project Name (server) %s", m_projectName.c_str(), apInfo.m_negotiationInfoMap[NegotiationInfo_ProjectName].c_str());

            if (!isBranchIdentifierMatch || !isProjectMatch)
            {
                AssetSystem::SendRequestToConnection(engineInfo, this); // we are sending the request so that assetprocessor can show the dialog for branch mismatch
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));    //Sleeping to make sure that the message get delivered to AP before socket get disconnected
                m_negotiationFailed = true;
                DebugMessage("AssetProcessorConnection::NegotiationWithClient - negotiation invalid");
                AssetSystemConnectionNotificationsBus::Broadcast(&AssetSystemConnectionNotificationsBus::Events::NegotiationFailed);
                return false;
            }

            bool isIdentifierMatch = apInfo.m_identifier == "ASSETPROCESSOR" ? true : false;

            if (isIdentifierMatch || (m_unitTesting && apInfo.m_identifier == "GAME"))
            {
                DebugMessage("AssetProcessorConnection::NegotiationWithClient - negotiation successful with %s", apInfo.m_identifier.c_str());
                AssetSystem::SendRequestToConnection(engineInfo, this);
            }
            else
            {
                m_negotiationFailed = true;
                DebugMessage("AssetProcessorConnection::NegotiationWithClient - negotiation invalid");
                AssetSystemConnectionNotificationsBus::Broadcast(&AssetSystemConnectionNotificationsBus::Events::NegotiationFailed);
                return false;
            }

            m_negotiationFailed = false;

            return true;
        }

        void AssetProcessorConnection::JoinSendRecvThreads()
        {
            JoinThread(m_sendThread, &m_sendEventNotEmpty);
            JoinThread(m_recvThread);
        }

        void AssetProcessorConnection::StartSendRecvThreads()
        {
            // must start recv first
            StartThread(m_recvThread);
            StartThread(m_sendThread);
        }

        void AssetProcessorConnection::StartThread(ThreadState& thread)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> locker(thread.m_mutex);

            DebugMessage("StartThread: Starting %s", thread.m_desc.m_name);
            thread.m_join = false;
            thread.m_thread = AZStd::thread(thread.m_desc, thread.m_main);
        }

        void AssetProcessorConnection::JoinThread(ThreadState& thread, AZStd::condition_variable* wakeUpCondition /* = nullptr */)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> locker(thread.m_mutex);
            // If we are in one of the worker threads, we can safely assume that the thread will exit
            // when control returns from the error handler.
            if (thread.m_thread.joinable())
            {
                // Signal the thread that it's join time
                thread.m_join = true;
                if (wakeUpCondition)
                {
                    wakeUpCondition->notify_all(); // this will wake up the thread and it should exit immediately
                    thread.m_thread.join();
                }
                else
                {
                    DebugMessage("JoinThread: Waiting for %s(%u) to join", thread.m_desc.m_name, thread.m_thread.get_id());
                    thread.m_thread.join();
                }

                DebugMessage("JoinThread: %s joined", thread.m_desc.m_name);
            }
        }

        void AssetProcessorConnection::StartDisconnecting()
        {
            // this function is always safe to call from any of the service threads.  Other threads besides the service threads
            // should just call Disconnect(...) instead.
            bool priorState = false;

            // only one thread may do this, the others must leave:
            m_isBusyDisconnecting.compare_exchange_strong(priorState, true);
            if (priorState)
            {
                // if we get here it means that either some other thread is already trying to start the disconnection sequence
                // or the disconnection sequence is already running and has not yet started any other threads again during retry
                // so its safe to just return - things are already being taken care of.
                DebugMessage("StartDisconnecting - someone else is already starting the disconnection sequence.");
                return;
            }

            DebugMessage("StartDisconnecting - this thread is now in control of the disconnection sequence.");
            // otherwise, if we get here, it means that nobody else is starting the disconnect sequence, or the disconnect sequence
            // is at a point where it has already crossed the line into starting new threads as part of retry.  In that case, we must
            // wait for it to shut down, and then start it again to clean up.
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_disconnectThread.m_mutex);
            JoinThread(m_disconnectThread);
            StartThread(m_disconnectThread);
        }

        void AssetProcessorConnection::Configure(const char* branchToken, const char* platform, const char* identifier, const char* projectName)
        {
            m_branchToken = branchToken;
            m_assetPlatform = platform;
            m_identifier = identifier;
            m_projectName = projectName;

            AZ_Error("AssetProcessorConnection", projectName && strlen(projectName) > 0, "projectName should not be empty");
        }

        bool AssetProcessorConnection::Listen(AZ::u16 port)
        {
            //set input params
            m_connectAddr.clear(); //listen only uses a port so clear the addr
            m_port = port;

            //set listening to true, as we are listening for a connection
            m_listening = true;
            m_negotiationFailed = false;

            //set retry after disconnect to true and kick off the disconnect thread
            //the idea here is to disconnect from any connection we may have
            //stop all the associated threads for that connection and try
            //to connect after that is done
            m_retryAfterDisconnect = true;
            StartDisconnecting();

            return true;
        }

        //the listen thread only does one thing, infinitely listens for a connection
        //if any socket error occurs we just start the disconnect thread
        void AssetProcessorConnection::ListenThread()
        {
            if (m_listenThread.m_join)
            {
                DebugMessage("AssetProcessorConnection::ListenThread - Join");
                return;
            }

            //make sure we have a valid socket
            AZ_Assert(m_connectionState != EConnectionState::Connecting, "");
            if (!GetValidAzSocket(m_socket))
            {
                DebugMessage("AssetProcessorConnection::ListenThread - No valid socket available to connect");
                StartDisconnecting();
                return;
            }

            //set the port and socket options
            AZ::AzSock::AzSocketAddress socketAddress;
            socketAddress.SetAddrPort(m_port);
            AZ::AzSock::SetSocketOption(m_socket, AZ::AzSock::AzSocketOption::REUSEADDR, true);
            AZ::AzSock::SetSocketOption(m_socket, AZ::AzSock::AzSocketOption::LINGER, false);
            DebugMessage("AzSockConnection::ListenThread: binding to port %d", m_port);
            int result = AZ::AzSock::Bind(m_socket, socketAddress);
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                // Error binding socket to listen
                DebugMessage("AssetProcessorConnection::ListenThread - bind returned an error %s", AZ::AzSock::GetStringForError(result));
                StartDisconnecting();
                return;
            }

            m_connectionState = EConnectionState::Listening;
            EngineConnectionEvents::Bus::Broadcast(&EngineConnectionEvents::Bus::Events::Listening, this);

            // Listen for one connection
            result = AZ::AzSock::Listen(m_socket, 1);
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                DebugMessage("AssetProcessorConnection::ListenThread - listen returned an error %s", AZ::AzSock::GetStringForError(result));
                StartDisconnecting();
                return;
            }

            DebugMessage("AssetProcessorConnection::ListenThread - socket %d listening on port %u", static_cast<int>(m_socket), m_port);

            // Block accepting incoming connection
            AZ::AzSock::AzSocketAddress incAddress;
            AZSOCKET connectionSocket = AZ::AzSock::Accept(m_socket, incAddress);
            if (!AZ::AzSock::IsAzSocketValid(connectionSocket))
            {
                DebugMessage("AssetProcessorConnection::ListenThread - accept returned an error %s", AZ::AzSock::GetStringForError(connectionSocket));
                if (m_listenThread.m_join)
                {
                    return;
                }
                StartDisconnecting();
                return;
            }

            if (m_listenThread.m_join)
            {
                DebugMessage("AssetProcessorConnection::ListenThread - Join");
                return;
            }

            result = AZ::AzSock::EnableTCPNoDelay(connectionSocket, true);
            if (result)
            {
                DebugMessage("AssetProcessorConnection::ConnectThread - EnableTCPNoDelay returned an error %s", AZ::AzSock::GetStringForError(result));
                if (m_listenThread.m_join)
                {
                    return;
                }
                StartDisconnecting();
                return;
            }

            DebugMessage("AssetProcessorConnection::ListenThread - accepted socket is %i", static_cast<int>(connectionSocket));
            DebugMessage("AssetProcessorConnection::ListenThread - closing listen socket %i", static_cast<int>(m_socket));

            result = AZ::AzSock::CloseSocket(m_socket);
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                // Error closing listening socket, probably not fatal as we no longer use it
                DebugMessage("AssetProcessorConnection::ListenThread - closesocket returned an error %s", AZ::AzSock::GetStringForError(result));
            }

            DebugMessage("AssetProcessorConnection::ListenThread - changing m_socket from %i to %i...", static_cast<int>(m_socket), static_cast<int>(connectionSocket));
            m_socket = connectionSocket;
            m_connectionState = SocketConnection::EConnectionState::Connecting;

            //negotiate, short circuit if unit testing
            if (m_unitTesting || NegotiateWithClient()) //negotiate with client will start send recv threads
            {
                 if (m_unitTesting) // we skip negotiation in unit tests because it's asymmetrical, but we need the threads to start
                {
                    StartSendRecvThreads();
                }

                m_connectionState = EConnectionState::Connected;
                EngineConnectionEvents::Bus::Broadcast(&EngineConnectionEvents::Bus::Events::Connected, this);
                return;
            }

            if (m_listenThread.m_join)
            {
                DebugMessage("AssetProcessorConnection::ListenThread - Join");
                return;
            }

            // Error negotiating connection
            DebugMessage("AssetProcessorConnection::ListenThread - Negotiation failed. Disconnecting");
            StartDisconnecting();
        }

        void AssetProcessorConnection::SendThread()
        {
            auto wakeUpConditionFunc = [&]() -> bool
            {
                return m_sendThread.m_join || !m_sendQueue.empty();
            };

            while (!m_sendThread.m_join)
            {
                decltype(m_sendQueue) localSendQueue;
                // Block until message in queue or awakened by joining
                {
                    AZStd::unique_lock<AZStd::mutex> lock(m_sendQueueMutex);
                    m_sendEventNotEmpty.wait(lock, wakeUpConditionFunc);

                    localSendQueue.swap(m_sendQueue);
                }

                if (m_sendThread.m_join)
                {
                    continue;
                }

                for (const MessageBuffer& outgoingBuffer : localSendQueue)
                {
                    DebugMessage("AssetProcessorConnection::SendThread: Sending %p", outgoingBuffer.data());

                    // Send message
                    AZ::u32 bytesSent = 0;
                    const AZ::u32 bytesToSend = aznumeric_caster(outgoingBuffer.size());
                    do
                    {
                        int sendResult = AZ::AzSock::Send(m_socket, outgoingBuffer.data() + bytesSent, bytesToSend - bytesSent, 0);
                        if (AZ::AzSock::SocketErrorOccured(sendResult))
                        {
                            // Error sending, abort
                            DebugMessage("AssetProcessorConnection::SendThread - AZ::AzSock::send returned an error %d, skipping this message", sendResult);
                            StartDisconnecting();
                            return;
                        }
                        bytesSent += sendResult;
                    } while (bytesSent < bytesToSend);
                }
            }
        }

        void AssetProcessorConnection::RecvThread()
        {
            const int recvBufferSize = 1024;
            char recvBuffer[recvBufferSize];
            AZ::OSString messageBuffer;
            AZ::u32 currentPayloadSize = 0;
            AZ::u32 currentMessageType = 0;
            AZ::u32 currentMessageSerial = 0;
            bool receivedMessageHeader = false;
            while (!m_recvThread.m_join)
            {
                // Blocking call on recv, will only return if data is read or socket disconnected
                int recvResult = AZ::AzSock::Recv(m_socket, recvBuffer, recvBufferSize, 0);

                //check for join before op
                if (m_recvThread.m_join)
                {
                    DebugMessage("AzSockConnection::RecvThread - join requested, closing");
                    return;
                }

                //check disconnect or error
                if (recvResult == 0 || AZ::AzSock::SocketErrorOccured(recvResult))//0 means disconnected
                {
                    DebugMessage("AzSockConnection::RecvThread - Socket closed by remote");
                    // we've been asked to join, do not start the disconnect thread.
                    if (m_recvThread.m_join)
                    {
                        return;
                    }
                    StartDisconnecting();
                    return;
                }

                //add it to the message buffer
                messageBuffer.append(recvBuffer, recvResult);

                // Loop over the recv'd data and parse out as many messages as we can
                for (;; )
                {
                    const size_t sizeOfHeader = sizeof(AZ::u32) * 3;
                    // Did we get a message header yet
                    if (!receivedMessageHeader && messageBuffer.size() >= sizeOfHeader)
                    {
                        // Pull out the first sizeof(AZ::u32) bytes to get the message type
                        currentMessageType = *reinterpret_cast<const AZ::u32*>(messageBuffer.data());
                        messageBuffer.erase(messageBuffer.begin(), messageBuffer.begin() + sizeof(AZ::u32));

                        // Pull out the next sizeof(AZ::u32) bytes to get the payload size
                        currentPayloadSize = *reinterpret_cast<const AZ::u32*>(messageBuffer.data());
                        messageBuffer.erase(messageBuffer.begin(), messageBuffer.begin() + sizeof(AZ::u32));

                        // Pull out the next sizeof(AZ::u32) bytes to get the serial
                        currentMessageSerial = *reinterpret_cast<const AZ::u32*>(messageBuffer.data());
                        messageBuffer.erase(messageBuffer.begin(), messageBuffer.begin() + sizeof(AZ::u32));

                        receivedMessageHeader = true;
                    }

                    // We know the message size, check to see if we have it all
                    if (receivedMessageHeader && messageBuffer.size() >= currentPayloadSize)
                    {
                        // Got a complete payload
                        DebugMessage("AzSockConnection::Recv: Recv'ed type=0x%08x, serial=%u, size=%u", currentMessageType, currentMessageSerial, currentPayloadSize);

                        // This is for backward compatibility, since we should always negotiate and invoke the response handler
                        if (currentMessageType == NegotiationMessage::MessageType)
                        {
                            InvokeResponseHandler(currentMessageType, currentMessageSerial, messageBuffer.data(), currentPayloadSize);
                        }
                        else if (currentMessageSerial & AzFramework::AssetSystem::RESPONSE_SERIAL_FLAG)
                        {
                            currentMessageSerial &= ~AzFramework::AssetSystem::RESPONSE_SERIAL_FLAG; // Clear the bit
                            InvokeResponseHandler(currentMessageType, currentMessageSerial, messageBuffer.data(), currentPayloadSize);
                        }
                        else
                        {
                            // Call message callback - This will provide a valid buffer pointer but no payload size
                            InvokeMessageHandler(currentMessageType, currentMessageSerial, messageBuffer.data(), currentPayloadSize);
                        }

                        // Remove data, only if we had a payload
                        if (currentPayloadSize > 0)
                        {
                            messageBuffer.erase(messageBuffer.begin(), messageBuffer.begin() + currentPayloadSize);
                        }

                        // Reset for next message
                        currentPayloadSize = 0;
                        currentMessageType = 0;
                        currentMessageSerial = 0;
                        receivedMessageHeader = false;
                    }
                    else
                    {
                        // If no current payload size or not enough in the buffer to match payload, then break out and recv again
                        break;
                    }
                }
            }
        }

        AssetProcessorConnection::EConnectionState AssetProcessorConnection::GetConnectionState() const
        {
            return m_connectionState;
        }

        bool AssetProcessorConnection::IsConnected() const
        {
            return m_connectionState == SocketConnection::EConnectionState::Connected;
        }

        bool AssetProcessorConnection::IsDisconnected() const
        {
            return m_connectionState == SocketConnection::EConnectionState::Disconnected;
        }

        bool AssetProcessorConnection::SendMsg(AZ::u32 typeId, const void* dataBuffer, AZ::u32 dataLength)
        {
            return SendMsg(typeId, AzFramework::AssetSystem::DEFAULT_SERIAL, dataBuffer, dataLength);
        }

        bool AssetProcessorConnection::SendMsg(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength)
        {
            QueueMessageForSend(typeId, serial, dataBuffer, dataLength);

            return true;
        }

        bool AssetProcessorConnection::SendRequest(AZ::u32 typeId, const void* dataBuffer, AZ::u32 dataLength, TMessageCallback handler)
        {
            AZ::u32 requestSerial;
            if (typeId == NegotiationMessage::MessageType)
            {
                requestSerial = AssetSystem::NEGOTIATION_SERIAL;
            }
            else
            {
                requestSerial = GetNextSerial();
            }
            return SendRequest(typeId, requestSerial, dataBuffer, dataLength, handler);
        }

        bool AssetProcessorConnection::SendRequest(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength, TMessageCallback handler)
        {
            // negotiation packets are always allowed
            if (m_connectionState != EConnectionState::Connected)
            {
                if ((typeId != NegotiationMessage::MessageType))
                {
                    return false;
                }
            }

            AZStd::binary_semaphore wait;
            bool responseArrived = false;
            AddResponseHandler(typeId, serial, [&wait, &handler, &responseArrived](AZ::u32 response_typeId, AZ::u32 response_serial, const void* response_dataBuffer, AZ::u32 response_dataLength)
                {
                    responseArrived = (response_typeId != 0);

                    if (responseArrived)
                    {
                        handler(response_typeId, response_serial, response_dataBuffer, response_dataLength);
                    }

                    wait.release();
                });

            QueueMessageForSend(typeId, serial, dataBuffer, dataLength);
            wait.acquire();

            return responseArrived;
        }

        void AssetProcessorConnection::QueueMessageForSend(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength)
        {
            MessageBuffer messageBuffer;

            // Build message buffer
            messageBuffer.resize_no_construct(dataLength + sizeof(typeId) + sizeof(serial) + sizeof(dataLength));
            // Send network type, serial, and length
            AZ::u32 offset = 0;
            memcpy(messageBuffer.data() + offset, reinterpret_cast<const void*>(&typeId), sizeof(typeId));
            offset += sizeof(typeId);
            memcpy(messageBuffer.data() + offset, &dataLength, sizeof(dataLength));
            offset += sizeof(dataLength);
            memcpy(messageBuffer.data() + offset, &serial, sizeof(serial));
            offset += sizeof(serial);
            // Use actual data length to copy into the buffer
            if (dataLength > 0)
            {
                memcpy(messageBuffer.data() + offset, dataBuffer, dataLength);
            }

            DebugMessage("AzSockConnection::QueueMessageForSend: queued type=0x%08x, serial=%u, size=%u, buffer=%p", typeId, serial, dataLength, messageBuffer.data());

            // Lock mutex and add to queue
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_sendQueueMutex);
                m_sendQueue.push_back(AZStd::move(messageBuffer));
            }

            // Release thread to do work
            m_sendEventNotEmpty.notify_one();
        }

        SocketConnection::TMessageCallbackHandle AssetProcessorConnection::AddMessageHandler(AZ::u32 typeId, SocketConnection::TMessageCallback callback)
        {
            if (!callback)
            {
                DebugMessage("AzSockConnection::AddMessageHandler - Tried to add a type callback that was null");
                return SocketConnection::s_invalidCallbackHandle;
            }
            SocketConnection::TMessageCallbackHandle callbackHandle = GetNewCallbackHandle();
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_responseHandlerMutex);

                // If we should queue up the add, put it in another list to be taken care of when done invoking type callbacks
                if (m_shouldQueueHandlerChanges)
                {
                    m_messageHandlersToAdd.push_back(AZStd::make_pair(typeId, AZStd::make_pair(callbackHandle, callback)));
                }
                else
                {
                    m_messageHandlers.insert(AZStd::make_pair(typeId, AZStd::make_pair(callbackHandle, callback)));
                }
            }
            return callbackHandle;
        }

        void AssetProcessorConnection::RemoveMessageHandler(AZ::u32 typeId, SocketConnection::TMessageCallbackHandle callbackHandle)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_responseHandlerMutex);
            // If we should queue up the remove, put it in another list to be taken care of when done invoking type callbacks
            if (m_shouldQueueHandlerChanges)
            {
                m_messageHandlersToRemove.push_back(callbackHandle);
                return;
            }
            else
            {
                // Get the range of callbacks that match this typeId
                auto handlerRange = m_messageHandlers.equal_range(typeId);
                for (auto callbackIterator = handlerRange.first; callbackIterator != handlerRange.second; ++callbackIterator)
                {
                    if (callbackIterator->second.first == callbackHandle)
                    {
                        m_messageHandlers.erase(callbackIterator);
                        return;
                    }
                }
            }
            DebugMessage("AzSockConnection::RemoveMessageHandler - Tried to remove a type callback that didn't exist");
        }

        AZ::s32 AssetProcessorConnection::GetLastResult() const
        {
            return m_lastErrorResult;
        }

        AZStd::string AssetProcessorConnection::GetLastErrorMessage() const
        {
            return m_lastErrorMessage;
        }

        void AssetProcessorConnection::InvokeMessageHandler(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_responseHandlerMutex);

            m_shouldQueueHandlerChanges = true;

            // Get the range of callbacks that match this typeId
            auto handlerRange = m_messageHandlers.equal_range(typeId);
            for (auto callbackIterator = handlerRange.first; callbackIterator != handlerRange.second; ++callbackIterator)
            {
                callbackIterator->second.second(typeId, serial, dataBuffer, dataLength);
            }

            m_shouldQueueHandlerChanges = false;

            // Remove callbacks that may have been removed
            for (auto removedCallbackHandleIterator = m_messageHandlersToRemove.begin(); removedCallbackHandleIterator != m_messageHandlersToRemove.end(); /*No increment, erase will do it for us*/)
            {
                // Get the range of callbacks that match this typeId
                auto callbacksRange = m_messageHandlers.equal_range(typeId);
                for (auto callbackIterator = callbacksRange.first; callbackIterator != callbacksRange.second; ++callbackIterator)
                {
                    if (callbackIterator->second.first == *removedCallbackHandleIterator)
                    {
                        m_messageHandlers.erase(callbackIterator);
                        break;
                    }
                }
                removedCallbackHandleIterator = m_messageHandlersToRemove.erase(removedCallbackHandleIterator);
            }

            // Add new callbacks
            for (auto addedCallbackIterator = m_messageHandlersToAdd.begin(); addedCallbackIterator != m_messageHandlersToAdd.end(); /*No increment, erase will do it for us*/)
            {
                m_messageHandlers.insert(*addedCallbackIterator);
                addedCallbackIterator = m_messageHandlersToAdd.erase(addedCallbackIterator);
            }
        }

        void AssetProcessorConnection::AddResponseHandler(AZ::u32 typeId, AZ::u32 serial, SocketConnection::TMessageCallback callback)
        {
            if (!callback)
            {
                DebugMessage("AzSockConnection::AddResponseHandler - Tried to add a type callback that was null");
                return;
            }

            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_responseHandlerMutex);
            // If we should queue up the add, put it in another list to be taken care of when done invoking type callbacks
            if (m_shouldQueueHandlerChanges)
            {
                m_responseHandlersToAdd.push_back(AZStd::make_pair(AZStd::make_pair(typeId, serial), callback));
            }
            else
            {
                m_responseHandlers.insert(AZStd::make_pair(AZStd::make_pair(typeId, serial), callback));
            }
        }

        void AssetProcessorConnection::RemoveResponseHandler(AZ::u32 typeId, AZ::u32 serial)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_responseHandlerMutex);
            // If we should queue up the remove, put it in another list to be taken care of when done invoking type callbacks
            if (m_shouldQueueHandlerChanges)
            {
                m_responseHandlersToRemove.push_back(AZStd::make_pair(typeId, serial));
                return;
            }
            else
            {
                // find the matching key pair (typeId, serial)
                auto keyPair = AZStd::make_pair(typeId, serial);
                auto callbackIt = m_responseHandlers.find(keyPair);
                if (callbackIt == m_responseHandlers.end())
                {
                    DebugMessage("AzSockConnection::RemoveResponseHandler - Tried to remove a handler that didn't exist");
                    return;
                }
                m_responseHandlers.erase(callbackIt);
            }
        }

        void AssetProcessorConnection::InvokeResponseHandler(AZ::u32 typeId, AZ::u32 serial, const void* dataBuffer, AZ::u32 dataLength)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_responseHandlerMutex);

            m_shouldQueueHandlerChanges = true;

            // Find the matching handler
            auto responseHandler = AZStd::make_pair(typeId, serial);
            auto callbackIt = m_responseHandlers.find(responseHandler);
            if (callbackIt != m_responseHandlers.end())
            {
                DebugMessage("AzSockConnection::InvokeResponseHandler - invoking handler for (type 0x%08x, %u)", typeId, serial);
                callbackIt->second(typeId, AzFramework::AssetSystem::DEFAULT_SERIAL, dataBuffer, dataLength);
                // remove callback automatically, as it is no longer needed
                m_responseHandlers.erase(callbackIt);
            }
            else
            {
                DebugMessage("AzSockConnection::InvokeResponseHandler - attempted to invoke a handler for non-existant type/serial pair (0x%08x, %u)", typeId, serial);
            }

            m_shouldQueueHandlerChanges = false;

            for (const auto& removeHandler : m_responseHandlersToRemove)
            {
                RemoveResponseHandler(removeHandler.first, removeHandler.second);
            }
            m_responseHandlersToRemove.clear();

            for (const auto& callbackPair : m_responseHandlersToAdd)
            {
                const auto& keyPair = callbackPair.first;
                AddResponseHandler(keyPair.first, keyPair.second, callbackPair.second);
            }
            m_responseHandlersToAdd.clear();
        }

        void AssetProcessorConnection::FlushResponseHandlers()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_responseHandlerMutex);
            for (auto callback : m_responseHandlers)
            {
                // send a 0, nullptr to each callback, which is the signal for "the request failed"
                callback.second(0, AzFramework::AssetSystem::DEFAULT_SERIAL,  nullptr, 0);
            }
            m_responseHandlers.clear();
        }

        SocketConnection::TMessageCallbackHandle AssetProcessorConnection::GetNewCallbackHandle()
        {
            return m_nextHandlerId++;
        }
    } // namespace AssetSystem
} // namespace AzFramework
