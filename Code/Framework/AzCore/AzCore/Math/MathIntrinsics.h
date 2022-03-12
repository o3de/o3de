/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(AZ_COMPILER_MSVC)
    #define az_ctz_u32(x)    _tzcnt_u32(x)
    #define az_ctz_u64(x)    _tzcnt_u64(x)
    #define az_clz_u32(x)    _lzcnt_u32(x)
    #define az_clz_u64(x)    _lzcnt_u64(x)
    #define az_popcnt_u32(x) __popcnt(x)
    #define az_popcnt_u64(x) __popcnt64(x)
#elif defined(AZ_COMPILER_CLANG)
    #define az_ctz_u32(x)    __builtin_ctz(x)
    #define az_ctz_u64(x)    __builtin_ctzll(x)
    #define az_clz_u32(x)    __builtin_clz(x)
    #define az_clz_u64(x)    __builtin_clzll(x)
    #define az_popcnt_u32(x) __builtin_popcount(x)
    #define az_popcnt_u64(x) __builtin_popcountll(x)
#else
    #error Count Leading Zeros, Count Trailing Zeros and Pop Count intrinsics isn't supported for this compiler
#endif
