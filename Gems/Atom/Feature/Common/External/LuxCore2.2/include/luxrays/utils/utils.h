/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

// NOTE: this file is included in LuxCore so any external dependency must be
// avoided here

#ifndef _LUXRAYS_UTILS_H
#define _LUXRAYS_UTILS_H

#include <cmath>
#include <limits>
#include <iomanip>

#if defined (__linux__)
#include <pthread.h>
#endif

#if (defined(WIN32) && defined(_MSC_VER) && _MSC_VER < 1800)
#include <float.h>
#define isnan(a) _isnan(a)
#define isinf(f) (!_finite((f)))
#else
#define isnan(a) std::isnan(a)
#define isinf(f) std::isinf(f)
#endif

#if defined(WIN32)
#define isnanf(a) _isnan(a)
#endif

#if defined(__APPLE__)
#include <string>
#endif

#if !defined(__APPLE__) && !defined(__OpenBSD__) && !defined(__FreeBSD__)
#  include <malloc.h> // for _alloca, memalign
#  if !defined(WIN32) || defined(__CYGWIN__)
#    include <alloca.h>
#  else
#    define memalign(a,b) _aligned_malloc(b, a)
#    define alloca _alloca
#  endif
#else
#  include <stdlib.h>
#  if defined(__APPLE__)
#    define memalign(a,b) valloc(b)
#  elif defined(__OpenBSD__) || defined(__FreeBSD__)
#    define memalign(a,b) malloc(b)
#  endif
#endif

#include <sstream>

#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__) || defined(__OpenBSD__) || defined(__FreeBSD__)
#include <stddef.h>
#include <sys/time.h>
#elif defined (WIN32)
#include <windows.h>
#else
#error "Unsupported Platform !!!"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef INFINITY
#define INFINITY (std::numeric_limits<float>::infinity())
#endif

#ifndef INV_PI
#define INV_PI  0.31830988618379067154f
#endif

#ifndef INV_TWOPI
#define INV_TWOPI  0.15915494309189533577f
#endif

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
typedef unsigned long long u_longlong;

