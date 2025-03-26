/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

#if defined(RAPIDJSON_RAPIDJSON_H_)
// if this happens, someone has already included the default rapidjson.h already, which is a bad idea as it won't
// include any customizations such as memory management.
#error vanilla rapidjson was included before including <AzCore/JSON/rapidjson.h>
#endif

// Only override RAPIDJSON_ customization macros which are not already defined.
// This allows the user to pre-define these before including O3DE if they want to use default values
// or their own values.

#if !defined(RAPIDJSON_ASSERT)
#   if defined(AZ_ENABLE_TRACING)
#       define RAPIDJSON_ASSERT(x) AZ_Assert(x, "Assert[rapidjson]: " #x)
#   else
#       define RAPIDJSON_ASSERT(x) (void)(0)
#   endif
#endif

#if !defined(RAPIDJSON_STATIC_ASSERT)
   #define RAPIDJSON_STATIC_ASSERT(x) static_assert(x, "Assert[rapidjson]: " #x)
#endif

#if !defined(RAPIDJSON_HAS_CXX11_TYPETRAITS)
#   define RAPIDJSON_HAS_CXX11_TYPETRAITS 1
#endif

#if !defined(RAPIDJSON_HAS_CXX11_RVALUE_REFS)
#   define RAPIDJSON_HAS_CXX11_RVALUE_REFS 1
#endif

// note for future maintainers:
// RapidJSON allows you to customize the default allocator type for documents by overriding with RAPIDJSON_DEFAULT_ALLOCATOR.
// However, customizing the default type of rapidjson::Document and rapidjson::Value in that way will cause incompatibilities
// with external software libraries that use unmodified RapidJSON.   It is not thus recommended
// to actually set the value of RAPIDJSON_DEFAULT_ALLOCATOR or redefine what a rapidjson::Document itself is.
// The other way to get a 'custom' allocator installed in RapidJSON is to make your own Document/Value/Pointer type, since it's templated
// on allocator type - for example, implement your own allocator and do
// using MyDocument = rapidjson::GenericDocument<rapidjson::UTF8<char>, MyAllocator, MyAllocator> and then use MyDocument in your code
// instead of RapidJSON::Document.  Note GenericDocument<> customized must use customized GenericValue<> and GenericPointer<>
// or else you haven't actually plugged into all the places where memory is allocated and will lose some.
// 
// Unfortunately, rapidjson::Pointer can't deal with these customized values or documents due to a bug in the code where functions
// like Set/Get/Create use the default template parameter instead of deducing it:
// ie, they are set up like somefunction<T>(GenericValue<char> val) which will only accept default GenericValue<char, CrtAllocator>.
// correct form: somefunction<T>(GenericValue<char, T::AllocatorType> val, which would allow MyValue<char, MyAllocator> to be passed in.
// thus, it is not recommended to use customized documents, values, or pointers either, (until that bug is fixed).

// this leaves just one avenue for customization of memory of Rapidjson - overriding the macros RAPIDJSON_xxxx (new,delete, malloc, realloc..)

// forward all allocations to our own allocators.  But preferably without actually having to drag our allocators into this header:

// note that we cannot partially define allocation forwarding - for example, it is not okay to just override new, but not delete.
// So either we define them all, or we define none.  If a user has defined any of them, we don't define any of them:
// Users can manually switch the entire system off by setting USING_AZCORE_RAPIDJSON_ALLOCATION_OVERRIDES 0

#if !defined(USING_AZCORE_RAPIDJSON_ALLOCATION_OVERRIDES)
#   if  !defined(RAPIDJSON_MALLOC) && !defined(RAPIDJSON_REALLOC) && !defined(RAPIDJSON_FREE) && !defined(RAPIDJSON_NEW) && !defined(RAPIDJSON_DELETE)
#       define USING_AZCORE_RAPIDJSON_ALLOCATION_OVERRIDES 1
#   else
#       define USING_AZCORE_RAPIDJSON_ALLOCATION_OVERRIDES 0
#   endif
#endif

#if USING_AZCORE_RAPIDJSON_ALLOCATION_OVERRIDES

    namespace AZ::JSON
    {
        typedef void* pointer_type;
        typedef size_t size_type;

        pointer_type do_malloc(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord);
        pointer_type do_realloc(pointer_type ptr, size_type newSize, size_type newAlignment);
        void do_free(pointer_type ptr);
    } 

#   define RAPIDJSON_MALLOC(_size)         AZ::JSON::do_malloc(_size, 16, 0, "RapidJSON", __FILE__, __LINE__, 0)
#   define RAPIDJSON_REALLOC(_ptr, _size)  AZ::JSON::do_realloc(_ptr, _size, 16)
#   define RAPIDJSON_FREE(_ptr)            AZ::JSON::do_free(_ptr)
#   define RAPIDJSON_NEW(x)           new (AZ::JSON::do_malloc(sizeof(x), alignof(x), 0, "RapidJSON", __FILE__, __LINE__, 0)) x
#   define RAPIDJSON_DELETE(x)                 \
    {                                          \
        if (x)                                 \
        {                                      \
            std::destroy_at(x);                \
            AZ::JSON::do_free(x);              \
        }                                      \
    }

#endif // USING_AZCORE_RAPIDJSON_ALLOCATION_OVERRIDES

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#endif

// RapidJSON has a number of optimizations available such as the ability to use NEON and SSE2.
// We do not enable these operations.  
// If you enable these optimizations in the current version of RapidJSON,
// the function that scans for whitespace in the buffer requires that all strings you feed it 
// to parse, 
// * must be aligned to 16 bytes 
// * must be a multiple of 16 bytes long,
// * cannot be malformed in terms of missing a closing quote or bracket.
// We cannot guarantee this as we use it for user generated input data, strings from
// document files, inline strings from string literals, etc.

// See https://github.com/Tencent/rapidjson/pull/2213 for the active discussion
// currently ongoing in the RapidJSON community about this.

#if defined(AZ_COMPILER_MSVC)
    // windows defines may or may not be present for unity builds, so ensure StrCmp resolves to StrCmpW to avoid conflicts
    #define StrCmp StrCmpW
#endif

// Push all of rapidjson into a different namespace (rapidjson_ly for backward compatibility, as this is what
// it used to be at some point).  
// The risk of using the default namespace is that during library search, the linker might find other versions
// of rapidjson from other 3rd party libraries that have the same mangled function names and use those when 
// linking the code in.  The problem with that is different versions of rapidjson have different struct layouts
// and thus even though the signature of the function is the same, the actual implementation is not compatible,
// leading to mystery crashes.

#define RAPIDJSON_NAMESPACE rapidjson_ly

// Now that all of the above is declared, bring the RapidJSON headers in.
// If you add additional definitions or configuration options, add them above.
#include <rapidjson/rapidjson.h>

// After using this header file, any use of 'rapidjson' points at O3DE's rapidjson.  This will also cause
// compiler errors and warnings about redefinition if you happen to ever use this file and another rapidjson
// in the same compile unit somehow.
namespace rapidjson = rapidjson_ly;

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
