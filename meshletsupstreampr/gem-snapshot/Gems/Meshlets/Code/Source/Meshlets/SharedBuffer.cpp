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

        uint32_t SharedBuffer::ComputeAlignment(
            const AZStd::vector<SrgBufferDescriptor>& buffersDescriptors,
            uint32_t minAllowedAlignment)
        {
            uint32_t alignment = minAllowedAlignment;
            for (size_t i = 0; i < buffersDescriptors.size(); ++i)
            {
                // The least common multiple ensures every view's element size divides
                // the alignment evenly, so byte offsets within the shared buffer are
                // always element-multiples for any descriptor in the set.
                alignment = std::lcm(alignment, buffersDescriptors[i].m_elementSize);
            }
            alignment = AZStd::max(alignment, minAllowedAlignment);
            uint32_t remainder = alignment % minAllowedAlignment;
            if (remainder != 0)
            {
                alignment += minAllowedAlignment - remainder;
            }
            return alignment;
        }

        //! This method that ensures that the alignment over the various BufferViews is
        //! always kept, given the various possible buffer descriptors using the buffer.
        void SharedBuffer::CalculateAlignment(AZStd::vector<SrgBufferDescriptor>& buffersDescriptors)
        {
            m_alignment = ComputeAlignment(buffersDescriptors, MinAllowedAlignment);
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

            // Create the global buffer that holds all buffer views.
            //
            // After the vertex-pull migration the shared buffer is only ever read as a
            // shader resource (compute writes via UAV, render reads via SRV). The legacy
            // InputAssembly / Indirect bind flags were inherited from the POC's
            // index-buffer-via-attachment design and are no longer needed. Keeping IA
            // here was suspected of forcing the D3D12 backend into a heap/state
            // configuration that conflicted with the SRV usage, contributing to
            // DXGI_ERROR_DEVICE_HUNG. ShaderReadWrite alone covers everything we
            // actually do with the buffer.
            //
            // Remark: in order to enable indirect usage in the future, the file
            // BufferSystem.cpp must be changed to support a pool that supports this
            // type or else a buffer view validation test will fail. The change should
            // be done in 'BufferSystem::CreateCommonBufferPool'.
            SrgBufferDescriptor  sharedBufferDesc = SrgBufferDescriptor(
                RPI::CommonBufferPoolType::ReadWrite,
                RHI::Format::Unknown,
                RHI::BufferBindFlags::ShaderReadWrite,
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

