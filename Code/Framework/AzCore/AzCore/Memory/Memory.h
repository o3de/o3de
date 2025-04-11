/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/Memory_fwd.h>
#include <AzCore/Memory/AllocatorWrapper.h>
#include <AzCore/Memory/Config.h>
#include <AzCore/Memory/IAllocator.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/aligned_storage.h>

#include <AzCore/std/typetraits/has_member_function.h>

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Memory/AllocatorInstance.h>

/**
 * By default AZCore doesn't overload operator new and delete. This is a no-no for middle-ware.
 * You are encouraged to do that in your executable. What you need to do is to pipe all allocation trough AZCore memory manager.
 * AZCore relies on \ref AZ_CLASS_ALLOCATOR to specify the class default allocator or on explicit
 * azcreate/azdestroy calls which provide the allocator. If you class doesn't not implement the
 * \ref AZ_CLASS_ALLOCATOR when you call a new/delete they will use the global operator new/delete. In addition
 * if you call aznew on a class without AZ_CLASS_ALLOCATOR you will need to implement new operator specific to
 * aznew call signature.
 * So in an exception free environment (AZLibs don't have exception support) you need to implement the following functions:
 *
 * void* operator new(std::size_t);
 * void* operator new[](std::size_t);
 * void operator delete(void*);
 * void operator delete[](void*);
 *
 * You can implement those functions anyway you like, or you can use the provided implementations for you! \ref Global New/Delete Operators
 * All allocations will happen using the AZ::SystemAllocator. Make sure you create it properly before any new calls.
 * If you use our default new implementation you should map the global functions like that:
 *
 * void* operator new(std::size_t size)         { return AZ::OperatorNew(size); }
 * void* operator new[](std::size_t size)       { return AZ::OperatorNewArray(size); }
 * void operator delete(void* ptr)              { AZ::OperatorDelete(ptr); }
 * void operator delete[](void* ptr)            { AZ::OperatorDeleteArray(ptr); }
 */
namespace AZ
{
    /**
    * Generic wrapper for binding allocator to an AZStd one.
    * \note AZStd allocators are one of the few differences from STD/STL.
    * It's very tedious to write a wrapper for STD too.
    */
    template <class Allocator>
    class AZStdAlloc
    {
    public:
        AZ_ALLOCATOR_DEFAULT_TRAITS

        AZ_FORCE_INLINE AZStdAlloc() = default;
        AZ_FORCE_INLINE AZStdAlloc(const AZStdAlloc& rhs) = default;
        AZ_FORCE_INLINE AZStdAlloc& operator=(const AZStdAlloc& rhs) = default;
        AZ_FORCE_INLINE pointer allocate(size_t byteSize, size_t alignment)
        {
            return AllocatorInstance<Allocator>::Get().allocate(byteSize, alignment);
        }
        AZ_FORCE_INLINE pointer reallocate(pointer ptr, size_type newSize, align_type newAlignment)
        {
            return AllocatorInstance<Allocator>::Get().Resize(ptr, newSize, newAlignment);
        }
        AZ_FORCE_INLINE void deallocate(pointer ptr, size_type byteSize, size_type alignment)
        {
            AllocatorInstance<Allocator>::Get().deallocate(ptr, byteSize, alignment);
        }
        size_type                   max_size() const            { return AllocatorInstance<Allocator>::Get().max_size(); }
        size_type                   get_allocated_size() const  { return AllocatorInstance<Allocator>::Get().get_allocated_size(); }

        AZ_FORCE_INLINE bool is_lock_free()                     { return AllocatorInstance<Allocator>::Get().is_lock_free(); }
        AZ_FORCE_INLINE bool is_stale_read_allowed()            { return AllocatorInstance<Allocator>::Get().is_stale_read_allowed(); }
        AZ_FORCE_INLINE bool is_delayed_recycling()             { return AllocatorInstance<Allocator>::Get().is_delayed_recycling(); }
    };

    AZ_TYPE_INFO_TEMPLATE(AZStdAlloc, "{42D0AA1E-3C6C-440E-ABF8-435931150470}", AZ_TYPE_INFO_CLASS);

    template<class Allocator>
    AZ_FORCE_INLINE bool operator==(const AZStdAlloc<Allocator>& a, const AZStdAlloc<Allocator>& b) { (void)a; (void)b; return true; } // always true since they use the same instance of AllocatorInstance<Allocator>
    template<class Allocator>
    AZ_FORCE_INLINE bool operator!=(const AZStdAlloc<Allocator>& a, const AZStdAlloc<Allocator>& b) { (void)a; (void)b; return false; } // always false since they use the same instance of AllocatorInstance<Allocator>

    /**
     * Generic wrapper for binding IAllocator interface allocator.
     * This is basically the same as \ref AZStdAlloc but it allows
     * you to remove the template parameter and set you interface on demand.
     * of course at a cost of a pointer.
     */
    class AZStdIAllocator
    {
    public:
        AZ_ALLOCATOR_DEFAULT_TRAITS

