/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Bits.h>
#include <AzCore/std/algorithm.h>
#include <RHI/ArgumentBuffer.h>
#include <RHI/Buffer.h>
#include <RHI/BufferView.h>
#include <RHI/BufferMemoryView.h>
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <RHI/MemoryView.h>
#include <RHI/PipelineState.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/SwapChain.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<CommandList> CommandList::Create()
        {
            return aznew CommandList();
        }

        void CommandList::Init(RHI::HardwareQueueClass hardwareQueueClass, Device* device)
        {
            CommandListBase::Init(hardwareQueueClass, device);
        }
        void CommandList::Reset()
        {
            m_state = State();
            
            m_state.m_pipelineState = nullptr;
            m_state.m_pipelineLayout = nullptr;
            m_state.m_rasterizerStateHash = AZ::HashValue64{0};
            m_state.m_depthStencilStateHash = 0;
            m_state.m_stencilRef = -1;
            AZStd::fill(m_state.m_streamsHashes.begin(), m_state.m_streamsHashes.end(), AZ::HashValue64{0});
            
            CommandListBase::Reset();
        }

        void CommandList::Close()
        {
            CommandListBase::Close();
        }

        void CommandList::FlushEncoder()
        {
            Reset();
            CommandListBase::FlushEncoder();
        }

        void CommandList::Submit(const RHI::DeviceCopyItem& copyItem, uint32_t submitIndex)
        {
            ValidateSubmitIndex(submitIndex);
            CreateEncoder(CommandEncoderType::Blit);

            id<MTLBlitCommandEncoder> blitEncoder = GetEncoder<id<MTLBlitCommandEncoder>>();
            switch (copyItem.m_type)
            {
                case RHI::CopyItemType::Buffer:
                {
                    const RHI::DeviceCopyBufferDescriptor& descriptor = copyItem.m_buffer;
                    const auto* sourceBuffer = static_cast<const Buffer*>(descriptor.m_sourceBuffer);
                    const auto* destinationBuffer = static_cast<const Buffer*>(descriptor.m_destinationBuffer);

                    [blitEncoder copyFromBuffer: sourceBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>()
                                   sourceOffset: descriptor.m_sourceOffset
                                       toBuffer: destinationBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>()
                              destinationOffset: descriptor.m_destinationOffset
                                           size: descriptor.m_size];

                    Platform::PublishBufferGpuChangeOnCpu(blitEncoder, destinationBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>());
                    break;
                }
                case RHI::CopyItemType::Image:
                {
                    const RHI::DeviceCopyImageDescriptor& descriptor = copyItem.m_image;
                    const Image* sourceImage = static_cast<const Image*>(descriptor.m_sourceImage);
                    const Image* destinationImage = static_cast<const Image*>(descriptor.m_destinationImage);

                    MTLOrigin sourceOrigin = MTLOriginMake(descriptor.m_sourceOrigin.m_left,
                                                           descriptor.m_sourceOrigin.m_top,
                                                           descriptor.m_sourceOrigin.m_front);

                    MTLSize sourceSize = MTLSizeMake(descriptor.m_sourceSize.m_width,
                                                     descriptor.m_sourceSize.m_height,
                                                     descriptor.m_sourceSize.m_depth);

                    MTLOrigin destinationOrigin = MTLOriginMake(descriptor.m_destinationOrigin.m_left,
                                                    descriptor.m_destinationOrigin.m_top,
                                                    descriptor.m_destinationOrigin.m_front);
                    
                    [blitEncoder copyFromTexture: sourceImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>()
                                     sourceSlice: descriptor.m_sourceSubresource.m_arraySlice
                                     sourceLevel: descriptor.m_sourceSubresource.m_mipSlice
                                    sourceOrigin: sourceOrigin
                                      sourceSize: sourceSize
                                       toTexture: destinationImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>()
                                destinationSlice: descriptor.m_destinationSubresource.m_arraySlice
                                destinationLevel: descriptor.m_destinationSubresource.m_mipSlice
                               destinationOrigin: destinationOrigin];

                    Platform::PublishTextureGpuChangeOnCpu(blitEncoder, destinationImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>());
                    break;
                }
                case RHI::CopyItemType::BufferToImage:
                {
                    const RHI::DeviceCopyBufferToImageDescriptor& descriptor = copyItem.m_bufferToImage;
                    const Buffer* sourceBuffer = static_cast<const Buffer*>(descriptor.m_sourceBuffer);
                    const Image* destinationImage = static_cast<const Image*>(descriptor.m_destinationImage);

                    MTLOrigin destinationOrigin = MTLOriginMake(descriptor.m_destinationOrigin.m_left,
                                                                descriptor.m_destinationOrigin.m_top,
                                                                descriptor.m_destinationOrigin.m_front);

                    MTLSize sourceSize = MTLSizeMake(descriptor.m_sourceSize.m_width,
                                                     descriptor.m_sourceSize.m_height,
                                                     descriptor.m_sourceSize.m_depth);

                    MTLBlitOption mtlBlitOption = GetBlitOption(descriptor.m_sourceFormat, descriptor.m_destinationSubresource.m_aspect);
                    [blitEncoder copyFromBuffer: sourceBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>()
                                   sourceOffset: sourceBuffer->GetMemoryView().GetOffset() + descriptor.m_sourceOffset
                              sourceBytesPerRow: descriptor.m_sourceBytesPerRow
                            sourceBytesPerImage: descriptor.m_sourceBytesPerImage
                                     sourceSize: sourceSize
                                      toTexture: destinationImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>()
                               destinationSlice: descriptor.m_destinationSubresource.m_arraySlice
                               destinationLevel: descriptor.m_destinationSubresource.m_mipSlice
                              destinationOrigin: destinationOrigin
                                        options: mtlBlitOption];

                    Platform::PublishTextureGpuChangeOnCpu(blitEncoder, destinationImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>());
                    break;
                }
                case RHI::CopyItemType::ImageToBuffer:
                {
                    const RHI::DeviceCopyImageToBufferDescriptor& descriptor = copyItem.m_imageToBuffer;
                    const auto* sourceImage = static_cast<const Image*>(descriptor.m_sourceImage);
                    const auto* destinationBuffer = static_cast<const Buffer*>(descriptor.m_destinationBuffer);

                    MTLOrigin sourceOrigin = MTLOriginMake(descriptor.m_sourceOrigin.m_left,
                                                           descriptor.m_sourceOrigin.m_top,
                                                           descriptor.m_sourceOrigin.m_front);

                    MTLSize sourceSize = MTLSizeMake(descriptor.m_sourceSize.m_width,
                                                     descriptor.m_sourceSize.m_height,
                                                     descriptor.m_sourceSize.m_depth);

                    MTLBlitOption mtlBlitOption = GetBlitOption(descriptor.m_destinationFormat, descriptor.m_sourceSubresource.m_aspect);
                    [blitEncoder copyFromTexture: sourceImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>()
                                     sourceSlice: descriptor.m_sourceSubresource.m_arraySlice
                                     sourceLevel: descriptor.m_sourceSubresource.m_mipSlice
                                    sourceOrigin: sourceOrigin
                                      sourceSize: sourceSize
                                        toBuffer: destinationBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>()
                               destinationOffset: destinationBuffer->GetMemoryView().GetOffset() + descriptor.m_destinationOffset
                          destinationBytesPerRow: descriptor.m_destinationBytesPerRow
                        destinationBytesPerImage: descriptor.m_destinationBytesPerImage
                                         options: mtlBlitOption];

                    Platform::PublishBufferGpuChangeOnCpu(blitEncoder, destinationBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>());
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Not Supported");
                }
            }
        }

        void CommandList::Submit(const RHI::DeviceDispatchItem& dispatchItem, uint32_t submitIndex)
        {
            AZ_PROFILE_FUNCTION(RHI);

            ValidateSubmitIndex(submitIndex);
            CreateEncoder(CommandEncoderType::Compute);
            bool bindResourceSuccessfull = CommitShaderResources<RHI::PipelineStateType::Dispatch>(dispatchItem);

            if(!bindResourceSuccessfull)
            {
                AZ_Assert(false, "Skip draw call as resource binding was unsuccessfully.");
                return;
            }
            const RHI::DispatchDirect& arguments = dispatchItem.m_arguments.m_direct;
            MTLSize threadsPerGroup = {arguments.m_threadsPerGroupX, arguments.m_threadsPerGroupY, arguments.m_threadsPerGroupZ};
            MTLSize numThreadGroup = {arguments.GetNumberOfGroupsX(), arguments.GetNumberOfGroupsY(), arguments.GetNumberOfGroupsZ()};

            id<MTLComputeCommandEncoder> computeEncoder = GetEncoder<id<MTLComputeCommandEncoder>>();
            [computeEncoder dispatchThreadgroups: numThreadGroup
                           threadsPerThreadgroup: threadsPerGroup];

        }

        void CommandList::Submit(const RHI::DeviceDispatchRaysItem& dispatchRaysItem, uint32_t submitIndex)
        {
            ValidateSubmitIndex(submitIndex);

            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
        }

        void CommandList::SetViewports(const RHI::Viewport* rhiViewports, uint32_t count)
        {
            m_state.m_viewportState.Set(AZStd::span<const RHI::Viewport>(rhiViewports, count));
        }

        void CommandList::SetScissors(const RHI::Scissor* rhiScissors, uint32_t count)
        {
            m_state.m_scissorState.Set(AZStd::span<const RHI::Scissor>(rhiScissors, count));
        }

        template <typename Item>
        void CommandList::SetRootConstants(const Item& item, const PipelineState* pipelineState)
        {
            const PipelineLayout* pipelineLayout = pipelineState->GetPipelineLayout();
            uint32_t rootConstantsSize = pipelineLayout->GetRootConstantsSize();
            uint32_t rootConstantsSlotIndex = pipelineLayout->GetRootConstantsSlotIndex();
            if (item.m_rootConstantSize  && rootConstantsSize > 0)
            {
                if(m_commandEncoderType == CommandEncoderType::Render)
                {
                    id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();

                    [renderEncoder setVertexBytes: item.m_rootConstants
                                           length: rootConstantsSize
                                          atIndex: rootConstantsSlotIndex];

                    [renderEncoder setFragmentBytes: item.m_rootConstants
                                             length: rootConstantsSize
                                            atIndex: rootConstantsSlotIndex];

                }
                else if(m_commandEncoderType == CommandEncoderType::Compute)
                {
                    id<MTLComputeCommandEncoder> computeEncoder = GetEncoder<id<MTLComputeCommandEncoder>>();
                    [computeEncoder setBytes: item.m_rootConstants
                                      length: rootConstantsSize
                                     atIndex: rootConstantsSlotIndex];

                }
            }
        }

        bool CommandList::SetArgumentBuffers(const PipelineState* pipelineState, RHI::PipelineStateType stateType)
        {
            bool bindNullDescriptorHeap = false;
            MTLRenderStages mtlRenderStagesForNullDescHeap = 0;
            ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(stateType);
            const PipelineLayout* pipelineLayout = pipelineState->GetPipelineLayout();

            uint32_t bufferVertexRegisterIdMin = RHI::Limits::Pipeline::ShaderResourceGroupCountMax;
            uint32_t bufferFragmentOrComputeRegisterIdMin = RHI::Limits::Pipeline::ShaderResourceGroupCountMax;
            uint32_t bufferVertexRegisterIdMax = 0;
            uint32_t bufferFragmentOrComputeRegisterIdMax = 0;

            m_mtlVertexArgBuffers.fill(nil);
            m_mtlFragmentOrComputeArgBuffers.fill(nil);
            m_mtlVertexArgBufferOffsets.fill(0);
            m_mtlFragmentOrComputeArgBufferOffsets.fill(0);

            for (uint32_t slot = 0; slot < RHI::Limits::Pipeline::ShaderResourceGroupCountMax; ++slot)
            {
                const ShaderResourceGroup* shaderResourceGroup = bindings.m_srgsBySlot[slot];
                uint32_t slotIndex = static_cast<uint32_t>(pipelineLayout->GetIndexBySlot(slot));
 
                //Check explicitly for Bindless SRG.
                if (slot == m_device->GetBindlessArgumentBuffer().GetBindlessSrgBindingSlot() &&
                    slotIndex != RHI::Limits::Pipeline::ShaderResourceGroupCountMax &&
                    shaderResourceGroup == nullptr)
                {
                    //Skip if the global static bindless heap is already bound
                    if (m_state.m_bindBindlessHeap)
                    {
                        continue;
                    }
                    
                    //Add the bindless AB info to the arrays in order to bind it to the appropriate encoder
                    m_device->GetBindlessArgumentBuffer().BindBindlessArgumentBuffer(slotIndex, m_commandEncoderType,
                                                                        m_mtlVertexArgBuffers, m_mtlVertexArgBufferOffsets,
                                                                        m_mtlFragmentOrComputeArgBuffers, m_mtlFragmentOrComputeArgBufferOffsets,
                                                                        bufferVertexRegisterIdMin, bufferVertexRegisterIdMax,
                                                                        bufferFragmentOrComputeRegisterIdMin, bufferFragmentOrComputeRegisterIdMax);
                    
                    //Make all the relevant ABs resident. This only applies to ABs related to unbounded array support
                    m_device->GetBindlessArgumentBuffer().MakeBindlessArgumentBuffersResident(m_commandEncoderType,
                                                                                              m_untrackedResourcesGfxRead,
                                                                                              m_untrackedResourcesComputeRead);
                     
                    m_state.m_bindBindlessHeap = true;
                    continue;
                }
                
                if(!shaderResourceGroup || slotIndex == RHI::Limits::Pipeline::ShaderResourceGroupCountMax)
                {
                    continue;
                }

                uint32_t srgVisIndex = static_cast<uint32_t>(pipelineLayout->GetIndexBySlot(shaderResourceGroup->GetBindingSlot()));
                const RHI::ShaderStageMask& srgVisInfo = pipelineLayout->GetSrgVisibility(srgVisIndex);
                const ShaderResourceGroupVisibility& srgResourcesVisInfo = pipelineLayout->GetSrgResourcesVisibility(srgVisIndex);

                bool isSrgUpdatd = bindings.m_srgsByIndex[slot] != shaderResourceGroup;
                if(isSrgUpdatd)
                {
                    bindings.m_srgsByIndex[slot] = shaderResourceGroup;
                    auto& compiledArgBuffer = shaderResourceGroup->GetCompiledArgumentBuffer();
                    id<MTLBuffer> argBuffer = compiledArgBuffer.GetArgEncoderBuffer();
                    size_t argBufferOffset = compiledArgBuffer.GetOffset();

                    if(srgVisInfo != RHI::ShaderStageMask::None)
                    {
                        bool isNullDescHeapNeeded = compiledArgBuffer.IsNullDescHeapNeeded();
                        bindNullDescriptorHeap |= isNullDescHeapNeeded;

                        //For graphics and compute shader stages, cache all the argument buffers, offsets and track the min/max indices
                        if(m_commandEncoderType == CommandEncoderType::Render)
                        {
                            uint8_t numBitsSet = RHI::CountBitsSet(static_cast<uint32_t>(srgVisInfo));
                            if( numBitsSet > 1 || srgVisInfo == RHI::ShaderStageMask::Vertex)
                            {
                                m_mtlVertexArgBuffers[slotIndex] = argBuffer;
                                m_mtlVertexArgBufferOffsets[slotIndex] = argBufferOffset;
                                bufferVertexRegisterIdMin = AZStd::min(slotIndex, bufferVertexRegisterIdMin);
                                bufferVertexRegisterIdMax = AZStd::max(slotIndex, bufferVertexRegisterIdMax);
                                mtlRenderStagesForNullDescHeap = shaderResourceGroup->IsNullHeapNeededForVertexStage(srgResourcesVisInfo) ?
                                                    mtlRenderStagesForNullDescHeap | MTLRenderStageVertex : mtlRenderStagesForNullDescHeap;
                            }

                            if( numBitsSet > 1 || srgVisInfo == RHI::ShaderStageMask::Fragment)
                            {
                                m_mtlFragmentOrComputeArgBuffers[slotIndex] = argBuffer;
                                m_mtlFragmentOrComputeArgBufferOffsets[slotIndex] = argBufferOffset;
                                bufferFragmentOrComputeRegisterIdMin = AZStd::min(slotIndex, bufferFragmentOrComputeRegisterIdMin);
                                bufferFragmentOrComputeRegisterIdMax = AZStd::max(slotIndex, bufferFragmentOrComputeRegisterIdMax);
                                mtlRenderStagesForNullDescHeap = isNullDescHeapNeeded ? mtlRenderStagesForNullDescHeap | MTLRenderStageFragment : mtlRenderStagesForNullDescHeap;
                            }
                        }
                        else if(m_commandEncoderType == CommandEncoderType::Compute)
                        {
                            m_mtlFragmentOrComputeArgBuffers[slotIndex] = argBuffer;
                            m_mtlFragmentOrComputeArgBufferOffsets[slotIndex] = argBufferOffset;
                            bufferFragmentOrComputeRegisterIdMin = AZStd::min(slotIndex, bufferFragmentOrComputeRegisterIdMin);
                            bufferFragmentOrComputeRegisterIdMax = AZStd::max(slotIndex, bufferFragmentOrComputeRegisterIdMax);
                        }
                    }
                }

                //Check if the srg has been updated or if the srg resources visibility hash has been updated
                //as it is possible for draw items to have different PSOs in the same pass.
                const AZ::HashValue64 srgResourcesVisHash = pipelineLayout->GetSrgResourcesVisibilityHash(srgVisIndex);
                if(bindings.m_srgVisHashByIndex[slot] != srgResourcesVisHash || isSrgUpdatd)
                {
                    bindings.m_srgVisHashByIndex[slot] = srgResourcesVisHash;
                    if(srgVisInfo != RHI::ShaderStageMask::None)
                    {
                        //For graphics and compute encoder cache all the resources used by the pass in order
                        //to eventually make them resident (call UseResource) for the duration
                        //of the work associated with the current scope and ensure that it's in a
                        //format compatible with the appropriate metal function.
                        if(m_commandEncoderType == CommandEncoderType::Render)
                        {
                            //Collect resources bound via Bindless SRG
                            CollectBindlessGfxUntrackedResources(shaderResourceGroup,
                                                                 m_untrackedResourcesGfxRead,
                                                                 m_untrackedResourcesGfxReadWrite);
                            
                            //Collect resources bound via shaderResourceGroup
                            shaderResourceGroup->CollectUntrackedResources(srgResourcesVisInfo,
                                                                           m_untrackedResourcesGfxRead,
                                                                           m_untrackedResourcesGfxReadWrite);
                        }
                        else if(m_commandEncoderType == CommandEncoderType::Compute)
                        {
                            //Collect resources bound via Bindless SRG
                            CollectBindlessComputeUntrackedResources(shaderResourceGroup,
                                                                     m_untrackedResourcesComputeRead,
                                                                     m_untrackedResourcesComputeReadWrite);
                            
                            //Collect resources bound via shaderResourceGroup
                            shaderResourceGroup->CollectUntrackedResources(srgResourcesVisInfo,
                                                                           m_untrackedResourcesComputeRead,
                                                                           m_untrackedResourcesComputeReadWrite);
                        }
                    }
                }
            }

            //For graphics and compute encoder bind all the argument buffers
            if(m_commandEncoderType == CommandEncoderType::Render)
            {
                BindArgumentBuffers(RHI::ShaderStage::Vertex,
                                    bufferVertexRegisterIdMin,
                                    bufferVertexRegisterIdMax,
                                    m_mtlVertexArgBuffers,
                                    m_mtlVertexArgBufferOffsets);

                BindArgumentBuffers(RHI::ShaderStage::Fragment,
                                    bufferFragmentOrComputeRegisterIdMin,
                                    bufferFragmentOrComputeRegisterIdMax,
                                    m_mtlFragmentOrComputeArgBuffers,
                                    m_mtlFragmentOrComputeArgBufferOffsets);
            }
            else if(m_commandEncoderType == CommandEncoderType::Compute)
            {
                BindArgumentBuffers(RHI::ShaderStage::Compute,
                                    bufferFragmentOrComputeRegisterIdMin,
                                    bufferFragmentOrComputeRegisterIdMax,
                                    m_mtlFragmentOrComputeArgBuffers,
                                    m_mtlFragmentOrComputeArgBufferOffsets);
            }

            if(bindNullDescriptorHeap)
            {
                MakeHeapsResident(mtlRenderStagesForNullDescHeap);
            }
            return true;
        }

        CommandList::ResourceProperties CommandList::GetResourceInfo(RHI::BindlessResourceType resourceType,
                                                                    const RHI::DeviceResourceView* resourceView)
        {
            id<MTLResource> mtlResourceView = nil;
            bool isReadOnlyResource = false;
            switch(resourceType)
            {
                case RHI::BindlessResourceType::m_Texture2D:
                case RHI::BindlessResourceType::m_TextureCube:
                {
                    isReadOnlyResource = true;
                }
                case RHI::BindlessResourceType::m_RWTexture2D:
                {
                    const ImageView* imageView = static_cast<const ImageView*>(resourceView);
                    mtlResourceView = imageView->GetMemoryView().GetGpuAddress<id<MTLResource>>();
                    break;
                }
                case RHI::BindlessResourceType::m_ByteAddressBuffer:
                {
                    isReadOnlyResource = true;
                }
                case RHI::BindlessResourceType::m_RWByteAddressBuffer:
                {
                    const BufferView* bufferView = static_cast<const BufferView*>(resourceView);
                    mtlResourceView = bufferView->GetMemoryView().GetGpuAddress<id<MTLResource>>();
                    break;
                }
            }
            return CommandList::ResourceProperties(isReadOnlyResource, mtlResourceView);
        }
    
        void CommandList::CollectBindlessComputeUntrackedResources(const ShaderResourceGroup* shaderResourceGroup,
                                                ArgumentBuffer::ResourcesForCompute& untrackedResourceComputeRead,
                                                ArgumentBuffer::ResourcesForCompute& untrackedResourceComputeReadWrite)
        {
            if(shaderResourceGroup->GetData().GetBindlessViewsSize()==0)
            {
                return;
            }
            
            for (const auto& it : shaderResourceGroup->GetData().GetBindlessResourceViews())
            {
                for(const auto& resourceViewsIt : it.second.m_bindlessResources)
                {
                    CommandList::ResourceProperties resourceInfo = GetResourceInfo(it.second.m_bindlessResourceType,
                                                                                    resourceViewsIt.get());
                    //Check if the resource is read only
                    if(resourceInfo.first)
                    {
                        m_untrackedResourcesComputeRead.insert(resourceInfo.second);
                    }
                    else
                    {
                        m_untrackedResourcesComputeReadWrite.insert(resourceInfo.second);
                    }
                }
            }
        }
    
        void CommandList::CollectBindlessGfxUntrackedResources(const ShaderResourceGroup* shaderResourceGroup,
                                            ArgumentBuffer::ResourcesPerStageForGraphics& untrackedResourcesGfxRead,
                                            ArgumentBuffer::ResourcesPerStageForGraphics& untrackedResourcesGfxReadWrite)
        {
            if(shaderResourceGroup->GetData().GetBindlessViewsSize()==0)
            {
                return;
            }
            
            for (const auto& it : shaderResourceGroup->GetData().GetBindlessResourceViews())
            {
                // Iterate through all the ResourceViews
                for(const auto& resourceViewsIt : it.second.m_bindlessResources)
                {
                    CommandList::ResourceProperties resourceInfo = GetResourceInfo(it.second.m_bindlessResourceType,
                                                                     resourceViewsIt.get());
                    //Check if the resource is read only
                    if(resourceInfo.first)
                    {
                        untrackedResourcesGfxRead[RHI::ShaderStageVertex].insert(resourceInfo.second);
                        untrackedResourcesGfxRead[RHI::ShaderStageFragment].insert(resourceInfo.second);
                    }
                    else
                    {
                        //For RW resources we do not call UseResouce for vertex shader as that causes a gpu crash
                        untrackedResourcesGfxReadWrite[RHI::ShaderStageFragment].insert(resourceInfo.second);
                    }
                }
            }
        }
    
        void CommandList::BindArgumentBuffers(RHI::ShaderStage shaderStage,
                                              uint16_t registerIdMin,
                                              uint16_t registerIdMax,
                                              MetalArgumentBufferArray& mtlArgBuffers,
                                              MetalArgumentBufferArrayOffsets mtlArgBufferOffsets)
        {
            //Metal Api only lets you bind multiple argument buffers in an array as long as there are no gaps in the array
            //In order to accomodate that we break up the calls when a gap is noticed in the array and reconfigure the NSRange.
            uint16_t startingIndex = registerIdMin;
            bool trackingRange = true;
            for(int i = registerIdMin; i <= registerIdMax+1; i++)
            {
                if(trackingRange)
                {
                    if(mtlArgBuffers[i] == nil)
                    {
                        NSRange range = { startingIndex, i-startingIndex };

                        switch(shaderStage)
                        {
                            case RHI::ShaderStage::Vertex:
                            {
                                id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                                [renderEncoder setVertexBuffers: &mtlArgBuffers[startingIndex]
                                                        offsets: &mtlArgBufferOffsets[startingIndex]
                                                      withRange: range];
                                break;
                            }
                            case RHI::ShaderStage::Fragment:
                            {
                                id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                                [renderEncoder setFragmentBuffers: &mtlArgBuffers[startingIndex]
                                                          offsets: &mtlArgBufferOffsets[startingIndex]
                                                        withRange: range];
                                break;
                            }
                            case RHI::ShaderStage::Compute:
                            {
                                id<MTLComputeCommandEncoder> computeEncoder = GetEncoder<id<MTLComputeCommandEncoder>>();
                                [computeEncoder     setBuffers: &mtlArgBuffers[startingIndex]
                                                       offsets: &mtlArgBufferOffsets[startingIndex]
                                                     withRange: range];
                                break;
                            }
                            default:
                            {
                                AZ_Assert(false, "Not supported");
                            }
                        }

                        trackingRange = false;

                    }
                }
                else
                {
                    if(mtlArgBuffers[i] != nil)
                    {
                        startingIndex = i;
                        trackingRange = true;
                    }
                }
            }
        }

        void CommandList::Submit(const RHI::DeviceDrawItem& drawItem, uint32_t submitIndex)
        {
            AZ_PROFILE_FUNCTION(RHI);

            ValidateSubmitIndex(submitIndex);
            CreateEncoder(CommandEncoderType::Render);

            RHI::CommandListScissorState scissorState;
            if (drawItem.m_scissorsCount)
            {
                scissorState = m_state.m_scissorState;
                SetScissors(drawItem.m_scissors, drawItem.m_scissorsCount);
            }
            RHI::CommandListViewportState viewportState;
            if (drawItem.m_viewportsCount)
            {
                viewportState = m_state.m_viewportState;
                SetViewports(drawItem.m_viewports, drawItem.m_viewportsCount);
            }
            CommitViewportState();
            CommitScissorState();

            const PipelineState* pipelineState = static_cast<const PipelineState*>(drawItem.m_pipelineState);
            AZ_Assert(pipelineState, "PipelineState can not be null");

            if(m_renderPassMultiSampleState != pipelineState->m_pipelineStateMultiSampleState)
            {
                AZ_Assert(false,"MultisampleState in the image descriptor needs to match the one provided in the pipeline state");
            }

            id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
            bool bindResourceSuccessfull = CommitShaderResources<RHI::PipelineStateType::Draw>(drawItem);
            if(!bindResourceSuccessfull)
            {
                AZ_Assert(false, "Skip draw call as resource binding was unsuccessfully.");
                return;
            }

            SetStreamBuffers(*drawItem.m_geometryView, drawItem.m_streamIndices);
            SetStencilRef(drawItem.m_stencilRef);

            MTLPrimitiveType mtlPrimType = pipelineState->GetPipelineTopology();

            switch (drawItem.m_geometryView->GetDrawArguments().m_type)
            {
                case RHI::DrawType::Indexed:
                {
                    const RHI::DrawIndexed& indexed = drawItem.m_geometryView->GetDrawArguments().m_indexed;
                    const RHI::DeviceIndexBufferView& indexBuffDescriptor = drawItem.m_geometryView->GetIndexBufferView();
                    const Buffer * buff = static_cast<const Buffer*>(indexBuffDescriptor.GetBuffer());
                    id<MTLBuffer> mtlBuff = buff->GetMemoryView().GetGpuAddress<id<MTLBuffer>>();
                    MTLIndexType mtlIndexType = (indexBuffDescriptor.GetIndexFormat() == RHI::IndexFormat::Uint16) ?
                                                            MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
                    uint32_t indexTypeSize = 0;
                    GetIndexTypeSizeInBytes(mtlIndexType, indexTypeSize);

                    uint32_t indexOffset = static_cast<uint32_t>(indexBuffDescriptor.GetByteOffset() + (indexed.m_indexOffset * indexTypeSize) + buff->GetMemoryView().GetOffset());
                    [renderEncoder drawIndexedPrimitives: mtlPrimType
                                              indexCount: indexed.m_indexCount
                                               indexType: mtlIndexType
                                             indexBuffer: mtlBuff
                                       indexBufferOffset: indexOffset
                                           instanceCount: drawItem.m_drawInstanceArgs.m_instanceCount
                                              baseVertex: indexed.m_vertexOffset
                                            baseInstance: drawItem.m_drawInstanceArgs.m_instanceOffset];
                    break;
                }

                case RHI::DrawType::Linear:
                {
                    const RHI::DrawLinear& linear = drawItem.m_geometryView->GetDrawArguments().m_linear;
                    [renderEncoder drawPrimitives: mtlPrimType
                                      vertexStart: linear.m_vertexOffset
                                      vertexCount: linear.m_vertexCount
                                    instanceCount: drawItem.m_drawInstanceArgs.m_instanceCount
                                     baseInstance: drawItem.m_drawInstanceArgs.m_instanceOffset];
                    break;
                }
            }

            // Restore the scissors if needed.
            if (scissorState.IsValid())
            {
                SetScissors(scissorState.m_states.data(), aznumeric_caster(scissorState.m_states.size()));
            }

            // Restore the viewports if needed.
            if (viewportState.IsValid())
            {
                SetViewports(viewportState.m_states.data(), aznumeric_caster(viewportState.m_states.size()));
            }
        }

        void CommandList::Shutdown()
        {
            CommandListBase::Shutdown();
        }

        void CommandList::SetPipelineState(const PipelineState* pipelineState)
        {
            if (m_state.m_pipelineState != pipelineState)
            {
                AZ_PROFILE_FUNCTION(RHI);
                m_state.m_pipelineState = pipelineState;

                switch (pipelineState->GetType())
                {
                    case RHI::PipelineStateType::Draw:
                    {
                        id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                        SetRasterizerState(pipelineState->GetRasterizerState());
                        
                        id<MTLDepthStencilState> mtlDSstate = pipelineState->GetDepthStencilState();
                        if(mtlDSstate && m_state.m_depthStencilStateHash != mtlDSstate.hash)
                        {
                            m_state.m_depthStencilStateHash = mtlDSstate.hash;
                            [renderEncoder setDepthStencilState: pipelineState->GetDepthStencilState()];
                        }
                        [renderEncoder setRenderPipelineState: pipelineState->GetGraphicsPipelineState()];
                        break;
                    }
                    case RHI::PipelineStateType::Dispatch:
                    {
                        id<MTLComputeCommandEncoder> computeEncoder = GetEncoder<id<MTLComputeCommandEncoder>>();
                        [computeEncoder setComputePipelineState: pipelineState->GetComputePipelineState()];
                        break;
                    }
                    default:
                    {
                        AZ_Assert(false, "Type not supported.");
                    }
                }

                ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(pipelineState->GetType());
                for (size_t i = 0; i < bindings.m_srgsByIndex.size(); ++i)
                {
                    bindings.m_srgsByIndex[i] = nullptr;
                    bindings.m_srgVisHashByIndex[i] = AZ::HashValue64{0};
                }

                const PipelineLayout* pipelineLayout = pipelineState->GetPipelineLayout();
                if (m_state.m_pipelineLayout != pipelineLayout)
                {
                    m_state.m_pipelineLayout = pipelineLayout;
                }
            }
        }

        void CommandList::SetStencilRef(uint8_t stencilRef)
        {
            if (m_state.m_stencilRef != stencilRef)
            {
                id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                [renderEncoder setStencilReferenceValue: stencilRef];
                m_state.m_stencilRef = stencilRef;
            }
        }

        void CommandList::SetStreamBuffers(const RHI::DeviceGeometryView& geometryBufferViews, const RHI::StreamBufferIndices& streamIndices)
        {
            auto streamIter = geometryBufferViews.CreateStreamIterator(streamIndices);
            bool needsBinding = false;
            for (u8 index = 0; !streamIter.HasEnded(); ++streamIter, ++index)
            {
                if (m_state.m_streamsHashes[index] != streamIter->GetHash())
                {
                    m_state.m_streamsHashes[index] = streamIter->GetHash();
                    needsBinding = true;
                }
            }
            
            AZStd::array<id<MTLBuffer>, METAL_MAX_ENTRIES_BUFFER_ARG_TABLE> mtlStreamBuffers;
            AZStd::array<NSUInteger, METAL_MAX_ENTRIES_BUFFER_ARG_TABLE> mtlStreamBufferOffsets;
            if (needsBinding)
            {
                uint16_t bufferArrayLen = 0;
                streamIter.Reset();
                u8 count = streamIndices.Size();
                AZ_Assert(count <= METAL_MAX_ENTRIES_BUFFER_ARG_TABLE , "Slots needed cannot exceed METAL_MAX_ENTRIES_BUFFER_ARG_TABLE");

                NSRange range = {METAL_MAX_ENTRIES_BUFFER_ARG_TABLE - count, count};
                //The stream buffers are populated from bottom to top as the top slots are taken by argument buffers
                for (int i = count-1; i >= 0; --i)
                {
                    if (streamIter[i].GetBuffer())
                    {
                        const Buffer * buff = static_cast<const Buffer*>(streamIter[i].GetBuffer());
                        id<MTLBuffer> mtlBuff = buff->GetMemoryView().GetGpuAddress<id<MTLBuffer>>();
                        uint32_t offset = static_cast<uint32_t>(streamIter[i].GetByteOffset() + buff->GetMemoryView().GetOffset());
                        mtlStreamBuffers[bufferArrayLen] = mtlBuff;
                        mtlStreamBufferOffsets[bufferArrayLen] = offset;
                        bufferArrayLen++;
                    }
                }
                id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                [renderEncoder setVertexBuffers: mtlStreamBuffers.data()
                                        offsets: mtlStreamBufferOffsets.data()
                                      withRange: range];
            }
        }

        void CommandList::SetRasterizerState(const RasterizerState& rastState)
        {
            if (m_state.m_rasterizerStateHash != rastState.m_hash)
            {
                m_state.m_rasterizerStateHash = rastState.m_hash;
                id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                [renderEncoder setCullMode: rastState.m_cullMode];
                [renderEncoder setDepthBias: rastState.m_depthBias slopeScale: rastState.m_depthSlopeScale clamp: rastState.m_depthBiasClamp];
                [renderEncoder setFrontFacingWinding: rastState.m_frontFaceWinding];
                [renderEncoder setTriangleFillMode: rastState.m_triangleFillMode];
                [renderEncoder setDepthClipMode: rastState.m_depthClipMode];
            }
        }

        void CommandList::SetShaderResourceGroupForDraw(const RHI::DeviceShaderResourceGroup& shaderResourceGroup)
        {
            SetShaderResourceGroup<RHI::PipelineStateType::Draw>(static_cast<const ShaderResourceGroup*>(&shaderResourceGroup));
        }

        void CommandList::SetShaderResourceGroupForDispatch(const RHI::DeviceShaderResourceGroup& shaderResourceGroup)
        {
            SetShaderResourceGroup<RHI::PipelineStateType::Dispatch>(static_cast<const ShaderResourceGroup*>(&shaderResourceGroup));
        }

        CommandList::ShaderResourceBindings& CommandList::GetShaderResourceBindingsByPipelineType(RHI::PipelineStateType pipelineType)
        {
            return m_state.m_bindingsByPipe[static_cast<size_t>(pipelineType)];
        }

        template <RHI::PipelineStateType pipelineType, typename Item>
        bool CommandList::CommitShaderResources(const Item& item)
        {
            const PipelineState* pipelineState = static_cast<const PipelineState*>(item.m_pipelineState);
            if(!pipelineState)
            {
                AZ_Error("CommandList", false, "Pipeline state not provided");
                return false;
            }
            
            const PipelineLayout* pipelineLayout = pipelineState->GetPipelineLayout();
            if(!pipelineLayout)
            {
                AZ_Error("CommandList", false, "Pipeline layout not provided");
                return false;
            }

            SetPipelineState(pipelineState);

            // Assign shader resource groups from the item to slot bindings.
            for (uint32_t srgIndex = 0; srgIndex < item.m_shaderResourceGroupCount; ++srgIndex)
            {
                SetShaderResourceGroup<pipelineType>(static_cast<const ShaderResourceGroup*>(item.m_shaderResourceGroups[srgIndex]));
            }

            if (item.m_uniqueShaderResourceGroup)
            {
                SetShaderResourceGroup<pipelineType>(static_cast<const ShaderResourceGroup*>(item.m_uniqueShaderResourceGroup));
            }

            SetRootConstants(item, pipelineState);
            return SetArgumentBuffers(pipelineState, pipelineType);
        }

        void CommandList::CommitViewportState()
        {
            if (!m_state.m_viewportState.m_isDirty)
            {
                return;
            }

            AZ_PROFILE_FUNCTION(RHI);
            const auto& viewports = m_state.m_viewportState.m_states;
            MTLViewport metalViewports[viewports.size()];

            for (uint32_t i = 0; i < viewports.size(); ++i)
            {
                metalViewports[i].originX = viewports[i].m_minX;
                metalViewports[i].originY = viewports[i].m_minY;
                metalViewports[i].width = viewports[i].m_maxX - viewports[i].m_minX;
                metalViewports[i].height = viewports[i].m_maxY - viewports[i].m_minY;
                metalViewports[i].znear = viewports[i].m_minZ;
                metalViewports[i].zfar = viewports[i].m_maxZ;
            }

            id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
            [renderEncoder setViewports: metalViewports
                                  count: viewports.size()];
            m_state.m_viewportState.m_isDirty = false;
        }

        void CommandList::CommitScissorState()
        {
            if (!m_state.m_scissorState.m_isDirty)
            {
                return;
            }

            AZStd::array<MTLScissorRect, MaxScissorsAllowed> metalScissorRects;
            const auto& scissors = m_state.m_scissorState.m_states;

            AZ_Assert(scissors.size() <= MaxScissorsAllowed , "Number of scissors violate the maximum number of scissors allowed");
            for (uint32_t i = 0; i < scissors.size(); ++i)
            {
                metalScissorRects[i].x = scissors[i].m_minX;
                metalScissorRects[i].y = scissors[i].m_minY;
                metalScissorRects[i].width = scissors[i].m_maxX - scissors[i].m_minX;
                metalScissorRects[i].height = scissors[i].m_maxY - scissors[i].m_minY;
            }

            id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
            [renderEncoder setScissorRects: metalScissorRects.data()
                                     count: scissors.size()];
            m_state.m_scissorState.m_isDirty = false;
        }

        void CommandList::BuildBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
        }

        void CommandList::UpdateBottomLevelAccelerationStructure(const RHI::DeviceRayTracingBlas& rayTracingBlas)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
        }

        void CommandList::QueryBlasCompactionSizes(
            const AZStd::vector<AZStd::pair<RHI::DeviceRayTracingBlas*, RHI::DeviceRayTracingCompactionQuery*>>& blasToQuery)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
        }

        void CommandList::CompactBottomLevelAccelerationStructure(
            const RHI::DeviceRayTracingBlas& sourceBlas, const RHI::DeviceRayTracingBlas& compactBlas)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
        }

        void CommandList::BuildTopLevelAccelerationStructure(
            const RHI::DeviceRayTracingTlas& rayTracingTlas, const AZStd::vector<const RHI::DeviceRayTracingBlas*>& changedBlasList)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
        }
    }
}
