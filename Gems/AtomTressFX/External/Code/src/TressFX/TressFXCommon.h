//---------------------------------------------------------------------------------------
// Constant buffer layouts.
//-------------------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

//#ifndef TRESSFXCOMMON_H_
//#define TRESSFXCOMMON_H_

#include <TressFX/AMD_Types.h>

#define TRESSFX_COLLISION_CAPSULES 0
#define TRESSFX_MAX_NUM_COLLISION_CAPSULES 8

#define TRESSFX_SIM_THREAD_GROUP_SIZE 64

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4201)  // disable warning C4201: nonstandard extension used : nameless struct/union
#endif

namespace AMD
{
    struct float2
    {
        union
        {
            struct
            {
                float x, y;
            };
            float v[2];
        };
    };
    struct float3
    {
        union
        {
            struct
            {
                float x, y, z;
            };
            float v[3];
        };
    };
    struct float4
    {
        union
        {
            struct
            {
                float x, y, z, w;
            };
            float v[4];
        };
    };
    struct float4x4
    {
        union
        {
            float4 r[4];
            float  m[16];
        };
    };
    struct uint2
    {
        union
        {
            struct
            {
                unsigned int x, y;
            };
            unsigned int v[2];
        };
    };
    struct uint3
    {
        union
        {
            struct
            {
                unsigned int x, y, z;
            };
            unsigned int v[3];
        };
    };
    struct uint4
    {
        union
        {
            struct
            {
                unsigned int x, y, z, w;
            };
            unsigned int v[4];
        };
    };
    struct sint2
    {
        union
        {
            struct
            {
                int x, y;
            };
            int v[2];
        };
    };
    struct sint3
    {
        union
        {
            struct
            {
                int x, y, z;
            };
            int v[3];
        };
    };
    struct sint4
    {
        union
        {
            struct
            {
                int x, y, z, w;
            };
            int v[4];
        };
    };
    struct sshort2
    {
        union
        {
            struct
            {
                short x, y;
            };
            short v[2];
        };
    };
    struct sshort3
    {
        union
        {
            struct
            {
                short x, y, z;
            };
            short v[3];
        };
    };
    struct sshort4
    {
        union
        {
            struct
            {
                short x, y, z, w;
            };
            short v[4];
        };
    };
    struct sbyte2
    {
        union
        {
            struct
            {
                signed char x, y;
            };
            signed char v[2];
        };
    };
    struct sbyte3
    {
        union
        {
            struct
            {
                signed char x, y, z;
            };
            signed char v[3];
        };
    };
    struct sbyte4
    {
        union
        {
            struct
            {
                signed char x, y, z, w;
            };
            signed char v[4];
        };
    };

} // namespace AMD

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#ifdef TRESSFX_NON_COPYABLE_MODERN_CPP

class TressFXNonCopyable
{
public:
    TressFXNonCopyable()  = default;
    ~TressFXNonCopyable() = default;

protected:
    TressFXNonCopyable(const TressFXNonCopyable&) = delete;
    void operator=(const TressFXNonCopyable&) = delete;
};

#else

class TressFXNonCopyable
{
public:
    TressFXNonCopyable() {}
    ~TressFXNonCopyable() {}

    TressFXNonCopyable(TressFXNonCopyable const&) = delete;
    TressFXNonCopyable& operator=(TressFXNonCopyable const&) = delete;
};

#endif

/// Computes the minimum
template<class Type>
inline Type TressFXMin(Type a, Type b)
{
    return (a < b) ? a : b;
}

//#endif  // TRESSFXCOMMON_H_
