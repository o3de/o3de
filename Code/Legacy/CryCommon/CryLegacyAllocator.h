/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "LegacyAllocator.h"

//-----------------------------------------------------------------------------
// CryModule allocation API
//-----------------------------------------------------------------------------
#define CryModuleMalloc(size) CryModuleMallocImpl(size, __FILE__, __LINE__)

inline void* CryModuleMallocImpl(size_t size, const char* file, const int line)
{
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().Allocate(size, 0, 0, "LegacyAllocator malloc", file, line);
}

#define CryModuleFree(ptr) CryModuleFreeImpl(ptr, __FILE__, __LINE__)
#define CryModuleMemalignFree(ptr) CryModuleFreeImpl(ptr, __FILE__, __LINE__)

inline void CryModuleFreeImpl(void* ptr, const char* file, const int line)
{

    AZ::IAllocator& allocator = AZ::AllocatorInstance<AZ::LegacyAllocator>::GetAllocator();

    if (allocator.IsAllocationSourceChanged())
    {
        allocator.GetAllocationSource()->DeAllocate(ptr);
    }
    else
    {
        static_cast<AZ::LegacyAllocator&>(allocator).DeAllocate(ptr, file, line);
    }
}

#define CryModuleMemalign(size, alignment) CryModuleMemalignImpl(size, alignment, __FILE__, __LINE__)

inline void* CryModuleMemalignImpl(size_t size, size_t alignment, const char* file, const int line)
{
    return AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().Allocate(size, alignment, 0, "LegacyAllocator memalign", file, line);
}

#define CryModuleCalloc(num, size) CryModuleCallocImpl(num, size, __FILE__, __LINE__)

inline void* CryModuleCallocImpl(size_t num, size_t size, const char* file, const int line)
{
    void* ptr = AZ::AllocatorInstance<AZ::LegacyAllocator>::Get().Allocate(num * size, 0, 0, "LegacyAllocator calloc", file, line);
    ::memset(ptr, 0, num * size);
    return ptr;
}

#define CryModuleRealloc(ptr, size) CryModuleReallocAlignImpl(ptr, size, 0, __FILE__, __LINE__)
#define CryModuleReallocAlign(ptr, size, alignment) CryModuleReallocAlignImpl(ptr, size, alignment, __FILE__, __LINE__)

inline void* CryModuleReallocAlignImpl(void* prev, size_t size, size_t alignment, const char* file, const int line)
{
    if (!prev)
    {
        // map realloc(nullptr, ...) -> alloc() so that we can track the location of the initial alloc
        return CryModuleMemalignImpl(size, alignment, file, line);
    }
    if (size == 0)
    {
        CryModuleFreeImpl(prev, file, line);
        return nullptr;
    }
    // There should not be any code using CryRealloc during static-init time or before the allocators
// are initialized.
#if defined(AZ_MONOLITHIC_BUILD)
    if (!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady())
    {
        AZ_Assert(false, "CryRealloc/CryReallocAlign cannot be used unless the LegacyAllocator has been initialized");
        return nullptr;
    }
#endif

    AZ::IAllocator& allocator = AZ::AllocatorInstance<AZ::LegacyAllocator>::GetAllocator();
    void *ptr;

    if (allocator.IsAllocationSourceChanged())
    {
        ptr = allocator.GetAllocationSource()->ReAllocate(prev, size, 0);
    }
    else
    {
        ptr = static_cast<AZ::LegacyAllocator&>(allocator).ReAllocate(prev, size, 0, file, line);
    }

    return ptr;
}
