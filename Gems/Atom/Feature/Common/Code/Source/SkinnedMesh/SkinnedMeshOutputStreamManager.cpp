/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshOutputStreamManager.h>

#include <AzCore/Console/IConsole.h>

#include <Atom/Feature/SkinnedMesh/SkinnedMeshVertexStreams.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshFeatureProcessorBus.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <numeric>

namespace AZ
{
    namespace Render
    {
        SkinnedMeshOutputStreamManager::SkinnedMeshOutputStreamManager()
        {
            Init();
        }

        SkinnedMeshOutputStreamManager::~SkinnedMeshOutputStreamManager()
        {
            m_bufferAsset = {};
        }

        void SkinnedMeshOutputStreamManager::CalculateAlignment()
        {
            m_alignment = 1;
            for (uint8_t outputStreamIndex = 0; outputStreamIndex < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams); ++outputStreamIndex)
            {
                // Using the least common multiple enables resource views to be typed and ensures they can get an offset in bytes that is a multiple of an element count
                const SkinnedMeshOutputVertexStreamInfo& outputStreamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(static_cast<SkinnedMeshOutputVertexStreams>(outputStreamIndex));
                m_alignment = std::lcm(m_alignment, outputStreamInfo.m_elementSize);
            }
        }

        void SkinnedMeshOutputStreamManager::CreateBufferAsset()
        {
            RHI::FreeListAllocator::Descriptor allocatorDescriptor;
            allocatorDescriptor.m_alignmentInBytes = m_alignment;
            allocatorDescriptor.m_capacityInBytes = m_sizeInBytes;
            allocatorDescriptor.m_policy = RHI::FreeListAllocatorPolicy::BestFit;
            allocatorDescriptor.m_garbageCollectLatency = 0;
            m_freeListAllocator.Init(allocatorDescriptor);

            // Create the actual buffer
            RPI::BufferAssetCreator creator;
            Uuid uuid = Uuid::CreateRandom();
            creator.Begin(uuid);
            creator.SetBufferName("SkinnedMeshOutputStream");

            creator.SetPoolAsset(SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamResourcePool());

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderReadWrite;
            bufferDescriptor.m_byteCount = m_sizeInBytes;
            bufferDescriptor.m_alignment = m_alignment;
            creator.SetBuffer(nullptr, 0, bufferDescriptor);

            RHI::BufferViewDescriptor viewDescriptor;
            viewDescriptor.m_elementFormat = RHI::Format::Unknown;
            viewDescriptor.m_elementSize = sizeof(float);
            viewDescriptor.m_elementCount = aznumeric_cast<uint32_t>(m_sizeInBytes) / viewDescriptor.m_elementSize;
            viewDescriptor.m_elementOffset = 0;
            creator.SetBufferViewDescriptor(viewDescriptor);

            creator.End(m_bufferAsset);
        }


        // default value of 256mb supports roughly 42 character instances at 100,000 vertices per character x 64 bytes per vertex (12 byte position + 12 byte previous frame position + 12 byte normal + 16 byte tangent + 12 byte bitangent)
        // This includes only the output of the skinning compute shader, not the input buffers or bone transforms
        AZ_CVAR(
            int,
            r_skinnedMeshInstanceMemoryPoolSize,
            256,
            nullptr,
            AZ::ConsoleFunctorFlags::NeedsReload,
            "The amount of memory in Mb available for all actor skinning data. Note that this must only be set once at application startup"
        );

        void SkinnedMeshOutputStreamManager::Init()
        {
        }

        void SkinnedMeshOutputStreamManager::EnsureInit()
        {
            if (!m_needsInit)
            {
                return;
            }
            m_needsInit = false;

            const AZ::u64 sizeInMb = r_skinnedMeshInstanceMemoryPoolSize;
            m_sizeInBytes = sizeInMb * (1024u * 1024u);

            CalculateAlignment();

            CreateBufferAsset();

            SystemTickBus::Handler::BusConnect();
        }

        AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation> SkinnedMeshOutputStreamManager::Allocate(size_t byteCount)
        {
            RHI::VirtualAddress result;
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_allocatorMutex);

                EnsureInit();
                result = m_freeListAllocator.Allocate(byteCount, m_alignment);
            }

            if (result.IsValid())
            {
                return aznew SkinnedMeshOutputStreamAllocation(result);
            }

            return nullptr;
        }

        void SkinnedMeshOutputStreamManager::DeAllocate(RHI::VirtualAddress allocation)
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

        void SkinnedMeshOutputStreamManager::DeAllocateNoSignal(RHI::VirtualAddress allocation)
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

        Data::Asset<RPI::BufferAsset> SkinnedMeshOutputStreamManager::GetBufferAsset()
        {
            EnsureInit();
            return m_bufferAsset;
        }

        Data::Instance<RPI::Buffer> SkinnedMeshOutputStreamManager::GetBuffer()
        {
            EnsureInit();
            if (!m_buffer)
            {
                m_buffer = RPI::Buffer::FindOrCreate(m_bufferAsset);
            }
            return m_buffer;
        }

        void SkinnedMeshOutputStreamManager::OnSystemTick()
        {
            GarbageCollect();
        }

        void SkinnedMeshOutputStreamManager::GarbageCollect()
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
                    SkinnedMeshOutputStreamNotificationBus::Broadcast(&SkinnedMeshOutputStreamNotificationBus::Events::OnSkinnedMeshOutputStreamMemoryAvailable);
                    m_broadcastMemoryAvailableEvent = false;
                }
            }
        }

    }// namespace Render
}// namespace AZ
