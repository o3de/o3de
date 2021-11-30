/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/HphaSchema.h>

namespace AZ
{
    class LegacyAllocator
        : public SimpleSchemaAllocator<AZ::HphaSchema, AZ::HphaSchema::Descriptor>
    {
    public:
        AZ_TYPE_INFO(LegacyAllocator, "{17FC25A4-92D9-48C5-BB85-7F860FCA2C6F}");

        using Descriptor = AZ::HphaSchema::Descriptor;
        using Base = SimpleSchemaAllocator<AZ::HphaSchema, Descriptor>;
        using pointer_type = typename Base::pointer_type;
        using size_type = typename Base::size_type;
        using difference_type = typename Base::difference_type;
        
        LegacyAllocator()
            : Base("LegacyAllocator", "Allocator for Legacy CryEngine systems")
        {
        }

        pointer_type Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;

        // DeAllocate with file/line, to track when allocs were freed from Cry
        void DeAllocate(pointer_type ptr, const char* file, const int line, size_type byteSize = 0, size_type alignment = 0);

        // Realloc with file/line, because Cry uses realloc(nullptr) and realloc(ptr, 0) to mimic malloc/free
        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment, const char* file, const int line);

        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
    };

    using StdLegacyAllocator = AZStdAlloc<LegacyAllocator>;

    // Specialize for the LegacyAllocator to provide one per module that does not use the
    // environment for its storage
    template <>
    class AllocatorInstance<LegacyAllocator> : public Internal::AllocatorInstanceBase<LegacyAllocator>
    {
    };
}
