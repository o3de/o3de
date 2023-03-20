/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

static_assert(sizeof(char) == 1);
static_assert(sizeof(float) == 4);
static_assert(sizeof(int) >= 4);


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

static_assert(sizeof(uchar)     == sizeof(schar));
static_assert(sizeof(ushort)    == sizeof(sshort));
static_assert(sizeof(uint)      == sizeof(sint));
static_assert(sizeof(ulong)     == sizeof(slong));
static_assert(sizeof(ulonglong) == sizeof(slonglong));

static_assert(sizeof(uchar)  <= sizeof(ushort));
static_assert(sizeof(ushort) <= sizeof(uint));
static_assert(sizeof(uint)   <= sizeof(ulong));
static_assert(sizeof(ulong)  <= sizeof(ulonglong));


typedef schar int8;
typedef schar sint8;
typedef uchar uint8;
static_assert(sizeof(uint8) == 1);
static_assert(sizeof(sint8) == 1);

typedef sshort int16;
typedef sshort sint16;
typedef ushort uint16;
static_assert(sizeof(uint16) == 2);
static_assert(sizeof(sint16) == 2);

typedef sint int32;
typedef sint sint32;
typedef uint uint32;
static_assert(sizeof(uint32) == 4);
static_assert(sizeof(sint32) == 4);

typedef slonglong int64;

#ifndef O3DE_INT64_DEFINED
#define O3DE_INT64_DEFINED
typedef slonglong sint64;
typedef ulonglong uint64;
static_assert(sizeof(uint64) == 8);
static_assert(sizeof(sint64) == 8);
#endif


typedef float  f32;
typedef double f64;
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);
