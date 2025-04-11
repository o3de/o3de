/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/SamplerState.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/ArgumentBuffer.h>
#include <RHI/BufferView.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<ArgumentBuffer> ArgumentBuffer::Create()
        {
            return aznew ArgumentBuffer();
        }

        void ArgumentBuffer::Init(Device* device,
                                  RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout,
                                  ShaderResourceGroupPool* srgPool)
        {
            @autoreleasepool
            {
                m_device = device;
                m_srgLayout = srgLayout;

                m_constantBufferSize = srgLayout->GetConstantDataSize();
                if (m_constantBufferSize)
                {
                    RHI::BufferDescriptor bufferDescriptor;
                    bufferDescriptor.m_byteCount = m_constantBufferSize;
                    bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::Constant;
                    AZStd::string constantBufferName = "ConstantBuffer";
#if defined(ARGUMENTBUFFER_PAGEALLOCATOR)
                    m_constantBuffer = device->GetArgBufferConstantBufferAllocator().Allocate(bufferDescriptor.m_byteCount);
#else
                    m_constantBuffer = device->CreateBufferCommitted(bufferDescriptor, RHI::HeapMemoryLevel::Host);
                    constantBufferName = AZStd::string::format("ConstantBuffer%s", srgPool->GetName().GetCStr());
                    m_constantBuffer.SetName(constantBufferName.c_str());
#endif
                    
                    AZ_Assert(m_constantBuffer.IsValid(), "Couldnt allocate memory for Constant buffer")
                }

                NSMutableArray* argBufferDecriptors = [[[NSMutableArray alloc] init] autorelease];
                bool argDescriptorsCreated = CreateArgumentDescriptors(argBufferDecriptors);

                if(argDescriptorsCreated)
                {
                    SetArgumentBuffer(argBufferDecriptors, AZStd::string::format("ArgumentBuffer_%s", srgPool->GetName().GetCStr()));

                    //Attach the static samplers
                    AttachStaticSamplers();

                    //Attach the constant buffer
                    AttachConstantBuffer();
                }
            }
        }

        void ArgumentBuffer::Init(Device* device,
                                  AZStd::vector<MTLArgumentDescriptor*> argBufferDescriptors,
                                  AZStd::string argBufferName)
        {
            @autoreleasepool
            {
                m_device = device;

                NSMutableArray* argBufferDecriptors = [[[NSMutableArray alloc] init] autorelease];
                for(MTLArgumentDescriptor* desc : argBufferDescriptors)
                {
                    [argBufferDecriptors addObject:desc];
                }
                
                SetArgumentBuffer(argBufferDecriptors, argBufferName);
            }
        }

        void ArgumentBuffer::SetArgumentBuffer(NSMutableArray* argBufferDecriptors, AZStd::string argBufferName)
        {
            NSSortDescriptor* sortDescriptor;
            sortDescriptor = [[[NSSortDescriptor alloc] initWithKey:@"index"
                                                         ascending:YES] autorelease];
            NSArray* sortedArgDescriptors = [argBufferDecriptors sortedArrayUsingDescriptors:@[sortDescriptor]];

            m_argumentEncoder = [m_device->GetMtlDevice() newArgumentEncoderWithArguments:sortedArgDescriptors];
            NSUInteger argumentBufferLength = m_argumentEncoder.encodedLength;

            RHI::BufferDescriptor bufferDescriptor;

            bufferDescriptor.m_byteCount = argumentBufferLength;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::Constant;
#if defined(ARGUMENTBUFFER_PAGEALLOCATOR)
            m_argumentBuffer = m_device->GetArgumentBufferAllocator().Allocate(bufferDescriptor.m_byteCount);
#else
            m_argumentBuffer = m_device->CreateBufferCommitted(bufferDescriptor, RHI::HeapMemoryLevel::Host);
            m_argumentBuffer.SetName(argBufferName.c_str());
#endif
            AZ_Assert(m_argumentBuffer.IsValid(), "Argument Buffer was not created");
            SetName(Name(argBufferName.c_str()));

            //Attach the argument buffer to the argument encoder
            [m_argumentEncoder setArgumentBuffer:m_argumentBuffer.GetGpuAddress<id<MTLBuffer>>()
                                                             offset:m_argumentBuffer.GetOffset()];
        }
    
        bool ArgumentBuffer::CreateArgumentDescriptors(NSMutableArray* argBufferDecriptors)
        {
            bool resourceAdded = false;

            
            for (const RHI::ShaderInputBufferDescriptor& shaderInputBuffer : m_srgLayout->GetShaderInputListForBuffers())
            {
                MTLArgumentDescriptor* bufferArgDescriptor = [[[MTLArgumentDescriptor alloc] init] autorelease];
                ConvertBufferArgumentDescriptor(bufferArgDescriptor, shaderInputBuffer);
                [argBufferDecriptors addObject:bufferArgDescriptor];
                resourceAdded = true;
            }

            for (const RHI::ShaderInputImageDescriptor& shaderInputImage : m_srgLayout->GetShaderInputListForImages())
            {
                // SubpassInputs do not use a texture in the SRG for Metal.
                if (shaderInputImage.m_type != RHI::ShaderInputImageType::SubpassInput)
                {
                    MTLArgumentDescriptor* imgArgDescriptor = [[[MTLArgumentDescriptor alloc] init] autorelease];
                    ConvertImageArgumentDescriptor(imgArgDescriptor, shaderInputImage);
                    [argBufferDecriptors addObject:imgArgDescriptor];
                    resourceAdded = true;
                }
            }

            for (const RHI::ShaderInputSamplerDescriptor& shaderInputSampler : m_srgLayout->GetShaderInputListForSamplers())
            {
                MTLArgumentDescriptor* samplerArgDescriptor = [[[MTLArgumentDescriptor alloc] init] autorelease];
                samplerArgDescriptor.dataType = MTLDataTypeSampler;
                samplerArgDescriptor.index = shaderInputSampler.m_registerId;
                samplerArgDescriptor.access = GetBindingAccess(RHI::ShaderInputImageAccess::Read);
                samplerArgDescriptor.arrayLength = shaderInputSampler.m_count > 1 ? shaderInputSampler.m_count:0;
                [argBufferDecriptors addObject:samplerArgDescriptor];
                resourceAdded = true;
            }

            for (const RHI::ShaderInputStaticSamplerDescriptor& staticSamplerInput : m_srgLayout->GetStaticSamplers())
            {
                MTLArgumentDescriptor* staticSamplerArgDescriptor = [[[MTLArgumentDescriptor alloc] init] autorelease];
                staticSamplerArgDescriptor.dataType = MTLDataTypeSampler;
                staticSamplerArgDescriptor.index = staticSamplerInput.m_registerId;
                staticSamplerArgDescriptor.access = GetBindingAccess(RHI::ShaderInputImageAccess::Read);
                [argBufferDecriptors addObject:staticSamplerArgDescriptor];
                resourceAdded = true;
            }

            AZStd::span<const RHI::ShaderInputConstantDescriptor> shaderInputConstantList = m_srgLayout->GetShaderInputListForConstants();
            if (!shaderInputConstantList.empty())
            {
                const RHI::ShaderInputConstantDescriptor& shaderInputConstant = shaderInputConstantList[0];
                MTLArgumentDescriptor* constBufferArgDescriptor = [[[MTLArgumentDescriptor alloc] init] autorelease];
                constBufferArgDescriptor.dataType = MTLDataTypePointer;
                constBufferArgDescriptor.index = shaderInputConstant.m_registerId;
                constBufferArgDescriptor.access = GetBindingAccess(RHI::ShaderInputImageAccess::Read);
                [argBufferDecriptors addObject:constBufferArgDescriptor];
                resourceAdded = true;
            }

            return resourceAdded;
        }

        void ArgumentBuffer::AttachStaticSamplers()
        {
            for (const RHI::ShaderInputStaticSamplerDescriptor& staticSampler : m_srgLayout->GetStaticSamplers())
            {
                MTLSamplerDescriptor* samplerDesc = [[[MTLSamplerDescriptor alloc] init] autorelease];
                ConvertSamplerState(staticSampler.m_samplerState, samplerDesc);
                id<MTLSamplerState> mtlSamplerState = GetMtlSampler(samplerDesc);
                [m_argumentEncoder setSamplerState:mtlSamplerState atIndex:staticSampler.m_registerId];
            }
        }

        void ArgumentBuffer::AttachConstantBuffer()
        {
            AZStd::span<const RHI::ShaderInputConstantDescriptor> shaderInputConstantList = m_srgLayout->GetShaderInputListForConstants();
            if (!shaderInputConstantList.empty())
            {
                const RHI::ShaderInputConstantDescriptor& shaderInputConstant = shaderInputConstantList[0];
                [m_argumentEncoder setBuffer:m_constantBuffer.GetGpuAddress<id<MTLBuffer>>() offset:m_constantBuffer.GetOffset() atIndex:shaderInputConstant.m_registerId];
            }
        }

        void ArgumentBuffer::BindNullSamplers(uint32_t registerId, uint32_t samplerCount)
        {
            AZStd::array<id<MTLSamplerState>, MaxEntriesInArgTable> mtlSamplers;
            NullDescriptorManager& nullDescriptorManager = m_device->GetNullDescriptorManager();
            id<MTLSamplerState> nullMtlSampler = nullDescriptorManager.GetNullSampler();
            for(int i = 0; i < samplerCount; i++)
            {
                mtlSamplers[i] = nullMtlSampler;
            }

            NSRange range = {registerId, samplerCount};
            [m_argumentEncoder setSamplerStates : mtlSamplers.data()
                               withRange  : range];
        }

        void ArgumentBuffer::UpdateImageViews(const RHI::ShaderInputImageDescriptor& shaderInputImage,
                                              const AZStd::span<const RHI::ConstPtr<RHI::DeviceImageView>>& imageViews)
        {
            if (shaderInputImage.m_type == RHI::ShaderInputImageType::SubpassInput)
            {
                // SubpassInputs don't need to update the argument buffer because they are not really a texture.
                return;
            }
            
            int imageArrayLen = 0;
            AZStd::array<id<MTLTexture>, MaxEntriesInArgTable> mtlTextures;
            
            m_resourceBindings[shaderInputImage.m_name].clear();
            for (const RHI::ConstPtr<RHI::DeviceImageView>& imageViewBase : imageViews)
            {
                if (imageViewBase && !imageViewBase->IsStale())
                {
                    const auto& imageView = static_cast<const ImageView&>(*imageViewBase);

                    RHI::Ptr<Memory> textureMemPtr = imageView.GetMemoryView().GetMemory();
                    mtlTextures[imageArrayLen] = textureMemPtr->GetGpuAddress<id<MTLTexture>>();
                    m_resourceBindings[shaderInputImage.m_name].insert(
                        ResourceBindingData{textureMemPtr->GetGpuAddress<id<MTLTexture>>(), textureMemPtr->GetResourceType(),
                                            .m_imageAccess = shaderInputImage.m_access});
                }
                else
                {
                    RHI::Ptr<Memory> nullMtlImagePtr = m_device->GetNullDescriptorManager().GetNullImage(shaderInputImage.m_type).GetMemory();
                    mtlTextures[imageArrayLen] = nullMtlImagePtr->GetGpuAddress<id<MTLTexture>>();
                    m_useNullDescriptorHeap = true;
                }
                imageArrayLen++;
            }

            AZ_Assert(imageArrayLen==shaderInputImage.m_count, "Make sure we have created the correct length of texture array");
            if(imageArrayLen > 0)
            {
                NSRange range = {shaderInputImage.m_registerId, imageArrayLen};
                [m_argumentEncoder setTextures : mtlTextures.data()
                                   withRange   : range];
            }
        }

        void ArgumentBuffer::UpdateSamplers(const RHI::ShaderInputSamplerDescriptor& shaderInputSampler,
                                            const AZStd::span<const RHI::SamplerState>& samplerStates)
        {
            int samplerArrayLen = 0;
            AZStd::array<id<MTLSamplerState>, MaxEntriesInArgTable> mtlSamplers;
            for (const RHI::SamplerState& samplerState : samplerStates)
            {
                MTLSamplerDescriptor* samplerDesc = [[[MTLSamplerDescriptor alloc] init] autorelease];
                ConvertSamplerState(samplerState, samplerDesc);

                mtlSamplers[samplerArrayLen] = GetMtlSampler(samplerDesc);
                samplerArrayLen++;
            }

            AZ_Assert(samplerArrayLen==shaderInputSampler.m_count, "Make sure we dont have a nil sampler within mtlSamplers");
            if(samplerArrayLen > 0)
            {
                NSRange range = {shaderInputSampler.m_registerId, samplerArrayLen};
                [m_argumentEncoder setSamplerStates : mtlSamplers.data()
                                   withRange  : range];
            }
            else
            {
                BindNullSamplers(shaderInputSampler.m_registerId, shaderInputSampler.m_count);
            }
        }

        void ArgumentBuffer::UpdateBufferViews(const RHI::ShaderInputBufferDescriptor& shaderInputBuffer,
                                               const AZStd::span<const RHI::ConstPtr<RHI::DeviceBufferView>>& bufferViews)
        {
            int bufferArrayLen = 0;
            AZStd::array<id<MTLBuffer>, MaxEntriesInArgTable> mtlBuffers;
            AZStd::array<NSUInteger, MaxEntriesInArgTable> mtlBufferOffsets;
            AZStd::array<id<MTLTexture>, MaxEntriesInArgTable> mtlTextures;

            m_resourceBindings[shaderInputBuffer.m_name].clear();
            for (const RHI::ConstPtr<RHI::DeviceBufferView>& bufferViewBase : bufferViews)
            {
                if (bufferViewBase && !bufferViewBase->IsStale())
                {
                    const auto& bufferView = static_cast<const BufferView&>(*bufferViewBase);
                    //Typed buffers (Buffer/RWBuffer) are represented as texture_buffer and as a result require texture view
                    if(shaderInputBuffer.m_type == RHI::ShaderInputBufferType::Typed)
                    {
                        RHI::Ptr<Memory> textureBufferMemPtr = bufferView.GetTextureBufferView().GetMemory();
                        AZ_Assert(textureBufferMemPtr, "This buffer does not have a texture view for texture_buffer");
                        mtlTextures[bufferArrayLen] = textureBufferMemPtr->GetGpuAddress<id<MTLTexture>>();
                        m_resourceBindings[shaderInputBuffer.m_name].insert(
                            ResourceBindingData{textureBufferMemPtr->GetGpuAddress<id<MTLTexture>>(), textureBufferMemPtr->GetResourceType(),
                                                .m_imageAccess = GetImageAccess(shaderInputBuffer.m_access)});
                    }
                    else
                    {
                        RHI::Ptr<Memory> bufferMemPtr = bufferView.GetMemoryView().GetMemory();
                        mtlBuffers[bufferArrayLen] = bufferMemPtr->GetGpuAddress<id<MTLBuffer>>();
                        mtlBufferOffsets[bufferArrayLen] = bufferView.GetMemoryView().GetOffset();
                        m_resourceBindings[shaderInputBuffer.m_name].insert(
                            ResourceBindingData{bufferMemPtr->GetGpuAddress<id<MTLBuffer>>(), bufferMemPtr->GetResourceType(),
                                                .m_bufferAccess = shaderInputBuffer.m_access});
                    }
                }
                else
                {
                    NullDescriptorManager& nullDescriptorManager = m_device->GetNullDescriptorManager();
                    if(shaderInputBuffer.m_type == RHI::ShaderInputBufferType::Typed)
                    {
                        RHI::Ptr<Memory> nullMtlBufferMemPtr = nullDescriptorManager.GetNullImageBuffer().GetMemory();
                        mtlTextures[bufferArrayLen] = nullMtlBufferMemPtr->GetGpuAddress<id<MTLTexture>>();
                        m_useNullDescriptorHeap = true;
                    }
                    else
                    {
                        RHI::Ptr<Memory> nullMtlBufferMemPtr = nullDescriptorManager.GetNullBuffer().GetMemory();
                        mtlBuffers[bufferArrayLen] = nullMtlBufferMemPtr->GetGpuAddress<id<MTLBuffer>>();
                        mtlBufferOffsets[bufferArrayLen] = nullDescriptorManager.GetNullBuffer().GetOffset();
                        m_resourceBindings[shaderInputBuffer.m_name].insert(
                            ResourceBindingData{nullMtlBufferMemPtr->GetGpuAddress<id<MTLBuffer>>(), nullMtlBufferMemPtr->GetResourceType(),
                                                .m_bufferAccess = shaderInputBuffer.m_access});
                    }
                }

                bufferArrayLen++;
            }

            AZ_Assert(bufferArrayLen==shaderInputBuffer.m_count, "Make sure we have created the correct length of buffer array");
            if(bufferArrayLen > 0)
            {
                NSRange range = {shaderInputBuffer.m_registerId, bufferArrayLen};

                if(shaderInputBuffer.m_type == RHI::ShaderInputBufferType::Typed)
                {
                    [m_argumentEncoder setTextures : mtlTextures.data()
                                           withRange   : range];
                }
                else
                {
                    [m_argumentEncoder setBuffers : mtlBuffers.data()
                                   offsets    : mtlBufferOffsets.data()
                                   withRange  : range];
                }
            }
        }

        void ArgumentBuffer::UpdateConstantBufferViews(AZStd::span<const uint8_t> rawData)
        {
            AZ_Assert(rawData.size() <= m_constantBufferSize, "rawData size can not be bigger than constant Buffer Size");
            if ( (m_constantBufferSize > 0) && (rawData.size() <= m_constantBufferSize))
            {
                memcpy(m_constantBuffer.GetCpuAddress(), rawData.data(), rawData.size());
            }
        }

        void ArgumentBuffer::Shutdown()
        {
            m_resourceBindings.clear();

#if defined(ARGUMENTBUFFER_PAGEALLOCATOR)
            if(m_constantBuffer.IsValid())
            {
                m_device->GetArgBufferConstantBufferAllocator().DeAllocate(m_constantBuffer);
            }
            if(m_argumentBuffer.IsValid())
            {
                m_device->GetArgumentBufferAllocator().DeAllocate(m_argumentBuffer);
            }
#else
            if(m_argumentBuffer.IsValid())
            {
                m_device->QueueForRelease(m_argumentBuffer);
            }

            if(m_constantBuffer.IsValid())
            {
                m_device->QueueForRelease(m_constantBuffer);
            }
#endif

            m_argumentBuffer = {};
            m_constantBuffer = {};

            [m_argumentEncoder release];
            m_argumentEncoder = nil;

            Base::Shutdown();
        }

        id<MTLBuffer> ArgumentBuffer::GetArgEncoderBuffer() const
        {
            return m_argumentBuffer.GetGpuAddress<id<MTLBuffer>>();
        };

        size_t ArgumentBuffer::GetOffset() const
        {
            return m_argumentBuffer.GetOffset();
        };

        id<MTLSamplerState> ArgumentBuffer::GetMtlSampler(MTLSamplerDescriptor* samplerDesc)
        {
            const NSCache* samplerCache = m_device->GetSamplerCache();
            id<MTLSamplerState> mtlSamplerState = [samplerCache objectForKey:samplerDesc];
            if(mtlSamplerState == nil)
            {
                mtlSamplerState = [m_device->GetMtlDevice() newSamplerStateWithDescriptor:samplerDesc];
                [samplerCache setObject:mtlSamplerState forKey:samplerDesc];
            }

            return mtlSamplerState;
        }

        void ArgumentBuffer::CollectUntrackedResources(const ShaderResourceGroupVisibility& srgResourcesVisInfo,
                                                       ResourcesForCompute& untrackedResourceComputeRead,
                                                       ResourcesForCompute& untrackedResourceComputeReadWrite) const
        {
            //Cache the constant buffer associated with a srg
            if (m_constantBufferSize &&
                static_cast<uint32_t>(srgResourcesVisInfo.m_constantDataStageMask) > 0)
            {
                id<MTLResource> mtlconstantBufferResource = m_constantBuffer.GetGpuAddress<id<MTLResource>>();
                if(RHI::CheckBitsAny(srgResourcesVisInfo.m_constantDataStageMask, RHI::ShaderStageMask::Compute))
                {
                    untrackedResourceComputeRead.insert(mtlconstantBufferResource);
                }
            }

            //Cach all the resources within a srg that are used by the shader based on the visibility information
            for (const auto& it : m_resourceBindings)
            {
                //Extract the visibility mask for the give resource
                auto visMaskIt = srgResourcesVisInfo.m_resourcesStageMask.find(it.first);
                AZ_Assert(visMaskIt != srgResourcesVisInfo.m_resourcesStageMask.end(), "No Visibility information available")

                //Only use this resource if it is used in one of the shaders
                if (static_cast<uint32_t>(visMaskIt->second) > 0 &&
                    RHI::CheckBitsAny(visMaskIt->second, RHI::ShaderStageMask::Compute))
                {
                    CollectResourcesForCompute(it.second, untrackedResourceComputeRead, untrackedResourceComputeReadWrite);
                }
            }
        }

        void ArgumentBuffer::CollectUntrackedResources(const ShaderResourceGroupVisibility& srgResourcesVisInfo,
                                                       ResourcesPerStageForGraphics& untrackedResourcesRead,
                                                       ResourcesPerStageForGraphics& untrackedResourcesReadWrite) const
        {
            //Cache the constant buffer associated with a srg
            if (m_constantBufferSize &&
                static_cast<uint32_t>(srgResourcesVisInfo.m_constantDataStageMask) > 0 &&
                !RHI::CheckBitsAny(srgResourcesVisInfo.m_constantDataStageMask, RHI::ShaderStageMask::Compute))
            {
                id<MTLResource> mtlconstantBufferResource = m_constantBuffer.GetGpuAddress<id<MTLResource>>();
                MTLRenderStages mtlRenderStages = GetRenderStages(srgResourcesVisInfo.m_constantDataStageMask);
                AddUntrackedResource(mtlRenderStages, mtlconstantBufferResource, untrackedResourcesRead);
            }

            //Cach all the resources within a srg that are used by the shader based on the visibility information
            for (const auto& it : m_resourceBindings)
            {
                //Extract the visibility mask for the give resource
                auto visMaskIt = srgResourcesVisInfo.m_resourcesStageMask.find(it.first);
                AZ_Assert(visMaskIt != srgResourcesVisInfo.m_resourcesStageMask.end(), "No Visibility information available")

                //Only use this resource if it is used in one of the shaders
                if (static_cast<uint32_t>(visMaskIt->second) > 0 &&
                    !RHI::CheckBitsAny(visMaskIt->second, RHI::ShaderStageMask::Compute))
                {
                    [[maybe_unused]] bool isBoundToGraphics = RHI::CheckBitsAny(visMaskIt->second, RHI::ShaderStageMask::Vertex) || RHI::CheckBitsAny(visMaskIt->second, RHI::ShaderStageMask::Fragment);
                    AZ_Assert(isBoundToGraphics, "The visibility mask %i is not set for Vertex or fragment stage", visMaskIt->second);
                    CollectResourcesForGraphics(visMaskIt->second, it.second, untrackedResourcesRead, untrackedResourcesReadWrite);
                }
            }
        }

        void ArgumentBuffer::CollectResourcesForCompute(const ResourceBindingsSet& resourceBindingDataSet,
                                                        ResourcesForCompute& untrackedResourceComputeRead,
                                                        ResourcesForCompute& untrackedResourceComputeReadWrite) const
        {
            for (const auto& resourceBindingData : resourceBindingDataSet)
            {
                MTLResourceUsage resourceUsage = MTLResourceUsageRead;
                switch(resourceBindingData.m_rescType)
                {
                    case ResourceType::MtlTextureType:
                    {
                        resourceUsage |= GetImageResourceUsage(resourceBindingData.m_imageAccess);
                        break;
                    }
                    case ResourceType::MtlBufferType:
                    {
                        resourceUsage |= GetBufferResourceUsage(resourceBindingData.m_bufferAccess);
                        break;
                    }
                    default:
                    {
                        AZ_Assert(false, "Undefined Resource type");
                    }
                }
                if(resourceUsage == MTLResourceUsageRead)
                {
                    untrackedResourceComputeRead.insert(resourceBindingData.m_resourcPtr);
                }
                else
                {
                    untrackedResourceComputeReadWrite.insert(resourceBindingData.m_resourcPtr);
                }
            }
        }

        void ArgumentBuffer::CollectResourcesForGraphics(RHI::ShaderStageMask visShaderMask,
                                                         const ResourceBindingsSet& resourceBindingDataSet,
                                                         ResourcesPerStageForGraphics& untrackedResourcesRead,
                                                         ResourcesPerStageForGraphics& untrackedResourcesReadWrite) const
        {
            MTLRenderStages mtlRenderStages = GetRenderStages(visShaderMask);
            MTLResourceUsage resourceUsage = MTLResourceUsageRead;
            for (const auto& resourceBindingData : resourceBindingDataSet)
            {
                switch(resourceBindingData.m_rescType)
                {
                    case ResourceType::MtlTextureType:
                    {
                        resourceUsage |= GetImageResourceUsage(resourceBindingData.m_imageAccess);
                        break;
                    }
                    case ResourceType::MtlBufferType:
                    {
                        resourceUsage |= GetBufferResourceUsage(resourceBindingData.m_bufferAccess);
                        break;
                    }
                    default:
                    {
                        AZ_Assert(false, "Undefined Resource type");
                    }
                }

                if(resourceUsage == MTLResourceUsageRead)
                {
                    AddUntrackedResource(mtlRenderStages, resourceBindingData.m_resourcPtr, untrackedResourcesRead);
                }
                else
                {
                    AddUntrackedResource(mtlRenderStages, resourceBindingData.m_resourcPtr, untrackedResourcesReadWrite);
                }
            }
        }

        void ArgumentBuffer::AddUntrackedResource(MTLRenderStages mtlRenderStages,
                                         id<MTLResource> resourcPtr,
                                         ResourcesPerStageForGraphics& resourceSet) const
        {
            if(mtlRenderStages & MTLRenderStageVertex)
            {
                resourceSet[RHI::ShaderStageVertex].insert(resourcPtr);
            }
            if(mtlRenderStages & MTLRenderStageFragment)
            {
                resourceSet[RHI::ShaderStageFragment].insert(resourcPtr);
            }
        }
    
        bool ArgumentBuffer::IsNullHeapNeededForVertexStage(const ShaderResourceGroupVisibility& srgResourcesVisInfo) const
        {
            bool isUsedByVertexStage = false;

            //Iterate over all the SRG entries
            for (const auto& it : srgResourcesVisInfo.m_resourcesStageMask)
            {
                //Only the ones not added to m_resourceBindings would require null heap
                if( m_resourceBindings.find(it.first) == m_resourceBindings.end())
                {
                    isUsedByVertexStage |= RHI::CheckBitsAny(it.second, RHI::ShaderStageMask::Vertex);
                }
            }
            return isUsedByVertexStage;
        }

        bool ArgumentBuffer::IsNullDescHeapNeeded() const
        {
            return m_useNullDescriptorHeap;
        }
    
        void ArgumentBuffer::UpdateTextureView(id <MTLTexture> mtlTexture, uint32_t index)
        {
            [m_argumentEncoder setTexture : mtlTexture
                                atIndex   : index];
        }
        
        void ArgumentBuffer::UpdateBufferView(id <MTLBuffer> mtlBuffer, uint32_t offset, uint32_t index)
        {
            [m_argumentEncoder setBuffer : mtlBuffer
                                  offset : offset
                                 atIndex : index];
        }
    
        const id<MTLArgumentEncoder> ArgumentBuffer::GetArgEncoder() const
        {
            return m_argumentEncoder;
        }
    }
}
