/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <bit>

#if !defined(__cpp_lib_byteswap) || __cpp_lib_byteswap < 202110L
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <type_traits>
#endif

namespace AZStd
{
    using std::bit_cast;

    using std::endian;

    using std::rotl;
    using std::rotr;

    using std::countl_zero;
    using std::countl_one;
    using std::countr_zero;
    using std::countr_one;
    using std::popcount;

    using std::has_single_bit;
    using std::bit_ceil;
    using std::bit_floor;
    using std::bit_width;

#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L
    // C++23
    using std::byteswap;
#else
    //! Pre-C++23
    template<std::integral T>
    [[nodiscard]] constexpr T byteswap(T value) noexcept
    {
        // Must come first, as make_unsigned is ill-formed for bool and the character types.
        if constexpr (sizeof(T) == 1)
        {
            return value;
        }
#if defined(_MSC_VER) && !defined(__clang__)
        // The MSVC intrinsics are not constexpr, unlike __builtin_bswap* below.
        else if constexpr (sizeof(T) == 2)
        {
            const auto bits = static_cast<unsigned short>(value);
            if (std::is_constant_evaluated())
            {
                return static_cast<T>(static_cast<unsigned short>((bits << 8) | (bits >> 8)));
            }
            else
            {
                return static_cast<T>(_byteswap_ushort(bits));
            }
        }
        else if constexpr (sizeof(T) == 4)
        {
            const auto bits = static_cast<unsigned long>(value);
            if (std::is_constant_evaluated())
            {
                return static_cast<T>(static_cast<unsigned long>(
                    ((bits & 0x0000'00FFuL) << 24) | ((bits & 0x0000'FF00uL) << 8) |
                    ((bits & 0x00FF'0000uL) >> 8) | ((bits & 0xFF00'0000uL) >> 24)));
            }
            else
            {
                return static_cast<T>(_byteswap_ulong(bits));
            }
        }
        else if constexpr (sizeof(T) == 8)
        {
            const auto bits = static_cast<unsigned long long>(value);
            if (std::is_constant_evaluated())
            {
                return static_cast<T>(static_cast<unsigned long long>(
                    ((bits & 0x0000'0000'0000'00FFuLL) << 56) | ((bits & 0x0000'0000'0000'FF00uLL) << 40) |
                    ((bits & 0x0000'0000'00FF'0000uLL) << 24) | ((bits & 0x0000'0000'FF00'0000uLL) << 8) |
                    ((bits & 0x0000'00FF'0000'0000uLL) >> 8) | ((bits & 0x0000'FF00'0000'0000uLL) >> 24) |
                    ((bits & 0x00FF'0000'0000'0000uLL) >> 40) | ((bits & 0xFF00'0000'0000'0000uLL) >> 56)));
            }
            else
            {
                return static_cast<T>(_byteswap_uint64(bits));
            }
        }
#else
        else if constexpr (sizeof(T) == 2)
        {
            return static_cast<T>(__builtin_bswap16(static_cast<unsigned short>(value)));
        }
        else if constexpr (sizeof(T) == 4)
        {
            return static_cast<T>(__builtin_bswap32(static_cast<unsigned int>(value)));
        }
        else if constexpr (sizeof(T) == 8)
        {
            return static_cast<T>(__builtin_bswap64(static_cast<unsigned long long>(value)));
        }
#endif
        else
        {
            // Extended integer types such as __int128.
            using UnsignedType = std::make_unsigned_t<T>;
            auto bits = static_cast<UnsignedType>(value);
            UnsignedType result{};
            for (std::size_t byteIndex = 0; byteIndex < sizeof(T); ++byteIndex)
            {
                result = static_cast<UnsignedType>(static_cast<UnsignedType>(result << 8) | (bits & UnsignedType{ 0xFF }));
                bits = static_cast<UnsignedType>(bits >> 8);
            }
            return static_cast<T>(result);
        }
    }
#endif
} // namespace AZStd
