/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/base.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Memory/IAllocator.h>

namespace AZ::Debug
{
    struct Magic32
    {
        static const AZ::u32 m_defValue = 0xfeedf00d;
        AZ_FORCE_INLINE Magic32()
        {
            m_value = (m_defValue ^ (AZ::u32)((AZStd::size_t)this));
        }
        AZ_FORCE_INLINE ~Magic32()
        {
            m_value = 0;
        }
        AZ_FORCE_INLINE bool Validate() const
        {
            return m_value == (m_defValue ^ (AZ::u32)((AZStd::size_t)this));
        }

    private:
        AZ::u32 m_value;
    };

    /**
     * The DebugAllocator is a wrapper to direct OS allocation, it doesnt have tracking and is meant
     * to be used by allocator structures to track memory allocations.
     * DebugAllocator SHOULD NOT be used through AllocatorInstance, but directly. DebugAllocator is
     * stateless and performs allocations directly to the OS, bypassing any tracking/allocator feature.
     * memory system framework
     */
    class DebugAllocator
    {
    public:
        AZ_ALLOCATOR_DEFAULT_TRAITS

        DebugAllocator() = default;
        virtual ~DebugAllocator() = default;

        pointer allocate(size_type byteSize, align_type alignment = 1);
        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0);
        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1);
    };

    AZ_FORCE_INLINE bool operator==(const DebugAllocator&, const DebugAllocator&)
    {
        return true;
    }

    AZ_FORCE_INLINE bool operator!=(const DebugAllocator&, const DebugAllocator&)
    {
        return false;
    }
}
