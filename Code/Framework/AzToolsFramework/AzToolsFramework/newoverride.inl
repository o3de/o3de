/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// overrides all new and delete and forwards them to the AZ allocator system
// for tracking purposes.

#include <AzCore/base.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/allocatorbase.h>
#include <AzCore/Memory/SystemAllocator.h>

void* operator new(std::size_t size, const AZ::Internal::AllocatorDummy*)
{
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ_Warning("MEMORY", false, "Memory is being allocated at static startup!");
        return malloc(size);
    }

    return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator aznew", 0, 0);
}
void* operator new[](std::size_t size, const AZ::Internal::AllocatorDummy*)
{
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ_Warning("MEMORY", false, "Memory is being allocated at static startup!");
        return malloc(size);
    }
    return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator aznew[]", 0, 0);
}
void* operator new(std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*)
{
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ_Warning("MEMORY", false, "Memory is being allocated at static startup!");
        return malloc(size);
    }
    return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, name ? name : "global operator aznew", fileName, lineNum);
}
void* operator new[](std::size_t size, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*)
{
    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ_Warning("MEMORY", false, "Memory is being allocated at static startup!");
        return malloc(size);
    }
    return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, name ? name : "global operator aznew[]", fileName, lineNum);
}

void* operator new(std::size_t size)
{
    if (size == 0)
    {
        size = 1;
    }

    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ_Warning("MEMORY", false, "Memory is being allocated at static startup!");
        return malloc(size);
    }
    return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator new", 0, 0);
}

//-----------------------------------
void* operator new[](std::size_t size)
//-----------------------------------
{
    if (size == 0)
    {
        size = 1;
    }

    if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ_Warning("MEMORY", false, "Memory is being allocated at static startup!");
        return _aligned_malloc(size, AZCORE_GLOBAL_NEW_ALIGNMENT);
    }

    return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator new[]", 0, 0);
}

//-----------------------------------
void* operator new(std::size_t size, std::nothrow_t const&)
//-----------------------------------
{
    return operator new(size);
}

//-----------------------------------
void* operator new[](std::size_t size, std::nothrow_t const&)
//-----------------------------------
{
    return operator new[](size);
}

// these deletes have to be created to match the new()
// and will only happen during exception handling when allocation fails.
void operator delete(void* ptr, const AZ::Internal::AllocatorDummy*)
{
    if (ptr == 0)
    {
        return;
    }
    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr);
}

void operator delete[](void* ptr, const AZ::Internal::AllocatorDummy*)
{
    if (ptr == 0)
    {
        return;
    }
    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr);
}

void operator delete(void* ptr, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*)
{
    (void)fileName;
    (void)lineNum;
    (void)name;
    if (ptr == 0)
    {
        return;
    }
    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr);
}

void operator delete[](void* ptr, const char* fileName, int lineNum, const char* name, const AZ::Internal::AllocatorDummy*)
{
    (void)fileName;
    (void)lineNum;
    (void)name;
    if (ptr == 0)
    {
        return;
    }
    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr);
}

void operator delete(void* ptr)
{
    if (ptr == 0)
    {
        return;
    }

    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr);
}

//-----------------------------------
void operator delete[](void* ptr)
//-----------------------------------
{
    if (ptr == 0)
    {
        return;
    }
    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr);
}

