/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameGraph.h>
#include <AzCore/Debug/EventTrace.h>
#include <RHI/Device.h>
#include <RHI/FrameGraphCompiler.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace Metal
    {

        RHI::Ptr<FrameGraphCompiler> FrameGraphCompiler::Create()
        {
            return aznew FrameGraphCompiler();
        }
        
        RHI::ResultCode FrameGraphCompiler::InitInternal(RHI::Device&)
        {
            return RHI::ResultCode::Success;
        }
        
        void FrameGraphCompiler::ShutdownInternal()
        {
        }
        
        RHI::MessageOutcome FrameGraphCompiler::CompileInternal(const RHI::FrameGraphCompileRequest& request)
        {
            AZ_PROFILE_SCOPE(RHI, "FrameGraphCompiler: CompileInternal(Metal)");
            RHI::FrameGraph& frameGraph = *request.m_frameGraph;
            if (!RHI::CheckBitsAny(request.m_compileFlags, RHI::FrameSchedulerCompileFlags::DisableAsyncQueues))
            {
                CompileAsyncQueueFences(frameGraph);
            }
            
            return AZ::Success();
        }
    
        void FrameGraphCompiler::CompileAsyncQueueFences(const RHI::FrameGraph& frameGraph)
        {
            Device& device = static_cast<Device&>(GetDevice());

            AZ_TRACE_METHOD();
            CommandQueueContext& context = device.GetCommandQueueContext();

            for (RHI::Scope* scopeBase : frameGraph.GetScopes())
            {
                Scope* scope = static_cast<Scope*>(scopeBase);

                bool hasCrossQueueConsumer = false;
                for (uint32_t hardwareQueueClassIdx = 0; hardwareQueueClassIdx < RHI::HardwareQueueClassCount; ++hardwareQueueClassIdx)
                {
                    const RHI::HardwareQueueClass hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(hardwareQueueClassIdx);
                    
                    //Metal should automatically synchronize tracked resources on the same queue.
                    if (scope->GetHardwareQueueClass() != hardwareQueueClass)
                    {
                        if (const Scope* producer = static_cast<const Scope*>(scope->GetProducerByQueue(hardwareQueueClass)))
                        {
                            scope->SetWaitFenceValueByQueue(hardwareQueueClass, producer->GetSignalFenceValue());
                        }

                        hasCrossQueueConsumer |= (scope->GetConsumerByQueue(hardwareQueueClass) != nullptr);
                    }
                }

                if (hasCrossQueueConsumer)
                {
                    scope->SetSignalFenceValue(context.IncrementHWQueueFence(scope->GetHardwareQueueClass()));
                }
            }
        }
    }
}
