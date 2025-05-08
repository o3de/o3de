/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(AZ_ENABLE_TRACING) && defined(AZ_MEMORY_TAG_TRACING) && !defined(_RELEASE) && defined(CARBONATED)

namespace AZ
{
    /**
    * Memory allocation marker to provide basic memory piece info to allocator.
    */
    class MemoryAllocationMarker
    {
    public:
        template<size_t N>
        MemoryAllocationMarker(char const (&name)[N], const char* file, int line)
        {
            Init(name, file, line, true);
        }
        MemoryAllocationMarker(char const* name, const char* file, int line)
        {
            Init(name, file, line, false);
        }
        ~MemoryAllocationMarker();

    private:
        void Init(const char* name, const char* file, int line, bool isLiteral);
    };


    // fill tags here, fill names accordingly in cpp
    enum class MemoryTagValue : unsigned int
    {
        Unused = 0,
        Overhead = 1, // reserved for extra memory consumed by the memory tracking system
        ClassData = 2,
        TaskGraphComponent = 3,
        MaterialInit = 4,
        ImageMip = 5,
        EMotionFX = 6,
        Mesh = 7,
        AssetCatalog = 8,
        RHIDevice = 9,
        Lod = 10,
        Shader = 11,

        GameSpecific = 32  // game tags starts from this index, use your own enum starting with this value
    };

    class MemoryTagMarker
    {
    public:
        MemoryTagMarker(MemoryTagValue v);
        ~MemoryTagMarker();
    };

    class AssetMemoryTagMarker
    {
    public:
        AssetMemoryTagMarker(const char* name);
        ~AssetMemoryTagMarker();
    };

    class AssetMemoryTagLimit
    {
    public:
        AssetMemoryTagLimit(size_t limit);
        ~AssetMemoryTagLimit();

        static void SetLimit(size_t limit);
        static size_t GetLimit();
    };

} // namespace AZ

#define MEMORY_TAGS_ACTIVE

#define MEMORY_ALLOCATION_MARKER AZ::MemoryAllocationMarker(nullptr, __FILE__, __LINE__)

// without these wrapper macros Name##_LINE__ is Name__LINE__ but not Name123
#define CONCATENATION_MACRO_1(a, b) a##b
#define CONCATENATION_MACRO_2(a, b) CONCATENATION_MACRO_1(a, b)

// the name must be a string literal
#define MEMORY_ALLOCATION_MARKER_NAME(name) AZ::MemoryAllocationMarker CONCATENATION_MACRO_2(memoryNameMarker_, __LINE__)(name, __FILE__, __LINE__)

#define MEMORY_TAG(x) AZ::MemoryTagMarker CONCATENATION_MACRO_2(memoryTagMarker_, __LINE__)(AZ::MemoryTagValue::x)
#define MEMORY_TAG_GAME(x) AZ::MemoryTagMarker CONCATENATION_MACRO_2(memoryTagMarkerGame_, __LINE__)(static_cast<AZ::MemoryTagValue>((unsigned int)(x)))

#define ASSET_TAG(x) AZ::AssetMemoryTagMarker CONCATENATION_MACRO_2(assetMemoryTagMarker_, __LINE__)(x)
#define MEMORY_ASSET_LIMIT(x) AZ::AssetMemoryTagLimit CONCATENATION_MACRO_2(assetMemoryTagLimit_, __LINE__)(x)
#define MEMORY_SET_ASSET_LIMIT(x) AZ::AssetMemoryTagLimit::SetLimit(x)
#define MEMORY_GET_ASSET_LIMIT() AZ::AssetMemoryTagLimit::GetLimit()

#else  // AZ_ENABLE_TRACING  && !_RELEASE && CARBONATED

#define MEMORY_ALLOCATION_MARKER
#define MEMORY_ALLOCATION_MARKER_NAME(name)

#define MEMORY_TAG(x)
#define MEMORY_TAG_GAME(x)

#define ASSET_TAG(x)
#define MEMORY_ASSET_LIMIT(x)
#define MEMORY_SET_ASSET_LIMIT(x)
#define MEMORY_GET_ASSET_LIMIT() 0

#endif  // AZ_ENABLE_TRACING  && !_RELEASE && CARBONATED
