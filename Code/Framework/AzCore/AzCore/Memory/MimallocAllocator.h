/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/IAllocator.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/base.h>

namespace AZ
{
    class AZCORE_API MimallocSchema final
        : public IAllocator
    {
    public:
        AZ_RTTI(MimallocSchema, "{2C91A6EC-41E5-4711-9A4E-7B93A3A1EAA2}");
        /**
         * Description - a wrapper for mimalloc allocator
         */

        MimallocSchema();
        virtual ~MimallocSchema() = default;

        AllocateAddress allocate(size_type byteSize, size_type alignment) override;
        size_type deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override;
        AllocateAddress reallocate(pointer ptr, size_type newSize, size_type newAlignment) override;
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override;

        size_type NumAllocatedBytes() const override;

        /// Return unused memory to the OS. Don't call this unless you really need free memory, it is slow.
        void GarbageCollect() override;

    private:
        AZStd::atomic<size_type> m_allocatedBytes = 0;
    };
} // namespace AZ
