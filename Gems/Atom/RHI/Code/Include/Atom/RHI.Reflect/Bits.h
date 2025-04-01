/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/MathUtils.h>

#ifndef AZ_BIT
#define AZ_BIT(x) (1u << x)
#endif

#ifndef AZ_BIT_MASK
#define AZ_BIT_MASK(BitCount) (AZ_BIT(BitCount) - 1u)
#endif

#ifndef AZ_BIT_MASK_OFFSET
#define AZ_BIT_MASK_OFFSET(BitCount, BitOffset) (AZ_BIT_MASK(BitCount) << BitOffset)
#endif

namespace AZ::RHI
{
    //! A constexpr version of array size that returns a 32 bit result. This exists
    //! to avoid requiring explicit casting from 64-bit types to 32-bit types in the RHI,
    //! since uint32_t is always used for array sizes.
    template <typename T, size_t N>
    constexpr uint32_t ArraySize(T const (&)[N])
    {
        return static_cast<uint32_t>(N);
    }

    //! Aligns value up to the given bit mask. Assumes mask is power of two minus 1.
    template <typename T> constexpr T AlignUpWithMask(T value, size_t mask)
    {
        return (T)(((size_t)value + mask) & ~mask);
    }

    //! Aligns value down to the given bit mask. Assumes mask is power of two minus 1.
    template <typename T> constexpr T AlignDownWithMask(T value, size_t mask)
    {
        return (T)((size_t)value & ~mask);
    }

    //! Aligns value up to the given alignment. Assumes alignment is power of two.
    template <typename T> constexpr T AlignUp(T value, size_t alignment)
    {
        return AlignUpWithMask(value, alignment - 1);
    }

    //! Aligns value up to the given alignment. It doesn't require the alignment to be a power of two
    template <typename T> constexpr T AlignUpNPOT(T value, size_t alignment)
    {
        return value + (value%alignment > 0 ? (alignment - (value%alignment)) : 0);
    }

    //! Aligns value down to the given alignment. Assumes alignment is power of two.
    template <typename T> constexpr T AlignDown(T value, size_t alignment)
    {
        return AlignDownWithMask(value, alignment - 1);
    }

    //! Returns whether the value is aligned to the given alignment.
    template <typename T> inline bool IsAligned(T value, size_t alignment)
    {
        return 0 == ((size_t)value & (alignment - 1));
    }

    //! Returns true if value is a power of two.
    template <typename T> inline bool IsPowerOfTwo(T value)
    {
        return 0 == (value & (value - 1));
    }

    //! Rounds value up to the next power of two.
    inline uint32_t NextPowerOfTwo(uint32_t value)
    {
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value++;
        return value;
    }

    //! Returns the offset in bytes from base to offset by subtracting the two pointers.
    inline size_t GetByteOffset(const void* base, const void* offset)
    {
        return reinterpret_cast<uintptr_t>(offset) - reinterpret_cast<uintptr_t>(base);
    }

    //! Returns the number of bits set in v.
    inline uint8_t CountBitsSet(uint8_t v)
    {
        uint8_t c = v;
        c = ((c >> 1) & 0x55) + (c & 0x55);
        c = ((c >> 2) & 0x33) + (c & 0x33);
        c = ((c >> 4) & 0x0f) + (c & 0x0f);
        return c;
    }

    //! Returns the number of bits set in v.
    inline uint8_t CountBitsSet(uint16_t v)
    {
        return
            CountBitsSet((uint8_t)((v)      & 0xff)) +
            CountBitsSet((uint8_t)((v >> 8) & 0xff));
    }

    //! Returns the number of bits set in v.
    inline uint8_t CountBitsSet(uint32_t v)
    {
        return
            CountBitsSet((uint8_t)((v)       & 0xff)) +
            CountBitsSet((uint8_t)((v >> 8)  & 0xff)) +
            CountBitsSet((uint8_t)((v >> 16) & 0xff)) +
            CountBitsSet((uint8_t)((v >> 24) & 0xff));
    }

    //! Returns the number of bits set in v.
    inline uint8_t CountBitsSet(uint64_t v)
    {
        return
            CountBitsSet((uint8_t)((v)       & 0xff)) +
            CountBitsSet((uint8_t)((v >> 8)  & 0xff)) +
            CountBitsSet((uint8_t)((v >> 16) & 0xff)) +
            CountBitsSet((uint8_t)((v >> 24) & 0xff)) +
            CountBitsSet((uint8_t)((v >> 32) & 0xff)) +
            CountBitsSet((uint8_t)((v >> 40) & 0xff)) +
            CountBitsSet((uint8_t)((v >> 48) & 0xff)) +
            CountBitsSet((uint8_t)((v >> 56) & 0xff));
    }

    //! Resets any non-zero bits in bits in v to 0.
    template <typename T>
    inline T ResetBits(T v, T bits)
    {
        return v & (~bits);
    }

    //! Sets the bit at bitIndex in v to 0.
    template<typename T, typename U>
    inline T ResetBit(T v, U bitIndex)
    {
        return v & static_cast<T>(~AZ_BIT(bitIndex));
    }

    //! Reset any zero bits in bits in v to 0.
    template <typename T>
    inline T FilterBits(T v, T bits)
    {
        return v & bits;
    }

    //! Sets any zero bits in bits in v to 1.
    template <typename T>
    inline T SetBits(T v, T bits)
    {
        return v | bits;
    }

    //! Sets the bit at bitIndex in v to 1.
    template<typename T, typename U>
    inline T SetBit(T v, U bitIndex)
    {
        return v | static_cast<T>(AZ_BIT(bitIndex));
    }

    //! Returns whether the bit at bitIndex is set.
    template<typename T, typename U>
    inline bool CheckBit(T v, U bitIndex)
    {
        return (v & static_cast<T>(AZ_BIT(bitIndex))) != static_cast<T>(0);
    }

    //! Returns whether all the set bits in bits are set in v.
    template <typename T>
    inline bool CheckBitsAll(T v, T bits)
    {
        return (v & bits) == bits;
    }

    //! Returns whether any of the set bits in bits are set in v.
    template <typename T>
    inline bool CheckBitsAny(T v, T bits)
    {
        return (v & bits) != (T)0;
    }

    //! Returns whether value is divisible by divisor.
    template <typename T> bool IsDivisible(T value, T divisor)
    {
        return (value / divisor) * divisor == value;
    }

    //! O3DE_DEPRECATION_NOTICE(GHI-7407)
    //! Returns the value divided by alignment, where the result is rounded up if the remainder is non-zero.
    template <typename T> inline T DivideByMultiple(T value, size_t alignment)
    {
        return (T)((value + alignment - 1) / alignment);
    }

    template <size_t S>
    struct _ENUM_FLAG_INTEGER_FOR_SIZE;

    template <>
    struct _ENUM_FLAG_INTEGER_FOR_SIZE<1>
    {
        typedef int8_t type;
    };

    template <>
    struct _ENUM_FLAG_INTEGER_FOR_SIZE<2>
    {
        typedef int16_t type;
    };

    template <>
    struct _ENUM_FLAG_INTEGER_FOR_SIZE<4>
    {
        typedef int32_t type;
    };

    template <>
    struct _ENUM_FLAG_INTEGER_FOR_SIZE<8>
    {
        typedef int64_t type;
    };

    // used as an approximation of std::underlying_type<T>
    template <class T>
    struct _ENUM_FLAG_SIZED_INTEGER
    {
        typedef typename _ENUM_FLAG_INTEGER_FOR_SIZE<sizeof(T)>::type type;
    };
}
