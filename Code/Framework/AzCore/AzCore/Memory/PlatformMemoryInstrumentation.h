/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/AzCore_Traits_Platform.h>

#if AZ_TRAIT_OS_MEMORY_INSTRUMENTATION && !defined(_RELEASE)
#define PLATFORM_MEMORY_INSTRUMENTATION_ENABLED 1
#else
#define PLATFORM_MEMORY_INSTRUMENTATION_ENABLED 0
#endif

#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED

#include <AzCore/base.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    /**
    * PlatformMemoryInstrumentation - Abstraction layer for platform specific memory instrumentation.
    */
    class PlatformMemoryInstrumentation
    {
    public:
        static uint16_t GetNextGroupId() { return m_nextGroupId++; };
        static void RegisterGroup(uint16_t id, const char* name, uint16_t parentGroup);
        static void Alloc(const void* ptr, uint64_t size, uint32_t padding, uint16_t group);
        static void Free(const void* ptr);
        static void ReallocBegin(const void* origPtr, uint64_t size, uint16_t group);
        static void ReallocEnd(const void* newPtr, uint64_t size, uint32_t padding);
        static const uint16_t m_groupRoot;
        static uint16_t m_nextGroupId;
    };
}

#endif // PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
