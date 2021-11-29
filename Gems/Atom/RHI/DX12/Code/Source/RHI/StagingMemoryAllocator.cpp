/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/StagingMemoryAllocator.h>
#include <RHI/Device.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace DX12
    {
        StagingMemoryAllocator::StagingMemoryAllocator()
            : m_mediumBlockAllocators{[this](MemoryLinearSubAllocator& subAllocator) { subAllocator.Init(m_mediumPageAllocator); }}
        {}

        void StagingMemoryAllocator::Init(const Descriptor& descriptor)
        {
            m_device = descriptor.m_device;

            {
                MemoryPageAllocator::Descriptor poolDesc;
                poolDesc.m_device = descriptor.m_device;
                poolDesc.m_pageSizeInBytes = descriptor.m_mediumPageSizeInBytes;
                poolDesc.m_collectLatency = descriptor.m_collectLatency;
                poolDesc.m_getHeapMemoryUsageFunction = [this]() { return &m_memoryUsage; };
                m_mediumPageAllocator.Init(poolDesc);
            }

            {
                MemoryPageAllocator::Descriptor poolDesc;
                poolDesc.m_device = descriptor.m_device;
                poolDesc.m_pageSizeInBytes = descriptor.m_largePageSizeInBytes;
                poolDesc.m_collectLatency = descriptor.m_collectLatency;
                poolDesc.m_getHeapMemoryUsageFunction = [this]() { return &m_memoryUsage; };
                m_largePageAllocator.Init(poolDesc);

                m_largeBlockAllocator.Init(m_largePageAllocator);
            }
        }

        void StagingMemoryAllocator::Shutdown()
        {
            m_mediumBlockAllocators.Clear();
            m_mediumPageAllocator.Shutdown();

            m_largeBlockAllocator.Shutdown();
            m_largePageAllocator.Shutdown();
        }

        void StagingMemoryAllocator::GarbageCollect()
        {
            AZ_PROFILE_SCOPE(RHI, "StagingMemoryAllocator: GarbageCollect(DX12)");
            m_mediumBlockAllocators.ForEach([](MemoryLinearSubAllocator& subAllocator)
            {
                subAllocator.GarbageCollect();
            });

            m_mediumPageAllocator.Collect();

            m_largeBlockMutex.lock();
            m_largeBlockAllocator.GarbageCollect();
            m_largeBlockMutex.unlock();

            m_largePageAllocator.Collect();
        }

        MemoryView StagingMemoryAllocator::Allocate(size_t sizeInBytes, size_t alignmentInBytes)
        {
            const size_t mediumPageSize = m_mediumPageAllocator.GetPageSize();
            const size_t mediumBlockThreshold = mediumPageSize / 2; /// Any larger than half the medium page should use the large page.

            if (sizeInBytes <= mediumBlockThreshold)
            {
                return MemoryView(m_mediumBlockAllocators.GetStorage().Allocate(sizeInBytes, alignmentInBytes), MemoryViewType::Buffer);
            }
            else if (sizeInBytes <= m_largePageAllocator.GetPageSize())
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_largeBlockMutex);
                return MemoryView(m_largeBlockAllocator.Allocate(sizeInBytes, alignmentInBytes), MemoryViewType::Buffer);
            }
            else
            {
                // Very large allocation. Make a committed resource.
                return AllocateUnique(sizeInBytes);
            }
        }

        void StagingMemoryAllocator::ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const
        {
            RHI::MemoryStatistics::Pool* poolStats = builder.BeginPool();

            if (builder.GetReportFlags() == RHI::MemoryStatisticsReportFlags::Detail)
            {
                for (size_t pageIdx = 0; pageIdx < m_mediumPageAllocator.GetPageCount(); ++pageIdx)
                {
                    RHI::MemoryStatistics::Buffer* bufferStats = builder.AddBuffer();
                    bufferStats->m_name = Name(AZStd::string::format("MediumStagingPage_%zu", pageIdx));
                    bufferStats->m_sizeInBytes = m_mediumPageAllocator.GetPageSize();
                }

                for (size_t pageIdx = 0; pageIdx < m_largePageAllocator.GetPageCount(); ++pageIdx)
                {
                    RHI::MemoryStatistics::Buffer* bufferStats = builder.AddBuffer();
                    bufferStats->m_name = Name(AZStd::string::format("LargeStagingPage_%zu", pageIdx));
                    bufferStats->m_sizeInBytes = m_largePageAllocator.GetPageSize();
                }
            }

            poolStats->m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Host) = m_memoryUsage;
            poolStats->m_name = Name("StagingMemory");
            builder.EndPool();
        }

        MemoryPageAllocator& StagingMemoryAllocator::GetMediumPageAllocator()
        {
            return m_mediumPageAllocator;
        }

        MemoryView StagingMemoryAllocator::AllocateUnique(size_t sizeInBytes)
        {
            AZ_TRACE_METHOD();
            RHI::BufferDescriptor descriptor;
            descriptor.m_byteCount = sizeInBytes;
            MemoryView memoryView = m_device->CreateBufferCommitted(descriptor, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
            memoryView.GetMemory()->SetName(L"Large Upload Buffer");

            // Queue the memory or deferred release immediately.
            m_device->QueueForRelease(memoryView);

            return memoryView;
        }
    }
}
