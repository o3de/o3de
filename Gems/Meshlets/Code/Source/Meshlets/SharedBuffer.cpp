/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>

#include <SharedBuffer.h>

#include <numeric>

namespace AZ::Meshlets
{
    //! When given null srg, the index handle is NOT set.
    //! Useful when creating a specific Srg buffer.
    void SharedBuffer::CreateSharedBuffer()
    {
        // Create the global buffer that holds all buffer views
        SrgBufferDescriptor  bufferDesc = SrgBufferDescriptor(
            RHI::Format::R32_UINT,
            RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
            sizeof(uint32_t), uint32_t(m_sizeInBytes / sizeof(uint32_t)),
            Name{ "SharedBuffer" }, Name{ "m_GlobalSharedBuffer" }, 0, 0
        );

        // Descriptor setting
        RPI::CommonBufferDescriptor desc;
        desc.m_elementFormat = bufferDesc.m_elementFormat;
        desc.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
        desc.m_elementSize = bufferDesc.m_elementSize;
        desc.m_bufferName = bufferDesc.m_bufferName.GetCStr();
        desc.m_byteCount = (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize;
        desc.m_bufferData = nullptr;

        // Buffer creation
        m_buffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
    }

    //--------------------------------------------------------------------------
    //! Setting the constructor as private will create compile error to remind the developer to set
    //! the buffer Init in the FeatureProcessor and initialize properly
    SharedBuffer::SharedBuffer()
    {
        AZ_Warning("SharedBuffer", false, "Missing information to properly create SharedBuffer. Init is required");
    }

    SharedBuffer::SharedBuffer(AZStd::string bufferName, AZStd::vector<SrgBufferDescriptor>& buffersDescriptors)
    {
        m_bufferName = bufferName;
        Init(bufferName, buffersDescriptors);
    }

    SharedBuffer::~SharedBuffer()
    {
        m_bufferAsset = {};
    }

    //! Crucial method that will ensure that the alignment for the BufferViews is always kept.
    //! This is important when requesting a BufferView as the offset needs to be aligned according
    //!  to the element type of the buffer.
    void SharedBuffer::CalculateAlignment(AZStd::vector<SrgBufferDescriptor>& buffersDescriptors)
    {
        m_alignment = 1;
        for (uint8_t bufferIndex = 0; bufferIndex < buffersDescriptors.size() ; ++bufferIndex)
        {
            // Using the least common multiple enables resource views to be typed and ensures they can get
            // an offset in bytes that is a multiple of an element count
            m_alignment = std::lcm(m_alignment, buffersDescriptors[bufferIndex].m_elementSize);
        }
    }

    void SharedBuffer::InitAllocator()
    {
        RHI::FreeListAllocator::Descriptor allocatorDescriptor;
        allocatorDescriptor.m_alignmentInBytes = m_alignment;
        allocatorDescriptor.m_capacityInBytes = m_sizeInBytes;
        allocatorDescriptor.m_policy = RHI::FreeListAllocatorPolicy::BestFit;
        allocatorDescriptor.m_garbageCollectLatency = 0;
        m_freeListAllocator.Init(allocatorDescriptor);
    }

    void SharedBuffer::Init(AZStd::string bufferName, AZStd::vector<SrgBufferDescriptor>& buffersDescriptors)
    {
        m_bufferName = bufferName;
        // m_sizeInBytes = 256u * (1024u * 1024u);
        //
        // [To Do] replace this with max size request for allocation that can be given by the calling function
        // This has the following problems:
        //  1. The need to have this aggregated size in advance
        //  2. The size might grow dynamically between frames
        //  3. Due to having several stream buffers (position, tangent, structured), alignment padding
        //      size calculation must be added.
        // Requirement: the buffer already has an assert on allocation beyond the memory.  In the future it should
        // support greedy memory allocation when memory has reached its end.  This must not invalidate the buffer during
        // the current frame, hence allocation of second buffer, fence and a copy must take place.

        CalculateAlignment(buffersDescriptors);

        InitAllocator();

        CreateSharedBuffer();

        SystemTickBus::Handler::BusConnect();
    }

    AZStd::intrusive_ptr<SharedBufferAllocation> SharedBuffer::Allocate(size_t byteCount)
    {
        RHI::VirtualAddress result;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
            result = m_freeListAllocator.Allocate(byteCount, m_alignment);
        }

        if (result.IsValid())
        {
            return aznew SharedBufferAllocation(result);
        }

        return nullptr;
    }

    void SharedBuffer::DeAllocate(RHI::VirtualAddress allocation)
    {
        if (allocation.IsValid())
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                m_freeListAllocator.DeAllocate(allocation);
            }

            m_memoryWasFreed = true;
            m_broadcastMemoryAvailableEvent = true;
        }
    }

    void SharedBuffer::DeAllocateNoSignal(RHI::VirtualAddress allocation)
    {
        if (allocation.IsValid())
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                m_freeListAllocator.DeAllocate(allocation);
            }
            m_memoryWasFreed = true;
        }
    }

    Data::Asset<RPI::BufferAsset> SharedBuffer::GetBufferAsset() const
    {
        return m_bufferAsset;
    }

    Data::Instance<RPI::Buffer> SharedBuffer::GetBuffer()
    {
        if (!m_buffer)
        {
            m_buffer = RPI::Buffer::FindOrCreate(m_bufferAsset);
        }
        return m_buffer;
    }

    //! Update buffer's content with sourceData at an offset of bufferByteOffset
    bool SharedBuffer::UpdateData(const void* sourceData, uint64_t sourceDataSizeInBytes, uint64_t bufferByteOffset)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
        if (m_buffer.get())
        {
            return m_buffer->UpdateData(sourceData, sourceDataSizeInBytes, bufferByteOffset);
        }
        AZ_Assert(false, "SharedBuffer error in data allocation - the buffer doesn't exist yet");
        return false;
    }

    void SharedBuffer::OnSystemTick()
    {
        GarbageCollect();
    }

    void SharedBuffer::GarbageCollect()
    {
        if (m_memoryWasFreed)
        {
            m_memoryWasFreed = false;
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);
                m_freeListAllocator.GarbageCollect();
            }
            if (m_broadcastMemoryAvailableEvent)
            {
                SharedBufferNotificationBus::Broadcast(&SharedBufferNotificationBus::Events::OnSharedBufferMemoryAvailable);
                m_broadcastMemoryAvailableEvent = false;
            }
        }
    }

    //! Utility function to create a resource view of different type than the shared buffer data.
    //! Since this class is sub-buffer container, this method should be used after creating
    //!  a new allocation to be used as a sub-buffer.
    //! Notice the alignment required according to the element size - this might need
    RHI::BufferViewDescriptor SharedBuffer::CreateResourceViewWithDifferentFormat(
        uint32_t offsetInBytes, uint32_t elementCount, uint32_t elementSize,
        RHI::Format format, RHI::BufferBindFlags overrideBindFlags)
    {
        RHI::BufferViewDescriptor viewDescriptor;

        // In the following line I use the element size and not the size based of the
        // element format since in the more interesting case of structured buffer, the
        // size will result in an error.
        uint32_t elementOffset = offsetInBytes / elementSize;
        viewDescriptor.m_elementOffset = elementOffset;
        viewDescriptor.m_elementCount = elementCount;
        viewDescriptor.m_elementFormat = format;
        viewDescriptor.m_elementSize = elementSize;
        viewDescriptor.m_overrideBindFlags = overrideBindFlags;
        return viewDescriptor;
    }

} // namespace AZ::Render
