/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/IPC/SharedMemory.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>

#include "EditorFrameworkAPI.h"

namespace LegacyFramework
{
    class IPCComponent
        : public AZ::Component
        , private IPCCommandBus::Handler
    {
    public:
        AZ_COMPONENT(LegacyFramework::IPCComponent, "{798D5FE3-034B-406A-9830-A6ED2AF05E26}")

        IPCComponent();
        virtual ~IPCComponent();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        virtual void Init();
        virtual void Activate();
        virtual void Deactivate();
        //////////////////////////////////////////////////////////////////////////

    private:

        //////////////////////////////////////////////////////////////////////////
        // IPCCommandBus::Listener
        virtual IPCHandleType RegisterIPCHandler(const char* commandName, IPCCommandHandler handlerFn);
        virtual void UnregisterIPCHandler(IPCHandleType handle);
        // call this to execute handlers.  They will be executed in the caller's thread.
        virtual void ExecuteIPCHandlers();
        // call this to send an IPC command to the primary.  you can only do this if you're not a primary
        virtual void SendIPCCommand(const char* commandName, const char* params);
        //////////////////////////////////////////////////////////////////////////

        class RegisteredIPCHandler
        {
        public:
            AZStd::string m_commandName; // "open"
            IPCCommandHandler m_handler;

            RegisteredIPCHandler(const char* command, IPCCommandHandler handler)
                : m_commandName(command)
                , m_handler(handler)
            {
            }

            RegisteredIPCHandler()
                : m_handler(NULL) {}
        };

        struct WaitingIPCCommand
        {
            AZStd::string m_commandName;
            AZStd::string m_Parameters;

            WaitingIPCCommand(const char* commandName, const char* params)
                : m_commandName(commandName)
                , m_Parameters(params) {}
        };

        void ProcessThread();

        typedef AZStd::vector<WaitingIPCCommand> CommandContainer;
        typedef AZStd::unordered_map<IPCHandleType, RegisteredIPCHandler> IPCHandlerContainer;
        IPCHandlerContainer m_RegisteredIPCHandlers;

        CommandContainer m_CommandsWaitingToExecute;
        AZStd::recursive_mutex m_CommandListMutex;

        AZStd::thread m_ProcessThread;
        AZStd::atomic_bool m_ShutdownThread;

        IPCHandleType m_LastAssignedHandle;
        AZ::SharedMemoryRingBuffer m_IPCBuffer;

        bool m_isPrimary;
        bool m_success; // are we able to send?

        static void Reflect(AZ::ReflectContext* reflection);
    };
}
