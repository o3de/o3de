/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : various integer bit fiddling hacks


#pragma once

#include "CompileTimeAssert.h"
#include <AzCore/Casting/numeric_cast.h>

// Section dictionary
#if defined(AZ_RESTRICTED_PLATFORM)
#define BITFIDDLING_H_SECTION_TRAITS 1
#define BITFIDDLING_H_SECTION_INTEGERLOG2 2
#endif

// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION BITFIDDLING_H_SECTION_TRAITS
    #include AZ_RESTRICTED_FILE(BitFiddling_h)
#elif defined(LINUX) || defined(APPLE)
#define BITFIDDLING_H_TRAIT_HAS_COUNT_LEADING_ZEROS 1
#endif

#if BITFIDDLING_H_TRAIT_HAS_COUNT_LEADING_ZEROS
#define countLeadingZeros32(x)              __builtin_clz(x)
#else       // Windows implementation
ILINE uint32 countLeadingZeros32(uint32 x)
{
    DWORD result = 32 ^ 31; // assumes result is unmodified if _BitScanReverse returns 0
    _BitScanReverse(&result, x);
    PREFAST_SUPPRESS_WARNING(6102);
    result ^= 31; // needed because the index is from LSB (whereas all other implementations are from MSB)
    return result;
}
#endif

inline uint32 circularShift(uint32 nbits, uint32 i)
{
    return (i << nbits) | (i >> (32 - nbits));
}

template <typename T>
inline size_t countTrailingZeroes(T v)
{
    size_t n = 0;

    v = ~v & (v - 1);
    while (v)
    {
        ++n;
        v >>= 1;
    }

    return n;
}

// this function returns the integer logarithm of various numbers without branching
#define IL2VAL(mask, shift)         \
    c |= ((x & mask) != 0) * shift; \
    x >>= ((x & mask) != 0) * shift

template <typename TInteger>
inline bool IsPowerOfTwo(TInteger x)
{
    return (x & (x - 1)) == 0;
}

// compile time version of IsPowerOfTwo, useful for STATIC_CHECK
template <int nValue>
struct IsPowerOfTwoCompileTime
{
    enum
    {
        IsPowerOfTwo = ((nValue & (nValue - 1)) == 0)
    };
};

inline uint32 NextPower2(uint32 n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

inline uint8 IntegerLog2(uint8 x)
{
    uint8 c = 0;
    IL2VAL(0xf0, 4);
    IL2VAL(0xc, 2);
    IL2VAL(0x2, 1);
    return c;
}
inline uint16 IntegerLog2(uint16 x)
{
    uint16 c = 0;
    IL2VAL(0xff00, 8);
    IL2VAL(0xf0, 4);
    IL2VAL(0xc, 2);
    IL2VAL(0x2, 1);
    return c;
}

inline uint32 IntegerLog2(uint32 x)
{
    return 31 - countLeadingZeros32(x);
}

inline uint64 IntegerLog2(uint64 x)
{
    uint64 c = 0;
    IL2VAL(0xffffffff00000000ull, 32);
    IL2VAL(0xffff0000u, 16);
    IL2VAL(0xff00, 8);
    IL2VAL(0xf0, 4);
    IL2VAL(0xc, 2);
    IL2VAL(0x2, 1);
    return c;
}

#if defined(APPLE) || defined(LINUX)
inline unsigned long int IntegerLog2(unsigned long int x)
{
    #if defined(PLATFORM_64BIT)
    return IntegerLog2((uint64)x);
  #else
    return IntegerLog2((uint32)x);
  #endif
}
#endif
#undef IL2VAL

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION BITFIDDLING_H_SECTION_INTEGERLOG2
    #include AZ_RESTRICTED_FILE(BitFiddling_h)
#endif

template <typename TInteger>
inline TInteger IntegerLog2_RoundUp(TInteger x)
{
    return 1 + IntegerLog2(x - 1);
}

static ILINE uint8 BitIndex(uint8 v)
{
    uint32 vv = v;
    return aznumeric_caster(31 - countLeadingZeros32(vv));
}

static ILINE uint8 BitIndex(uint16 v)
{
    uint32 vv = v;
    return aznumeric_caster(31 - countLeadingZeros32(vv));
}

static ILINE uint8 BitIndex(uint32 v)
{
    return aznumeric_caster(31 - countLeadingZeros32(v));
}

static ILINE uint8 CountBits(uint8 v)
{
    uint8 c = v;
    c = ((c >> 1) & 0x55) + (c & 0x55);
    c = ((c >> 2) & 0x33) + (c & 0x33);
    c = ((c >> 4) & 0x0f) + (c & 0x0f);
    return c;
}

static ILINE uint8 CountBits(uint16 v)
{
    return CountBits((uint8)(v  & 0xff)) +
           CountBits((uint8)((v >> 8) & 0xff));
}

static ILINE uint8 CountBits(uint32 v)
{
    return CountBits((uint8)(v          & 0xff)) +
           CountBits((uint8)((v >> 8) & 0xff)) +
           CountBits((uint8)((v >> 16) & 0xff)) +
           CountBits((uint8)((v >> 24) & 0xff));
}

// Branchless version of return v < 0 ? alt : v;
ILINE int32 Isel32(int32 v, int32 alt)
{
    return ((static_cast<int32>(v) >> 31) & alt) | ((static_cast<int32>(~v) >> 31) & v);
}

template <uint32 ILOG>
struct CompileTimeIntegerLog2
{
    static const uint32 result = 1 + CompileTimeIntegerLog2<(ILOG >> 1)>::result;
};
template <>
struct CompileTimeIntegerLog2<1>
{
    static const uint32 result = 0;
};
template <>
struct CompileTimeIntegerLog2<0>; // keep it undefined, we cannot represent "minus infinity" result

COMPILE_TIME_ASSERT(CompileTimeIntegerLog2<1>::result == 0);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2<2>::result == 1);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2<3>::result == 1);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2<4>::result == 2);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2<5>::result == 2);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2<255>::result == 7);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2<256>::result == 8);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2<257>::result == 8);

