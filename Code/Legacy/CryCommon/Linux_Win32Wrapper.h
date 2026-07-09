/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(PLATFORM_64BIT)
#   define MEMORY_ALLOCATION_ALIGNMENT 16
#else
#   define MEMORY_ALLOCATION_ALIGNMENT 8
#endif

#if !defined(_CPU_SSE)
typedef int64 __m128;
#endif

//////////////////////////////////////////////////////////////////////////
// io.h stuff
#if !defined(ANDROID)
extern int errno;
#endif

//////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus

//////////////////////////////////////////////////////////////////////////

#if !defined(_CPU_SSE)
#define _MM_HINT_T0     (1)
#define _MM_HINT_T1     (2)
#define _MM_HINT_T2     (3)
#define _MM_HINT_NTA    (0)
inline void _mm_prefetch(const char*, int) { }
#endif // !_CPU_SSE

#endif //__cplusplus
