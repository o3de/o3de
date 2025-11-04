/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/IStreamerTypes.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ::IO::IStreamerTypes
{
    AZ::u64 Recommendations::CalculateRecommendedMemorySize(u64 readSize, u64 readOffset)
    {
        u64 offsetAdjustment = readOffset - AZ_SIZE_ALIGN_DOWN(readOffset, m_sizeAlignment);
        return AZ_SIZE_ALIGN_UP((readSize + offsetAdjustment), m_sizeAlignment);
    }

    DefaultRequestMemoryAllocator::DefaultRequestMemoryAllocator()
        : m_allocator(AZ::AllocatorInstance<AZ::SystemAllocator>::Get())
    {}

    DefaultRequestMemoryAllocator::DefaultRequestMemoryAllocator(AZ::IAllocator& allocator)
        : m_allocator(allocator)
    {}

    DefaultRequestMemoryAllocator::~DefaultRequestMemoryAllocator()
    {
        AZ_Assert(m_lockCounter == 0, "There are still %i file requests using this allocator.", GetNumLocks());
        AZ_Assert(m_allocationCounter == 0, "There are still %i allocations from this allocator.", static_cast<int>(m_allocationCounter));
    }

    void DefaultRequestMemoryAllocator::LockAllocator()
    {
        m_lockCounter++;
    }

    void DefaultRequestMemoryAllocator::UnlockAllocator()
    {
        m_lockCounter--;
    }

    RequestMemoryAllocatorResult DefaultRequestMemoryAllocator::Allocate([[maybe_unused]] u64 minimalSize, u64 recommendedSize, size_t alignment)
    {
        RequestMemoryAllocatorResult result;
        result.m_size = recommendedSize;
        result.m_type = MemoryType::ReadWrite;
        result.m_address = (recommendedSize > 0) ?
            m_allocator.Allocate(recommendedSize, alignment) :
            nullptr;

#ifdef AZ_ENABLE_TRACING
        if (result.m_address)
        {
            m_allocationCounter++;
        }
#endif

        return result;
    }

    void DefaultRequestMemoryAllocator::Release(void* address)
    {
        if (address)
        {
#ifdef AZ_ENABLE_TRACING
            m_allocationCounter--;
#endif

            m_allocator.DeAllocate(address);
        }
    }

    int DefaultRequestMemoryAllocator::GetNumLocks() const
    {
        return m_lockCounter;
    }
} // namespace AZ::IO::IStreamer