template <uint32 ILOG>
struct CompileTimeIntegerLog2_RoundUp
{
    static const uint32 result = CompileTimeIntegerLog2<ILOG>::result + ((ILOG & (ILOG - 1)) != 0);
};
template <>
struct CompileTimeIntegerLog2_RoundUp<0>; // we can return 0, but let's keep it undefined (same as CompileTimeIntegerLog2<0>)

COMPILE_TIME_ASSERT(CompileTimeIntegerLog2_RoundUp<1>::result == 0);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2_RoundUp<2>::result == 1);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2_RoundUp<3>::result == 2);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2_RoundUp<4>::result == 2);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2_RoundUp<5>::result == 3);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2_RoundUp<255>::result == 8);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2_RoundUp<256>::result == 8);
COMPILE_TIME_ASSERT(CompileTimeIntegerLog2_RoundUp<257>::result == 9);

// Character-to-bitfield mapping

inline uint32 AlphaBit(char c)
{
    return c >= 'a' && c <= 'z' ? 1 << (c - 'z' + 31) : 0;
}

inline uint64 AlphaBit64(char c)
{
    return (c >= 'a' && c <= 'z' ? 1U << (c - 'z' + 31) : 0) |
           (c >= 'A' && c <= 'Z' ? 1LL << (c - 'Z' + 63) : 0);
}

inline uint32 AlphaBits(uint32 wc)
{
    // Handle wide multi-char constants, can be evaluated at compile-time.
    return AlphaBit((char)wc)
           | AlphaBit((char)(wc >> 8))
           | AlphaBit((char)(wc >> 16))
           | AlphaBit((char)(wc >> 24));
}

inline uint32 AlphaBits(const char* s)
{
    // Handle string of any length.
    uint32 n = 0;
    while (*s)
    {
        n |= AlphaBit(*s++);
    }
    return n;
}

inline uint64 AlphaBits64(const char* s)
{
    // Handle string of any length.
    uint64 n = 0;
    while (*s)
    {
        n |= AlphaBit64(*s++);
    }
    return n;
}

// s should point to a buffer at least 65 chars long
inline void BitsAlpha64(uint64 n, char* s)
{
    for (int i = 0; n != 0; n >>= 1, i++)
    {
        if (n & 1)
        {
            *s++ = i < 32 ? static_cast<char>(i + 'z' - 31) : static_cast<char>(i + 'Z' - 63);
        }
    }
    *s++ = '\0';
}


