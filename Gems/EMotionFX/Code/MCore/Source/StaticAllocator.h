/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/allocator.h>

namespace MCore
{
    class StaticAllocator
        : public AZStd::allocator
    {
    public:
        StaticAllocator::pointer_type allocate(size_type byteSize, size_type alignment, int flags = 0);

        void deallocate(pointer_type ptr, size_type byteSize, size_type alignment);

        StaticAllocator::size_type resize(pointer_type ptr, size_type newSize);

        StaticAllocator::size_type max_size() const;

        StaticAllocator::size_type get_allocated_size() const;
    };
} // end namespace MCore
