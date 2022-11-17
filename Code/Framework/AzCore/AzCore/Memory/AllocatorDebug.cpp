/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorDebug.h>
#include <AzCore/Memory/OSAllocator_Platform.h>

namespace AZ::Debug
{
    DebugAllocator::pointer DebugAllocator::allocate(size_type byteSize, align_type alignment)
    {
        return AZ_OS_MALLOC(byteSize, alignment);
    }

    void DebugAllocator::deallocate(pointer ptr, [[maybe_unused]] size_type byteSize, [[maybe_unused]] align_type alignment)
    {
        AZ_OS_FREE(ptr);
    }

    DebugAllocator::pointer DebugAllocator::reallocate(pointer ptr, size_type newSize, align_type alignment)
    {
        return AZ_OS_REALLOC(ptr, newSize, alignment);
    }
}
