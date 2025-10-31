/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Object.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/MultisampleState.h>
#include <Metal/Metal.h>
#include <RHI/Fence.h>
#include <RHI/Metal.h>
#include <RHI/ArgumentBuffer.h>

namespace AZ
{
    namespace Metal
    {
        class Device;
        class CommandQueueCommandBuffer;
    
        class CommandListBase
            : public RHI::Object
        {
        public:
            CommandListBase() = default;
            virtual ~CommandListBase() = 0;
            CommandListBase(const CommandListBase&) = delete;
            virtual void Reset();
            virtual void Close();
            virtual void FlushEncoder();
            
            //! This function is called to indicate that this commandlist is open for encoding.
            //! We cant open the encoder here and have to create it lazily because we don't know the type of work the CommandList will do until 'Submit' is called.
            //! @param mtlCommandBuffer The command buffer that is assigned to this command list to be used to create the encoder
            void Open(id <MTLCommandBuffer> mtlCommandBuffer);
            
            //! @param subEncoder - Since subRenderEncoders are created by higher level code (in order to maintain correct ordering) they can be directly provided to the commandlist to order to be used for encoding. SubEncoders only apply to Graphics related work.
            //! @param mtlCommandBuffer - The command buffer that is assigned to this command list to be used for fencing related commands
            void Open(id <MTLCommandEncoder> subEncoder, id <MTLCommandBuffer> mtlCommandBuffer);
            
            //! This function returns true if the commandlist is going to encode something.
            bool IsEncoded();
            
            //! Cache render pass related data required to create an encoder or do validation checks
            void SetRenderPassInfo(MTLRenderPassDescriptor* renderPassDescriptor,
                                   const RHI::MultisampleState renderPassMultisampleState,
                                   const AZStd::set<id<MTLHeap>>& residentHeaps);

            void CreateEncoder(CommandEncoderType encoderType);

            //Fencing operations for aliased resources
            void WaitOnResourceFence(const Fence& fence);
            void SignalResourceFence(const Fence& fence);
            
            //! Attach visibility buffer for occlusion testing
            void AttachVisibilityBuffer(id<MTLBuffer> visibilityResultBuffer);
            //! Support for binary/precise occlusion 
            void SetVisibilityResultMode(MTLVisibilityResultMode visibilityResultMode, size_t queryOffset);
            //! Get the Command buffer associated with this command list
            const id<MTLCommandBuffer> GetMtlCommandBuffer() const;
#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
            //! Tell the driver to embed a sampling call. The type of sample depends on MTLCounterSampleBuffer passeed in
            void SampleCounters(id<MTLCounterSampleBuffer> counterSampleBuffer, uint32_t sampleIndex);
            //! Check if an encoder is available and if not queue it for when the encoder is created.
            void SamplePassCounters(id<MTLCounterSampleBuffer> counterSampleBuffer, uint32_t sampleIndex);
#endif
            
        protected:
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////
            
            void Init(RHI::HardwareQueueClass hardwareQueueClass, Device* device);
            void Shutdown();
                
            //! Go through all the heaps and call UseHeap on them to make them resident for the upcoming pass.
            void MakeHeapsResident(MTLRenderStages renderStages);

            template <typename T>
            T GetEncoder() const
            {
                return static_cast<T>(m_encoder);
            }
            
            id <MTLCommandEncoder>      m_encoder = nil;
            CommandEncoderType          m_commandEncoderType = Metal::CommandEncoderType::Invalid;
            
            /// Cache multisample state. Used mainly to validate the MSAA image descriptor against the one passed into the pipelinestate
            RHI::MultisampleState       m_renderPassMultiSampleState;
            
            // Data structures to cache untracked resources for Graphics and Compute Passes.
            // At the end of the pass we call UseResource on them in a batch to tell the
            // driver to ensure they are resident when needed.
            ArgumentBuffer::ResourcesPerStageForGraphics m_untrackedResourcesGfxRead;
            ArgumentBuffer::ResourcesPerStageForGraphics m_untrackedResourcesGfxReadWrite;
            ArgumentBuffer::ResourcesForCompute m_untrackedResourcesComputeRead;
            ArgumentBuffer::ResourcesForCompute m_untrackedResourcesComputeReadWrite;

            Device* m_device = nullptr;
        private:
            
            bool m_isEncoded                                    = false;
            bool m_isNullDescHeapBound                          = false;
            RHI::HardwareQueueClass m_hardwareQueueClass        = RHI::HardwareQueueClass::Graphics;
            NSString* m_encoderScopeName                        = nullptr;
            id <MTLCommandBuffer> m_mtlCommandBuffer            = nil;
            MTLRenderPassDescriptor* m_renderPassDescriptor     = nil;
            
            const AZStd::set<id<MTLHeap>>* m_residentHeaps = nullptr;

            bool m_supportsInterDrawTimestamps                  = AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING; // iOS/TVOS = false, MacOS = defaults to true

#if AZ_TRAIT_ATOM_METAL_COUNTER_SAMPLING
            struct TimeStampData
            {
                uint32_t m_timeStampIndex = -1;
                id<MTLCounterSampleBuffer> m_counterSampleBuffer;
            };
            AZStd::vector<TimeStampData>     m_timeStampQueue;
#endif
        };
    }
}
