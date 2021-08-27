/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <AzCore/Debug/EventTrace.h>
#include <RHI/CommandListBase.h>
#include <Source/RHI/Device.h>

namespace AZ
{
    namespace Metal
    {
        CommandListBase::~CommandListBase()
        {
            Shutdown();
        }
        
        void CommandListBase::Shutdown()
        {
            m_commandEncoderType = Metal::CommandEncoderType::Invalid;
            m_isEncoded = false;
        }
        
        void CommandListBase::Init(RHI::HardwareQueueClass hardwareQueueClass, Device* device)
        {
            AZ_UNUSED(device);
            m_hardwareQueueClass = hardwareQueueClass;
            m_supportsInterDrawTimestamps = AZ::RHI::QueryTypeFlags::Timestamp == (device->GetFeatures().m_queryTypesMask[static_cast<uint32_t>(hardwareQueueClass)] & AZ::RHI::QueryTypeFlags::Timestamp);
        }
        
        void CommandListBase::Reset()
        {
            m_renderPassDescriptor = nil;
        }

        void CommandListBase::Open(id <MTLCommandBuffer> mtlCommandBuffer)
        {
            m_isEncoded = false;
            m_mtlCommandBuffer = mtlCommandBuffer;
        }
        
        void CommandListBase::Open(id <MTLCommandEncoder> subEncoder)
        {
            m_isEncoded = true;
            
            //Sub Encoder can only do Graphics work
            m_encoder = subEncoder;
            m_commandEncoderType = CommandEncoderType::Render;
        }
        
        void CommandListBase::Close()
        {
            m_mtlCommandBuffer = nil;
        }

        void CommandListBase::FlushEncoder()
        {
            if (m_encoder)
            {
                [m_encoder endEncoding];
                m_encoder = nil;
                m_isNullDescHeapBound = false;
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
                if (m_supportsInterDrawTimestamps)
                {
                    m_timeStampQueue.clear();
                }
#endif
            }
        }
        
        void CommandListBase::MakeHeapsResident(MTLRenderStages renderStages)
        {
            if(m_isNullDescHeapBound)
            {
                return;
            }
            
            switch(m_commandEncoderType)
            {
                case CommandEncoderType::Render:
                {
                    if(renderStages != 0)
                    {
                        id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                        for (id<MTLHeap> residentHeap : *m_residentHeaps)
                        {
                            [renderEncoder useHeap : residentHeap
                                           stages  : renderStages];
                        }
                    }
                    break;
                }
                case CommandEncoderType::Compute:
                {
                    id<MTLComputeCommandEncoder> computeEncoder = GetEncoder<id<MTLComputeCommandEncoder>>();
                    for (id<MTLHeap> residentHeap : *m_residentHeaps)
                    {
                        [computeEncoder useHeap : residentHeap];
                    }
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Encoder Type not supported");
                }
            }
            m_isNullDescHeapBound = true;
        }

        void CommandListBase::CreateEncoder(CommandEncoderType encoderType)
        {
            //No need to create one if it exists already from previous calls or from ParallelRendercommandEncoder
            if(m_encoder != nil)
            {
                return;
            }
           
            switch(encoderType)
            {
                case CommandEncoderType::Render:
                {
                    m_commandEncoderType = CommandEncoderType::Render;
                    m_encoder = [m_mtlCommandBuffer renderCommandEncoderWithDescriptor : m_renderPassDescriptor];
                    m_renderPassDescriptor = nil;
                    break;
                }
                case CommandEncoderType::Compute:
                {
                    m_commandEncoderType = CommandEncoderType::Compute;
                    m_encoder = [m_mtlCommandBuffer computeCommandEncoder];
                    break;
                }
                case CommandEncoderType::Blit:
                {
                    m_commandEncoderType = CommandEncoderType::Blit;
                    m_encoder = [m_mtlCommandBuffer blitCommandEncoder];
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Encoder Type not supported");
                }
            }
            
            m_encoder.label = m_encoderScopeName;
            AZ_Assert(m_encoder != nil, "Could not create the encoder");
            m_isEncoded = true;
            
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
            if (m_supportsInterDrawTimestamps)
            {
                for(auto& timeStamp: m_timeStampQueue)
                {
                    SampleCounters(timeStamp.m_counterSampleBuffer, timeStamp.m_timeStampIndex);
                }
            }
#endif
        }

