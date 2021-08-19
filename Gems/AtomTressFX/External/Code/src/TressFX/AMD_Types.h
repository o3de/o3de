//---------------------------------------------------------------------------------------
// Type definitions and a few convenient macros
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
//#ifndef AMD_LIB_TYPES_H
//#define AMD_LIB_TYPES_H

namespace AMD
{
    typedef long long int64;
    typedef int       int32;
    typedef short     int16;
    typedef char      int8;
    typedef char      byte;

    typedef long long sint64;
    typedef int       sint32;
    typedef short     sint16;
    typedef char      sint8;
    typedef int       sint;
    typedef short     sshort;
    typedef char      sbyte;
    typedef char      schar;

    typedef unsigned long long uint64;
    typedef unsigned int       uint32;
    typedef unsigned short     uint16;
    typedef unsigned char      uint8;

    typedef float  real32;
    typedef double real64;

    typedef unsigned long long ulong;
    typedef unsigned int       uint;
    typedef unsigned short     ushort;
    typedef unsigned char      ubyte;
    typedef unsigned char      uchar;

    // The common return codes
    typedef enum RETURN_CODE_t {
        RETURN_CODE_SUCCESS,
        RETURN_CODE_FAIL,
        RETURN_CODE_INVALID_DEVICE,
        RETURN_CODE_INVALID_DEVICE_CONTEXT,
        RETURN_CODE_COUNT,
    } RETURN_CODE;
} // namespace AMD


#define AMD_FUNCTION_WIDEN2(x) L##x
#define AMD_FUNCTION_WIDEN(x) AMD_FUNCTION_WIDEN2(x)
#define AMD_FUNCTION_WIDE_NAME AMD_FUNCTION_WIDEN(__FUNCTION__)
#define AMD_FUNCTION_NAME __FUNCTION__

#define AMD_PITCHED_SIZE(x, y) ((x + y - 1) / y)

#define AMD_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define AMD_PI 3.141592654f
#define AMD_ROT_ANGLE (AMD_PI / 180.f)

#ifdef AMD_TRESSFX_DEBUG
#define AMD_COMPILE_TIME_ASSERT(condition, name)                                                  \
    unsigned char g_AMD_CompileTimeAssertExpression##name[(condition) ? 1 : -1];
#else
#define AMD_COMPILE_TIME_ASSERT(condition, name)
#endif

//#endif  // AMD_LIB_TYPES_H
