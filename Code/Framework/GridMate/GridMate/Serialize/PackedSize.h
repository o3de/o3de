/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_PACKEDSIZE
#define GM_PACKEDSIZE

#include <AzCore/std/utils.h>

namespace GridMate
{
    /**
    * Represents an exact binary size in both bytes and any additional bits.
    *
    * This is frequently need during bitpacking of Read- and WriteBuffers.
    */
    class PackedSize
    {
    public:
        typedef AZStd::size_t SizeType;

        // Default value of zero
        PackedSize() : m_totalBits(0) {}
        // When you only need to specify bytes
        PackedSize(SizeType bytes) : m_totalBits(bytes * CHAR_BIT) {}
        /*
        * When the length also has to include non-8 amount of bits.
        * For example, if you want to define a size of 3 bits only, etc.
        */
        PackedSize(SizeType bytes, SizeType bits) : m_totalBits(bytes * CHAR_BIT + bits) {}

        // Returns the number of full bytes, note there may be some additional bits left, see ::GetAdditionalBits()
        AZ_FORCE_INLINE SizeType GetBytes() const { return m_totalBits >> 3; }

        /*
         * Range of [0..7]
         *
         * If the size is not expressable in bytes exactly, then additional bits are returned, otherwise returns zero.
         */
        AZ_FORCE_INLINE AZ::u8 GetAdditionalBits() const { return m_totalBits % CHAR_BIT; }

        AZ_FORCE_INLINE AZStd::size_t GetTotalSizeInBits() const { return m_totalBits; }

        // Returns the size in bytes plus another byte if there are any bits included as well.
        AZ_FORCE_INLINE SizeType GetSizeInBytesRoundUp() const
        {
            return GetBytes() + ((GetAdditionalBits() > 0) ? 1 : 0);
        }

        AZ_FORCE_INLINE void IncrementBytes(SizeType bytes)
        {
            m_totalBits += bytes * CHAR_BIT;
        }

        AZ_FORCE_INLINE void DecrementBytes(SizeType bytes)
        {
            AZ_Assert(GetBytes() >= bytes, "Negative resulting size isn't supported");

            m_totalBits -= bytes * CHAR_BIT;
        }

        AZ_FORCE_INLINE void IncrementBit()
        {
            m_totalBits++;
        }

        AZ_FORCE_INLINE void IncrementBits(SizeType bits)
        {
            m_totalBits += bits;
        }

        AZ_FORCE_INLINE void DecrementBits(SizeType bits)
        {
            AZ_Assert(m_totalBits >= bits, "Negative resulting size is an error");

            m_totalBits -= bits;
        }

        AZ_FORCE_INLINE void operator+=(const PackedSize& other)
        {
            m_totalBits += other.m_totalBits;
        }

        AZ_FORCE_INLINE void operator-=(const PackedSize& other)
        {
            AZ_Assert(m_totalBits >= other.m_totalBits, "Negative resulting size is an error");
            m_totalBits -= other.m_totalBits;
        }

    private:
        AZStd::size_t m_totalBits; // the whole size, represented in bits
    };

    AZ_FORCE_INLINE bool operator==(const PackedSize& lhs, const PackedSize&rhs)
    {
        return lhs.GetTotalSizeInBits() == rhs.GetTotalSizeInBits();
    }

    AZ_FORCE_INLINE bool operator!=(const PackedSize& lhs, const PackedSize&rhs)
    {
        return !(lhs == rhs);
    }

    AZ_FORCE_INLINE bool operator>(const PackedSize& lhs, const PackedSize&rhs)
    {
        return lhs.GetTotalSizeInBits() > rhs.GetTotalSizeInBits();
    }

    AZ_FORCE_INLINE bool operator<(const PackedSize& lhs, const PackedSize&rhs)
    {
        return lhs.GetTotalSizeInBits() < rhs.GetTotalSizeInBits();
    }

    AZ_FORCE_INLINE bool operator>=(const PackedSize& lhs, const PackedSize&rhs)
    {
        return !(lhs < rhs);
    }

    AZ_FORCE_INLINE bool operator<=(const PackedSize& lhs, const PackedSize&rhs)
    {
        return !(lhs > rhs);
    }

    AZ_FORCE_INLINE PackedSize operator-(const PackedSize& lhs, const PackedSize&rhs)
    {
        PackedSize out(lhs);
        out -= rhs;
        return out;
    }

    AZ_FORCE_INLINE PackedSize operator+(const PackedSize& lhs, const PackedSize&rhs)
    {
        PackedSize out(lhs);
        out += rhs;
        return out;
    }
}

#endif // GM_PACKEDSIZE
