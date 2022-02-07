/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <BaseTypes.h>

//////////////////////////////////////////////////////////////////////////
// Endian support
//////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
// NEED_ENDIAN_SWAP is an older define still used in several places to toggle endian swapping.
// It is only used when reading files which are assumed to be little-endian.
// For legacy support, define it to swap on big endian platforms.
/////////////////////////////////////////////////////////////////////////////////////

typedef bool    EEndian;

#if defined(SYSTEM_IS_LITTLE_ENDIAN)
#   undef SYSTEM_IS_LITTLE_ENDIAN
#endif
#if defined(SYSTEM_IS_BIG_ENDIAN)
#   undef SYSTEM_IS_BIG_ENDIAN
#endif

#if 0 // no big-endian platforms right now, but keep the code
      // Big-endian platform
#   define SYSTEM_IS_LITTLE_ENDIAN 0
#   define SYSTEM_IS_BIG_ENDIAN 1
#else
// Little-endian platform
#   define SYSTEM_IS_LITTLE_ENDIAN 1
#   define SYSTEM_IS_BIG_ENDIAN 0
#endif

#if SYSTEM_IS_BIG_ENDIAN
// Big-endian platform. Swap to/from little.
    #define eBigEndian          false
    #define eLittleEndian       true
    #define NEED_ENDIAN_SWAP
#else
// Little-endian platform. Swap to/from big.
    #define eLittleEndian       false
    #define eBigEndian          true
    #undef NEED_ENDIAN_SWAP
#endif


enum EEndianness
{
    eEndianness_Little,
    eEndianness_Big,
#if SYSTEM_IS_BIG_ENDIAN
    eEndianness_Native = eEndianness_Big,
    eEndianness_NonNative = eEndianness_Little,
#else
    eEndianness_Native = eEndianness_Little,
    eEndianness_NonNative = eEndianness_Big,
#endif
};

/////////////////////////////////////////////////////////////////////////////////////

// SwapEndian function, using TypeInfo.
struct CTypeInfo;
void SwapEndian(const CTypeInfo& Info, size_t nSizeCheck, void* pData, size_t nCount = 1, bool bWriting = false);

// Default template utilizes TypeInfo.
template<class T>
inline void SwapEndianBase(T* t, size_t nCount = 1, bool bWriting = false)
{
    SwapEndian(TypeInfo(t), sizeof(T), t, nCount, bWriting);
}

/////////////////////////////////////////////////////////////////////////////////////
// SwapEndianBase functions.
// Always swap the data (the functions named SwapEndian swap based on an optional bSwapEndian parameter).
// The bWriting parameter must be specified in general when the output is for writing,
// but it matters only for types with bitfields.

// Overrides for base types.

template<>
inline void SwapEndianBase(char* p, size_t nCount, bool bWriting)
{
    (void)p;
    (void)nCount;
    (void)bWriting;
}

template<>
inline void SwapEndianBase(uint8* p, size_t nCount, bool bWriting)
{
    (void)p;
    (void)nCount;
    (void)bWriting;
}

template<>
inline void SwapEndianBase(int8* p, size_t nCount, bool bWriting)
{
    (void)p;
    (void)nCount;
    (void)bWriting;
}

template<>
inline void SwapEndianBase(uint16* p, size_t nCount, bool bWriting)
{
    (void)bWriting;
    for (; nCount-- > 0; p++)
    {
        *p = (uint16) (((*p >> 8) + (*p << 8)) & 0xFFFF);
    }
}

template<>
inline void SwapEndianBase(int16* p, size_t nCount, bool bWriting)
{
    (void)bWriting;
    SwapEndianBase((uint16*)p, nCount);
}

template<>
inline void SwapEndianBase(uint32* p, size_t nCount, bool bWriting)
{
    (void)bWriting;
    for (; nCount-- > 0; p++)
    {
        *p = (*p >> 24) + ((*p >> 8) & 0xFF00) + ((*p & 0xFF00) << 8) + (*p << 24);
    }
}

template<>
inline void SwapEndianBase(int32* p, size_t nCount, bool bWriting)
{
    (void)bWriting;
    SwapEndianBase((uint32*)p, nCount);
}

template<>
inline void SwapEndianBase(float* p, size_t nCount, bool bWriting)
{
    (void)bWriting;
    SwapEndianBase((uint32*)p, nCount);
}

template<>
inline void SwapEndianBase(uint64* p, size_t nCount, bool bWriting)
{
    (void)bWriting;
    for (; nCount-- > 0; p++)
    {
        *p = (*p >> 56) + ((*p >> 40) & 0xFF00) + ((*p >> 24) & 0xFF0000) + ((*p >> 8) & 0xFF000000)
            + ((*p & 0xFF000000) << 8) + ((*p & 0xFF0000) << 24) + ((*p & 0xFF00) << 40) + (*p << 56);
    }
}

template<>
inline void SwapEndianBase(int64* p, size_t nCount, bool bWriting)
{
    (void)bWriting;
    SwapEndianBase((uint64*)p, nCount);
}

template<>
inline void SwapEndianBase(double* p, size_t nCount, bool bWriting)
{
    (void)bWriting;
    SwapEndianBase((uint64*)p, nCount);
}

//---------------------------------------------------------------------------
// SwapEndian functions.
// bSwapEndian argument optional, and defaults to swapping from LittleEndian format.

template<class T>
inline void SwapEndian(T* t, size_t nCount, bool bSwapEndian = eLittleEndian)
{
    if (bSwapEndian)
    {
        SwapEndianBase(t, nCount);
    }
}

// Specify int and uint as well as size_t, to resolve overload ambiguities.
template<class T>
inline void SwapEndian(T* t, int nCount, bool bSwapEndian = eLittleEndian)
{
    if (bSwapEndian)
    {
        SwapEndianBase(t, nCount);
    }
}

#if defined(PLATFORM_64BIT)
template<class T>
inline void SwapEndian(T* t, unsigned int nCount, bool bSwapEndian = eLittleEndian)
{
    if (bSwapEndian)
    {
        SwapEndianBase(t, nCount);
    }
}
#endif

template<class T>
inline void SwapEndian(T& t, bool bSwapEndian = eLittleEndian)
{
    if (bSwapEndian)
    {
        SwapEndianBase(&t, 1);
    }
}
