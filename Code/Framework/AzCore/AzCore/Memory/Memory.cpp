/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/parallel/containers/concurrent_fixed_unordered_set.h>

#include <AzCore/Memory/AllocatorManager.h>

AZ::AllocatorStorage::LazyAllocatorRef::~LazyAllocatorRef()
{
    m_destructor(*m_allocator);
}

void AZ::AllocatorStorage::LazyAllocatorRef::Init(size_t size, size_t alignment, CreationFn creationFn, DestructionFn destructionFn)
{
    m_allocator = AZ::AllocatorManager::CreateLazyAllocator(size, alignment, creationFn);
    m_destructor = destructionFn;
}
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

void* AZ::OperatorNew(std::size_t size, const char* fileName, int lineNum, const char* name)
{
    return AllocatorInstance<SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, name ? name : "global operator aznew", fileName, lineNum);
}

#if __cpp_aligned_new
// C++17 enables calling of the allocating operator new with the alignment to deal with over-aligned types(i.e types > std::max_align_t)
void* AZ::OperatorNew(std::size_t size, std::align_val_t align, const char* fileName, int lineNum, const char* name)
{
    return AllocatorInstance<SystemAllocator>::Get().Allocate(size, static_cast<size_t>(align), 0, name ? name : "global operator aznew", fileName, lineNum);
}
#endif

// General new operators
void* AZ::OperatorNew(std::size_t byteSize)
{
    return AllocatorInstance<SystemAllocator>::Get().Allocate(byteSize, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator aznew");
}

#if __cpp_aligned_new
//C++17 enables calling of the allocating operator new with the alignment to deal with over-aligned types(i.e types > std::max_align_t)
void* AZ::OperatorNew(std::size_t byteSize, std::align_val_t align)
{
    return AllocatorInstance<SystemAllocator>::Get().Allocate(byteSize, static_cast<size_t>(align), 0, "global operator aznew");
}
#endif

void AZ::OperatorDelete(void* ptr)
{
    AllocatorInstance<SystemAllocator>::Get().DeAllocate(ptr);
}

void AZ::OperatorDelete(void* ptr, std::size_t size)
{
    AllocatorInstance<SystemAllocator>::Get().DeAllocate(ptr, size);
}

#if __cpp_aligned_new
void AZ::OperatorDelete(void* ptr, std::size_t size, std::align_val_t align)
{
    AllocatorInstance<SystemAllocator>::Get().DeAllocate(ptr, size, static_cast<size_t>(align));
}
#endif

void* AZ::OperatorNewArray([[maybe_unused]] std::size_t size, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, const char*)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    return AllocatorInstance<SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator aznew[]", fileName, lineNum);
#else
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"
                    "new[] inserts a header (platform dependent) to keep track of the array size!\n"
                    "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
    return AZ_INVALID_POINTER;
#endif
}

#if __cpp_aligned_new
// C++17 enables calling of the allocating operator new with the alignment to deal with over-aligned types(i.e types > std::max_align_t)
void* AZ::OperatorNewArray([[maybe_unused]] std::size_t size, [[maybe_unused]] std::align_val_t align, [[maybe_unused]] const char* fileName, [[maybe_unused]] int lineNum, const char*)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    return AllocatorInstance<SystemAllocator>::Get().Allocate(size, static_cast<size_t>(align), 0, "global operator aznew[]", fileName, lineNum);
#else
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"
                    "new[] inserts a header (platform dependent) to keep track of the array size!\n"
                    "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
    return AZ_INVALID_POINTER;
#endif
}
#endif

void* AZ::OperatorNewArray([[maybe_unused]] std::size_t size)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    //AZ_Warning("Memory",false,"You should use aznew instead of new! It has better tracking and guarantees you will call the SystemAllocator always!");
    return AllocatorInstance<SystemAllocator>::Get().Allocate(size, AZCORE_GLOBAL_NEW_ALIGNMENT, 0, "global operator new[]", nullptr, 0);
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
    return AllocatorInstance<SystemAllocator>::Get().Allocate(size, static_cast<size_t>(align), 0, "global operator new[]", nullptr, 0);
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
    AllocatorInstance<SystemAllocator>::Get().DeAllocate(ptr);
#else
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"
                    "new[] inserts a header (platform dependent) to keep track of the array size!\n"
                    "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
#endif
}

void AZ::OperatorDeleteArray([[maybe_unused]] void* ptr, [[maybe_unused]] std::size_t size)
{
#if defined(AZ_NEW_DELETE_SUPPORT_ARRAYS)
    AllocatorInstance<SystemAllocator>::Get().DeAllocate(ptr, size);
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
    AllocatorInstance<SystemAllocator>::Get().DeAllocate(ptr, size, static_cast<size_t>(align));
#else
    AZ_Assert(false, "We DO NOT support array operators, because it's really hard/impossible to handle alignment without proper tracking!\n"
                    "new[] inserts a header (platform dependent) to keep track of the array size!\n"
                    "Use AZStd::vector,AZStd::array,AZStd::fixed_vector or placement new and it's your responsibility!");
#endif
}
#endif
