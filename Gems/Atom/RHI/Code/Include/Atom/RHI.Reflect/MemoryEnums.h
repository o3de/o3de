/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ::RHI
{
    //! Describes the memory requirements for allocating a resource.
    struct ResourceMemoryRequirements
    {
        size_t m_alignmentInBytes = 0;
        size_t m_sizeInBytes = 0;
    };

    //! Regions of the host memory heap are optimized for either read or write access.
    enum class HostMemoryAccess : uint32_t
    {
        Write = 0,
        Read
    };

    //! Memory heaps in the RHI form a hierarchy, similar to caches. Higher level heaps push or pull
    //! from lower level heaps. Higher level heaps represent faster / more direct GPU access. On most
    //! platforms, this will also translate to higher level heap memory being more scarce.
    enum class HeapMemoryLevel : uint32_t
    {
        /// Represents host memory stored local to the CPU.
        Host = 0,

        /// Represents device memory stored local to a discrete GPU.
        Device
    };

    const uint32_t HeapMemoryLevelCount = 2;

    // Bind enums with uuids. Required for named enum support.
    // Note: AZ_TYPE_INFO_SPECIALIZE has to be declared in AZ namespace
    AZ_TYPE_INFO_SPECIALIZE(HostMemoryAccess, "{27A01E51-4532-42B8-8135-9CF20F9AC119}");
    AZ_TYPE_INFO_SPECIALIZE(HeapMemoryLevel, "{32192E65-98F5-4FC9-BF9E-4F99A85DBF72}");
}
