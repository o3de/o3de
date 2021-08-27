/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/parallel/config.h>
#include <atomic>

namespace AZStd
{
    using std::atomic;
    using std::atomic_bool;
    using std::atomic_flag;
    using std::memory_order_acq_rel;
    using std::memory_order_acquire;
    using std::memory_order_release;
    using std::memory_order_relaxed;
    using std::memory_order_consume;
    using std::memory_order_seq_cst;
    using std::memory_order;

    using atomic_char = std::atomic<char>;
    using atomic_uchar = std::atomic<unsigned char>;
    using atomic_wchar_t = std::atomic<wchar_t>;
    using atomic_short = std::atomic<short>;
    using atomic_ushort = std::atomic<unsigned short>;
    using atomic_int = std::atomic<int>;
    using atomic_uint = std::atomic<unsigned int>;
    using atomic_long = std::atomic<long>;
    using atomic_ulong = std::atomic<unsigned long>;
    using atomic_llong = std::atomic<long long>;
    using atomic_ullong = std::atomic<unsigned long long>;
    using atomic_size_t = std::atomic<size_t>;
    using atomic_ptrdiff_t = std::atomic<ptrdiff_t>;
    using atomic_int8_t = std::atomic<int8_t>;
    using atomic_uint8_t = std::atomic<uint8_t>;
    using atomic_int16_t = std::atomic<int16_t>;
    using atomic_uint16_t = std::atomic<uint16_t>;
    using atomic_int32_t = std::atomic<int32_t>;
    using atomic_uint32_t = std::atomic<uint32_t>;
    using atomic_int64_t = std::atomic<int64_t>;
    using atomic_uint64_t = std::atomic<uint64_t>;

    using std::atomic_thread_fence;
    using std::atomic_signal_fence;
    using std::kill_dependency;

    using std::atomic_init;
    using std::atomic_is_lock_free;
    using std::atomic_store;
    using std::atomic_store_explicit;
    using std::atomic_load;
    using std::atomic_load_explicit;
    using std::atomic_exchange;
    using std::atomic_exchange_explicit;
    using std::atomic_compare_exchange_weak;
    using std::atomic_compare_exchange_weak_explicit;
    using std::atomic_compare_exchange_strong;
    using std::atomic_compare_exchange_strong_explicit;
    using std::atomic_fetch_add;
    using std::atomic_fetch_add_explicit;
    using std::atomic_fetch_sub;
    using std::atomic_fetch_sub_explicit;
    using std::atomic_fetch_and;
    using std::atomic_fetch_and_explicit;
    using std::atomic_fetch_or;
    using std::atomic_fetch_or_explicit;
    using std::atomic_fetch_xor;
    using std::atomic_fetch_xor_explicit;
    using std::atomic_flag_test_and_set;
    using std::atomic_flag_test_and_set_explicit;
    using std::atomic_flag_clear;
    using std::atomic_flag_clear_explicit;
}

///Specifies if integral types are lock-free, 0=never, 1=sometimes, 2=always
#define AZ_ATOMIC_INTEGRAL_LOCK_FREE ATOMIC_INTEGRAL_LOCK_FREE
///Specifies if address types are lock-free, 0=never, 1=sometimes, 2=always
#define AZ_ATOMIC_ADDRESS_LOCK_FREE ATOMIC_ADDRESS_LOCK_FREE
#define AZ_ATOMIC_BOOL_LOCK_FREE ATOMIC_BOOL_LOCK_FREE
#define AZ_ATOMIC_CHAR_LOCK_FREE ATOMIC_CHAR_LOCK_FREE
#define AZ_ATOMIC_CHAR16_T_LOCK_FREE ATOMIC_CHAR16_T_LOCK_FREE
#define AZ_ATOMIC_CHAR32_T_LOCK_FREE ATOMIC_CHAR32_T_LOCK_FREE
#define AZ_ATOMIC_WCHAR_T_LOCK_FREE ATOMIC_WCHAR_T_LOCK_FREE
#define AZ_ATOMIC_SHORT_LOCK_FREE ATOMIC_SHORT_LOCK_FREE
#define AZ_ATOMIC_INT_LOCK_FREE ATOMIC_INT_LOCK_FREE
#define AZ_ATOMIC_LONG_LOCK_FREE ATOMIC_LONG_LOCK_FREE
#define AZ_ATOMIC_LLONG_LOCK_FREE ATOMIC_LLONG_LOCK_FREE
#define AZ_ATOMIC_POINTER_LOCK_FREE ATOMIC_POINTER_LOCK_FREE
#define AZ_ATOMIC_FLAG_INIT ATOMIC_FLAG_INIT
#define AZ_ATOMIC_VAR_INIT ATOMIC_VAR_INIT
