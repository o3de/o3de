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

// Description : Implements an aligned allocator for STL
//               based on the Mallocator (http://blogs.msdn.com/b/vcblog/archive/2008/08/28/the-mallocator.aspx)

#pragma once

#include <stddef.h>  // Required for size_t and ptrdiff_t and NULL

#include <AzCore/Memory/AllocatorBase.h>

namespace stl
{
    template <size_t Alignment>
    class AlignedAllocator
        : public AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::LegacyAllocator>>
    {
    public:
        AZ_TYPE_INFO(AlignedAllocator, "{DF152D8A-36ED-4A2A-9FA6-734F212716C6}");
        using Base = AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::LegacyAllocator>>;
        using Descriptor = Base::Descriptor;
        using Schema = AZ::ChildAllocatorSchema<AZ::LegacyAllocator>;

        AlignedAllocator()
            : Base("AlignedAllocator", "Legacy Cry Aligned Allocator")
        {
        }

        pointer_type Allocate(size_type byteSize, size_type /*alignment*/, int flags /* = 0 */, const char* name /* = 0 */, const char* fileName /* = 0 */, int lineNum /* = 0 */, unsigned int suppressStackRecord /* = 0 */) override
        {
            return Base::Allocate(byteSize, Alignment, flags, name, fileName, lineNum, suppressStackRecord);
        }

        void DeAllocate(pointer_type ptr, size_type byteSize, [[maybe_unused]] size_type alignment) override
        {
            return Base::DeAllocate(ptr, byteSize, Alignment);
        }

        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type /*newAlignment*/) override
        {
            return Base::ReAllocate(ptr, newSize, Alignment);
        }
    };

    template <size_t Alignment>
    using aligned_alloc = AZ::AZStdAlloc<AlignedAllocator<Alignment>>;

    //////////////////////////////////////////////////////////////////////////
    // Defines aligned vector type
    //////////////////////////////////////////////////////////////////////////
    template <typename T, int AlignSize>
    class aligned_vector
        : public AZStd::vector<T, aligned_alloc<AlignSize> >
    {
    public:
        typedef aligned_alloc<AlignSize>      MyAlloc;
        typedef AZStd::vector<T, MyAlloc>     MySuperClass;
        typedef aligned_vector<T, AlignSize>  MySelf;
        typedef size_t size_type;

        aligned_vector() {}
        explicit aligned_vector(const MyAlloc& _Al)
            : MySuperClass(_Al) {}
        explicit aligned_vector(size_type _Count)
            : MySuperClass(_Count) {};
        aligned_vector(size_type _Count, const T& _Val)
            : MySuperClass(_Count, _Val) {}
        aligned_vector(size_type _Count, const T& _Val, const MyAlloc& _Al)
            : MySuperClass(_Count, _Val) {}
        aligned_vector(const MySelf& _Right)
            : MySuperClass(_Right) {};


        template<class _Iter>
        aligned_vector(_Iter _First, _Iter _Last)
            : MySuperClass(_First, _Last) {};

        template<class _Iter>
        aligned_vector(_Iter _First, _Iter _Last, const MyAlloc& _Al)
            : MySuperClass(_First, _Last, _Al) {};
    };

    template <class Vec>
    inline size_t size_of_aligned_vector(const Vec& c)
    {
        if (!c.empty())
        {
            // Not really correct as not taking alignment into the account
            return c.capacity() * sizeof(typename Vec::value_type);
        }
        return 0;
    }
} // namespace stl

// Specialize for the AlignedAllocator to provide one per module that does not use the
// environment for its storage. Since this allocator just uses LegacyAllocator
// to do the real work, it's fine if there is one of these per cry module
namespace AZ
{
    template <size_t Alignment>
    class AllocatorInstance<stl::AlignedAllocator<Alignment>> : public Internal::AllocatorInstanceBase<stl::AlignedAllocator<Alignment>, AllocatorStorage::ModuleStoragePolicy<stl::AlignedAllocator<Alignment>>>
    {
    };
}
