/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Device.h>
#include <AzCore/Debug/EventTraceDrillerBus.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace DX12
    {
        namespace EventTrace
        {
            const AZStd::thread_id GpuQueueIds[] =
            {
                /// Graphics
                AZStd::thread_id{(size_t)2},

                /// Compute
                AZStd::thread_id{(size_t)3}
            };

            const char* GpuQueueNames[] =
            {
                "Graphics Queue",
                "Compute Queue",
                "Copy Queue"
            };
        }

        void CommandQueueContext::Init(RHI::Device& deviceBase)
        {
            Device& device = static_cast<Device&>(deviceBase);
            m_currentFrameIndex = 0;
            m_frameFences.resize(RHI::Limits::Device::FrameCountMax);
            for (FenceSet& fences : m_frameFences)
            {
                fences.Init(device.GetDevice(), RHI::FenceState::Signaled);
            }

            m_compiledFences.Init(device.GetDevice(), RHI::FenceState::Reset);

            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_commandQueues[hardwareQueueIdx] = CommandQueue::Create();

                CommandQueueDescriptor commandQueueDesc;
                commandQueueDesc.m_hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(hardwareQueueIdx);
                commandQueueDesc.m_hardwareQueueSubclass = HardwareQueueSubclass::Primary;
                m_commandQueues[hardwareQueueIdx]->SetName(Name{ EventTrace::GpuQueueNames[hardwareQueueIdx] });
                m_commandQueues[hardwareQueueIdx]->Init(device, commandQueueDesc);
            }

            Debug::EventTraceDrillerSetupBus::Broadcast(
                &Debug::EventTraceDrillerSetupBus::Events::SetThreadName,
                EventTrace::GpuQueueIds[static_cast<size_t>(RHI::HardwareQueueClass::Graphics)],
                EventTrace::GpuQueueNames[static_cast<size_t>(RHI::HardwareQueueClass::Graphics)]);

            Debug::EventTraceDrillerSetupBus::Broadcast(
                &Debug::EventTraceDrillerSetupBus::Events::SetThreadName,
                EventTrace::GpuQueueIds[static_cast<size_t>(RHI::HardwareQueueClass::Compute)],
                EventTrace::GpuQueueNames[static_cast<size_t>(RHI::HardwareQueueClass::Compute)]);

            CalibrateClocks();
        }

        void CommandQueueContext::Shutdown()
        {
            WaitForIdle();

            m_compiledFences.Shutdown();

            for (FenceSet& fenceSet : m_frameFences)
            {
                fenceSet.Shutdown();
            }
            m_frameFences.clear();

            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_commandQueues[hardwareQueueIdx] = nullptr;
            }
        }

        void CommandQueueContext::QueueGpuSignals(FenceSet& fenceSet)
        {
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                const RHI::HardwareQueueClass hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(hardwareQueueIdx);
                Fence& fence = fenceSet.GetFence(hardwareQueueClass);
                m_commandQueues[hardwareQueueIdx]->QueueGpuSignal(fence);
            }
        }

        void CommandQueueContext::WaitForIdle()
        {
            AZ_PROFILE_SCOPE(RHI, "CommandQueueContext: WaitForIdle");
            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                if (m_commandQueues[hardwareQueueIdx])
                {
                    m_commandQueues[hardwareQueueIdx]->WaitForIdle();
                }
            }
        }

        void CommandQueueContext::Begin()
        {
            AZ_PROFILE_SCOPE(RHI, "CommandQueueContext: Begin");

            {
                AZ_PROFILE_SCOPE(RHI, "Clearing Command Queue Timers");
                for (const RHI::Ptr<CommandQueue>& commandQueue : m_commandQueues)
                {
                    commandQueue->ClearTimers();
                }
            }
        }

        uint64_t CommandQueueContext::IncrementFence(RHI::HardwareQueueClass hardwareQueueClass)
        {
            return m_compiledFences.GetFence(hardwareQueueClass).Increment();
        }

        void CommandQueueContext::End()
        {
            AZ_PROFILE_SCOPE(RHI, "CommandQueueContext: End");

            QueueGpuSignals(m_frameFences[m_currentFrameIndex]);

            for (uint32_t hardwareQueueIdx = 0; hardwareQueueIdx < RHI::HardwareQueueClassCount; ++hardwareQueueIdx)
            {
                m_commandQueues[hardwareQueueIdx]->FlushCommands();
            }

            // Advance to the next frame and wait for its resources to be available before continuing.
            m_currentFrameIndex = (m_currentFrameIndex + 1) % aznumeric_cast<uint32_t>(m_frameFences.size());

            {
                AZ_PROFILE_SCOPE(RHI, "Wait and Reset Fence");

                FenceEvent event("FrameFence");
                m_frameFences[m_currentFrameIndex].Wait(event);
                m_frameFences[m_currentFrameIndex].Reset();
            }

            CalibrateClocks();
        }

        void CommandQueueContext::CalibrateClocks()
        {
            for (uint32_t i = 0; i < RHI::HardwareQueueClassCount; ++i)
            {
                m_commandQueues[i]->CalibrateClock();
            }
        }

        void CommandQueueContext::ExecuteWork(
            RHI::HardwareQueueClass hardwareQueueClass,
            const ExecuteWorkRequest& request)
        {
            GetCommandQueue(hardwareQueueClass).ExecuteWork(request);

#if defined (AZ_DX12_FORCE_FLUSH_SCOPES)
            WaitForIdle();
#endif
        }

        CommandQueue& CommandQueueContext::GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass)
        {
            return *m_commandQueues[static_cast<uint32_t>(hardwareQueueClass)];
        }

        const CommandQueue& CommandQueueContext::GetCommandQueue(RHI::HardwareQueueClass hardwareQueueClass) const
        {
            return *m_commandQueues[static_cast<uint32_t>(hardwareQueueClass)];
        }

        void CommandQueueContext::UpdateCpuTimingStatistics() const
        {
            if (auto statsProfiler = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get(); statsProfiler)
            {
                auto& rhiMetrics = statsProfiler->GetProfiler(rhiMetricsId);

                AZStd::sys_time_t presentDuration = 0;
                for (const RHI::Ptr<CommandQueue>& commandQueue : m_commandQueues)
                {
                    const AZ::Crc32 commandQueueId(commandQueue->GetName().GetHash());
                    rhiMetrics.PushSample(commandQueueId, static_cast<double>(commandQueue->GetLastExecuteDuration()));
                    presentDuration += commandQueue->GetLastPresentDuration();
                }

                rhiMetrics.PushSample(AZ_CRC_CE("Present"), static_cast<double>(presentDuration));
            }
        }

        const FenceSet& CommandQueueContext::GetCompiledFences()
        {
            return m_compiledFences;
        };
    }
}
