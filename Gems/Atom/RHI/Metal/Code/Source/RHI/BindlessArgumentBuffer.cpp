/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/ArgumentBuffer.h>
#include <RHI/BindlessArgumentBuffer.h>
#include <RHI/BufferView.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>
#include <Metal/Metal.h>


namespace AZ::Metal
{
    RHI::ResultCode BindlessArgumentBuffer::Init(Device* device, const AZ::RHI::BindlessSrgDescriptor& bindlessSrgDesc)
    {
        m_device = device;
        m_bindlessSrgDesc = bindlessSrgDesc;
        m_unboundedArraySupported = device->GetFeatures().m_unboundedArrays;
        m_unboundedArraySimulated = device->GetFeatures().m_simulateBindlessUA;

        if (!m_unboundedArraySupported && !m_unboundedArraySimulated)
        {
            return RHI::ResultCode::Success;
        }

        AZStd::vector<id<MTLBuffer>> mtlArgBuffers;
        AZStd::vector<NSUInteger> mtlArgBufferOffsets;
        AZStd::vector<MTLArgumentDescriptor*> argBufferDescriptors;

        if (m_unboundedArraySupported)
        {
            //Create a root Argument buffer that will work as a container for ABs related unbounded arrays
            m_rootArgBuffer = ArgumentBuffer::Create();

            //Create an Argument buffer per resource type that has an unbounded array
            m_bindlessTextureArgBuffer = ArgumentBuffer::Create();
            m_bindlessRWTextureArgBuffer = ArgumentBuffer::Create();
            m_bindlessBufferArgBuffer = ArgumentBuffer::Create();
            m_bindlessRWBufferArgBuffer = ArgumentBuffer::Create();

            AZStd::vector<RHI::Ptr<ArgumentBuffer>> argBuffers;
            //Unbounded read only textures
            MTLArgumentDescriptor* argDescriptor = [[MTLArgumentDescriptor alloc]init];
            argDescriptor.dataType = MTLDataTypeTexture;
            argDescriptor.index = 0;
            argDescriptor.access = GetBindingAccess(RHI::ShaderInputImageAccess::Read);
            argDescriptor.textureType = MTLTextureType2D;
            argDescriptor.arrayLength = RHI::Limits::Pipeline::UnboundedArraySize;
            argBufferDescriptors.push_back(argDescriptor);
            m_bindlessTextureArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessROTextures");
            mtlArgBuffers.push_back(m_bindlessTextureArgBuffer->GetArgEncoderBuffer());
            mtlArgBufferOffsets.push_back(m_bindlessTextureArgBuffer->GetOffset());

            //Unbounded read write textures
            argDescriptor.access = MTLBindingAccessReadWrite;
            argBufferDescriptors[0] = argDescriptor;
            m_bindlessRWTextureArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessRWTextures");
            mtlArgBuffers.push_back(m_bindlessRWTextureArgBuffer->GetArgEncoderBuffer());
            mtlArgBufferOffsets.push_back(m_bindlessRWTextureArgBuffer->GetOffset());
            
            //Unbounded read cube textures
            argDescriptor.access = GetBindingAccess(RHI::ShaderInputImageAccess::Read);
            argDescriptor.textureType = MTLTextureTypeCube;
            argBufferDescriptors[0] = argDescriptor;
            m_bindlessCubeTextureArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessCubeROTextures");
            mtlArgBuffers.push_back(m_bindlessCubeTextureArgBuffer->GetArgEncoderBuffer());
            mtlArgBufferOffsets.push_back(m_bindlessCubeTextureArgBuffer->GetOffset());

            //Unbounded read only buffers
            argDescriptor.dataType = MTLDataTypePointer;
            argDescriptor.access = GetBindingAccess(RHI::ShaderInputImageAccess::Read);
            argBufferDescriptors[0] = argDescriptor;
            m_bindlessBufferArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessROBuffers");
            mtlArgBuffers.push_back(m_bindlessBufferArgBuffer->GetArgEncoderBuffer());
            mtlArgBufferOffsets.push_back(m_bindlessBufferArgBuffer->GetOffset());

            //Unbounded read write buffers
            argDescriptor.access = MTLBindingAccessReadWrite;
            argBufferDescriptors[0] = argDescriptor;
            m_bindlessRWBufferArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessRWBuffers");
            mtlArgBuffers.push_back(m_bindlessRWBufferArgBuffer->GetArgEncoderBuffer());
            mtlArgBufferOffsets.push_back(m_bindlessRWBufferArgBuffer->GetOffset());
            
            //Container Argument buffer for all the unbounded arrays
            argDescriptor.dataType = MTLDataTypePointer;
            argDescriptor.index = 0;
            argDescriptor.access = GetBindingAccess(RHI::ShaderInputImageAccess::Read);
            argDescriptor.arrayLength = static_cast<uint32_t>(RHI::BindlessResourceType::Count) - 1;
            argBufferDescriptors[0] = argDescriptor;
            m_rootArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessRoot");
            [argDescriptor release] ;
            argDescriptor = nil;

            NSRange range = { 0, mtlArgBuffers.size() };
            [m_rootArgBuffer->GetArgEncoder()     setBuffers: mtlArgBuffers.data()
                                                  offsets:    mtlArgBufferOffsets.data()
                                                  withRange:  range];
        }
        else if (m_unboundedArraySimulated)
        {
            m_boundedArgBuffer = ArgumentBuffer::Create();
            //For the bounded approach we have one AB that holds all the bindless resource types
            for (uint32_t i = 0; i < static_cast<uint32_t>(RHI::BindlessResourceType::Count); ++i)
            {
                MTLArgumentDescriptor* resourceArgDescriptor = [[MTLArgumentDescriptor alloc] init];
                
                resourceArgDescriptor.arrayLength = RHI::Limits::Pipeline::UnboundedArraySize;
                resourceArgDescriptor.dataType = MTLDataTypeTexture;
                resourceArgDescriptor.access = GetBindingAccess(RHI::ShaderInputImageAccess::Read);

                if(i == m_bindlessSrgDesc.m_roTextureIndex)
                {
                    resourceArgDescriptor.index = RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_roTextureIndex;
                    resourceArgDescriptor.textureType = MTLTextureType2D;
                }
                else if(i == m_bindlessSrgDesc.m_rwTextureIndex)
                {
                    resourceArgDescriptor.index = RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_rwTextureIndex;
                    resourceArgDescriptor.textureType = MTLTextureType2D;
                    resourceArgDescriptor.access = MTLBindingAccessReadWrite;
                }
                else if(i == m_bindlessSrgDesc.m_roTextureCubeIndex)
                {
                    resourceArgDescriptor.index = RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_roTextureCubeIndex;
                    resourceArgDescriptor.textureType = MTLTextureTypeCube;
                }
                else if(i == m_bindlessSrgDesc.m_roBufferIndex)
                {
                    resourceArgDescriptor.index = RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_roBufferIndex;
                    resourceArgDescriptor.dataType = MTLDataTypePointer;
                }
                else if(i == m_bindlessSrgDesc.m_rwBufferIndex)
                {
                    resourceArgDescriptor.index = RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_rwBufferIndex;
                    resourceArgDescriptor.dataType = MTLDataTypePointer;
                    resourceArgDescriptor.access = MTLBindingAccessReadWrite;
                }
                argBufferDescriptors.push_back(resourceArgDescriptor);
            }

            m_boundedArgBuffer->Init(device, argBufferDescriptors, "ArgumentBuffer_BindlessSrg");

            for (MTLArgumentDescriptor* desc : argBufferDescriptors)
            {
                [desc release];
                desc = nil;
            }
        }

        //Init the free list allocator related to the unbounded arrays for each resource type
        for (uint32_t i = 0; i < static_cast<uint32_t>(RHI::BindlessResourceType::Count); ++i)
        {
            RHI::FreeListAllocator::Descriptor desc;
            desc.m_capacityInBytes = RHI::Limits::Pipeline::UnboundedArraySize;
            desc.m_alignmentInBytes = 1;
            desc.m_garbageCollectLatency = RHI::Limits::Device::FrameCountMax;
            desc.m_policy = RHI::FreeListAllocatorPolicy::FirstFit;
            m_allocators[i].Init(desc);
        }

        return RHI::ResultCode::Success;
    }

