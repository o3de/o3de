/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/array.h>
#include <AzCore/Math/CrcInternal.h>

namespace AZ
{
    namespace Internal
    {
        template <auto CrcValue>
        inline static constexpr AZ::Crc32 CompileTimeCrc32 = AZ::Crc32(CrcValue);
    }

    template <class ByteType>
    constexpr auto Crc32::Set(const ByteType* data, size_t size, bool forceLowerCase)
        -> AZStd::enable_if_t<sizeof(ByteType) == 1>
    {
        Internal::Crc32Set(data, size, forceLowerCase, m_value);
    }

    //=========================================================================
    //
    // Crc32 constructor
    //
    //=========================================================================
    constexpr Crc32::Crc32(AZStd::string_view view)
        : m_value{ 0 }
    {
        if (!view.empty())
        {
            Set(view.data(), view.size(), true);
        }
    }

    //=========================================================================
    //
    // Crc32 constructor
    //
    //=========================================================================
    template<class ByteType, class>
    constexpr Crc32::Crc32(const ByteType* data, size_t size, bool forceLowerCase)
        : m_value{ 0 }
    {
        Set(data, size, forceLowerCase);
    }
    constexpr Crc32::Crc32(AZStd::span<const AZStd::byte> inputSpan)
        : m_value{ 0 }
    {
        Set(inputSpan.data(), inputSpan.size(), false);
    }

    //=========================================================================
    // Crc32 - Add
    //=========================================================================
    constexpr void Crc32::Add(AZStd::string_view view)
    {
        if (!view.empty())
        {
            size_t len = view.size();
            u32 crc = static_cast<u32>(Crc32(view));
            Combine(crc, len);
        }
    }

    //=========================================================================
    // Crc32 - Add
    //=========================================================================
    template <class ByteType>
    constexpr auto Crc32::Add(const ByteType* data, size_t size, bool forceLowerCase)
        -> AZStd::enable_if_t<sizeof(ByteType) == 1>
    {
        Combine(Crc32{ data, size, forceLowerCase }, size);
    }

    constexpr void Crc32::Add(AZStd::span<const AZStd::byte> inputSpan)
    {
        Combine(Crc32{ inputSpan }, inputSpan.size());
    }

    constexpr u32 MatrixTimes(u32* mat, u32 vec)
    {
        u32 sum = 0;
        while (vec)
        {
            if (vec & 1)
            {
                sum ^= *mat;
            }
            vec >>= 1;
            mat++;
        }
        return sum;
    }

    //=========================================================================
    // Crc32 - Combine
    //=========================================================================
    constexpr void Crc32::Combine(u32 crc, size_t len)
    {
        constexpr size_t GF2_DIM = 32; /* dimension of GF(2) vectors (length of CRC) */

        int n{};
        u32 row{};
        u32 even[GF2_DIM]{};    /* even-power-of-two zeros operator */
        u32 odd[GF2_DIM]{};     /* odd-power-of-two zeros operator */

        /* degenerate case (also disallow negative lengths) */
        if (len <= 0)
        {
            return;
        }

        /* put operator for one zero bit in odd */
        odd[0] = 0xedb88320UL;          /* CRC-32 polynomial */
        row = 1;
        for (n = 1; n < GF2_DIM; n++)
        {
            odd[n] = row;
            row <<= 1;
        }

        /* put operator for two zero bits in even */
        for (n = 0; n < GF2_DIM; n++)
        {
            even[n] = MatrixTimes(odd, odd[n]);
        }

        /* put operator for four zero bits in odd */
        for (n = 0; n < GF2_DIM; n++)
        {
            odd[n] = MatrixTimes(even, even[n]);
        }

        u32 value = m_value;

        /* apply len2 zeros to crc1 (first square will put the operator for one
           zero byte, eight zero bits, in even) */
        do
        {
            /* apply zeros operator for this bit of len2 */
            for (n = 0; n < GF2_DIM; n++)
            {
                even[n] = MatrixTimes(odd, odd[n]);
            }

            if (len & 1)
            {
                value = MatrixTimes(even, value);
            }
            len >>= 1;

            /* if no more bits set, then done */
            if (len == 0)
            {
                break;
            }

            /* another iteration of the loop with odd and even swapped */
            for (n = 0; n < GF2_DIM; n++)
            {
                odd[n] = MatrixTimes(even, even[n]);
            }

            if (len & 1)
            {
                value = MatrixTimes(odd, value);
            }
            len >>= 1;

            /* if no more bits set, then done */
        } while (len != 0);

        m_value = value ^ crc;
    }
}
