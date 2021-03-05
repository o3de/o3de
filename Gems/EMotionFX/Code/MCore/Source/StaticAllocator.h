/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        StaticAllocator::size_type get_max_size() const;

        StaticAllocator::size_type get_allocated_size() const;
    };
} // end namespace MCore
