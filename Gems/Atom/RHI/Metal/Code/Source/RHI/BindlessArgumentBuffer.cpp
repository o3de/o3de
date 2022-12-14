/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/ArgumentBuffer.h>
#include <RHI/BindlessArgumentBuffer.h>
#include <RHI/Device.h>
#include <Metal/Metal.h>


namespace AZ::Metal
{
     // TODO(BINDLESS): Data drive this number
     // It's hardcoded here to match the unbounded array size (UNBOUNDED_SIZE) specified in AzslcHeader.azsli
     constexpr static uint32_t UnboundedArraySize = 100000;

    RHI::ResultCode BindlessArgumentBuffer::Init(Device* device)
    {
        m_device = device;
        m_unboundedArraySupported = device->GetFeatures().m_unboundedArrays;
        m_unboundedArraySupported = false; //Remove this when unbounded array support is added to spirv-cross
        
        AZStd::vector<id<MTLBuffer>> mtlArgBuffers;
        AZStd::vector<NSUInteger> mtlArgBufferOffsets;
        AZStd::vector<MTLArgumentDescriptor*> argBufferDescriptors;
        
        if(m_unboundedArraySupported)
        {
            //Create a root Argument buffer that will work as a container for ABs related unbouned arrays
            m_rootArgBuffer = ArgumentBuffer::Create();
            
            //Create an Argument buffer per resource type that has an unbounded array
            m_bindlessTextureArgBuffer = ArgumentBuffer::Create();
            m_bindlessRWTextureArgBuffer = ArgumentBuffer::Create();
            m_bindlessBufferArgBuffer = ArgumentBuffer::Create();
            m_bindlessRWBufferArgBuffer = ArgumentBuffer::Create();
            
            AZStd::vector<RHI::Ptr<ArgumentBuffer>> argBuffers;
            //Unbounded read only textures
            MTLArgumentDescriptor* argDescriptor = [[MTLArgumentDescriptor alloc] init];
            argDescriptor.dataType = MTLDataTypeTexture;
            argDescriptor.index = 0;
            argDescriptor.access = MTLArgumentAccessReadOnly;
            argDescriptor.textureType = MTLTextureType2D;
            argDescriptor.arrayLength = UnboundedArraySize;
            argBufferDescriptors.push_back(argDescriptor);
            m_bindlessTextureArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessROTextures");
            mtlArgBuffers.push_back(m_bindlessTextureArgBuffer->GetArgEncoderBuffer());
            mtlArgBufferOffsets.push_back(m_bindlessTextureArgBuffer->GetOffset());
                                        
            //Unbounded read write textures
            argDescriptor.access = MTLArgumentAccessReadWrite;
            argBufferDescriptors[0] = argDescriptor;
            m_bindlessRWTextureArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessRWTextures");
            mtlArgBuffers.push_back(m_bindlessRWTextureArgBuffer->GetArgEncoderBuffer());
            mtlArgBufferOffsets.push_back(m_bindlessRWTextureArgBuffer->GetOffset());
            
            //Unbounded read only buffers
            argDescriptor.dataType = MTLDataTypePointer;
            argDescriptor.access = MTLArgumentAccessReadOnly;
            argBufferDescriptors[0] = argDescriptor;
            m_bindlessBufferArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessROBuffers");
            mtlArgBuffers.push_back(m_bindlessBufferArgBuffer->GetArgEncoderBuffer());
            mtlArgBufferOffsets.push_back(m_bindlessBufferArgBuffer->GetOffset());
              
            //Unbounded read write buffers
            argDescriptor.access = MTLArgumentAccessReadWrite;
            argBufferDescriptors[0] = argDescriptor;
            m_bindlessRWBufferArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessRWTextures");
            mtlArgBuffers.push_back(m_bindlessRWBufferArgBuffer->GetArgEncoderBuffer());
            mtlArgBufferOffsets.push_back(m_bindlessRWBufferArgBuffer->GetOffset());
            
            //Container Argument buffer for all the unbounded arrays
            argDescriptor.dataType = MTLDataTypePointer;
            argDescriptor.index = 0;
            argDescriptor.access = MTLArgumentAccessReadOnly;
            argDescriptor.arrayLength = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::Count) - 1;
            argBufferDescriptors[0] = argDescriptor;
            m_rootArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessRoot");
            [argDescriptor release];
            argDescriptor = nil;
            
            NSRange range = {0, mtlArgBuffers.size()};
            [m_rootArgBuffer->GetArgEncoder()     setBuffers: mtlArgBuffers.data()
                                                     offsets: mtlArgBufferOffsets.data()
                                                   withRange: range];
        }
        else
        {
            m_boundedArgBuffer = ArgumentBuffer::Create();
            //For the bounded approach we have one AB hold all the bindless resource types
            for (uint32_t i = 0; i < static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::Count); ++i)
            {
                MTLArgumentDescriptor* textureArgDescriptor = [[MTLArgumentDescriptor alloc] init];
                textureArgDescriptor.index = UnboundedArraySize*i;
                textureArgDescriptor.arrayLength = UnboundedArraySize;
                textureArgDescriptor.dataType = MTLDataTypePointer;
                if(i == static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadTexture) ||
                   i == static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteTexture))
                {
                    textureArgDescriptor.dataType = MTLDataTypeTexture;
                    textureArgDescriptor.textureType = MTLTextureType2D;
                }
                textureArgDescriptor.access = MTLArgumentAccessReadOnly;
                if(i == static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteTexture) ||
                   i == static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteBuffer))
                {
                    textureArgDescriptor.access = MTLArgumentAccessReadWrite;
                }
                argBufferDescriptors.push_back(textureArgDescriptor);
            }
            
            m_boundedArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessSrg");
            
            for(MTLArgumentDescriptor* desc : argBufferDescriptors)
            {
                [desc release];
                desc = nil;
            }
        }
        
        //Init the free list allocator related to the unbounded arrays for each resource type
        for (uint32_t i = 0; i < static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::Count); ++i)
        {
            RHI::FreeListAllocator::Descriptor desc;
            desc.m_capacityInBytes = UnboundedArraySize;
            desc.m_alignmentInBytes = 1;
            desc.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;
            desc.m_policy = RHI::FreeListAllocatorPolicy::FirstFit;
            m_allocators[i].Init(desc);
        }

        return RHI::ResultCode::Success;
    }

    uint32_t BindlessArgumentBuffer::AttachReadImage(id <MTLTexture> mtlTexture)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        uint32_t allocatorIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadTexture);
        RHI::VirtualAddress address = m_allocators[allocatorIndex].Allocate(1, 1);
        AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
        uint32_t heapIndex = static_cast<uint32_t>(address.m_ptr);
        if (m_unboundedArraySupported)
        {
            m_bindlessTextureArgBuffer->UpdateTextureView(mtlTexture, heapIndex);
        }
        else
        {
            m_boundedArgBuffer->UpdateTextureView(mtlTexture, (heapIndex + (UnboundedArraySize * allocatorIndex)));
        }
        return heapIndex;
    }
    
    uint32_t BindlessArgumentBuffer::AttachReadWriteImage(id <MTLTexture> mtlTexture)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        uint32_t allocatorIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteTexture);
        RHI::VirtualAddress address = m_allocators[allocatorIndex].Allocate(1, 1);
        AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
        uint32_t heapIndex = static_cast<uint32_t>(address.m_ptr);
        if (m_unboundedArraySupported)
        {
            m_bindlessRWTextureArgBuffer->UpdateTextureView(mtlTexture, heapIndex);
        }
        else
        {
            m_boundedArgBuffer->UpdateTextureView(mtlTexture, (heapIndex + (UnboundedArraySize * allocatorIndex)));
        }
        return heapIndex;
    }
    
    uint32_t BindlessArgumentBuffer::AttachReadBuffer(id <MTLBuffer> mtlBuffer, uint32_t offset)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        uint32_t allocatorIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadBuffer);
        RHI::VirtualAddress address = m_allocators[allocatorIndex].Allocate(1, 1);
        AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
        uint32_t heapIndex = static_cast<uint32_t>(address.m_ptr);
        if (m_unboundedArraySupported)
        {
            m_bindlessBufferArgBuffer->UpdateBufferView(mtlBuffer, offset, heapIndex);
        }
        else
        {
            m_boundedArgBuffer->UpdateBufferView(mtlBuffer, offset, (heapIndex + (UnboundedArraySize * allocatorIndex)));
        }
        return heapIndex;
    }

    uint32_t BindlessArgumentBuffer::AttachReadWriteBuffer(id <MTLBuffer> mtlBuffer, uint32_t offset)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        uint32_t allocatorIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteBuffer);
        RHI::VirtualAddress address = m_allocators[allocatorIndex].Allocate(1, 1);
        AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
        uint32_t heapIndex = static_cast<uint32_t>(address.m_ptr);
        if (m_unboundedArraySupported)
        {
            m_bindlessRWBufferArgBuffer->UpdateBufferView(mtlBuffer, offset, heapIndex);
        }
        else
        {
            m_boundedArgBuffer->UpdateBufferView(mtlBuffer, offset, (heapIndex + (UnboundedArraySize * allocatorIndex)));
        }
        return heapIndex;
    }

    void BindlessArgumentBuffer::DetachReadImage(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        uint32_t allocatorIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadTexture);
        m_allocators[allocatorIndex].DeAllocate({ index });
    }

    void BindlessArgumentBuffer::DetachReadWriteImage(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        uint32_t allocatorIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteTexture);
        m_allocators[allocatorIndex].DeAllocate({ index });
    }

    void BindlessArgumentBuffer::DetachReadBuffer(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        uint32_t allocatorIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadBuffer);
        m_allocators[allocatorIndex].DeAllocate({ index });
    }

    void BindlessArgumentBuffer::DetachReadWriteBuffer(uint32_t index)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        uint32_t allocatorIndex = static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::ReadWriteBuffer);
        m_allocators[allocatorIndex].DeAllocate({ index });
    }

    RHI::Ptr<ArgumentBuffer> BindlessArgumentBuffer::GetBindlessArgbuffer() const
    {
        if(m_unboundedArraySupported)
        {
            return m_rootArgBuffer;
        }
        else
        {
            return m_boundedArgBuffer;
        }
    }

    void BindlessArgumentBuffer::BindBindlessArgumentBuffer(uint32_t slotIndex,
                                                    CommandEncoderType commandEncoderType,
                                                    CommandList::MetalArgumentBufferArray& mtlVertexArgBuffers,
                                                    CommandList::MetalArgumentBufferArrayOffsets& mtlVertexArgBufferOffsets,
                                                    CommandList::MetalArgumentBufferArray& mtlFragmentOrComputeArgBuffers,
                                                    CommandList::MetalArgumentBufferArrayOffsets& mtlFragmentOrComputeArgBufferOffsets,
                                                    uint32_t& bufferVertexRegisterIdMin,
                                                    uint32_t& bufferVertexRegisterIdMax,
                                                    uint32_t& bufferFragmentOrComputeRegisterIdMin,
                                                    uint32_t& bufferFragmentOrComputeRegisterIdMax)
    {
        if(commandEncoderType == CommandEncoderType::Render)
        {
            mtlVertexArgBuffers[slotIndex] = GetBindlessArgbuffer()->GetArgEncoderBuffer();
            mtlVertexArgBufferOffsets[slotIndex] = GetBindlessArgbuffer()->GetOffset();
            mtlFragmentOrComputeArgBuffers[slotIndex] = GetBindlessArgbuffer()->GetArgEncoderBuffer();
            mtlFragmentOrComputeArgBufferOffsets[slotIndex] = GetBindlessArgbuffer()->GetOffset();
            bufferVertexRegisterIdMin = AZStd::min(slotIndex, bufferVertexRegisterIdMin);
            bufferVertexRegisterIdMax = AZStd::max(slotIndex, bufferVertexRegisterIdMax);
        }
        else if(commandEncoderType == CommandEncoderType::Compute)
        {
            mtlFragmentOrComputeArgBuffers[slotIndex] = GetBindlessArgbuffer()->GetArgEncoderBuffer();
            mtlFragmentOrComputeArgBufferOffsets[slotIndex] = GetBindlessArgbuffer()->GetOffset();
        }
        bufferFragmentOrComputeRegisterIdMin = AZStd::min(slotIndex, bufferFragmentOrComputeRegisterIdMin);
        bufferFragmentOrComputeRegisterIdMax = AZStd::max(slotIndex, bufferFragmentOrComputeRegisterIdMax);
    }

    void BindlessArgumentBuffer::MakeBindlessArgumentBuffersResident(CommandEncoderType commandEncoderType,
                                                                     ArgumentBuffer::ResourcesPerStageForGraphics& untrackedResourcesGfxRead,
                                                                     ArgumentBuffer::ResourcesForCompute& untrackedResourceComputeRead)
    {
        //If unbounded arrays are supported (i.e we are using rootAB approach) make all the children ABs resident
        if(m_unboundedArraySupported)
        {
            if(commandEncoderType == CommandEncoderType::Render)
            {
                untrackedResourcesGfxRead[RHI::ShaderStageVertex].insert(m_bindlessTextureArgBuffer->GetArgEncoderBuffer());
                untrackedResourcesGfxRead[RHI::ShaderStageVertex].insert(m_bindlessRWTextureArgBuffer->GetArgEncoderBuffer());
                untrackedResourcesGfxRead[RHI::ShaderStageVertex].insert(m_bindlessBufferArgBuffer->GetArgEncoderBuffer());
                untrackedResourcesGfxRead[RHI::ShaderStageVertex].insert(m_bindlessRWBufferArgBuffer->GetArgEncoderBuffer());
                
                untrackedResourcesGfxRead[RHI::ShaderStageFragment].insert(m_bindlessTextureArgBuffer->GetArgEncoderBuffer());
                untrackedResourcesGfxRead[RHI::ShaderStageFragment].insert(m_bindlessRWTextureArgBuffer->GetArgEncoderBuffer());
                untrackedResourcesGfxRead[RHI::ShaderStageFragment].insert(m_bindlessBufferArgBuffer->GetArgEncoderBuffer());
                untrackedResourcesGfxRead[RHI::ShaderStageFragment].insert(m_bindlessRWBufferArgBuffer->GetArgEncoderBuffer());
            }
            else if(commandEncoderType == CommandEncoderType::Compute)
            {
                untrackedResourceComputeRead.insert(m_bindlessTextureArgBuffer->GetArgEncoderBuffer());
                untrackedResourceComputeRead.insert(m_bindlessRWTextureArgBuffer->GetArgEncoderBuffer());
                untrackedResourceComputeRead.insert(m_bindlessBufferArgBuffer->GetArgEncoderBuffer());
                untrackedResourceComputeRead.insert(m_bindlessRWBufferArgBuffer->GetArgEncoderBuffer());
            }
        }
    }

    void BindlessArgumentBuffer::GarbageCollect()
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(RHI::ShaderResourceGroupData::BindlessResourceType::Count); ++i)
        {
            m_allocators[i].GarbageCollect();
        }
    }
}

