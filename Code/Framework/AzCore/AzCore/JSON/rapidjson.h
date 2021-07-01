/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#define RAPIDJSON_MALLOC(_size) AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(_size, 16, 0, "RapidJson", __FILE__, __LINE__, 0)
#define RAPIDJSON_REALLOC(_ptr, _newSize) _ptr ? AZ::AllocatorInstance<AZ::SystemAllocator>::Get().ReAllocate(_ptr, _newSize, 16) : RAPIDJSON_MALLOC(_newSize)
#define RAPIDJSON_FREE(_ptr) if (_ptr) { AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(_ptr, 0, 0); }
#define RAPIDJSON_CLASS_ALLOCATOR(_class) AZ_CLASS_ALLOCATOR(_class, AZ::SystemAllocator, 0)

// Set custom namespace for AzCore's rapidjson to avoid various collisions.
#define RAPIDJSON_NAMESPACE rapidjson_ly

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#endif

// Make you have available rapidjson/include folder. Currently 3rdParty\rapidjson\rapidjson-1.1.0\include
#include <rapidjson/rapidjson.h>

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

// Allow our existing code to continue use "rapidjson::"
#define rapidjson RAPIDJSON_NAMESPACE
