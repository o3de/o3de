/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if defined(AZ_ENABLE_TRACING) && !defined(_RELEASE) && defined(CARBONATED)

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

        GameSpecific = 32  // game tags starts from this index, use your own enum starting with this value
    };

    class MemoryTagMarker
    {
    public:
        MemoryTagMarker(MemoryTagValue v);
        ~MemoryTagMarker();
    };
}

#define MEMORY_ALLOCATION_MARKER AZ::MemoryAllocationMarker(nullptr, __FILE__, __LINE__)

// the name must be a string literal
#define MEMORY_ALLOCATION_MARKER_NAME(name) AZ::MemoryAllocationMarker(name, __FILE__, __LINE__)

#define MEMORY_TAG(x) AZ::MemoryTagMarker tagMarker##__LINE__(AZ::MemoryTagValue::x)
#define MEMORY_TAG_GAME(x) AZ::MemoryTagMarker tagMarker##__LINE__(static_cast<AZ::MemoryTagValue>((unsigned int)(x)))

#else  // AZ_ENABLE_TRACING  && !_RELEASE && CARBONATED

#define MEMORY_ALLOCATION_MARKER
#define MEMORY_ALLOCATION_MARKER_NAME(name)

#define MEMORY_TAG(x)
#define MEMORY_TAG_GAME(x)

#endif  // AZ_ENABLE_TRACING  && !_RELEASE && CARBONATED
