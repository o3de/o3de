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
#pragma once

#include "AzCore/std/parallel/atomic.h"

#if !defined(_RELEASE)
# define AZRHI_DEBUG 1
#else
# define AZRHI_DEBUG 0
#endif

#if AZRHI_DEBUG
# define AZRHI_ASSERT(x) \
    do {                        \
        if (!(x)) {             \
            __debugbreak(); }   \
    } while (false)
#else
# define AZRHI_ASSERT(x) (void)0
#endif

#define AZRHI_VERIFY(x) \
    do {                       \
        if (!(x)) {            \
            __debugbreak(); }  \
    } while (false)

namespace AzRHI
{
    inline void SIMDCopy(void* dst, const void* bytes, size_t registerCount)
    {
#if defined(_CPU_SSE)
        if ((uintptr_t(bytes) & 0xF) == 0u)
        {
            __m128* const __restrict sseDst = (__m128*)dst;
            const __m128* const __restrict sseSrc = (const __m128*)bytes;
            for (size_t i = 0; i < registerCount; i++)
            {
               sseDst[i] = sseSrc[i];
            }
        }
        else
#endif
        {
            const float* const __restrict copySource = (const float*)bytes;
            float* const __restrict copyDest = (float*)dst;
            for (size_t i = 0; i < registerCount * 4; i += 4)
            {
                copyDest[i] = copySource[i];
                copyDest[i + 1] = copySource[i + 1];
                copyDest[i + 2] = copySource[i + 2];
                copyDest[i + 3] = copySource[i + 3];
            }
        }
    }

    // BitScanReverse/BitScanForward available?

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(Base_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(LINUX)
#define HAS_BIT_SCAN_REVERSE_FORWARD 0
#elif defined(APPLE)
#define HAS_BIT_SCAN_REVERSE_FORWARD 0
#else
#define HAS_BIT_SCAN_REVERSE_FORWARD 1
#endif

#if !HAS_BIT_SCAN_REVERSE_FORWARD
    inline AZ::u32 ScanBitsReverse(AZ::u32 input)
    {
        signed mask = iszero((signed)input) - 1;
        signed clz = __builtin_clz(input);
        signed index = 31 - clz;
        return (index & mask) | (32 & ~mask);
    }
    inline AZ::u32 ScanBitsForward(AZ::u32 input)
    {
        signed mask = iszero((signed)input) - 1;
        signed ctz = __builtin_ctz(input);
        signed index = ctz;
        return (index & mask) | (32 & ~mask);
    }
#else
    inline AZ::u32 ScanBitsReverse(AZ::u32 input)
    {
        unsigned long index;
        signed result = BitScanReverse(&index, input);
        signed mask = iszero(result) - 1;
        return (index & mask) | (32 & ~mask);
    }
    inline AZ::u32 ScanBitsForward(AZ::u32 input)
    {
        unsigned long index;
        signed result = BitScanForward(&index, input);
        signed mask = iszero(result) - 1;
        return (index & mask) | (32 & ~mask);
    }
#endif

    class ReferenceCounted
    {
    public:
        ReferenceCounted()
            : m_refCount(0)
        { }

        ReferenceCounted(ReferenceCounted&& r)
            : m_refCount(r.m_refCount.load())
        {}

        ReferenceCounted& operator=(ReferenceCounted&& r)
        {
            m_refCount = r.m_refCount.load();
            return *this;
        }

        AZ::u32 AddRef()
        {
            return m_refCount++;
        }

        AZ::u32 Release()
        {
            AZ_Assert(m_refCount != 0, "Releasing an already released object");
            const AZ::u32 refCount = --m_refCount;
            if (refCount == 0)
            {
                delete this;
                return 0;
            }
            return refCount;
        }

    protected:
        virtual ~ReferenceCounted() {}

    private:
        AZStd::atomic_uint m_refCount;
    };
}
