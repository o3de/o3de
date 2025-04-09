/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "IPCComponent.h"
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/EditContext.h>

namespace LegacyFramework
{
    void IPCComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<IPCComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    IPCComponent::IPCComponent()
    {
        m_LastAssignedHandle = 1;
        m_isPrimary = false;
    }

    IPCComponent::~IPCComponent()
    {
    }

    void IPCComponent::Init()
    {
        IPCCommandBus::Handler::BusConnect();
    }

    void IPCComponent::Activate()
    {
        m_isPrimary = LegacyFramework::isPrimary();
        AZStd::string ipcName = AZStd::string::format("%s-IPCComponent", LegacyFramework::appName());

        m_success = true;
        // create  / flush buffer if primary:
        if (!m_IPCBuffer.Create(ipcName.c_str(), 8 * 1024, true))
        {
            AZ_Warning("App", false, "PRIMARY: Could not Open IPC buffer, open files will not work.");
            m_success = false;
        }

        if ((m_success) && (!m_IPCBuffer.Map()))
        {
            AZ_Warning("App", false, "PRIMARY: Could not Map IPC buffer, open files will not work.");
            m_success = false;
            m_IPCBuffer.Close();
        }

        if ((m_isPrimary) && (m_success))
        {
            // start the listen thread:
            m_ProcessThread = AZStd::thread(AZStd::bind(&IPCComponent::ProcessThread, this));
        }
    }

    void IPCComponent::Deactivate()
    {
        m_ShutdownThread = true;
        if (m_success)
        {
            if (m_isPrimary)
            {
                m_ProcessThread.join();
            }
            m_IPCBuffer.UnMap();
            m_IPCBuffer.Close();
        }
    }

    IPCHandleType IPCComponent::RegisterIPCHandler(const char* commandName, IPCCommandHandler handlerFn)
    {
        m_RegisteredIPCHandlers[++m_LastAssignedHandle] = RegisteredIPCHandler(commandName, handlerFn);
        return m_LastAssignedHandle;
    }

    void IPCComponent::UnregisterIPCHandler(IPCHandleType handle)
    {
        m_RegisteredIPCHandlers.erase(handle);
    }

    // call this to send an IPC command to the primary.  you can only do this if you're not a primary
    void IPCComponent::SendIPCCommand(const char* commandName, const char* params)
    {
        AZ_Assert(!m_isPrimary, "Only clients may send commands");
        if (m_isPrimary)
        {
            return;
        }

        if (!m_success)
        {
            AZ_Warning("IPC", m_success, "Unable to send IPC command - we never were able to attach to the Shared Memory");
            return;
        }

        unsigned int commandSize = (unsigned int)strlen(commandName);
        unsigned int dataSize = (unsigned int)strlen(params);

        if (commandSize + dataSize > 4096)
        {
            AZ_Warning("IPC", m_success, "Unable to send IPC command - The data is too big");
            return;
        }

        {
            AZ::SharedMemory::MemoryGuard g(m_IPCBuffer);
            if (!m_IPCBuffer.Write(LegacyFramework::appName(), (unsigned int)strlen(LegacyFramework::appName())))
            {
                return;
            }
            if (!m_IPCBuffer.Write(&commandSize, sizeof(unsigned int)))
            {
                return;
            }
            if (!m_IPCBuffer.Write(&dataSize, sizeof(unsigned int)))
            {
                return;
            }
            if (!m_IPCBuffer.Write(commandName, commandSize))
            {
                return;
            }
            if (!m_IPCBuffer.Write(params, dataSize))
            {
                return;
            }
        }
    }

    void IPCComponent::ProcessThread()
    {
        m_ShutdownThread = false;
        bool flush = true;
        char databuffer[4096] = {0};

        while (!m_ShutdownThread)
        {
            if (flush)
            {
                AZ::SharedMemory::MemoryGuard g(m_IPCBuffer);
                m_IPCBuffer.Clear();
                flush = false;
                continue;
            }


            unsigned int dataSize = 0;
            {
                dataSize = 0;
                AZ::SharedMemory::MemoryGuard g(m_IPCBuffer);
                if (m_IPCBuffer.IsLockAbandoned())
                {
                    flush = true;
                    continue;
                }
                else
                {
                    dataSize = m_IPCBuffer.Read(databuffer, (sizeof(unsigned int) * 2) + (unsigned int)strlen(LegacyFramework::appName()));
                }
            }

            if (dataSize == 0)
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10)); // how do we deal with this?
                continue; // no data
            }

            if (dataSize < (sizeof(unsigned int) * 2) + strlen(LegacyFramework::appName()))
            {
                AZ_Warning("Application", false, "Invalid size of IPC receieved on the IPC SHM socket - flushing.");
                flush = true;
                continue;
            }

            if (strncmp(databuffer,  LegacyFramework::appName(), strlen(LegacyFramework::appName())) != 0)
            {
                AZ_Warning("Application", false, "Invalid header of IPC receieved on the IPC SHM socket.");
                flush = true;
                continue;
            }

            unsigned int* currentReadPos = reinterpret_cast<unsigned int*>(databuffer + strlen(LegacyFramework::appName()));
            unsigned int commandSize = *currentReadPos;
            ++currentReadPos;
            unsigned int contentsSize = *currentReadPos;
            ++currentReadPos;

            if (commandSize + contentsSize > 4096)
            {
                AZ_Warning("Application", false, "Invalid size of read-in IPC receieved on the IPC SHM.");
                flush = true;
                continue;
            }

            // now read that data:
            dataSize = m_IPCBuffer.Read(databuffer, commandSize + contentsSize);
            if (dataSize < commandSize + contentsSize)
            {
                AZ_Warning("Application", false, "Truncated read in IPC SHM.");
                flush = true;
                continue;
            }

            char* currentStringReadPos = reinterpret_cast<char*>(databuffer);

            AZStd::string command;
            AZStd::string contents;
            command.assign(currentStringReadPos, currentStringReadPos + commandSize);
            currentStringReadPos += commandSize;
            contents.assign(currentStringReadPos, currentStringReadPos + contentsSize);

            {
                AZStd::lock_guard<AZStd::recursive_mutex> guard(m_CommandListMutex);
                m_CommandsWaitingToExecute.push_back(WaitingIPCCommand(command.c_str(), contents.c_str()));
            }

            AZ::SystemTickBus::QueueFunction(&IPCComponent::ExecuteIPCHandlers, this);
        }
    }

    // call this to execute handlers.  They will be executed in the caller's thread.
    void IPCComponent::ExecuteIPCHandlers()
    {
        CommandContainer batch;
        {
            AZStd::lock_guard<AZStd::recursive_mutex> guard(m_CommandListMutex);
            batch = AZStd::move(m_CommandsWaitingToExecute);
        }

        for (auto command = batch.begin(); command != batch.end(); ++command)
        {
            bool foundHandler = false;
            for (auto handler = m_RegisteredIPCHandlers.begin(); handler != m_RegisteredIPCHandlers.end(); ++handler)
            {
                if (handler->second.m_commandName.compare(command->m_commandName) == 0)
                {
                    handler->second.m_handler(command->m_Parameters);
                    foundHandler = true;
                }
            }

            if (!foundHandler)
            {
                AZ_Warning("Application", false, "No handler for IPC: '%s' '%s", command->m_commandName.c_str(), command->m_Parameters.c_str());
            }
        }
    }
}

