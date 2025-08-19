/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef SIMU_CORE_PLATFORM_PLATFORM_H
#define SIMU_CORE_PLATFORM_PLATFORM_H

/**
 * SIMUCORE_PLATFORM_INFO
 * 0000 0000 0000 0000  0000 0000  0000 0000
 * 0-7   ARCH
 * 8-15  Platform
 * 16-23 SIMD
 */

#define SIMUCORE_ARCH_X86   (0x01)
#define SIMUCORE_ARCH_ARM32 (0x02)
#define SIMUCORE_ARCH_ARM64 (0x04)

#define SIMUCORE_ARCH_WIN32   (0x01 << 8)
#define SIMUCORE_ARCH_MACOS   (0x02 << 8)
#define SIMUCORE_ARCH_LINUX   (0x04 << 8)
#define SIMUCORE_ARCH_ANDROID (0x08 << 8)

#define SIMUCORE_SIMD_SSE41   (0x01 << 16)
#define SIMUCORE_SIMD_NEON    (0x02 << 16)

#define SIMUCORE_MSVC       (0x01 << 24)
#define SIMUCORE_CLANG      (0x02 << 24)
#define SIMUCORE_GCC        (0x04 << 24)

/**
 *           DEF         x64        amd64        arm32     arm64
 *  MSVC     _MSC_VER    _M_IX86    _M_X64       _M_ARM    _M_ARM64
 *  GCC      __GNUC__    __i386__   __x86_64__   __arm__   __aarch64__
 *  Clang    __clang__   __i386__   __x86_64__   __arm__   __aarch64__
 */
#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__)
#define SIMUCORE_ARCH SIMUCORE_ARCH_X86
#define SIMUCORE_FORCE_SSE
#elif defined(_M_ARM) || defined(__arm__)
#define SIMUCORE_ARCH SIMUCORE_ARCH_ARM32
// define SIMUPARTICLE_FORCE_NEON
#elif defined(_M_ARM64) || defined(__aarch64__)
#define SIMUCORE_ARCH SIMUCORE_ARCH_ARM64
// define SIMUPARTICLE_FORCE_NEON
#else
#error Invalid ARCH
#endif

#if defined(_MSC_VER)
#define SIMUCORE_COMPILER SIMUCORE_MSVC
#elif defined(__clang__)
#define SIMUCORE_COMPILER SIMUCORE_CLANG
#elif defined(__GNUC__)
#define SIMUCORE_COMPILER SIMUCORE_GCC
#endif

/**
 *                PUBLIC           64
 *  Windows       _WIN32           _WIN64
 *  macOS         __APPLE__        __LP64__
 *  Linux         __linux__        __LP64__
 *  Android       __ANDROID__      __LP64__
 */
#if defined(_WIN32)
#define SIMUCORE_PLATFORM SIMUCORE_ARCH_WIN32
#elif defined(__APPLE__)
#define SIMUCORE_PLATFORM SIMUCORE_ARCH_MACOS
#elif defined(__linux__)
#define SIMUCORE_PLATFORM SIMUCORE_ARCH_LINUX
#elif defined(__ANDROID__)
#define SIMUCORE_PLATFORM SIMUCORE_ARCH_ANDROID
#elif defined(__OHOS__) || defined(OHOS)
#define SIMUCORE_PLATFORM SIMUCORE_ARCH_OHOS
#else
#error Invalid Platform
#endif

/**
 * SIMD
 */
#if defined(SIMUCORE_FORCE_SSE) // defined(__SSE4_1__) || defined(SIMUCORE_FORCE_SSE)
#define SIMUCORE_SIMD SIMUCORE_SIMD_SSE41
#elif  defined(SIMUPARTICLE_FORCE_NEON) // defined(__ARM_NEON) || defined(SIMUPARTICLE_FORCE_NEON)
#define SIMUCORE_SIMD SIMUCORE_SIMD_NEON
#else
#define SIMUCORE_SIMD 0
#endif

#define SIMUCORE_PLATFORM_INFO (SIMUCORE_ARCH | SIMUCORE_PLATFORM | SIMUCORE_SIMD | SIMUCORE_COMPILER)

#define SIMUCORE_ENABLE_SIMD
#if ((SIMUCORE_PLATFORM_INFO & SIMUCORE_SIMD_SSE41) != 0)
#include <smmintrin.h>
#elif ((SIMUCORE_PLATFORM_INFO & SIMUCORE_SIMD_NEON) != 0)
#include <arm_neon.h>
#else
#undef SIMUCORE_ENABLE_SIMD
#endif

#if !defined(NOMINMAX)
#define NOMINMAX
#endif

#endif // SIMU_CORE_PLATFORM_PLATFORM_H
