// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This is a byte oriented implementation
// Note: this implementation does not handle message longer than
//       2^32 bytes.

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/span.h>

namespace AZ
{
    class Sha1
    {
    public:
        using DigestType = AZ::u32(&)[5];

        constexpr Sha1() = default;

        constexpr void Reset();

        constexpr void ProcessByte(AZStd::byte byte);
        constexpr void ProcessBytes(AZStd::span<AZStd::byte const> byteSpan);
        constexpr void ProcessBytes(AZStd::byte const* buffer, size_t byteCount);

        constexpr void GetDigest(DigestType digest);

    private:
        constexpr void ProcessBlock();

        AZ_FORCE_INLINE constexpr AZ::u32 LeftRotate(AZ::u32 x, size_t n)
        {
            return (x << n) ^ (x >> (32 - n));
        }

        AZ::u32 m_h[5]
        {
            0x67452301,
            0xEFCDAB89,
            0x98BADCFE,
            0x10325476,
            0xC3D2E1F0
        };

        AZStd::byte m_block[64]{ AZStd::byte{} };

        size_t m_blockByteIndex{};
        size_t m_byteCount{};
    };

    inline constexpr void Sha1::Reset()
    {
        m_h[0] = 0x67452301;
        m_h[1] = 0xEFCDAB89;
        m_h[2] = 0x98BADCFE;
        m_h[3] = 0x10325476;
        m_h[4] = 0xC3D2E1F0;

        m_blockByteIndex = 0;
        m_byteCount = 0;
    }

    inline constexpr void Sha1::ProcessByte(AZStd::byte byte)
    {
        m_block[m_blockByteIndex++] = byte;
        ++m_byteCount;
        if (m_blockByteIndex == 64)
        {
            m_blockByteIndex = 0;
            ProcessBlock();
        }
    }

    inline constexpr void Sha1::ProcessBytes(AZStd::span<AZStd::byte const> byteSpan)
    {
        for (AZStd::byte byteElement : byteSpan)
        {
            ProcessByte(byteElement);
        }
    }

    inline constexpr void Sha1::ProcessBytes(AZStd::byte const* buffer, size_t byteCount)
    {
        ProcessBytes({ buffer, byteCount });
    }

    inline constexpr void Sha1::ProcessBlock()
    {
        AZ::u32 w[80]{ 0 };
        for (size_t i = 0; i < 16; ++i)
        {
            w[i] = static_cast<AZ::u32>(m_block[i * 4 + 0]) << 24;
            w[i] |= static_cast<AZ::u32>(m_block[i * 4 + 1]) << 16;
            w[i] |= static_cast<AZ::u32>(m_block[i * 4 + 2]) << 8;
            w[i] |= static_cast<AZ::u32>(m_block[i * 4 + 3]);
        }
        for (size_t i = 16; i < 80; ++i)
        {
            w[i] = LeftRotate((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]), 1);
        }

        AZ::u32 a = m_h[0];
        AZ::u32 b = m_h[1];
        AZ::u32 c = m_h[2];
        AZ::u32 d = m_h[3];
        AZ::u32 e = m_h[4];

        for (size_t i = 0; i < 80; ++i)
        {
            AZ::u32 f{};
            AZ::u32 k{};

            if (i < 20)
            {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            AZ::u32 temp = LeftRotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = LeftRotate(b, 30);
            b = a;
            a = temp;
        }

        m_h[0] += a;
        m_h[1] += b;
        m_h[2] += c;
        m_h[3] += d;
        m_h[4] += e;
    }

    inline constexpr void Sha1::GetDigest(DigestType digest)
    {
        size_t bitCount = m_byteCount * 8;

        // append the bit '1' to the message
        ProcessByte(AZStd::byte{ 0x80 });

        // append k bits '0', where k is the minimum number >= 0
        // such that the resulting message length is congruent to 56 (mod 64)
        // check if there is enough space for padding and bit_count
        if (m_blockByteIndex > 56)
        {
            // finish this block
            while (m_blockByteIndex != 0)
            {
                ProcessByte(AZStd::byte{});
            }

            // one more block
            while (m_blockByteIndex < 56)
            {
                ProcessByte(AZStd::byte{});
            }
        }
        else
        {
            while (m_blockByteIndex < 56)
            {
                ProcessByte({});
            }
        }

        // append length of message (before pre-processing) 
        // as a 64-bit big-endian integer
        ProcessByte({});
        ProcessByte({});
        ProcessByte({});
        ProcessByte({});
        ProcessByte(static_cast<AZStd::byte>((bitCount >> 24) & 0xFF));
        ProcessByte(static_cast<AZStd::byte>((bitCount >> 16) & 0xFF));
        ProcessByte(static_cast<AZStd::byte>((bitCount >> 8) & 0xFF));
        ProcessByte(static_cast<AZStd::byte>((bitCount) & 0xFF));

        // get final digest
        digest[0] = m_h[0];
        digest[1] = m_h[1];
        digest[2] = m_h[2];
        digest[3] = m_h[3];
        digest[4] = m_h[4];
    }
} // namespace AZ
