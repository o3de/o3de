/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/CommandQueue.h>
#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI/Device.h>

namespace AZ
{
    namespace RHI
    {

        RHI::HardwareQueueClass CommandQueue::GetHardwareQueueClass() const
        {
            return m_descriptor.m_hardwareQueueClass;
        }

        ResultCode CommandQueue::Init(Device& device, const CommandQueueDescriptor& descriptor)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

#if defined (AZ_RHI_ENABLE_VALIDATION)
            if (IsInitialized())
            {
                AZ_Error("CommandQueue", false, "CommandQueue is already initialized!");
                return ResultCode::InvalidOperation;
            }
#endif
            const ResultCode resultCode = InitInternal(device, descriptor);

            if (resultCode == ResultCode::Success)
            {
                DeviceObject::Init(device);
                
                m_descriptor = descriptor;
                m_isQuitting = false;
                m_isWorkQueueEmpty = true;
                
                AZStd::thread_desc threadDesc{ GetName().GetCStr() };
                m_thread = AZStd::thread([&]() { ProcessQueue(); }, &threadDesc);
            }
            return resultCode;
        }

        void CommandQueue::Shutdown()
        {
            if (IsInitialized())
            {
                m_isQuitting = true;
                m_workQueueCondition.notify_all();
                if (m_thread.joinable())
                {
                   m_thread.join();
                }
                else
                {
                   AZ_Error("CommandQueue", false, "CommandQueue was shut down but thread was not joinable!");
                }
                ShutdownInternal();
                DeviceObject::Shutdown();
            }
        }

        void CommandQueue::QueueCommand(Command command)
        {
            if (m_thread.get_id() == AZStd::this_thread::get_id())
            {
                // No need to queue the command since we are already in queue thread.
                command(GetNativeQueue());
                return;
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_workQueueMutex);
            m_workQueue.emplace(command);
            m_workQueueCondition.notify_all();
            m_isWorkQueueEmpty = false;
        }

        void CommandQueue::FlushCommands()
        {
            AZ_ATOM_PROFILE_FUNCTION("RHI", "CommandQueue: FlushCommands");
            while (!m_isWorkQueueEmpty && !m_isQuitting)
            {
                AZStd::this_thread::yield();
            }
        }
        
        void CommandQueue::ProcessQueue()
        {
            //runs forever in a background thread
            Command command;
            for (;;)
            {
                {
                    AZStd::unique_lock<AZStd::mutex> lock(m_workQueueMutex);
                    
                    if (m_workQueue.empty())
                    {
                        m_isWorkQueueEmpty = true;
                        m_workQueueCondition.wait(lock, [this]() { return !m_workQueue.empty() || m_isQuitting; });
                    }
                    
                    if (m_isQuitting)
                    {
                        return;
                    }
                    
                    command = std::move(m_workQueue.front());
                    m_workQueue.pop();
                }

                //run a command
                {
                    AZ_PROFILE_SCOPE(Debug::ProfileCategory::AzRender, "RHI::CommandQueue - Execute Command");
                    command(GetNativeQueue());
                }
            }
        }
    
        const CommandQueueDescriptor& CommandQueue::GetDescriptor() const
        {
            return m_descriptor;
        }
    }
}
