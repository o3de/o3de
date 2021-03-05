// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


/////////////////////////////////////////////////////////////////////////
/// @file   atomic.h
///
/// @brief  Wrappers and utilities for atomics.
/////////////////////////////////////////////////////////////////////////

#pragma once

#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/platform.h>

#include <atomic>
#define OIIO_USE_STDATOMIC 1



OIIO_NAMESPACE_BEGIN

using std::atomic;
typedef atomic<int> atomic_int;
typedef atomic<long long> atomic_ll;



/// Atomically set avar to the minimum of its current value and bval.
template<typename T>
OIIO_FORCEINLINE void
atomic_min(atomic<T>& avar, const T& bval)
{
    do {
        T a = avar.load();
        if (a <= bval || avar.compare_exchange_weak(a, bval))
            break;
    } while (true);
}


/// Atomically set avar to the maximum of its current value and bval.
template<typename T>
OIIO_FORCEINLINE void
atomic_max(atomic<T>& avar, const T& bval)
{
    do {
        T a = avar.load();
        if (a >= bval || avar.compare_exchange_weak(a, bval))
            break;
    } while (true);
}



// Add atomically to a float and return the original value.
OIIO_FORCEINLINE float
atomic_fetch_add(atomic<float>& a, float f)
{
    do {
        float oldval = a.load();
        float newval = oldval + f;
        if (a.compare_exchange_weak(oldval, newval))
            return oldval;
    } while (true);
}


// Add atomically to a double and return the original value.
OIIO_FORCEINLINE double
atomic_fetch_add(atomic<double>& a, double f)
{
    do {
        double oldval = a.load();
        double newval = oldval + f;
        if (a.compare_exchange_weak(oldval, newval))
            return oldval;
    } while (true);
}


OIIO_NAMESPACE_END
