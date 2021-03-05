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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_STLPOOLALLOCATOR_H
#define CRYINCLUDE_CRYCOMMON_STLPOOLALLOCATOR_H
#pragma once


//---------------------------------------------------------------------------
// STL-compatible interface for the pool allocator (see PoolAllocator.h).
//
// This class is suitable for use as an allocator for STL lists. Note it will
// not work with vectors, since it allocates fixed-size blocks, while vectors
// allocate elements in variable-sized contiguous chunks.
//
// To create a list of type UserDataType using this allocator, use the
// following syntax:
//
// std::list<UserDataType, STLPoolAllocator<UserDataType> > myList;
//---------------------------------------------------------------------------

#include "PoolAllocator.h"
#include "MetaUtils.h"
#include <stddef.h>
#include <climits>

namespace stl
{
    namespace STLPoolAllocatorHelper
    {
        inline void destruct(char*) {}
        inline void destruct(wchar_t*) {}
        template <typename T>
        inline void destruct(T* t) {t->~T(); }
    }

    template <size_t S, class L, size_t A, bool FreeWhenEmpty, typename T>
    struct STLPoolAllocatorStatic
    {
        // Non-freeing stl pool allocators should just go on the global heap - only if they've been explicitly
        // set to cleanup should they go on the default heap.
        typedef SizePoolAllocator<
            HeapAllocator<
                L,
                typename metautils::select<FreeWhenEmpty, HeapSysAllocator, GlobalHeapSysAllocator>::type>
            > AllocatorType;

        static AllocatorType* GetOrCreateAllocator()
        {
            if (allocator)
            {
                return allocator;
            }

            allocator = new AllocatorType(S, A, FHeap().FreeWhenEmpty(FreeWhenEmpty));
            return allocator;
        }

        static AllocatorType* allocator;
    };

    template <class T, class L, size_t A, bool FreeWhenEmpty>
    struct STLPoolAllocatorKungFu
        : public STLPoolAllocatorStatic<sizeof(T), L, A, FreeWhenEmpty, T>
    {
    };

    template <class T, class L = PSyncMultiThread, size_t A = 0, bool FreeWhenEmpty = false>
    class STLPoolAllocator
    {
    public:
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;
        typedef T*        pointer;
        typedef const T*  const_pointer;
        typedef T&        reference;
        typedef const T&  const_reference;
        typedef T         value_type;

        template <class U>
        struct rebind
        {
            typedef STLPoolAllocator<U, L, A, FreeWhenEmpty> other;
        };

        STLPoolAllocator() throw()
        {
        }

        STLPoolAllocator(const STLPoolAllocator&) throw()
        {
        }

        template <class U, class M, size_t B, bool FreeWhenEmptyA>
        STLPoolAllocator(const STLPoolAllocator<U, M, B, FreeWhenEmptyA>&) throw()
        {
        }

        ~STLPoolAllocator() throw()
        {
        }

        pointer address(reference x) const
        {
            return &x;
        }

        const_pointer address(const_reference x) const
        {
            return &x;
        }

        pointer allocate([[maybe_unused]] size_type n = 1, [[maybe_unused]] const void* hint = 0)
        {
            assert(n == 1);
            typename STLPoolAllocatorKungFu<T, L, A, FreeWhenEmpty>::AllocatorType * allocator = STLPoolAllocatorKungFu<T, L, A, FreeWhenEmpty>::GetOrCreateAllocator();
            return static_cast<T*>(allocator->Allocate());
        }

        void deallocate(pointer p, [[maybe_unused]] size_type n = 1)
        {
            assert(n == 1);
            typename STLPoolAllocatorKungFu<T, L, A, FreeWhenEmpty>::AllocatorType * allocator = STLPoolAllocatorKungFu<T, L, A, FreeWhenEmpty>::allocator;
            allocator->Deallocate(p);
        }

        size_type max_size() const throw()
        {
            return INT_MAX;
        }
#ifndef _LIBCPP_VERSION
        void construct(pointer p, const T& val)
        {
            new(static_cast<void*>(p))T(val);
        }

        void construct(pointer p)
        {
            new(static_cast<void*>(p))T();
        }
#endif // !(_LIBCPP_VERSION)
        void destroy(pointer p)
        {
            STLPoolAllocatorHelper::destruct(p);
        }

        pointer new_pointer()
        {
            return new(allocate())T();
        }

        pointer new_pointer(const T& val)
        {
            return new(allocate())T(val);
        }

        void delete_pointer(pointer p)
        {
            p->~T();
            deallocate(p);
        }

        bool operator==(const STLPoolAllocator&) {return true; }
        bool operator!=(const STLPoolAllocator&) {return false; }

        static void GetMemoryUsage(ICrySizer* pSizer)
        {
            pSizer->AddObject(STLPoolAllocatorKungFu<T, L, A, FreeWhenEmpty>::allocator);
        }
    };

    template <class T, size_t A = 0, bool FreeWhenEmpty = false>
    class STLPoolAllocatorNoMT
        : public STLPoolAllocator<T, PSyncNone, A, FreeWhenEmpty>
    {
    };

    template <>
    class STLPoolAllocator<void>
    {
    public:
        typedef void* pointer;
        typedef const void* const_pointer;
        typedef void value_type;
        template <class U, class L>
        struct rebind
        {
            typedef STLPoolAllocator<U> other;
        };
    };

    template <size_t S, typename L, size_t A, bool FreeWhenEmpty, typename T>
    typename STLPoolAllocatorStatic<S, L, A, FreeWhenEmpty, T>::AllocatorType * STLPoolAllocatorStatic<S, L, A, FreeWhenEmpty, T>::allocator;
}



#endif // CRYINCLUDE_CRYCOMMON_STLPOOLALLOCATOR_H