// if hardware doesn't support 3Dc we can convert to DXT5 (different channels are used)
// with almost the same quality but the same memory requirements
inline void ConvertBlock3DcToDXT5(uint8 pDstBlock[16], const uint8 pSrcBlock[16])
{
    assert(pDstBlock != pSrcBlock);       // does not work in place

    // 4x4 block requires 8 bytes in DXT5 or 3DC

    // DXT5:  8 bit alpha0, 8 bit alpha1, 16*3 bit alpha lerp
    //        16bit col0, 16 bit col1 (R5G6B5 low byte then high byte), 16*2 bit color lerp

    //  3DC:  8 bit x0, 8 bit x1, 16*3 bit x lerp
    //        8 bit y0, 8 bit y1, 16*3 bit y lerp

    for (uint32 dwK = 0; dwK < 8; ++dwK)
    {
        pDstBlock[dwK] = pSrcBlock[dwK];
    }
    for (uint32 dwK = 8; dwK < 16; ++dwK)
    {
        pDstBlock[dwK] = 0;
    }

    // 6 bit green channel (highest bits)
    // by using all 3 channels with a slight offset we can get more precision but then a dot product would be needed in PS
    // because of bilinear filter we cannot just distribute bits to get perfect result
    uint16 colDst0 = (((uint16)pSrcBlock[8] + 2) >> 2) << 5;
    uint16 colDst1 = (((uint16)pSrcBlock[9] + 2) >> 2) << 5;

    bool bFlip = colDst0 <= colDst1;

    if (bFlip)
    {
        uint16 help = colDst0;
        colDst0 = colDst1;
        colDst1 = help;
    }

    bool bEqual = colDst0 == colDst1;

    // distribute bytes by hand to not have problems with endianess
    pDstBlock[8 + 0] = (uint8)colDst0;
    pDstBlock[8 + 1] = (uint8)(colDst0 >> 8);
    pDstBlock[8 + 2] = (uint8)colDst1;
    pDstBlock[8 + 3] = (uint8)(colDst1 >> 8);

    uint16* pSrcBlock16 = (uint16*)(pSrcBlock + 10);
    uint16* pDstBlock16 = (uint16*)(pDstBlock + 12);

    // distribute 16 3 bit values to 16 2 bit values (loosing LSB)
    for (uint32 dwK = 0; dwK < 16; ++dwK)
    {
        uint32 dwBit0 = dwK * 3 + 0;
        uint32 dwBit1 = dwK * 3 + 1;
        uint32 dwBit2 = dwK * 3 + 2;

        uint8 hexDataIn = (((pSrcBlock16[(dwBit2 >> 4)] >> (dwBit2 & 0xf)) & 1) << 2)         // get HSB
            | (((pSrcBlock16[(dwBit1 >> 4)] >> (dwBit1 & 0xf)) & 1) << 1)
            | ((pSrcBlock16[(dwBit0 >> 4)] >> (dwBit0 & 0xf)) & 1);                 // get LSB

        uint8 hexDataOut = 0;

        switch (hexDataIn)
        {
        case 0:
            hexDataOut = 0;
            break;                          // color 0
        case 1:
            hexDataOut = 1;
            break;                          // color 1

        case 2:
            hexDataOut = 0;
            break;                          // mostly color 0
        case 3:
            hexDataOut = 2;
            break;
        case 4:
            hexDataOut = 2;
            break;
        case 5:
            hexDataOut = 3;
            break;
        case 6:
            hexDataOut = 3;
            break;
        case 7:
            hexDataOut = 1;
            break;                          // mostly color 1

        default:
            assert(0);
        }

        if (bFlip)
        {
            if (hexDataOut < 2)
            {
                hexDataOut = 1 - hexDataOut;        // 0<->1
            }
            else
            {
                hexDataOut = 5 - hexDataOut;  // 2<->3
            }
        }

        if (bEqual)
        {
            if (hexDataOut == 3)
            {
                hexDataOut = 1;
            }
        }

        pDstBlock16[(dwK >> 3)] |= (hexDataOut << ((dwK & 0x7) << 1));
    }
}




// is a bit on in a new bit field, but off in an old bit field
static ILINE bool TurnedOnBit(unsigned bit, unsigned oldBits, unsigned newBits)
{
    return (newBits & bit) != 0 && (oldBits & bit) == 0;
}







inline uint32 cellUtilCountLeadingZero(uint32 x)
{
    uint32 y;
    uint32 n = 32;

    y = x >> 16;
    if (y != 0)
    {
        n = n - 16;
        x = y;
    }
    y = x >>  8;
    if (y != 0)
    {
        n = n -  8;
        x = y;
    }
    y = x >>  4;
    if (y != 0)
    {
        n = n -  4;
        x = y;
    }
    y = x >>  2;
    if (y != 0)
    {
        n = n -  2;
        x = y;
    }
    y = x >>  1;
    if (y != 0)
    {
        return n - 2;
    }
    return n - x;
}

inline uint32 cellUtilLog2(uint32 x)
{
    return 31 - cellUtilCountLeadingZero(x);
}