namespace luxrays {

inline double WallClockTime() {
#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__) || defined(__OpenBSD__) || defined(__FreeBSD__)
    struct timeval t;
    gettimeofday(&t, NULL);

    return t.tv_sec + t.tv_usec / 1000000.0;
#elif defined (WIN32)
    return GetTickCount() / 1000.0;
#else
#error "Unsupported Platform !!!"
#endif
}

template<class T> inline T Lerp(float t, T v1, T v2) {
    // Linear interpolation
    return v1 + t * (v2 - v1);
}

template<class T> inline T Cerp(float t, T v0, T v1, T v2, T v3) {
    // Cubic interpolation
    return v1 + .5f *
            t * (v2 - v0 +
                t * (2.f * v0 - 5.f * v1 + 4.f * v2 - v3 +
                    t * (3.f * (v1 - v2) + v3 - v0)));
}

template<class T> inline T Clamp(T val, T low, T high) {
    return val > low ? (val < high ? val : high) : low;
}

template<class T> inline void Swap(T &a, T &b) {
    const T tmp = a;
    a = b;
    b = tmp;
}

template<class T> inline T Max(T a, T b) {
    return a > b ? a : b;
}

template<class T> inline T Min(T a, T b) {
    return a < b ? a : b;
}

inline float Sgn(float a) {
    return a < 0.f ? -1.f : 1.f;
}

inline int Sgn(int a) {
    return a < 0 ? -1 : 1;
}

template<class T> inline T Sqr(T a) {
    return a * a;
}

inline int Round2Int(double val) {
    return static_cast<int>(val > 0. ? val + .5 : val - .5);
}

inline int Round2Int(float val) {
    return static_cast<int>(val > 0.f ? val + .5f : val - .5f);
}

inline unsigned int Round2UInt(double val) {
    return static_cast<unsigned int>(val > 0. ? val + .5 : 0.);
}

inline unsigned int Round2UInt(float val) {
    return static_cast<unsigned int>(val > 0.f ? val + .5f : 0.f);
}

template<class T> inline T Mod(T a, T b) {
    if (b == 0)
        b = 1;

    a %= b;
    if (a < 0)
        a += b;

    return a;
}

inline float Radians(float deg) {
    return (M_PI / 180.f) * deg;
}

inline float Degrees(float rad) {
    return (180.f / M_PI) * rad;
}

inline float Log2(float x) {
    return logf(x) / logf(2.f);
}

inline int Log2Int(float v) {
    return Round2Int(Log2(v));
}

inline bool IsPowerOf2(int v) {
    return (v & (v - 1)) == 0;
}

inline bool IsPowerOf2(unsigned int v) {
    return (v & (v - 1)) == 0;
}

template <class T> inline T RoundUp(const T a, const T b) {
    const unsigned int r = a % b;
    if (r == 0)
        return a;
    else
        return a + b - r;
}

template <class T> inline T RoundUpPow2(T v) {
    v--;

    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    return v+1;
}

inline unsigned int RoundUpPow2(unsigned int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v+1;
}

template<class T> inline int Float2Int(T val) {
    return static_cast<int>(val);
}

template<class T> inline unsigned int Float2UInt(T val) {
    return val >= 0 ? static_cast<unsigned int>(val) : 0;
}

inline int Floor2Int(double val) {
    return static_cast<int>(floor(val));
}

inline int Floor2Int(float val) {
    return static_cast<int>(floorf(val));
}

inline unsigned int Floor2UInt(double val) {
    return val > 0. ? static_cast<unsigned int>(floor(val)) : 0;
}

inline unsigned int Floor2UInt(float val) {
    return val > 0.f ? static_cast<unsigned int>(floorf(val)) : 0;
}

inline int Ceil2Int(double val) {
    return static_cast<int>(ceil(val));
}

inline int Ceil2Int(float val) {
    return static_cast<int>(ceilf(val));
}

inline unsigned int Ceil2UInt(double val) {
    return val > 0. ? static_cast<unsigned int>(ceil(val)) : 0;
}

inline unsigned int Ceil2UInt(float val) {
    return val > 0.f ? static_cast<unsigned int>(ceilf(val)) : 0;
}

inline bool Quadratic(float A, float B, float C, float *t0, float *t1) {
    // Find quadratic discriminant
    float discrim = B * B - 4.f * A * C;
    if (discrim < 0.f)
        return false;
    float rootDiscrim = sqrtf(discrim);
    // Compute quadratic _t_ values
    float q;
    if (B < 0)
        q = -.5f * (B - rootDiscrim);
    else
        q = -.5f * (B + rootDiscrim);
    *t0 = q / A;
    *t1 = C / q;
    if (*t0 > *t1)
        Swap(*t0, *t1);
    return true;
}

inline float SmoothStep(float min, float max, float value) {
    float v = Clamp((value - min) / (max - min), 0.f, 1.f);
    return v * v * (-2.f * v  + 3.f);
}

template <class T> int SignOf(T x)
{
    return (x > 0) - (x < 0);
}

template <class T> inline std::string ToString(const T &t) {
    std::ostringstream ss;
    ss << t;
    return ss.str();
}

inline std::string ToString(const float t) {
    std::ostringstream ss;
    ss << std::setprecision(std::numeric_limits<float>::digits10 + 1) << t;
    return ss.str();
}

inline std::string ToMemString(const size_t size) {
    return (size < 10000 ? (ToString(size) + "bytes") : (ToString(size / 1024) + "Kbytes"));
}

inline unsigned int UIntLog2(unsigned int value) {
    unsigned int l = 0;
    while (value >>= 1) l++;
    return l;
}

// Remap a value that is in sourceRange into targetRange.
template <class T> T inline Remap(const T value,
                                  const T sourceMin, const T sourceMax,
                                  const T targetMin, const T targetMax) {
    if (sourceMin == sourceMax)
        return sourceMin;

    return (value - sourceMin)
           * (targetMax - targetMin)
           / (sourceMax - sourceMin)
           + targetMin;
}

inline bool IsValid(float a) {
    return !isnan(a) && !isinf(a) && (a >= 0.f);
}

}

#endif  /* _LUXRAYS_UTILS_H */
