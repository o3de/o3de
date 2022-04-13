/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/variadic.h>
#include <AzCore/PlatformDef.h> ///< Platform/compiler specific defines
#include <AzCore/base_Platform.h>

#include <AzCore/AzCore_Traits_Platform.h>

/// Return an array size for static arrays.
namespace AZ::Internal
{
    template<class T>
    struct StaticArraySize
    {
        static_assert(std::is_array_v<T>, "AZ_ARRAY_SIZE can only be used with a C-style array");
        static constexpr size_t value = sizeof(T) / sizeof(std::remove_extent_t<T>);
    };
}
#define AZ_ARRAY_SIZE(__a)  AZ::Internal::StaticArraySize<std::remove_reference_t<decltype(__a)>>::value


#ifndef AZ_SIZE_ALIGN_UP
/// Align to the next bigger/up size
#   define AZ_SIZE_ALIGN_UP(_size, _align)       ((_size+(_align-1)) & ~(_align-1))
#endif // AZ_SIZE_ALIGN_UP
#ifndef AZ_SIZE_ALIGN_DOWN
/// Align to the next smaller/down size
#   define AZ_SIZE_ALIGN_DOWN(_size, _align)     ((_size) & ~(_align-1))
#endif // AZ_SIZE_ALIGN_DOWN
#ifndef AZ_SIZE_ALIGN
/// Same as AZ_SIZE_ALIGN_UP
    #define AZ_SIZE_ALIGN(_size, _align)         AZ_SIZE_ALIGN_UP(_size, _align)
#endif // AZ_SIZE_ALIGN

#define AZ_JOIN(X, Y) AZSTD_DO_JOIN(X, Y)
#define AZSTD_DO_JOIN(X, Y) AZSTD_DO_JOIN2(X, Y)
#define AZSTD_DO_JOIN2(X, Y) X##Y

/**
 * Macros for calling into strXXX functions. These are simple wrappers that call into the platform
 * implementation. Definitions provide inputs for destination size to allow calling to strXXX_s functions
 * on supported platforms. With that said there are no guarantees that this size will be respected on all of them.
 * The main goal here is to help devs avoid #ifdefs when calling to platform API. The goal here is wrap only the function
 * that differ, otherwise you should call directly into functions like strcmp, strstr, etc.
 * If you want guranteed "secure" functions, you should not use those wrappers. In general we recommend using AZStd::string/wstring for manipulating strings.
 */
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
#   define azsnprintf(_buffer, _size, ...)                  _snprintf_s(_buffer, _size, _size-1, __VA_ARGS__)
#   define azvsnprintf(_buffer, _size, _format, _va_list)   _vsnprintf_s(_buffer, _size, _size-1, _format, _va_list)
#   define azsnwprintf(_buffer, _size, ...)                 _snwprintf_s(_buffer, _size, _size-1, __VA_ARGS__)
#   define azvsnwprintf(_buffer, _size, _format, _va_list)  _vsnwprintf_s(_buffer, _size, _size-1, _format, _va_list)
#   define azscprintf                                       _scprintf
#   define azvscprintf(_format, _va_list)                   _vscprintf(_format, _va_list)
#   define azscwprintf                                      _scwprintf
#   define azvscwprintf                                     _vscwprintf
#   define azstrtok(_buffer, _size, _delim, _context)       strtok_s(_buffer, _delim, _context)
#   define azstrcat(_dest, _destSize, _src)                 strcat_s(_dest, _destSize, _src)
#   define azstrncat(_dest, _destSize, _src, _count)        strncat_s(_dest, _destSize, _src, _count)
#   define strtoll                                          _strtoi64
#   define strtoull                                         _strtoui64
#   define azsscanf                                         sscanf_s
#   define azstrcpy(_dest, _destSize, _src)                 strcpy_s(_dest, _destSize, _src)
#   define azstrncpy(_dest, _destSize, _src, _count)        strncpy_s(_dest, _destSize, _src, _count)
#   define azstricmp                                        _stricmp
#   define azstrnicmp                                       _strnicmp
#   define azisfinite                                       _finite
#   define azltoa                                           _ltoa_s
#   define azitoa                                           _itoa_s
#   define azui64toa                                        _ui64toa_s
#   define azswscanf                                        swscanf_s
#   define azwcsicmp                                        _wcsicmp
#   define azwcsnicmp                                       _wcsnicmp

