/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

// Skip the error as we are including the recommended way.
#define RAPIDJSON_SKIP_AZCORE_ERROR

#if defined(AZ_ENABLE_TRACING)
#   define RAPIDJSON_ASSERT(x) AZ_Assert(x, "Assert[rapidjson]: " #x)
#else
#   define RAPIDJSON_ASSERT(x) (void)(0)
#endif
#define RAPIDJSON_STATIC_ASSERT(x) static_assert(x, "Assert[rapidjson]: " #x)
#define RAPIDJSON_HAS_CXX11_TYPETRAITS 1
#define RAPIDJSON_HAS_CXX11_RVALUE_REFS 1

namespace rapidjson_ly_internal
{
    template<typename T>
    void Delete(T* instance)
    {
        if (instance)
        {
            instance->~T();
            azfree(instance, AZ::SystemAllocator);
        }
    }
}


#define RAPIDJSON_NEW(x)  new(azmalloc(sizeof(x), alignof(x), AZ::SystemAllocator, "RapidJSON")) x
#define RAPIDJSON_DELETE(x) rapidjson_ly_internal::Delete(x)
#define RAPIDJSON_MALLOC(_size) AZ::AllocatorInstance<AZ::SystemAllocator>::Get().allocate(_size, 16)
#define RAPIDJSON_REALLOC(_ptr, _newSize) _ptr ? AZ::AllocatorInstance<AZ::SystemAllocator>::Get().reallocate(_ptr, _newSize, 16) : RAPIDJSON_MALLOC(_newSize)
#define RAPIDJSON_FREE(_ptr) if (_ptr) { AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(_ptr, 0, 0); }
#define RAPIDJSON_CLASS_ALLOCATOR(_class) AZ_CLASS_ALLOCATOR(_class, AZ::SystemAllocator, 0)

// By default, RapidJSON uses its own pooling allocator that allocates in 64k chunks and then uses the
// contents of those chunks internally.
// However, O3DE defines the above macros, which redirect the RapidJSON allocations into the O3DE system
// allocator, which itself is a also a chunk-based pooling allocator (HPHA).
// This double-chunking would be wasteful, so tell RapidJSON to directly ask for allocations instead
// of attempting to pool them.  The rapidjson::CrtAllocator just passes allocation and deallocation to the above
// macros.
#define RAPIDJSON_DEFAULT_ALLOCATOR CrtAllocator

// Set custom namespace for AzCore's rapidjson to avoid various collisions.
#define RAPIDJSON_NAMESPACE rapidjson_ly

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#endif

// Detect what is available in the compiler and enable those features in RapidJSON.
// Note that RapidJSON will use the combination of any of these to determine its final
// set of instructions to use, so its best to set all that are applicable:
#if defined(__SSE4_2__)
#define RAPIDJSON_SSE42
#endif

#if defined(__SSE2__)
#define RAPIDJSON_SSE2
#endif

#if defined(__ARM_NEON__) || defined(__ARM_NEON) // older compilers define __ARM_NEON
#define RAPIDJSON_NEON
#endif

#if defined(AZ_COMPILER_MSVC)
    // windows defines may or may not be present for unity builds, so ensure StrCmp resolves to StrCmpW to avoid conflicts
    #define StrCmp StrCmpW

    // MSVC compiler does not necessarily specify any of the above macros.
    // if we're compiling for a X64 target we can target SSE2.x at the very least.
    #if defined(_M_AMD64)
        #define RAPIDJSON_SSE2
    #endif
#endif

// Now that all of the above is declared, bring the RapidJSON headers in.
// If you add additional definitions or configuration options, add them above.
#include <rapidjson/rapidjson.h>

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

// Allow our existing code to continue use "rapidjson::"
#define rapidjson RAPIDJSON_NAMESPACE