    uint32_t BindlessArgumentBuffer::AttachReadImage(ImageView& imageView)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = imageView.GetBindlessReadIndex();
        // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
        if (heapIndex == ImageView::InvalidBindlessIndex)
        {
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_roTextureIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }

        uint32_t resourceTypeHeapIndex = heapIndex + (RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_roTextureIndex);
        MemoryView& memoryView = imageView.GetMemoryView();
        id <MTLTexture> mtlTexture = memoryView.GetGpuAddress<id<MTLTexture>>();

        if (m_unboundedArraySupported)
        {
            m_bindlessTextureArgBuffer->UpdateTextureView(mtlTexture, heapIndex);
        }
        else
        {
            m_boundedArgBuffer->UpdateTextureView(mtlTexture, resourceTypeHeapIndex);
        }
        return heapIndex;
    }

    uint32_t BindlessArgumentBuffer::AttachReadCubeMapImage(ImageView& imageView)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = imageView.GetBindlessReadIndex();
        // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
        if (heapIndex == ImageView::InvalidBindlessIndex)
        {
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_roTextureCubeIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }

        uint32_t resourceTypeHeapIndex = heapIndex + (RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_roTextureCubeIndex);
        MemoryView& memoryView = imageView.GetMemoryView();
        id <MTLTexture> mtlTexture = memoryView.GetGpuAddress<id<MTLTexture>>();

        if (m_unboundedArraySupported)
        {
            m_bindlessCubeTextureArgBuffer->UpdateTextureView(mtlTexture, heapIndex);
        }
        else
        {
            m_boundedArgBuffer->UpdateTextureView(mtlTexture, resourceTypeHeapIndex);
        }
        return heapIndex;
    }

    uint32_t BindlessArgumentBuffer::AttachReadWriteImage(ImageView& imageView)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = imageView.GetBindlessReadWriteIndex();
        // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
        if (heapIndex == ImageView::InvalidBindlessIndex)
        {
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_rwTextureIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }
        
        uint32_t resourceTypeHeapIndex = heapIndex + (RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_rwTextureIndex);
        MemoryView& memoryView = imageView.GetMemoryView();
        id <MTLTexture> mtlTexture = memoryView.GetGpuAddress<id<MTLTexture>>();

        if (m_unboundedArraySupported)
        {
            m_bindlessRWTextureArgBuffer->UpdateTextureView(mtlTexture, heapIndex);
        }
        else
        {
            m_boundedArgBuffer->UpdateTextureView(mtlTexture, resourceTypeHeapIndex);
        }
        return heapIndex;
    }

    uint32_t BindlessArgumentBuffer::AttachReadBuffer(BufferView& bufferView)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = bufferView.GetBindlessReadIndex();
        // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
        if (heapIndex == BufferView::InvalidBindlessIndex)
        {
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_roBufferIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }
        
        uint32_t resourceTypeHeapIndex = heapIndex + (RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_roBufferIndex);
        MemoryView& memoryView = bufferView.GetMemoryView();
        id <MTLBuffer> mtlBuffer = memoryView.GetGpuAddress<id<MTLBuffer>>();
        uint32_t offset = static_cast<uint32_t>(memoryView.GetOffset());

        if (m_unboundedArraySupported)
        {
            m_bindlessBufferArgBuffer->UpdateBufferView(mtlBuffer, offset, heapIndex);
        }
        else
        {
            m_boundedArgBuffer->UpdateBufferView(mtlBuffer, offset, resourceTypeHeapIndex);
        }
        return heapIndex;
    }

    uint32_t BindlessArgumentBuffer::AttachReadWriteBuffer(BufferView& bufferView)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

        uint32_t heapIndex = bufferView.GetBindlessReadWriteIndex();
        // Only allocate a new index if the view doesn't already have one. This allows views to update in-place.
        if (heapIndex == BufferView::InvalidBindlessIndex)
        {
            RHI::VirtualAddress address = m_allocators[m_bindlessSrgDesc.m_rwBufferIndex].Allocate(1, 1);
            AZ_Assert(address.IsValid(), "Bindless allocator ran out of space.");
            heapIndex = static_cast<uint32_t>(address.m_ptr);
        }

        uint32_t resourceTypeHeapIndex = heapIndex + (RHI::Limits::Pipeline::UnboundedArraySize * m_bindlessSrgDesc.m_rwBufferIndex);
        MemoryView& memoryView = bufferView.GetMemoryView();
        id <MTLBuffer> mtlBuffer = memoryView.GetGpuAddress<id<MTLBuffer>>();
        uint32_t offset = static_cast<uint32_t>(memoryView.GetOffset());

        if (m_unboundedArraySupported)
        {
            m_bindlessRWBufferArgBuffer->UpdateBufferView(mtlBuffer, offset, heapIndex);
        }
        else
        {
            m_boundedArgBuffer->UpdateBufferView(mtlBuffer, offset, resourceTypeHeapIndex);
        }
        return heapIndex;
    }

    void BindlessArgumentBuffer::DetachReadImage(uint32_t heapIndex)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_roTextureIndex].DeAllocate({ heapIndex });
    }

    void BindlessArgumentBuffer::DetachReadCubeMapImage(uint32_t heapIndex)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_roTextureCubeIndex].DeAllocate({ heapIndex });
    }

    void BindlessArgumentBuffer::DetachReadWriteImage(uint32_t heapIndex)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_rwTextureIndex].DeAllocate({ heapIndex });
    }

    void BindlessArgumentBuffer::DetachReadBuffer(uint32_t heapIndex)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_roBufferIndex].DeAllocate({ heapIndex });
    }

    void BindlessArgumentBuffer::DetachReadWriteBuffer(uint32_t heapIndex)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
        m_allocators[m_bindlessSrgDesc.m_rwBufferIndex].DeAllocate({ heapIndex });
    }

    RHI::Ptr<ArgumentBuffer> BindlessArgumentBuffer::GetBindlessArgbuffer() const
    {
        if (m_unboundedArraySupported)
        {
            return m_rootArgBuffer;
        }
        else
        {
            return m_boundedArgBuffer;
        }
    }

    void BindlessArgumentBuffer::BindBindlessArgumentBuffer(
        uint32_t slotIndex,
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
        if (commandEncoderType == CommandEncoderType::Render)
        {
            mtlVertexArgBuffers[slotIndex] = GetBindlessArgbuffer()->GetArgEncoderBuffer();
            mtlVertexArgBufferOffsets[slotIndex] = GetBindlessArgbuffer()->GetOffset();
            mtlFragmentOrComputeArgBuffers[slotIndex] = GetBindlessArgbuffer()->GetArgEncoderBuffer();
            mtlFragmentOrComputeArgBufferOffsets[slotIndex] = GetBindlessArgbuffer()->GetOffset();
            bufferVertexRegisterIdMin = AZStd::min(slotIndex, bufferVertexRegisterIdMin);
            bufferVertexRegisterIdMax = AZStd::max(slotIndex, bufferVertexRegisterIdMax);
        }
        else if (commandEncoderType == CommandEncoderType::Compute)
        {
            mtlFragmentOrComputeArgBuffers[slotIndex] = GetBindlessArgbuffer()->GetArgEncoderBuffer();
            mtlFragmentOrComputeArgBufferOffsets[slotIndex] = GetBindlessArgbuffer()->GetOffset();
        }
        bufferFragmentOrComputeRegisterIdMin = AZStd::min(slotIndex, bufferFragmentOrComputeRegisterIdMin);
        bufferFragmentOrComputeRegisterIdMax = AZStd::max(slotIndex, bufferFragmentOrComputeRegisterIdMax);
    }

    void BindlessArgumentBuffer::MakeBindlessArgumentBuffersResident(
        CommandEncoderType commandEncoderType,
        ArgumentBuffer::ResourcesPerStageForGraphics& untrackedResourcesGfxRead,
        ArgumentBuffer::ResourcesForCompute& untrackedResourceComputeRead)
    {
        //If unbounded arrays are supported (i.e we are using rootAB approach) make all the children ABs resident
        if (m_unboundedArraySupported)
        {
            if (commandEncoderType == CommandEncoderType::Render)
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
            else if (commandEncoderType == CommandEncoderType::Compute)
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
        for (uint32_t i = 0; i < static_cast<uint32_t>(RHI::BindlessResourceType::Count); ++i)
        {
            m_allocators[i].GarbageCollect();
        }
    }

    uint32_t BindlessArgumentBuffer::GetBindlessSrgBindingSlot()
    {
        return m_bindlessSrgDesc.m_bindlesSrgBindingSlot;
    }

    bool BindlessArgumentBuffer::IsInitialized() const
    {
        return m_rootArgBuffer || m_boundedArgBuffer;
    }
}

