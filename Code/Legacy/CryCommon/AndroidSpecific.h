/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Description : Specific to Android declarations, inline functions etc.


#if defined(__arm__) || defined(__aarch64__)
#define _CPU_ARM
#endif

#if defined(__aarch64__)
#define PLATFORM_64BIT
#endif

#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#define _CPU_NEON
#endif

#ifndef MOBILE
#define MOBILE
#endif

//////////////////////////////////////////////////////////////////////////
// Standard includes.
//////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <stdint.h>
#include <fcntl.h>
#include <float.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <sys/socket.h>
#include <errno.h>
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Define platform independent types.
//////////////////////////////////////////////////////////////////////////
#include "BaseTypes.h"

#define TARGET_DEFAULT_ALIGN (16U)

#ifdef _RELEASE
    #define __debugbreak()
#else
    #define __debugbreak() __builtin_trap()
#endif

// there is no __finite in android, only variants of isfinite
#undef __finite
#if NDK_REV_MAJOR >= 16
    #define __finite isfinite
#else
    #define __finite __isfinite
#endif

#include <android/api-level.h>

#if __ANDROID_API__ == 19
    // The following were apparently introduced in API 21, however in earlier versions of the
    // platform specific headers they were defines.  In the move to unified headers, the follwoing
    // defines were removed from stat.h
    #ifndef stat64
        #define stat64 stat
    #endif

    #ifndef fstat64
        #define fstat64 fstat
    #endif

    #ifndef lstat64
        #define lstat64 lstat
    #endif
#endif // __ANDROID_API__ == 19
