/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_OS_ALLOCATOR_H
#define AZCORE_OS_ALLOCATOR_H

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/allocator.h>

// OS Allocations macros AZ_OS_MALLOC/AZ_OS_FREE
#include <AzCore/Memory/OSAllocator_Platform.h>

namespace AZ
{
    /**
     * OS allocator should be used for direct OS allocations (C heap)
     * It's memory usage is NOT tracked. If you don't create this allocator, it will be implicitly
     * created by the SystemAllocator when it is needed. In addition this allocator is used for
     * debug data (like memory tracking, etc.)
     */
    class OSAllocator
        : public AllocatorBase
    {
    public:
        AZ_RTTI(OSAllocator, "{9F835EE3-F23C-454E-B4E3-011E2F3C8118}", AllocatorBase)

        OSAllocator();
        OSAllocator(const OSAllocator&) = delete;
        OSAllocator(OSAllocator&&) = delete;
        OSAllocator& operator=(const OSAllocator&) = delete;
        OSAllocator& operator=(OSAllocator&&) = delete;
        ~OSAllocator() override;

        bool Create();

        void Destroy() override;

        //////////////////////////////////////////////////////////////////////////
        // IAllocator
        AllocatorDebugConfig GetDebugConfig() override;

        //////////////////////////////////////////////////////////////////////////
        // IAllocator
        pointer    allocate(size_type byteSize, size_type alignment) override;
        void            deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer         reallocate(pointer ptr, size_type newSize, align_type newAlignment) override;
        size_type       get_allocated_size(pointer ptr, align_type alignment = 1) const override;

        size_type       NumAllocatedBytes() const override       { return m_numAllocatedBytes; }

    protected:
        AZStd::atomic<size_type> m_numAllocatedBytes = 0;
    };

    typedef AZStdAlloc<OSAllocator> OSStdAllocator;
}

#endif // AZCORE_OS_ALLOCATOR_H
#pragma once


