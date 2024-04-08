/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

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
        Overhead = 0, // reserved for extra memory consumed by the memory tracking system
        AI = 1,

        GameSpecific = 32  // game tags starts from this index, use your own enum
    };

    class MemoryTagMarker
    {
    public:
        MemoryTagMarker(MemoryTagValue v);
        ~MemoryTagMarker();
    };

}

#if !defined(_RELEASE) && defined(AZ_ENABLE_TRACING)

#define MEMORY_ALLOCATION_MARKER AZ::MemoryAllocationMarker(nullptr, __FILE__, __LINE__)

// the name must be a string literal
#define MEMORY_ALLOCATION_MARKER_NAME(name) AZ::MemoryAllocationMarker(name, __FILE__, __LINE__)

#define MEMORY_TAG(x) AZ::MemoryTagMarker tagMarker##__LINE__((unsigned int)(AZ::MemoryTagValue::x))
#define MEMORY_TAG_GAME(x) AZ::MemoryTagMarker tagMarker##__LINE__((unsigned int)(x))

#else

#define MEMORY_ALLOCATION_MARKER
#define MEMORY_ALLOCATION_MARKER_NAME(name)

#define MEMORY_TAG(x)
#define MEMORY_TAG_GAME(x)

#endif
