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

// Generic unaligned memory access helpers.
#pragma once
#include <stddef.h>
#include <type_traits>

namespace Detail
{
    template<typename RealType, typename BlittedElement>
    struct Blitter
    {
        static_assert(std::is_trivial<BlittedElement>::value, "Blittable elements should be trivial (ie, integral types)");
        static_assert((std::alignment_of<BlittedElement>::value < std::alignment_of<RealType>::value), "Blittable memory has sufficient alignment, do not use unaligned store or load");
        static_assert(sizeof(RealType) > sizeof(BlittedElement), "Blittable element is larger than real type");
        static_assert((sizeof(RealType) % sizeof(BlittedElement)) == 0, "Blitted element has the wrong size for the real type");
        typedef std::integral_constant<size_t, sizeof(RealType) / sizeof(BlittedElement)> NumElements;

        static void BlitLoad(const BlittedElement* pSource, RealType& target)
        {
            BlittedElement* pTarget = alias_cast<BlittedElement*>(&target);
            for (size_t i = 0; i < NumElements::value; ++i, ++pSource, ++pTarget)
            {
                * pTarget = *pSource;
            }
        }

        static void BlitStore(const RealType& source, BlittedElement* pTarget)
        {
            const BlittedElement* pSource = alias_cast<const BlittedElement*>(&source);
            for (size_t i = 0; i < NumElements::value; ++i, ++pSource, ++pTarget)
            {
                * pTarget = *pSource;
            }
        }
    };
}

// Load RealType from unaligned memory using some blittable type.
// The source memory must be suitably aligned for accessing BlittedElement.
// If no memory alignment can be guaranteed, use char for BlittedElement.
template<typename RealType, typename BlittedElement>
inline void LoadUnaligned(const BlittedElement* pMemory, RealType& value,
    typename std::enable_if<(std::alignment_of<RealType>::value > std::alignment_of<BlittedElement>::value)>::type* = nullptr)
{
    Detail::Blitter<RealType, BlittedElement>::BlitLoad(pMemory, value);
}

// Load RealType from aligned memory (fallback overload).
// This is used if there is no reason to call the blitter, because sufficient alignment is guaranteed by BlittedElement.
template<typename RealType, typename BlittedElement>
inline void LoadUnaligned(const BlittedElement* pMemory, RealType& value,
    typename std::enable_if<(std::alignment_of<RealType>::value <= std::alignment_of<BlittedElement>::value)>::type* = nullptr)
{
    value = *alias_cast<RealType*>(pMemory);
}

// Store to unaligned memory using some blittable type.
// The target memory must be suitably aligned for accessing BlittedElement.
// If no memory alignment can be guaranteed, use char for BlittedElement.
template<typename RealType, typename BlittedElement>
inline void StoreUnaligned(BlittedElement* pMemory, const RealType& value,
    typename std::enable_if<(std::alignment_of<RealType>::value > std::alignment_of<BlittedElement>::value)>::type* = nullptr)
{
    Detail::Blitter<RealType, BlittedElement>::BlitStore(value, pMemory);
}

// Store to aligned memory (fallback overload).
// This is used if there is no reason to call the blitter, because sufficient alignment is guaranteed by BlittedElement.
template<typename RealType, typename BlittedElement>
inline void StoreUnaligned(BlittedElement* pMemory, const RealType& value,
    typename std::enable_if<(std::alignment_of<RealType>::value <= std::alignment_of<BlittedElement>::value)>::type* = nullptr)
{
    *alias_cast<RealType*>(pMemory) = value;
}

// Pads the given pointer to the next possible aligned location for RealType
// Use this to ensure RealType can be referenced in some buffer of BlittedElement's, without using LoadUnaligned/StoreUnaligned
template<typename RealType, typename BlittedElement>
inline BlittedElement* AlignPointer(BlittedElement* pMemory,
    typename std::enable_if<(std::alignment_of<RealType>::value % std::alignment_of<BlittedElement>::value) == 0>::type* = nullptr)
{
    const size_t align = std::alignment_of<RealType>::value;
    const size_t mask = align - 1;
    const size_t address = reinterpret_cast<size_t>(pMemory);
    const size_t offset = (align - (address & mask)) & mask;
    return pMemory + (offset / sizeof(BlittedElement));
}

// Pads the given address to the next possible aligned location for RealType
// Use this to ensure RealType can be referenced inside memory, without using LoadUnaligned/StoreUnaligned
template<typename RealType>
inline size_t AlignAddress(size_t address)
{
    return reinterpret_cast<size_t>(AlignPointer<RealType>(reinterpret_cast<char*>(address)));
}

// Provides aligned storage for T, optionally aligned at a specific boundary (default being the native alignment of T)
// The specified T is not initialized automatically, use of placement new/delete is the user's responsibility
template<typename T, size_t Align = std::alignment_of<T>::value>
struct SUninitialized
{
    typedef typename std::aligned_storage<sizeof(T), Align>::type Storage;
    Storage storage;

    void DefaultConstruct()
    {
        new(static_cast<void*>(&storage))T();
    }

    void CopyConstruct(const T& value)
    {
        new(static_cast<void*>(&storage))T(value);
    }

    void MoveConstruct(T&& value)
    {
        new(static_cast<void*>(&storage))T(std::move(value));
    }

    void Destruct()
    {
        alias_cast<T*>(&storage)->~T();
    }

    operator T& ()
    {
        return *alias_cast<T*>(&storage);
    }
};
