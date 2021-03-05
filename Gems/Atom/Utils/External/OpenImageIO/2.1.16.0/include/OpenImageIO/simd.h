// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

/// @file  simd.h
///
/// @brief Classes for SIMD processing.
///
/// Nice references for all the Intel intrinsics (SSE*, AVX*, etc.):
///   https://software.intel.com/sites/landingpage/IntrinsicsGuide/
///
/// It helped me a lot to peruse the source of these packages:
///   Syrah:     https://github.com/boulos/syrah
///   Embree:    https://github.com/embree
///   Vectorial: https://github.com/scoopr/vectorial
///
/// To find out which CPU features you have:
///   Linux: cat /proc/cpuinfo
///   OSX:   sysctl machdep.cpu.features
///
/// Additional web resources:
///   http://www.codersnotes.com/notes/maths-lib-2016/

// clang-format off

#pragma once

#include <algorithm>
#include <cstring>

#include <OpenEXR/ImathVec.h>
#include <OpenEXR/ImathMatrix.h>

#include <OpenImageIO/dassert.h>
#include <OpenImageIO/platform.h>


//////////////////////////////////////////////////////////////////////////
// Sort out which SIMD capabilities we have and set definitions
// appropriately. This is mostly for internal (within this file) use,
// but client applications using this header may find a few of the macros
// we define to be useful:
//
// OIIO_SIMD : Will be 0 if no hardware SIMD support is specified. If SIMD
//             hardware is available, this will hold the width in number of
//             float SIMD "lanes" of widest SIMD registers available. For
//             example, OIIO_SIMD will be 4 if vfloat4/vint4/vbool4 are
//             hardware accelerated, 8 if vfloat8/vint8/vbool8 are accelerated,
//             etc. Using SIMD classes wider than this should work (will be
//             emulated with narrower SIMD or scalar operations), but is not
//             expected to have high performance.
// OIIO_SIMD_SSE : if Intel SSE is supported, this will be nonzero,
//             specifically 2 for SSE2, 3 for SSSE3, 4 for SSE4.1 or
//             higher (including AVX).
// OIIO_SIMD_AVX : If Intel AVX is supported, this will be nonzero, and
//             specifically 1 for AVX (1.0), 2 for AVX2, 512 for AVX512f.
// OIIO_SIMD_NEON : If ARM NEON is supported, this will be nonzero.
// OIIO_SIMD_MAX_SIZE : holds the width in bytes of the widest SIMD
//             available (generally will be OIIO_SIMD*4).
// OIIO_SIMD4_ALIGN : macro for best alignment of 4-wide SIMD values in mem.
// OIIO_SIMD8_ALIGN : macro for best alignment of 8-wide SIMD values in mem.
// OIIO_SIMD16_ALIGN : macro for best alignment of 16-wide SIMD values in mem.
// OIIO_SIMD_HAS_MATRIX4 : nonzero if matrix44 is defined
// OIIO_SIMD_HAS_SIMD8 : nonzero if vfloat8, vint8, vbool8 are defined
// OIIO_SIMD_HAS_SIMD16 : nonzero if vfloat16, vint16, vbool16 are defined

#if defined(_WIN32)
#  include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#  include <x86intrin.h>
#elif defined(__GNUC__) && defined(__ARM_NEON__)
#  include <arm_neon.h>
#endif

// Disable SSE for 32 bit Windows patforms, it's unreliable and hard for us
// to test thoroughly. We presume that anybody needing high performance
// badly enough to want SIMD also is on a 64 bit CPU.
#if defined(_WIN32) && defined(__i386__) && !defined(__x86_64__) && !defined(OIIO_NO_SSE)
#define OIIO_NO_SSE 1
#endif

#if (defined(__SSE2__) || (_MSC_VER >= 1300 && !_M_CEE_PURE)) && !defined(OIIO_NO_SSE)
#  if (defined(__SSE4_1__) || defined(__SSE4_2__))
#    define OIIO_SIMD_SSE 4
      /* N.B. We consider both SSE4.1 and SSE4.2 to be "4". There are a few
       * instructions specific to 4.2, but they are all related to string
       * comparisons and CRCs, which don't currently seem relevant to OIIO,
       * so for simplicity, we sweep this difference under the rug.
       */
#  elif defined(__SSSE3__)
#    define OIIO_SIMD_SSE 3
     /* N.B. We only use OIIO_SIMD_SSE = 3 when fully at SSSE3. In theory,
      * there are a few older architectures that are SSE3 but not SSSE3,
      * and this simplification means that these particular old platforms
      * will only get SSE2 goodness out of our code. So be it. Anybody who
      * cares about performance is probably using a 64 bit machine that's
      * SSE 4.x or AVX by now.
      */
#  else
#    define OIIO_SIMD_SSE 2
#  endif
#  define OIIO_SIMD 4
#  define OIIO_SIMD_MAX_SIZE_BYTES 16
#  define OIIO_SIMD4_ALIGN OIIO_ALIGN(16)
#  define OIIO_SSE_ALIGN OIIO_ALIGN(16)
#else
#  define OIIO_SIMD_SSE 0
#endif

#if defined(__AVX__) && !defined(OIIO_NO_AVX)
   // N.B. Any machine with AVX will also have SSE
#  if defined(__AVX2__) && !defined(OIIO_NO_AVX2)
#    define OIIO_SIMD_AVX 2
#  else
#    define OIIO_SIMD_AVX 1
#  endif
#  undef OIIO_SIMD
#  define OIIO_SIMD 8
#  undef OIIO_SIMD_MAX_SIZE_BYTES
#  define OIIO_SIMD_MAX_SIZE_BYTES 32
#  define OIIO_SIMD8_ALIGN OIIO_ALIGN(32)
#  define OIIO_AVX_ALIGN OIIO_ALIGN(32)
#  if defined(__AVX512F__)
#    undef OIIO_SIMD_AVX
#    define OIIO_SIMD_AVX 512
#    undef OIIO_SIMD_MAX_SIZE_BYTES
#    define OIIO_SIMD_MAX_SIZE_BYTES 64
#    undef OIIO_SIMD
#    define OIIO_SIMD 16
#    define OIIO_SIMD16_ALIGN OIIO_ALIGN(64)
#    define OIIO_AVX512_ALIGN OIIO_ALIGN(64)
#    define OIIO_AVX512F_ENABLED 1
#  endif
#  if defined(__AVX512DQ__)
#    define OIIO_AVX512DQ_ENABLED 1   /* Doubleword and quadword */
#  else
#    define OIIO_AVX512DQ_ENABLED 0
#  endif
#  if defined(__AVX512PF__)
#    define OIIO_AVX512PF_ENABLED 1   /* Prefetch */
#  else
#    define OIIO_AVX512PF_ENABLED 0
#  endif
#  if defined(__AVX512ER__)
#    define OIIO_AVX512ER_ENABLED 1   /* Exponential & reciprocal */
#  else
#    define OIIO_AVX512ER_ENABLED 0
#  endif
#  if defined(__AVX512CD__)
#    define OIIO_AVX512CD_ENABLED 1   /* Conflict detection */
#  else
#    define OIIO_AVX512CD_ENABLED 0
#  endif
#  if defined(__AVX512BW__)
#    define OIIO_AVX512BW_ENABLED 1   /* Byte and word */
#  else
#    define OIIO_AVX512BW_ENABLED 0
#  endif
#  if defined(__AVX512VL__)
#    define OIIO_AVX512VL_ENABLED 1   /* Vector length extensions */
#  else
#    define OIIO_AVX512VL_ENABLED 0
#  endif
#else
#  define OIIO_SIMD_AVX 0
#  define OIIO_AVX512VL_ENABLED 0
#  define OIIO_AVX512DQ_ENABLED 0
#  define OIIO_AVX512PF_ENABLED 0
#  define OIIO_AVX512ER_ENABLED 0
#  define OIIO_AVX512CD_ENABLED 0
#  define OIIO_AVX512BW_ENABLED 0
#endif

#if defined(__FMA__)
#  define OIIO_FMA_ENABLED 1
#else
#  define OIIO_FMA_ENABLED 0
#endif
#if defined(__AVX512IFMA__)
#  define OIIO_AVX512IFMA_ENABLED 1
#else
#  define OIIO_AVX512IFMA_ENABLED 0
#endif

#if defined(__F16C__)
#  define OIIO_F16C_ENABLED 1
#else
#  define OIIO_F16C_ENABLED 0
#endif

// FIXME Future: support ARM Neon
// Uncomment this when somebody with Neon can verify it works
#if 0 && defined(__ARM_NEON__) && !defined(OIIO_NO_NEON)
#  define OIIO_SIMD 4
#  define OIIO_SIMD_NEON 1
#  define OIIO_SIMD_MAX_SIZE_BYTES 16
#  define OIIO_SIMD4_ALIGN OIIO_ALIGN(16)
#  define OIIO_SSE_ALIGN OIIO_ALIGN(16)
#else
#  define OIIO_SIMD_NEON 0
#endif

#ifndef OIIO_SIMD
   // No SIMD available
#  define OIIO_SIMD 0
#  define OIIO_SIMD4_ALIGN
#  define OIIO_SIMD_MAX_SIZE_BYTES 16
#endif

#ifndef OIIO_SIMD8_ALIGN
#  define OIIO_SIMD8_ALIGN OIIO_SIMD4_ALIGN
#endif
#ifndef OIIO_SIMD16_ALIGN
#  define OIIO_SIMD16_ALIGN OIIO_SIMD8_ALIGN
#endif


// General features that client apps may want to test for, for conditional
// compilation. Will add to this over time as needed. Note that just
// because a feature is present doesn't mean it's fast -- HAS_SIMD8 means
// the vfloat8 class (and friends) are in this version of simd.h, but that's
// different from OIIO_SIMD >= 8, which means it's supported in hardware.
#define OIIO_SIMD_HAS_MATRIX4 1  /* matrix44 defined */
#define OIIO_SIMD_HAS_FLOAT8 1   /* DEPRECATED(1.8) */
#define OIIO_SIMD_HAS_SIMD8 1    /* vfloat8, vint8, vbool8 defined */
#define OIIO_SIMD_HAS_SIMD16 1   /* vfloat16, vint16, vbool16 defined */


// Embarrassing hack: Xlib.h #define's True and False!
#ifdef True
#    undef True
#endif
#ifdef False
#    undef False
#endif



OIIO_NAMESPACE_BEGIN

namespace simd {

//////////////////////////////////////////////////////////////////////////
// Forward declarations of our main SIMD classes

class vbool4;
class vint4;
class vfloat4;
class vfloat3;
class matrix44;
class vbool8;
class vint8;
class vfloat8;
class vbool16;
class vint16;
class vfloat16;

// Deprecated names -- remove these in 1.9
typedef vbool4 mask4;    // old name
typedef vbool4 bool4;
typedef vbool8 bool8;
typedef vint4 int4;
typedef vint8 int8;
typedef vfloat3 float3;
typedef vfloat4 float4;
typedef vfloat8 float8;



//////////////////////////////////////////////////////////////////////////
// Template magic to determine the raw SIMD types involved, and other
// things helpful for metaprogramming.

template <typename T, int N> struct simd_raw_t { struct type { T val[N]; }; };
template <int N> struct simd_bool_t { struct type { int val[N]; }; };

#if OIIO_SIMD_SSE
template<> struct simd_raw_t<int,4> { typedef __m128i type; };
template<> struct simd_raw_t<float,4> { typedef __m128 type; };
template<> struct simd_bool_t<4> { typedef __m128 type; };
#endif

#if OIIO_SIMD_AVX
template<> struct simd_raw_t<int,8> { typedef __m256i type; };
template<> struct simd_raw_t<float,8> { typedef __m256 type; };
template<> struct simd_bool_t<8> { typedef __m256 type; };
#endif

#if OIIO_SIMD_AVX >= 512
template<> struct simd_raw_t<int,16> { typedef __m512i type; };
template<> struct simd_raw_t<float,16> { typedef __m512 type; };
template<> struct simd_bool_t<16> { typedef __mmask16 type; };
#else
// Note: change in strategy for 16-wide SIMD: instead of int[16] for
// vbool16, it's just a plain old bitmask, and __mask16 for actual HW.
template<> struct simd_bool_t<16> { typedef uint16_t type; };
#endif

#if OIIO_SIMD_NEON
template<> struct simd_raw_t<int,4> { typedef int32x4 type; };
template<> struct simd_raw_t<float,4> { typedef float32x4_t type; };
template<> struct simd_bool_t<4> { typedef int32x4 type; };
#endif


/// Template to retrieve the vector type from the scalar. For example,
/// simd::VecType<int,4> will be vfloat4.
template<typename T,int elements> struct VecType {};
template<> struct VecType<int,1>   { typedef int type; };
template<> struct VecType<float,1> { typedef float type; };
template<> struct VecType<int,4>   { typedef vint4 type; };
template<> struct VecType<float,4>   { typedef vfloat4 type; };
template<> struct VecType<float,3> { typedef vfloat3 type; };
template<> struct VecType<bool,4>  { typedef vbool4 type; };
template<> struct VecType<int,8>   { typedef vint8 type; };
template<> struct VecType<float,8> { typedef vfloat8 type; };
template<> struct VecType<bool,8>  { typedef vbool8 type; };
template<> struct VecType<int,16>   { typedef vint16 type; };
template<> struct VecType<float,16> { typedef vfloat16 type; };
template<> struct VecType<bool,16>  { typedef vbool16 type; };

/// Template to retrieve the SIMD size of a SIMD type. Rigged to be 1 for
/// anything but our SIMD types.
template<typename T> struct SimdSize { static const int size = 1; };
template<> struct SimdSize<vint4>     { static const int size = 4; };
template<> struct SimdSize<vfloat4>   { static const int size = 4; };
template<> struct SimdSize<vfloat3>   { static const int size = 4; };
template<> struct SimdSize<vbool4>    { static const int size = 4; };
template<> struct SimdSize<vint8>     { static const int size = 8; };
template<> struct SimdSize<vfloat8>   { static const int size = 8; };
template<> struct SimdSize<vbool8>    { static const int size = 8; };
template<> struct SimdSize<vint16>    { static const int size = 16; };
template<> struct SimdSize<vfloat16>  { static const int size = 16; };
template<> struct SimdSize<vbool16>   { static const int size = 16; };

/// Template to retrieve the number of elements size of a SIMD type. Rigged
/// to be 1 for anything but our SIMD types.
template<typename T> struct SimdElements { static const int size = SimdSize<T>::size; };
template<> struct SimdElements<vfloat3>   { static const int size = 3; };

/// Template giving a printable name for each type
template<typename T> struct SimdTypeName { static const char *name() { return "unknown"; } };
template<> struct SimdTypeName<vfloat4>   { static const char *name() { return "vfloat4"; } };
template<> struct SimdTypeName<vint4>     { static const char *name() { return "vint4"; } };
template<> struct SimdTypeName<vbool4>    { static const char *name() { return "vbool4"; } };
template<> struct SimdTypeName<vfloat8>   { static const char *name() { return "vfloat8"; } };
template<> struct SimdTypeName<vint8>     { static const char *name() { return "vint8"; } };
template<> struct SimdTypeName<vbool8>    { static const char *name() { return "vbool8"; } };
template<> struct SimdTypeName<vfloat16>  { static const char *name() { return "vfloat16"; } };
template<> struct SimdTypeName<vint16>    { static const char *name() { return "vint16"; } };
template<> struct SimdTypeName<vbool16>   { static const char *name() { return "vbool16"; } };


//////////////////////////////////////////////////////////////////////////
// Macros helpful for making static constants in code.

# define OIIO_SIMD_FLOAT4_CONST(name,val) \
    static const OIIO_SIMD4_ALIGN float name[4] = { (val), (val), (val), (val) }
# define OIIO_SIMD_FLOAT4_CONST4(name,v0,v1,v2,v3) \
    static const OIIO_SIMD4_ALIGN float name[4] = { (v0), (v1), (v2), (v3) }
# define OIIO_SIMD_INT4_CONST(name,val) \
    static const OIIO_SIMD4_ALIGN int name[4] = { (val), (val), (val), (val) }
# define OIIO_SIMD_INT4_CONST4(name,v0,v1,v2,v3) \
    static const OIIO_SIMD4_ALIGN int name[4] = { (v0), (v1), (v2), (v3) }
# define OIIO_SIMD_UINT4_CONST(name,val) \
    static const OIIO_SIMD4_ALIGN uint32_t name[4] = { (val), (val), (val), (val) }
# define OIIO_SIMD_UINT4_CONST4(name,v0,v1,v2,v3) \
    static const OIIO_SIMD4_ALIGN uint32_t name[4] = { (v0), (v1), (v2), (v3) }

# define OIIO_SIMD_FLOAT8_CONST(name,val) \
    static const OIIO_SIMD8_ALIGN float name[8] = { (val), (val), (val), (val), \
                                                    (val), (val), (val), (val) }
# define OIIO_SIMD_FLOAT8_CONST8(name,v0,v1,v2,v3,v4,v5,v6,v7) \
    static const OIIO_SIMD8_ALIGN float name[8] = { (v0), (v1), (v2), (v3), \
                                                    (v4), (v5), (v6), (v7) }
# define OIIO_SIMD_INT8_CONST(name,val) \
    static const OIIO_SIMD8_ALIGN int name[8] = { (val), (val), (val), (val), \
                                                  (val), (val), (val), (val) }
# define OIIO_SIMD_INT8_CONST8(name,v0,v1,v2,v3,v4,v5,v6,v7) \
    static const OIIO_SIMD8_ALIGN int name[8] = { (v0), (v1), (v2), (v3), \
                                                  (v4), (v5), (v6), (v7) }
# define OIIO_SIMD_UINT8_CONST(name,val) \
    static const OIIO_SIMD8_ALIGN uint32_t name[8] = { (val), (val), (val), (val), \
                                                       (val), (val), (val), (val) }
# define OIIO_SIMD_UINT8_CONST8(name,v0,v1,v2,v3,v4,v5,v6,v7) \
    static const OIIO_SIMD8_ALIGN uint32_t name[8] = { (v0), (v1), (v2), (v3), \
                                                       (v4), (v5), (v6), (v7) }

# define OIIO_SIMD_VFLOAT16_CONST(name,val) \
    static const OIIO_SIMD16_ALIGN float name[16] = {           \
        (val), (val), (val), (val), (val), (val), (val), (val), \
        (val), (val), (val), (val), (val), (val), (val), (val) }
# define OIIO_SIMD_VFLOAT16_CONST16(name,v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15) \
    static const OIIO_SIMD16_ALIGN float name[16] = {           \
        (v0), (v1), (v2), (v3), (v4), (v5), (v6), (v7),         \
        (v8), (v9), (v10), (v11), (v12), (v13), (v14), (v15) }
# define OIIO_SIMD_INT16_CONST(name,val) \
    static const OIIO_SIMD16_ALIGN int name[16] = {             \
        (val), (val), (val), (val), (val), (val), (val), (val), \
        (val), (val), (val), (val), (val), (val), (val), (val) }
# define OIIO_SIMD_INT16_CONST16(name,v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15) \
    static const OIIO_SIMD16_ALIGN int name[16] = { \
        (v0), (v1), (v2), (v3), (v4), (v5), (v6), (v7), \
        (v8), (v9), (v10), (v11), (v12), (v13), (v14), (v15) }
# define OIIO_SIMD_UINT16_CONST(name,val) \
    static const OIIO_SIMD16_ALIGN uint32_t name[16] = {        \
        (val), (val), (val), (val), (val), (val), (val), (val), \
        (val), (val), (val), (val), (val), (val), (val), (val) }
# define OIIO_SIMD_UINT16_CONST16(name,v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15) \
    static const OIIO_SIMD16_ALIGN uint32_t name[16] = {        \
        (val), (val), (val), (val), (val), (val), (val), (val), \
        (val), (val), (val), (val), (val), (val), (val), (val) }


//////////////////////////////////////////////////////////////////////////
// Some macros just for use in this file (#undef-ed at the end) making
// it more succinct to express per-element operations.

#define SIMD_DO(x) for (int i = 0; i < elements; ++i) x
#define SIMD_CONSTRUCT(x) for (int i = 0; i < elements; ++i) m_val[i] = (x)
#define SIMD_CONSTRUCT_PAD(x) for (int i = 0; i < elements; ++i) m_val[i] = (x); \
                              for (int i = elements; i < paddedelements; ++i) m_val[i] = 0
#define SIMD_RETURN(T,x) T r; for (int i = 0; i < r.elements; ++i) r[i] = (x); return r
#define SIMD_RETURN_REDUCE(T,init,op) T r = init; for (int i = 0; i < v.elements; ++i) op; return r



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// The public declarations of the main SIMD classes follow: boolN, intN,
// floatN, matrix44.
//
// These class declarations are intended to be brief and self-documenting,
// and give all the information that users or client applications need to
// know to use these classes.
//
// No implementations are given inline except for the briefest, completely
// generic methods that don't have any architecture-specific overloads.
// After the class defintions, there will be an immense pile of full
// implementation definitions, which casual users are not expected to
// understand.
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// vbool4: An 4-vector whose elements act mostly like bools, accelerated by
/// SIMD instructions when available. This is what is naturally produced by
/// SIMD comparison operators on the vfloat4 and vint4 types.
class vbool4 {
public:
    static const char* type_name() { return "vbool4"; }
    typedef bool value_t;        ///< Underlying equivalent scalar value type
    enum { elements = 4 };       ///< Number of scalar elements
    enum { paddedelements = 4 }; ///< Number of scalar elements for full pad
    enum { bits = elements*32 }; ///< Total number of bits
    typedef simd_bool_t<4>::type simd_t;  ///< the native SIMD type used

    /// Default constructor (contents undefined)
    vbool4 () { }

    /// Construct from a single value (store it in all slots)
    vbool4 (bool a) { load(a); }

    explicit vbool4 (const bool *a);

    /// Construct from 4 bool values
    vbool4 (bool a, bool b, bool c, bool d) { load (a, b, c, d); }

    /// Copy construct from another vbool4
    vbool4 (const vbool4 &other) { m_simd = other.m_simd; }

    /// Construct from 4 int values
    vbool4 (int a, int b, int c, int d) {
        load (bool(a), bool(b), bool(c), bool(d));
    }

    /// Construct from a SIMD int (is each element nonzero?)
    vbool4 (const vint4 &i);

    /// Construct from the underlying SIMD type
    vbool4 (const simd_t& m) : m_simd(m) { }

    /// Return the raw SIMD type
    operator simd_t () const { return m_simd; }
    simd_t simd () const { return m_simd; }

    /// Extract the bitmask
    int bitmask () const;

    /// Convert from integer bitmask to a true vbool4
    static vbool4 from_bitmask (int bitmask);

    /// Set all components to false
    void clear ();

    /// Return a vbool4 the is 'false' for all values
    static const vbool4 False ();

    /// Return a vbool4 the is 'true' for all values
    static const vbool4 True ();

    /// Assign one value to all components
    const vbool4 & operator= (bool a) { load(a); return *this; }

    /// Assignment of another vbool4
    const vbool4 & operator= (const vbool4 & other);

    /// Component access (get)
    int operator[] (int i) const;

    /// Component access (set).
    void setcomp (int i, bool value);

    /// Component access (set).
    /// NOTE: avoid this unsafe construct. It will go away some day.
    int& operator[] (int i);

    /// Helper: load a single value into all components.
    void load (bool a);

    /// Helper: load separate values into each component.
    void load (bool a, bool b, bool c, bool d);

    /// Helper: store the values into memory as bools.
    void store (bool *values) const;

    /// Store the first n values into memory.
    void store (bool *values, int n) const;

    /// Logical/bitwise operators, component-by-component
    friend vbool4 operator! (const vbool4& a);
    friend vbool4 operator& (const vbool4& a, const vbool4& b);
    friend vbool4 operator| (const vbool4& a, const vbool4& b);
    friend vbool4 operator^ (const vbool4& a, const vbool4& b);
    friend vbool4 operator~ (const vbool4& a);
    friend const vbool4& operator&= (vbool4& a, const vbool4& b);
    friend const vbool4& operator|= (vbool4& a, const vbool4& b);
    friend const vbool4& operator^= (vbool4& a, const vbool4& b);

    /// Comparison operators, component by component
    friend vbool4 operator== (const vbool4& a, const vbool4& b);
    friend vbool4 operator!= (const vbool4& a, const vbool4& b);

    /// Stream output
    friend std::ostream& operator<< (std::ostream& cout, const vbool4 & a);

private:
    // The actual data representation
    union {
        simd_t m_simd;
        int m_val[paddedelements];
    };
};



/// Helper: shuffle/swizzle with constant (templated) indices.
/// Example: shuffle<1,1,2,2>(vbool4(a,b,c,d)) returns (b,b,c,c)
template<int i0, int i1, int i2, int i3> vbool4 shuffle (const vbool4& a);

/// shuffle<i>(a) is the same as shuffle<i,i,i,i>(a)
template<int i> vbool4 shuffle (const vbool4& a);

/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i> bool extract (const vbool4& a);

/// Helper: substitute val for a[i]
template<int i> vbool4 insert (const vbool4& a, bool val);

/// Logical reduction across all components.
bool reduce_and (const vbool4& v);
bool reduce_or (const vbool4& v);

// Are all/any/no components true?
bool all (const vbool4& v);
bool any (const vbool4& v);
bool none (const vbool4& v);

// It's handy to have this defined for regular bool as well
inline bool all (bool v) { return v; }



/// vbool8: An 8-vector whose elements act mostly like bools, accelerated by
/// SIMD instructions when available. This is what is naturally produced by
/// SIMD comparison operators on the vfloat8 and vint8 types.
class vbool8 {
public:
    static const char* type_name() { return "vbool8"; }
    typedef bool value_t;        ///< Underlying equivalent scalar value type
    enum { elements = 8 };       ///< Number of scalar elements
    enum { paddedelements = 8 }; ///< Number of scalar elements for full pad
    enum { bits = elements*32 }; ///< Total number of bits
    typedef simd_bool_t<8>::type simd_t;  ///< the native SIMD type used

    /// Default constructor (contents undefined)
    vbool8 () { }

    /// Construct from a single value (store it in all slots)
    vbool8 (bool a) { load (a); }

    explicit vbool8 (const bool *values);

    /// Construct from 8 bool values
    vbool8 (bool a, bool b, bool c, bool d, bool e, bool f, bool g, bool h);

    /// Copy construct from another vbool8
    vbool8 (const vbool8 &other) { m_simd = other.m_simd; }

    /// Construct from 8 int values
    vbool8 (int a, int b, int c, int d, int e, int f, int g, int h);

    /// Construct from a SIMD int (is each element nonzero?)
    vbool8 (const vint8 &i);

    /// Construct from two vbool4's
    vbool8 (const vbool4 &lo, const vbool4 &hi);

    /// Construct from the underlying SIMD type
    vbool8 (const simd_t& m) : m_simd(m) { }

    /// Return the raw SIMD type
    operator simd_t () const { return m_simd; }
    simd_t simd () const { return m_simd; }

    /// Extract the bitmask
    int bitmask () const;

    /// Convert from integer bitmask to a true vbool8
    static vbool8 from_bitmask (int bitmask);

    /// Set all components to false
    void clear ();

    /// Return a vbool8 the is 'false' for all values
    static const vbool8 False ();

    /// Return a vbool8 the is 'true' for all values
    static const vbool8 True ();

    /// Assign one value to all components
    const vbool8 & operator= (bool a);

    /// Assignment of another vbool8
    const vbool8 & operator= (const vbool8 & other);

    /// Component access (get)
    int operator[] (int i) const;

    /// Component access (set).
    void setcomp (int i, bool value);

    /// Component access (set).
    /// NOTE: avoid this unsafe construct. It will go away some day.
    int& operator[] (int i);

    /// Extract the lower percision vbool4
    vbool4 lo () const;

    /// Extract the higher percision vbool4
    vbool4 hi () const;

    /// Helper: load a single value into all components.
    void load (bool a);

    /// Helper: load separate values into each component.
    void load (bool a, bool b, bool c, bool d,
               bool e, bool f, bool g, bool h);

    /// Helper: store the values into memory as bools.
    void store (bool *values) const;

    /// Store the first n values into memory.
    void store (bool *values, int n) const;

    /// Logical/bitwise operators, component-by-component
    friend vbool8 operator! (const vbool8& a);
    friend vbool8 operator& (const vbool8& a, const vbool8& b);
    friend vbool8 operator| (const vbool8& a, const vbool8& b);
    friend vbool8 operator^ (const vbool8& a, const vbool8& b);
    friend vbool8 operator~ (const vbool8& a);
    friend const vbool8& operator&= (vbool8& a, const vbool8& b);
    friend const vbool8& operator|= (vbool8& a, const vbool8& b);
    friend const vbool8& operator^= (vbool8& a, const vbool8& b);

    /// Comparison operators, component by component
    friend vbool8 operator== (const vbool8& a, const vbool8& b);
    friend vbool8 operator!= (const vbool8& a, const vbool8& b);

    /// Stream output
    friend std::ostream& operator<< (std::ostream& cout, const vbool8 & a);

private:
    // The actual data representation
    union {
        simd_t m_simd;
        int m_val[paddedelements];
        simd_bool_t<4>::type m_4[2];
    };
};



/// Helper: shuffle/swizzle with constant (templated) indices.
/// Example: shuffle<1,1,2,2>(vbool4(a,b,c,d)) returns (b,b,c,c)
template<int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7>
vbool8 shuffle (const vbool8& a);

/// shuffle<i>(a) is the same as shuffle<i,i,i,i>(a)
template<int i> vbool8 shuffle (const vbool8& a);

/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i> bool extract (const vbool8& a);

/// Helper: substitute val for a[i]
template<int i> vbool8 insert (const vbool8& a, bool val);

/// Logical reduction across all components.
bool reduce_and (const vbool8& v);
bool reduce_or (const vbool8& v);

// Are all/any/no components true?
bool all (const vbool8& v);
bool any (const vbool8& v);
bool none (const vbool8& v);




/// vbool16: An 16-vector whose elements act mostly like bools, accelerated
/// by SIMD instructions when available. This is what is naturally produced
/// by SIMD comparison operators on the vfloat16 and vint16 types.
class vbool16 {
public:
    static const char* type_name() { return "vbool16"; }
    typedef bool value_t;        ///< Underlying equivalent scalar value type
    enum { elements = 16 };       ///< Number of scalar elements
    enum { paddedelements = 16 }; ///< Number of scalar elements for full pad
    enum { bits = 16 };           ///< Total number of bits
    typedef simd_bool_t<16>::type simd_t;  ///< the native SIMD type used

    /// Default constructor (contents undefined)
    vbool16 () { }

    /// Construct from a single value (store it in all slots)
    vbool16 (bool a) { load (a); }

    explicit vbool16 (int bitmask) { load_bitmask (bitmask); }

    explicit vbool16 (const bool *values);

    /// Construct from 16 bool values
    vbool16 (bool v0, bool v1, bool v2, bool v3, bool v4, bool v5, bool v6, bool v7,
            bool v8, bool v9, bool v10, bool v11, bool v12, bool v13, bool v14, bool v15);

    /// Copy construct from another vbool16
    vbool16 (const vbool16 &other) { m_simd = other.m_simd; }

    /// Construct from 16 int values
    vbool16 (int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7,
             int v8, int v9, int v10, int v11, int v12, int v13, int v14, int v15);

    /// Construct from a SIMD int (is each element nonzero?)
    vbool16 (const vint16 &i);

    /// Construct from two vbool8's
    vbool16 (const vbool8 &lo, const vbool8 &hi);

    /// Construct from four vbool4's
    vbool16 (const vbool4 &b4a, const vbool4 &b4b, const vbool4 &b4c, const vbool4 &b4d);

    /// Construct from the underlying SIMD type
    vbool16 (const simd_t& m) : m_simd(m) { }

    /// Return the raw SIMD type
    operator simd_t () const { return m_simd; }
    simd_t simd () const { return m_simd; }

    int bitmask () const;

    /// Convert from integer bitmask to a true vbool16
    static vbool16 from_bitmask (int bitmask) { return vbool16(bitmask); }

    /// Set all components to false
    void clear ();

    /// Return a vbool16 the is 'false' for all values
    static const vbool16 False ();

    /// Return a vbool16 the is 'true' for all values
    static const vbool16 True ();

    /// Assign one value to all components
    const vbool16 & operator= (bool a);

    /// Assignment of another vbool16
    const vbool16 & operator= (const vbool16 & other);

    /// Component access (get)
    int operator[] (int i) const;

    /// Component access (set).
    void setcomp (int i, bool value);

    /// Extract the lower percision vbool8
    vbool8 lo () const;

    /// Extract the higher percision vbool8
    vbool8 hi () const;

    /// Helper: load a single value into all components.
    void load (bool a);

    /// Helper: load separate values into each component.
    void load (bool v0, bool v1, bool v2, bool v3, bool v4, bool v5, bool v6, bool v7,
               bool v8, bool v9, bool v10, bool v11, bool v12, bool v13, bool v14, bool v15);

    /// Helper: load all components from a bitmask in an int.
    void load_bitmask (int a);

    /// Helper: store the values into memory as bools.
    void store (bool *values) const;

    /// Store the first n values into memory.
    void store (bool *values, int n) const;

    /// Logical/bitwise operators, component-by-component
    friend vbool4 operator! (const vbool4& a);
    friend vbool16 operator! (const vbool16& a);
    friend vbool16 operator& (const vbool16& a, const vbool16& b);
    friend vbool16 operator| (const vbool16& a, const vbool16& b);
    friend vbool16 operator^ (const vbool16& a, const vbool16& b);
    friend vbool16 operator~ (const vbool16& a);
    friend const vbool16& operator&= (vbool16& a, const vbool16& b);
    friend const vbool16& operator|= (vbool16& a, const vbool16& b);
    friend const vbool16& operator^= (vbool16& a, const vbool16& b);

    /// Comparison operators, component by component
    friend vbool16 operator== (const vbool16& a, const vbool16& b);
    friend vbool16 operator!= (const vbool16& a, const vbool16& b);

    /// Stream output
    friend std::ostream& operator<< (std::ostream& cout, const vbool16 & a);

private:
    // The actual data representation
    union {
        simd_t   m_simd;
        uint16_t m_bits;
    };
};



/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i> bool extract (const vbool16& a);

/// Helper: substitute val for a[i]
template<int i> vbool16 insert (const vbool16& a, bool val);

/// Logical reduction across all components.
bool reduce_and (const vbool16& v);
bool reduce_or (const vbool16& v);

// Are all/any/no components true?
bool all (const vbool16& v);
bool any (const vbool16& v);
bool none (const vbool16& v);





/// Integer 4-vector, accelerated by SIMD instructions when available.
class vint4 {
public:
    static const char* type_name() { return "vint4"; }
    typedef int value_t;      ///< Underlying equivalent scalar value type
    enum { elements = 4 };    ///< Number of scalar elements
    enum { paddedelements =4 }; ///< Number of scalar elements for full pad
    enum { bits = 128 };      ///< Total number of bits
    typedef simd_raw_t<int,elements>::type simd_t;  ///< the native SIMD type used
    typedef vbool4 vbool_t;   ///< bool type of the same length
    typedef vfloat4 vfloat_t; ///< float type of the same length
    typedef vint4 vint_t;     ///< int type of the same length
    typedef vbool4 bool_t;   // old name (deprecated 1.8)
    typedef vfloat4 float_t; // old name (deprecated 1.8)

    /// Default constructor (contents undefined)
    vint4 () { }

    /// Construct from a single value (store it in all slots)
    vint4 (int a);

    /// Construct from 2 values -- (a,a,b,b)
    vint4 (int a, int b);

    /// Construct from 4 values
    vint4 (int a, int b, int c, int d);

    /// Construct from a pointer to values
    vint4 (const int *vals);

    /// Construct from a pointer to unsigned short values
    explicit vint4 (const unsigned short *vals);

    /// Construct from a pointer to signed short values
    explicit vint4 (const short *vals);

    /// Construct from a pointer to unsigned char values (0 - 255)
    explicit vint4 (const unsigned char *vals);

    /// Construct from a pointer to signed char values (-128 - 127)
    explicit vint4 (const char *vals);

    /// Copy construct from another vint4
    vint4 (const vint4 & other) { m_simd = other.m_simd; }

    /// Convert a vfloat to an vint. Equivalent to i = (int)f;
    explicit vint4 (const vfloat4& f); // implementation below

    /// Construct from the underlying SIMD type
    vint4 (const simd_t& m) : m_simd(m) { }

    /// Return the raw SIMD type
    operator simd_t () const { return m_simd; }
    simd_t simd () const { return m_simd; }

    /// Return a pointer to the underlying scalar type
    const value_t* data () const { return (const value_t*)this; }
    value_t* data () { return (value_t*)this; }

    /// Sset all components to 0
    void clear () ;

    /// Return an vint4 with all components set to 0
    static const vint4 Zero ();

    /// Return an vint4 with all components set to 1
    static const vint4 One ();

    /// Return an vint4 with all components set to -1 (aka 0xffffffff)
    static const vint4 NegOne ();

    /// Return an vint4 with incremented components (e.g., 0,1,2,3).
    /// Optional arguments can give a non-zero starting point and step size.
    static const vint4 Iota (int start=0, int step=1);

    /// Return an vint4 with "geometric" iota: (1, 2, 4, 8).
    static const vint4 Giota ();

    /// Assign one value to all components.
    const vint4 & operator= (int a);

    /// Assignment from another vint4
    const vint4 & operator= (const vint4& other) ;

    /// Component access (get)
    int operator[] (int i) const;

    /// Component access (set)
    int& operator[] (int i);

    /// Component access (set).
    void setcomp (int i, int value);

    value_t x () const;
    value_t y () const;
    value_t z () const;
    value_t w () const;
    void set_x (value_t val);
    void set_y (value_t val);
    void set_z (value_t val);
    void set_w (value_t val);

    /// Helper: load a single int into all components
    void load (int a);

    /// Helper: load separate values into each component.
    void load (int a, int b, int c, int d);

    /// Load from an array of 4 values
    void load (const int *values);

    void load (const int *values, int n) ;

    /// Load from an array of 4 unsigned short values, convert to vint4
    void load (const unsigned short *values) ;

    /// Load from an array of 4 unsigned short values, convert to vint4
    void load (const short *values);

    /// Load from an array of 4 unsigned char values, convert to vint4
    void load (const unsigned char *values);

    /// Load from an array of 4 unsigned char values, convert to vint4
    void load (const char *values);

    /// Store the values into memory
    void store (int *values) const;

    /// Store the first n values into memory
    void store (int *values, int n) const;

    /// Store the least significant 16 bits of each element into adjacent
    /// unsigned shorts.
    void store (unsigned short *values) const;

    /// Store the least significant 8 bits of each element into adjacent
    /// unsigned chars.
    void store (unsigned char *values) const;

    /// Masked load -- read from values[] where mask is 1, load zero where
    /// mask is 0.
    void load_mask (int mask, const value_t *values);
    void load_mask (const vbool_t& mask, const value_t *values);

    /// Masked store -- write to values[] where mask is enabled, don't
    /// touch values[] where it's not.
    void store_mask (int mask, value_t *values) const;
    void store_mask (const vbool_t& mask, value_t *values) const;

    /// Load values from addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void gather (const value_t *baseptr, const vint_t& vindex);
    /// Gather elements defined by the mask, leave others unchanged.
    template<int scale=4>
    void gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex);
    template<int scale=4>
    void gather_mask (int mask, const value_t *baseptr, const vint_t& vindex);

    /// Store values at addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void scatter (value_t *baseptr, const vint_t& vindex) const;
    /// Scatter elements defined by the mask
    template<int scale=4>
    void scatter_mask (const bool_t& mask, value_t *baseptr, const vint_t& vindex) const;
    template<int scale=4>
    void scatter_mask (int mask, value_t *baseptr, const vint_t& vindex) const;

    // Arithmetic operators (component-by-component)
    friend vint4 operator+ (const vint4& a, const vint4& b);
    friend vint4 operator- (const vint4& a);
    friend vint4 operator- (const vint4& a, const vint4& b);
    friend vint4 operator* (const vint4& a, const vint4& b);
    friend vint4 operator/ (const vint4& a, const vint4& b);
    friend vint4 operator% (const vint4& a, const vint4& b);
    friend const vint4 & operator+= (vint4& a, const vint4& b);
    friend const vint4 & operator-= (vint4& a, const vint4& b);
    friend const vint4 & operator*= (vint4& a, const vint4& b);
    friend const vint4 & operator/= (vint4& a, const vint4& b);
    friend const vint4 & operator%= (vint4& a, const vint4& b);
    // Bitwise operators (component-by-component)
    friend vint4 operator& (const vint4& a, const vint4& b);
    friend vint4 operator| (const vint4& a, const vint4& b);
    friend vint4 operator^ (const vint4& a, const vint4& b);
    friend const vint4& operator&= (vint4& a, const vint4& b);
    friend const vint4& operator|= (vint4& a, const vint4& b);
    friend const vint4& operator^= (vint4& a, const vint4& b);
    friend vint4 operator~ (const vint4& a);
    friend vint4 operator<< (const vint4& a, unsigned int bits);
    friend vint4 operator>> (const vint4& a, unsigned int bits);
    friend const vint4& operator<<= (vint4& a, unsigned int bits);
    friend const vint4& operator>>= (vint4& a, unsigned int bits);
    // Comparison operators (component-by-component)
    friend vbool4 operator== (const vint4& a, const vint4& b);
    friend vbool4 operator!= (const vint4& a, const vint4& b);
    friend vbool4 operator<  (const vint4& a, const vint4& b);
    friend vbool4 operator>  (const vint4& a, const vint4& b);
    friend vbool4 operator>= (const vint4& a, const vint4& b);
    friend vbool4 operator<= (const vint4& a, const vint4& b);

    /// Stream output
    friend std::ostream& operator<< (std::ostream& cout, const vint4 & a);

private:
    // The actual data representation
    union {
        simd_t  m_simd;
        value_t m_val[elements];
    };
};



// Shift right logical -- unsigned shift. This differs from operator>>
// in how it handles the sign bit.  (1<<31) >> 1 == (1<<31), but
// srl((1<<31),1) == 1<<30.
vint4 srl (const vint4& val, const unsigned int bits);

/// Helper: shuffle/swizzle with constant (templated) indices.
/// Example: shuffle<1,1,2,2>(vbool4(a,b,c,d)) returns (b,b,c,c)
template<int i0, int i1, int i2, int i3> vint4 shuffle (const vint4& a);

/// shuffle<i>(a) is the same as shuffle<i,i,i,i>(a)
template<int i> vint4 shuffle (const vint4& a);

/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i> int extract (const vint4& v);

/// The sum of all components, returned in all components.
vint4 vreduce_add (const vint4& v);

// Reduction across all components
int reduce_add (const vint4& v);
int reduce_and (const vint4& v);
int reduce_or (const vint4& v);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// and b (if mask[i] is true), i.e., mask[i] ? b[i] : a[i].
vint4 blend (const vint4& a, const vint4& b, const vbool4& mask);

/// Use a bool mask to select between `a` (if mask[i] is true) or 0 if
/// mask[i] is false), i.e., mask[i] ? a[i] : 0. Equivalent to
/// blend(0,a,mask).
vint4 blend0 (const vint4& a, const vbool4& mask);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// or 0 (if mask[i] is true), i.e., mask[i] ? 0 : a[i]. Equivalent to
/// blend(0,a,!mask), or blend(a,0,mask).
vint4 blend0not (const vint4& a, const vbool4& mask);

/// Select 'a' where mask is true, 'b' where mask is false. Sure, it's a
/// synonym for blend with arguments rearranged, but this is more clear
/// because the arguments are symmetric to scalar (cond ? a : b).
vint4 select (const vbool4& mask, const vint4& a, const vint4& b);

// Per-element math
vint4 abs (const vint4& a);
vint4 min (const vint4& a, const vint4& b);
vint4 max (const vint4& a, const vint4& b);

/// Circular bit rotate by s bits, for N values at once.
vint4 rotl (const vint4& x, const int s);
// DEPRECATED(2.1)
vint4 rotl32 (const vint4& x, const unsigned int k);

/// andnot(a,b) returns ((~a) & b)
vint4 andnot (const vint4& a, const vint4& b);

/// Bitcast back and forth to intN (not a convert -- move the bits!)
vint4 bitcast_to_int (const vbool4& x);
vint4 bitcast_to_int (const vfloat4& x);
vfloat4 bitcast_to_float (const vint4& x);

void transpose (vint4 &a, vint4 &b, vint4 &c, vint4 &d);
void transpose (const vint4& a, const vint4& b, const vint4& c, const vint4& d,
                vint4 &r0, vint4 &r1, vint4 &r2, vint4 &r3);

vint4 AxBxCxDx (const vint4& a, const vint4& b, const vint4& c, const vint4& d);

// safe_mod(a,b) is like a%b, but safely returns 0 when b==0.
vint4 safe_mod (const vint4& a, const vint4& b);
vint4 safe_mod (const vint4& a, int b);




/// Integer 8-vector, accelerated by SIMD instructions when available.
class vint8 {
public:
    static const char* type_name() { return "vint8"; }
    typedef int value_t;      ///< Underlying equivalent scalar value type
    enum { elements = 8 };    ///< Number of scalar elements
    enum { paddedelements =8 }; ///< Number of scalar elements for full pad
    enum { bits = elements*32 }; ///< Total number of bits
    typedef simd_raw_t<int,elements>::type simd_t;  ///< the native SIMD type used
    typedef vbool8 vbool_t;   ///< bool type of the same length
    typedef vfloat8 vfloat_t; ///< float type of the same length
    typedef vint8 vint_t;     ///< int type of the same length
    typedef vbool8 bool_t;   // old name (deprecated 1.8)
    typedef vfloat8 float_t; // old name (deprecated 1.8)

    /// Default constructor (contents undefined)
    vint8 () { }

    /// Construct from a single value (store it in all slots)
    vint8 (int a);

    /// Construct from 2 values -- (a,a,b,b)
    vint8 (int a, int b);

    /// Construct from 8 values (won't work for vint8)
    vint8 (int a, int b, int c, int d, int e, int f, int g, int h);

    /// Construct from a pointer to values
    vint8 (const int *vals);

    /// Construct from a pointer to unsigned short values
    explicit vint8 (const unsigned short *vals);

    /// Construct from a pointer to signed short values
    explicit vint8 (const short *vals);

    /// Construct from a pointer to unsigned char values (0 - 255)
    explicit vint8 (const unsigned char *vals);

    /// Construct from a pointer to signed char values (-128 - 127)
    explicit vint8 (const char *vals);

    /// Copy construct from another vint8
    vint8 (const vint8 & other) { m_simd = other.m_simd; }

    /// Convert a vfloat8 to an vint8. Equivalent to i = (int)f;
    explicit vint8 (const vfloat8& f); // implementation below

    /// Construct from two vint4's
    vint8 (const vint4 &lo, const vint4 &hi);

    /// Construct from the underlying SIMD type
    vint8 (const simd_t& m) : m_simd(m) { }

    /// Return the raw SIMD type
    operator simd_t () const { return m_simd; }
    simd_t simd () const { return m_simd; }

    /// Return a pointer to the underlying scalar type
    const value_t* data () const { return (const value_t*)this; }
    value_t* data () { return (value_t*)this; }

    /// Sset all components to 0
    void clear () ;

    /// Return an vint8 with all components set to 0
    static const vint8 Zero ();

    /// Return an vint8 with all components set to 1
    static const vint8 One ();

    /// Return an vint8 with all components set to -1 (aka 0xffffffff)
    static const vint8 NegOne ();

    /// Return an vint8 with incremented components (e.g., 0,1,2,3).
    /// Optional arguments can give a non-zero starting point and step size.
    static const vint8 Iota (int start=0, int step=1);

    /// Return an vint8 with "geometric" iota: (1, 2, 4, 8, ...).
    static const vint8 Giota ();

    /// Assign one value to all components.
    const vint8 & operator= (int a);

    /// Assignment from another vint8
    const vint8 & operator= (const vint8& other) ;

    /// Component access (get)
    int operator[] (int i) const;

    /// Component access (set)
    int& operator[] (int i);

    /// Component access (set).
    void setcomp (int i, int value);

    value_t x () const;
    value_t y () const;
    value_t z () const;
    value_t w () const;
    void set_x (value_t val);
    void set_y (value_t val);
    void set_z (value_t val);
    void set_w (value_t val);

    /// Extract the lower percision vint4
    vint4 lo () const;

    /// Extract the higher percision vint4
    vint4 hi () const;

    /// Helper: load a single int into all components
    void load (int a);

    /// Load separate values into each component.
    void load (int a, int b, int c, int d, int e, int f, int g, int h);

    /// Load from an array of 8 values
    void load (const int *values);

    void load (const int *values, int n) ;

    /// Load from an array of 8 unsigned short values, convert to vint8
    void load (const unsigned short *values) ;

    /// Load from an array of 8 unsigned short values, convert to vint8
    void load (const short *values);

    /// Load from an array of 8 unsigned char values, convert to vint8
    void load (const unsigned char *values);

    /// Load from an array of 8 unsigned char values, convert to vint8
    void load (const char *values);

    /// Store the values into memory
    void store (int *values) const;

    /// Store the first n values into memory
    void store (int *values, int n) const;

    /// Store the least significant 16 bits of each element into adjacent
    /// unsigned shorts.
    void store (unsigned short *values) const;

    /// Store the least significant 8 bits of each element into adjacent
    /// unsigned chars.
    void store (unsigned char *values) const;

    /// Masked load -- read from values[] where mask is 1, load zero where
    /// mask is 0.
    void load_mask (int mask, const value_t *values);
    void load_mask (const vbool_t& mask, const value_t *values);

    /// Masked store -- write to values[] where mask is enabled, don't
    /// touch values[] where it's not.
    void store_mask (int mask, value_t *values) const;
    void store_mask (const vbool_t& mask, value_t *values) const;

    /// Load values from addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void gather (const value_t *baseptr, const vint_t& vindex);
    /// Gather elements defined by the mask, leave others unchanged.
    template<int scale=4>
    void gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex);
    template<int scale=4>
    void gather_mask (int mask, const value_t *baseptr, const vint_t& vindex);

    /// Store values at addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void scatter (value_t *baseptr, const vint_t& vindex) const;
    /// Scatter elements defined by the mask
    template<int scale=4>
    void scatter_mask (const bool_t& mask, value_t *baseptr, const vint_t& vindex) const;
    template<int scale=4>
    void scatter_mask (int mask, value_t *baseptr, const vint_t& vindex) const;

    // Arithmetic operators (component-by-component)
    friend vint8 operator+ (const vint8& a, const vint8& b);
    friend vint8 operator- (const vint8& a);
    friend vint8 operator- (const vint8& a, const vint8& b);
    friend vint8 operator* (const vint8& a, const vint8& b);
    friend vint8 operator/ (const vint8& a, const vint8& b);
    friend vint8 operator% (const vint8& a, const vint8& b);
    friend const vint8 & operator+= (vint8& a, const vint8& b);
    friend const vint8 & operator-= (vint8& a, const vint8& b);
    friend const vint8 & operator*= (vint8& a, const vint8& b);
    friend const vint8 & operator/= (vint8& a, const vint8& b);
    friend const vint8 & operator%= (vint8& a, const vint8& b);
    // Bitwise operators (component-by-component)
    friend vint8 operator& (const vint8& a, const vint8& b);
    friend vint8 operator| (const vint8& a, const vint8& b);
    friend vint8 operator^ (const vint8& a, const vint8& b);
    friend const vint8& operator&= (vint8& a, const vint8& b);
    friend const vint8& operator|= (vint8& a, const vint8& b);
    friend const vint8& operator^= (vint8& a, const vint8& b);
    friend vint8 operator~ (const vint8& a);
    friend vint8 operator<< (const vint8& a, unsigned int bits);
    friend vint8 operator>> (const vint8& a, unsigned int bits);
    friend const vint8& operator<<= (vint8& a, unsigned int bits);
    friend const vint8& operator>>= (vint8& a, unsigned int bits);
    // Comparison operators (component-by-component)
    friend vbool8 operator== (const vint8& a, const vint8& b);
    friend vbool8 operator!= (const vint8& a, const vint8& b);
    friend vbool8 operator<  (const vint8& a, const vint8& b);
    friend vbool8 operator>  (const vint8& a, const vint8& b);
    friend vbool8 operator>= (const vint8& a, const vint8& b);
    friend vbool8 operator<= (const vint8& a, const vint8& b);

    /// Stream output
    friend std::ostream& operator<< (std::ostream& cout, const vint8& a);

private:
    // The actual data representation
    union {
        simd_t  m_simd;
        value_t m_val[elements];
        simd_raw_t<int,4>::type m_4[2];
    };
};



// Shift right logical -- unsigned shift. This differs from operator>>
// in how it handles the sign bit.  (1<<31) >> 1 == (1<<31), but
// srl((1<<31),1) == 1<<30.
vint8 srl (const vint8& val, const unsigned int bits);

/// Helper: shuffle/swizzle with constant (templated) indices.
/// Example: shuffle<1,1,2,2>(vbool4(a,b,c,d)) returns (b,b,c,c)
template<int i0, int i1, int i2, int i3,
         int i4, int i5, int i6, int i7> vint8 shuffle (const vint8& a);

/// shuffle<i>(a) is the same as shuffle<i,i,i,i>(a)
template<int i> vint8 shuffle (const vint8& a);

/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i> int extract (const vint8& v);

/// Helper: substitute val for a[i]
template<int i> vint8 insert (const vint8& a, int val);

/// The sum of all components, returned in all components.
vint8 vreduce_add (const vint8& v);

// Reduction across all components
int reduce_add (const vint8& v);
int reduce_and (const vint8& v);
int reduce_or (const vint8& v);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// and b (if mask[i] is true), i.e., mask[i] ? b[i] : a[i].
vint8 blend (const vint8& a, const vint8& b, const vbool8& mask);

/// Use a bool mask to select between `a` (if mask[i] is true) or 0 if
/// mask[i] is false), i.e., mask[i] ? a[i] : 0. Equivalent to
/// blend(0,a,mask).
vint8 blend0 (const vint8& a, const vbool8& mask);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// or 0 (if mask[i] is true), i.e., mask[i] ? 0 : a[i]. Equivalent to
/// blend(0,a,!mask), or blend(a,0,mask).
vint8 blend0not (const vint8& a, const vbool8& mask);

/// Select 'a' where mask is true, 'b' where mask is false. Sure, it's a
/// synonym for blend with arguments rearranged, but this is more clear
/// because the arguments are symmetric to scalar (cond ? a : b).
vint8 select (const vbool8& mask, const vint8& a, const vint8& b);

// Per-element math
vint8 abs (const vint8& a);
vint8 min (const vint8& a, const vint8& b);
vint8 max (const vint8& a, const vint8& b);

/// Circular bit rotate by s bits, for N values at once.
vint8 rotl (const vint8& x, const int s);
// DEPRECATED(2.1)
vint8 rotl32 (const vint8& x, const unsigned int k);

/// andnot(a,b) returns ((~a) & b)
vint8 andnot (const vint8& a, const vint8& b);

/// Bitcast back and forth to intN (not a convert -- move the bits!)
vint8 bitcast_to_int (const vbool8& x);
vint8 bitcast_to_int (const vfloat8& x);
vfloat8 bitcast_to_float (const vint8& x);

// safe_mod(a,b) is like a%b, but safely returns 0 when b==0.
vint8 safe_mod (const vint8& a, const vint8& b);
vint8 safe_mod (const vint8& a, int b);





/// Integer 16-vector, accelerated by SIMD instructions when available.
class vint16 {
public:
    static const char* type_name() { return "vint16"; }
    typedef int value_t;      ///< Underlying equivalent scalar value type
    enum { elements = 16 };    ///< Number of scalar elements
    enum { paddedelements =16 }; ///< Number of scalar elements for full pad
    enum { bits = 128 };      ///< Total number of bits
    typedef simd_raw_t<int,elements>::type simd_t;  ///< the native SIMD type used
    typedef vbool16 vbool_t;   ///< bool type of the same length
    typedef vfloat16 vfloat_t; ///< float type of the same length
    typedef vint16 vint_t;     ///< int type of the same length
    typedef vbool16 bool_t;   // old name (deprecated 1.8)
    typedef vfloat16 float_t; // old name (deprecated 1.8)

    /// Default constructor (contents undefined)
    vint16 () { }

    /// Construct from a single value (store it in all slots)
    vint16 (int a);

    /// Construct from 16 values (won't work for vint16)
    vint16 (int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7,
           int v8, int v9, int v10, int v11, int v12, int v13, int v14, int v15);

    /// Construct from a pointer to values
    vint16 (const int *vals);

    /// Construct from a pointer to unsigned short values
    explicit vint16 (const unsigned short *vals);

    /// Construct from a pointer to signed short values
    explicit vint16 (const short *vals);

    /// Construct from a pointer to unsigned char values (0 - 255)
    explicit vint16 (const unsigned char *vals);

    /// Construct from a pointer to signed char values (-128 - 127)
    explicit vint16 (const char *vals);

    /// Copy construct from another vint16
    vint16 (const vint16 & other) { m_simd = other.m_simd; }

    /// Convert a vfloat16 to an vint16. Equivalent to i = (int)f;
    explicit vint16 (const vfloat16& f); // implementation below

    /// Construct from two vint8's
    vint16 (const vint8 &lo, const vint8 &hi);

    /// Construct from four vint4's
    vint16 (const vint4 &a, const vint4 &b, const vint4 &c, const vint4 &d);

    /// Construct from the underlying SIMD type
    vint16 (const simd_t& m) : m_simd(m) { }

    /// Return the raw SIMD type
    operator simd_t () const { return m_simd; }
    simd_t simd () const { return m_simd; }

    /// Return a pointer to the underlying scalar type
    const value_t* data () const { return (const value_t*)this; }
    value_t* data () { return (value_t*)this; }

    /// Sset all components to 0
    void clear () ;

    /// Return an vint16 with all components set to 0
    static const vint16 Zero ();

    /// Return an vint16 with all components set to 1
    static const vint16 One ();

    /// Return an vint16 with all components set to -1 (aka 0xffffffff)
    static const vint16 NegOne ();

    /// Return an vint16 with incremented components (e.g., 0,1,2,3).
    /// Optional arguments can give a non-zero starting point and step size.
    static const vint16 Iota (int start=0, int step=1);

    /// Return an vint16 with "geometric" iota: (1, 2, 4, 8, ...).
    static const vint16 Giota ();

    /// Assign one value to all components.
    const vint16 & operator= (int a);

    /// Assignment from another vint16
    const vint16 & operator= (const vint16& other) ;

    /// Component access (get)
    int operator[] (int i) const;

    /// Component access (set)
    int& operator[] (int i);

    /// Component access (set).
    void setcomp (int i, int value);

    value_t x () const;
    value_t y () const;
    value_t z () const;
    value_t w () const;
    void set_x (value_t val);
    void set_y (value_t val);
    void set_z (value_t val);
    void set_w (value_t val);

    /// Extract the lower percision vint8
    vint8 lo () const;

    /// Extract the higher percision vint8
    vint8 hi () const;

    /// Helper: load a single int into all components
    void load (int a);

    /// Load separate values into each component.
    void load (int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7,
               int v8, int v9, int v10, int v11, int v12, int v13, int v14, int v15);

    /// Load from an array of 16 values
    void load (const int *values);

    void load (const int *values, int n) ;

    /// Load from an array of 16 unsigned short values, convert to vint16
    void load (const unsigned short *values) ;

    /// Load from an array of 16 unsigned short values, convert to vint16
    void load (const short *values);

    /// Load from an array of 16 unsigned char values, convert to vint16
    void load (const unsigned char *values);

    /// Load from an array of 16 unsigned char values, convert to vint16
    void load (const char *values);

    /// Store the values into memory
    void store (int *values) const;

    /// Store the first n values into memory
    void store (int *values, int n) const;

    /// Store the least significant 16 bits of each element into adjacent
    /// unsigned shorts.
    void store (unsigned short *values) const;

    /// Store the least significant 8 bits of each element into adjacent
    /// unsigned chars.
    void store (unsigned char *values) const;

    /// Masked load -- read from values[] where mask is 1, load zero where
    /// mask is 0.
    void load_mask (const vbool_t &mask, const value_t *values);
    void load_mask (int mask, const value_t *values) { load_mask(vbool_t(mask), values); }

    /// Masked store -- write to values[] where mask is enabled, don't
    /// touch values[] where it's not.
    void store_mask (const vbool_t &mask, value_t *values) const;
    void store_mask (int mask, value_t *values) const { store_mask(vbool_t(mask), values); }

    /// Load values from addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void gather (const value_t *baseptr, const vint_t& vindex);
    /// Gather elements defined by the mask, leave others unchanged.
    template<int scale=4>
    void gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex);
    template<int scale=4>
    void gather_mask (int mask, const value_t *baseptr, const vint_t& vindex) {
        gather_mask<scale> (vbool_t(mask), baseptr, vindex);
    }

    /// Store values at addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void scatter (value_t *baseptr, const vint_t& vindex) const;
    /// Scatter elements defined by the mask
    template<int scale=4>
    void scatter_mask (const bool_t& mask, value_t *baseptr, const vint_t& vindex) const;
    template<int scale=4>
    void scatter_mask (int mask, value_t *baseptr, const vint_t& vindex) const {
        scatter_mask<scale> (vbool_t(mask), baseptr, vindex);
    }

    // Arithmetic operators (component-by-component)
    friend vint16 operator+ (const vint16& a, const vint16& b);
    friend vint16 operator- (const vint16& a);
    friend vint16 operator- (const vint16& a, const vint16& b);
    friend vint16 operator* (const vint16& a, const vint16& b);
    friend vint16 operator/ (const vint16& a, const vint16& b);
    friend vint16 operator% (const vint16& a, const vint16& b);
    friend const vint16 & operator+= (vint16& a, const vint16& b);
    friend const vint16 & operator-= (vint16& a, const vint16& b);
    friend const vint16 & operator*= (vint16& a, const vint16& b);
    friend const vint16 & operator/= (vint16& a, const vint16& b);
    friend const vint16 & operator%= (vint16& a, const vint16& b);
    // Bitwise operators (component-by-component)
    friend vint16 operator& (const vint16& a, const vint16& b);
    friend vint16 operator| (const vint16& a, const vint16& b);
    friend vint16 operator^ (const vint16& a, const vint16& b);
    friend const vint16& operator&= (vint16& a, const vint16& b);
    friend const vint16& operator|= (vint16& a, const vint16& b);
    friend const vint16& operator^= (vint16& a, const vint16& b);
    friend vint16 operator~ (const vint16& a);
    friend vint16 operator<< (const vint16& a, unsigned int bits);
    friend vint16 operator>> (const vint16& a, unsigned int bits);
    friend const vint16& operator<<= (vint16& a, unsigned int bits);
    friend const vint16& operator>>= (vint16& a, unsigned int bits);
    // Comparison operators (component-by-component)
    friend vbool16 operator== (const vint16& a, const vint16& b);
    friend vbool16 operator!= (const vint16& a, const vint16& b);
    friend vbool16 operator<  (const vint16& a, const vint16& b);
    friend vbool16 operator>  (const vint16& a, const vint16& b);
    friend vbool16 operator>= (const vint16& a, const vint16& b);
    friend vbool16 operator<= (const vint16& a, const vint16& b);

    /// Stream output
    friend std::ostream& operator<< (std::ostream& cout, const vint16& a);

private:
    // The actual data representation
    union {
        simd_t  m_simd;
        value_t m_val[elements];
        vint8    m_8[2];
    };
};



/// Shift right logical -- unsigned shift. This differs from operator>>
/// in how it handles the sign bit.  (1<<31) >> 1 == (1<<31), but
/// srl((1<<31),1) == 1<<30.
vint16 srl (const vint16& val, const unsigned int bits);

/// Shuffle groups of 4
template<int i0, int i1, int i2, int i3>
vint16 shuffle4 (const vint16& a);

/// shuffle4<i>(a) is the same as shuffle4<i,i,i,i>(a)
template<int i> vint16 shuffle4 (const vint16& a);

/// Shuffle within each group of 4
template<int i0, int i1, int i2, int i3>
vint16 shuffle (const vint16& a);

/// shuffle<i>(a) is the same as shuffle<i,i,i,i>(a)
template<int i> vint16 shuffle (const vint16& a);

/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i> int extract (const vint16& v);

/// Helper: substitute val for a[i]
template<int i> vint16 insert (const vint16& a, int val);

/// The sum of all components, returned in all components.
vint16 vreduce_add (const vint16& v);

// Reduction across all components
int reduce_add (const vint16& v);
int reduce_and (const vint16& v);
int reduce_or  (const vint16& v);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// and b (if mask[i] is true), i.e., mask[i] ? b[i] : a[i].
vint16 blend (const vint16& a, const vint16& b, const vbool16& mask);

/// Use a bool mask to select between `a` (if mask[i] is true) or 0 if
/// mask[i] is false), i.e., mask[i] ? a[i] : 0. Equivalent to
/// blend(0,a,mask).
vint16 blend0 (const vint16& a, const vbool16& mask);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// or 0 (if mask[i] is true), i.e., mask[i] ? 0 : a[i]. Equivalent to
/// blend(0,a,!mask), or blend(a,0,mask).
vint16 blend0not (const vint16& a, const vbool16& mask);

/// Select 'a' where mask is true, 'b' where mask is false. Sure, it's a
/// synonym for blend with arguments rearranged, but this is more clear
/// because the arguments are symmetric to scalar (cond ? a : b).
vint16 select (const vbool16& mask, const vint16& a, const vint16& b);

// Per-element math
vint16 abs (const vint16& a);
vint16 min (const vint16& a, const vint16& b);
vint16 max (const vint16& a, const vint16& b);

/// Circular bit rotate by s bits, for N values at once.
vint16 rotl (const vint16& x, const int s);
// DEPRECATED(2.1)
vint16 rotl32 (const vint16& x, const unsigned int k);

/// andnot(a,b) returns ((~a) & b)
vint16 andnot (const vint16& a, const vint16& b);

/// Bitcast back and forth to intN (not a convert -- move the bits!)
vint16 bitcast_to_int (const vbool16& x);
vint16 bitcast_to_int (const vfloat16& x);
vfloat16 bitcast_to_float (const vint16& x);

// safe_mod(a,b) is like a%b, but safely returns 0 when b==0.
vint16 safe_mod (const vint16& a, const vint16& b);
vint16 safe_mod (const vint16& a, int b);





/// Floating point 4-vector, accelerated by SIMD instructions when
/// available.
class vfloat4 {
public:
    static const char* type_name() { return "vfloat4"; }
    typedef float value_t;    ///< Underlying equivalent scalar value type
    enum { elements = 4 };    ///< Number of scalar elements
    enum { paddedelements = 4 }; ///< Number of scalar elements for full pad
    enum { bits = elements*32 }; ///< Total number of bits
    typedef simd_raw_t<float,4>::type simd_t;  ///< the native SIMD type used
    typedef vfloat4 vfloat_t; ///< SIMD int type
    typedef vint4 vint_t;     ///< SIMD int type
    typedef vbool4 vbool_t;   ///< SIMD bool type
    typedef vint4 int_t;      // old name (deprecated 1.8)
    typedef vbool4 bool_t;    // old name (deprecated 1.8)

    /// Default constructor (contents undefined)
    vfloat4 () { }

    /// Construct from a single value (store it in all slots)
    vfloat4 (float a) { load(a); }

    /// Construct from 3 or 4 values
    vfloat4 (float a, float b, float c, float d=0.0f) { load(a,b,c,d); }

    /// Construct from a pointer to 4 values
    vfloat4 (const float *f) { load (f); }

    /// Copy construct from another vfloat4
    vfloat4 (const vfloat4 &other) { m_simd = other.m_simd; }

    /// Construct from an vint4 (promoting all components to float)
    explicit vfloat4 (const vint4& ival);

    /// Construct from the underlying SIMD type
    vfloat4 (const simd_t& m) : m_simd(m) { }

    /// Return the raw SIMD type
    operator simd_t () const { return m_simd; }
    simd_t simd () const { return m_simd; }

    /// Return a pointer to the underlying scalar type
    const value_t* data () const { return (const value_t*)this; }
    value_t* data () { return (value_t*)this; }

    /// Construct from a Imath::V3f
    vfloat4 (const Imath::V3f &v) { load (v[0], v[1], v[2]); }

    /// Cast to a Imath::V3f
    const Imath::V3f& V3f () const { return *(const Imath::V3f*)this; }

    /// Construct from a Imath::V4f
    vfloat4 (const Imath::V4f &v) { load ((const float *)&v); }

    /// Cast to a Imath::V4f
    const Imath::V4f& V4f () const { return *(const Imath::V4f*)this; }

    /// Construct from a pointer to 4 unsigned short values
    explicit vfloat4 (const unsigned short *vals) { load(vals); }

    /// Construct from a pointer to 4 short values
    explicit vfloat4 (const short *vals) { load(vals); }

    /// Construct from a pointer to 4 unsigned char values
    explicit vfloat4 (const unsigned char *vals) { load(vals); }

    /// Construct from a pointer to 4 char values
    explicit vfloat4 (const char *vals) { load(vals); }

#ifdef _HALF_H_
    /// Construct from a pointer to 4 half (16 bit float) values
    explicit vfloat4 (const half *vals) { load(vals); }
#endif

    /// Assign a single value to all components
    const vfloat4 & operator= (float a) { load(a); return *this; }

    /// Assign a vfloat4
    const vfloat4 & operator= (vfloat4 other) {
        m_simd = other.m_simd;
        return *this;
    }

    /// Return a vfloat4 with all components set to 0.0
    static const vfloat4 Zero ();

    /// Return a vfloat4 with all components set to 1.0
    static const vfloat4 One ();

    /// Return a vfloat4 with incremented components (e.g., 0.0,1.0,2.0,3.0).
    /// Optional argument can give a non-zero starting point and non-1 step.
    static const vfloat4 Iota (float start=0.0f, float step=1.0f);

    /// Set all components to 0.0
    void clear ();

    /// Assign from a Imath::V4f
    const vfloat4 & operator= (const Imath::V4f &v);

    /// Assign from a Imath::V3f
    const vfloat4 & operator= (const Imath::V3f &v);

    /// Component access (get)
    float operator[] (int i) const;
    /// Component access (set)
    float& operator[] (int i);

    /// Component access (set).
    void setcomp (int i, float value);

    value_t x () const;
    value_t y () const;
    value_t z () const;
    value_t w () const;
    void set_x (value_t val);
    void set_y (value_t val);
    void set_z (value_t val);
    void set_w (value_t val);

    /// Helper: load a single value into all components
    void load (float val);

    /// Helper: load 3 or 4 values. (If 3 are supplied, the 4th will be 0.)
    void load (float a, float b, float c, float d=0.0f);

    /// Load from an array of 4 values
    void load (const float *values);

    /// Load from a partial array of <=4 values. Unassigned values are
    /// undefined.
    void load (const float *values, int n);

    /// Load from an array of 4 unsigned short values, convert to float
    void load (const unsigned short *values);

    /// Load from an array of 4 short values, convert to float
    void load (const short *values);

    /// Load from an array of 4 unsigned char values, convert to float
    void load (const unsigned char *values);

    /// Load from an array of 4 char values, convert to float
    void load (const char *values);

#ifdef _HALF_H_
    /// Load from an array of 4 half values, convert to float
    void load (const half *values);
#endif /* _HALF_H_ */

    void store (float *values) const;

    /// Store the first n values into memory
    void store (float *values, int n) const;

#ifdef _HALF_H_
    void store (half *values) const;
#endif

    /// Masked load -- read from values[] where mask is 1, load zero where
    /// mask is 0.
    void load_mask (int mask, const value_t *values);
    void load_mask (const vbool_t& mask, const value_t *values);

    /// Masked store -- write to values[] where mask is enabled, don't
    /// touch values[] where it's not.
    void store_mask (int mask, value_t *values) const;
    void store_mask (const vbool_t& mask, value_t *values) const;

    /// Load values from addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void gather (const value_t *baseptr, const vint_t& vindex);
    /// Gather elements defined by the mask, leave others unchanged.
    template<int scale=4>
    void gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex);
    template<int scale=4>
    void gather_mask (int mask, const value_t *baseptr, const vint_t& vindex);

    /// Store values at addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void scatter (value_t *baseptr, const vint_t& vindex) const;
    /// Scatter elements defined by the mask
    template<int scale=4>
    void scatter_mask (const bool_t& mask, value_t *baseptr, const vint_t& vindex) const;
    template<int scale=4>
    void scatter_mask (int mask, value_t *baseptr, const vint_t& vindex) const;

    // Arithmetic operators
    friend vfloat4 operator+ (const vfloat4& a, const vfloat4& b);
    const vfloat4 & operator+= (const vfloat4& a);
    vfloat4 operator- () const;
    friend vfloat4 operator- (const vfloat4& a, const vfloat4& b);
    const vfloat4 & operator-= (const vfloat4& a);
    friend vfloat4 operator* (const vfloat4& a, const vfloat4& b);
    const vfloat4 & operator*= (const vfloat4& a);
    const vfloat4 & operator*= (float val);
    friend vfloat4 operator/ (const vfloat4& a, const vfloat4& b);
    const vfloat4 & operator/= (const vfloat4& a);
    const vfloat4 & operator/= (float val);

    // Comparison operations
    friend vbool4 operator== (const vfloat4& a, const vfloat4& b);
    friend vbool4 operator!= (const vfloat4& a, const vfloat4& b);
    friend vbool4 operator<  (const vfloat4& a, const vfloat4& b);
    friend vbool4 operator>  (const vfloat4& a, const vfloat4& b);
    friend vbool4 operator>= (const vfloat4& a, const vfloat4& b);
    friend vbool4 operator<= (const vfloat4& a, const vfloat4& b);

    // Some oddball items that are handy

    /// Combine the first two components of A with the first two components
    /// of B.
    friend vfloat4 AxyBxy (const vfloat4& a, const vfloat4& b);

    /// Combine the first two components of A with the first two components
    /// of B, but interleaved.
    friend vfloat4 AxBxAyBy (const vfloat4& a, const vfloat4& b);

    /// Return xyz components, plus 0 for w
    vfloat4 xyz0 () const;

    /// Return xyz components, plus 1 for w
    vfloat4 xyz1 () const;

    /// Stream output
    friend inline std::ostream& operator<< (std::ostream& cout, const vfloat4& val);

protected:
    // The actual data representation
    union {
        simd_t  m_simd;
        value_t m_val[paddedelements];
    };
};


/// Helper: shuffle/swizzle with constant (templated) indices.
/// Example: shuffle<1,1,2,2>(vbool4(a,b,c,d)) returns (b,b,c,c)
template<int i0, int i1, int i2, int i3> vfloat4 shuffle (const vfloat4& a);

/// shuffle<i>(a) is the same as shuffle<i,i,i,i>(a)
template<int i> vfloat4 shuffle (const vfloat4& a);

/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i> float extract (const vfloat4& a);

/// Helper: substitute val for a[i]
template<int i> vfloat4 insert (const vfloat4& a, float val);

/// The sum of all components, returned in all components.
vfloat4 vreduce_add (const vfloat4& v);

/// The sum of all components, returned as a scalar.
float reduce_add (const vfloat4& v);

/// Return the float dot (inner) product of a and b in every component.
vfloat4 vdot (const vfloat4 &a, const vfloat4 &b);

/// Return the float dot (inner) product of a and b.
float dot (const vfloat4 &a, const vfloat4 &b);

/// Return the float 3-component dot (inner) product of a and b in
/// all components.
vfloat4 vdot3 (const vfloat4 &a, const vfloat4 &b);

/// Return the float 3-component dot (inner) product of a and b.
float dot3 (const vfloat4 &a, const vfloat4 &b);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// and b (if mask[i] is true), i.e., mask[i] ? b[i] : a[i].
vfloat4 blend (const vfloat4& a, const vfloat4& b, const vbool4& mask);

/// Use a bool mask to select between `a` (if mask[i] is true) or 0 if
/// mask[i] is false), i.e., mask[i] ? a[i] : 0. Equivalent to
/// blend(0,a,mask).
vfloat4 blend0 (const vfloat4& a, const vbool4& mask);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// or 0 (if mask[i] is true), i.e., mask[i] ? 0 : a[i]. Equivalent to
/// blend(0,a,!mask), or blend(a,0,mask).
vfloat4 blend0not (const vfloat4& a, const vbool4& mask);

/// "Safe" divide of vfloat4/vfloat4 -- for any component of the divisor
/// that is 0, return 0 rather than Inf.
vfloat4 safe_div (const vfloat4 &a, const vfloat4 &b);

/// Homogeneous divide to turn a vfloat4 into a vfloat3.
vfloat3 hdiv (const vfloat4 &a);

/// Select 'a' where mask is true, 'b' where mask is false. Sure, it's a
/// synonym for blend with arguments rearranged, but this is more clear
/// because the arguments are symmetric to scalar (cond ? a : b).
vfloat4 select (const vbool4& mask, const vfloat4& a, const vfloat4& b);

// Per-element math
vfloat4 abs (const vfloat4& a);    ///< absolute value (float)
vfloat4 sign (const vfloat4& a);   ///< 1.0 when value >= 0, -1 when negative
vfloat4 ceil (const vfloat4& a);
vfloat4 floor (const vfloat4& a);
vint4 ifloor (const vfloat4& a);    ///< (int)floor
inline vint4 floori (const vfloat4& a) { return ifloor(a); }  // DEPRECATED(1.8) alias

/// Per-element round to nearest integer (rounding away from 0 in cases
/// that are exactly half way).
vfloat4 round (const vfloat4& a);

/// Per-element round to nearest integer (rounding away from 0 in cases
/// that are exactly half way).
vint4 rint (const vfloat4& a);

vfloat4 rcp_fast (const vfloat4 &a);  ///< Fast, approximate 1/a
vfloat4 sqrt (const vfloat4 &a);
vfloat4 rsqrt (const vfloat4 &a);   ///< Fully accurate 1/sqrt
vfloat4 rsqrt_fast (const vfloat4 &a);  ///< Fast, approximate 1/sqrt
vfloat4 min (const vfloat4& a, const vfloat4& b); ///< Per-element min
vfloat4 max (const vfloat4& a, const vfloat4& b); ///< Per-element max
template <typename T> T exp (const T& v);  // template for all SIMD variants
template <typename T> T log (const T& v);

/// andnot(a,b) returns ((~a) & b)
vfloat4 andnot (const vfloat4& a, const vfloat4& b);

// Fused multiply and add (or subtract):
vfloat4 madd (const vfloat4& a, const vfloat4& b, const vfloat4& c); // a*b + c
vfloat4 msub (const vfloat4& a, const vfloat4& b, const vfloat4& c); // a*b - c
vfloat4 nmadd (const vfloat4& a, const vfloat4& b, const vfloat4& c); // -a*b + c
vfloat4 nmsub (const vfloat4& a, const vfloat4& b, const vfloat4& c); // -a*b - c

/// Transpose the rows and columns of the 4x4 matrix [a b c d].
/// In the end, a will have the original (a[0], b[0], c[0], d[0]),
/// b will have the original (a[1], b[1], c[1], d[1]), and so on.
void transpose (vfloat4 &a, vfloat4 &b, vfloat4 &c, vfloat4 &d);
void transpose (const vfloat4& a, const vfloat4& b, const vfloat4& c, const vfloat4& d,
                vfloat4 &r0, vfloat4 &r1, vfloat4 &r2, vfloat4 &r3);

/// Make a vfloat4 consisting of the first element of each of 4 vfloat4's.
vfloat4 AxBxCxDx (const vfloat4& a, const vfloat4& b,
                 const vfloat4& c, const vfloat4& d);



/// Floating point 3-vector, aligned to be internally identical to a vfloat4.
/// The way it differs from vfloat4 is that all of he load functions only
/// load three values, and all the stores only store 3 values. The vast
/// majority of ops just fall back to the vfloat4 version, and so will
/// operate on the 4th component, but we won't care about that results.
class vfloat3 : public vfloat4 {
public:
    static const char* type_name() { return "vfloat3"; }
    enum { elements = 3 };    ///< Number of scalar elements
    enum { paddedelements = 4 }; ///< Number of scalar elements for full pad

    /// Default constructor (contents undefined)
    vfloat3 () { }

    /// Construct from a single value (store it in all slots)
    vfloat3 (float a) { load(a); }

    /// Construct from 3 or 4 values
    vfloat3 (float a, float b, float c) { vfloat4::load(a,b,c); }

    /// Construct from a pointer to 4 values
    vfloat3 (const float *f) { load (f); }

    /// Copy construct from another vfloat3
    vfloat3 (const vfloat3 &other);

    explicit vfloat3 (const vfloat4 &other);

#if OIIO_SIMD
    /// Construct from the underlying SIMD type
    explicit vfloat3 (const simd_t& m) : vfloat4(m) { }
#endif

    /// Construct from a Imath::V3f
    vfloat3 (const Imath::V3f &v) : vfloat4(v) { }

    /// Cast to a Imath::V3f
    const Imath::V3f& V3f () const { return *(const Imath::V3f*)this; }

    /// Construct from a pointer to 4 unsigned short values
    explicit vfloat3 (const unsigned short *vals) { load(vals); }

    /// Construct from a pointer to 4 short values
    explicit vfloat3 (const short *vals) { load(vals); }

    /// Construct from a pointer to 4 unsigned char values
    explicit vfloat3 (const unsigned char *vals) { load(vals); }

    /// Construct from a pointer to 4 char values
    explicit vfloat3 (const char *vals) { load(vals); }

#ifdef _HALF_H_
    /// Construct from a pointer to 4 half (16 bit float) values
    explicit vfloat3 (const half *vals) { load(vals); }
#endif

    /// Assign a single value to all components
    const vfloat3 & operator= (float a) { load(a); return *this; }

    /// Return a vfloat3 with all components set to 0.0
    static const vfloat3 Zero ();

    /// Return a vfloat3 with all components set to 1.0
    static const vfloat3 One ();

    /// Return a vfloat3 with incremented components (e.g., 0.0,1.0,2.0).
    /// Optional argument can give a non-zero starting point and non-1 step.
    static const vfloat3 Iota (float start=0.0f, float step=1.0f);

    /// Helper: load a single value into all components
    void load (float val);

    /// Load from an array of 4 values
    void load (const float *values);

    /// Load from an array of 4 values
    void load (const float *values, int n);

    /// Load from an array of 4 unsigned short values, convert to float
    void load (const unsigned short *values);

    /// Load from an array of 4 short values, convert to float
    void load (const short *values);

    /// Load from an array of 4 unsigned char values, convert to float
    void load (const unsigned char *values);

    /// Load from an array of 4 char values, convert to float
    void load (const char *values);

#ifdef _HALF_H_
    /// Load from an array of 4 half values, convert to float
    void load (const half *values);
#endif /* _HALF_H_ */

    void store (float *values) const;

    void store (float *values, int n) const;

#ifdef _HALF_H_
    void store (half *values) const;
#endif

    /// Store into an Imath::V3f reference.
    void store (Imath::V3f &vec) const;

    // Math operators -- define in terms of vfloat3.
    friend vfloat3 operator+ (const vfloat3& a, const vfloat3& b);
    const vfloat3 & operator+= (const vfloat3& a);
    vfloat3 operator- () const;
    friend vfloat3 operator- (const vfloat3& a, const vfloat3& b);
    const vfloat3 & operator-= (const vfloat3& a);
    friend vfloat3 operator* (const vfloat3& a, const vfloat3& b);
    const vfloat3 & operator*= (const vfloat3& a);
    const vfloat3 & operator*= (float a);
    friend vfloat3 operator/ (const vfloat3& a, const vfloat3& b);
    const vfloat3 & operator/= (const vfloat3& a);
    const vfloat3 & operator/= (float a);

    /// Square of the length of the vector
    float length2() const;
    /// Length of the vector
    float length() const;

    /// Return a normalized version of the vector.
    vfloat3 normalized () const;
    /// Return a fast, approximate normalized version of the vector.
    vfloat3 normalized_fast () const;
    /// Normalize in place.
    void normalize() { *this = normalized(); }

    /// Stream output
    friend inline std::ostream& operator<< (std::ostream& cout, const vfloat3& val);
};




/// SIMD-based 4x4 matrix. This is guaranteed to have memory layout (when
/// not in registers) isomorphic to Imath::M44f.
class matrix44 {
public:
    // Uninitialized
    OIIO_FORCEINLINE matrix44 ()
#ifndef OIIO_SIMD_SSE
        : m_mat(Imath::UNINITIALIZED)
#endif
    { }

    /// Construct from a reference to an Imath::M44f
    OIIO_FORCEINLINE matrix44 (const Imath::M44f &M) {
#if OIIO_SIMD_SSE
        m_row[0].load (M[0]);
        m_row[1].load (M[1]);
        m_row[2].load (M[2]);
        m_row[3].load (M[3]);
#else
        m_mat = M;
#endif
    }

    /// Construct from a float array
    OIIO_FORCEINLINE explicit matrix44 (const float *f) {
#if OIIO_SIMD_SSE
        m_row[0].load (f+0);
        m_row[1].load (f+4);
        m_row[2].load (f+8);
        m_row[3].load (f+12);
#else
        m_mat = *(const Imath::M44f*)f;
#endif
    }

    /// Construct from 4 vfloat4 rows
    OIIO_FORCEINLINE explicit matrix44 (const vfloat4& a, const vfloat4& b,
                                        const vfloat4& c, const vfloat4& d) {
#if OIIO_SIMD_SSE
        m_row[0] = a; m_row[1] = b; m_row[2] = c; m_row[3] = d;
#else
        a.store (m_mat[0]);
        b.store (m_mat[1]);
        c.store (m_mat[2]);
        d.store (m_mat[3]);
#endif
    }
    /// Construct from 4 float[4] rows
    OIIO_FORCEINLINE explicit matrix44 (const float *a, const float *b,
                                        const float *c, const float *d) {
#if OIIO_SIMD_SSE
        m_row[0].load(a); m_row[1].load(b); m_row[2].load(c); m_row[3].load(d);
#else
        memcpy (m_mat[0], a, 4*sizeof(float));
        memcpy (m_mat[1], b, 4*sizeof(float));
        memcpy (m_mat[2], c, 4*sizeof(float));
        memcpy (m_mat[3], d, 4*sizeof(float));
#endif
    }

    /// Construct from 16 floats
    OIIO_FORCEINLINE matrix44 (float f00, float f01, float f02, float f03,
                               float f10, float f11, float f12, float f13,
                               float f20, float f21, float f22, float f23,
                               float f30, float f31, float f32, float f33)
    {
#if OIIO_SIMD_SSE
        m_row[0].load (f00, f01, f02, f03);
        m_row[1].load (f10, f11, f12, f13);
        m_row[2].load (f20, f21, f22, f23);
        m_row[3].load (f30, f31, f32, f33);
#else
        m_mat[0][0] = f00; m_mat[0][1] = f01; m_mat[0][2] = f02; m_mat[0][3] = f03;
        m_mat[1][0] = f10; m_mat[1][1] = f11; m_mat[1][2] = f12; m_mat[1][3] = f13;
        m_mat[2][0] = f20; m_mat[2][1] = f21; m_mat[2][2] = f22; m_mat[2][3] = f23;
        m_mat[3][0] = f30; m_mat[3][1] = f31; m_mat[3][2] = f32; m_mat[3][3] = f33;
#endif
    }

    /// Present as an Imath::M44f
    const Imath::M44f& M44f() const;

    /// Return one row
    vfloat4 operator[] (int i) const;

    /// Return the transposed matrix
    matrix44 transposed () const;

    /// Transform 3-point V by 4x4 matrix M.
    vfloat3 transformp (const vfloat3 &V) const;

    /// Transform 3-vector V by 4x4 matrix M.
    vfloat3 transformv (const vfloat3 &V) const;

    /// Transform 3-vector V by the transpose of 4x4 matrix M.
    vfloat3 transformvT (const vfloat3 &V) const;

    friend vfloat4 operator* (const vfloat4 &V, const matrix44& M);
    friend vfloat4 operator* (const matrix44& M, const vfloat4 &V);

    bool operator== (const matrix44& m) const;

    bool operator== (const Imath::M44f& m) const ;
    friend bool operator== (const Imath::M44f& a, const matrix44 &b);

    bool operator!= (const matrix44& m) const;

    bool operator!= (const Imath::M44f& m) const;
    friend bool operator!= (const Imath::M44f& a, const matrix44 &b);

    /// Return the inverse of the matrix.
    matrix44 inverse() const;

    /// Stream output
    friend inline std::ostream& operator<< (std::ostream& cout, const matrix44 &M);

private:
#if OIIO_SIMD_SSE
    vfloat4 m_row[4];
#else
    Imath::M44f m_mat;
#endif
};

/// Transform 3-point V by 4x4 matrix M.
vfloat3 transformp (const matrix44 &M, const vfloat3 &V);
vfloat3 transformp (const Imath::M44f &M, const vfloat3 &V);

/// Transform 3-vector V by 4x4 matrix M.
vfloat3 transformv (const matrix44 &M, const vfloat3 &V);
vfloat3 transformv (const Imath::M44f &M, const vfloat3 &V);

// Transform 3-vector by the transpose of 4x4 matrix M.
vfloat3 transformvT (const matrix44 &M, const vfloat3 &V);
vfloat3 transformvT (const Imath::M44f &M, const vfloat3 &V);




/// Floating point 8-vector, accelerated by SIMD instructions when
/// available.
class vfloat8 {
public:
    static const char* type_name() { return "vfloat8"; }
    typedef float value_t;    ///< Underlying equivalent scalar value type
    enum { elements = 8 };    ///< Number of scalar elements
    enum { paddedelements = 8 }; ///< Number of scalar elements for full pad
    enum { bits = elements*32 }; ///< Total number of bits
    typedef simd_raw_t<float,8>::type simd_t;  ///< the native SIMD type used
    typedef vfloat8 vfloat_t; ///< SIMD int type
    typedef vint8 vint_t;     ///< SIMD int type
    typedef vbool8 vbool_t;   ///< SIMD bool type
    typedef vint8 int_t;      // old name (deprecated 1.8)
    typedef vbool8 bool_t;    // old name (deprecated 1.8)

    /// Default constructor (contents undefined)
    vfloat8 () { }

    /// Construct from a single value (store it in all slots)
    vfloat8 (float a) { load(a); }

    /// Construct from 8 values
    vfloat8 (float a, float b, float c, float d,
            float e, float f, float g, float h) { load(a,b,c,d,e,f,g,h); }

    /// Construct from a pointer to 8 values
    vfloat8 (const float *f) { load (f); }

    /// Copy construct from another vfloat8
    vfloat8 (const vfloat8 &other) { m_simd = other.m_simd; }

    /// Construct from an int vector (promoting all components to float)
    explicit vfloat8 (const vint8& ival);

    /// Construct from two vfloat4's
    vfloat8 (const vfloat4 &lo, const vfloat4 &hi);

    /// Construct from the underlying SIMD type
    vfloat8 (const simd_t& m) : m_simd(m) { }

    /// Return the raw SIMD type
    operator simd_t () const { return m_simd; }
    simd_t simd () const { return m_simd; }

    /// Return a pointer to the underlying scalar type
    const value_t* data () const { return (const value_t*)this; }
    value_t* data () { return (value_t*)this; }

    /// Construct from a pointer to unsigned short values
    explicit vfloat8 (const unsigned short *vals) { load(vals); }

    /// Construct from a pointer to short values
    explicit vfloat8 (const short *vals) { load(vals); }

    /// Construct from a pointer to unsigned char values
    explicit vfloat8 (const unsigned char *vals) { load(vals); }

    /// Construct from a pointer to char values
    explicit vfloat8 (const char *vals) { load(vals); }

#ifdef _HALF_H_
    /// Construct from a pointer to half (16 bit float) values
    explicit vfloat8 (const half *vals) { load(vals); }
#endif

    /// Assign a single value to all components
    const vfloat8& operator= (float a) { load(a); return *this; }

    /// Assign a vfloat8
    const vfloat8& operator= (vfloat8 other) {
        m_simd = other.m_simd;
        return *this;
    }

    /// Return a vfloat8 with all components set to 0.0
    static const vfloat8 Zero ();

    /// Return a vfloat8 with all components set to 1.0
    static const vfloat8 One ();

    /// Return a vfloat8 with incremented components (e.g., 0,1,2,3,...)
    /// Optional argument can give a non-zero starting point and non-1 step.
    static const vfloat8 Iota (float start=0.0f, float step=1.0f);

    /// Set all components to 0.0
    void clear ();

    /// Component access (get)
    float operator[] (int i) const;
    /// Component access (set)
    float& operator[] (int i);

    /// Component access (set).
    void setcomp (int i, float value);

    value_t x () const;
    value_t y () const;
    value_t z () const;
    value_t w () const;
    void set_x (value_t val);
    void set_y (value_t val);
    void set_z (value_t val);
    void set_w (value_t val);

    /// Extract the lower percision vfloat4
    vfloat4 lo () const;

    /// Extract the higher percision vfloat4
    vfloat4 hi () const;

    /// Helper: load a single value into all components
    void load (float val);

    /// Helper: load 8 values
    void load (float a, float b, float c, float d,
               float e, float f, float g, float h);

    /// Load from an array of values
    void load (const float *values);

    /// Load from a partial array of <=8 values. Unassigned values are
    /// undefined.
    void load (const float *values, int n);

    /// Load from an array of 8 unsigned short values, convert to float
    void load (const unsigned short *values);

    /// Load from an array of 8 short values, convert to float
    void load (const short *values);

    /// Load from an array of 8 unsigned char values, convert to float
    void load (const unsigned char *values);

    /// Load from an array of 8 char values, convert to float
    void load (const char *values);

#ifdef _HALF_H_
    /// Load from an array of 8 half values, convert to float
    void load (const half *values);
#endif /* _HALF_H_ */

    void store (float *values) const;

    /// Store the first n values into memory
    void store (float *values, int n) const;

#ifdef _HALF_H_
    void store (half *values) const;
#endif

    /// Masked load -- read from values[] where mask is 1, load zero where
    /// mask is 0.
    void load_mask (int mask, const value_t *values);
    void load_mask (const vbool_t& mask, const value_t *values);

    /// Masked store -- write to values[] where mask is enabled, don't
    /// touch values[] where it's not.
    void store_mask (int mask, value_t *values) const;
    void store_mask (const vbool_t& mask, value_t *values) const;

    /// Load values from addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void gather (const value_t *baseptr, const vint_t& vindex);
    template<int scale=4>
    // Fastest way to fill with all 1 bits is to cmp any value to itself.
    void gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex);
    template<int scale=4>
    void gather_mask (int mask, const value_t *baseptr, const vint_t& vindex);

    /// Store values at addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void scatter (value_t *baseptr, const vint_t& vindex) const;
    /// Scatter elements defined by the mask
    template<int scale=4>
    void scatter_mask (const bool_t& mask, value_t *baseptr, const vint_t& vindex) const;
    template<int scale=4>
    void scatter_mask (int mask, value_t *baseptr, const vint_t& vindex) const;

    // Arithmetic operators (component-by-component)
    friend vfloat8 operator+ (const vfloat8& a, const vfloat8& b);
    friend vfloat8 operator- (const vfloat8& a);
    friend vfloat8 operator- (const vfloat8& a, const vfloat8& b);
    friend vfloat8 operator* (const vfloat8& a, const vfloat8& b);
    friend vfloat8 operator/ (const vfloat8& a, const vfloat8& b);
    friend vfloat8 operator% (const vfloat8& a, const vfloat8& b);
    friend const vfloat8 & operator+= (vfloat8& a, const vfloat8& b);
    friend const vfloat8 & operator-= (vfloat8& a, const vfloat8& b);
    friend const vfloat8 & operator*= (vfloat8& a, const vfloat8& b);
    friend const vfloat8 & operator/= (vfloat8& a, const vfloat8& b);

    // Comparison operations
    friend vbool8 operator== (const vfloat8& a, const vfloat8& b);
    friend vbool8 operator!= (const vfloat8& a, const vfloat8& b);
    friend vbool8 operator<  (const vfloat8& a, const vfloat8& b);
    friend vbool8 operator>  (const vfloat8& a, const vfloat8& b);
    friend vbool8 operator>= (const vfloat8& a, const vfloat8& b);
    friend vbool8 operator<= (const vfloat8& a, const vfloat8& b);

    // Some oddball items that are handy

    /// Stream output
    friend inline std::ostream& operator<< (std::ostream& cout, const vfloat8& val);

protected:
    // The actual data representation
    union {
        simd_t  m_simd;
        value_t m_val[paddedelements];
        simd_raw_t<float,4>::type m_4[2];
    };
};


/// Helper: shuffle/swizzle with constant (templated) indices.
/// Example: shuffle<1,1,2,2>(vbool4(a,b,c,d)) returns (b,b,c,c)
template<int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7>
vfloat8 shuffle (const vfloat8& a);

/// shuffle<i>(a) is the same as shuffle<i,i,i,i,...>(a)
template<int i> vfloat8 shuffle (const vfloat8& a);

/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i> float extract (const vfloat8& a);

/// Helper: substitute val for a[i]
template<int i> vfloat8 insert (const vfloat8& a, float val);

/// The sum of all components, returned in all components.
vfloat8 vreduce_add (const vfloat8& v);

/// The sum of all components, returned as a scalar.
float reduce_add (const vfloat8& v);

/// Return the float dot (inner) product of a and b in every component.
vfloat8 vdot (const vfloat8 &a, const vfloat8 &b);

/// Return the float dot (inner) product of a and b.
float dot (const vfloat8 &a, const vfloat8 &b);

/// Return the float 3-component dot (inner) product of a and b in
/// all components.
vfloat8 vdot3 (const vfloat8 &a, const vfloat8 &b);

/// Return the float 3-component dot (inner) product of a and b.
float dot3 (const vfloat8 &a, const vfloat8 &b);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// and b (if mask[i] is true), i.e., mask[i] ? b[i] : a[i].
vfloat8 blend (const vfloat8& a, const vfloat8& b, const vbool8& mask);

/// Use a bool mask to select between `a` (if mask[i] is true) or 0 if
/// mask[i] is false), i.e., mask[i] ? a[i] : 0. Equivalent to
/// blend(0,a,mask).
vfloat8 blend0 (const vfloat8& a, const vbool8& mask);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// or 0 (if mask[i] is true), i.e., mask[i] ? 0 : a[i]. Equivalent to
/// blend(0,a,!mask), or blend(a,0,mask).
vfloat8 blend0not (const vfloat8& a, const vbool8& mask);

/// "Safe" divide of vfloat8/vfloat8 -- for any component of the divisor
/// that is 0, return 0 rather than Inf.
vfloat8 safe_div (const vfloat8 &a, const vfloat8 &b);

/// Select 'a' where mask is true, 'b' where mask is false. Sure, it's a
/// synonym for blend with arguments rearranged, but this is more clear
/// because the arguments are symmetric to scalar (cond ? a : b).
vfloat8 select (const vbool8& mask, const vfloat8& a, const vfloat8& b);

// Per-element math
vfloat8 abs (const vfloat8& a);    ///< absolute value (float)
vfloat8 sign (const vfloat8& a);   ///< 1.0 when value >= 0, -1 when negative
vfloat8 ceil (const vfloat8& a);
vfloat8 floor (const vfloat8& a);
vint8 ifloor (const vfloat8& a);    ///< (int)floor
inline vint8 floori (const vfloat8& a) { return ifloor(a); }  // DEPRECATED(1.8) alias

/// Per-element round to nearest integer (rounding away from 0 in cases
/// that are exactly half way).
vfloat8 round (const vfloat8& a);

/// Per-element round to nearest integer (rounding away from 0 in cases
/// that are exactly half way).
vint8 rint (const vfloat8& a);

vfloat8 rcp_fast (const vfloat8 &a);  ///< Fast, approximate 1/a
vfloat8 sqrt (const vfloat8 &a);
vfloat8 rsqrt (const vfloat8 &a);   ///< Fully accurate 1/sqrt
vfloat8 rsqrt_fast (const vfloat8 &a);  ///< Fast, approximate 1/sqrt
vfloat8 min (const vfloat8& a, const vfloat8& b); ///< Per-element min
vfloat8 max (const vfloat8& a, const vfloat8& b); ///< Per-element max
// vfloat8 exp (const vfloat8& v);  // See template with vfloat4
// vfloat8 log (const vfloat8& v);  // See template with vfloat4

/// andnot(a,b) returns ((~a) & b)
vfloat8 andnot (const vfloat8& a, const vfloat8& b);

// Fused multiply and add (or subtract):
vfloat8 madd (const vfloat8& a, const vfloat8& b, const vfloat8& c); // a*b + c
vfloat8 msub (const vfloat8& a, const vfloat8& b, const vfloat8& c); // a*b - c
vfloat8 nmadd (const vfloat8& a, const vfloat8& b, const vfloat8& c); // -a*b + c
vfloat8 nmsub (const vfloat8& a, const vfloat8& b, const vfloat8& c); // -a*b - c



/// Floating point 16-vector, accelerated by SIMD instructions when
/// available.
class vfloat16 {
public:
    static const char* type_name() { return "vfloat16"; }
    typedef float value_t;    ///< Underlying equivalent scalar value type
    enum { elements = 16 };    ///< Number of scalar elements
    enum { paddedelements = 16 }; ///< Number of scalar elements for full pad
    enum { bits = elements*32 }; ///< Total number of bits
    typedef simd_raw_t<float,16>::type simd_t;  ///< the native SIMD type used
    typedef vfloat16 vfloat_t; ///< SIMD int type
    typedef vint16 vint_t;     ///< SIMD int type
    typedef vbool16 vbool_t;   ///< SIMD bool type
    typedef vint16 int_t;      // old name (deprecated 1.8)
    typedef vbool16 bool_t;    // old name (deprecated 1.8)

    /// Default constructor (contents undefined)
    vfloat16 () { }

    /// Construct from a single value (store it in all slots)
    vfloat16 (float a) { load(a); }

    /// Construct from 16 values
    vfloat16 (float v0, float v1, float v2, float v3,
             float v4, float v5, float v6, float v7,
             float v8, float v9, float v10, float v11,
             float v12, float v13, float v14, float v15);

    /// Construct from a pointer to 16 values
    vfloat16 (const float *f) { load (f); }

    /// Copy construct from another vfloat16
    vfloat16 (const vfloat16 &other) { m_simd = other.m_simd; }

    /// Construct from an int vector (promoting all components to float)
    explicit vfloat16 (const vint16& ival);

    /// Construct from two vfloat8's
    vfloat16 (const vfloat8 &lo, const vfloat8 &hi);

    /// Construct from four vfloat4's
    vfloat16 (const vfloat4 &a, const vfloat4 &b, const vfloat4 &c, const vfloat4 &d);

    /// Construct from the underlying SIMD type
    vfloat16 (const simd_t& m) : m_simd(m) { }

    /// Return the raw SIMD type
    operator simd_t () const { return m_simd; }
    simd_t simd () const { return m_simd; }

    /// Return a pointer to the underlying scalar type
    const value_t* data () const { return (const value_t*)this; }
    value_t* data () { return (value_t*)this; }

    /// Construct from a pointer to unsigned short values
    explicit vfloat16 (const unsigned short *vals) { load(vals); }

    /// Construct from a pointer to short values
    explicit vfloat16 (const short *vals) { load(vals); }

    /// Construct from a pointer to unsigned char values
    explicit vfloat16 (const unsigned char *vals) { load(vals); }

    /// Construct from a pointer to char values
    explicit vfloat16 (const char *vals) { load(vals); }

#ifdef _HALF_H_
    /// Construct from a pointer to half (16 bit float) values
    explicit vfloat16 (const half *vals) { load(vals); }
#endif

    /// Assign a single value to all components
    const vfloat16& operator= (float a) { load(a); return *this; }

    /// Assign a vfloat16
    const vfloat16& operator= (vfloat16 other) {
        m_simd = other.m_simd;
        return *this;
    }

    /// Return a vfloat16 with all components set to 0.0
    static const vfloat16 Zero ();

    /// Return a vfloat16 with all components set to 1.0
    static const vfloat16 One ();

    /// Return a vfloat16 with incremented components (e.g., 0,1,2,3,...)
    /// Optional argument can give a non-zero starting point and non-1 step.
    static const vfloat16 Iota (float start=0.0f, float step=1.0f);

    /// Set all components to 0.0
    void clear ();

    /// Component access (get)
    float operator[] (int i) const;
    /// Component access (set)
    float& operator[] (int i);

    /// Component access (set).
    void setcomp (int i, float value);

    value_t x () const;
    value_t y () const;
    value_t z () const;
    value_t w () const;
    void set_x (value_t val);
    void set_y (value_t val);
    void set_z (value_t val);
    void set_w (value_t val);

    /// Extract the lower percision vfloat8
    vfloat8 lo () const;

    /// Extract the higher percision vfloat8
    vfloat8 hi () const;

    /// Helper: load a single value into all components
    void load (float val);

    /// Load separate values into each component.
    void load (float v0, float v1, float v2, float v3,
               float v4, float v5, float v6, float v7,
               float v8, float v9, float v10, float v11,
               float v12, float v13, float v14, float v15);

    /// Load from an array of values
    void load (const float *values);

    /// Load from a partial array of <=16 values. Unassigned values are
    /// undefined.
    void load (const float *values, int n);

    /// Load from an array of 16 unsigned short values, convert to float
    void load (const unsigned short *values);

    /// Load from an array of 16 short values, convert to float
    void load (const short *values);

    /// Load from an array of 16 unsigned char values, convert to float
    void load (const unsigned char *values);

    /// Load from an array of 16 char values, convert to float
    void load (const char *values);

#ifdef _HALF_H_
    /// Load from an array of 16 half values, convert to float
    void load (const half *values);
#endif /* _HALF_H_ */

    void store (float *values) const;

    /// Store the first n values into memory
    void store (float *values, int n) const;

#ifdef _HALF_H_
    void store (half *values) const;
#endif

    /// Masked load -- read from values[] where mask is 1, load zero where
    /// mask is 0.
    void load_mask (const vbool_t &mask, const value_t *values);
    void load_mask (int mask, const value_t *values) { load_mask(vbool_t(mask), values); }

    /// Masked store -- write to values[] where mask is enabled, don't
    /// touch values[] where it's not.
    void store_mask (const vbool_t &mask, value_t *values) const;
    void store_mask (int mask, value_t *values) const { store_mask(vbool_t(mask), values); }

    /// Load values from addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void gather (const value_t *baseptr, const vint_t& vindex);
    /// Gather elements defined by the mask, leave others unchanged.
    template<int scale=4>
    void gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex);
    template<int scale=4>
    void gather_mask (int mask, const value_t *baseptr, const vint_t& vindex) {
        gather_mask<scale> (vbool_t(mask), baseptr, vindex);
    }

    /// Store values at addresses  (char*)basepatr + vindex[i]*scale
    template<int scale=4>
    void scatter (value_t *baseptr, const vint_t& vindex) const;
    /// Scatter elements defined by the mask
    template<int scale=4>
    void scatter_mask (const bool_t& mask, value_t *baseptr, const vint_t& vindex) const;
    template<int scale=4>
    void scatter_mask (int mask, value_t *baseptr, const vint_t& vindex) const {
        scatter_mask<scale> (vbool_t(mask), baseptr, vindex);
    }

    // Arithmetic operators (component-by-component)
    friend vfloat16 operator+ (const vfloat16& a, const vfloat16& b);
    friend vfloat16 operator- (const vfloat16& a);
    friend vfloat16 operator- (const vfloat16& a, const vfloat16& b);
    friend vfloat16 operator* (const vfloat16& a, const vfloat16& b);
    friend vfloat16 operator/ (const vfloat16& a, const vfloat16& b);
    friend vfloat16 operator% (const vfloat16& a, const vfloat16& b);
    friend const vfloat16 & operator+= (vfloat16& a, const vfloat16& b);
    friend const vfloat16 & operator-= (vfloat16& a, const vfloat16& b);
    friend const vfloat16 & operator*= (vfloat16& a, const vfloat16& b);
    friend const vfloat16 & operator/= (vfloat16& a, const vfloat16& b);

    // Comparison operations
    friend vbool16 operator== (const vfloat16& a, const vfloat16& b);
    friend vbool16 operator!= (const vfloat16& a, const vfloat16& b);
    friend vbool16 operator<  (const vfloat16& a, const vfloat16& b);
    friend vbool16 operator>  (const vfloat16& a, const vfloat16& b);
    friend vbool16 operator>= (const vfloat16& a, const vfloat16& b);
    friend vbool16 operator<= (const vfloat16& a, const vfloat16& b);

    // Some oddball items that are handy

    /// Stream output
    friend inline std::ostream& operator<< (std::ostream& cout, const vfloat16& val);

protected:
    // The actual data representation
    union {
        simd_t  m_simd;
        value_t m_val[paddedelements];
        vfloat8  m_8[2];
    };
};


/// Shuffle groups of 4
template<int i0, int i1, int i2, int i3>
vfloat16 shuffle4 (const vfloat16& a);

/// shuffle4<i>(a) is the same as shuffle4<i,i,i,i>(a)
template<int i> vfloat16 shuffle4 (const vfloat16& a);

/// Shuffle within each group of 4
template<int i0, int i1, int i2, int i3>
vfloat16 shuffle (const vfloat16& a);

/// shuffle<i>(a) is the same as shuffle<i,i,i,i>(a)
template<int i> vfloat16 shuffle (const vfloat16& a);

/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i> float extract (const vfloat16& a);

/// Helper: substitute val for a[i]
template<int i> vfloat16 insert (const vfloat16& a, float val);

/// The sum of all components, returned in all components.
vfloat16 vreduce_add (const vfloat16& v);

/// The sum of all components, returned as a scalar.
float reduce_add (const vfloat16& v);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// and b (if mask[i] is true), i.e., mask[i] ? b[i] : a[i].
vfloat16 blend (const vfloat16& a, const vfloat16& b, const vbool4& mask);

/// Use a bool mask to select between `a` (if mask[i] is true) or 0 if
/// mask[i] is false), i.e., mask[i] ? a[i] : 0. Equivalent to
/// blend(0,a,mask).
vfloat16 blend0 (const vfloat16& a, const vbool4& mask);

/// Use a bool mask to select between components of a (if mask[i] is false)
/// or 0 (if mask[i] is true), i.e., mask[i] ? 0 : a[i]. Equivalent to
/// blend(0,a,!mask), or blend(a,0,mask).
vfloat16 blend0not (const vfloat16& a, const vbool4& mask);

/// "Safe" divide of vfloat16/vfloat16 -- for any component of the divisor
/// that is 0, return 0 rather than Inf.
vfloat16 safe_div (const vfloat16 &a, const vfloat16 &b);

/// Select 'a' where mask is true, 'b' where mask is false. Sure, it's a
/// synonym for blend with arguments rearranged, but this is more clear
/// because the arguments are symmetric to scalar (cond ? a : b).
vfloat16 select (const vbool16& mask, const vfloat16& a, const vfloat16& b);

// Per-element math
vfloat16 abs (const vfloat16& a);    ///< absolute value (float)
vfloat16 sign (const vfloat16& a);   ///< 1.0 when value >= 0, -1 when negative
vfloat16 ceil (const vfloat16& a);
vfloat16 floor (const vfloat16& a);
vint16 ifloor (const vfloat16& a);    ///< (int)floor
inline vint16 floori (const vfloat16& a) { return ifloor(a); }  // DEPRECATED(1.8) alias

/// Per-element round to nearest integer (rounding away from 0 in cases
/// that are exactly half way).
vfloat16 round (const vfloat16& a);

/// Per-element round to nearest integer (rounding away from 0 in cases
/// that are exactly half way).
vint16 rint (const vfloat16& a);

vfloat16 rcp_fast (const vfloat16 &a);  ///< Fast, approximate 1/a
vfloat16 sqrt (const vfloat16 &a);
vfloat16 rsqrt (const vfloat16 &a);   ///< Fully accurate 1/sqrt
vfloat16 rsqrt_fast (const vfloat16 &a);  ///< Fast, approximate 1/sqrt
vfloat16 min (const vfloat16& a, const vfloat16& b); ///< Per-element min
vfloat16 max (const vfloat16& a, const vfloat16& b); ///< Per-element max
// vfloat16 exp (const vfloat16& v);  // See template with vfloat4
// vfloat16 log (const vfloat16& v);  // See template with vfloat4

/// andnot(a,b) returns ((~a) & b)
vfloat16 andnot (const vfloat16& a, const vfloat16& b);

// Fused multiply and add (or subtract):
vfloat16 madd (const vfloat16& a, const vfloat16& b, const vfloat16& c); // a*b + c
vfloat16 msub (const vfloat16& a, const vfloat16& b, const vfloat16& c); // a*b - c
vfloat16 nmadd (const vfloat16& a, const vfloat16& b, const vfloat16& c); // -a*b + c
vfloat16 nmsub (const vfloat16& a, const vfloat16& b, const vfloat16& c); // -a*b - c



// Odds and ends, other CPU hardware tricks

// Try to set the flush_zero_mode CPU flag on x86. Return true if we are
// able, otherwise false (because it's not available on that platform,
// or because it's gcc 4.8 which has a bug that lacks this intrinsic).
inline bool set_flush_zero_mode (bool on) {
#if (defined(__x86_64__) || defined(__i386__)) && (OIIO_GNUC_VERSION == 0 || OIIO_GNUC_VERSION > 40900)
    _MM_SET_FLUSH_ZERO_MODE (on ? _MM_FLUSH_ZERO_ON : _MM_FLUSH_ZERO_OFF);
    return true;
#endif
    return false;
}

// Try to set the denorms_zero_mode CPU flag on x86. Return true if we are
// able, otherwise false (because it's not available on that platform,
// or because it's gcc 4.8 which has a bug that lacks this intrinsic).
inline bool set_denorms_zero_mode (bool on) {
#if (defined(__x86_64__) || defined(__i386__)) && (OIIO_GNUC_VERSION == 0 || OIIO_GNUC_VERSION > 40900)
    _MM_SET_DENORMALS_ZERO_MODE (on ? _MM_DENORMALS_ZERO_ON : _MM_DENORMALS_ZERO_OFF);
    return true;
#endif
    return false;
}

// Get the flush_zero_mode CPU flag on x86.
inline bool get_flush_zero_mode () {
#if (defined(__x86_64__) || defined(__i386__)) && (OIIO_GNUC_VERSION == 0 || OIIO_GNUC_VERSION > 40900)
    return _MM_GET_FLUSH_ZERO_MODE() == _MM_FLUSH_ZERO_ON;
#endif
    return false;
}

// Get the denorms_zero_mode CPU flag on x86.
inline bool get_denorms_zero_mode () {
#if (defined(__x86_64__) || defined(__i386__)) && (OIIO_GNUC_VERSION == 0 || OIIO_GNUC_VERSION > 40900)
    return _MM_GET_DENORMALS_ZERO_MODE() == _MM_DENORMALS_ZERO_ON;
#endif
    return false;
}






//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// Gory implementation details follow.
//
// ^^^ All declarations and documention is above ^^^
//
// vvv Below is the implementation, often considerably cluttered with
//     #if's for each architeture, and unapologitic use of intrinsics and
//     every manner of dirty trick we can think of to make things fast.
//     Some of this isn't pretty. We won't recapitulate comments or
//     documentation of what the functions are supposed to do, please
//     consult the declarations above for that.
//
//     Here be dragons.
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// vbool4 implementation


OIIO_FORCEINLINE int vbool4::operator[] (int i) const {
    OIIO_DASSERT(i >= 0 && i < elements);
#if OIIO_SIMD_SSE
    return ((_mm_movemask_ps(m_simd) >> i) & 1) ? -1 : 0;
#else
    return m_val[i];
#endif
}

OIIO_FORCEINLINE int& vbool4::operator[] (int i) {
    OIIO_DASSERT(i >= 0 && i < elements);
    return m_val[i];
}


OIIO_FORCEINLINE void vbool4::setcomp (int i, bool value) {
    OIIO_DASSERT(i >= 0 && i < elements);
    m_val[i] = value ? -1 : 0;
}


OIIO_FORCEINLINE std::ostream& operator<< (std::ostream& cout, const vbool4& a) {
    cout << a[0];
    for (int i = 1; i < a.elements; ++i)
        cout << ' ' << a[i];
    return cout;
}


OIIO_FORCEINLINE void vbool4::load (bool a) {
#if OIIO_SIMD_SSE
    m_simd = _mm_castsi128_ps(_mm_set1_epi32(-int(a)));
#else
    int val = -int(a);
    SIMD_CONSTRUCT (val);
#endif
}


OIIO_FORCEINLINE void vbool4::load (bool a, bool b, bool c, bool d) {
#if OIIO_SIMD_SSE
    // N.B. -- we need to reverse the order because of our convention
    // of storing a,b,c,d in the same order in memory.
    m_simd = _mm_castsi128_ps(_mm_set_epi32(-int(d), -int(c), -int(b), -int(a)));
#else
    m_val[0] = -int(a);
    m_val[1] = -int(b);
    m_val[2] = -int(c);
    m_val[3] = -int(d);
#endif
}

OIIO_FORCEINLINE vbool4::vbool4 (const bool *a) {
    load (a[0], a[1], a[2], a[3]);
}

OIIO_FORCEINLINE const vbool4& vbool4::operator= (const vbool4 & other) {
    m_simd = other.m_simd;
    return *this;
}


OIIO_FORCEINLINE int vbool4::bitmask () const {
#if OIIO_SIMD_SSE
    return _mm_movemask_ps(m_simd);
#else
    int r = 0;
    for (int i = 0; i < elements; ++i)
        if (m_val[i])
            r |= 1<<i;
    return r;
#endif
}


OIIO_FORCEINLINE vbool4
vbool4::from_bitmask (int bitmask) {
    // I think this is a fast conversion from int bitmask to vbool4
    return (vint4::Giota() & vint4(bitmask)) != vint4::Zero();
}


OIIO_FORCEINLINE void vbool4::clear () {
#if OIIO_SIMD_SSE
    m_simd = _mm_setzero_ps();
#else
    *this = false;
#endif
}


OIIO_FORCEINLINE const vbool4 vbool4::False () {
#if OIIO_SIMD_SSE
    return _mm_setzero_ps();
#else
    return false;
#endif
}

OIIO_FORCEINLINE const vbool4 vbool4::True () {
    // Fastest way to fill with all 1 bits is to cmp any value to itself.
#if OIIO_SIMD_SSE
# if OIIO_SIMD_AVX && (OIIO_GNUC_VERSION > 50000)
    __m128i anyval = _mm_undefined_si128();
# else
    __m128i anyval = _mm_setzero_si128();
# endif
    return _mm_castsi128_ps (_mm_cmpeq_epi8 (anyval, anyval));
#else
    return true;
#endif
}

OIIO_FORCEINLINE void vbool4::store (bool *values) const {
    SIMD_DO (values[i] = m_val[i] ? true : false);
}

OIIO_FORCEINLINE void vbool4::store (bool *values, int n) const {
    OIIO_DASSERT (n >= 0 && n <= elements);
    for (int i = 0; i < n; ++i)
        values[i] = m_val[i] ? true : false;
}



OIIO_FORCEINLINE vbool4 operator! (const vbool4 & a) {
#if OIIO_SIMD_SSE
    return _mm_xor_ps (a.simd(), vbool4::True());
#else
    SIMD_RETURN (vbool4, a[i] ^ (-1));
#endif
}

OIIO_FORCEINLINE vbool4 operator& (const vbool4 & a, const vbool4 & b) {
#if OIIO_SIMD_SSE
    return _mm_and_ps (a.simd(), b.simd());
#else
    SIMD_RETURN (vbool4, a[i] & b[i]);
#endif
}

OIIO_FORCEINLINE vbool4 operator| (const vbool4 & a, const vbool4 & b) {
#if OIIO_SIMD_SSE
    return _mm_or_ps (a.simd(), b.simd());
#else
    SIMD_RETURN (vbool4, a[i] | b[i]);
#endif
}

OIIO_FORCEINLINE vbool4 operator^ (const vbool4& a, const vbool4& b) {
#if OIIO_SIMD_SSE
    return _mm_xor_ps (a.simd(), b.simd());
#else
    SIMD_RETURN (vbool4, a[i] ^ b[i]);
#endif
}


OIIO_FORCEINLINE const vbool4& operator&= (vbool4& a, const vbool4 &b) {
    return a = a & b;
}

OIIO_FORCEINLINE const vbool4& operator|= (vbool4& a, const vbool4& b) {
    return a = a | b;
}

OIIO_FORCEINLINE const vbool4& operator^= (vbool4& a, const vbool4& b) {
    return a = a ^ b;
}

OIIO_FORCEINLINE vbool4 operator~ (const vbool4& a) {
#if OIIO_SIMD_SSE
    // Fastest way to bit-complement in SSE is to xor with 0xffffffff.
    return _mm_xor_ps (a.simd(), vbool4::True());
#else
    SIMD_RETURN (vbool4, ~a[i]);
#endif
}

OIIO_FORCEINLINE vbool4 operator== (const vbool4 & a, const vbool4 & b) {
#if OIIO_SIMD_SSE
    return _mm_castsi128_ps (_mm_cmpeq_epi32 (_mm_castps_si128 (a), _mm_castps_si128(b)));
#else
    SIMD_RETURN (vbool4, a[i] == b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool4 operator!= (const vbool4 & a, const vbool4 & b) {
#if OIIO_SIMD_SSE
    return _mm_xor_ps (a, b);
#else
    SIMD_RETURN (vbool4, a[i] != b[i] ? -1 : 0);
#endif
}




#if OIIO_SIMD_SSE
// Shuffling. Use like this:  x = shuffle<3,2,1,0>(b)
template<int i0, int i1, int i2, int i3>
OIIO_FORCEINLINE __m128i shuffle_sse (__m128i v) {
    return _mm_shuffle_epi32(v, _MM_SHUFFLE(i3, i2, i1, i0));
}
#endif

#if OIIO_SIMD_SSE >= 3
// SSE3 has intrinsics for a few special cases
template<> OIIO_FORCEINLINE __m128i shuffle_sse<0, 0, 2, 2> (__m128i a) {
    return _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(a)));
}
template<> OIIO_FORCEINLINE __m128i shuffle_sse<1, 1, 3, 3> (__m128i a) {
    return _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(a)));
}
template<> OIIO_FORCEINLINE __m128i shuffle_sse<0, 1, 0, 1> (__m128i a) {
    return _mm_castpd_si128(_mm_movedup_pd(_mm_castsi128_pd(a)));
}
#endif

#if OIIO_SIMD_SSE
template<int i0, int i1, int i2, int i3>
OIIO_FORCEINLINE __m128 shuffle_sse (__m128 a) {
    return _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), _MM_SHUFFLE(i3, i2, i1, i0)));
}
#endif

#if OIIO_SIMD_SSE >= 3
// SSE3 has intrinsics for a few special cases
template<> OIIO_FORCEINLINE __m128 shuffle_sse<0, 0, 2, 2> (__m128 a) {
    return _mm_moveldup_ps(a);
}
template<> OIIO_FORCEINLINE __m128 shuffle_sse<1, 1, 3, 3> (__m128 a) {
    return _mm_movehdup_ps(a);
}
template<> OIIO_FORCEINLINE __m128 shuffle_sse<0, 1, 0, 1> (__m128 a) {
    return _mm_castpd_ps(_mm_movedup_pd(_mm_castps_pd(a)));
}
#endif


/// Helper: shuffle/swizzle with constant (templated) indices.
/// Example: shuffle<1,1,2,2>(vbool4(a,b,c,d)) returns (b,b,c,c)
template<int i0, int i1, int i2, int i3>
OIIO_FORCEINLINE vbool4 shuffle (const vbool4& a) {
#if OIIO_SIMD_SSE
    return shuffle_sse<i0,i1,i2,i3> (a.simd());
#else
    return vbool4 (a[i0], a[i1], a[i2], a[i3]);
#endif
}

/// shuffle<i>(a) is the same as shuffle<i,i,i,i>(a)
template<int i> OIIO_FORCEINLINE vbool4 shuffle (const vbool4& a) {
    return shuffle<i,i,i,i>(a);
}


/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i>
OIIO_FORCEINLINE bool extract (const vbool4& a) {
#if OIIO_SIMD_SSE >= 4
    return _mm_extract_epi32(_mm_castps_si128(a.simd()), i);  // SSE4.1 only
#else
    return a[i];
#endif
}

/// Helper: substitute val for a[i]
template<int i>
OIIO_FORCEINLINE vbool4 insert (const vbool4& a, bool val) {
#if OIIO_SIMD_SSE >= 4
    int ival = -int(val);
    return _mm_castsi128_ps (_mm_insert_epi32 (_mm_castps_si128(a), ival, i));
#else
    vbool4 tmp = a;
    tmp[i] = -int(val);
    return tmp;
#endif
}

OIIO_FORCEINLINE bool reduce_and (const vbool4& v) {
#if OIIO_SIMD_AVX
    return _mm_testc_ps (v, vbool4(true)) != 0;
#elif OIIO_SIMD_SSE
    return _mm_movemask_ps(v.simd()) == 0xf;
#else
    SIMD_RETURN_REDUCE (bool, true, r &= (v[i] != 0));
#endif
}

OIIO_FORCEINLINE bool reduce_or (const vbool4& v) {
#if OIIO_SIMD_AVX
    return ! _mm_testz_ps (v, v);
#elif OIIO_SIMD_SSE
    return _mm_movemask_ps(v) != 0;
#else
    SIMD_RETURN_REDUCE (bool, false, r |= (v[i] != 0));
#endif
}

OIIO_FORCEINLINE bool all (const vbool4& v) { return reduce_and(v) == true; }
OIIO_FORCEINLINE bool any (const vbool4& v) { return reduce_or(v) == true; }
OIIO_FORCEINLINE bool none (const vbool4& v) { return reduce_or(v) == false; }



//////////////////////////////////////////////////////////////////////
// vbool8 implementation


OIIO_FORCEINLINE int vbool8::operator[] (int i) const {
    OIIO_DASSERT(i >= 0 && i < elements);
#if OIIO_SIMD_AVX
    return ((_mm256_movemask_ps(m_simd) >> i) & 1) ? -1 : 0;
#else
    return m_val[i];
#endif
}

OIIO_FORCEINLINE void vbool8::setcomp (int i, bool value) {
    OIIO_DASSERT(i >= 0 && i < elements);
    m_val[i] = value ? -1 : 0;
}

OIIO_FORCEINLINE int& vbool8::operator[] (int i) {
    OIIO_DASSERT(i >= 0 && i < elements);
    return m_val[i];
}


OIIO_FORCEINLINE std::ostream& operator<< (std::ostream& cout, const vbool8& a) {
    cout << a[0];
    for (int i = 1; i < a.elements; ++i)
        cout << ' ' << a[i];
    return cout;
}


OIIO_FORCEINLINE void vbool8::load (bool a) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_castsi256_ps(_mm256_set1_epi32(-int(a)));
#else
    int val = -int(a);
    SIMD_CONSTRUCT (val);
#endif
}


OIIO_FORCEINLINE void vbool8::load (bool a, bool b, bool c, bool d,
                                   bool e, bool f, bool g, bool h) {
#if OIIO_SIMD_AVX
    // N.B. -- we need to reverse the order because of our convention
    // of storing a,b,c,d in the same order in memory.
    m_simd = _mm256_castsi256_ps(_mm256_set_epi32(-int(h), -int(g), -int(f), -int(e),
                                                  -int(d), -int(c), -int(b), -int(a)));
#else
    m_val[0] = -int(a);
    m_val[1] = -int(b);
    m_val[2] = -int(c);
    m_val[3] = -int(d);
    m_val[4] = -int(e);
    m_val[5] = -int(f);
    m_val[6] = -int(g);
    m_val[7] = -int(h);
#endif
}

OIIO_FORCEINLINE vbool8::vbool8 (bool a, bool b, bool c, bool d,
                               bool e, bool f, bool g, bool h) {
    load (a, b, c, d, e, f, g, h);
}

OIIO_FORCEINLINE vbool8::vbool8 (int a, int b, int c, int d,
                                 int e, int f, int g, int h) {
    load (bool(a), bool(b), bool(c), bool(d),
          bool(e), bool(f), bool(g), bool(h));
}

OIIO_FORCEINLINE vbool8::vbool8 (const bool *a) {
    load (a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
}


OIIO_FORCEINLINE const vbool8& vbool8::operator= (bool a) {
    load(a);
    return *this;
}

OIIO_FORCEINLINE const vbool8& vbool8::operator= (const vbool8 & other) {
    m_simd = other.m_simd;
    return *this;
}

OIIO_FORCEINLINE int vbool8::bitmask () const {
#if OIIO_SIMD_AVX
    return _mm256_movemask_ps(m_simd);
#else
    return lo().bitmask() | (hi().bitmask() << 4);
#endif
}


OIIO_FORCEINLINE vbool8
vbool8::from_bitmask (int bitmask) {
    // I think this is a fast conversion from int bitmask to vbool8
    return (vint8::Giota() & vint8(bitmask)) != vint8::Zero();
}


OIIO_FORCEINLINE void vbool8::clear () {
#if OIIO_SIMD_AVX
    m_simd = _mm256_setzero_ps();
#else
    *this = false;
#endif
}

OIIO_FORCEINLINE const vbool8 vbool8::False () {
#if OIIO_SIMD_AVX
    return _mm256_setzero_ps();
#else
    return false;
#endif
}


OIIO_FORCEINLINE const vbool8 vbool8::True () {
#if OIIO_SIMD_AVX
# if OIIO_SIMD_AVX >= 2 && (OIIO_GNUC_VERSION > 50000)
    // Fastest way to fill with all 1 bits is to cmp any value to itself.
    __m256i anyval = _mm256_undefined_si256();
    return _mm256_castsi256_ps (_mm256_cmpeq_epi8 (anyval, anyval));
# else
    return _mm256_castsi256_ps (_mm256_set1_epi32 (-1));
# endif
#else
    return true;
#endif
}


OIIO_FORCEINLINE void vbool8::store (bool *values) const {
    SIMD_DO (values[i] = m_val[i] ? true : false);
}

OIIO_FORCEINLINE void vbool8::store (bool *values, int n) const {
    OIIO_DASSERT (n >= 0 && n <= elements);
    for (int i = 0; i < n; ++i)
        values[i] = m_val[i] ? true : false;
}


OIIO_FORCEINLINE vbool4 vbool8::lo () const {
#if OIIO_SIMD_AVX
    return _mm256_castps256_ps128 (simd());
#else
    return m_4[0];
#endif
}

OIIO_FORCEINLINE vbool4 vbool8::hi () const {
#if OIIO_SIMD_AVX
    return _mm256_extractf128_ps (simd(), 1);
#else
    return m_4[1];
#endif
}


OIIO_FORCEINLINE vbool8::vbool8 (const vbool4& lo, const vbool4 &hi) {
#if OIIO_SIMD_AVX
    __m256 r = _mm256_castps128_ps256 (lo);
    m_simd = _mm256_insertf128_ps (r, hi, 1);
    // N.B. equivalent, if available: m_simd = _mm256_set_m128 (hi, lo);
#else
    m_4[0] = lo;
    m_4[1] = hi;
#endif
}


OIIO_FORCEINLINE vbool8 operator! (const vbool8 & a) {
#if OIIO_SIMD_AVX
    return _mm256_xor_ps (a.simd(), vbool8::True());
#else
    SIMD_RETURN (vbool8, a[i] ^ (-1));
#endif
}

OIIO_FORCEINLINE vbool8 operator& (const vbool8 & a, const vbool8 & b) {
#if OIIO_SIMD_AVX
    return _mm256_and_ps (a.simd(), b.simd());
#else
    SIMD_RETURN (vbool8, a[i] & b[i]);
#endif
}

OIIO_FORCEINLINE vbool8 operator| (const vbool8 & a, const vbool8 & b) {
#if OIIO_SIMD_AVX
    return _mm256_or_ps (a.simd(), b.simd());
#else
    SIMD_RETURN (vbool8, a[i] | b[i]);
#endif
}

OIIO_FORCEINLINE vbool8 operator^ (const vbool8& a, const vbool8& b) {
#if OIIO_SIMD_AVX
    return _mm256_xor_ps (a.simd(), b.simd());
#else
    SIMD_RETURN (vbool8, a[i] ^ b[i]);
#endif
}


OIIO_FORCEINLINE const vbool8& operator&= (vbool8& a, const vbool8 &b) {
    return a = a & b;
}

OIIO_FORCEINLINE const vbool8& operator|= (vbool8& a, const vbool8& b) {
    return a = a | b;
}

OIIO_FORCEINLINE const vbool8& operator^= (vbool8& a, const vbool8& b) {
    return a = a ^ b;
}


OIIO_FORCEINLINE vbool8 operator~ (const vbool8& a) {
#if OIIO_SIMD_AVX
    // Fastest way to bit-complement in SSE is to xor with 0xffffffff.
    return _mm256_xor_ps (a.simd(), vbool8::True());
#else
    SIMD_RETURN (vbool8, ~a[i]);
#endif
}


OIIO_FORCEINLINE vbool8 operator== (const vbool8 & a, const vbool8 & b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_castsi256_ps (_mm256_cmpeq_epi32 (_mm256_castps_si256 (a), _mm256_castps_si256(b)));
#elif OIIO_SIMD_AVX
    return _mm256_cmp_ps (a, b, _CMP_EQ_UQ);
#else
    SIMD_RETURN (vbool8, a[i] == b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool8 operator!= (const vbool8 & a, const vbool8 & b) {
#if OIIO_SIMD_AVX
    return _mm256_xor_ps (a, b);
#else
    SIMD_RETURN (vbool8, a[i] != b[i] ? -1 : 0);
#endif
}


template<int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7>
OIIO_FORCEINLINE vbool8 shuffle (const vbool8& a) {
#if OIIO_SIMD_AVX >= 2
    vint8 index (i0, i1, i2, i3, i4, i5, i6, i7);
    return _mm256_permutevar8x32_ps (a.simd(), index.simd());
#else
    return vbool8 (a[i0], a[i1], a[i2], a[i3], a[i4], a[i5], a[i6], a[i7]);
#endif
}

template<int i> OIIO_FORCEINLINE vbool8 shuffle (const vbool8& a) {
    return shuffle<i,i,i,i,i,i,i,i>(a);
}


template<int i>
OIIO_FORCEINLINE bool extract (const vbool8& a) {
#if OIIO_SIMD_AVX && !_WIN32
    return _mm256_extract_epi32(_mm256_castps_si256(a.simd()), i);  // SSE4.1 only
#else
    return a[i];
#endif
}

template<int i>
OIIO_FORCEINLINE vbool8 insert (const vbool8& a, bool val) {
#if OIIO_SIMD_AVX && !_WIN32
    int ival = -int(val);
    return _mm256_castsi256_ps (_mm256_insert_epi32 (_mm256_castps_si256(a.simd()), ival, i));
#else
    vbool8 tmp = a;
    tmp[i] = -int(val);
    return tmp;
#endif
}


OIIO_FORCEINLINE bool reduce_and (const vbool8& v) {
#if OIIO_SIMD_AVX
    return _mm256_testc_ps (v, vbool8(true)) != 0;
    // return _mm256_movemask_ps(v.simd()) == 0xff;
#else
    SIMD_RETURN_REDUCE (bool, true, r &= bool(v[i]));
#endif
}

OIIO_FORCEINLINE bool reduce_or (const vbool8& v) {
#if OIIO_SIMD_AVX
    return ! _mm256_testz_ps (v, v);   // FIXME? Not in all immintrin.h !
    // return _mm256_movemask_ps(v) != 0;
#else
    SIMD_RETURN_REDUCE (bool, false, r |= bool(v[i]));
#endif
}


OIIO_FORCEINLINE bool all (const vbool8& v) { return reduce_and(v) == true; }
OIIO_FORCEINLINE bool any (const vbool8& v) { return reduce_or(v) == true; }
OIIO_FORCEINLINE bool none (const vbool8& v) { return reduce_or(v) == false; }



//////////////////////////////////////////////////////////////////////
// vbool16 implementation


OIIO_FORCEINLINE int vbool16::operator[] (int i) const {
    OIIO_DASSERT(i >= 0 && i < elements);
#if OIIO_SIMD_AVX >= 512
    return (int(m_simd) >> i) & 1;
#else
    return (m_bits >> i) & 1;
#endif
}

OIIO_FORCEINLINE void vbool16::setcomp (int i, bool value) {
    OIIO_DASSERT(i >= 0 && i < elements);
    int bits = m_bits;
    bits &= (0xffff ^ (1<<i));
    bits |= (int(value)<<i);
    m_bits = bits;
}


OIIO_FORCEINLINE std::ostream& operator<< (std::ostream& cout, const vbool16& a) {
    cout << a[0];
    for (int i = 1; i < a.elements; ++i)
        cout << ' ' << a[i];
    return cout;
}


OIIO_FORCEINLINE void vbool16::load (bool a) {
    m_simd = a ? 0xffff : 0;
}


OIIO_FORCEINLINE void vbool16::load_bitmask (int a) {
    m_simd = simd_t(a);
}


OIIO_FORCEINLINE void vbool16::load (bool v0, bool v1, bool v2, bool v3,
                                    bool v4, bool v5, bool v6, bool v7,
                                    bool v8, bool v9, bool v10, bool v11,
                                    bool v12, bool v13, bool v14, bool v15) {
    m_simd = simd_t((int(v0) << 0) |
                    (int(v1) << 1) |
                    (int(v2) << 2) |
                    (int(v3) << 3) |
                    (int(v4) << 4) |
                    (int(v5) << 5) |
                    (int(v6) << 6) |
                    (int(v7) << 7) |
                    (int(v8) << 8) |
                    (int(v9) << 9) |
                    (int(v10) << 10) |
                    (int(v11) << 11) |
                    (int(v12) << 12) |
                    (int(v13) << 13) |
                    (int(v14) << 14) |
                    (int(v15) << 15));
}

OIIO_FORCEINLINE vbool16::vbool16 (bool v0, bool v1, bool v2, bool v3,
                                 bool v4, bool v5, bool v6, bool v7,
                                 bool v8, bool v9, bool v10, bool v11,
                                 bool v12, bool v13, bool v14, bool v15) {
    load (v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15);
}

OIIO_FORCEINLINE vbool16::vbool16 (int v0, int v1, int v2, int v3,
                                   int v4, int v5, int v6, int v7,
                                   int v8, int v9, int v10, int v11,
                                   int v12, int v13, int v14, int v15) {
    load (bool(v0), bool(v1), bool(v2), bool(v3),
          bool(v4), bool(v5), bool(v6), bool(v7),
          bool(v8), bool(v9), bool(v10), bool(v11),
          bool(v12), bool(v13), bool(v14), bool(v15));
}

OIIO_FORCEINLINE vbool16::vbool16 (const vbool8& a, const vbool8& b) {
    load_bitmask (a.bitmask() | (b.bitmask() << 8));
}

OIIO_FORCEINLINE vbool16::vbool16 (const bool *a) {
    load (a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
          a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15]);
}


OIIO_FORCEINLINE const vbool16& vbool16::operator= (bool a) {
    load(a);
    return *this;
}

OIIO_FORCEINLINE const vbool16& vbool16::operator= (const vbool16 & other) {
    m_simd = other.m_simd;
    return *this;
}


OIIO_FORCEINLINE int vbool16::bitmask () const {
#if OIIO_SIMD_AVX >= 512
    return int(m_simd);
#else
    return int(m_bits);
#endif
}


OIIO_FORCEINLINE void vbool16::clear () {
    m_simd = simd_t(0);
}

OIIO_FORCEINLINE const vbool16 vbool16::False () {
    return simd_t(0);
}


OIIO_FORCEINLINE const vbool16 vbool16::True () {
    return simd_t(0xffff);
}


OIIO_FORCEINLINE void vbool16::store (bool *values) const {
    SIMD_DO (values[i] = m_bits & (1<<i));
}

OIIO_FORCEINLINE void vbool16::store (bool *values, int n) const {
    OIIO_DASSERT (n >= 0 && n <= elements);
    for (int i = 0; i < n; ++i)
        values[i] = m_bits & (1<<i);
}



OIIO_FORCEINLINE vbool8 vbool16::lo () const {
#if OIIO_SIMD_AVX >= 512
    return _mm256_castsi256_ps (_mm256_maskz_set1_epi32 (bitmask()&0xff, -1));
#else
    SIMD_RETURN (vbool8, (*this)[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool8 vbool16::hi () const {
#if OIIO_SIMD_AVX >= 512
    return _mm256_castsi256_ps (_mm256_maskz_set1_epi32 (bitmask()>>8, -1));
#else
    SIMD_RETURN (vbool8, (*this)[i+8] ? -1 : 0);
#endif
}


OIIO_FORCEINLINE vbool16 operator! (const vbool16 & a) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_knot (a.simd());
#else
    return vbool16 (a.m_bits ^ 0xffff);
#endif
}

OIIO_FORCEINLINE vbool16 operator& (const vbool16 & a, const vbool16 & b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_kand (a.simd(), b.simd());
#else
    return vbool16 (a.m_bits & b.m_bits);
#endif
}

OIIO_FORCEINLINE vbool16 operator| (const vbool16 & a, const vbool16 & b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_kor (a.simd(), b.simd());
#else
    return vbool16 (a.m_bits | b.m_bits);
#endif
}

OIIO_FORCEINLINE vbool16 operator^ (const vbool16& a, const vbool16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_kxor (a.simd(), b.simd());
#else
    return vbool16 (a.m_bits ^ b.m_bits);
#endif
}


OIIO_FORCEINLINE const vbool16& operator&= (vbool16& a, const vbool16 &b) {
    return a = a & b;
}

OIIO_FORCEINLINE const vbool16& operator|= (vbool16& a, const vbool16& b) {
    return a = a | b;
}

OIIO_FORCEINLINE const vbool16& operator^= (vbool16& a, const vbool16& b) {
    return a = a ^ b;
}


OIIO_FORCEINLINE vbool16 operator~ (const vbool16& a) {
    return a ^ vbool16::True();
}


OIIO_FORCEINLINE vbool16 operator== (const vbool16 & a, const vbool16 & b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_kxnor (a.simd(), b.simd());
#else
    return vbool16 (!(a.m_bits ^ b.m_bits));
#endif
}

OIIO_FORCEINLINE vbool16 operator!= (const vbool16 & a, const vbool16 & b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_kxor (a.simd(), b.simd());
#else
    return vbool16 (a.m_bits ^ b.m_bits);
#endif
}


template<int i>
OIIO_FORCEINLINE bool extract (const vbool16& a) {
    return a[i];
}

template<int i>
OIIO_FORCEINLINE vbool16 insert (const vbool16& a, bool val) {
    vbool16 tmp = a;
    tmp.setcomp (i, val);
    return tmp;
}


OIIO_FORCEINLINE bool reduce_and (const vbool16& v) {
    return v.bitmask() == 0xffff;
}

OIIO_FORCEINLINE bool reduce_or (const vbool16& v) {
    return v.bitmask() != 0;
}


OIIO_FORCEINLINE bool all (const vbool16& v) { return reduce_and(v) == true; }
OIIO_FORCEINLINE bool any (const vbool16& v) { return reduce_or(v) == true; }
OIIO_FORCEINLINE bool none (const vbool16& v) { return reduce_or(v) == false; }






//////////////////////////////////////////////////////////////////////
// vint4 implementation

OIIO_FORCEINLINE const vint4 & vint4::operator= (const vint4& other) {
    m_simd = other.m_simd;
    return *this;
}

OIIO_FORCEINLINE int vint4::operator[] (int i) const {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}

OIIO_FORCEINLINE int& vint4::operator[] (int i) {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}

OIIO_FORCEINLINE void vint4::setcomp (int i, int val) {
    OIIO_DASSERT(i<elements);
    m_val[i] = val;
}


OIIO_FORCEINLINE void vint4::load (int a) {
#if OIIO_SIMD_SSE
    m_simd = _mm_set1_epi32 (a);
#else
    SIMD_CONSTRUCT (a);
#endif
}



OIIO_FORCEINLINE void vint4::load (int a, int b, int c, int d) {
#if OIIO_SIMD_SSE
    m_simd = _mm_set_epi32 (d, c, b, a);
#else
    m_val[0] = a;
    m_val[1] = b;
    m_val[2] = c;
    m_val[3] = d;
#endif
}


// OIIO_FORCEINLINE void vint4::load (int a, int b, int c, int d,
//                                   int e, int f, int g, int h) {
//     load (a, b, c, d);
// }



OIIO_FORCEINLINE void vint4::load (const int *values) {
#if OIIO_SIMD_SSE
    m_simd = _mm_loadu_si128 ((const simd_t *)values);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vint4::load (const int *values, int n)
{
    OIIO_DASSERT (n >= 0 && n <= elements);
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm_maskz_loadu_epi32 (__mmask8(~(0xf << n)), values);
#elif OIIO_SIMD_SSE
    switch (n) {
    case 1:
        m_simd = _mm_castps_si128 (_mm_load_ss ((const float *)values));
        break;
    case 2:
        // Trickery: load one double worth of bits!
        m_simd = _mm_castpd_si128 (_mm_load_sd ((const double*)values));
        break;
    case 3:
        // Trickery: load one double worth of bits, then a float,
        // and combine, casting to ints.
        m_simd = _mm_castps_si128 (_mm_movelh_ps(_mm_castpd_ps(_mm_load_sd((const double*)values)),
                                                _mm_load_ss ((const float *)values + 2)));
        break;
    case 4:
        m_simd = _mm_loadu_si128 ((const simd_t *)values);
        break;
    default:
        clear ();
        break;
    }
#else
    for (int i = 0; i < n; ++i)
        m_val[i] = values[i];
    for (int i = n; i < elements; ++i)
        m_val[i] = 0;
#endif
}


OIIO_FORCEINLINE void vint4::load (const unsigned short *values) {
#if OIIO_SIMD_SSE >= 4
    // Trickery: load one double worth of bits = 4 ushorts!
    simd_t a = _mm_castpd_si128 (_mm_load_sd ((const double *)values));
    m_simd = _mm_cvtepu16_epi32 (a);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vint4::load (const short *values) {
#if OIIO_SIMD_SSE >= 4
    // Trickery: load one double worth of bits = 4 shorts!
    simd_t a = _mm_castpd_si128 (_mm_load_sd ((const double *)values));
    m_simd = _mm_cvtepi16_epi32 (a);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vint4::load (const unsigned char *values) {
#if OIIO_SIMD_SSE >= 4
    // Trickery: load one float worth of bits = 4 uchars!
    simd_t a = _mm_castps_si128 (_mm_load_ss ((const float *)values));
    m_simd = _mm_cvtepu8_epi32 (a);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vint4::load (const char *values) {
#if OIIO_SIMD_SSE >= 4
    // Trickery: load one float worth of bits = 4 chars!
    simd_t a = _mm_castps_si128 (_mm_load_ss ((const float *)values));
    m_simd = _mm_cvtepi8_epi32 (a);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE vint4::vint4 (int a) { load(a); }

OIIO_FORCEINLINE vint4::vint4 (int a, int b) { load(a,a,b,b); }

OIIO_FORCEINLINE vint4::vint4 (int a, int b, int c, int d) { load(a,b,c,d); }

// OIIO_FORCEINLINE vint4::vint4 (int a, int b, int c, int d,
//                              int e, int f, int g, int h) {
//     load(a,b,c,d,e,f,g,h);
// }

OIIO_FORCEINLINE vint4::vint4 (const int *vals) { load (vals); }
OIIO_FORCEINLINE vint4::vint4 (const unsigned short *vals) { load(vals); }
OIIO_FORCEINLINE vint4::vint4 (const short *vals) { load(vals); }
OIIO_FORCEINLINE vint4::vint4 (const unsigned char *vals) { load(vals); }
OIIO_FORCEINLINE vint4::vint4 (const char *vals) { load(vals); }

OIIO_FORCEINLINE const vint4 & vint4::operator= (int a) { load(a); return *this; }


OIIO_FORCEINLINE void vint4::store (int *values) const {
#if OIIO_SIMD_SSE
    // Use an unaligned store -- it's just as fast when the memory turns
    // out to be aligned, nearly as fast even when unaligned. Not worth
    // the headache of using stores that require alignment.
    _mm_storeu_si128 ((simd_t *)values, m_simd);
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}


OIIO_FORCEINLINE void vint4::load_mask (int mask, const value_t *values) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm_maskz_loadu_epi32 (__mmask8(mask), (const simd_t *)values);
#elif OIIO_SIMD_AVX >= 2
    m_simd = _mm_maskload_epi32 (values, _mm_castps_si128(vbool_t::from_bitmask(mask)));
#else
    SIMD_CONSTRUCT ((mask>>i) & 1 ? values[i] : 0);
#endif
}


OIIO_FORCEINLINE void vint4::load_mask (const vbool_t& mask, const value_t *values) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm_maskz_loadu_epi32 (__mmask8(mask.bitmask()), (const simd_t *)values);
#elif OIIO_SIMD_AVX >= 2
    m_simd = _mm_maskload_epi32 (values, _mm_castps_si128(mask));
#else
    SIMD_CONSTRUCT (mask[i] ? values[i] : 0);
#endif
}


OIIO_FORCEINLINE void vint4::store_mask (int mask, value_t *values) const {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm_mask_storeu_epi32 (values, __mmask8(mask), m_simd);
#elif OIIO_SIMD_AVX >= 2
    _mm_maskstore_epi32 (values, _mm_castps_si128(vbool_t::from_bitmask(mask)), m_simd);
#else
    SIMD_DO (if ((mask>>i) & 1) values[i] = (*this)[i]);
#endif
}


OIIO_FORCEINLINE void vint4::store_mask (const vbool_t& mask, value_t *values) const {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm_mask_storeu_epi32 (values, mask.bitmask(), m_simd);
#elif OIIO_SIMD_AVX >= 2
    _mm_maskstore_epi32 (values, _mm_castps_si128(mask), m_simd);
#else
    SIMD_DO (if (mask[i]) values[i] = (*this)[i]);
#endif
}


template <int scale>
OIIO_FORCEINLINE void
vint4::gather (const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm_i32gather_epi32 (baseptr, vindex, scale);
#else
    SIMD_CONSTRUCT (*(const value_t *)((const char *)baseptr + vindex[i]*scale));
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vint4::gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm_mask_i32gather_epi32 (m_simd, baseptr, vindex, _mm_cvtps_epi32(mask), scale);
#else
    SIMD_CONSTRUCT (mask[i] ? *(const value_t *)((const char *)baseptr + vindex[i]*scale) : 0);
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vint4::scatter (value_t *baseptr, const vint_t& vindex) const
{
#if 0 && OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // FIXME: disable because it benchmarks slower than the dumb way
    _mm_i32scatter_epi32 (baseptr, vindex, m_simd, scale);
#else
    SIMD_DO (*(value_t *)((char *)baseptr + vindex[i]*scale) = m_val[i]);
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vint4::scatter_mask (const bool_t& mask, value_t *baseptr,
                     const vint_t& vindex) const
{
#if 0 && OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // FIXME: disable because it benchmarks slower than the dumb way
    _mm_mask_i32scatter_epi32 (baseptr, mask.bitmask(), vindex, m_simd, scale);
#else
    SIMD_DO (if (mask[i]) *(value_t *)((char *)baseptr + vindex[i]*scale) = m_val[i]);
#endif
}


OIIO_FORCEINLINE void vint4::clear () {
#if OIIO_SIMD_SSE
    m_simd = _mm_setzero_si128();
#else
    *this = 0;
#endif
}



OIIO_FORCEINLINE const vint4 vint4::Zero () {
#if OIIO_SIMD_SSE
    return _mm_setzero_si128();
#else
    return 0;
#endif
}


OIIO_FORCEINLINE const vint4 vint4::One () { return vint4(1); }

OIIO_FORCEINLINE const vint4 vint4::NegOne () {
#if OIIO_SIMD_SSE
    // Fastest way to fill an __m128 with all 1 bits is to cmpeq_epi8
    // any value to itself.
# if OIIO_SIMD_AVX && (OIIO_GNUC_VERSION > 50000)
    __m128i anyval = _mm_undefined_si128();
# else
    __m128i anyval = _mm_setzero_si128();
# endif
    return _mm_cmpeq_epi8 (anyval, anyval);
#else
    return vint4(-1);
#endif
}



OIIO_FORCEINLINE const vint4 vint4::Iota (int start, int step) {
    return vint4 (start+0*step, start+1*step, start+2*step, start+3*step);
}


OIIO_FORCEINLINE const vint4 vint4::Giota () {
    return vint4 (1<<0, 1<<1, 1<<2, 1<<3);
}


OIIO_FORCEINLINE vint4 operator+ (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return _mm_add_epi32 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint4, a[i] + b[i]);
#endif
}

OIIO_FORCEINLINE const vint4& operator+= (vint4& a, const vint4& b) {
    return a = a + b;
}


OIIO_FORCEINLINE vint4 operator- (const vint4& a) {
#if OIIO_SIMD_SSE
    return _mm_sub_epi32 (_mm_setzero_si128(), a);
#else
    SIMD_RETURN (vint4, -a[i]);
#endif
}


OIIO_FORCEINLINE vint4 operator- (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return _mm_sub_epi32 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint4, a[i] - b[i]);
#endif
}


OIIO_FORCEINLINE const vint4 &operator-= (vint4& a, const vint4& b) {
    return a = a - b;
}


#if OIIO_SIMD_SSE
// Shamelessly lifted from Syrah which lifted from Manta which lifted it
// from intel.com
OIIO_FORCEINLINE __m128i mul_epi32 (__m128i a, __m128i b) {
#if OIIO_SIMD_SSE >= 4  /* SSE >= 4.1 */
    return _mm_mullo_epi32(a, b);
#else
    // Prior to SSE 4.1, there is no _mm_mullo_epi32 instruction, so we have
    // to fake it.
    __m128i t0;
    __m128i t1;
    t0 = _mm_mul_epu32 (a, b);
    t1 = _mm_mul_epu32 (_mm_shuffle_epi32 (a, 0xB1),
                        _mm_shuffle_epi32 (b, 0xB1));
    t0 = _mm_shuffle_epi32 (t0, 0xD8);
    t1 = _mm_shuffle_epi32 (t1, 0xD8);
    return _mm_unpacklo_epi32 (t0, t1);
#endif
}
#endif


OIIO_FORCEINLINE vint4 operator* (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return mul_epi32 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint4, a[i] * b[i]);
#endif
}


OIIO_FORCEINLINE const vint4& operator*= (vint4& a, const vint4& b) { return a = a * b; }
OIIO_FORCEINLINE const vint4& operator*= (vint4& a, int b) { return a = a * b; }


OIIO_FORCEINLINE vint4 operator/ (const vint4& a, const vint4& b) {
    // NO INTEGER DIVISION IN SSE!
    SIMD_RETURN (vint4, a[i] / b[i]);
}


OIIO_FORCEINLINE const vint4& operator/= (vint4& a, const vint4& b) { return a = a / b; }

OIIO_FORCEINLINE vint4 operator% (const vint4& a, const vint4& b) {
    // NO INTEGER MODULUS IN SSE!
    SIMD_RETURN (vint4, a[i] % b[i]);
}



OIIO_FORCEINLINE const vint4& operator%= (vint4& a, const vint4& b) { return a = a % b; }


OIIO_FORCEINLINE vint4 operator% (const vint4& a, int w) {
    // NO INTEGER MODULUS in SSE!
    SIMD_RETURN (vint4, a[i] % w);
}


OIIO_FORCEINLINE const vint4& operator%= (vint4& a, int b) { return a = a % b; }


OIIO_FORCEINLINE vint4 operator& (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return _mm_and_si128 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint4, a[i] & b[i]);
#endif
}


OIIO_FORCEINLINE const vint4& operator&= (vint4& a, const vint4& b) { return a = a & b; }



OIIO_FORCEINLINE vint4 operator| (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return _mm_or_si128 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint4, a[i] | b[i]);
#endif
}

OIIO_FORCEINLINE const vint4& operator|= (vint4& a, const vint4& b) { return a = a | b; }


OIIO_FORCEINLINE vint4 operator^ (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return _mm_xor_si128 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint4, a[i] ^ b[i]);
#endif
}


OIIO_FORCEINLINE const vint4& operator^= (vint4& a, const vint4& b) { return a = a ^ b; }


OIIO_FORCEINLINE vint4 operator~ (const vint4& a) {
#if OIIO_SIMD_SSE
    return a ^ a.NegOne();
#else
    SIMD_RETURN (vint4, ~a[i]);
#endif
}

OIIO_FORCEINLINE vint4 operator<< (const vint4& a, unsigned int bits) {
#if OIIO_SIMD_SSE
    return _mm_slli_epi32 (a, bits);
#else
    SIMD_RETURN (vint4, a[i] << bits);
#endif
}

OIIO_FORCEINLINE const vint4& operator<<= (vint4& a, const unsigned int bits) {
    return a = a << bits;
}


OIIO_FORCEINLINE vint4 operator>> (const vint4& a, const unsigned int bits) {
#if OIIO_SIMD_SSE
    return _mm_srai_epi32 (a, bits);
#else
    SIMD_RETURN (vint4, a[i] >> bits);
#endif
}

OIIO_FORCEINLINE const vint4& operator>>= (vint4& a, const unsigned int bits) {
    return a = a >> bits;
}


OIIO_FORCEINLINE vint4 srl (const vint4& a, const unsigned int bits) {
#if OIIO_SIMD_SSE
    return _mm_srli_epi32 (a, bits);
#else
    SIMD_RETURN (vint4, int ((unsigned int)(a[i]) >> bits));
#endif
}


OIIO_FORCEINLINE vbool4 operator== (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return _mm_castsi128_ps(_mm_cmpeq_epi32 (a, b));
#else
    SIMD_RETURN (vbool4, a[i] == b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool4 operator!= (const vint4& a, const vint4& b) {
    return ! (a == b);
}


OIIO_FORCEINLINE vbool4 operator> (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return _mm_castsi128_ps(_mm_cmpgt_epi32 (a, b));
#else
    SIMD_RETURN (vbool4, a[i] > b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool4 operator< (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return _mm_castsi128_ps(_mm_cmplt_epi32 (a, b));
#else
    SIMD_RETURN (vbool4, a[i] < b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool4 operator>= (const vint4& a, const vint4& b) {
    return (b < a) | (a == b);
}

OIIO_FORCEINLINE vbool4 operator<= (const vint4& a, const vint4& b) {
    return (b > a) | (a == b);
}

inline std::ostream& operator<< (std::ostream& cout, const vint4& val) {
    cout << val[0];
    for (int i = 1; i < val.elements; ++i)
        cout << ' ' << val[i];
    return cout;
}


OIIO_FORCEINLINE void vint4::store (int *values, int n) const {
    OIIO_DASSERT (n >= 0 && n <= elements);
#if 0 && OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // This SHOULD be fast, but in my benchmarks, it is slower!
    // (At least on the AVX512 hardware I have, Xeon Silver 4110.)
    // Re-test this periodically with new Intel hardware.
    _mm_mask_storeu_epi32 (values, __mmask8(~(0xf << n)), m_simd);
#elif OIIO_SIMD
    // For full SIMD, there is a speed advantage to storing all components.
    if (n == elements)
        store (values);
    else
        for (int i = 0; i < n; ++i)
            values[i] = m_val[i];
#else
    for (int i = 0; i < n; ++i)
        values[i] = m_val[i];
#endif
}



OIIO_FORCEINLINE void vint4::store (unsigned short *values) const {
#if OIIO_AVX512VL_ENABLED
    _mm_mask_cvtepi32_storeu_epi16 (values, __mmask8(0xf), m_simd);
#elif OIIO_SIMD_SSE
    // Expressed as half-words and considering little endianness, we
    // currently have AxBxCxDx (the 'x' means don't care).
    vint4 clamped = m_simd & vint4(0xffff);   // A0B0C0D0
    vint4 low = _mm_shufflelo_epi16 (clamped, (0<<0) | (2<<2) | (1<<4) | (1<<6));
                    // low = AB00xxxx
    vint4 high = _mm_shufflehi_epi16 (clamped, (1<<0) | (1<<2) | (0<<4) | (2<<6));
                    // high = xxxx00CD
    vint4 highswapped = shuffle_sse<2,3,0,1>(high);  // 00CDxxxx
    vint4 result = low | highswapped;   // ABCDxxxx
    _mm_storel_pd ((double *)values, _mm_castsi128_pd(result));
    // At this point, values[] should hold A,B,C,D
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}



OIIO_FORCEINLINE void vint4::store (unsigned char *values) const {
#if OIIO_AVX512VL_ENABLED
    _mm_mask_cvtepi32_storeu_epi8 (values, __mmask8(0xf), m_simd);
#elif OIIO_SIMD_SSE
    // Expressed as bytes and considering little endianness, we
    // currently have AxBxCxDx (the 'x' means don't care).
    vint4 clamped = m_simd & vint4(0xff);          // A000 B000 C000 D000
    vint4 swapped = shuffle_sse<1,0,3,2>(clamped); // B000 A000 D000 C000
    vint4 shifted = swapped << 8;                  // 0B00 0A00 0D00 0C00
    vint4 merged = clamped | shifted;              // AB00 xxxx CD00 xxxx
    vint4 merged2 = shuffle_sse<2,2,2,2>(merged);  // CD00 ...
    vint4 shifted2 = merged2 << 16;                // 00CD ...
    vint4 result = merged | shifted2;              // ABCD ...
    *(int*)values = result[0]; //extract<0>(result);
    // At this point, values[] should hold A,B,C,D
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}




template<int i0, int i1, int i2, int i3>
OIIO_FORCEINLINE vint4 shuffle (const vint4& a) {
#if OIIO_SIMD_SSE
    return shuffle_sse<i0,i1,i2,i3> (__m128i(a));
#else
    return vint4(a[i0], a[i1], a[i2], a[i3]);
#endif
}

template<int i> OIIO_FORCEINLINE vint4 shuffle (const vint4& a) { return shuffle<i,i,i,i>(a); }


template<int i>
OIIO_FORCEINLINE int extract (const vint4& v) {
#if OIIO_SIMD_SSE >= 4
    return _mm_extract_epi32(v.simd(), i);  // SSE4.1 only
#else
    return v[i];
#endif
}

#if OIIO_SIMD_SSE
template<> OIIO_FORCEINLINE int extract<0> (const vint4& v) {
    return _mm_cvtsi128_si32(v.simd());
}
#endif

template<int i>
OIIO_FORCEINLINE vint4 insert (const vint4& a, int val) {
#if OIIO_SIMD_SSE >= 4
    return _mm_insert_epi32 (a.simd(), val, i);
#else
    vint4 tmp = a;
    tmp[i] = val;
    return tmp;
#endif
}



OIIO_FORCEINLINE int vint4::x () const { return extract<0>(*this); }
OIIO_FORCEINLINE int vint4::y () const { return extract<1>(*this); }
OIIO_FORCEINLINE int vint4::z () const { return extract<2>(*this); }
OIIO_FORCEINLINE int vint4::w () const { return extract<3>(*this); }
OIIO_FORCEINLINE void vint4::set_x (int val) { *this = insert<0>(*this, val); }
OIIO_FORCEINLINE void vint4::set_y (int val) { *this = insert<1>(*this, val); }
OIIO_FORCEINLINE void vint4::set_z (int val) { *this = insert<2>(*this, val); }
OIIO_FORCEINLINE void vint4::set_w (int val) { *this = insert<3>(*this, val); }


OIIO_FORCEINLINE vint4 bitcast_to_int (const vbool4& x)
{
#if OIIO_SIMD_SSE
    return _mm_castps_si128 (x.simd());
#else
    return *(vint4 *)&x;
#endif
}

// Old names: (DEPRECATED 1.8)
inline vint4 bitcast_to_int4 (const vbool4& x) { return bitcast_to_int(x); }


OIIO_FORCEINLINE vint4 vreduce_add (const vint4& v) {
#if OIIO_SIMD_SSE >= 3
    // People seem to agree that SSE3 does add reduction best with 2
    // horizontal adds.
    // suppose v = (a, b, c, d)
    simd::vint4 ab_cd = _mm_hadd_epi32 (v.simd(), v.simd());
    // ab_cd = (a+b, c+d, a+b, c+d)
    simd::vint4 abcd = _mm_hadd_epi32 (ab_cd.simd(), ab_cd.simd());
    // all abcd elements are a+b+c+d, return an element as fast as possible
    return abcd;
#elif OIIO_SIMD_SSE >= 2
    // I think this is the best we can do for SSE2, and I'm still not sure
    // it's faster than the default scalar operation. But anyway...
    // suppose v = (a, b, c, d)
    vint4 ab_ab_cd_cd = shuffle<1,0,3,2>(v) + v;
    // ab_ab_cd_cd = (b,a,d,c) + (a,b,c,d) = (a+b,a+b,c+d,c+d)
    vint4 cd_cd_ab_ab = shuffle<2,3,0,1>(ab_ab_cd_cd);
    // cd_cd_ab_ab = (c+d,c+d,a+b,a+b)
    vint4 abcd = ab_ab_cd_cd + cd_cd_ab_ab;   // a+b+c+d in all components
    return abcd;
#else
    SIMD_RETURN_REDUCE (vint4, 0, r += v[i]);
#endif
}


OIIO_FORCEINLINE int reduce_add (const vint4& v) {
#if OIIO_SIMD_SSE
    return extract<0> (vreduce_add(v));
#else
    SIMD_RETURN_REDUCE (int, 0, r += v[i]);
#endif
}


OIIO_FORCEINLINE int reduce_and (const vint4& v) {
#if OIIO_SIMD_SSE
    vint4 ab = v & shuffle<1,1,3,3>(v); // ab bb cd dd
    vint4 abcd = ab & shuffle<2>(ab);
    return extract<0>(abcd);
#else
    SIMD_RETURN_REDUCE (int, -1, r &= v[i]);
#endif
}


OIIO_FORCEINLINE int reduce_or (const vint4& v) {
#if OIIO_SIMD_SSE
    vint4 ab = v | shuffle<1,1,3,3>(v); // ab bb cd dd
    vint4 abcd = ab | shuffle<2>(ab);
    return extract<0>(abcd);
#else
    SIMD_RETURN_REDUCE (int, 0, r |= v[i]);
#endif
}



OIIO_FORCEINLINE vint4 blend (const vint4& a, const vint4& b, const vbool4& mask) {
#if OIIO_SIMD_SSE >= 4 /* SSE >= 4.1 */
    return _mm_castps_si128 (_mm_blendv_ps (_mm_castsi128_ps(a.simd()),
                                            _mm_castsi128_ps(b.simd()), mask));
#elif OIIO_SIMD_SSE
    return _mm_or_si128 (_mm_and_si128(_mm_castps_si128(mask.simd()), b.simd()),
                         _mm_andnot_si128(_mm_castps_si128(mask.simd()), a.simd()));
#else
    SIMD_RETURN (vint4, mask[i] ? b[i] : a[i]);
#endif
}

OIIO_FORCEINLINE vint4 blend0 (const vint4& a, const vbool4& mask) {
#if OIIO_SIMD_SSE
    return _mm_and_si128(_mm_castps_si128(mask), a.simd());
#else
    SIMD_RETURN (vint4, mask[i] ? a[i] : 0.0f);
#endif
}


OIIO_FORCEINLINE vint4 blend0not (const vint4& a, const vbool4& mask) {
#if OIIO_SIMD_SSE
    return _mm_andnot_si128(_mm_castps_si128(mask), a.simd());
#else
    SIMD_RETURN (vint4, mask[i] ? 0.0f : a[i]);
#endif
}


OIIO_FORCEINLINE vint4 select (const vbool4& mask, const vint4& a, const vint4& b) {
    return blend (b, a, mask);
}



OIIO_FORCEINLINE vint4 abs (const vint4& a) {
#if OIIO_SIMD_SSE >= 3
    return _mm_abs_epi32(a.simd());
#else
    SIMD_RETURN (vint4, std::abs(a[i]));
#endif
}



OIIO_FORCEINLINE vint4 min (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE >= 4 /* SSE >= 4.1 */
    return _mm_min_epi32 (a, b);
#else
    SIMD_RETURN (vint4, std::min(a[i], b[i]));
#endif
}


OIIO_FORCEINLINE vint4 max (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE >= 4 /* SSE >= 4.1 */
    return _mm_max_epi32 (a, b);
#else
    SIMD_RETURN (vint4, std::max(a[i], b[i]));
#endif
}


OIIO_FORCEINLINE vint4 rotl(const vint4& x, int s) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // return _mm_rol_epi32 (x, s);
    // We want to do this ^^^ but this intrinsic only takes an *immediate*
    // argument for s, and there isn't a way to express in C++ that a
    // parameter must be an immediate/literal value from the caller.
    return (x<<s) | srl(x,32-s);
#else
    return (x<<s) | srl(x,32-s);
#endif
}

// DEPRECATED (2.1)
OIIO_FORCEINLINE vint4 rotl32 (const vint4& x, const unsigned int k) {
    return rotl(x, k);
}


OIIO_FORCEINLINE vint4 andnot (const vint4& a, const vint4& b) {
#if OIIO_SIMD_SSE
    return _mm_andnot_si128 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint4, ~(a[i]) & b[i]);
#endif
}


// Implementation had to be after the definition of vint4::Zero.
OIIO_FORCEINLINE vbool4::vbool4 (const vint4& ival) {
    m_simd = (ival != vint4::Zero());
}



OIIO_FORCEINLINE vint4 safe_mod (const vint4& a, const vint4& b) {
    // NO INTEGER MODULUS IN SSE!
    SIMD_RETURN (vint4, b[i] ? a[i] % b[i] : 0);
}

OIIO_FORCEINLINE vint4 safe_mod (const vint4& a, int b) {
    return b ? (a % b) : vint4::Zero();
}




//////////////////////////////////////////////////////////////////////
// vint8 implementation

OIIO_FORCEINLINE const vint8 & vint8::operator= (const vint8& other) {
    m_simd = other.m_simd;
    return *this;
}

OIIO_FORCEINLINE int vint8::operator[] (int i) const {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}

OIIO_FORCEINLINE int& vint8::operator[] (int i) {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}

OIIO_FORCEINLINE void vint8::setcomp (int i, int val) {
    OIIO_DASSERT(i<elements);
    m_val[i] = val;
}


OIIO_FORCEINLINE void vint8::load (int a) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_set1_epi32 (a);
#else
    SIMD_CONSTRUCT (a);
#endif
}


OIIO_FORCEINLINE void vint8::load (int a, int b, int c, int d,
                                  int e, int f, int g, int h) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_set_epi32 (h, g, f, e, d, c, b, a);
#else
    m_val[0] = a;
    m_val[1] = b;
    m_val[2] = c;
    m_val[3] = d;
    m_val[4] = e;
    m_val[5] = f;
    m_val[6] = g;
    m_val[7] = h;
#endif
}


OIIO_FORCEINLINE void vint8::load (const int *values) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_loadu_si256 ((const simd_t *)values);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vint8::load (const int *values, int n)
{
    OIIO_DASSERT (n >= 0 && n <= elements);
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm256_maskz_loadu_epi32 ((~(0xff << n)), values);
#elif OIIO_SIMD_SSE
    if (n > 4) {
        vint4 lo, hi;
        lo.load (values);
        hi.load (values+4, n-4);
        m_4[0] = lo;
        m_4[1] = hi;
    } else {
        vint4 lo, hi;
        lo.load (values, n);
        hi.clear();
        m_4[0] = lo;
        m_4[1] = hi;
    }
#else
    for (int i = 0; i < n; ++i)
        m_val[i] = values[i];
    for (int i = n; i < elements; ++i)
        m_val[i] = 0;
#endif
}


OIIO_FORCEINLINE void vint8::load (const short *values) {
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*)values));
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}

OIIO_FORCEINLINE void vint8::load (const unsigned short *values) {
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm256_cvtepu16_epi32(_mm_loadu_si128((__m128i*)values));
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vint8::load (const char *values) {
#if OIIO_SIMD_AVX >= 2
    __m128i bytes = _mm_castpd_si128 (_mm_load_sd ((const double *)values));
    m_simd = _mm256_cvtepi8_epi32 (bytes);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}

OIIO_FORCEINLINE void vint8::load (const unsigned char *values) {
#if OIIO_SIMD_AVX >= 2
    __m128i bytes = _mm_castpd_si128 (_mm_load_sd ((const double *)values));
    m_simd = _mm256_cvtepu8_epi32 (bytes);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}



OIIO_FORCEINLINE vint8::vint8 (int a) { load(a); }

OIIO_FORCEINLINE vint8::vint8 (int a, int b, int c, int d,
                             int e, int f, int g, int h) {
    load(a,b,c,d,e,f,g,h);
}

OIIO_FORCEINLINE vint8::vint8 (const int *vals) { load (vals); }
OIIO_FORCEINLINE vint8::vint8 (const unsigned short *vals) { load(vals); }
OIIO_FORCEINLINE vint8::vint8 (const short *vals) { load(vals); }
OIIO_FORCEINLINE vint8::vint8 (const unsigned char *vals) { load(vals); }
OIIO_FORCEINLINE vint8::vint8 (const char *vals) { load(vals); }

OIIO_FORCEINLINE const vint8 & vint8::operator= (int a) { load(a); return *this; }


OIIO_FORCEINLINE void vint8::store (int *values) const {
#if OIIO_SIMD_AVX
    // Use an unaligned store -- it's just as fast when the memory turns
    // out to be aligned, nearly as fast even when unaligned. Not worth
    // the headache of using stores that require alignment.
    _mm256_storeu_si256 ((simd_t *)values, m_simd);
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}


OIIO_FORCEINLINE void vint8::load_mask (int mask, const int *values) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm256_maskz_loadu_epi32 (__mmask8(mask), (const simd_t *)values);
#elif OIIO_SIMD_AVX >= 2
    m_simd = _mm256_maskload_epi32 (values, _mm256_castps_si256(vbool8::from_bitmask(mask)));
#else
    SIMD_CONSTRUCT ((mask>>i) & 1 ? values[i] : 0);
#endif
}


OIIO_FORCEINLINE void vint8::load_mask (const vbool8& mask, const int *values) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm256_maskz_loadu_epi32 (__mmask8(mask.bitmask()), (const simd_t *)values);
#elif OIIO_SIMD_AVX >= 2
    m_simd = _mm256_maskload_epi32 (values, _mm256_castps_si256(mask));
#else
    SIMD_CONSTRUCT (mask[i] ? values[i] : 0);
#endif
}


OIIO_FORCEINLINE void vint8::store_mask (int mask, int *values) const {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm256_mask_storeu_epi32 (values, __mmask8(mask), m_simd);
#elif OIIO_SIMD_AVX >= 2
    _mm256_maskstore_epi32 (values, _mm256_castps_si256(vbool8::from_bitmask(mask)), m_simd);
#else
    SIMD_DO (if ((mask>>i) & 1) values[i] = (*this)[i]);
#endif
}


OIIO_FORCEINLINE void vint8::store_mask (const vbool8& mask, int *values) const {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm256_mask_storeu_epi32 (values, __mmask8(mask.bitmask()), m_simd);
#elif OIIO_SIMD_AVX >= 2
    _mm256_maskstore_epi32 (values, _mm256_castps_si256(mask), m_simd);
#else
    SIMD_DO (if (mask[i]) values[i] = (*this)[i]);
#endif
}


template <int scale>
OIIO_FORCEINLINE void
vint8::gather (const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm256_i32gather_epi32 (baseptr, vindex, scale);
#else
    SIMD_CONSTRUCT (*(const value_t *)((const char *)baseptr + vindex[i]*scale));
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vint8::gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm256_mask_i32gather_epi32 (m_simd, baseptr, vindex, _mm256_cvtps_epi32(mask), scale);
#else
    SIMD_CONSTRUCT (mask[i] ? *(const value_t *)((const char *)baseptr + vindex[i]*scale) : 0);
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vint8::scatter (value_t *baseptr, const vint_t& vindex) const
{
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm256_i32scatter_epi32 (baseptr, vindex, m_simd, scale);
#else
    SIMD_DO (*(value_t *)((char *)baseptr + vindex[i]*scale) = m_val[i]);
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vint8::scatter_mask (const bool_t& mask, value_t *baseptr,
                     const vint_t& vindex) const
{
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm256_mask_i32scatter_epi32 (baseptr, mask.bitmask(), vindex, m_simd, scale);
#else
    SIMD_DO (if (mask[i]) *(value_t *)((char *)baseptr + vindex[i]*scale) = m_val[i]);
#endif
}


OIIO_FORCEINLINE void vint8::clear () {
#if OIIO_SIMD_AVX
    m_simd = _mm256_setzero_si256();
#else
    *this = 0;
#endif
}


OIIO_FORCEINLINE const vint8 vint8::Zero () {
#if OIIO_SIMD_AVX
    return _mm256_setzero_si256();
#else
    return 0;
#endif
}

OIIO_FORCEINLINE const vint8 vint8::One () { return vint8(1); }

OIIO_FORCEINLINE const vint8 vint8::NegOne () { return vint8(-1); }


OIIO_FORCEINLINE const vint8 vint8::Iota (int start, int step) {
    return vint8 (start+0*step, start+1*step, start+2*step, start+3*step,
                 start+4*step, start+5*step, start+6*step, start+7*step);
}


OIIO_FORCEINLINE const vint8 vint8::Giota () {
    return vint8 (1<<0, 1<<1, 1<<2, 1<<3,  1<<4, 1<<5, 1<<6, 1<<7);
}


OIIO_FORCEINLINE vint4 vint8::lo () const {
#if OIIO_SIMD_AVX
    return _mm256_castsi256_si128 (simd());
#else
    return m_4[0];
#endif
}

OIIO_FORCEINLINE vint4 vint8::hi () const {
#if OIIO_SIMD_AVX
    return _mm256_extractf128_si256 (simd(), 1);
#else
    return m_4[1];
#endif
}


OIIO_FORCEINLINE vint8::vint8 (const vint4& lo, const vint4 &hi) {
#if OIIO_SIMD_AVX
    __m256i r = _mm256_castsi128_si256 (lo);
    m_simd = _mm256_insertf128_si256 (r, hi, 1);
    // N.B. equivalent, if available: m_simd = _mm256_set_m128i (hi, lo);
    // FIXME: when would this not be available?
#else
    m_4[0] = lo;
    m_4[1] = hi;
#endif
}


OIIO_FORCEINLINE vint8 operator+ (const vint8& a, const vint8& b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_add_epi32 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint8, a[i] + b[i]);
#endif
}


OIIO_FORCEINLINE const vint8& operator+= (vint8& a, const vint8& b) {
    return a = a + b;
}


OIIO_FORCEINLINE vint8 operator- (const vint8& a) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_sub_epi32 (_mm256_setzero_si256(), a);
#else
    SIMD_RETURN (vint8, -a[i]);
#endif
}


OIIO_FORCEINLINE vint8 operator- (const vint8& a, const vint8& b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_sub_epi32 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint8, a[i] - b[i]);
#endif
}


OIIO_FORCEINLINE const vint8 &operator-= (vint8& a, const vint8& b) {
    return a = a - b;
}


OIIO_FORCEINLINE vint8 operator* (const vint8& a, const vint8& b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_mullo_epi32 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint8, a[i] * b[i]);
#endif
}


OIIO_FORCEINLINE const vint8& operator*= (vint8& a, const vint8& b) { return a = a * b; }
OIIO_FORCEINLINE const vint8& operator*= (vint8& a, int b) { return a = a * b; }


OIIO_FORCEINLINE vint8 operator/ (const vint8& a, const vint8& b) {
    // NO INTEGER DIVISION IN SSE or AVX!
    SIMD_RETURN (vint8, a[i] / b[i]);
}

OIIO_FORCEINLINE const vint8& operator/= (vint8& a, const vint8& b) { return a = a / b; }


OIIO_FORCEINLINE vint8 operator% (const vint8& a, const vint8& b) {
    // NO INTEGER MODULUS IN SSE or AVX!
    SIMD_RETURN (vint8, a[i] % b[i]);
}

OIIO_FORCEINLINE const vint8& operator%= (vint8& a, const vint8& b) { return a = a % b; }

OIIO_FORCEINLINE vint8 operator% (const vint8& a, int w) {
    // NO INTEGER MODULUS in SSE or AVX!
    SIMD_RETURN (vint8, a[i] % w);
}

OIIO_FORCEINLINE const vint8& operator%= (vint8& a, int b) { return a = a % b; }


OIIO_FORCEINLINE vint8 operator& (const vint8& a, const vint8& b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_and_si256 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint8, a[i] & b[i]);
#endif
}

OIIO_FORCEINLINE const vint8& operator&= (vint8& a, const vint8& b) { return a = a & b; }

OIIO_FORCEINLINE vint8 operator| (const vint8& a, const vint8& b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_or_si256 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint8, a[i] | b[i]);
#endif
}

OIIO_FORCEINLINE const vint8& operator|= (vint8& a, const vint8& b) { return a = a | b; }

OIIO_FORCEINLINE vint8 operator^ (const vint8& a, const vint8& b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_xor_si256 (a.simd(), b.simd());
#else
    SIMD_RETURN (vint8, a[i] ^ b[i]);
#endif
}

OIIO_FORCEINLINE const vint8& operator^= (vint8& a, const vint8& b) { return a = a ^ b; }


OIIO_FORCEINLINE vint8 operator~ (const vint8& a) {
#if OIIO_SIMD_AVX >= 2
    return a ^ a.NegOne();
#else
    SIMD_RETURN (vint8, ~a[i]);
#endif
}


OIIO_FORCEINLINE vint8 operator<< (const vint8& a, unsigned int bits) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_slli_epi32 (a, bits);
#elif OIIO_SIMD_SSE
    return vint8 (a.lo() << bits, a.hi() << bits);
#else
    SIMD_RETURN (vint8, a[i] << bits);
#endif
}


OIIO_FORCEINLINE const vint8& operator<<= (vint8& a, const unsigned int bits) {
    return a = a << bits;
}

OIIO_FORCEINLINE vint8 operator>> (const vint8& a, const unsigned int bits) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_srai_epi32 (a, bits);
#elif OIIO_SIMD_SSE
    return vint8 (a.lo() >> bits, a.hi() >> bits);
#else
    SIMD_RETURN (vint8, a[i] >> bits);
#endif
}

OIIO_FORCEINLINE const vint8& operator>>= (vint8& a, const unsigned int bits) {
    return a = a >> bits;
}


OIIO_FORCEINLINE vint8 srl (const vint8& a, const unsigned int bits) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_srli_epi32 (a, bits);
#else
    SIMD_RETURN (vint8, int ((unsigned int)(a[i]) >> bits));
#endif
}


OIIO_FORCEINLINE vbool8 operator== (const vint8& a, const vint8& b) {
    // FIXME: on AVX-512 should we use _mm256_cmp_epi32_mask() ?
#if OIIO_SIMD_AVX >= 2
    return _mm256_castsi256_ps(_mm256_cmpeq_epi32 (a.m_simd, b.m_simd));
#elif OIIO_SIMD_SSE  /* Fall back to 4-wide */
    return vbool8 (a.lo() == b.lo(), a.hi() == b.hi());
#else
    SIMD_RETURN (vbool8, a[i] == b[i] ? -1 : 0);
#endif
}


OIIO_FORCEINLINE vbool8 operator!= (const vint8& a, const vint8& b) {
    // FIXME: on AVX-512 should we use _mm256_cmp_epi32_mask() ?
    return ! (a == b);
}


OIIO_FORCEINLINE vbool8 operator> (const vint8& a, const vint8& b) {
    // FIXME: on AVX-512 should we use _mm256_cmp_epi32_mask() ?
#if OIIO_SIMD_AVX >= 2
    return _mm256_castsi256_ps(_mm256_cmpgt_epi32 (a, b));
#elif OIIO_SIMD_SSE  /* Fall back to 4-wide */
    return vbool8 (a.lo() > b.lo(), a.hi() > b.hi());
#else
    SIMD_RETURN (vbool8, a[i] > b[i] ? -1 : 0);
#endif
}


OIIO_FORCEINLINE vbool8 operator< (const vint8& a, const vint8& b) {
    // FIXME: on AVX-512 should we use _mm256_cmp_epi32_mask() ?
#if OIIO_SIMD_AVX >= 2
    // No lt or lte!
    return (b > a);
#elif OIIO_SIMD_SSE  /* Fall back to 4-wide */
    return vbool8 (a.lo() < b.lo(), a.hi() < b.hi());
#else
    SIMD_RETURN (vbool8, a[i] < b[i] ? -1 : 0);
#endif
}


OIIO_FORCEINLINE vbool8 operator>= (const vint8& a, const vint8& b) {
    // FIXME: on AVX-512 should we use _mm256_cmp_epi32_mask() ?
    return (a > b) | (a == b);
}


OIIO_FORCEINLINE vbool8 operator<= (const vint8& a, const vint8& b) {
    // FIXME: on AVX-512 should we use _mm256_cmp_epi32_mask() ?
    return (b > a) | (a == b);
}


inline std::ostream& operator<< (std::ostream& cout, const vint8& val) {
    cout << val[0];
    for (int i = 1; i < val.elements; ++i)
        cout << ' ' << val[i];
    return cout;
}


OIIO_FORCEINLINE void vint8::store (int *values, int n) const {
    OIIO_DASSERT (n >= 0 && n <= elements);
#if 0 && OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // This SHOULD be fast, but in my benchmarks, it is slower!
    // (At least on the AVX512 hardware I have, Xeon Silver 4110.)
    // Re-test this periodically with new Intel hardware.
    _mm256_mask_storeu_epi32 (values, __mmask8(~(0xff << n)), m_simd);
#elif OIIO_SIMD_SSE
    if (n <= 4) {
        lo().store (values, n);
    } else if (n < 8) {
        lo().store (values);
        hi().store (values+4, n-4);
    } else {
        store (values);
    }
#else
    for (int i = 0; i < n; ++i)
        values[i] = m_val[i];
#endif
}


// FIXME(AVX): fast vint8 store to unsigned short, unsigned char

OIIO_FORCEINLINE void vint8::store (unsigned short *values) const {
#if OIIO_AVX512VL_ENABLED
    _mm256_mask_cvtepi32_storeu_epi16 (values, __mmask8(0xff), m_simd);
#elif OIIO_SIMD_SSE
    lo().store (values);
    hi().store (values+4);
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}


OIIO_FORCEINLINE void vint8::store (unsigned char *values) const {
#if OIIO_AVX512VL_ENABLED
    _mm256_mask_cvtepi32_storeu_epi8 (values, __mmask8(0xff), m_simd);
#elif OIIO_SIMD_SSE
    lo().store (values);
    hi().store (values+4);
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}


template<int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7>
OIIO_FORCEINLINE vint8 shuffle (const vint8& a) {
#if OIIO_SIMD_AVX >= 2
    vint8 index (i0, i1, i2, i3, i4, i5, i6, i7);
    return _mm256_castps_si256 (_mm256_permutevar8x32_ps (_mm256_castsi256_ps(a.simd()), index.simd()));
#else
    return vint8 (a[i0], a[i1], a[i2], a[i3], a[i4], a[i5], a[i6], a[i7]);
#endif
}

template<int i> OIIO_FORCEINLINE vint8 shuffle (const vint8& a) {
    return shuffle<i,i,i,i,i,i,i,i>(a);
}


template<int i>
OIIO_FORCEINLINE int extract (const vint8& v) {
#if OIIO_SIMD_AVX && !_WIN32
    return _mm256_extract_epi32(v.simd(), i);
#else
    return v[i];
#endif
}


template<int i>
OIIO_FORCEINLINE vint8 insert (const vint8& a, int val) {
#if OIIO_SIMD_AVX && !_WIN32
    return _mm256_insert_epi32 (a.simd(), val, i);
#else
    vint8 tmp = a;
    tmp[i] = val;
    return tmp;
#endif
}


OIIO_FORCEINLINE int vint8::x () const { return extract<0>(*this); }
OIIO_FORCEINLINE int vint8::y () const { return extract<1>(*this); }
OIIO_FORCEINLINE int vint8::z () const { return extract<2>(*this); }
OIIO_FORCEINLINE int vint8::w () const { return extract<3>(*this); }
OIIO_FORCEINLINE void vint8::set_x (int val) { *this = insert<0>(*this, val); }
OIIO_FORCEINLINE void vint8::set_y (int val) { *this = insert<1>(*this, val); }
OIIO_FORCEINLINE void vint8::set_z (int val) { *this = insert<2>(*this, val); }
OIIO_FORCEINLINE void vint8::set_w (int val) { *this = insert<3>(*this, val); }


OIIO_FORCEINLINE vint8 bitcast_to_int (const vbool8& x)
{
#if OIIO_SIMD_AVX
    return _mm256_castps_si256 (x.simd());
#else
    return *(vint8 *)&x;
#endif
}


OIIO_FORCEINLINE vint8 vreduce_add (const vint8& v) {
#if OIIO_SIMD_AVX >= 2
    // From Syrah:
    vint8 ab_cd_0_0_ef_gh_0_0 = _mm256_hadd_epi32(v.simd(), _mm256_setzero_si256());
    vint8 abcd_0_0_0_efgh_0_0_0 = _mm256_hadd_epi32(ab_cd_0_0_ef_gh_0_0, _mm256_setzero_si256());
    // get efgh in the 0-idx slot
    vint8 efgh = shuffle<4>(abcd_0_0_0_efgh_0_0_0);
    vint8 final_sum = abcd_0_0_0_efgh_0_0_0 + efgh;
    return shuffle<0>(final_sum);
#elif OIIO_SIMD_SSE
    vint4 hadd4 = vreduce_add(v.lo()) + vreduce_add(v.hi());
    return vint8(hadd4, hadd4);
#else
    SIMD_RETURN_REDUCE (vint8, 0, r += v[i]);
#endif
}


OIIO_FORCEINLINE int reduce_add (const vint8& v) {
#if OIIO_SIMD_SSE
    return extract<0> (vreduce_add(v));
#else
    SIMD_RETURN_REDUCE (int, 0, r += v[i]);
#endif
}


OIIO_FORCEINLINE int reduce_and (const vint8& v) {
#if OIIO_SSE_AVX >= 2
    vint8 ab = v & shuffle<1,1,3,3,5,5,7,7>(v); // ab bb cd dd ef ff gh hh
    vint8 abcd = ab & shuffle<2,2,2,2,6,6,6,6>(ab); // abcd x x x efgh x x x
    vint8 abcdefgh = abcd & shuffle<4>(abcdefgh); // abcdefgh x x x x x x x
    return extract<0> (abcdefgh);
#else
    // AVX 1.0 or less -- use SSE
    return reduce_and(v.lo() & v.hi());
#endif
}


OIIO_FORCEINLINE int reduce_or (const vint8& v) {
#if OIIO_SSE_AVX >= 2
    vint8 ab = v | shuffle<1,1,3,3,5,5,7,7>(v); // ab bb cd dd ef ff gh hh
    vint8 abcd = ab | shuffle<2,2,2,2,6,6,6,6>(ab); // abcd x x x efgh x x x
    vint8 abcdefgh = abcd | shuffle<4>(abcdefgh); // abcdefgh x x x x x x x
    return extract<0> (abcdefgh);
#else
    // AVX 1.0 or less -- use SSE
    return reduce_or(v.lo() | v.hi());
#endif
}


OIIO_FORCEINLINE vint8 blend (const vint8& a, const vint8& b, const vbool8& mask) {
#if OIIO_SIMD_AVX
    return _mm256_castps_si256 (_mm256_blendv_ps (_mm256_castsi256_ps(a.simd()),
                                                  _mm256_castsi256_ps(b.simd()), mask));
#elif OIIO_SIMD_SSE
    return vint8 (blend(a.lo(), b.lo(), mask.lo()),
                 blend(a.hi(), b.hi(), mask.hi()));
#else
    SIMD_RETURN (vint8, mask[i] ? b[i] : a[i]);
#endif
}


OIIO_FORCEINLINE vint8 blend0 (const vint8& a, const vbool8& mask) {
// FIXME: More efficient for AVX-512 to use
// _mm256_maxkz_mov_epi32(_mm256_movemask_ps(maxk),a))?
#if OIIO_SIMD_AVX
    return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(a.simd()), mask));
#elif OIIO_SIMD_SSE
    return vint8 (blend0(a.lo(), mask.lo()),
                 blend0(a.hi(), mask.hi()));
#else
    SIMD_RETURN (vint8, mask[i] ? a[i] : 0.0f);
#endif
}


OIIO_FORCEINLINE vint8 blend0not (const vint8& a, const vbool8& mask) {
// FIXME: More efficient for AVX-512 to use
// _mm256_maxkz_mov_epi32(_mm256_movemask_ps(!maxk),a))?
#if OIIO_SIMD_AVX
    return _mm256_castps_si256 (_mm256_andnot_ps (mask.simd(), _mm256_castsi256_ps(a.simd())));
#elif OIIO_SIMD_SSE
    return vint8 (blend0not(a.lo(), mask.lo()),
                 blend0not(a.hi(), mask.hi()));
#else
    SIMD_RETURN (vint8, mask[i] ? 0.0f : a[i]);
#endif
}

OIIO_FORCEINLINE vint8 select (const vbool8& mask, const vint8& a, const vint8& b) {
    return blend (b, a, mask);
}


OIIO_FORCEINLINE vint8 abs (const vint8& a) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_abs_epi32(a.simd());
#else
    SIMD_RETURN (vint8, std::abs(a[i]));
#endif
}


OIIO_FORCEINLINE vint8 min (const vint8& a, const vint8& b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_min_epi32 (a, b);
#else
    SIMD_RETURN (vint8, std::min(a[i], b[i]));
#endif
}


OIIO_FORCEINLINE vint8 max (const vint8& a, const vint8& b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_max_epi32 (a, b);
#else
    SIMD_RETURN (vint8, std::max(a[i], b[i]));
#endif
}


OIIO_FORCEINLINE vint8 rotl(const vint8& x, int s) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // return _mm256_rol_epi32 (x, s);
    // We want to do this ^^^ but this intrinsic only takes an *immediate*
    // argument for s, and there isn't a way to express in C++ that a
    // parameter must be an immediate/literal value from the caller.
    return (x<<s) | srl(x,32-s);
#else
    return (x<<s) | srl(x,32-s);
#endif
}

// DEPRECATED (2.1)
OIIO_FORCEINLINE vint8 rotl32 (const vint8& x, const unsigned int k) {
    return rotl(x, k);
}


OIIO_FORCEINLINE vint8 andnot (const vint8& a, const vint8& b) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_andnot_si256 (a.simd(), b.simd());
#elif OIIO_SIMD_AVX >= 1
    return _mm256_castps_si256 (_mm256_andnot_ps (_mm256_castsi256_ps(a.simd()), _mm256_castsi256_ps(b.simd())));
#else
    SIMD_RETURN (vint8, ~(a[i]) & b[i]);
#endif
}


// Implementation had to be after the definition of vint8::Zero.
OIIO_FORCEINLINE vbool8::vbool8 (const vint8& ival) {
    m_simd = (ival != vint8::Zero());
}



OIIO_FORCEINLINE vint8 safe_mod (const vint8& a, const vint8& b) {
    // NO INTEGER MODULUS IN SSE!
    SIMD_RETURN (vint8, b[i] ? a[i] % b[i] : 0);
}

OIIO_FORCEINLINE vint8 safe_mod (const vint8& a, int b) {
    return b ? (a % b) : vint8::Zero();
}




//////////////////////////////////////////////////////////////////////
// vint16 implementation

OIIO_FORCEINLINE const vint16 & vint16::operator= (const vint16& other) {
    m_simd = other.m_simd;
    return *this;
}

OIIO_FORCEINLINE int vint16::operator[] (int i) const {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}

OIIO_FORCEINLINE int& vint16::operator[] (int i) {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}

OIIO_FORCEINLINE void vint16::setcomp (int i, int val) {
    OIIO_DASSERT(i<elements);
    m_val[i] = val;
}


OIIO_FORCEINLINE void vint16::load (int a) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_set1_epi32 (a);
#else
    m_8[0].load (a);
    m_8[1].load (a);
#endif
}


OIIO_FORCEINLINE void vint16::load (int v0, int v1, int v2, int v3,
                                   int v4, int v5, int v6, int v7,
                                   int v8, int v9, int v10, int v11,
                                   int v12, int v13, int v14, int v15) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_setr_epi32 (v0, v1, v2, v3, v4, v5, v6, v7,
                                v8, v9, v10, v11, v12, v13, v14, v15);
#else
    m_val[ 0] = v0;
    m_val[ 1] = v1;
    m_val[ 2] = v2;
    m_val[ 3] = v3;
    m_val[ 4] = v4;
    m_val[ 5] = v5;
    m_val[ 6] = v6;
    m_val[ 7] = v7;
    m_val[ 8] = v8;
    m_val[ 9] = v9;
    m_val[10] = v10;
    m_val[11] = v11;
    m_val[12] = v12;
    m_val[13] = v13;
    m_val[14] = v14;
    m_val[15] = v15;
#endif
}


OIIO_FORCEINLINE void vint16::load (const int *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_loadu_si512 ((const simd_t *)values);
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}


OIIO_FORCEINLINE void vint16::load (const int *values, int n)
{
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_maskz_loadu_epi32 (__mmask16(~(0xffff << n)), values);
#else
    if (n > 8) {
        m_8[0].load (values);
        m_8[1].load (values+8, n-8);
    } else {
        m_8[0].load (values, n);
        m_8[1].clear ();
    }
#endif
}


OIIO_FORCEINLINE void vint16::load (const short *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_cvtepi16_epi32(_mm256_loadu_si256((__m256i*)values));
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}

OIIO_FORCEINLINE void vint16::load (const unsigned short *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_cvtepu16_epi32(_mm256_loadu_si256((__m256i*)values));
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}


OIIO_FORCEINLINE void vint16::load (const char *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_cvtepi8_epi32(_mm_loadu_si128((__m128i*)values));
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}

OIIO_FORCEINLINE void vint16::load (const unsigned char *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_cvtepu8_epi32(_mm_loadu_si128((__m128i*)values));
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}


OIIO_FORCEINLINE vint16::vint16 (int a) { load(a); }

OIIO_FORCEINLINE vint16::vint16 (int v0, int v1, int v2, int v3,
                               int v4, int v5, int v6, int v7,
                               int v8, int v9, int v10, int v11,
                               int v12, int v13, int v14, int v15) {
    load (v0, v1, v2, v3, v4, v5, v6, v7,
          v8, v9, v10, v11, v12, v13, v14, v15);
}

OIIO_FORCEINLINE vint16::vint16 (const int *vals) { load (vals); }
OIIO_FORCEINLINE vint16::vint16 (const unsigned short *vals) { load(vals); }
OIIO_FORCEINLINE vint16::vint16 (const short *vals) { load(vals); }
OIIO_FORCEINLINE vint16::vint16 (const unsigned char *vals) { load(vals); }
OIIO_FORCEINLINE vint16::vint16 (const char *vals) { load(vals); }

OIIO_FORCEINLINE const vint16 & vint16::operator= (int a) { load(a); return *this; }


OIIO_FORCEINLINE void vint16::load_mask (const vbool16 &mask, const int *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_maskz_loadu_epi32 (mask, (const simd_t *)values);
#else
    m_8[0].load_mask (mask.lo(), values);
    m_8[1].load_mask (mask.hi(), values+8);
#endif
}


OIIO_FORCEINLINE void vint16::store_mask (const vbool16 &mask, int *values) const {
#if OIIO_SIMD_AVX >= 512
    _mm512_mask_storeu_epi32 (values, mask.bitmask(), m_simd);
#else
    lo().store_mask (mask.lo(), values);
    hi().store_mask (mask.hi(), values+8);
#endif
}


template <int scale>
OIIO_FORCEINLINE void
vint16::gather (const value_t *baseptr, const vint_t& vindex) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_i32gather_epi32 (vindex, baseptr, scale);
#else
    m_8[0].gather<scale> (baseptr, vindex.lo());
    m_8[1].gather<scale> (baseptr, vindex.hi());
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vint16::gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_mask_i32gather_epi32 (m_simd, mask, vindex, baseptr, scale);
#else
    m_8[0].gather_mask<scale> (mask.lo(), baseptr, vindex.lo());
    m_8[1].gather_mask<scale> (mask.hi(), baseptr, vindex.hi());
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vint16::scatter (value_t *baseptr, const vint_t& vindex) const {
#if OIIO_SIMD_AVX >= 512
    _mm512_i32scatter_epi32 (baseptr, vindex, m_simd, scale);
#else
    lo().scatter<scale> (baseptr, vindex.lo());
    hi().scatter<scale> (baseptr, vindex.hi());
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vint16::scatter_mask (const bool_t& mask, value_t *baseptr,
                      const vint_t& vindex) const {
#if OIIO_SIMD_AVX >= 512
    _mm512_mask_i32scatter_epi32 (baseptr, mask, vindex, m_simd, scale);
#else
    lo().scatter_mask<scale> (mask.lo(), baseptr, vindex.lo());
    hi().scatter_mask<scale> (mask.hi(), baseptr, vindex.hi());
#endif
}


OIIO_FORCEINLINE void vint16::store (int *values) const {
#if OIIO_SIMD_AVX >= 512
    // Use an unaligned store -- it's just as fast when the memory turns
    // out to be aligned, nearly as fast even when unaligned. Not worth
    // the headache of using stores that require alignment.
    _mm512_storeu_si512 ((simd_t *)values, m_simd);
#else
    lo().store (values);
    hi().store (values+8);
#endif
}


OIIO_FORCEINLINE void vint16::clear () {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_setzero_si512();
#else
    *this = 0;
#endif
}


OIIO_FORCEINLINE const vint16 vint16::Zero () {
#if OIIO_SIMD_AVX >= 512
    return _mm512_setzero_epi32();
#else
    return 0;
#endif
}

OIIO_FORCEINLINE const vint16 vint16::One () { return vint16(1); }

OIIO_FORCEINLINE const vint16 vint16::NegOne () { return vint16(-1); }


OIIO_FORCEINLINE const vint16 vint16::Iota (int start, int step) {
    return vint16 (start+0*step, start+1*step, start+2*step, start+3*step,
                  start+4*step, start+5*step, start+6*step, start+7*step,
                  start+8*step, start+9*step, start+10*step, start+11*step,
                  start+12*step, start+13*step, start+14*step, start+15*step);
}


OIIO_FORCEINLINE const vint16 vint16::Giota () {
    return vint16 (1<<0, 1<<1, 1<<2, 1<<3,  1<<4, 1<<5, 1<<6, 1<<7,
                  1<<8, 1<<9, 1<<10, 1<<11,  1<<12, 1<<13, 1<<14, 1<<15);
}


OIIO_FORCEINLINE vint8 vint16::lo () const {
#if OIIO_SIMD_AVX >= 512
    return _mm512_castsi512_si256 (simd());
#else
    return m_8[0];
#endif
}

OIIO_FORCEINLINE vint8 vint16::hi () const {
#if OIIO_SIMD_AVX >= 512
    return _mm512_extracti64x4_epi64 (simd(), 1);
#else
    return m_8[1];
#endif
}


OIIO_FORCEINLINE vint16::vint16 (const vint8& lo, const vint8 &hi) {
#if OIIO_SIMD_AVX >= 512
    __m512i r = _mm512_castsi256_si512 (lo);
    m_simd = _mm512_inserti32x8 (r, hi, 1);
#else
    m_8[0] = lo;
    m_8[1] = hi;
#endif
}


OIIO_FORCEINLINE vint16::vint16 (const vint4 &a, const vint4 &b, const vint4 &c, const vint4 &d) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_broadcast_i32x4(a);
    m_simd = _mm512_inserti32x4 (m_simd, b, 1);
    m_simd = _mm512_inserti32x4 (m_simd, c, 2);
    m_simd = _mm512_inserti32x4 (m_simd, d, 3);
#else
    m_8[0] = vint8(a,b);
    m_8[1] = vint8(c,d);
#endif
}


OIIO_FORCEINLINE vint16 operator+ (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_add_epi32 (a.simd(), b.simd());
#else
    return vint16 (a.lo()+b.lo(), a.hi()+b.hi());
#endif
}


OIIO_FORCEINLINE const vint16& operator+= (vint16& a, const vint16& b) {
    return a = a + b;
}


OIIO_FORCEINLINE vint16 operator- (const vint16& a) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_sub_epi32 (_mm512_setzero_si512(), a);
#else
    return vint16 (-a.lo(), -a.hi());
#endif
}


OIIO_FORCEINLINE vint16 operator- (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_sub_epi32 (a.simd(), b.simd());
#else
    return vint16 (a.lo()-b.lo(), a.hi()-b.hi());
#endif
}


OIIO_FORCEINLINE const vint16 &operator-= (vint16& a, const vint16& b) {
    return a = a - b;
}


OIIO_FORCEINLINE vint16 operator* (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_mullo_epi32 (a.simd(), b.simd());
#else
    return vint16 (a.lo()*b.lo(), a.hi()*b.hi());
#endif
}


OIIO_FORCEINLINE const vint16& operator*= (vint16& a, const vint16& b) { return a = a * b; }
OIIO_FORCEINLINE const vint16& operator*= (vint16& a, int b) { return a = a * b; }


OIIO_FORCEINLINE vint16 operator/ (const vint16& a, const vint16& b) {
    // NO INTEGER DIVISION IN AVX512!
    SIMD_RETURN (vint16, a[i] / b[i]);
}

OIIO_FORCEINLINE const vint16& operator/= (vint16& a, const vint16& b) { return a = a / b; }


OIIO_FORCEINLINE vint16 operator% (const vint16& a, const vint16& b) {
    // NO INTEGER MODULUS IN AVX512!
    SIMD_RETURN (vint16, a[i] % b[i]);
}

OIIO_FORCEINLINE const vint16& operator%= (vint16& a, const vint16& b) { return a = a % b; }

OIIO_FORCEINLINE vint16 operator% (const vint16& a, int w) {
    // NO INTEGER MODULUS in AVX512!
    SIMD_RETURN (vint16, a[i] % w);
}

OIIO_FORCEINLINE const vint16& operator%= (vint16& a, int b) { return a = a % b; }


OIIO_FORCEINLINE vint16 operator& (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_and_si512 (a.simd(), b.simd());
#else
    return vint16 (a.lo() & b.lo(), a.hi() & b.hi());
#endif
}

OIIO_FORCEINLINE const vint16& operator&= (vint16& a, const vint16& b) { return a = a & b; }

OIIO_FORCEINLINE vint16 operator| (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_or_si512 (a.simd(), b.simd());
#else
    return vint16 (a.lo() | b.lo(), a.hi() | b.hi());
#endif
}

OIIO_FORCEINLINE const vint16& operator|= (vint16& a, const vint16& b) { return a = a | b; }

OIIO_FORCEINLINE vint16 operator^ (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_xor_si512 (a.simd(), b.simd());
#else
    return vint16 (a.lo() ^ b.lo(), a.hi() ^ b.hi());
#endif
}

OIIO_FORCEINLINE const vint16& operator^= (vint16& a, const vint16& b) { return a = a ^ b; }


OIIO_FORCEINLINE vint16 operator~ (const vint16& a) {
#if OIIO_SIMD_AVX >= 512
    return a ^ a.NegOne();
#else
    return vint16 (~a.lo(), ~a.hi());
#endif
}


OIIO_FORCEINLINE vint16 operator<< (const vint16& a, const unsigned int bits) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_sllv_epi32 (a, vint16(int(bits)));
    // return _mm512_slli_epi32 (a, bits);
    // FIXME: can this be slli?
#else
    return vint16 (a.lo() << bits, a.hi() << bits);
#endif
}


OIIO_FORCEINLINE const vint16& operator<<= (vint16& a, const unsigned int bits) {
    return a = a << bits;
}

OIIO_FORCEINLINE vint16 operator>> (const vint16& a, const unsigned int bits) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_srav_epi32 (a, vint16(int(bits)));
    // FIXME: can this be srai?
#else
    return vint16 (a.lo() >> bits, a.hi() >> bits);
#endif
}

OIIO_FORCEINLINE const vint16& operator>>= (vint16& a, const unsigned int bits) {
    return a = a >> bits;
}


OIIO_FORCEINLINE vint16 srl (const vint16& a, const unsigned int bits) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_srlv_epi32 (a, vint16(int(bits)));
    // FIXME: can this be srli?
#else
    return vint16 (srl(a.lo(), bits), srl (a.hi(), bits));
#endif
}


OIIO_FORCEINLINE vbool16 operator== (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_epi32_mask (a.simd(), b.simd(), 0 /*_MM_CMPINT_EQ*/);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() == b.lo(), a.hi() == b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator!= (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_epi32_mask (a.simd(), b.simd(), 4 /*_MM_CMPINT_NEQ*/);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() != b.lo(), a.hi() != b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator> (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_epi32_mask (a.simd(), b.simd(), 6 /*_MM_CMPINT_NLE*/);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() > b.lo(), a.hi() > b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator< (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_epi32_mask (a.simd(), b.simd(), 1 /*_MM_CMPINT_LT*/);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() < b.lo(), a.hi() < b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator>= (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_epi32_mask (a.simd(), b.simd(), 5 /*_MM_CMPINT_NLT*/);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() >= b.lo(), a.hi() >= b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator<= (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_epi32_mask (a.simd(), b.simd(), 2 /*_MM_CMPINT_LE*/);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() <= b.lo(), a.hi() <= b.hi());
#endif
}


inline std::ostream& operator<< (std::ostream& cout, const vint16& val) {
    cout << val[0];
    for (int i = 1; i < val.elements; ++i)
        cout << ' ' << val[i];
    return cout;
}



OIIO_FORCEINLINE void vint16::store (int *values, int n) const {
    OIIO_DASSERT (n >= 0 && n <= elements);
#if 0 && OIIO_SIMD_AVX >= 512
    // This SHOULD be fast, but in my benchmarks, it is slower!
    // (At least on the AVX512 hardware I have, Xeon Silver 4110.)
    // Re-test this periodically with new Intel hardware.
    _mm512_mask_storeu_epi32 (values, __mmask16(~(0xffff << n)), m_simd);
#else
    if (n > 8) {
        m_8[0].store (values);
        m_8[1].store (values+8, n-8);
    } else {
        m_8[0].store (values, n);
    }
#endif
}


OIIO_FORCEINLINE void vint16::store (unsigned short *values) const {
#if OIIO_SIMD_AVX512
    _mm512_mask_cvtepi32_storeu_epi16 (values, __mmask16(0xff), m_simd);
#elif OIIO_SIMD_AVX >= 2
    lo().store (values);
    hi().store (values+8);
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}


OIIO_FORCEINLINE void vint16::store (unsigned char *values) const {
#if OIIO_SIMD_AVX512
    _mm512_mask_cvtepi32_storeu_epi8 (values, __mmask16(0xff), m_simd);
#elif OIIO_SIMD_AVX >= 2
    lo().store (values);
    hi().store (values+8);
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}



// Shuffle groups of 4
template<int i0, int i1, int i2, int i3>
vint16 shuffle4 (const vint16& a) {
#if OIIO_SIMD_AVX >= 512
    __m512 x = _mm512_castsi512_ps(a);
    return _mm512_castps_si512(_mm512_shuffle_f32x4(x,x,_MM_SHUFFLE(i3,i2,i1,i0)));
#else
    vint4 x[4];
    a.store ((int *)x);
    return vint16 (x[i0], x[i1], x[i2], x[i3]);
#endif
}

template<int i> vint16 shuffle4 (const vint16& a) {
    return shuffle4<i,i,i,i> (a);
}

template<int i0, int i1, int i2, int i3>
vint16 shuffle (const vint16& a) {
#if OIIO_SIMD_AVX >= 512
    __m512 x = _mm512_castsi512_ps(a);
    return _mm512_castps_si512(_mm512_permute_ps(x,_MM_SHUFFLE(i3,i2,i1,i0)));
#else
    vint4 x[4];
    a.store ((int *)x);
    return vint16 (shuffle<i0,i1,i2,i3>(x[0]), shuffle<i0,i1,i2,i3>(x[1]),
                  shuffle<i0,i1,i2,i3>(x[2]), shuffle<i0,i1,i2,i3>(x[3]));
#endif
}

template<int i> vint16 shuffle (const vint16& a) {
    return shuffle<i,i,i,i> (a);
}


template<int i>
OIIO_FORCEINLINE int extract (const vint16& a) {
    return a[i];
}


template<int i>
OIIO_FORCEINLINE vint16 insert (const vint16& a, int val) {
    vint16 tmp = a;
    tmp[i] = val;
    return tmp;
}


OIIO_FORCEINLINE int vint16::x () const {
#if OIIO_SIMD_AVX >= 512
    return _mm_cvtsi128_si32(_mm512_castsi512_si128(m_simd));
#else
    return m_val[0];
#endif
}

OIIO_FORCEINLINE int vint16::y () const { return m_val[1]; }
OIIO_FORCEINLINE int vint16::z () const { return m_val[2]; }
OIIO_FORCEINLINE int vint16::w () const { return m_val[3]; }
OIIO_FORCEINLINE void vint16::set_x (int val) { m_val[0] = val; }
OIIO_FORCEINLINE void vint16::set_y (int val) { m_val[1] = val; }
OIIO_FORCEINLINE void vint16::set_z (int val) { m_val[2] = val; }
OIIO_FORCEINLINE void vint16::set_w (int val) { m_val[3] = val; }


OIIO_FORCEINLINE vint16 bitcast_to_int (const vbool16& x)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_maskz_set1_epi32 (x, -1);
#else
    return vint16 (bitcast_to_int(x.lo()), bitcast_to_int(x.hi()));
#endif
}


OIIO_FORCEINLINE vint16 vreduce_add (const vint16& v) {
#if OIIO_SIMD_AVX >= 512
    // Nomenclature: ABCD are the vint4's comprising v
    // First, add the vint4's and make them all the same
    vint16 AB_AB_CD_CD = v + shuffle4<1,0,3,2>(v);  // each adjacent vint4 is summed
    vint16 w = AB_AB_CD_CD + shuffle4<2,3,0,1>(AB_AB_CD_CD);  // ABCD in all quads
    // Now, add within each vint4
    vint16 ab_ab_cd_cd = w + shuffle<1,0,3,2>(w);  // each adjacent int is summed
    return ab_ab_cd_cd + shuffle<2,3,0,1>(ab_ab_cd_cd);
#else
    vint8 sum = vreduce_add(v.lo()) + vreduce_add(v.hi());
    return vint16 (sum, sum);
#endif
}


OIIO_FORCEINLINE int reduce_add (const vint16& v) {
#if OIIO_SIMD_AVX >= 512
    return vreduce_add(v).x();
#else
    return reduce_add(v.lo()) + reduce_add(v.hi());
#endif
}


OIIO_FORCEINLINE int reduce_and (const vint16& v) {
#if OIIO_SIMD_AVX >= 512
    // Nomenclature: ABCD are the vint4's comprising v
    // First, and the vint4's and make them all the same
    vint16 AB_AB_CD_CD = v & shuffle4<1,0,3,2>(v);  // each adjacent vint4 is summed
    vint16 w = AB_AB_CD_CD & shuffle4<2,3,0,1>(AB_AB_CD_CD);
    // Now, and within each vint4
    vint16 ab_ab_cd_cd = w & shuffle<1,0,3,2>(w);  // each adjacent int is summed
    vint16 r = ab_ab_cd_cd & shuffle<2,3,0,1>(ab_ab_cd_cd);
    return r.x();
#else
    return reduce_and(v.lo()) & reduce_and(v.hi());
#endif
}


OIIO_FORCEINLINE int reduce_or (const vint16& v) {
#if OIIO_SIMD_AVX >= 512
    // Nomenclature: ABCD are the vint4's comprising v
    // First, or the vint4's or make them all the same
    vint16 AB_AB_CD_CD = v | shuffle4<1,0,3,2>(v);  // each adjacent vint4 is summed
    vint16 w = AB_AB_CD_CD | shuffle4<2,3,0,1>(AB_AB_CD_CD);
    // Now, or within each vint4
    vint16 ab_ab_cd_cd = w | shuffle<1,0,3,2>(w);  // each adjacent int is summed
    vint16 r = ab_ab_cd_cd | shuffle<2,3,0,1>(ab_ab_cd_cd);
    return r.x();
#else
    return reduce_or(v.lo()) | reduce_or(v.hi());
#endif
}



OIIO_FORCEINLINE vint16 blend (const vint16& a, const vint16& b, const vbool16& mask) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_mask_blend_epi32 (mask, a, b);
#else
    return vint16 (blend (a.lo(), b.lo(), mask.lo()),
                  blend (a.hi(), b.hi(), mask.hi()));
#endif
}


OIIO_FORCEINLINE vint16 blend0 (const vint16& a, const vbool16& mask) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_maskz_mov_epi32 (mask, a);
#else
    return vint16 (blend0 (a.lo(), mask.lo()),
                  blend0 (a.hi(), mask.hi()));
#endif
}


OIIO_FORCEINLINE vint16 blend0not (const vint16& a, const vbool16& mask) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_maskz_mov_epi32 (!mask, a);
#else
    return vint16 (blend0not (a.lo(), mask.lo()),
                  blend0not (a.hi(), mask.hi()));
#endif
}

OIIO_FORCEINLINE vint16 select (const vbool16& mask, const vint16& a, const vint16& b) {
    return blend (b, a, mask);
}


OIIO_FORCEINLINE vint16 abs (const vint16& a) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_abs_epi32(a.simd());
#else
    return vint16 (abs(a.lo()), abs(a.hi()));
#endif
}


OIIO_FORCEINLINE vint16 min (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_min_epi32 (a, b);
#else
    return vint16 (min(a.lo(), b.lo()), min(a.hi(), b.hi()));
#endif
}


OIIO_FORCEINLINE vint16 max (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_max_epi32 (a, b);
#else
    return vint16 (max(a.lo(), b.lo()), max(a.hi(), b.hi()));
#endif
}


OIIO_FORCEINLINE vint16 rotl(const vint16& x, int s) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // return _mm512_rol_epi32 (x, s);
    // We want to do this ^^^ but this intrinsic only takes an *immediate*
    // argument for s, and there isn't a way to express in C++ that a
    // parameter must be an immediate/literal value from the caller.
    return (x<<s) | srl(x,32-s);
#else
    return (x<<s) | srl(x,32-s);
#endif
}

// DEPRECATED (2.1)
OIIO_FORCEINLINE vint16 rotl32 (const vint16& x, const unsigned int k) {
    return rotl(x, k);
}


OIIO_FORCEINLINE vint16 andnot (const vint16& a, const vint16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_andnot_epi32 (a.simd(), b.simd());
#else
    return vint16 (andnot(a.lo(), b.lo()), andnot(a.hi(), b.hi()));
#endif
}



OIIO_FORCEINLINE vint16 safe_mod (const vint16& a, const vint16& b) {
    // NO INTEGER MODULUS IN SSE!
    SIMD_RETURN (vint16, b[i] ? a[i] % b[i] : 0);
}

OIIO_FORCEINLINE vint16 safe_mod (const vint16& a, int b) {
    return b ? (a % b) : vint16::Zero();
}





//////////////////////////////////////////////////////////////////////
// vfloat4 implementation


OIIO_FORCEINLINE vfloat4::vfloat4 (const vint4& ival) {
#if OIIO_SIMD_SSE
    m_simd = _mm_cvtepi32_ps (ival.simd());
#else
    SIMD_CONSTRUCT (float(ival[i]));
#endif
}


OIIO_FORCEINLINE const vfloat4 vfloat4::Zero () {
#if OIIO_SIMD_SSE
    return _mm_setzero_ps();
#else
    return vfloat4(0.0f);
#endif
}

OIIO_FORCEINLINE const vfloat4 vfloat4::One () {
    return vfloat4(1.0f);
}

OIIO_FORCEINLINE const vfloat4 vfloat4::Iota (float start, float step) {
    return vfloat4 (start+0.0f*step, start+1.0f*step, start+2.0f*step, start+3.0f*step);
}

/// Set all components to 0.0
OIIO_FORCEINLINE void vfloat4::clear () {
#if OIIO_SIMD_SSE
    m_simd = _mm_setzero_ps();
#else
    load (0.0f);
#endif
}

OIIO_FORCEINLINE const vfloat4 & vfloat4::operator= (const Imath::V4f &v) {
    load ((const float *)&v);
    return *this;
}

OIIO_FORCEINLINE const vfloat4 & vfloat4::operator= (const Imath::V3f &v) {
    load (v[0], v[1], v[2], 0.0f);
    return *this;
}

OIIO_FORCEINLINE float& vfloat4::operator[] (int i) {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}

OIIO_FORCEINLINE float vfloat4::operator[] (int i) const {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}


OIIO_FORCEINLINE void vfloat4::load (float val) {
#if OIIO_SIMD_SSE
    m_simd = _mm_set1_ps (val);
#elif OIIO_SIMD_NEON
    m_simd = vdupq_n_f32 (val);
#else
    SIMD_CONSTRUCT (val);
#endif
}

OIIO_FORCEINLINE void vfloat4::load (float a, float b, float c, float d) {
#if OIIO_SIMD_SSE
    m_simd = _mm_set_ps (d, c, b, a);
#elif OIIO_SIMD_NEON
    float values[4] = { a, b, c, d };
    m_simd = vld1q_f32 (values);
#else
    m_val[0] = a;
    m_val[1] = b;
    m_val[2] = c;
    m_val[3] = d;
#endif
}

    /// Load from an array of 4 values
OIIO_FORCEINLINE void vfloat4::load (const float *values) {
#if OIIO_SIMD_SSE
    m_simd = _mm_loadu_ps (values);
#elif OIIO_SIMD_NEON
    m_simd = vld1q_f32 (values);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vfloat4::load (const float *values, int n) {
    OIIO_DASSERT (n >= 0 && n <= elements);
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm_maskz_loadu_ps (__mmask8(~(0xf << n)), values);
#elif OIIO_SIMD_SSE
    switch (n) {
    case 1:
        m_simd = _mm_load_ss (values);
        break;
    case 2:
        // Trickery: load one double worth of bits!
        m_simd = _mm_castpd_ps (_mm_load_sd ((const double*)values));
        break;
    case 3:
        m_simd = _mm_setr_ps (values[0], values[1], values[2], 0.0f);
        // This looks wasteful, but benchmarks show that it's the
        // fastest way to set 3 values with the 4th getting zero.
        // Actually, gcc and clang both turn it into something more
        // efficient than _mm_setr_ps. The version below looks smart,
        // but was much more expensive as the _mm_setr_ps!
        //   __m128 xy = _mm_castsi128_ps(_mm_loadl_epi64((const __m128i*)values));
        //   m_simd = _mm_movelh_ps(xy, _mm_load_ss (values + 2));
        break;
    case 4:
        m_simd = _mm_loadu_ps (values);
        break;
    default:
        clear();
        break;
    }
#elif OIIO_SIMD_NEON
    switch (n) {
    case 1: m_simd = vdupq_n_f32 (val);                    break;
    case 2: load (values[0], values[1], 0.0f, 0.0f);      break;
    case 3: load (values[0], values[1], values[2], 0.0f); break;
    case 4: m_simd = vld1q_f32 (values);                   break;
    default: break;
    }
#else
    for (int i = 0; i < n; ++i)
        m_val[i] = values[i];
    for (int i = n; i < paddedelements; ++i)
        m_val[i] = 0;
#endif
}


OIIO_FORCEINLINE void vfloat4::load (const unsigned short *values) {
#if OIIO_SIMD_SSE >= 2
    m_simd = _mm_cvtepi32_ps (vint4(values).simd());
    // You might guess that the following is faster, but it's NOT:
    //   NO!  m_simd = _mm_cvtpu16_ps (*(__m64*)values);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vfloat4::load (const short *values) {
#if OIIO_SIMD_SSE >= 2
    m_simd = _mm_cvtepi32_ps (vint4(values).simd());
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vfloat4::load (const unsigned char *values) {
#if OIIO_SIMD_SSE >= 2
    m_simd = _mm_cvtepi32_ps (vint4(values).simd());
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}

    /// Load from an array of 4 char values, convert to float
OIIO_FORCEINLINE void vfloat4::load (const char *values) {
#if OIIO_SIMD_SSE >= 2
    m_simd = _mm_cvtepi32_ps (vint4(values).simd());
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}

#ifdef _HALF_H_
OIIO_FORCEINLINE void vfloat4::load (const half *values) {
#if OIIO_F16C_ENABLED && OIIO_SIMD_SSE
    /* Enabled 16 bit float instructions! */
    __m128i a = _mm_castpd_si128 (_mm_load_sd ((const double *)values));
    m_simd = _mm_cvtph_ps (a);
#elif OIIO_SIMD_SSE >= 2
    // SSE half-to-float by Fabian "ryg" Giesen. Public domain.
    // https://gist.github.com/rygorous/2144712
    vint4 h ((const unsigned short *)values);
# define CONSTI(name) *(const __m128i *)&name
# define CONSTF(name) *(const __m128 *)&name
    OIIO_SIMD_UINT4_CONST(mask_nosign, 0x7fff);
    OIIO_SIMD_UINT4_CONST(magic,       (254 - 15) << 23);
    OIIO_SIMD_UINT4_CONST(was_infnan,  0x7bff);
    OIIO_SIMD_UINT4_CONST(exp_infnan,  255 << 23);
    __m128i mnosign     = CONSTI(mask_nosign);
    __m128i expmant     = _mm_and_si128(mnosign, h);
    __m128i justsign    = _mm_xor_si128(h, expmant);
    __m128i expmant2    = expmant; // copy (just here for counting purposes)
    __m128i shifted     = _mm_slli_epi32(expmant, 13);
    __m128  scaled      = _mm_mul_ps(_mm_castsi128_ps(shifted), *(const __m128 *)&magic);
    __m128i b_wasinfnan = _mm_cmpgt_epi32(expmant2, CONSTI(was_infnan));
    __m128i sign        = _mm_slli_epi32(justsign, 16);
    __m128  infnanexp   = _mm_and_ps(_mm_castsi128_ps(b_wasinfnan), CONSTF(exp_infnan));
    __m128  sign_inf    = _mm_or_ps(_mm_castsi128_ps(sign), infnanexp);
    __m128  final       = _mm_or_ps(scaled, sign_inf);
    // ~11 SSE2 ops.
    m_simd = final;
# undef CONSTI
# undef CONSTF
#else /* No SIMD defined: */
    SIMD_CONSTRUCT (values[i]);
#endif
}
#endif /* _HALF_H_ */

OIIO_FORCEINLINE void vfloat4::store (float *values) const {
#if OIIO_SIMD_SSE
    // Use an unaligned store -- it's just as fast when the memory turns
    // out to be aligned, nearly as fast even when unaligned. Not worth
    // the headache of using stores that require alignment.
    _mm_storeu_ps (values, m_simd);
#elif OIIO_SIMD_NEON
    vst1q_f32 (values, m_simd);
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}

OIIO_FORCEINLINE void vfloat4::store (float *values, int n) const {
    OIIO_DASSERT (n >= 0 && n <= 4);
#if 0 && OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // This SHOULD be fast, but in my benchmarks, it is slower!
    // (At least on the AVX512 hardware I have, Xeon Silver 4110.)
    // Re-test this periodically with new Intel hardware.
    _mm_mask_storeu_ps (values, __mmask8(~(0xf << n)), m_simd);
#elif OIIO_SIMD_SSE
    switch (n) {
        case 1:
        _mm_store_ss (values, m_simd);
        break;
    case 2:
        // Trickery: store two floats as a double worth of bits
        _mm_store_sd ((double*)values, _mm_castps_pd(m_simd));
        break;
    case 3:
        values[0] = m_val[0];
        values[1] = m_val[1];
        values[2] = m_val[2];
        // This looks wasteful, but benchmarks show that it's the
        // fastest way to store 3 values, in benchmarks was faster than
        // this, below:
        //   _mm_store_sd ((double*)values, _mm_castps_pd(m_simd));
        //   _mm_store_ss (values + 2, _mm_movehl_ps(m_simd,m_simd));
        break;
    case 4:
        store (values);
        break;
    default:
        break;
    }
#elif OIIO_SIMD_NEON
    switch (n) {
    case 1:
        vst1q_lane_f32 (values, m_simd, 0);
        break;
    case 2:
        vst1q_lane_f32 (values++, m_simd, 0);
        vst1q_lane_f32 (values, m_simd, 1);
        break;
    case 3:
        vst1q_lane_f32 (values++, m_simd, 0);
        vst1q_lane_f32 (values++, m_simd, 1);
        vst1q_lane_f32 (values, m_simd, 2);
        break;
    case 4:
        vst1q_f32 (values, m_simd); break;
    default:
        break;
    }
#else
    for (int i = 0; i < n; ++i)
        values[i] = m_val[i];
#endif
}

#ifdef _HALF_H_
OIIO_FORCEINLINE void vfloat4::store (half *values) const {
#if OIIO_F16C_ENABLED && OIIO_SIMD_SSE
    __m128i h = _mm_cvtps_ph (m_simd, (_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC));
    _mm_store_sd ((double *)values, _mm_castsi128_pd(h));
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}
#endif


OIIO_FORCEINLINE void vfloat4::load_mask (int mask, const float *values) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm_maskz_loadu_ps (__mmask8(mask), (const simd_t *)values);
#elif OIIO_SIMD_AVX
    m_simd = _mm_maskload_ps (values, _mm_castps_si128(vbool_t::from_bitmask(mask)));
#else
    SIMD_CONSTRUCT ((mask>>i) & 1 ? values[i] : 0.0f);
#endif
}


OIIO_FORCEINLINE void vfloat4::load_mask (const vbool_t& mask, const float *values) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm_maskz_loadu_ps (__mmask8(mask.bitmask()), (const simd_t *)values);
#elif OIIO_SIMD_AVX
    m_simd = _mm_maskload_ps (values, _mm_castps_si128(mask));
#else
    SIMD_CONSTRUCT (mask[i] ? values[i] : 0.0f);
#endif
}


OIIO_FORCEINLINE void vfloat4::store_mask (int mask, float *values) const {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm_mask_storeu_ps (values, __mmask8(mask), m_simd);
#elif OIIO_SIMD_AVX
    _mm_maskstore_ps (values, _mm_castps_si128(vbool_t::from_bitmask(mask)), m_simd);
#else
    SIMD_DO (if ((mask>>i) & 1) values[i] = (*this)[i]);
#endif
}


OIIO_FORCEINLINE void vfloat4::store_mask (const vbool_t& mask, float *values) const {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm_mask_storeu_ps (values, __mmask8(mask.bitmask()), m_simd);
#elif OIIO_SIMD_AVX
    _mm_maskstore_ps (values, _mm_castps_si128(mask.simd()), m_simd);
#else
    SIMD_DO (if (mask[i]) values[i] = (*this)[i]);
#endif
}


template <int scale>
OIIO_FORCEINLINE void
vfloat4::gather (const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm_i32gather_ps (baseptr, vindex, scale);
#else
    SIMD_CONSTRUCT (*(const value_t *)((const char *)baseptr + vindex[i]*scale));
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vfloat4::gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm_mask_i32gather_ps (m_simd, baseptr, vindex, mask, scale);
#else
    SIMD_CONSTRUCT (mask[i] ? *(const value_t *)((const char *)baseptr + vindex[i]*scale) : 0);
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vfloat4::scatter (value_t *baseptr, const vint_t& vindex) const
{
#if 0 && OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // FIXME: disable because it benchmarks slower than the dumb way
    _mm_i32scatter_ps (baseptr, vindex, m_simd, scale);
#else
    SIMD_DO (*(value_t *)((char *)baseptr + vindex[i]*scale) = m_val[i]);
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vfloat4::scatter_mask (const bool_t& mask, value_t *baseptr,
                       const vint_t& vindex) const
{
#if 0 && OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // FIXME: disable because it benchmarks slower than the dumb way
    _mm_mask_i32scatter_ps (baseptr, mask.bitmask(), vindex, m_simd, scale);
#else
    SIMD_DO (if (mask[i]) *(value_t *)((char *)baseptr + vindex[i]*scale) = m_val[i]);
#endif
}


OIIO_FORCEINLINE vfloat4 operator+ (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_add_ps (a.m_simd, b.m_simd);
#elif OIIO_SIMD_NEON
    return vaddq_f32 (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vfloat4, a[i] + b[i]);
#endif
}

OIIO_FORCEINLINE const vfloat4 & vfloat4::operator+= (const vfloat4& a) {
#if OIIO_SIMD_SSE
    m_simd = _mm_add_ps (m_simd, a.m_simd);
#elif OIIO_SIMD_NEON
    m_simd = vaddq_f32 (m_simd, a.m_simd);
#else
    SIMD_DO (m_val[i] += a[i]);
#endif
    return *this;
    }

OIIO_FORCEINLINE vfloat4 vfloat4::operator- () const {
#if OIIO_SIMD_SSE
    return _mm_sub_ps (_mm_setzero_ps(), m_simd);
#elif OIIO_SIMD_NEON
    return vsubq_f32 (Zero(), m_simd);
#else
    SIMD_RETURN (vfloat4, -m_val[i]);
#endif
}

OIIO_FORCEINLINE vfloat4 operator- (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_sub_ps (a.m_simd, b.m_simd);
#elif OIIO_SIMD_NEON
    return vsubq_f32 (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vfloat4, a[i] - b[i]);
#endif
}

OIIO_FORCEINLINE const vfloat4 & vfloat4::operator-= (const vfloat4& a) {
#if OIIO_SIMD_SSE
    m_simd = _mm_sub_ps (m_simd, a.m_simd);
#elif OIIO_SIMD_NEON
    m_simd = vsubq_f32 (m_simd, a.m_simd);
#else
    SIMD_DO (m_val[i] -= a[i]);
#endif
    return *this;
}

OIIO_FORCEINLINE vfloat4 operator* (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_mul_ps (a.m_simd, b.m_simd);
#elif OIIO_SIMD_NEON
    return vmulq_f32 (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vfloat4, a[i] * b[i]);
#endif
}

OIIO_FORCEINLINE const vfloat4 & vfloat4::operator*= (const vfloat4& a) {
#if OIIO_SIMD_SSE
    m_simd = _mm_mul_ps (m_simd, a.m_simd);
#elif OIIO_SIMD_NEON
    m_simd = vmulq_f32 (m_simd, a.m_simd);
#else
    SIMD_DO (m_val[i] *= a[i]);
#endif
    return *this;
}

OIIO_FORCEINLINE const vfloat4 & vfloat4::operator*= (float val) {
#if OIIO_SIMD_SSE
    m_simd = _mm_mul_ps (m_simd, _mm_set1_ps(val));
#elif OIIO_SIMD_NEON
    m_simd = vmulq_f32 (m_simd, vdupq_n_f32(val));
#else
    SIMD_DO (m_val[i] *= val);
#endif
    return *this;
}

OIIO_FORCEINLINE vfloat4 operator/ (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_div_ps (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vfloat4, a[i] / b[i]);
#endif
}

OIIO_FORCEINLINE const vfloat4 & vfloat4::operator/= (const vfloat4& a) {
#if OIIO_SIMD_SSE
    m_simd = _mm_div_ps (m_simd, a.m_simd);
#else
    SIMD_DO (m_val[i] /= a[i]);
#endif
    return *this;
}

OIIO_FORCEINLINE const vfloat4 & vfloat4::operator/= (float val) {
#if OIIO_SIMD_SSE
    m_simd = _mm_div_ps (m_simd, _mm_set1_ps(val));
#else
    SIMD_DO (m_val[i] /= val);
#endif
    return *this;
}

OIIO_FORCEINLINE vbool4 operator== (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_cmpeq_ps (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vbool4, a[i] == b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool4 operator!= (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_cmpneq_ps (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vbool4, a[i] != b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool4 operator< (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_cmplt_ps (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vbool4, a[i] < b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool4 operator>  (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_cmpgt_ps (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vbool4, a[i] > b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool4 operator>= (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_cmpge_ps (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vbool4, a[i] >= b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vbool4 operator<= (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_cmple_ps (a.m_simd, b.m_simd);
#else
    SIMD_RETURN (vbool4, a[i] <= b[i] ? -1 : 0);
#endif
}

OIIO_FORCEINLINE vfloat4 AxyBxy (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_movelh_ps (a.m_simd, b.m_simd);
#else
    return vfloat4 (a[0], a[1], b[0], b[1]);
#endif
}

OIIO_FORCEINLINE vfloat4 AxBxAyBy (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_unpacklo_ps (a.m_simd, b.m_simd);
#else
    return vfloat4 (a[0], b[0], a[1], b[1]);
#endif
}

OIIO_FORCEINLINE vfloat4 vfloat4::xyz0 () const {
    return insert<3>(*this, 0.0f);
}

OIIO_FORCEINLINE vfloat4 vfloat4::xyz1 () const {
    return insert<3>(*this, 1.0f);
}

inline std::ostream& operator<< (std::ostream& cout, const vfloat4& val) {
    cout << val[0];
    for (int i = 1; i < val.elements; ++i)
        cout << ' ' << val[i];
    return cout;
}


// Implementation had to be after the definition of vfloat4.
OIIO_FORCEINLINE vint4::vint4 (const vfloat4& f)
{
#if OIIO_SIMD_SSE
    m_simd = _mm_cvttps_epi32(f.simd());
#else
    SIMD_CONSTRUCT ((int) f[i]);
#endif
}


template<int i0, int i1, int i2, int i3>
OIIO_FORCEINLINE vfloat4 shuffle (const vfloat4& a) {
#if OIIO_SIMD_SSE
    return shuffle_sse<i0,i1,i2,i3> (__m128(a));
#else
    return vfloat4(a[i0], a[i1], a[i2], a[i3]);
#endif
}

template<int i> OIIO_FORCEINLINE vfloat4 shuffle (const vfloat4& a) { return shuffle<i,i,i,i>(a); }

#if OIIO_SIMD_NEON
template<> OIIO_FORCEINLINE vfloat4 shuffle<0> (const vfloat4& a) {
    vfloat32x2_t t = vget_low_f32(a.m_simd); return vdupq_lane_f32(t,0);
}
template<> OIIO_FORCEINLINE vfloat4 shuffle<1> (const vfloat4& a) {
    vfloat32x2_t t = vget_low_f32(a.m_simd); return vdupq_lane_f32(t,1);
}
template<> OIIO_FORCEINLINE vfloat4 shuffle<2> (const vfloat4& a) {
    vfloat32x2_t t = vget_high_f32(a.m_simd); return vdupq_lane_f32(t,0);
}
template<> OIIO_FORCEINLINE vfloat4 shuffle<3> (const vfloat4& a) {
    vfloat32x2_t t = vget_high_f32(a.m_simd); return vdupq_lane_f32(t,1);
}
#endif



/// Helper: as rapid as possible extraction of one component, when the
/// index is fixed.
template<int i>
OIIO_FORCEINLINE float extract (const vfloat4& a) {
#if OIIO_SIMD_SSE
    return _mm_cvtss_f32(shuffle_sse<i,i,i,i>(a.simd()));
#else
    return a[i];
#endif
}

#if OIIO_SIMD_SSE
template<> OIIO_FORCEINLINE float extract<0> (const vfloat4& a) {
    return _mm_cvtss_f32(a.simd());
}
#endif


/// Helper: substitute val for a[i]
template<int i>
OIIO_FORCEINLINE vfloat4 insert (const vfloat4& a, float val) {
#if OIIO_SIMD_SSE >= 4
    return _mm_insert_ps (a, _mm_set_ss(val), i<<4);
#else
    vfloat4 tmp = a;
    tmp[i] = val;
    return tmp;
#endif
}

#if OIIO_SIMD_SSE
// Slightly faster special cases for SSE
template<> OIIO_FORCEINLINE vfloat4 insert<0> (const vfloat4& a, float val) {
    return _mm_move_ss (a.simd(), _mm_set_ss(val));
}
#endif


OIIO_FORCEINLINE float vfloat4::x () const { return extract<0>(*this); }
OIIO_FORCEINLINE float vfloat4::y () const { return extract<1>(*this); }
OIIO_FORCEINLINE float vfloat4::z () const { return extract<2>(*this); }
OIIO_FORCEINLINE float vfloat4::w () const { return extract<3>(*this); }
OIIO_FORCEINLINE void vfloat4::set_x (float val) { *this = insert<0>(*this, val); }
OIIO_FORCEINLINE void vfloat4::set_y (float val) { *this = insert<1>(*this, val); }
OIIO_FORCEINLINE void vfloat4::set_z (float val) { *this = insert<2>(*this, val); }
OIIO_FORCEINLINE void vfloat4::set_w (float val) { *this = insert<3>(*this, val); }


OIIO_FORCEINLINE vint4 bitcast_to_int (const vfloat4& x)
{
#if OIIO_SIMD_SSE
    return _mm_castps_si128 (x.simd());
#else
    return *(vint4 *)&x;
#endif
}

OIIO_FORCEINLINE vfloat4 bitcast_to_float (const vint4& x)
{
#if OIIO_SIMD_SSE
    return _mm_castsi128_ps (x.simd());
#else
    return *(vfloat4 *)&x;
#endif
}


// Old names:
inline vint4 bitcast_to_int4 (const vfloat4& x) { return bitcast_to_int(x); }
inline vfloat4 bitcast_to_float4 (const vint4& x) { return bitcast_to_float(x); }



OIIO_FORCEINLINE vfloat4 vreduce_add (const vfloat4& v) {
#if OIIO_SIMD_SSE >= 3
    // People seem to agree that SSE3 does add reduction best with 2
    // horizontal adds.
    // suppose v = (a, b, c, d)
    simd::vfloat4 ab_cd = _mm_hadd_ps (v.simd(), v.simd());
    // ab_cd = (a+b, c+d, a+b, c+d)
    simd::vfloat4 abcd = _mm_hadd_ps (ab_cd.simd(), ab_cd.simd());
    // all abcd elements are a+b+c+d
    return abcd;
#elif OIIO_SIMD_SSE
    // I think this is the best we can do for SSE2, and I'm still not sure
    // it's faster than the default scalar operation. But anyway...
    // suppose v = (a, b, c, d)
    vfloat4 ab_ab_cd_cd = shuffle<1,0,3,2>(v) + v;
    // now x = (b,a,d,c) + (a,b,c,d) = (a+b,a+b,c+d,c+d)
    vfloat4 cd_cd_ab_ab = shuffle<2,3,0,1>(ab_ab_cd_cd);
    // now y = (c+d,c+d,a+b,a+b)
    vfloat4 abcd = ab_ab_cd_cd + cd_cd_ab_ab;   // a+b+c+d in all components
    return abcd;
#else
    return vfloat4 (v[0] + v[1] + v[2] + v[3]);
#endif
}


OIIO_FORCEINLINE float reduce_add (const vfloat4& v) {
#if OIIO_SIMD_SSE
    return _mm_cvtss_f32(vreduce_add (v));
#else
    return v[0] + v[1] + v[2] + v[3];
#endif
}

OIIO_FORCEINLINE vfloat4 vdot (const vfloat4 &a, const vfloat4 &b) {
#if OIIO_SIMD_SSE >= 4
    return _mm_dp_ps (a.simd(), b.simd(), 0xff);
#elif OIIO_SIMD_NEON
    float32x4_t ab = vmulq_f32(a, b);
    float32x4_t sum1 = vaddq_f32(ab, vrev64q_f32(ab));
    return vaddq_f32(sum1, vcombine_f32(vget_high_f32(sum1), vget_low_f32(sum1)));
#else
    return vreduce_add (a*b);
#endif
}

OIIO_FORCEINLINE float dot (const vfloat4 &a, const vfloat4 &b) {
#if OIIO_SIMD_SSE >= 4
    return _mm_cvtss_f32 (_mm_dp_ps (a.simd(), b.simd(), 0xff));
#else
    return reduce_add (a*b);
#endif
}

OIIO_FORCEINLINE vfloat4 vdot3 (const vfloat4 &a, const vfloat4 &b) {
#if OIIO_SIMD_SSE >= 4
    return _mm_dp_ps (a.simd(), b.simd(), 0x7f);
#else
    return vreduce_add((a*b).xyz0());
#endif
}

OIIO_FORCEINLINE float dot3 (const vfloat4 &a, const vfloat4 &b) {
#if OIIO_SIMD_SSE >= 4
    return _mm_cvtss_f32 (_mm_dp_ps (a.simd(), b.simd(), 0x77));
#else
    return reduce_add ((a*b).xyz0());
#endif
}


OIIO_FORCEINLINE vfloat4 blend (const vfloat4& a, const vfloat4& b, const vbool4& mask)
{
#if OIIO_SIMD_SSE >= 4
    // SSE >= 4.1 only
    return _mm_blendv_ps (a.simd(), b.simd(), mask.simd());
#elif OIIO_SIMD_SSE
    // Trick for SSE < 4.1
    return _mm_or_ps (_mm_and_ps(mask.simd(), b.simd()),
                      _mm_andnot_ps(mask.simd(), a.simd()));
#else
    return vfloat4 (mask[0] ? b[0] : a[0],
                   mask[1] ? b[1] : a[1],
                   mask[2] ? b[2] : a[2],
                   mask[3] ? b[3] : a[3]);
#endif
}


OIIO_FORCEINLINE vfloat4 blend0 (const vfloat4& a, const vbool4& mask)
{
#if OIIO_SIMD_SSE
    return _mm_and_ps(mask.simd(), a.simd());
#else
    return vfloat4 (mask[0] ? a[0] : 0.0f,
                   mask[1] ? a[1] : 0.0f,
                   mask[2] ? a[2] : 0.0f,
                   mask[3] ? a[3] : 0.0f);
#endif
}


OIIO_FORCEINLINE vfloat4 blend0not (const vfloat4& a, const vbool4& mask)
{
#if OIIO_SIMD_SSE
    return _mm_andnot_ps(mask.simd(), a.simd());
#else
    return vfloat4 (mask[0] ? 0.0f : a[0],
                   mask[1] ? 0.0f : a[1],
                   mask[2] ? 0.0f : a[2],
                   mask[3] ? 0.0f : a[3]);
#endif
}


OIIO_FORCEINLINE vfloat4 safe_div (const vfloat4 &a, const vfloat4 &b) {
#if OIIO_SIMD_SSE
    return blend0not (a/b, b == vfloat4::Zero());
#else
    return vfloat4 (b[0] == 0.0f ? 0.0f : a[0] / b[0],
                   b[1] == 0.0f ? 0.0f : a[1] / b[1],
                   b[2] == 0.0f ? 0.0f : a[2] / b[2],
                   b[3] == 0.0f ? 0.0f : a[3] / b[3]);
#endif
}


OIIO_FORCEINLINE vfloat3 hdiv (const vfloat4 &a)
{
#if OIIO_SIMD_SSE
    return vfloat3(safe_div(a, shuffle<3>(a)).xyz0());
#else
    float d = a[3];
    return d == 0.0f ? vfloat3 (0.0f) : vfloat3 (a[0]/d, a[1]/d, a[2]/d);
#endif
}



OIIO_FORCEINLINE vfloat4 select (const vbool4& mask, const vfloat4& a, const vfloat4& b)
{
    return blend (b, a, mask);
}


OIIO_FORCEINLINE vfloat4 abs (const vfloat4& a)
{
#if OIIO_SIMD_SSE
    // Just clear the sign bit for cheap fabsf
    return _mm_and_ps (a.simd(), _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
#else
    SIMD_RETURN (vfloat4, fabsf(a[i]));
#endif
}


OIIO_FORCEINLINE vfloat4 sign (const vfloat4& a)
{
    vfloat4 one(1.0f);
    return blend (one, -one, a < vfloat4::Zero());
}


OIIO_FORCEINLINE vfloat4 ceil (const vfloat4& a)
{
#if OIIO_SIMD_SSE >= 4  /* SSE >= 4.1 */
    return _mm_ceil_ps (a);
#else
    SIMD_RETURN (vfloat4, ceilf(a[i]));
#endif
}

OIIO_FORCEINLINE vfloat4 floor (const vfloat4& a)
{
#if OIIO_SIMD_SSE >= 4  /* SSE >= 4.1 */
    return _mm_floor_ps (a);
#else
    SIMD_RETURN (vfloat4, floorf(a[i]));
#endif
}

OIIO_FORCEINLINE vfloat4 round (const vfloat4& a)
{
#if OIIO_SIMD_SSE >= 4  /* SSE >= 4.1 */
    return _mm_round_ps (a, (_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC));
#else
    SIMD_RETURN (vfloat4, roundf(a[i]));
#endif
}

OIIO_FORCEINLINE vint4 ifloor (const vfloat4& a)
{
    // FIXME: look into this, versus the method of quick_floor in texturesys.cpp
#if OIIO_SIMD_SSE >= 4  /* SSE >= 4.1 */
    return vint4(floor(a));
#else
    SIMD_RETURN (vint4, (int)floorf(a[i]));
#endif
}


OIIO_FORCEINLINE vint4 rint (const vfloat4& a)
{
    return vint4 (round(a));
}


OIIO_FORCEINLINE vfloat4 rcp_fast (const vfloat4 &a)
{
#if OIIO_SIMD_AVX512 && OIIO_AVX512VL_ENABLED
    // avx512vl directly has rcp14 on float4
    vfloat4 r = _mm_rcp14_ps(a);
    return r * nmadd(r,a,vfloat4(2.0f));
#elif OIIO_SIMD_AVX512
    // Trickery: in and out of the 512 bit registers to use fast approx rcp
    vfloat16 r = _mm512_rcp14_ps(_mm512_castps128_ps512(a));
    return _mm512_castps512_ps128(r);
#elif OIIO_SIMD_SSE
    vfloat4 r = _mm_rcp_ps(a);
    return r * nmadd(r,a,vfloat4(2.0f));
#else
    SIMD_RETURN (vfloat4, 1.0f/a[i]);
#endif
}


OIIO_FORCEINLINE vfloat4 sqrt (const vfloat4 &a)
{
#if OIIO_SIMD_SSE
    return _mm_sqrt_ps (a.simd());
#else
    SIMD_RETURN (vfloat4, sqrtf(a[i]));
#endif
}


OIIO_FORCEINLINE vfloat4 rsqrt (const vfloat4 &a)
{
#if OIIO_SIMD_SSE
    return _mm_div_ps (_mm_set1_ps(1.0f), _mm_sqrt_ps (a.simd()));
#else
    SIMD_RETURN (vfloat4, 1.0f/sqrtf(a[i]));
#endif
}


OIIO_FORCEINLINE vfloat4 rsqrt_fast (const vfloat4 &a)
{
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512ER_ENABLED
    // Trickery: in and out of the 512 bit registers to use fast approx rsqrt
    return _mm512_castps512_ps128(_mm512_rsqrt28_round_ps(_mm512_castps128_ps512(a), _MM_FROUND_NO_EXC));
#elif OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // Trickery: in and out of the 512 bit registers to use fast approx rsqrt
    return _mm512_castps512_ps128(_mm512_rsqrt14_ps(_mm512_castps128_ps512(a)));
#elif OIIO_SIMD_SSE
    return _mm_rsqrt_ps (a.simd());
#else
    SIMD_RETURN (vfloat4, 1.0f/sqrtf(a[i]));
#endif
}


OIIO_FORCEINLINE vfloat4 min (const vfloat4& a, const vfloat4& b)
{
#if OIIO_SIMD_SSE
    return _mm_min_ps (a, b);
#else
    SIMD_RETURN (vfloat4, std::min (a[i], b[i]));
#endif
}

OIIO_FORCEINLINE vfloat4 max (const vfloat4& a, const vfloat4& b)
{
#if OIIO_SIMD_SSE
    return _mm_max_ps (a, b);
#else
    SIMD_RETURN (vfloat4, std::max (a[i], b[i]));
#endif
}


OIIO_FORCEINLINE vfloat4 andnot (const vfloat4& a, const vfloat4& b) {
#if OIIO_SIMD_SSE
    return _mm_andnot_ps (a.simd(), b.simd());
#else
    const int *ai = (const int *)&a;
    const int *bi = (const int *)&b;
    return bitcast_to_float (vint4(~(ai[0]) & bi[0],
                                  ~(ai[1]) & bi[1],
                                  ~(ai[2]) & bi[2],
                                  ~(ai[3]) & bi[3]));
#endif
}


OIIO_FORCEINLINE vfloat4 madd (const simd::vfloat4& a, const simd::vfloat4& b,
                              const simd::vfloat4& c)
{
#if OIIO_SIMD_SSE && OIIO_FMA_ENABLED
    // If we are sure _mm_fmadd_ps intrinsic is available, use it.
    return _mm_fmadd_ps (a, b, c);
#elif OIIO_SIMD_SSE && !defined(_MSC_VER)
    // If we directly access the underlying __m128, on some platforms and
    // compiler flags, it will turn into fma anyway, even if we don't use
    // the intrinsic.
    return a.simd() * b.simd() + c.simd();
#else
    // Fallback: just use regular math and hope for the best.
    return a * b + c;
#endif
}


OIIO_FORCEINLINE vfloat4 msub (const simd::vfloat4& a, const simd::vfloat4& b,
                              const simd::vfloat4& c)
{
#if OIIO_SIMD_SSE && OIIO_FMA_ENABLED
    // If we are sure _mm_fnmsub_ps intrinsic is available, use it.
    return _mm_fmsub_ps (a, b, c);
#elif OIIO_SIMD_SSE && !defined(_MSC_VER)
    // If we directly access the underlying __m128, on some platforms and
    // compiler flags, it will turn into fma anyway, even if we don't use
    // the intrinsic.
    return a.simd() * b.simd() - c.simd();
#else
    // Fallback: just use regular math and hope for the best.
    return a * b - c;
#endif
}



OIIO_FORCEINLINE vfloat4 nmadd (const simd::vfloat4& a, const simd::vfloat4& b,
                               const simd::vfloat4& c)
{
#if OIIO_SIMD_SSE && OIIO_FMA_ENABLED
    // If we are sure _mm_fnmadd_ps intrinsic is available, use it.
    return _mm_fnmadd_ps (a, b, c);
#elif OIIO_SIMD_SSE && !defined(_MSC_VER)
    // If we directly access the underlying __m128, on some platforms and
    // compiler flags, it will turn into fma anyway, even if we don't use
    // the intrinsic.
    return c.simd() - a.simd() * b.simd();
#else
    // Fallback: just use regular math and hope for the best.
    return c - a * b;
#endif
}



OIIO_FORCEINLINE vfloat4 nmsub (const simd::vfloat4& a, const simd::vfloat4& b,
                               const simd::vfloat4& c)
{
#if OIIO_SIMD_SSE && OIIO_FMA_ENABLED
    // If we are sure _mm_fnmsub_ps intrinsic is available, use it.
    return _mm_fnmsub_ps (a, b, c);
#elif OIIO_SIMD_SSE && !defined(_MSC_VER)
    // If we directly access the underlying __m128, on some platforms and
    // compiler flags, it will turn into fma anyway, even if we don't use
    // the intrinsic.
    return -(a.simd() * b.simd()) - c.simd();
#else
    // Fallback: just use regular math and hope for the best.
    return -(a * b) - c;
#endif
}



// Full precision exp() of all components of a SIMD vector.
template<typename T>
OIIO_FORCEINLINE T exp (const T& v)
{
#if OIIO_SIMD_SSE
    // Implementation inspired by:
    // https://github.com/embree/embree/blob/master/common/simd/sse_special.h
    // Which is listed as Copyright (C) 2007  Julien Pommier and distributed
    // under the zlib license.
    typedef typename T::vint_t int_t;
    T x = v;
    const float exp_hi (88.3762626647949f);
    const float exp_lo (-88.3762626647949f);
    const float cephes_LOG2EF (1.44269504088896341f);
    const float cephes_exp_C1 (0.693359375f);
    const float cephes_exp_C2 (-2.12194440e-4f);
    const float cephes_exp_p0 (1.9875691500E-4f);
    const float cephes_exp_p1 (1.3981999507E-3f);
    const float cephes_exp_p2 (8.3334519073E-3f);
    const float cephes_exp_p3 (4.1665795894E-2f);
    const float cephes_exp_p4 (1.6666665459E-1f);
    const float cephes_exp_p5 (5.0000001201E-1f);
    T tmp (0.0f);
    T one (1.0f);
    x = min (x, T(exp_hi));
    x = max (x, T(exp_lo));
    T fx = madd (x, T(cephes_LOG2EF), T(0.5f));
    int_t emm0 = int_t(fx);
    tmp = T(emm0);
    T mask = bitcast_to_float (bitcast_to_int(tmp > fx) & bitcast_to_int(one));
    fx = tmp - mask;
    tmp = fx * cephes_exp_C1;
    T z = fx * cephes_exp_C2;
    x = x - tmp;
    x = x - z;
    z = x * x;
    T y = cephes_exp_p0;
    y = madd (y, x, cephes_exp_p1);
    y = madd (y, x, cephes_exp_p2);
    y = madd (y, x, cephes_exp_p3);
    y = madd (y, x, cephes_exp_p4);
    y = madd (y, x, cephes_exp_p5);
    y = madd (y, z, x);
    y = y + one;
    emm0 = (int_t(fx) + int_t(0x7f)) << 23;
    T pow2n = bitcast_to_float(emm0);
    y = y * pow2n;
    return y;
#else
    SIMD_RETURN (T, expf(v[i]));
#endif
}



// Full precision log() of all components of a SIMD vector.
template<typename T>
OIIO_FORCEINLINE T log (const T& v)
{
#if OIIO_SIMD_SSE
    // Implementation inspired by:
    // https://github.com/embree/embree/blob/master/common/simd/sse_special.h
    // Which is listed as Copyright (C) 2007  Julien Pommier and distributed
    // under the zlib license.
    typedef typename T::vint_t int_t;
    typedef typename T::vbool_t bool_t;
    T x = v;
    int_t emm0;
    T zero (T::Zero());
    T one (1.0f);
    bool_t invalid_mask = (x <= zero);
    const int min_norm_pos ((int)0x00800000);
    const int inv_mant_mask ((int)~0x7f800000);
    x = max(x, bitcast_to_float(int_t(min_norm_pos)));  /* cut off denormalized stuff */
    emm0 = srl (bitcast_to_int(x), 23);
    /* keep only the fractional part */
    x = bitcast_to_float (bitcast_to_int(x) & int_t(inv_mant_mask));
    x = bitcast_to_float (bitcast_to_int(x) | bitcast_to_int(T(0.5f)));
    emm0 = emm0 - int_t(0x7f);
    T e (emm0);
    e = e + one;
    // OIIO_SIMD_vFLOAT4_CONST (cephes_SQRTHF, 0.707106781186547524f);
    const float cephes_SQRTHF (0.707106781186547524f);
    bool_t mask = (x < T(cephes_SQRTHF));
    T tmp = bitcast_to_float (bitcast_to_int(x) & bitcast_to_int(mask));
    x = x - one;
    e = e - bitcast_to_float (bitcast_to_int(one) & bitcast_to_int(mask));
    x = x + tmp;
    T z = x * x;
    const float cephes_log_p0 (7.0376836292E-2f);
    const float cephes_log_p1 (- 1.1514610310E-1f);
    const float cephes_log_p2 (1.1676998740E-1f);
    const float cephes_log_p3 (- 1.2420140846E-1f);
    const float cephes_log_p4 (+ 1.4249322787E-1f);
    const float cephes_log_p5 (- 1.6668057665E-1f);
    const float cephes_log_p6 (+ 2.0000714765E-1f);
    const float cephes_log_p7 (- 2.4999993993E-1f);
    const float cephes_log_p8 (+ 3.3333331174E-1f);
    const float cephes_log_q1 (-2.12194440e-4f);
    const float cephes_log_q2 (0.693359375f);
    T y = cephes_log_p0;
    y = madd (y, x, T(cephes_log_p1));
    y = madd (y, x, T(cephes_log_p2));
    y = madd (y, x, T(cephes_log_p3));
    y = madd (y, x, T(cephes_log_p4));
    y = madd (y, x, T(cephes_log_p5));
    y = madd (y, x, T(cephes_log_p6));
    y = madd (y, x, T(cephes_log_p7));
    y = madd (y, x, T(cephes_log_p8));
    y = y * x;
    y = y * z;
    y = madd(e, T(cephes_log_q1), y);
    y = nmadd (z, 0.5f, y);
    x = x + y;
    x = madd (e, T(cephes_log_q2), x);
    x = bitcast_to_float (bitcast_to_int(x) | bitcast_to_int(invalid_mask)); // negative arg will be NAN
    return x;
#else
    SIMD_RETURN (T, logf(v[i]));
#endif
}



OIIO_FORCEINLINE void transpose (vfloat4 &a, vfloat4 &b, vfloat4 &c, vfloat4 &d)
{
#if OIIO_SIMD_SSE
    _MM_TRANSPOSE4_PS (a, b, c, d);
#else
    vfloat4 A (a[0], b[0], c[0], d[0]);
    vfloat4 B (a[1], b[1], c[1], d[1]);
    vfloat4 C (a[2], b[2], c[2], d[2]);
    vfloat4 D (a[3], b[3], c[3], d[3]);
    a = A;  b = B;  c = C;  d = D;
#endif
}


OIIO_FORCEINLINE void transpose (const vfloat4& a, const vfloat4& b, const vfloat4& c, const vfloat4& d,
                                 vfloat4 &r0, vfloat4 &r1, vfloat4 &r2, vfloat4 &r3)
{
#if OIIO_SIMD_SSE
    //_MM_TRANSPOSE4_PS (a, b, c, d);
    vfloat4 l02 = _mm_unpacklo_ps (a, c);
    vfloat4 h02 = _mm_unpackhi_ps (a, c);
    vfloat4 l13 = _mm_unpacklo_ps (b, d);
    vfloat4 h13 = _mm_unpackhi_ps (b, d);
    r0 = _mm_unpacklo_ps (l02, l13);
    r1 = _mm_unpackhi_ps (l02, l13);
    r2 = _mm_unpacklo_ps (h02, h13);
    r3 = _mm_unpackhi_ps (h02, h13);
#else
    r0.load (a[0], b[0], c[0], d[0]);
    r1.load (a[1], b[1], c[1], d[1]);
    r2.load (a[2], b[2], c[2], d[2]);
    r3.load (a[3], b[3], c[3], d[3]);
#endif
}


OIIO_FORCEINLINE void transpose (vint4 &a, vint4 &b, vint4 &c, vint4 &d)
{
#if OIIO_SIMD_SSE
    __m128 A = _mm_castsi128_ps (a);
    __m128 B = _mm_castsi128_ps (b);
    __m128 C = _mm_castsi128_ps (c);
    __m128 D = _mm_castsi128_ps (d);
    _MM_TRANSPOSE4_PS (A, B, C, D);
    a = _mm_castps_si128 (A);
    b = _mm_castps_si128 (B);
    c = _mm_castps_si128 (C);
    d = _mm_castps_si128 (D);
#else
    vint4 A (a[0], b[0], c[0], d[0]);
    vint4 B (a[1], b[1], c[1], d[1]);
    vint4 C (a[2], b[2], c[2], d[2]);
    vint4 D (a[3], b[3], c[3], d[3]);
    a = A;  b = B;  c = C;  d = D;
#endif
}

OIIO_FORCEINLINE void transpose (const vint4& a, const vint4& b, const vint4& c, const vint4& d,
                                 vint4 &r0, vint4 &r1, vint4 &r2, vint4 &r3)
{
#if OIIO_SIMD_SSE
    //_MM_TRANSPOSE4_PS (a, b, c, d);
    __m128 A = _mm_castsi128_ps (a);
    __m128 B = _mm_castsi128_ps (b);
    __m128 C = _mm_castsi128_ps (c);
    __m128 D = _mm_castsi128_ps (d);
    _MM_TRANSPOSE4_PS (A, B, C, D);
    r0 = _mm_castps_si128 (A);
    r1 = _mm_castps_si128 (B);
    r2 = _mm_castps_si128 (C);
    r3 = _mm_castps_si128 (D);
#else
    r0.load (a[0], b[0], c[0], d[0]);
    r1.load (a[1], b[1], c[1], d[1]);
    r2.load (a[2], b[2], c[2], d[2]);
    r3.load (a[3], b[3], c[3], d[3]);
#endif
}


OIIO_FORCEINLINE vfloat4 AxBxCxDx (const vfloat4& a, const vfloat4& b,
                                  const vfloat4& c, const vfloat4& d)
{
#if OIIO_SIMD_SSE
    vfloat4 l02 = _mm_unpacklo_ps (a, c);
    vfloat4 l13 = _mm_unpacklo_ps (b, d);
    return _mm_unpacklo_ps (l02, l13);
#else
    return vfloat4 (a[0], b[0], c[0], d[0]);
#endif
}


OIIO_FORCEINLINE vint4 AxBxCxDx (const vint4& a, const vint4& b,
                                const vint4& c, const vint4& d)
{
#if OIIO_SIMD_SSE
    vint4 l02 = _mm_unpacklo_epi32 (a, c);
    vint4 l13 = _mm_unpacklo_epi32 (b, d);
    return _mm_unpacklo_epi32 (l02, l13);
#else
    return vint4 (a[0], b[0], c[0], d[0]);
#endif
}



//////////////////////////////////////////////////////////////////////
// vfloat3 implementation

OIIO_FORCEINLINE vfloat3::vfloat3 (const vfloat3 &other)  : vfloat4(other) {
#if OIIO_SIMD_SSE || OIIO_SIMD_NEON
    m_simd = other.m_simd;
#else
    SIMD_CONSTRUCT_PAD (other[i]);
#endif
}

OIIO_FORCEINLINE vfloat3::vfloat3 (const vfloat4 &other) {
#if OIIO_SIMD_SSE || OIIO_SIMD_NEON
    m_simd = other.simd();
#else
    SIMD_CONSTRUCT_PAD (other[i]);
    m_val[3] = 0.0f;
#endif
}

OIIO_FORCEINLINE const vfloat3 vfloat3::Zero () { return vfloat3(vfloat4::Zero()); }

OIIO_FORCEINLINE const vfloat3 vfloat3::One () { return vfloat3(1.0f); }

OIIO_FORCEINLINE const vfloat3 vfloat3::Iota (float start, float step) {
    return vfloat3 (start+0.0f*step, start+1.0f*step, start+2.0f*step);
}


OIIO_FORCEINLINE void vfloat3::load (float val) { vfloat4::load (val, val, val, 0.0f); }

OIIO_FORCEINLINE void vfloat3::load (const float *values) { vfloat4::load (values, 3); }

OIIO_FORCEINLINE void vfloat3::load (const float *values, int n) {
    vfloat4::load (values, n);
}

OIIO_FORCEINLINE void vfloat3::load (const unsigned short *values) {
    vfloat4::load (float(values[0]), float(values[1]), float(values[2]));
}

OIIO_FORCEINLINE void vfloat3::load (const short *values) {
    vfloat4::load (float(values[0]), float(values[1]), float(values[2]));
}

OIIO_FORCEINLINE void vfloat3::load (const unsigned char *values) {
    vfloat4::load (float(values[0]), float(values[1]), float(values[2]));
}

OIIO_FORCEINLINE void vfloat3::load (const char *values) {
    vfloat4::load (float(values[0]), float(values[1]), float(values[2]));
}

#ifdef _HALF_H_
OIIO_FORCEINLINE void vfloat3::load (const half *values) {
    vfloat4::load (float(values[0]), float(values[1]), float(values[2]));
}
#endif /* _HALF_H_ */

OIIO_FORCEINLINE void vfloat3::store (float *values) const {
    vfloat4::store (values, 3);
}

OIIO_FORCEINLINE void vfloat3::store (float *values, int n) const {
    vfloat4::store (values, n);
}

#ifdef _HALF_H_
OIIO_FORCEINLINE void vfloat3::store (half *values) const {
    SIMD_DO (values[i] = m_val[i]);
}
#endif

OIIO_FORCEINLINE void vfloat3::store (Imath::V3f &vec) const {
    store ((float *)&vec);
}

OIIO_FORCEINLINE vfloat3 operator+ (const vfloat3& a, const vfloat3& b) {
    return vfloat3 (vfloat4(a) + vfloat4(b));
}

OIIO_FORCEINLINE const vfloat3 & vfloat3::operator+= (const vfloat3& a) {
    *this = *this + a; return *this;
}

OIIO_FORCEINLINE vfloat3 vfloat3::operator- () const {
    return vfloat3 (-vfloat4(*this));
}

OIIO_FORCEINLINE vfloat3 operator- (const vfloat3& a, const vfloat3& b) {
    return vfloat3 (vfloat4(a) - vfloat4(b));
}

OIIO_FORCEINLINE const vfloat3 & vfloat3::operator-= (const vfloat3& a) {
    *this = *this - a; return *this;
}

OIIO_FORCEINLINE vfloat3 operator* (const vfloat3& a, const vfloat3& b) {
    return vfloat3 (vfloat4(a) * vfloat4(b));
}

OIIO_FORCEINLINE const vfloat3 & vfloat3::operator*= (const vfloat3& a) {
    *this = *this * a; return *this;
}

OIIO_FORCEINLINE const vfloat3 & vfloat3::operator*= (float a) {
    *this = *this * a; return *this;
}

OIIO_FORCEINLINE vfloat3 operator/ (const vfloat3& a, const vfloat3& b) {
    return vfloat3 (vfloat4(a) / b.xyz1()); // Avoid divide by zero!
}

OIIO_FORCEINLINE const vfloat3 & vfloat3::operator/= (const vfloat3& a) {
    *this = *this / a; return *this;
}

OIIO_FORCEINLINE const vfloat3 & vfloat3::operator/= (float a) {
    *this = *this / a; return *this;
}


inline std::ostream& operator<< (std::ostream& cout, const vfloat3& val) {
    cout << val[0];
    for (int i = 1; i < val.elements; ++i)
        cout << ' ' << val[i];
    return cout;
}


OIIO_FORCEINLINE vfloat3 vreduce_add (const vfloat3& v) {
#if OIIO_SIMD_SSE
    return vfloat3 ((vreduce_add(vfloat4(v))).xyz0());
#else
    return vfloat3 (v[0] + v[1] + v[2]);
#endif
}


OIIO_FORCEINLINE vfloat3 vdot (const vfloat3 &a, const vfloat3 &b) {
#if OIIO_SIMD_SSE >= 4
    return vfloat3(_mm_dp_ps (a.simd(), b.simd(), 0x77));
#else
    return vreduce_add (a*b);
#endif
}


OIIO_FORCEINLINE float dot (const vfloat3 &a, const vfloat3 &b) {
#if OIIO_SIMD_SSE >= 4
    return _mm_cvtss_f32 (_mm_dp_ps (a.simd(), b.simd(), 0x77));
#elif OIIO_SIMD
    return reduce_add (a*b);
#else
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
#endif
}


OIIO_FORCEINLINE vfloat3 vdot3 (const vfloat3 &a, const vfloat3 &b) {
#if OIIO_SIMD_SSE >= 4
    return vfloat3(_mm_dp_ps (a.simd(), b.simd(), 0x77));
#else
    return vfloat3 (vreduce_add((a*b).xyz0()).xyz0());
#endif
}


OIIO_FORCEINLINE float vfloat3::length2 () const
{
    return dot(*this, *this);
}


OIIO_FORCEINLINE float vfloat3::length () const
{
    return sqrtf(dot(*this, *this));
}


OIIO_FORCEINLINE vfloat3 vfloat3::normalized () const
{
#if OIIO_SIMD
    vfloat3 len2 = vdot3 (*this, *this);
    return vfloat3 (safe_div (*this, sqrt(len2)));
#else
    float len2 = dot (*this, *this);
    return len2 > 0.0f ? (*this) / sqrtf(len2) : vfloat3::Zero();
#endif
}


OIIO_FORCEINLINE vfloat3 vfloat3::normalized_fast () const
{
#if OIIO_SIMD
    vfloat3 len2 = vdot3 (*this, *this);
    vfloat4 invlen = blend0not (rsqrt_fast (len2), len2 == vfloat4::Zero());
    return vfloat3 ((*this) * invlen);
#else
    float len2 = dot (*this, *this);
    return len2 > 0.0f ? (*this) / sqrtf(len2) : vfloat3::Zero();
#endif
}



//////////////////////////////////////////////////////////////////////
// matrix44 implementation


OIIO_FORCEINLINE const Imath::M44f& matrix44::M44f() const {
    return *(Imath::M44f*)this;
}


OIIO_FORCEINLINE vfloat4 matrix44::operator[] (int i) const {
#if OIIO_SIMD_SSE
    return m_row[i];
#else
    return vfloat4 (m_mat[i]);
#endif
}


OIIO_FORCEINLINE matrix44 matrix44::transposed () const {
    matrix44 T;
#if OIIO_SIMD_SSE
    simd::transpose (m_row[0], m_row[1], m_row[2], m_row[3],
                     T.m_row[0], T.m_row[1], T.m_row[2], T.m_row[3]);
#else
    T = m_mat.transposed();
#endif
    return T;
}

OIIO_FORCEINLINE vfloat3 matrix44::transformp (const vfloat3 &V) const {
#if OIIO_SIMD_SSE
    vfloat4 R = shuffle<0>(V) * m_row[0] + shuffle<1>(V) * m_row[1] +
               shuffle<2>(V) * m_row[2] + m_row[3];
    R = R / shuffle<3>(R);
    return vfloat3 (R.xyz0());
#else
    Imath::V3f R;
    m_mat.multVecMatrix (*(Imath::V3f *)&V, R);
    return vfloat3(R);
#endif
}

OIIO_FORCEINLINE vfloat3 matrix44::transformv (const vfloat3 &V) const {
#if OIIO_SIMD_SSE
    vfloat4 R = shuffle<0>(V) * m_row[0] + shuffle<1>(V) * m_row[1] +
               shuffle<2>(V) * m_row[2];
    return vfloat3 (R.xyz0());
#else
    Imath::V3f R;
    m_mat.multDirMatrix (*(Imath::V3f *)&V, R);
    return vfloat3(R);
#endif
}

OIIO_FORCEINLINE vfloat3 matrix44::transformvT (const vfloat3 &V) const {
#if OIIO_SIMD_SSE
    matrix44 T = transposed();
    vfloat4 R = shuffle<0>(V) * T[0] + shuffle<1>(V) * T[1] +
               shuffle<2>(V) * T[2];
    return vfloat3 (R.xyz0());
#else
    Imath::V3f R;
    m_mat.transposed().multDirMatrix (*(Imath::V3f *)&V, R);
    return vfloat3(R);
#endif
}

OIIO_FORCEINLINE vfloat4 operator* (const vfloat4 &V, const matrix44& M)
{
#if OIIO_SIMD_SSE
    return shuffle<0>(V) * M[0] + shuffle<1>(V) * M[1] +
           shuffle<2>(V) * M[2] + shuffle<3>(V) * M[3];
#else
    return vfloat4(V.V4f() * M.M44f());
#endif
}

OIIO_FORCEINLINE vfloat4 operator* (const matrix44& M, const vfloat4 &V)
{
#if OIIO_SIMD_SSE >= 3
    vfloat4 m0v = M[0] * V;  // [ M00*Vx, M01*Vy, M02*Vz, M03*Vw ]
    vfloat4 m1v = M[1] * V;  // [ M10*Vx, M11*Vy, M12*Vz, M13*Vw ]
    vfloat4 m2v = M[2] * V;  // [ M20*Vx, M21*Vy, M22*Vz, M23*Vw ]
    vfloat4 m3v = M[3] * V;  // [ M30*Vx, M31*Vy, M32*Vz, M33*Vw ]
    vfloat4 s01 = _mm_hadd_ps(m0v, m1v);
       // [ M00*Vx + M01*Vy, M02*Vz + M03*Vw, M10*Vx + M11*Vy, M12*Vz + M13*Vw ]
    vfloat4 s23 = _mm_hadd_ps(m2v, m3v);
       // [ M20*Vx + M21*Vy, M22*Vz + M23*Vw, M30*Vx + M31*Vy, M32*Vz + M33*Vw ]
    vfloat4 result = _mm_hadd_ps(s01, s23);
       // [ M00*Vx + M01*Vy + M02*Vz + M03*Vw,
       //   M10*Vx + M11*Vy + M12*Vz + M13*Vw,
       //   M20*Vx + M21*Vy + M22*Vz + M23*Vw,
       //   M30*Vx + M31*Vy + M32*Vz + M33*Vw ]
    return result;
#else
    return vfloat4(dot(M[0], V), dot(M[1], V), dot(M[2], V), dot(M[3], V));
#endif
}


OIIO_FORCEINLINE bool matrix44::operator== (const matrix44& m) const {
#if OIIO_SIMD_SSE
    vbool4 b0 = (m_row[0] == m[0]);
    vbool4 b1 = (m_row[1] == m[1]);
    vbool4 b2 = (m_row[2] == m[2]);
    vbool4 b3 = (m_row[3] == m[3]);
    return simd::all (b0 & b1 & b2 & b3);
#else
    return memcmp(this, &m, 16*sizeof(float)) == 0;
#endif
}

OIIO_FORCEINLINE bool matrix44::operator== (const Imath::M44f& m) const {
    return memcmp(this, &m, 16*sizeof(float)) == 0;
}

OIIO_FORCEINLINE bool operator== (const Imath::M44f& a, const matrix44 &b) {
    return (b == a);
}

OIIO_FORCEINLINE bool matrix44::operator!= (const matrix44& m) const {
#if OIIO_SIMD_SSE
    vbool4 b0 = (m_row[0] != m[0]);
    vbool4 b1 = (m_row[1] != m[1]);
    vbool4 b2 = (m_row[2] != m[2]);
    vbool4 b3 = (m_row[3] != m[3]);
    return simd::any (b0 | b1 | b2 | b3);
#else
    return memcmp(this, &m, 16*sizeof(float)) != 0;
#endif
}

OIIO_FORCEINLINE bool matrix44::operator!= (const Imath::M44f& m) const {
    return memcmp(this, &m, 16*sizeof(float)) != 0;
}

OIIO_FORCEINLINE bool operator!= (const Imath::M44f& a, const matrix44 &b) {
    return (b != a);
}

OIIO_FORCEINLINE matrix44 matrix44::inverse() const {
#if OIIO_SIMD_SSE
    // Adapted from this code from Intel:
    // ftp://download.intel.com/design/pentiumiii/sml/24504301.pdf
    vfloat4 minor0, minor1, minor2, minor3;
    vfloat4 row0, row1, row2, row3;
    vfloat4 det, tmp1;
    const float *src = (const float *)this;
    vfloat4 zero = vfloat4::Zero();
    tmp1 = _mm_loadh_pi(_mm_loadl_pi(zero, (__m64*)(src)), (__m64*)(src+ 4));
    row1 = _mm_loadh_pi(_mm_loadl_pi(zero, (__m64*)(src+8)), (__m64*)(src+12));
    row0 = _mm_shuffle_ps(tmp1, row1, 0x88);
    row1 = _mm_shuffle_ps(row1, tmp1, 0xDD);
    tmp1 = _mm_loadh_pi(_mm_loadl_pi(tmp1, (__m64*)(src+ 2)), (__m64*)(src+ 6));
    row3 = _mm_loadh_pi(_mm_loadl_pi(zero, (__m64*)(src+10)), (__m64*)(src+14));
    row2 = _mm_shuffle_ps(tmp1, row3, 0x88);
    row3 = _mm_shuffle_ps(row3, tmp1, 0xDD);
    // -----------------------------------------------
    tmp1 = row2 * row3;
    tmp1 = shuffle<1,0,3,2>(tmp1);
    minor0 = row1 * tmp1;
    minor1 = row0 * tmp1;
    tmp1 = shuffle<2,3,0,1>(tmp1);
    minor0 = (row1 * tmp1) - minor0;
    minor1 = (row0 * tmp1) - minor1;
    minor1 = shuffle<2,3,0,1>(minor1);
    // -----------------------------------------------
    tmp1 = row1 * row2;
    tmp1 = shuffle<1,0,3,2>(tmp1);
    minor0 = (row3 * tmp1) + minor0;
    minor3 = row0 * tmp1;
    tmp1 = shuffle<2,3,0,1>(tmp1);
    minor0 = minor0 - (row3 * tmp1);
    minor3 = (row0 * tmp1) - minor3;
    minor3 = shuffle<2,3,0,1>(minor3);
    // -----------------------------------------------
    tmp1 = shuffle<2,3,0,1>(row1) * row3;
    tmp1 = shuffle<1,0,3,2>(tmp1);
    row2 = shuffle<2,3,0,1>(row2);
    minor0 = (row2 * tmp1) + minor0;
    minor2 = row0 * tmp1;
    tmp1 = shuffle<2,3,0,1>(tmp1);
    minor0 = minor0 - (row2 * tmp1);
    minor2 = (row0 * tmp1) - minor2;
    minor2 = shuffle<2,3,0,1>(minor2);
    // -----------------------------------------------
    tmp1 = row0 * row1;
    tmp1 = shuffle<1,0,3,2>(tmp1);
    minor2 = (row3 * tmp1) + minor2;
    minor3 = (row2 * tmp1) - minor3;
    tmp1 = shuffle<2,3,0,1>(tmp1);
    minor2 = (row3 * tmp1) - minor2;
    minor3 = minor3 - (row2 * tmp1);
    // -----------------------------------------------
    tmp1 = row0 * row3;
    tmp1 = shuffle<1,0,3,2>(tmp1);
    minor1 = minor1 - (row2 * tmp1);
    minor2 = (row1 * tmp1) + minor2;
    tmp1 = shuffle<2,3,0,1>(tmp1);
    minor1 = (row2 * tmp1) + minor1;
    minor2 = minor2 - (row1 * tmp1);
    // -----------------------------------------------
    tmp1 = row0 * row2;
    tmp1 = shuffle<1,0,3,2>(tmp1);
    minor1 = (row3 * tmp1) + minor1;
    minor3 = minor3 - (row1 * tmp1);
    tmp1 = shuffle<2,3,0,1>(tmp1);
    minor1 = minor1 - (row3 * tmp1);
    minor3 = (row1 * tmp1) + minor3;
    // -----------------------------------------------
    det = row0 * minor0;
    det = shuffle<2,3,0,1>(det) + det;
    det = _mm_add_ss(shuffle<1,0,3,2>(det), det);
    tmp1 = _mm_rcp_ss(det);
    det = _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
    det = shuffle<0>(det);
    return matrix44 (det*minor0, det*minor1, det*minor2, det*minor3);
#else
    return m_mat.inverse();
#endif
}


inline std::ostream& operator<< (std::ostream& cout, const matrix44 &M) {
    const float *m = (const float *)&M;
    cout << m[0];
    for (int i = 1; i < 16; ++i)
        cout << ' ' << m[i];
    return cout;
}



OIIO_FORCEINLINE vfloat3 transformp (const matrix44 &M, const vfloat3 &V) {
    return M.transformp (V);
}

OIIO_FORCEINLINE vfloat3 transformp (const Imath::M44f &M, const vfloat3 &V)
{
#if OIIO_SIMD
    return matrix44(M).transformp (V);
#else
    Imath::V3f R;
    M.multVecMatrix (*(Imath::V3f *)&V, R);
    return vfloat3(R);
#endif
}


OIIO_FORCEINLINE vfloat3 transformv (const matrix44 &M, const vfloat3 &V) {
    return M.transformv (V);
}

OIIO_FORCEINLINE vfloat3 transformv (const Imath::M44f &M, const vfloat3 &V)
{
#if OIIO_SIMD
    return matrix44(M).transformv (V);
#else
    Imath::V3f R;
    M.multDirMatrix (*(Imath::V3f *)&V, R);
    return vfloat3(R);
#endif
}

OIIO_FORCEINLINE vfloat3 transformvT (const matrix44 &M, const vfloat3 &V)
{
    return M.transformvT (V);
}

OIIO_FORCEINLINE vfloat3 transformvT (const Imath::M44f &M, const vfloat3 &V)
{
#if OIIO_SIMD
    return matrix44(M).transformvT(V);
#else
    return transformv (M.transposed(), V);
#endif
}



//////////////////////////////////////////////////////////////////////
// vfloat8 implementation

OIIO_FORCEINLINE float& vfloat8::operator[] (int i) {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}

OIIO_FORCEINLINE float vfloat8::operator[] (int i) const {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}


inline std::ostream& operator<< (std::ostream& cout, const vfloat8& val) {
    cout << val[0];
    for (int i = 1; i < val.elements; ++i)
        cout << ' ' << val[i];
    return cout;
}


OIIO_FORCEINLINE vfloat4 vfloat8::lo () const {
#if OIIO_SIMD_AVX
    return _mm256_castps256_ps128 (simd());
#else
    return m_4[0];
#endif
}

OIIO_FORCEINLINE vfloat4 vfloat8::hi () const {
#if OIIO_SIMD_AVX
    return _mm256_extractf128_ps (simd(), 1);
#else
    return m_4[1];
#endif
}


OIIO_FORCEINLINE vfloat8::vfloat8 (const vfloat4& lo, const vfloat4 &hi) {
#if OIIO_SIMD_AVX
    __m256 r = _mm256_castps128_ps256 (lo);
    m_simd = _mm256_insertf128_ps (r, hi, 1);
    // N.B. equivalent, if available: m_simd = _mm256_set_m128 (hi, lo);
    // FIXME: when would that not be available?
#else
    m_4[0] = lo;
    m_4[1] = hi;
#endif
}


OIIO_FORCEINLINE vfloat8::vfloat8 (const vint8& ival) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_cvtepi32_ps (ival);
#else
    SIMD_CONSTRUCT (float(ival[i]));
#endif
}


OIIO_FORCEINLINE const vfloat8 vfloat8::Zero () {
#if OIIO_SIMD_AVX
    return _mm256_setzero_ps();
#else
    return vfloat8(0.0f);
#endif
}

OIIO_FORCEINLINE const vfloat8 vfloat8::One () {
    return vfloat8(1.0f);
}

OIIO_FORCEINLINE const vfloat8 vfloat8::Iota (float start, float step) {
    return vfloat8 (start+0.0f*step, start+1.0f*step, start+2.0f*step, start+3.0f*step,
                   start+4.0f*step, start+5.0f*step, start+6.0f*step, start+7.0f*step);
}

/// Set all components to 0.0
OIIO_FORCEINLINE void vfloat8::clear () {
#if OIIO_SIMD_AVX
    m_simd = _mm256_setzero_ps();
#else
    load (0.0f);
#endif
}



OIIO_FORCEINLINE void vfloat8::load (float val) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_set1_ps (val);
#else
    SIMD_CONSTRUCT (val);
#endif
}

OIIO_FORCEINLINE void vfloat8::load (float a, float b, float c, float d,
                                    float e, float f, float g, float h) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_set_ps (h, g, f, e, d, c, b, a);
#else
    m_val[0] = a;
    m_val[1] = b;
    m_val[2] = c;
    m_val[3] = d;
    m_val[4] = e;
    m_val[5] = f;
    m_val[6] = g;
    m_val[7] = h;
#endif
}


OIIO_FORCEINLINE void vfloat8::load (const float *values) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_loadu_ps (values);
#elif OIIO_SIMD_SSE
    m_4[0] = vfloat4(values);
    m_4[1] = vfloat4(values+4);
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vfloat8::load (const float *values, int n) {
    OIIO_DASSERT (n >= 0 && n <= elements);
#if 0 && OIIO_AVX512VL_ENABLED
    // This SHOULD be fast, but in my benchmarks, it is slower!
    // (At least on the AVX512 hardware I have, Xeon Silver 4110.)
    // Re-test this periodically with new Intel hardware.
    m_simd = _mm256_maskz_loadu_ps ((~(0xff << n)), values);
#elif OIIO_SIMD_SSE
    if (n > 4) {
        vfloat4 lo, hi;
        lo.load (values);
        hi.load (values+4, n-4);
        m_4[0] = lo;
        m_4[1] = hi;
    } else {
        vfloat4 lo, hi;
        lo.load (values, n);
        hi.clear();
        m_4[0] = lo;
        m_4[1] = hi;
    }
#else
    for (int i = 0; i < n; ++i)
        m_val[i] = values[i];
    for (int i = n; i < paddedelements; ++i)
        m_val[i] = 0;
#endif
}


OIIO_FORCEINLINE void vfloat8::load (const unsigned short *values) {
#if OIIO_SIMD_AVX
    // Rely on the ushort->int conversion, then convert to float
    m_simd = _mm256_cvtepi32_ps (vint8(values).simd());
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vfloat8::load (const short *values) {
#if OIIO_SIMD_AVX
    // Rely on the short->int conversion, then convert to float
    m_simd = _mm256_cvtepi32_ps (vint8(values).simd());
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vfloat8::load (const unsigned char *values) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_cvtepi32_ps (vint8(values).simd());
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}


OIIO_FORCEINLINE void vfloat8::load (const char *values) {
#if OIIO_SIMD_AVX
    m_simd = _mm256_cvtepi32_ps (vint8(values).simd());
#else
    SIMD_CONSTRUCT (values[i]);
#endif
}

#ifdef _HALF_H_
OIIO_FORCEINLINE void vfloat8::load (const half *values) {
#if OIIO_SIMD_AVX && OIIO_F16C_ENABLED
    /* Enabled 16 bit float instructions! */
    vint4 a ((const int *)values);
    m_simd = _mm256_cvtph_ps (a);
#elif OIIO_SIMD_SSE >= 2
    m_4[0] = vfloat4(values);
    m_4[1] = vfloat4(values+4);
#else /* No SIMD defined: */
    SIMD_CONSTRUCT (values[i]);
#endif
}
#endif /* _HALF_H_ */


OIIO_FORCEINLINE void vfloat8::store (float *values) const {
#if OIIO_SIMD_AVX
    // Use an unaligned store -- it's just as fast when the memory turns
    // out to be aligned, nearly as fast even when unaligned. Not worth
    // the headache of using stores that require alignment.
    _mm256_storeu_ps (values, m_simd);
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}


OIIO_FORCEINLINE void vfloat8::store (float *values, int n) const {
    OIIO_DASSERT (n >= 0 && n <= elements);
#if 0 && OIIO_AVX512VL_ENABLED
    // This SHOULD be fast, but in my benchmarks, it is slower!
    // (At least on the AVX512 hardware I have, Xeon Silver 4110.)
    // Re-test this periodically with new Intel hardware.
    _mm256_mask_storeu_ps (values,  __mmask8(~(0xff << n)), m_simd);
#elif OIIO_SIMD_SSE
    if (n <= 4) {
        lo().store (values, n);
    } else if (n <= 8) {
        lo().store (values);
        hi().store (values+4, n-4);
    }
#else
    for (int i = 0; i < n; ++i)
        values[i] = m_val[i];
#endif
}

#ifdef _HALF_H_
OIIO_FORCEINLINE void vfloat8::store (half *values) const {
#if OIIO_SIMD_AVX && OIIO_F16C_ENABLED
    __m128i h = _mm256_cvtps_ph (m_simd, (_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC));
    _mm_storeu_si128 ((__m128i *)values, h);
#else
    SIMD_DO (values[i] = m_val[i]);
#endif
}
#endif


OIIO_FORCEINLINE void vfloat8::load_mask (int mask, const float *values) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm256_maskz_loadu_ps (__mmask8(mask), (const simd_t *)values);
#elif OIIO_SIMD_AVX
    m_simd = _mm256_maskload_ps (values, _mm256_castps_si256(vbool8::from_bitmask(mask)));
#else
    SIMD_CONSTRUCT ((mask>>i) & 1 ? values[i] : 0.0f);
#endif
}


OIIO_FORCEINLINE void vfloat8::load_mask (const vbool8& mask, const float *values) {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    m_simd = _mm256_maskz_loadu_ps (__mmask8(mask.bitmask()), (const simd_t *)values);
#elif OIIO_SIMD_AVX
    m_simd = _mm256_maskload_ps (values, _mm256_castps_si256(mask));
#else
    SIMD_CONSTRUCT (mask[i] ? values[i] : 0.0f);
#endif
}


OIIO_FORCEINLINE void vfloat8::store_mask (int mask, float *values) const {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm256_mask_storeu_ps (values, __mmask8(mask), m_simd);
#elif OIIO_SIMD_AVX
    _mm256_maskstore_ps (values, _mm256_castps_si256(vbool8::from_bitmask(mask)), m_simd);
#else
    SIMD_DO (if ((mask>>i) & 1) values[i] = (*this)[i]);
#endif
}


OIIO_FORCEINLINE void vfloat8::store_mask (const vbool8& mask, float *values) const {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm256_mask_storeu_ps (values, __mmask8(mask.bitmask()), m_simd);
#elif OIIO_SIMD_AVX
    _mm256_maskstore_ps (values, _mm256_castps_si256(mask.simd()), m_simd);
#else
    SIMD_DO (if (mask[i]) values[i] = (*this)[i]);
#endif
}


template <int scale>
OIIO_FORCEINLINE void
vfloat8::gather (const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm256_i32gather_ps (baseptr, vindex, scale);
#else
    SIMD_CONSTRUCT (*(const value_t *)((const char *)baseptr + vindex[i]*scale));
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vfloat8::gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 2
    m_simd = _mm256_mask_i32gather_ps (m_simd, baseptr, vindex, mask, scale);
#else
    SIMD_CONSTRUCT (mask[i] ? *(const value_t *)((const char *)baseptr + vindex[i]*scale) : 0);
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vfloat8::scatter (value_t *baseptr, const vint_t& vindex) const
{
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm256_i32scatter_ps (baseptr, vindex, m_simd, scale);
#else
    SIMD_DO (*(value_t *)((char *)baseptr + vindex[i]*scale) = m_val[i]);
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vfloat8::scatter_mask (const bool_t& mask, value_t *baseptr,
                       const vint_t& vindex) const
{
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    _mm256_mask_i32scatter_ps (baseptr, mask.bitmask(), vindex, m_simd, scale);
#else
    SIMD_DO (if (mask[i]) *(value_t *)((char *)baseptr + vindex[i]*scale) = m_val[i]);
#endif
}



OIIO_FORCEINLINE vfloat8 operator+ (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_add_ps (a, b);
#else
    return vfloat8 (a.lo()+b.lo(), a.hi()+b.hi());
#endif
}

OIIO_FORCEINLINE const vfloat8 & operator+= (vfloat8 & a, const vfloat8& b) {
    return a = a + b;
}

OIIO_FORCEINLINE vfloat8 operator- (const vfloat8& a) {
#if OIIO_SIMD_AVX
    return _mm256_sub_ps (_mm256_setzero_ps(), a);
#else
    return vfloat8 (-a.lo(), -a.hi());
#endif
}

OIIO_FORCEINLINE vfloat8 operator- (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_sub_ps (a, b);
#else
    return vfloat8 (a.lo()-b.lo(), a.hi()-b.hi());
#endif
}

OIIO_FORCEINLINE const vfloat8 & operator-= (vfloat8 & a, const vfloat8& b) {
    return a = a - b;
}

OIIO_FORCEINLINE vfloat8 operator* (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_mul_ps (a, b);
#else
    return vfloat8 (a.lo()*b.lo(), a.hi()*b.hi());
#endif
}

OIIO_FORCEINLINE const vfloat8 & operator*= (vfloat8 & a, const vfloat8& b) {
    return a = a * b;
}

OIIO_FORCEINLINE vfloat8 operator/ (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_div_ps (a, b);
#else
    return vfloat8 (a.lo()/b.lo(), a.hi()/b.hi());
#endif
}

OIIO_FORCEINLINE const vfloat8 & operator/= (vfloat8 & a, const vfloat8& b) {
    return a = a / b;
}

OIIO_FORCEINLINE vbool8 operator== (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_cmp_ps (a, b, _CMP_EQ_OQ);
#else
    return vbool8 (a.lo() == b.lo(), a.hi() == b.hi());
#endif
}

OIIO_FORCEINLINE vbool8 operator!= (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_cmp_ps (a, b, _CMP_NEQ_OQ);
#else
    return vbool8 (a.lo() != b.lo(), a.hi() != b.hi());
#endif
}

OIIO_FORCEINLINE vbool8 operator< (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_cmp_ps (a, b, _CMP_LT_OQ);
#else
    return vbool8 (a.lo() < b.lo(), a.hi() < b.hi());
#endif
}

OIIO_FORCEINLINE vbool8 operator>  (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_cmp_ps (a, b, _CMP_GT_OQ);
#else
    return vbool8 (a.lo() > b.lo(), a.hi() > b.hi());
#endif
}

OIIO_FORCEINLINE vbool8 operator>= (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_cmp_ps (a, b, _CMP_GE_OQ);
#else
    return vbool8 (a.lo() >= b.lo(), a.hi() >= b.hi());
#endif
}

OIIO_FORCEINLINE vbool8 operator<= (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_cmp_ps (a, b, _CMP_LE_OQ);
#else
    return vbool8 (a.lo() <= b.lo(), a.hi() <= b.hi());
#endif
}


// Implementation had to be after the definition of vfloat8.
OIIO_FORCEINLINE vint8::vint8 (const vfloat8& f)
{
#if OIIO_SIMD_AVX
    m_simd = _mm256_cvttps_epi32(f);
#else
    SIMD_CONSTRUCT ((int) f[i]);
#endif
}


template<int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7>
OIIO_FORCEINLINE vfloat8 shuffle (const vfloat8& a) {
#if OIIO_SIMD_AVX >= 2
    vint8 index (i0, i1, i2, i3, i4, i5, i6, i7);
    return _mm256_permutevar8x32_ps (a, index);
#else
    return vfloat8 (a[i0], a[i1], a[i2], a[i3], a[i4], a[i5], a[i6], a[i7]);
#endif
}

template<int i> OIIO_FORCEINLINE vfloat8 shuffle (const vfloat8& a) {
#if OIIO_SIMD_AVX >= 2
    return _mm256_permutevar8x32_ps (a, vint8(i));
#else
    return shuffle<i,i,i,i,i,i,i,i>(a);
#endif
}


template<int i>
OIIO_FORCEINLINE float extract (const vfloat8& v) {
#if OIIO_SIMD_AVX_NO_FIXME
    // Looks like the fastest we can do it is to extract a vfloat4,
    // shuffle its one element everywhere, then extract element 0.
    _m128 f4 = _mm256_extractf128_ps (i >> 2);
    int j = i & 3;
    return _mm_cvtss_f32(shuffle_sse<j,j,j,j>(a.simd()));
#else
    return v[i];
#endif
}


template<int i>
OIIO_FORCEINLINE vfloat8 insert (const vfloat8& a, float val) {
#if OIIO_SIMD_AVX_NO_FIXME
    return _mm256_insert_epi32 (a, val, i);
#else
    vfloat8 tmp = a;
    tmp[i] = val;
    return tmp;
#endif
}


OIIO_FORCEINLINE float vfloat8::x () const { return extract<0>(*this); }
OIIO_FORCEINLINE float vfloat8::y () const { return extract<1>(*this); }
OIIO_FORCEINLINE float vfloat8::z () const { return extract<2>(*this); }
OIIO_FORCEINLINE float vfloat8::w () const { return extract<3>(*this); }
OIIO_FORCEINLINE void vfloat8::set_x (float val) { *this = insert<0>(*this, val); }
OIIO_FORCEINLINE void vfloat8::set_y (float val) { *this = insert<1>(*this, val); }
OIIO_FORCEINLINE void vfloat8::set_z (float val) { *this = insert<2>(*this, val); }
OIIO_FORCEINLINE void vfloat8::set_w (float val) { *this = insert<3>(*this, val); }


OIIO_FORCEINLINE vint8 bitcast_to_int (const vfloat8& x)
{
#if OIIO_SIMD_AVX
    return _mm256_castps_si256 (x.simd());
#else
    return *(vint8 *)&x;
#endif
}

OIIO_FORCEINLINE vfloat8 bitcast_to_float (const vint8& x)
{
#if OIIO_SIMD_AVX
    return _mm256_castsi256_ps (x.simd());
#else
    return *(vfloat8 *)&x;
#endif
}


OIIO_FORCEINLINE vfloat8 vreduce_add (const vfloat8& v) {
#if OIIO_SIMD_AVX
    // From Syrah:
    vfloat8 ab_cd_0_0_ef_gh_0_0 = _mm256_hadd_ps(v.simd(), _mm256_setzero_ps());
    vfloat8 abcd_0_0_0_efgh_0_0_0 = _mm256_hadd_ps(ab_cd_0_0_ef_gh_0_0, _mm256_setzero_ps());
    // get efgh in the 0-idx slot
    vfloat8 efgh = shuffle<4>(abcd_0_0_0_efgh_0_0_0);
    vfloat8 final_sum = abcd_0_0_0_efgh_0_0_0 + efgh;
    return shuffle<0>(final_sum);
#else
    vfloat4 hadd4 = vreduce_add(v.lo()) + vreduce_add(v.hi());
    return vfloat8(hadd4, hadd4);
#endif
}


OIIO_FORCEINLINE float reduce_add (const vfloat8& v) {
#if OIIO_SIMD_AVX >= 2
    return extract<0>(vreduce_add(v));
#else
    return reduce_add(v.lo()) + reduce_add(v.hi());
#endif
}


OIIO_FORCEINLINE vfloat8 blend (const vfloat8& a, const vfloat8& b, const vbool8& mask)
{
#if OIIO_SIMD_AVX
    return _mm256_blendv_ps (a, b, mask);
#elif OIIO_SIMD_SSE
    return vfloat8 (blend (a.lo(), b.lo(), mask.lo()),
                   blend (a.hi(), b.hi(), mask.hi()));
#else
    SIMD_RETURN (vfloat8, mask[i] ? b[i] : a[i]);
#endif
}


OIIO_FORCEINLINE vfloat8 blend0 (const vfloat8& a, const vbool8& mask)
{
#if OIIO_SIMD_AVX
    return _mm256_and_ps(mask, a);
#elif OIIO_SIMD_SSE
    return vfloat8 (blend0 (a.lo(), mask.lo()),
                   blend0 (a.hi(), mask.hi()));
#else
    SIMD_RETURN (vfloat8, mask[i] ? a[i] : 0.0f);
#endif
}


OIIO_FORCEINLINE vfloat8 blend0not (const vfloat8& a, const vbool8& mask)
{
#if OIIO_SIMD_AVX
    return _mm256_andnot_ps(mask, a);
#elif OIIO_SIMD_SSE
    return vfloat8 (blend0not (a.lo(), mask.lo()),
                   blend0not (a.hi(), mask.hi()));
#else
    SIMD_RETURN (vfloat8, mask[i] ? 0.0f : a[i]);
#endif
}


OIIO_FORCEINLINE vfloat8 select (const vbool8& mask, const vfloat8& a, const vfloat8& b)
{
    return blend (b, a, mask);
}


OIIO_FORCEINLINE vfloat8 safe_div (const vfloat8 &a, const vfloat8 &b) {
#if OIIO_SIMD_SSE
    return blend0not (a/b, b == vfloat8::Zero());
#else
    SIMD_RETURN (vfloat8, b[i] == 0.0f ? 0.0f : a[i] / b[i]);
#endif
}


OIIO_FORCEINLINE vfloat8 abs (const vfloat8& a)
{
#if OIIO_SIMD_AVX
    // Just clear the sign bit for cheap fabsf
    return _mm256_and_ps (a.simd(), _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff)));
#else
    SIMD_RETURN (vfloat8, fabsf(a[i]));
#endif
}


OIIO_FORCEINLINE vfloat8 sign (const vfloat8& a)
{
    vfloat8 one(1.0f);
    return blend (one, -one, a < vfloat8::Zero());
}


OIIO_FORCEINLINE vfloat8 ceil (const vfloat8& a)
{
#if OIIO_SIMD_AVX
    return _mm256_ceil_ps (a);
#else
    SIMD_RETURN (vfloat8, ceilf(a[i]));
#endif
}

OIIO_FORCEINLINE vfloat8 floor (const vfloat8& a)
{
#if OIIO_SIMD_AVX
    return _mm256_floor_ps (a);
#else
    SIMD_RETURN (vfloat8, floorf(a[i]));
#endif
}

OIIO_FORCEINLINE vfloat8 round (const vfloat8& a)
{
#if OIIO_SIMD_AVX
    return _mm256_round_ps (a, (_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC));
#else
    SIMD_RETURN (vfloat8, roundf(a[i]));
#endif
}

OIIO_FORCEINLINE vint8 ifloor (const vfloat8& a)
{
    // FIXME: look into this, versus the method of quick_floor in texturesys.cpp
#if OIIO_SIMD_AVX
    return vint8(floor(a));
#elif OIIO_SIMD_SSE   /* SSE2/3 */
    return vint8 (ifloor(a.lo()), ifloor(a.hi()));
#else
    SIMD_RETURN (vint8, (int)floorf(a[i]));
#endif
}


OIIO_FORCEINLINE vint8 rint (const vfloat8& a)
{
    return vint8 (round(a));
}



OIIO_FORCEINLINE vfloat8 rcp_fast (const vfloat8 &a)
{
#if OIIO_SIMD_AVX512 && OIIO_AVX512VL_ENABLED
    vfloat8 r = _mm256_rcp14_ps(a);
    return r * nmadd(r,a,vfloat8(2.0f));
#elif OIIO_SIMD_AVX
    vfloat8 r = _mm256_rcp_ps(a);
    return r * nmadd(r,a,vfloat8(2.0f));
#else
    return vfloat8(rcp_fast(a.lo()), rcp_fast(a.hi()));
#endif
}


OIIO_FORCEINLINE vfloat8 sqrt (const vfloat8 &a)
{
#if OIIO_SIMD_AVX
    return _mm256_sqrt_ps (a.simd());
#else
    SIMD_RETURN (vfloat8, sqrtf(a[i]));
#endif
}



OIIO_FORCEINLINE vfloat8 rsqrt (const vfloat8 &a)
{
#if OIIO_SIMD_AVX
    return _mm256_div_ps (_mm256_set1_ps(1.0f), _mm256_sqrt_ps (a.simd()));
#else
    SIMD_RETURN (vfloat8, 1.0f/sqrtf(a[i]));
#endif
}



OIIO_FORCEINLINE vfloat8 rsqrt_fast (const vfloat8 &a)
{
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512ER_ENABLED
    // Trickery: in and out of the 512 bit registers to use fast approx rsqrt
    return _mm512_castps512_ps256(_mm512_rsqrt28_round_ps(_mm512_castps256_ps512(a), _MM_FROUND_NO_EXC));
#elif OIIO_SIMD_AVX >= 512 && OIIO_AVX512VL_ENABLED
    // Trickery: in and out of the 512 bit registers to use fast approx rsqrt
    return _mm512_castps512_ps256(_mm512_rsqrt14_ps(_mm512_castps256_ps512(a)));
#elif OIIO_SIMD_AVX
    return _mm256_rsqrt_ps (a.simd());
#elif OIIO_SIMD_SSE
    return vfloat8 (rsqrt_fast(a.lo()), rsqrt_fast(a.hi()));
#else
    SIMD_RETURN (vfloat8, 1.0f/sqrtf(a[i]));
#endif
}



OIIO_FORCEINLINE vfloat8 min (const vfloat8& a, const vfloat8& b)
{
#if OIIO_SIMD_AVX
    return _mm256_min_ps (a, b);
#else
    SIMD_RETURN (vfloat8, std::min (a[i], b[i]));
#endif
}

OIIO_FORCEINLINE vfloat8 max (const vfloat8& a, const vfloat8& b)
{
#if OIIO_SIMD_AVX
    return _mm256_max_ps (a, b);
#else
    SIMD_RETURN (vfloat8, std::max (a[i], b[i]));
#endif
}


OIIO_FORCEINLINE vfloat8 andnot (const vfloat8& a, const vfloat8& b) {
#if OIIO_SIMD_AVX
    return _mm256_andnot_ps (a.simd(), b.simd());
#else
    const int *ai = (const int *)&a;
    const int *bi = (const int *)&b;
    return bitcast_to_float (vint8(~(ai[0]) & bi[0],
                                  ~(ai[1]) & bi[1],
                                  ~(ai[2]) & bi[2],
                                  ~(ai[3]) & bi[3],
                                  ~(ai[4]) & bi[4],
                                  ~(ai[5]) & bi[5],
                                  ~(ai[6]) & bi[6],
                                  ~(ai[7]) & bi[7]));
#endif
}


OIIO_FORCEINLINE vfloat8 madd (const simd::vfloat8& a, const simd::vfloat8& b,
                              const simd::vfloat8& c)
{
#if OIIO_SIMD_AVX && OIIO_FMA_ENABLED
    // If we are sure _mm256_fmadd_ps intrinsic is available, use it.
    return _mm256_fmadd_ps (a, b, c);
#else
    // Fallback: just use regular math and hope for the best.
    return a * b + c;
#endif
}


OIIO_FORCEINLINE vfloat8 msub (const simd::vfloat8& a, const simd::vfloat8& b,
                              const simd::vfloat8& c)
{
#if OIIO_SIMD_AVX && OIIO_FMA_ENABLED
    // If we are sure _mm256_fnmsub_ps intrinsic is available, use it.
    return _mm256_fmsub_ps (a, b, c);
#else
    // Fallback: just use regular math and hope for the best.
    return a * b - c;
#endif
}



OIIO_FORCEINLINE vfloat8 nmadd (const simd::vfloat8& a, const simd::vfloat8& b,
                               const simd::vfloat8& c)
{
#if OIIO_SIMD_AVX && OIIO_FMA_ENABLED
    // If we are sure _mm256_fnmadd_ps intrinsic is available, use it.
    return _mm256_fnmadd_ps (a, b, c);
#else
    // Fallback: just use regular math and hope for the best.
    return c - a * b;
#endif
}



OIIO_FORCEINLINE vfloat8 nmsub (const simd::vfloat8& a, const simd::vfloat8& b,
                               const simd::vfloat8& c)
{
#if OIIO_SIMD_AVX && OIIO_FMA_ENABLED
    // If we are sure _mm256_fnmsub_ps intrinsic is available, use it.
    return _mm256_fnmsub_ps (a, b, c);
#else
    // Fallback: just use regular math and hope for the best.
    return -(a * b) - c;
#endif
}




//////////////////////////////////////////////////////////////////////
// vfloat16 implementation

OIIO_FORCEINLINE float& vfloat16::operator[] (int i) {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}

OIIO_FORCEINLINE float vfloat16::operator[] (int i) const {
    OIIO_DASSERT(i<elements);
    return m_val[i];
}


inline std::ostream& operator<< (std::ostream& cout, const vfloat16& val) {
    cout << val[0];
    for (int i = 1; i < val.elements; ++i)
        cout << ' ' << val[i];
    return cout;
}


OIIO_FORCEINLINE vfloat8 vfloat16::lo () const {
#if OIIO_SIMD_AVX >= 512
    return _mm512_castps512_ps256 (simd());
#else
    return m_8[0];
#endif
}

OIIO_FORCEINLINE vfloat8 vfloat16::hi () const {
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512DQ_ENABLED
    return _mm512_extractf32x8_ps (simd(), 1);
#else
    return m_8[1];
#endif
}


OIIO_FORCEINLINE vfloat16::vfloat16 (float v0, float v1, float v2, float v3,
                                   float v4, float v5, float v6, float v7,
                                   float v8, float v9, float v10, float v11,
                                   float v12, float v13, float v14, float v15) {
    load (v0, v1, v2, v3, v4, v5, v6, v7,
          v8, v9, v10, v11, v12, v13, v14, v15);
}

OIIO_FORCEINLINE vfloat16::vfloat16 (const vfloat8& lo, const vfloat8 &hi) {
#if OIIO_SIMD_AVX >= 512
    __m512 r = _mm512_castps256_ps512 (lo);
    m_simd = _mm512_insertf32x8 (r, hi, 1);
#else
    m_8[0] = lo;
    m_8[1] = hi;
#endif
}

OIIO_FORCEINLINE vfloat16::vfloat16 (const vfloat4 &a, const vfloat4 &b, const vfloat4 &c, const vfloat4 &d) {
#if OIIO_SIMD_AVX >= 512 
    m_simd = _mm512_broadcast_f32x4(a);
    m_simd = _mm512_insertf32x4 (m_simd, b, 1);
    m_simd = _mm512_insertf32x4 (m_simd, c, 2);
    m_simd = _mm512_insertf32x4 (m_simd, d, 3);
#else
    m_8[0] = vfloat8(a,b);
    m_8[1] = vfloat8(c,d);
#endif
}


OIIO_FORCEINLINE vfloat16::vfloat16 (const vint16& ival) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_cvtepi32_ps (ival);
#else
    SIMD_CONSTRUCT (float(ival[i]));
#endif
}


OIIO_FORCEINLINE const vfloat16 vfloat16::Zero () {
#if OIIO_SIMD_AVX >= 512
    return _mm512_setzero_ps();
#else
    return vfloat16(0.0f);
#endif
}

OIIO_FORCEINLINE const vfloat16 vfloat16::One () {
    return vfloat16(1.0f);
}

OIIO_FORCEINLINE const vfloat16 vfloat16::Iota (float start, float step) {
    return vfloat16 (start+0.0f*step, start+1.0f*step, start+2.0f*step, start+3.0f*step,
                    start+4.0f*step, start+5.0f*step, start+6.0f*step, start+7.0f*step,
                    start+8.0f*step, start+9.0f*step, start+10.0f*step, start+11.0f*step,
                    start+12.0f*step, start+13.0f*step, start+14.0f*step, start+15.0f*step);
}

/// Set all components to 0.0
OIIO_FORCEINLINE void vfloat16::clear () {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_setzero_ps();
#else
    load (0.0f);
#endif
}


OIIO_FORCEINLINE void vfloat16::load (float a) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_set1_ps (a);
#else
    m_8[0].load (a);
    m_8[1].load (a);
#endif
}


OIIO_FORCEINLINE void vfloat16::load (float v0, float v1, float v2, float v3,
                                     float v4, float v5, float v6, float v7,
                                     float v8, float v9, float v10, float v11,
                                     float v12, float v13, float v14, float v15) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_setr_ps (v0, v1, v2, v3, v4, v5, v6, v7,
                             v8, v9, v10, v11, v12, v13, v14, v15);
#else
    m_val[ 0] = v0;
    m_val[ 1] = v1;
    m_val[ 2] = v2;
    m_val[ 3] = v3;
    m_val[ 4] = v4;
    m_val[ 5] = v5;
    m_val[ 6] = v6;
    m_val[ 7] = v7;
    m_val[ 8] = v8;
    m_val[ 9] = v9;
    m_val[10] = v10;
    m_val[11] = v11;
    m_val[12] = v12;
    m_val[13] = v13;
    m_val[14] = v14;
    m_val[15] = v15;
#endif
}


OIIO_FORCEINLINE void vfloat16::load (const float *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_loadu_ps (values);
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}


OIIO_FORCEINLINE void vfloat16::load (const float *values, int n)
{
    OIIO_DASSERT (n >= 0 && n <= elements);
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_maskz_loadu_ps (__mmask16(~(0xffff << n)), values);
#else
    if (n > 8) {
        m_8[0].load (values);
        m_8[1].load (values+8, n-8);
    } else {
        m_8[0].load (values, n);
        m_8[1].clear ();
    }
#endif
}


OIIO_FORCEINLINE void vfloat16::load (const unsigned short *values) {
#if OIIO_SIMD_AVX >= 512
    // Rely on the ushort->int conversion, then convert to float
    m_simd = _mm512_cvtepi32_ps (vint16(values).simd());
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}


OIIO_FORCEINLINE void vfloat16::load (const short *values) {
#if OIIO_SIMD_AVX >= 512
    // Rely on the short->int conversion, then convert to float
    m_simd = _mm512_cvtepi32_ps (vint16(values).simd());
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}


OIIO_FORCEINLINE void vfloat16::load (const unsigned char *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_cvtepi32_ps (vint16(values).simd());
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}


OIIO_FORCEINLINE void vfloat16::load (const char *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_cvtepi32_ps (vint16(values).simd());
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}


#ifdef _HALF_H_
OIIO_FORCEINLINE void vfloat16::load (const half *values) {
#if OIIO_SIMD_AVX >= 512
    /* Enabled 16 bit float instructions! */
    vint8 a ((const int *)values);
    m_simd = _mm512_cvtph_ps (a);
#else
    m_8[0].load (values);
    m_8[1].load (values+8);
#endif
}
#endif /* _HALF_H_ */



OIIO_FORCEINLINE void vfloat16::store (float *values) const {
#if OIIO_SIMD_AVX >= 512
    // Use an unaligned store -- it's just as fast when the memory turns
    // out to be aligned, nearly as fast even when unaligned. Not worth
    // the headache of using stores that require alignment.
    _mm512_storeu_ps (values, m_simd);
#else
    m_8[0].store (values);
    m_8[1].store (values+8);
#endif
}


OIIO_FORCEINLINE void vfloat16::store (float *values, int n) const {
    OIIO_DASSERT (n >= 0 && n <= elements);
    // FIXME: is this faster with AVX masked stores?
#if 0 && OIIO_SIMD_AVX >= 512
    // This SHOULD be fast, but in my benchmarks, it is slower!
    // (At least on the AVX512 hardware I have, Xeon Silver 4110.)
    // Re-test this periodically with new Intel hardware.
    _mm512_mask_storeu_ps (values, __mmask16(~(0xffff << n)), m_simd);
#else
    if (n <= 8) {
        lo().store (values, n);
    } else if (n < 16) {
        lo().store (values);
        hi().store (values+8, n-8);
    } else {
        store (values);
    }
#endif
}

#ifdef _HALF_H_
OIIO_FORCEINLINE void vfloat16::store (half *values) const {
#if OIIO_SIMD_AVX >= 512
    __m256i h = _mm512_cvtps_ph (m_simd, (_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC));
    _mm256_storeu_si256 ((__m256i *)values, h);
#else
    m_8[0].store (values);
    m_8[1].store (values+8);
#endif
}
#endif


OIIO_FORCEINLINE void vfloat16::load_mask (const vbool16 &mask, const float *values) {
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_maskz_loadu_ps (mask, (const simd_t *)values);
#else
    m_8[0].load_mask (mask.lo(), values);
    m_8[1].load_mask (mask.hi(), values+8);
#endif
}


OIIO_FORCEINLINE void vfloat16::store_mask (const vbool16 &mask, float *values) const {
#if OIIO_SIMD_AVX >= 512
    _mm512_mask_storeu_ps (values, mask.bitmask(), m_simd);
#else
    lo().store_mask (mask.lo(), values);
    hi().store_mask (mask.hi(), values+8);
#endif
}



template <int scale>
OIIO_FORCEINLINE void
vfloat16::gather (const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_i32gather_ps (vindex, baseptr, scale);
#else
    m_8[0].gather<scale> (baseptr, vindex.lo());
    m_8[1].gather<scale> (baseptr, vindex.hi());
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vfloat16::gather_mask (const bool_t& mask, const value_t *baseptr, const vint_t& vindex)
{
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_mask_i32gather_ps (m_simd, mask, vindex, baseptr, scale);
#else
    m_8[0].gather_mask<scale> (mask.lo(), baseptr, vindex.lo());
    m_8[1].gather_mask<scale> (mask.hi(), baseptr, vindex.hi());
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vfloat16::scatter (value_t *baseptr, const vint_t& vindex) const
{
#if OIIO_SIMD_AVX >= 512
    _mm512_i32scatter_ps (baseptr, vindex, m_simd, scale);
#else
    lo().scatter<scale> (baseptr, vindex.lo());
    hi().scatter<scale> (baseptr, vindex.hi());
#endif
}

template<int scale>
OIIO_FORCEINLINE void
vfloat16::scatter_mask (const bool_t& mask, value_t *baseptr,
                        const vint_t& vindex) const
{
#if OIIO_SIMD_AVX >= 512
    _mm512_mask_i32scatter_ps (baseptr, mask, vindex, m_simd, scale);
#else
    lo().scatter_mask<scale> (mask.lo(), baseptr, vindex.lo());
    hi().scatter_mask<scale> (mask.hi(), baseptr, vindex.hi());
#endif
}



OIIO_FORCEINLINE vfloat16 operator+ (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_add_ps (a.m_simd, b.m_simd);
#else
    return vfloat16 (a.lo()+b.lo(), a.hi()+b.hi());
#endif
}

OIIO_FORCEINLINE const vfloat16 & operator+= (vfloat16& a, const vfloat16& b) {
    return a = a + b;
}

OIIO_FORCEINLINE vfloat16 operator- (const vfloat16& a) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_sub_ps (_mm512_setzero_ps(), a.simd());
#else
    return vfloat16 (-a.lo(), -a.hi());
#endif
}

OIIO_FORCEINLINE vfloat16 operator- (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_sub_ps (a.m_simd, b.m_simd);
#else
    return vfloat16 (a.lo()-b.lo(), a.hi()-b.hi());
#endif
}

OIIO_FORCEINLINE const vfloat16 & operator-= (vfloat16& a, const vfloat16& b) {
    return a = a - b;
}


OIIO_FORCEINLINE vfloat16 operator* (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_mul_ps (a.m_simd, b.m_simd);
#else
    return vfloat16 (a.lo()*b.lo(), a.hi()*b.hi());
#endif
}

OIIO_FORCEINLINE const vfloat16 & operator*= (vfloat16& a, const vfloat16& b) {
    return a = a * b;
}

OIIO_FORCEINLINE vfloat16 operator/ (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_div_ps (a.m_simd, b.m_simd);
#else
    return vfloat16 (a.lo()/b.lo(), a.hi()/b.hi());
#endif
}

OIIO_FORCEINLINE const vfloat16 & operator/= (vfloat16& a, const vfloat16& b) {
    return a = a / b;
}


OIIO_FORCEINLINE vbool16 operator== (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_ps_mask (a.simd(), b.simd(), _CMP_EQ_OQ);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() == b.lo(), a.hi() == b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator!= (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_ps_mask (a.simd(), b.simd(), _CMP_NEQ_OQ);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() != b.lo(), a.hi() != b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator< (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_ps_mask (a.simd(), b.simd(), _CMP_LT_OQ);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() < b.lo(), a.hi() < b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator> (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_ps_mask (a.simd(), b.simd(), _CMP_GT_OQ);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() > b.lo(), a.hi() > b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator>= (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_ps_mask (a.simd(), b.simd(), _CMP_GE_OQ);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() >= b.lo(), a.hi() >= b.hi());
#endif
}


OIIO_FORCEINLINE vbool16 operator<= (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_cmp_ps_mask (a.simd(), b.simd(), _CMP_LE_OQ);
#else  /* Fall back to 8-wide */
    return vbool16 (a.lo() <= b.lo(), a.hi() <= b.hi());
#endif
}


// Implementation had to be after the definition of vfloat16.
OIIO_FORCEINLINE vint16::vint16 (const vfloat16& f)
{
#if OIIO_SIMD_AVX >= 512
    m_simd = _mm512_cvttps_epi32(f);
#else
    *this = vint16 (vint8(f.lo()), vint8(f.hi()));
#endif
}



// Shuffle groups of 4
template<int i0, int i1, int i2, int i3>
vfloat16 shuffle4 (const vfloat16& a) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_shuffle_f32x4(a,a,_MM_SHUFFLE(i3,i2,i1,i0));
#else
    vfloat4 x[4];
    a.store ((float *)x);
    return vfloat16 (x[i0], x[i1], x[i2], x[i3]);
#endif
}

template<int i> vfloat16 shuffle4 (const vfloat16& a) {
    return shuffle4<i,i,i,i> (a);
}

template<int i0, int i1, int i2, int i3>
vfloat16 shuffle (const vfloat16& a) {
#if OIIO_SIMD_AVX >= 512
    return _mm512_permute_ps(a,_MM_SHUFFLE(i3,i2,i1,i0));
#else
    vfloat4 x[4];
    a.store ((float *)x);
    return vfloat16 (shuffle<i0,i1,i2,i3>(x[0]), shuffle<i0,i1,i2,i3>(x[1]),
                    shuffle<i0,i1,i2,i3>(x[2]), shuffle<i0,i1,i2,i3>(x[3]));
#endif
}

template<int i> vfloat16 shuffle (const vfloat16& a) {
    return shuffle<i,i,i,i> (a);
}


template<int i>
OIIO_FORCEINLINE float extract (const vfloat16& a) {
    return a[i];
}


template<int i>
OIIO_FORCEINLINE vfloat16 insert (const vfloat16& a, float val) {
    vfloat16 tmp = a;
    tmp[i] = val;
    return tmp;
}


OIIO_FORCEINLINE float vfloat16::x () const {
#if OIIO_SIMD_AVX >= 512
    return _mm_cvtss_f32(_mm512_castps512_ps128(m_simd));
#else
    return m_val[0];
#endif
}

OIIO_FORCEINLINE float vfloat16::y () const { return m_val[1]; }
OIIO_FORCEINLINE float vfloat16::z () const { return m_val[2]; }
OIIO_FORCEINLINE float vfloat16::w () const { return m_val[3]; }
OIIO_FORCEINLINE void vfloat16::set_x (float val) { m_val[0] = val; }
OIIO_FORCEINLINE void vfloat16::set_y (float val) { m_val[1] = val; }
OIIO_FORCEINLINE void vfloat16::set_z (float val) { m_val[2] = val; }
OIIO_FORCEINLINE void vfloat16::set_w (float val) { m_val[3] = val; }


OIIO_FORCEINLINE vint16 bitcast_to_int (const vfloat16& x)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_castps_si512 (x.simd());
#else
    return *(vint16 *)&x;
#endif
}

OIIO_FORCEINLINE vfloat16 bitcast_to_float (const vint16& x)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_castsi512_ps (x.simd());
#else
    return *(vfloat16 *)&x;
#endif
}


OIIO_FORCEINLINE vfloat16 vreduce_add (const vfloat16& v) {
#if OIIO_SIMD_AVX >= 512
    // Nomenclature: ABCD are the vint4's comprising v
    // First, add the vint4's and make them all the same
    vfloat16 AB_AB_CD_CD = v + shuffle4<1,0,3,2>(v);  // each adjacent vint4 is summed
    vfloat16 w = AB_AB_CD_CD + shuffle4<2,3,0,1>(AB_AB_CD_CD);
    // Now, add within each vint4
    vfloat16 ab_ab_cd_cd = w + shuffle<1,0,3,2>(w);  // each adjacent int is summed
    return ab_ab_cd_cd + shuffle<2,3,0,1>(ab_ab_cd_cd);
#else
    vfloat8 sum = vreduce_add(v.lo()) + vreduce_add(v.hi());
    return vfloat16 (sum, sum);
#endif
}


OIIO_FORCEINLINE float reduce_add (const vfloat16& v) {
#if OIIO_SIMD_AVX >= 512
    return vreduce_add(v).x();
#else
    return reduce_add(v.lo()) + reduce_add(v.hi());
#endif
}


OIIO_FORCEINLINE vfloat16 blend (const vfloat16& a, const vfloat16& b, const vbool16& mask)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_mask_blend_ps (mask, a, b);
#else
    return vfloat16 (blend (a.lo(), b.lo(), mask.lo()),
                    blend (a.hi(), b.hi(), mask.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 blend0 (const vfloat16& a, const vbool16& mask)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_maskz_mov_ps (mask, a);
#else
    return vfloat16 (blend0 (a.lo(), mask.lo()),
                    blend0 (a.hi(), mask.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 blend0not (const vfloat16& a, const vbool16& mask)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_maskz_mov_ps (!mask, a);
#else
    return vfloat16 (blend0not (a.lo(), mask.lo()),
                    blend0not (a.hi(), mask.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 select (const vbool16& mask, const vfloat16& a, const vfloat16& b)
{
    return blend (b, a, mask);
}


OIIO_FORCEINLINE vfloat16 safe_div (const vfloat16 &a, const vfloat16 &b) {
#if OIIO_SIMD_SSE
    return blend0not (a/b, b == vfloat16::Zero());
#else
    SIMD_RETURN (vfloat16, b[i] == 0.0f ? 0.0f : a[i] / b[i]);
#endif
}


OIIO_FORCEINLINE vfloat16 abs (const vfloat16& a)
{
#if OIIO_SIMD_AVX >= 512
    // Not available?  return _mm512_abs_ps (a.simd());
    // Just clear the sign bit for cheap fabsf
    return _mm512_castsi512_ps (_mm512_and_epi32 (_mm512_castps_si512(a.simd()),
                                                  _mm512_set1_epi32(0x7fffffff)));
#else
    return vfloat16(abs(a.lo()), abs(a.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 sign (const vfloat16& a)
{
    vfloat16 one(1.0f);
    return blend (one, -one, a < vfloat16::Zero());
}


OIIO_FORCEINLINE vfloat16 ceil (const vfloat16& a)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_ceil_ps (a);
#else
    return vfloat16(ceil(a.lo()), ceil(a.hi()));
#endif
}

OIIO_FORCEINLINE vfloat16 floor (const vfloat16& a)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_floor_ps (a);
#else
    return vfloat16(floor(a.lo()), floor(a.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 round (const vfloat16& a)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_roundscale_ps (a, (1<<4) | 3); // scale=1, round to nearest smaller mag int
#else
    return vfloat16(round(a.lo()), round(a.hi()));
#endif
}

OIIO_FORCEINLINE vint16 ifloor (const vfloat16& a)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_cvt_roundps_epi32 (a, (_MM_FROUND_TO_NEG_INF |_MM_FROUND_NO_EXC));
#else
    return vint16(floor(a));
#endif
}


OIIO_FORCEINLINE vint16 rint (const vfloat16& a)
{
    return vint16(round(a));
}


OIIO_FORCEINLINE vfloat16 rcp_fast (const vfloat16 &a)
{
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512ER_ENABLED
    return _mm512_rcp28_ps(a);
#elif OIIO_SIMD_AVX >= 512
    vfloat16 r = _mm512_rcp14_ps(a);
    return r * nmadd (r, a, vfloat16(2.0f));
#else
    return vfloat16(rcp_fast(a.lo()), rcp_fast(a.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 sqrt (const vfloat16 &a)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_sqrt_ps (a);
#else
    return vfloat16(sqrt(a.lo()), sqrt(a.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 rsqrt (const vfloat16 &a)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_div_ps (_mm512_set1_ps(1.0f), _mm512_sqrt_ps (a));
#else
    return vfloat16(rsqrt(a.lo()), rsqrt(a.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 rsqrt_fast (const vfloat16 &a)
{
#if OIIO_SIMD_AVX >= 512 && OIIO_AVX512ER_ENABLED
    return _mm512_rsqrt28_round_ps(a, _MM_FROUND_NO_EXC);
#elif OIIO_SIMD_AVX >= 512
    return _mm512_rsqrt14_ps (a);
#else
    return vfloat16(rsqrt_fast(a.lo()), rsqrt_fast(a.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 min (const vfloat16& a, const vfloat16& b)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_min_ps (a, b);
#else
    return vfloat16(min(a.lo(),b.lo()), min(a.hi(),b.hi()));
#endif
}

OIIO_FORCEINLINE vfloat16 max (const vfloat16& a, const vfloat16& b)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_max_ps (a, b);
#else
    return vfloat16(max(a.lo(),b.lo()), max(a.hi(),b.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 andnot (const vfloat16& a, const vfloat16& b) {
#if OIIO_SIMD_AVX >= 512 && defined(__AVX512DQ__)
    return _mm512_andnot_ps (a, b);
#else
    return vfloat16(andnot(a.lo(),b.lo()), andnot(a.hi(),b.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 madd (const simd::vfloat16& a, const simd::vfloat16& b,
                               const simd::vfloat16& c)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_fmadd_ps (a, b, c);
#else
    return vfloat16 (madd(a.lo(), b.lo(), c.lo()),
                    madd(a.hi(), b.hi(), c.hi()));
#endif
}


OIIO_FORCEINLINE vfloat16 msub (const simd::vfloat16& a, const simd::vfloat16& b,
                               const simd::vfloat16& c)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_fmsub_ps (a, b, c);
#else
    return vfloat16 (msub(a.lo(), b.lo(), c.lo()),
                    msub(a.hi(), b.hi(), c.hi()));
#endif
}



OIIO_FORCEINLINE vfloat16 nmadd (const simd::vfloat16& a, const simd::vfloat16& b,
                                const simd::vfloat16& c)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_fnmadd_ps (a, b, c);
#else
    return vfloat16 (nmadd(a.lo(), b.lo(), c.lo()),
                    nmadd(a.hi(), b.hi(), c.hi()));
#endif
}



OIIO_FORCEINLINE vfloat16 nmsub (const simd::vfloat16& a, const simd::vfloat16& b,
                                const simd::vfloat16& c)
{
#if OIIO_SIMD_AVX >= 512
    return _mm512_fnmsub_ps (a, b, c);
#else
    return vfloat16 (nmsub(a.lo(), b.lo(), c.lo()),
                    nmsub(a.hi(), b.hi(), c.hi()));
#endif
}




} // end namespace simd

OIIO_NAMESPACE_END


#undef SIMD_DO
#undef SIMD_CONSTRUCT
#undef SIMD_CONSTRUCT_PAD
#undef SIMD_RETURN
#undef SIMD_RETURN_REDUCE
