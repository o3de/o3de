/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_BASETYPES_H
#define CRYINCLUDE_CRYCOMMON_BASETYPES_H
#pragma once

#include "CompileTimeAssert.h"


COMPILE_TIME_ASSERT(sizeof(char) == 1);
COMPILE_TIME_ASSERT(sizeof(float) == 4);
COMPILE_TIME_ASSERT(sizeof(int) >= 4);


typedef unsigned char  uchar;
typedef signed   char  schar;

typedef unsigned short ushort;
typedef signed   short sshort;

#if !defined(CLANG_FIX_UINT_REDEF)
typedef unsigned int   uint;
#endif
typedef signed   int   sint;

typedef unsigned long  ulong;
typedef signed   long  slong;

typedef unsigned long long ulonglong;
typedef signed   long long slonglong;

COMPILE_TIME_ASSERT(sizeof(uchar)     == sizeof(schar));
COMPILE_TIME_ASSERT(sizeof(ushort)    == sizeof(sshort));
COMPILE_TIME_ASSERT(sizeof(uint)      == sizeof(sint));
COMPILE_TIME_ASSERT(sizeof(ulong)     == sizeof(slong));
COMPILE_TIME_ASSERT(sizeof(ulonglong) == sizeof(slonglong));

COMPILE_TIME_ASSERT(sizeof(uchar)  <= sizeof(ushort));
COMPILE_TIME_ASSERT(sizeof(ushort) <= sizeof(uint));
COMPILE_TIME_ASSERT(sizeof(uint)   <= sizeof(ulong));
COMPILE_TIME_ASSERT(sizeof(ulong)  <= sizeof(ulonglong));


typedef schar int8;
typedef schar sint8;
typedef uchar uint8;
COMPILE_TIME_ASSERT(sizeof(uint8) == 1);
COMPILE_TIME_ASSERT(sizeof(sint8) == 1);

typedef sshort int16;
typedef sshort sint16;
typedef ushort uint16;
COMPILE_TIME_ASSERT(sizeof(uint16) == 2);
COMPILE_TIME_ASSERT(sizeof(sint16) == 2);

typedef sint int32;
typedef sint sint32;
typedef uint uint32;
COMPILE_TIME_ASSERT(sizeof(uint32) == 4);
COMPILE_TIME_ASSERT(sizeof(sint32) == 4);

typedef slonglong int64;

#ifndef O3DE_INT64_DEFINED
#define O3DE_INT64_DEFINED
typedef slonglong sint64;
typedef ulonglong uint64;
COMPILE_TIME_ASSERT(sizeof(uint64) == 8);
COMPILE_TIME_ASSERT(sizeof(sint64) == 8);
#endif


typedef float  f32;
typedef double f64;
COMPILE_TIME_ASSERT(sizeof(f32) == 4);
COMPILE_TIME_ASSERT(sizeof(f64) == 8);

#endif // CRYINCLUDE_CRYCOMMON_BASETYPES_H