        bool CommandListBase::IsEncoded()
        {
            return m_isEncoded;
        }
        
        void CommandListBase::SetNameInternal(const AZStd::string_view& name)
        {
            m_encoderScopeName = [NSString stringWithCString:name.data() encoding:NSUTF8StringEncoding];
        }
        
        void CommandListBase::SetRenderPassInfo(MTLRenderPassDescriptor* renderPassDescriptor,
                                                const RHI::MultisampleState renderPassMultisampleState,
                                                const AZStd::set<id<MTLHeap>>& residentHeaps)
        {
            AZ_Assert(m_renderPassDescriptor == nil, "m_renderPassDescriptor should be nil from previous work");
            m_renderPassDescriptor = renderPassDescriptor;
            m_renderPassMultiSampleState = renderPassMultisampleState;
            m_residentHeaps = &residentHeaps;
        }

    
        void CommandListBase::WaitOnResourceFence(const Fence& fence)
        {
            fence.WaitOnGpu(m_mtlCommandBuffer);
        }
    
        void CommandListBase::SignalResourceFence(const Fence& fence)
        {
            fence.SignalFromGpu(m_mtlCommandBuffer);
        }
    
        void CommandListBase::AttachVisibilityBuffer(id<MTLBuffer> visibilityResultBuffer)
        {
            m_renderPassDescriptor.visibilityResultBuffer = visibilityResultBuffer;
        }
    
        const id<MTLCommandBuffer> CommandListBase::GetMtlCommandBuffer() const
        {
            return m_mtlCommandBuffer;
        }

#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
        void CommandListBase::SampleCounters(id<MTLCounterSampleBuffer> counterSampleBuffer, uint32_t sampleIndex)
        {
            if (!m_supportsInterDrawTimestamps)
            {
                return;
            }

            AZ_Assert(sampleIndex >= 0, "Invalid sample index");
            //useBarrier - Inserting a barrier ensures that encoded work is complete before the GPU samples the hardware counters.
            //If it is true there is a performance penalty but you will get consistent results
            bool useBarrier = false;
            
            switch(m_commandEncoderType)
            {
                case CommandEncoderType::Render:
                {
                    id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                    [renderEncoder sampleCountersInBuffer:counterSampleBuffer
                                            atSampleIndex:sampleIndex
                                              withBarrier:useBarrier];
                    break;
                }
                case CommandEncoderType::Compute:
                {
                    id<MTLComputeCommandEncoder> computeEncoder = GetEncoder<id<MTLComputeCommandEncoder>>();
                    [computeEncoder sampleCountersInBuffer:counterSampleBuffer
                                             atSampleIndex:sampleIndex
                                               withBarrier:useBarrier];
                    break;
                }
                case CommandEncoderType::Blit:
                {
                    id<MTLBlitCommandEncoder> blitEncoder = GetEncoder<id<MTLBlitCommandEncoder>>();
                    [blitEncoder sampleCountersInBuffer:counterSampleBuffer
                                          atSampleIndex:sampleIndex
                                            withBarrier:useBarrier];
                    break;
                }
            }
        }
    
        void CommandListBase::SamplePassCounters(id<MTLCounterSampleBuffer> counterSampleBuffer, uint32_t sampleIndex)
        {
            if (!m_supportsInterDrawTimestamps)
            {
                return;
            }

            if(m_encoder == nil)
            {
                //Queue the query to be activated upon encoder creation. Applies to timestamp queries
                m_timeStampQueue.push_back(TimeStampData{sampleIndex, counterSampleBuffer});
            }
            else
            {
                SampleCounters(counterSampleBuffer, sampleIndex);
            }
        }
#endif
    
        void CommandListBase::SetVisibilityResultMode(MTLVisibilityResultMode visibilityResultMode, size_t queryOffset)
        {
            AZ_Assert(m_commandEncoderType == CommandEncoderType::Render, "Occlusion query is only posible on render encoders");
            id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
            [renderEncoder setVisibilityResultMode:visibilityResultMode
                                        offset:queryOffset];
        }
    }
}
