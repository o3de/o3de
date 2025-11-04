/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// New overloads
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// In monolithic builds, our new and delete become the global new and delete
// As such, we must support array new/delete
#if defined(AZ_MONOLITHIC_BUILD) || AZ_TRAIT_OS_MEMORY_ALWAYS_NEW_DELETE_SUPPORT_ARRAYS
#define AZ_NEW_DELETE_SUPPORT_ARRAYS
#endif

// General new operators
void* AZ::OperatorNew(std::size_t byteSize)
{
    return AllocatorInstance<SystemAllocator>::Get().allocate(byteSize, AZCORE_GLOBAL_NEW_ALIGNMENT);
}

#if __cpp_aligned_new
//C++17 enables calling of the allocating operator new with the alignment to deal with over-aligned types(i.e types > std::max_align_t)
void* AZ::OperatorNew(std::size_t byteSize, std::align_val_t align)
{
    return AllocatorInstance<SystemAllocator>::Get().allocate(byteSize, static_cast<size_t>(align));
}
#endif

void AZ::OperatorDelete(void* ptr)
{
    AllocatorInstance<SystemAllocator>::Get().deallocate(ptr);
}

void AZ::OperatorDelete(void* ptr, std::size_t size)
{
    AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, size);
}

#if __cpp_aligned_new
void AZ::OperatorDelete(void* ptr, std::size_t size, std::align_val_t align)
{
    AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, size, static_cast<size_t>(align));
}
#endif

void* AZ::OperatorNewArray([[maybe_unused]] std::size_t size)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    //AZ_Warning("Memory",false,"You should use aznew instead of new! It has better tracking and guarantees you will call the SystemAllocator always!");
    return AllocatorInstance<SystemAllocator>::Get().allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT);
#else
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"
                    "new[] inserts a header (platform dependent) to keep track of the array size!\n"
                    "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
    return AZ_INVALID_POINTER;
#endif
}

#if __cpp_aligned_new
// C++17 enables calling of the allocating operator new with the alignment to deal with over-aligned types(i.e types > std::max_align_t)
void* AZ::OperatorNewArray([[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t align)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    //AZ_Warning("Memory",false,"You should use aznew instead of new! It has better tracking and guarantees you will call the SystemAllocator always!");
    return AllocatorInstance<SystemAllocator>::Get().allocate(size, static_cast<size_t>(align));
#else
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"
                    "new[] inserts a header (platform dependent) to keep track of the array size!\n"
                    "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
    return AZ_INVALID_POINTER;
#endif
}
#endif

void AZ::OperatorDeleteArray([[maybe_unused]] void* ptr)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    AllocatorInstance<SystemAllocator>::Get().deallocate(ptr);
#else
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"
                    "new[] inserts a header (platform dependent) to keep track of the array size!\n"
                    "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
#endif
}

void AZ::OperatorDeleteArray([[maybe_unused]] void* ptr, [[maybe_unused]] std::size_t size)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, size);
#else
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"
                    "new[] inserts a header (platform dependent) to keep track of the array size!\n"
                    "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
#endif
}

#if __cpp_aligned_new
void AZ::OperatorDeleteArray([[maybe_unused]] void* ptr, [[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t align)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    AllocatorInstance<SystemAllocator>::Get().deallocate(ptr, size, static_cast<size_t>(align));
#else
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"
                    "new[] inserts a header (platform dependent) to keep track of the array size!\n"
                    "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
#endif
}
#endif
