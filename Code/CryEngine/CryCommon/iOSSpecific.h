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

// Description : iOS specific declarations


#ifndef CRYINCLUDE_CRYCOMMON_IOSSPECIFIC_H
#define CRYINCLUDE_CRYCOMMON_IOSSPECIFIC_H
#pragma once


#include "AppleSpecific.h"
#include <float.h>
#include <TargetConditionals.h>

#if TARGET_IPHONE_SIMULATOR
#define IOS_SIMULATOR
#include <xmmintrin.h>
#define _CPU_AMD64
#define _CPU_SSE
#else
#define _CPU_ARM
#define _CPU_NEON
#endif

// detect 64bit iOS
#if defined(__LP64__)
#define PLATFORM_64BIT
#endif

#if defined(_CPU_ARM) && defined(PLATFORM_64BIT)
#   define INTERLOCKED_COMPARE_EXCHANGE_128_NOT_SUPPORTED
#endif // defined(_CPU_ARM) && defined(PLATFORM_64BIT)

#ifndef MOBILE
#define MOBILE
#endif

// stubs for virtual keys, isn't used on iOS
#define VK_UP               0
#define VK_DOWN         0
#define VK_RIGHT        0
#define VK_LEFT         0
#define VK_CONTROL  0
#define VK_SCROLL       0


//#define USE_CRT 1
#if !defined(PLATFORM_64BIT)
#error "IOS build only supports the 64bit architecture"
#else
#define SIZEOF_PTR 8
typedef uint64_t threadID;
#endif

#endif // CRYINCLUDE_CRYCOMMON_IOSSPECIFIC_H
