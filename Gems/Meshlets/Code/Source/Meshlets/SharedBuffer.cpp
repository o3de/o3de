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

namespace AZ
{
    namespace Meshlets
    {
        //! When given null srg, the index handle is NOT set.
        //! Useful when creating a specific Srg buffer.
        void SharedBuffer::CreateSharedBuffer(SrgBufferDescriptor& bufferDesc)
        {
            // Descriptor setting
            RPI::CommonBufferDescriptor descriptor;

            descriptor.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
            descriptor.m_elementFormat = bufferDesc.m_elementFormat;
            descriptor.m_elementSize = bufferDesc.m_elementSize;
            descriptor.m_bufferName = bufferDesc.m_bufferName.GetCStr();
            descriptor.m_byteCount = (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize;
            descriptor.m_bufferData = nullptr;

            // The actual RPI shared buffer creation
            m_buffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(descriptor);
        }

        //--------------------------------------------------------------------------
        //! Setting the constructor as private will create compile error to remind the developer to set
        //! the buffer Init in the FeatureProcessor and initialize properly
        SharedBuffer::SharedBuffer()
        {
            AZ_Warning("SharedBuffer", false, "Missing information to properly create SharedBuffer.");

            Interface<SharedBufferInterface>::Register(this);
        }

        SharedBuffer::SharedBuffer(
            AZStd::string bufferName, uint32_t sharedBufferSize,
            AZStd::vector<SrgBufferDescriptor>& buffersDescriptors)
        {
            m_sizeInBytes = sharedBufferSize;
            m_bufferName = bufferName;
            Init(bufferName, buffersDescriptors);
        }

        SharedBuffer::~SharedBuffer()
        {
        }

        //! This method that ensures that the alignment over the various BufferViews is
        //! always kept, given the various possible buffer descriptors using the buffer.
        void SharedBuffer::CalculateAlignment(AZStd::vector<SrgBufferDescriptor>& buffersDescriptors)
        {
            m_alignment = MinAllowedAlignment;
            for (uint8_t bufferIndex = 0; bufferIndex < buffersDescriptors.size(); ++bufferIndex)
            {
                // Using the least common multiple enables resource views to be typed and ensures they can get
                // an offset in bytes that is a multiple of an element count
                m_alignment = std::lcm(m_alignment, buffersDescriptors[bufferIndex].m_elementSize);
            }
            m_alignment = AZStd::max(m_alignment, MinAllowedAlignment) +
                (MinAllowedAlignment - m_alignment % MinAllowedAlignment);
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
            AZStd::string sufferNameInShader = "m_" + bufferName;
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

            // Create the global buffer that holds all buffer views
            // Remark: in order to enable indirect usage, the file BufferSystem.cpp must
            // be changed to support a pool that supports this type or else a buffer view
            // validation test will fail.
            // The change should be done in 'BufferSystem::CreateCommonBufferPool'.
            SrgBufferDescriptor  sharedBufferDesc = SrgBufferDescriptor(
                RPI::CommonBufferPoolType::ReadWrite, 
                RHI::Format::Unknown,
                RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderReadWrite,
                sizeof(uint32_t), uint32_t(m_sizeInBytes / sizeof(uint32_t)),
                Name{ bufferName }, Name{ sufferNameInShader }, 0, 0, nullptr
            );

            // Use the following method to calculate alignment given a list of descriptors
            CalculateAlignment(buffersDescriptors);

            InitAllocator();

            CreateSharedBuffer(sharedBufferDesc);

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

        Data::Instance<RPI::Buffer> SharedBuffer::GetBuffer()
        {
            AZ_Assert(m_buffer, "SharedBuffer - the buffer doesn't exist yet");
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

    } // namespace Meshlets
} // namespace AZ