// note: for cross-platform compatibility, do not use the return value of azfopen. On Windows, it's an errno_t and 0 indicates success. On other platforms, the return value is a FILE*, and a 0 value indicates failure.
#   define azfopen(_fp, _filename, _attrib)                 fopen_s(_fp, _filename, _attrib)
#   define azfscanf                                         fscanf_s

#   define azsprintf(_buffer, ...)                          sprintf_s(_buffer, AZ_ARRAY_SIZE(_buffer), __VA_ARGS__)
#   define azstrlwr                                         _strlwr_s
#   define azvsprintf                                       vsprintf_s
#   define azwcscpy                                         wcscpy_s
#   define azstrtime                                        _strtime_s
#   define azstrdate                                        _strdate_s
#   define azlocaltime(time, result)                        localtime_s(result, time)
#else
#   define azsnprintf                                       snprintf
#   define azvsnprintf                                      vsnprintf
#   if AZ_TRAIT_COMPILER_DEFINE_AZSWNPRINTF_AS_SWPRINTF
#       define azsnwprintf                                  swprintf
#       define azvsnwprintf                                 vswprintf
#   else
#       define azsnwprintf                                  snwprintf
#       define azvsnwprintf                                 vsnwprintf
#   endif
#   define azscprintf(...)                                  azsnprintf(nullptr, static_cast<size_t>(0), __VA_ARGS__);
#   define azvscprintf(_format, _va_list)                   azvsnprintf(nullptr, static_cast<size_t>(0), _format, _va_list);
#   define azscwprintf(...)                                 azsnwprintf(nullptr, static_cast<size_t>(0), __VA_ARGS__);
#   define azvscwprintf(_format, _va_list)                  azvsnwprintf(nullptr, static_cast<size_t>(0), _format, _va_list);
#   define azstrtok(_buffer, _size, _delim, _context)       strtok(_buffer, _delim)
#   define azstrcat(_dest, _destSize, _src)                 strcat(_dest, _src)
#   define azstrncat(_dest, _destSize, _src, _count)        strncat(_dest, _src, _count)
#   define azsscanf                                         sscanf
#   define azstrcpy(_dest, _destSize, _src)                 strcpy(_dest, _src)
#   define azstrncpy(_dest, _destSize, _src, _count)        strncpy(_dest, _src, _count)
#   define azstricmp                                        strcasecmp
#   define azstrnicmp                                       strncasecmp
#   if defined(NDK_REV_MAJOR) && NDK_REV_MAJOR < 16
#       define azisfinite                                   __isfinitef
#   else
#       define azisfinite                                   isfinite
#   endif
#   define azltoa(_value, _buffer, _size, _radix)           ltoa(_value, _buffer, _radix)
#   define azitoa(_value, _buffer, _size, _radix)           itoa(_value, _buffer, _radix)
#   define azui64toa(_value, _buffer, _size, _radix)        _ui64toa(_value, _buffer, _radix)
#   define azswscanf                                        swscanf
#   define azwcsicmp                                        wcscasecmp
#   define azwcsnicmp                                       wcsnicmp

// note: for cross-platform compatibility, do not use the return value of azfopen. On Windows, it's an errno_t and 0 indicates success. On other platforms, the return value is a FILE*, and a 0 value indicates failure.
#   define azfopen(_fp, _filename, _attrib)                 *(_fp) = fopen(_filename, _attrib)
#   define azfscanf                                         fscanf

#   define azsprintf                                        sprintf
#   define azstrlwr(_buffer, _size)                         strlwr(_buffer)
#   define azvsprintf                                       vsprintf
#   define azwcscpy(_dest, _size, _buffer)                  wcscpy(_dest, _buffer)
#   define azstrtime                                        _strtime
#   define azstrdate                                        _strdate
#   define azlocaltime                                      localtime_r
#endif

