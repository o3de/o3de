/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
