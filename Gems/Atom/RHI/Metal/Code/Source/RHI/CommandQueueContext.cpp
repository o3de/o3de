/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/Debug/EventTrace.h>
#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI/CommandQueue.h>
#include <Atom/RHI.Reflect/CpuTimingStatistics.h>
#include <RHI/Device.h>
#include <RHI/CommandQueue.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace Metal
    {
        void CommandQueueContext::Init(RHI::Device& deviceBase)
        {
            static const char* QueueNames[RHI::HardwareQueueClassCount] =
            {
                "Graphics Submit Queue",
                "Compute Submit Queue",
                "Copy Submit Queue"
            };

            auto& device = static_cast<Device&>(deviceBase);
            m_compiledFences.Init(&device, RHI::FenceState::Reset);
            for (uint32_t frameIdx = 0; frameIdx < RHI::Limits::Device::FrameCountMax; ++frameIdx)
            {
                m_frameFences[frameIdx].Init(&device, RHI::FenceState::Signaled);
            }
            
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_commandQueues[hardwareQueueIdx] = CommandQueue::Create();
                RHI::CommandQueueDescriptor commandQueueDescriptor;
                commandQueueDescriptor.m_hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(hardwareQueueIdx);
                m_commandQueues[hardwareQueueIdx]->SetName(Name{ QueueNames[hardwareQueueIdx] });
                m_commandQueues[hardwareQueueIdx]->Init(deviceBase, commandQueueDescriptor);
            }
        }
        
        void CommandQueueContext::Shutdown()
        {
            WaitForIdle();
            m_compiledFences.Shutdown();
            
            for (uint32_t frameIdx = 0; frameIdx < RHI::Limits::Device::FrameCountMax; ++frameIdx)            
            {
                m_frameFences[frameIdx].Shutdown();
            }
            
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_commandQueues[hardwareQueueIdx] = nullptr;
            }
        }

        void CommandQueueContext::WaitForIdle()
        {
            AZ_TRACE_METHOD();
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_commandQueues[hardwareQueueIdx]->WaitForIdle();
            }
        }

        void CommandQueueContext::Begin()
        {
            for (const RHI::Ptr<CommandQueue>& commandQueue : m_commandQueues)
            {
                commandQueue->ClearTimers();
            }
        }

        void CommandQueueContext::End()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzRender);
            
            QueueGpuSignals(m_frameFences[m_currentFrameIndex]);
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_commandQueues[hardwareQueueIdx]->FlushCommands();
            }
            
            // Advance to the next frame and wait for its resources to be available before continuing.
            m_currentFrameIndex = (m_currentFrameIndex + 1) % aznumeric_cast<uint32_t>(m_frameFences.size());

            {
                AZ_PROFILE_SCOPE_IDLE(AZ::Debug::ProfileCategory::AzRender, "Wait and Reset Fence");
                AZ_ATOM_PROFILE_TIME_GROUP_REGION("RHI", "CommandQueueContext: Wait on Fences");

                //Synchronize the CPU with the GPU by waiting on the fence until signalled by the GPU. CPU can only go upto
                //RHI::Limits::Device::FrameCountMax frames ahead of the GPU
                m_frameFences[m_currentFrameIndex].Wait();
                m_frameFences[m_currentFrameIndex].Reset();
            }
        }
    
        uint64_t CommandQueueContext::IncrementHWQueueFence(RHI::HardwareQueueClass hardwareQueueClass)
        {
            return m_compiledFences.GetFence(hardwareQueueClass).Increment();
        }
    
        void CommandQueueContext::ExecuteWork(
            RHI::HardwareQueueClass hardwareQueueClass,
            const ExecuteWorkRequest& request)
        {
            GetCommandQueue(hardwareQueueClass).ExecuteWork(request);
        }

        CommandQueue& CommandQueueContext::GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass)
        {
            return *m_commandQueues[static_cast<uint32_t>(hardwareQueueClass)];
        }

        const CommandQueue& CommandQueueContext::GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass) const
        {
            return *m_commandQueues[static_cast<uint32_t>(hardwareQueueClass)];
        }

        const FenceSet& CommandQueueContext::GetCompiledFences()
        {
            return m_compiledFences;
        };
    
        void CommandQueueContext::QueueGpuSignals(FenceSet& fenceSet)
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                const RHI::HardwareQueueClass hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(hardwareQueueIdx);
                Fence& fence = fenceSet.GetFence(hardwareQueueClass);
                m_commandQueues[hardwareQueueIdx]->QueueGpuSignal(fence);
            }
        }
    
        void CommandQueueContext::UpdateCpuTimingStatistics(RHI::CpuTimingStatistics& cpuTimingStatistics) const
        {
            cpuTimingStatistics.Reset();

            AZStd::sys_time_t presentDuration = 0;
            for (const RHI::Ptr<CommandQueue>& commandQueue : m_commandQueues)
            {
                cpuTimingStatistics.m_queueStatistics.push_back({ commandQueue->GetName(), commandQueue->GetLastExecuteDuration() });
                presentDuration += commandQueue->GetLastPresentDuration();
            }
            cpuTimingStatistics.m_presentDuration = presentDuration;
        }
    }
}
