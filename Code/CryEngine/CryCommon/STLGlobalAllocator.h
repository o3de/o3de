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

#ifndef CRYINCLUDE_CRYCOMMON_STLGLOBALALLOCATOR_H
#define CRYINCLUDE_CRYCOMMON_STLGLOBALALLOCATOR_H
#pragma once


//---------------------------------------------------------------------------
// STL-compatible interface for an std::allocator using the global heap.
//---------------------------------------------------------------------------

#include <stddef.h>
#include <climits>

#include "CryMemoryManager.h"

#include <AzCore/Memory/AllocatorBase.h>
#include <AzCore/Memory/HphaSchema.h>
#include <AzCore/Memory/AllocatorManager.h>

struct CryLegacySTLAllocatorDescriptor
    : public AZ::HphaSchema::Descriptor
{
    CryLegacySTLAllocatorDescriptor()
    {
        m_systemChunkSize = 4 * 1024 * 1024; // Ask the OS for 4MB at a time
    }
};

class CryLegacySTLAllocator
    : public AZ::SimpleSchemaAllocator<AZ::HphaSchema, CryLegacySTLAllocatorDescriptor>
{
public:
    AZ_TYPE_INFO(CryLegacySTLAllocator, "{87EE21F1-8215-4979-B493-AF13D8D91DAD}");
    using Descriptor = CryLegacySTLAllocatorDescriptor;
    using Base = AZ::SimpleSchemaAllocator<AZ::HphaSchema, CryLegacySTLAllocatorDescriptor>;
    CryLegacySTLAllocator()
        : Base("CryLegacySTLAllocator", "Allocator used to dodge limits on static init time allocations")
    {
    }
};

// Specialize for the CryLegacySTLAllocator to provide one per module that does not use the
// environment for its storage, since this thing is designed to get around the lack
// of static allocators
namespace AZ
{
    template <>
    class AllocatorInstance<CryLegacySTLAllocator> : public Internal::AllocatorInstanceBase<CryLegacySTLAllocator, AllocatorStorage::ModuleStoragePolicy<CryLegacySTLAllocator>>
    {
    };
}


class ICrySizer;
namespace stl
{
    template <class T>
    class STLGlobalAllocator
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
            typedef STLGlobalAllocator<U> other;
        };

        STLGlobalAllocator() throw()
        {
        }

        STLGlobalAllocator(const STLGlobalAllocator&) throw()
        {
        }

        template <class U>
        STLGlobalAllocator(const STLGlobalAllocator<U>&) throw()
        {
        }

        ~STLGlobalAllocator() throw()
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

        pointer allocate(size_type n = 1, const void* hint = 0)
        {
            (void)hint;
            pointer ret = static_cast<pointer>(AZ::AllocatorInstance<CryLegacySTLAllocator>::Get().Allocate(n * sizeof(T), 0));
            return ret;
        }

        void deallocate(pointer p, [[maybe_unused]] size_type n = 1)
        {
            AZ::AllocatorInstance<CryLegacySTLAllocator>::Get().DeAllocate(p);
        }

        size_type max_size() const throw()
        {
            return INT_MAX;
        }
#if !defined(_LIBCPP_VERSION)
        void construct(pointer p, const T& val)
        {
            new(static_cast<void*>(p))T(val);
        }

        void construct(pointer p)
        {
            new(static_cast<void*>(p))T();
        }
#endif // !_LIBCPP_VERSION
        void destroy(pointer p)
        {
            p->~T();
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

        bool operator==(const STLGlobalAllocator&) const { return true; }
        bool operator!=(const STLGlobalAllocator&) const { return false; }

        static void GetMemoryUsage(ICrySizer* pSizer)
        {
        }
    };

    template <>
    class STLGlobalAllocator<void>
    {
    public:
        typedef void* pointer;
        typedef const void* const_pointer;
        typedef void value_type;
        template <class U>
        struct rebind
        {
            typedef STLGlobalAllocator<U> other;
        };
    };
}

#endif // CRYINCLUDE_CRYCOMMON_STLGLOBALALLOCATOR_H
