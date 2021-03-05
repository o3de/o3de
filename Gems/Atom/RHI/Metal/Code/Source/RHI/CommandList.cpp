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
#include "Atom_RHI_Metal_precompiled.h"

#include <Atom/RHI.Reflect/Bits.h>
#include <AzCore/Debug/EventTrace.h>
#include <RHI/ArgumentBuffer.h>
#include <RHI/Buffer.h>
#include <RHI/BufferMemoryView.h>
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
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
            //We are deliberately not doing m_state = State(); because on ios build server machines we were getting this issue
            //Undefined symbols for architecture arm64:
            //   "_objc_memmove_collectable", referenced from:
            //We can come back and revisit this after upgrading the build server machines to Mojave.
            
            m_state.m_pipelineState = nullptr;
            m_state.m_pipelineLayout = nullptr;
            m_state.m_streamsHash = AZ::HashValue64{0};
            m_state.m_indicesHash = AZ::HashValue64{0};
            m_state.m_stencilRef = -1;
            
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
        
        void CommandList::Submit(const RHI::CopyItem& copyItem)
        {
            CreateEncoder(CommandEncoderType::Blit);
            
            id<MTLBlitCommandEncoder> blitEncoder = GetEncoder<id<MTLBlitCommandEncoder>>();
            switch (copyItem.m_type)
            {
                case RHI::CopyItemType::Buffer:
                {
                    const RHI::CopyBufferDescriptor& descriptor = copyItem.m_buffer;
                    const auto* sourceBuffer = static_cast<const Buffer*>(descriptor.m_sourceBuffer);
                    const auto* destinationBuffer = static_cast<const Buffer*>(descriptor.m_destinationBuffer);

                    [blitEncoder copyFromBuffer:sourceBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>()
                                   sourceOffset:descriptor.m_sourceOffset
                                       toBuffer:destinationBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>()
                              destinationOffset:descriptor.m_destinationOffset
                                           size:descriptor.m_size];
                    
                    break;
                }
                case RHI::CopyItemType::Image:
                {
                    const RHI::CopyImageDescriptor& descriptor = copyItem.m_image;
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
                    break;
                }
                case RHI::CopyItemType::BufferToImage:
                {
                    const RHI::CopyBufferToImageDescriptor& descriptor = copyItem.m_bufferToImage;
                    const Buffer* sourceBuffer = static_cast<const Buffer*>(descriptor.m_sourceBuffer);
                    const Image* destinationImage = static_cast<const Image*>(descriptor.m_destinationImage);

                    MTLOrigin destinationOrigin = MTLOriginMake(descriptor.m_destinationOrigin.m_left,
                                                                descriptor.m_destinationOrigin.m_top,
                                                                descriptor.m_destinationOrigin.m_front);
                    
                    MTLSize sourceSize = MTLSizeMake(descriptor.m_sourceSize.m_width,
                                                     descriptor.m_sourceSize.m_height,
                                                     descriptor.m_sourceSize.m_depth);
                    
                    [blitEncoder copyFromBuffer:sourceBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>()
                                   sourceOffset:sourceBuffer->GetMemoryView().GetOffset() + descriptor.m_sourceOffset
                              sourceBytesPerRow:descriptor.m_sourceBytesPerRow
                            sourceBytesPerImage:descriptor.m_sourceBytesPerImage
                                     sourceSize:sourceSize
                                      toTexture:destinationImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>()
                               destinationSlice:descriptor.m_destinationSubresource.m_arraySlice
                               destinationLevel:descriptor.m_destinationSubresource.m_mipSlice
                              destinationOrigin:destinationOrigin];
                    
                    Platform::SynchronizeTextureOnGPU(blitEncoder, destinationImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>());
                    break;
                }
                case RHI::CopyItemType::ImageToBuffer:
                {
                    const RHI::CopyImageToBufferDescriptor& descriptor = copyItem.m_imageToBuffer;
                    const auto* sourceImage = static_cast<const Image*>(descriptor.m_sourceImage);
                    const auto* destinationBuffer = static_cast<const Buffer*>(descriptor.m_destinationBuffer);
                    
                    MTLOrigin sourceOrigin = MTLOriginMake(descriptor.m_sourceOrigin.m_left,
                                                           descriptor.m_sourceOrigin.m_top,
                                                           descriptor.m_sourceOrigin.m_front);
                    
                    MTLSize sourceSize = MTLSizeMake(descriptor.m_sourceSize.m_width,
                                                     descriptor.m_sourceSize.m_height,
                                                     descriptor.m_sourceSize.m_depth);
                    
                    [blitEncoder copyFromTexture:sourceImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>()
                                     sourceSlice:descriptor.m_sourceSubresource.m_arraySlice
                                     sourceLevel:descriptor.m_sourceSubresource.m_mipSlice
                                    sourceOrigin:sourceOrigin
                                      sourceSize:sourceSize
                                        toBuffer:destinationBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>()
                               destinationOffset:destinationBuffer->GetMemoryView().GetOffset() + descriptor.m_destinationOffset
                          destinationBytesPerRow:descriptor.m_destinationBytesPerRow
                        destinationBytesPerImage:descriptor.m_destinationBytesPerImage];
                    
                    Platform::SynchronizeBufferOnGPU(blitEncoder, destinationBuffer->GetMemoryView().GetGpuAddress<id<MTLBuffer>>());
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Not Supported");
                }
            }
        }
        
        void CommandList::Submit(const RHI::DispatchItem& dispatchItem)
        {
            AZ_TRACE_METHOD();
            
            CreateEncoder(CommandEncoderType::Compute);
            bool bindResourceSuccessfull = CommitShaderResources<RHI::PipelineStateType::Dispatch>(dispatchItem);
            
            if(!bindResourceSuccessfull)
            {
                AZ_Assert(false, "Resource binding was unsuccessfully.");
                return;
            }
            const RHI::DispatchDirect& arguments = dispatchItem.m_arguments.m_direct;
            MTLSize threadsPerGroup = {arguments.m_threadsPerGroupX, arguments.m_threadsPerGroupY, arguments.m_threadsPerGroupZ}; 
            MTLSize numThreadGroup = {arguments.GetNumberOfGroupsX(), arguments.GetNumberOfGroupsY(), arguments.GetNumberOfGroupsZ()};
            
            id<MTLComputeCommandEncoder> computeEncoder = GetEncoder<id<MTLComputeCommandEncoder>>();
            [computeEncoder dispatchThreadgroups: numThreadGroup
                             threadsPerThreadgroup: threadsPerGroup];
            
        }

        void CommandList::Submit(const RHI::DispatchRaysItem& dispatchRaysItem)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
        }

        void CommandList::SetViewports(const RHI::Viewport* rhiViewports, uint32_t count)
        {
            m_state.m_viewportState.Set(AZStd::array_view<RHI::Viewport>(rhiViewports, count));
        }

        void CommandList::SetScissors(const RHI::Scissor* rhiScissors, uint32_t count)
        {
            m_state.m_scissorState.Set(AZStd::array_view<RHI::Scissor>(rhiScissors, count));
        }
    
        template <typename Item>
        void CommandList::SetRootConstants(const Item& item, const PipelineState* pipelineState)
        {
            const PipelineLayout& pipelineLayout = pipelineState->GetPipelineLayout();
            if (item.m_rootConstantSize  && pipelineLayout.GetRootConstantsSize() > 0)
            {
                if(m_commandEncoderType == CommandEncoderType::Render)
                {
                    id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                    
                    [renderEncoder setVertexBytes: item.m_rootConstants
                                           length: pipelineLayout.GetRootConstantsSize()
                                          atIndex: pipelineLayout.GetRootConstantsSlotIndex()];
                    
                    [renderEncoder setFragmentBytes: item.m_rootConstants
                                             length: pipelineLayout.GetRootConstantsSize()
                                            atIndex: pipelineLayout.GetRootConstantsSlotIndex()];
                    
                }
                else if(m_commandEncoderType == CommandEncoderType::Compute)
                {
                    id<MTLComputeCommandEncoder> computeEncoder = GetEncoder<id<MTLComputeCommandEncoder>>();
                    [computeEncoder setBytes: item.m_rootConstants
                                     length: pipelineLayout.GetRootConstantsSize()
                                    atIndex: pipelineLayout.GetRootConstantsSlotIndex()];
                    
                }
            }
        }
 
        bool CommandList::SetArgumentBuffers(const PipelineState* pipelineState, RHI::PipelineStateType stateType)
        {
            ShaderResourceBindings& bindings = GetShaderResourceBindingsByPipelineType(stateType);
            const PipelineLayout& pipelineLayout = pipelineState->GetPipelineLayout();
            
            for (uint32_t srgIndex = 0; srgIndex < RHI::Limits::Pipeline::ShaderResourceGroupCountMax; ++srgIndex)
            {
                const ShaderResourceGroup* shaderResourceGroup = bindings.m_srgsBySlot[srgIndex];
                uint32_t slotIndex = pipelineLayout.GetSlotByIndex(srgIndex);
                if(!shaderResourceGroup || slotIndex == RHI::Limits::Pipeline::ShaderResourceGroupCountMax)
                {
                    continue;
                }

                if (bindings.m_srgsByIndex[srgIndex] != shaderResourceGroup)
                {
                    bindings.m_srgsByIndex[srgIndex] = shaderResourceGroup;
                    auto& compiledArgBuffer = shaderResourceGroup->GetCompiledArgumentBuffer();
                    
                    id<MTLBuffer> argBuffer = compiledArgBuffer.GetArgEncoderBuffer();
                    size_t argBufferOffset = compiledArgBuffer.GetOffset();
                    
                    uint32_t srgVisIndex = pipelineLayout.GetSlotByIndex(shaderResourceGroup->GetBindingSlot());
                    const RHI::ShaderStageMask& srgVisInfo = pipelineLayout.GetSrgVisibility(srgVisIndex);
                    
                    if(srgVisInfo != RHI::ShaderStageMask::None)
                    {
                        const ShaderResourceGroupVisibility& srgResourcesVisInfo = pipelineLayout.GetSrgResourcesVisibility(srgVisIndex);
                        
                        //For graphics and compute encoder bind the argument buffer and
                        //make the resource resident for the duration of the work associated with the current scope
                        //and ensure that it's in a format compatible with the appropriate metal function.
                        if(m_commandEncoderType == CommandEncoderType::Render)
                        {
                            id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                            uint8_t numBitsSet = RHI::CountBitsSet(static_cast<uint64_t>(srgVisInfo));
                            if( numBitsSet > 1 || srgVisInfo == RHI::ShaderStageMask::Vertex)
                            {
                                [renderEncoder setVertexBuffer:argBuffer
                                                                offset:argBufferOffset
                                                                atIndex:slotIndex];
                            }
                            
                            if( numBitsSet > 1 || srgVisInfo == RHI::ShaderStageMask::Fragment)
                            {
                                [renderEncoder setFragmentBuffer:argBuffer
                                                            offset:argBufferOffset
                                                           atIndex:slotIndex];
                            }
                            shaderResourceGroup->AddUntrackedResourcesToEncoder(m_encoder, srgResourcesVisInfo);
                        }
                        else if(m_commandEncoderType == CommandEncoderType::Compute)
                        {
                            id<MTLComputeCommandEncoder> computeEncoder = GetEncoder<id<MTLComputeCommandEncoder>>();
                            [computeEncoder setBuffer:argBuffer
                                                 offset:argBufferOffset
                                                atIndex:pipelineLayout.GetSlotByIndex(srgIndex)];
                            shaderResourceGroup->AddUntrackedResourcesToEncoder(m_encoder, srgResourcesVisInfo);
                        }
                    }
                }
            }
                        
            return true;
        }
        
        void CommandList::Submit(const RHI::DrawItem& drawItem)
        {
            AZ_TRACE_METHOD();
            
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
                AZ_Assert(false, "Resource binding was unsuccessfully.");
                return;
            }
            
            SetStreamBuffers(drawItem.m_streamBufferViews, drawItem.m_streamBufferViewCount);
            SetStencilRef(drawItem.m_stencilRef);

            MTLPrimitiveType mtlPrimType = pipelineState->GetPipelineTopology();
            
            switch (drawItem.m_arguments.m_type)
            {
                case RHI::DrawType::Indexed:
                {
                    const RHI::DrawIndexed& indexed = drawItem.m_arguments.m_indexed;
       
                    const RHI::IndexBufferView& indexBuffDescriptor = *drawItem.m_indexBufferView;
                    AZ::HashValue64 indicesHash = indexBuffDescriptor.GetHash();
                    
                    m_state.m_indicesHash = indicesHash;
                    const Buffer * buff = static_cast<const Buffer*>(indexBuffDescriptor.GetBuffer());
                    id<MTLBuffer> mtlBuff = buff->GetMemoryView().GetGpuAddress<id<MTLBuffer>>();
                    MTLIndexType mtlIndexType = (indexBuffDescriptor.GetIndexFormat() == RHI::IndexFormat::Uint16) ?
                                                            MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
                    uint32_t indexTypeSize = 0;
                    GetIndexTypeSizeInBytes(mtlIndexType, indexTypeSize);
                    
                    uint32_t indexOffset = indexBuffDescriptor.GetByteOffset() + (indexed.m_indexOffset * indexTypeSize) + buff->GetMemoryView().GetOffset();
                    [renderEncoder drawIndexedPrimitives: mtlPrimType
                                                indexCount: indexed.m_indexCount
                                                indexType: mtlIndexType
                                                indexBuffer: mtlBuff
                                                indexBufferOffset: indexOffset
                                                instanceCount: indexed.m_instanceCount
                                                baseVertex: indexed.m_vertexOffset
                                                baseInstance: indexed.m_instanceOffset];
                    break;
                }
                
                case RHI::DrawType::Linear:
                {                    
                    const RHI::DrawLinear& linear = drawItem.m_arguments.m_linear;
                    [renderEncoder drawPrimitives: mtlPrimType
                                        vertexStart: linear.m_vertexOffset
                                        vertexCount: linear.m_vertexCount
                                      instanceCount: linear.m_instanceCount
                                       baseInstance: linear.m_instanceOffset];                     
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
                AZ_TRACE_METHOD();
                m_state.m_pipelineState = pipelineState;

                switch (pipelineState->GetType())
                {
                    case RHI::PipelineStateType::Draw:
                    {
                        id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                        SetRasterizerState(pipelineState->GetRasterizerState());
                        if(pipelineState->GetDepthStencilState())
                        {
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
                }

                const PipelineLayout& pipelineLayout = pipelineState->GetPipelineLayout();
                if (m_state.m_pipelineLayout != &pipelineLayout)
                {
                    m_state.m_pipelineLayout = &pipelineLayout;
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
        
        void CommandList::SetStreamBuffers(const RHI::StreamBufferView* streams, uint32_t count)
        {
            AZ::HashValue64 streamsHash = AZ::HashValue64{0};
            for (uint32_t i = 0; i < count; ++i)
            {
                streamsHash = AZ::TypeHash64(streamsHash, streams[i].GetHash());
            }
            
            if (streamsHash != m_state.m_streamsHash)
            {
                m_state.m_streamsHash = streamsHash;
                AZ_Assert(count <= METAL_MAX_ENTRIES_BUFFER_ARG_TABLE , "Slots needed cannot exceed METAL_MAX_ENTRIES_BUFFER_ARG_TABLE");
                for (uint32_t i = 0; i < count; ++i)
                {
                    if (streams[i].GetBuffer())
                    {
                        const Buffer * buff = static_cast<const Buffer*>(streams[i].GetBuffer());
                        id<MTLBuffer> mtlBuff = buff->GetMemoryView().GetGpuAddress<id<MTLBuffer>>();
                        uint32_t VBIndex = (METAL_MAX_ENTRIES_BUFFER_ARG_TABLE - 1) - i;
                        uint32_t offset = streams[i].GetByteOffset() + buff->GetMemoryView().GetOffset();
                        id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
                        [renderEncoder setVertexBuffer: mtlBuff offset: offset atIndex: VBIndex];
                    }
                }
            }
        }
        
        void CommandList::SetRasterizerState(const RasterizerState& rastState)
        {
            id<MTLRenderCommandEncoder> renderEncoder = GetEncoder<id<MTLRenderCommandEncoder>>();
            [renderEncoder setCullMode: rastState.m_cullMode];
            [renderEncoder setDepthBias: rastState.m_depthBias slopeScale: rastState.m_depthSlopeScale clamp: rastState.m_depthBiasClamp];
            [renderEncoder setFrontFacingWinding: rastState.m_frontFaceWinding];
            [renderEncoder setTriangleFillMode: rastState.m_triangleFillMode];
            [renderEncoder setDepthClipMode: rastState.m_depthClipMode];
        }
        
        void CommandList::SetShaderResourceGroupForDraw(const RHI::ShaderResourceGroup& shaderResourceGroup)
        {
            SetShaderResourceGroup<RHI::PipelineStateType::Draw>(static_cast<const ShaderResourceGroup*>(&shaderResourceGroup));
        }
        
        void CommandList::SetShaderResourceGroupForDispatch(const RHI::ShaderResourceGroup& shaderResourceGroup)
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
                AZ_Assert(false, "Pipeline state not provided");
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
            
            AZ_TRACE_METHOD();
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

        void CommandList::BuildBottomLevelAccelerationStructure(const RHI::RayTracingBlas& rayTracingBlas)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
        }

        void CommandList::BuildTopLevelAccelerationStructure(const RHI::RayTracingTlas& rayTracingTlas)
        {
            // [GFX TODO][ATOM-5268] Implement Metal Ray Tracing
            AZ_Assert(false, "Not implemented");
        }
    }
}