        AZ_FORCE_INLINE AZStdIAllocator(IAllocator* allocator)
            : m_allocator(allocator)
        {
            AZ_Assert(m_allocator != NULL, "You must provide a valid allocator!");
        }
        AZ_FORCE_INLINE AZStdIAllocator(const AZStdIAllocator& rhs)
            : m_allocator(rhs.m_allocator)
        {}
        AZ_FORCE_INLINE AZStdIAllocator& operator=(const AZStdIAllocator& rhs) { m_allocator = rhs.m_allocator; return *this; }
        AZ_FORCE_INLINE pointer allocate(size_t byteSize, size_t alignment)
        {
            return m_allocator->allocate(byteSize, alignment);
        }
        AZ_FORCE_INLINE pointer reallocate(pointer ptr, size_t newSize, align_type newAlignment = 1)
        {
            return m_allocator->reallocate(ptr, newSize, newAlignment);
        }
        AZ_FORCE_INLINE void deallocate(pointer ptr, size_t byteSize, size_t alignment)
        {
            m_allocator->deallocate(ptr, byteSize, alignment);
        }
        size_type                   max_size() const { return m_allocator->GetMaxContiguousAllocationSize(); }
        size_type                   get_allocated_size() const { return m_allocator->NumAllocatedBytes(); }

        AZ_FORCE_INLINE bool operator==(const AZStdIAllocator& rhs) const { return m_allocator == rhs.m_allocator; }
        AZ_FORCE_INLINE bool operator!=(const AZStdIAllocator& rhs) const { return m_allocator != rhs.m_allocator; }
    private:
        IAllocator* m_allocator;
    };

    /**
    * Generic wrapper for binding IAllocator interface allocator.
    * This is basically the same as \ref AZStdAlloc but it allows
    * you to remove the template parameter and retrieve the allocator from a supplied function
    * pointer
    */
    class AZStdFunctorAllocator
    {
    public:
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;
        using functor_type = IAllocator&(*)(); ///< Function Pointer must return IAllocator&.
                                               ///< function pointers do not support covariant return types

        constexpr AZStdFunctorAllocator(functor_type allocatorFunctor)
            : m_allocatorFunctor(allocatorFunctor)
        {
        }
        constexpr AZStdFunctorAllocator(const AZStdFunctorAllocator& rhs) = default;
        constexpr AZStdFunctorAllocator& operator=(const AZStdFunctorAllocator& rhs) = default;
        pointer allocate(size_t byteSize, size_t alignment)
        {
            return m_allocatorFunctor().allocate(byteSize, alignment);
        }
        pointer reallocate(pointer ptr, size_t newSize, size_t newAlignment = 1)
        {
            return m_allocatorFunctor().reallocate(ptr, newSize, newAlignment);
        }
        void deallocate(pointer ptr, size_t byteSize, size_t alignment)
        {
            m_allocatorFunctor().deallocate(ptr, byteSize, alignment);
        }
        size_type max_size() const { return m_allocatorFunctor().GetMaxContiguousAllocationSize(); }
        size_type get_allocated_size() const { return m_allocatorFunctor().NumAllocatedBytes(); }

        constexpr bool operator==(const AZStdFunctorAllocator& rhs) const { return m_allocatorFunctor == rhs.m_allocatorFunctor; }
        constexpr bool operator!=(const AZStdFunctorAllocator& rhs) const { return m_allocatorFunctor != rhs.m_allocatorFunctor; }
    private:
        functor_type m_allocatorFunctor;
    };

    // {@ Global New/Delete Operators
    [[nodiscard]] void* OperatorNew(std::size_t size);
    void OperatorDelete(void* ptr);
    void OperatorDelete(void* ptr, std::size_t size);

    [[nodiscard]] void* OperatorNewArray(std::size_t size);
    void OperatorDeleteArray(void* ptr);
    void OperatorDeleteArray(void* ptr, std::size_t size);

#if __cpp_aligned_new
    [[nodiscard]] void* OperatorNew(std::size_t size, std::align_val_t align);
    [[nodiscard]] void* OperatorNewArray(std::size_t size, std::align_val_t align);
    void OperatorDelete(void* ptr, std::size_t size, std::align_val_t align);
    void OperatorDeleteArray(void* ptr, std::size_t size, std::align_val_t align);
#endif
    // @}
}

#define AZ_PAGE_SIZE AZ_TRAIT_OS_DEFAULT_PAGE_SIZE
#define AZ_DEFAULT_ALIGNMENT (sizeof(void*))

// define unlimited allocator limits (scaled to real number when we check if there is enough memory to allocate)
#define AZ_CORE_MAX_ALLOCATOR_SIZE AZ_TRAIT_OS_MEMORY_MAX_ALLOCATOR_SIZE