inline void convertSwizzle(uint8*& dst, const uint8*&    src,
    const uint32 SrcPitch, const uint32 depth,
    const uint32 xpos, const uint32 ypos,
    const uint32 SciX1, const uint32 SciY1,
    const uint32 SciX2, const uint32 SciY2,
    const uint32 level)
{
    if (level == 1)
    {
        switch (depth)
        {
        case 16:
            if (xpos >= SciX1 && xpos < SciX2 && ypos >= SciY1 && ypos < SciY2)
            {
                //                  *((uint32*&)dst)++ = ((uint32*)src)[ypos * width + xpos];
                //                  *((uint32*&)dst)++ = ((uint32*)src)[ypos * width + xpos+1];
                //                  *((uint32*&)dst)++ = ((uint32*)src)[ypos * width + xpos+2];
                //                  *((uint32*&)dst)++ = ((uint32*)src)[ypos * width + xpos+3];
                *((uint32*&)dst)++ = *((uint32*)(src + (ypos * SrcPitch + xpos * 16)));
                *((uint32*&)dst)++ = *((uint32*)(src + (ypos * SrcPitch + xpos * 16 + 4)));
                *((uint32*&)dst)++ = *((uint32*)(src + (ypos * SrcPitch + xpos * 16 + 8)));
                *((uint32*&)dst)++ = *((uint32*)(src + (ypos * SrcPitch + xpos * 16 + 12)));
            }
            else
            {
                ((uint32*&)dst) += 4;
            }
            break;
        case 8:
            if (xpos >= SciX1 && xpos < SciX2 && ypos >= SciY1 && ypos < SciY2)
            {
                *((uint32*&)dst)++ = *((uint32*)(src + (ypos * SrcPitch + xpos * 8)));
                *((uint32*&)dst)++ = *((uint32*)(src + (ypos * SrcPitch + xpos * 8 + 4)));
            }
            else
            {
                ((uint32*&)dst) += 2;
            }
            break;
        case 4:
            if (xpos >= SciX1 && xpos < SciX2 && ypos >= SciY1 && ypos < SciY2)
            {
                *((uint32*&)dst) = *((uint32*)(src + (ypos * SrcPitch + xpos * 4)));
            }
            dst += 4;
            break;
        case 3:
            if (xpos >= SciX1 && xpos < SciX2 && ypos >= SciY1 && ypos < SciY2)
            {
                *dst++ = src[ypos * SrcPitch + xpos * depth];
                *dst++ = src[ypos * SrcPitch + xpos * depth + 1];
                *dst++ = src[ypos * SrcPitch + xpos * depth + 2];
            }
            else
            {
                dst += 3;
            }
            break;
        case 1:
            if (xpos >= SciX1 && xpos < SciX2 && ypos >= SciY1 && ypos < SciY2)
            {
                *dst++ = src[ypos * SrcPitch + xpos * depth];
            }
            else
            {
                dst++;
            }
            break;
        default:
            assert(0);
        }
        return;
    }
    else
    {
        convertSwizzle(dst, src, SrcPitch, depth, xpos, ypos, SciX1, SciY1, SciX2, SciY2, level - 1);
        convertSwizzle(dst, src, SrcPitch, depth, xpos + (1U << (level - 2)), ypos, SciX1, SciY1, SciX2, SciY2, level - 1);
        convertSwizzle(dst, src, SrcPitch, depth, xpos, ypos + (1U << (level - 2)), SciX1, SciY1, SciX2, SciY2, level - 1);
        convertSwizzle(dst, src, SrcPitch, depth, xpos + (1U << (level - 2)), ypos + (1U << (level - 2)), SciX1, SciY1, SciX2, SciY2, level - 1);
    }
}




inline void Linear2Swizzle(uint8*   dst,
    const uint8*     src,
    const uint32 SrcPitch,
    const uint32 width,
    const uint32 height,
    const uint32 depth,
    const uint32 SciX1, const uint32 SciY1,
    const uint32 SciX2, const uint32 SciY2)
{
    src -= SciY1 * SrcPitch + SciX1 * depth;
    if (width == height)
    {
        convertSwizzle(dst, src, SrcPitch, depth, 0, 0, SciX1, SciY1, SciX2, SciY2, cellUtilLog2(width) + 1);
    }
    else
    if (width > height)
    {
        uint32 baseLevel    = cellUtilLog2(width) - (cellUtilLog2(width) - cellUtilLog2(height));
        for (uint32 i = 0; i < (1UL << (cellUtilLog2(width) - cellUtilLog2(height))); i++)
        {
            convertSwizzle(dst, src, SrcPitch, depth, (1U << baseLevel) * i, 0, SciX1, SciY1, SciX2, SciY2, baseLevel + 1);
        }
    }
    else
    //  if (width < height)//wtf
    {
        uint32 baseLevel    = cellUtilLog2(height) - (cellUtilLog2(height) - cellUtilLog2(width));
        for (uint32 i = 0; i < (1UL << (cellUtilLog2(height) - cellUtilLog2(width))); i++)
        {
            convertSwizzle(dst, src, SrcPitch, depth, 0, (1U << baseLevel) * i, SciX1, SciY1, SciX2, SciY2, baseLevel + 1);
        }
    }
}
