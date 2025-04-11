/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class HphaSchema;

    /**
     * System allocator
     * The system allocator is the base allocator for
     * AZ memory lib. It is a singleton (like all other allocators), but
     * must be initialized first and destroyed last. All other allocators
     * will use them for internal allocations. This doesn't mean all other allocators
     * will be sub allocators, because we might have different memory system on consoles.
     * But the allocator utility system will use the system allocator.
     */
    class SystemAllocator
        : public AllocatorBase
    {
    public:
        AZ_TYPE_INFO_WITH_NAME_DECL(SystemAllocator);
        AZ_RTTI_NO_TYPE_INFO_DECL();

        SystemAllocator();
        ~SystemAllocator() override;

        bool Create();

        //////////////////////////////////////////////////////////////////////////
        // IAllocator
        AllocatorDebugConfig GetDebugConfig() override;

        //////////////////////////////////////////////////////////////////////////
        // IAllocator

        AllocateAddress allocate(size_type byteSize, size_type alignment) override;
        size_type       deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override;
        AllocateAddress reallocate(pointer ptr, size_type newSize, size_type newAlignment) override;
        size_type get_allocated_size(pointer ptr, size_type alignment) const override;
        void            GarbageCollect() override                 { m_subAllocator->GarbageCollect(); }

        size_type       NumAllocatedBytes() const override;

        //////////////////////////////////////////////////////////////////////////

    protected:
        SystemAllocator(const SystemAllocator&);
        SystemAllocator& operator=(const SystemAllocator&);

        AZStd::unique_ptr<IAllocator> m_subAllocator;
    };
}



