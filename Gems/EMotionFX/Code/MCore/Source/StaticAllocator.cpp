/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <MCore/Source/StaticAllocator.h>
#include <AzCore/Memory/OSAllocator.h>

namespace MCore
{
    StaticAllocator::pointer_type StaticAllocator::allocate(size_type byteSize, size_type alignment, int flags)
    {
        AZ_UNUSED(flags);
        AZ::OSAllocator::pointer_type address = AZ_OS_MALLOC(byteSize, alignment);

        if (address == nullptr)
        {
            AZ_Error("Memory", false, "MCore::StaticAllocator ran out of system memory!\n");
        }

        return address;
    }

    void StaticAllocator::deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
    {
        AZ_UNUSED(byteSize);
        AZ_UNUSED(alignment);
        AZ_OS_FREE(ptr);
    }

    StaticAllocator::size_type StaticAllocator::resize(pointer_type ptr, size_type newSize)
    {
        AZ_UNUSED(ptr);
        AZ_UNUSED(newSize);
        return 0;
    }

    StaticAllocator::size_type StaticAllocator::max_size() const
    {
        return 0;
    }

    StaticAllocator::size_type StaticAllocator::get_allocated_size() const
    {
        return 0;
    }
} // end namespace MCore