#if AZ_TRAIT_USE_POSIX_STRERROR_R
#   define azstrerror_s(_dst, _num, _err)                   strerror_r(_err, _dst, _num)
#else
#   define azstrerror_s                                     strerror_s
#endif

#define AZ_INVALID_POINTER  reinterpret_cast<void*>(0x0badf00dul)

//////////////////////////////////////////////////////////////////////////

#include <cstddef> // the macros NULL and offsetof as well as the types ptrdiff_t, wchar_t, and size_t.

// bring to global namespace, this is what string.h does anyway
using std::size_t;
using std::ptrdiff_t;

/**
 * Data types with specific length. They should be used ONLY when we serialize data if
 * they have native type equivalent, which should take precedence.
 */

#include <cstdint>

namespace AZ
{
    typedef int8_t    s8;
    typedef uint8_t   u8;
    typedef int16_t   s16;
    typedef uint16_t  u16;
    typedef int32_t   s32;
    typedef uint32_t  u32;
#   if AZ_TRAIT_COMPILER_INT64_T_IS_LONG // int64_t is long
    typedef signed long long        s64;
    typedef unsigned long long      u64;
#   else
    typedef int64_t   s64;
    typedef uint64_t  u64;
#   endif //


    typedef struct
    {
        s64 a, b;
    } s128;
    typedef struct
    {
        u64 a, b;
    } u128;

    template<typename T>
    inline T SizeAlignUp(T s, size_t a) { return static_cast<T>((s+(a-1)) & ~(a-1)); }

    template<typename T>
    inline T SizeAlignDown(T s, size_t a) { return static_cast<T>((s) & ~(a-1)); }

    template<typename T>
    inline T* PointerAlignUp(T* p, size_t a)    { return reinterpret_cast<T*>((reinterpret_cast<size_t>(p)+(a-1)) & ~(a-1));    }

    template<typename T>
    inline T* PointerAlignDown(T* p, size_t a) { return reinterpret_cast<T*>((reinterpret_cast<size_t>(p)) & ~(a-1));   }

    /**
    * Does an safe alias cast using a union. This will allow you to properly cast types that when
    * strict aliasing is enabled.
    */
    template<typename T, typename S>
    T AliasCast(S source)
    {
        static_assert(sizeof(T) == sizeof(S), "Source and target should be the same size!");
        union
        {
            S source;
            T target;
        } cast;
        cast.source = source;
        return cast.target;
    }
}

#include <AzCore/Debug/Trace.h> // Global access to AZ_Assert, AZ_Error, AZ_Warning, etc.

// \ref AZ::AliasCast
#define azalias_cast AZ::AliasCast

// Macros to disable the auto-generated copy/move constructors and assignment operators for a class
// Note: AZ_DISABLE_COPY must also set the move constructor and move assignment operator
// to `=default`, otherwise they will be implicitly disabled by the compiler.
// If you wish to implement your own move constructor and assignment operator, you must
// explicitly delete the copy and assignment operator without the use of this macro.
#define AZ_DISABLE_COPY(_Class) \
    _Class(const _Class&) = delete; _Class& operator=(const _Class&) = delete; \
    _Class(_Class&&) = default; _Class& operator=(_Class&&) = default;
// Note: AZ_DISABLE_MOVE is DEPRECATED and will be removed.
// The preferred approach to use if a type is to be copy only is to simply provide copy and
// assignment operators (either `=default` or user provided), moves will be implicitly disabled.
#define AZ_DISABLE_MOVE(_Class) \
    _Class(_Class&&) = delete; _Class& operator=(_Class&&) = delete;
// Note: Setting the move constructor and move assignment operator to `=delete` here is not
// strictly necessary (as they will be implicitly disabled when the copy/assignment operators
// are declared) but is done to be explicit that this is the intention.
#define AZ_DISABLE_COPY_MOVE(_Class) \
    _Class(const _Class&) = delete; _Class& operator=(const _Class&) = delete; AZ_DISABLE_MOVE(_Class)

// Macros to default the auto-generated copy/move constructors and assignment operators for a class
#define AZ_DEFAULT_COPY(_Class) _Class(const _Class&) = default; _Class& operator=(const _Class&) = default;
#define AZ_DEFAULT_MOVE(_Class) _Class(_Class&&) = default; _Class& operator=(_Class&&) = default;
#define AZ_DEFAULT_COPY_MOVE(_Class) AZ_DEFAULT_COPY(_Class) AZ_DEFAULT_MOVE(_Class)

// Macro that can be used to avoid unreferenced variable warnings
#define AZ_UNUSED_1(x)                                          (void)(x);
#define AZ_UNUSED_2(x1, x2)                                     AZ_UNUSED_1(x1)                     AZ_UNUSED_1(x2)
#define AZ_UNUSED_3(x1, x2, x3)                                 AZ_UNUSED_1(x1)                     AZ_UNUSED_2(x2, x3)
#define AZ_UNUSED_4(x1, x2, x3, x4)                             AZ_UNUSED_2(x1, x2)                 AZ_UNUSED_2(x3, x4)
#define AZ_UNUSED_5(x1, x2, x3, x4, x5)                         AZ_UNUSED_2(x1, x2)                 AZ_UNUSED_3(x3, x4, x5)
#define AZ_UNUSED_6(x1, x2, x3, x4, x5, x6)                     AZ_UNUSED_3(x1, x2, x3)             AZ_UNUSED_3(x4, x5, x6)
#define AZ_UNUSED_7(x1, x2, x3, x4, x5, x6, x7)                 AZ_UNUSED_3(x1, x2, x3)             AZ_UNUSED_4(x4, x5, x6, x7)
#define AZ_UNUSED_8(x1, x2, x3, x4, x5, x6, x7, x8)             AZ_UNUSED_4(x1, x2, x3, x4)         AZ_UNUSED_4(x5, x6, x7, x8)
#define AZ_UNUSED_9(x1, x2, x3, x4, x5, x6, x7, x8, x9)         AZ_UNUSED_4(x1, x2, x3, x4)         AZ_UNUSED_5(x5, x6, x7, x8, x9)
#define AZ_UNUSED_10(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10)   AZ_UNUSED_5(x1, x2, x3, x4, x5)     AZ_UNUSED_5(x6, x7, x8, x9, x10)
#define AZ_UNUSED_11(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11)   AZ_UNUSED_5(x1, x2, x3, x4, x5)     AZ_UNUSED_6(x6, x7, x8, x9, x10, x11)
#define AZ_UNUSED_12(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12)   AZ_UNUSED_6(x1, x2, x3, x4, x5, x6)     AZ_UNUSED_6(x7, x8, x9, x10, x11, x12)

#define AZ_UNUSED(...) AZ_MACRO_SPECIALIZE(AZ_UNUSED_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

#define AZ_DEFINE_ENUM_BITWISE_OPERATORS(EnumType) \
inline constexpr EnumType operator | (EnumType a, EnumType b) \
    { return EnumType(((AZStd::underlying_type<EnumType>::type)a) | ((AZStd::underlying_type<EnumType>::type)b)); } \
inline constexpr EnumType& operator |= (EnumType &a, EnumType b) \
    { return a = a | b; } \
inline constexpr EnumType operator & (EnumType a, EnumType b) \
    { return EnumType(((AZStd::underlying_type<EnumType>::type)a) & ((AZStd::underlying_type<EnumType>::type)b)); } \
inline constexpr EnumType& operator &= (EnumType &a, EnumType b) \
    { return a = a & b; } \
inline constexpr EnumType operator ~ (EnumType a) \
    { return EnumType(~((AZStd::underlying_type<EnumType>::type)a)); } \
inline constexpr EnumType operator ^ (EnumType a, EnumType b) \
    { return EnumType(((AZStd::underlying_type<EnumType>::type)a) ^ ((AZStd::underlying_type<EnumType>::type)b)); } \
inline constexpr EnumType& operator ^= (EnumType &a, EnumType b) \
    { return a = a ^ b; }
