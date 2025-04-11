/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/MemoryUsage.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Name/Name.h>

namespace AZ::RHI
{
    struct MemoryStatistics
    {
        struct Buffer
        {
            //! The user-provided name of the buffer instance.
            Name m_name;

            //! Bind flags of the buffer.
            BufferBindFlags m_bindFlags = BufferBindFlags::None;

            //! The memory usage of the buffer on the pool.
            size_t m_sizeInBytes = 0;

            //! The fragmentation within the buffer (optional). If supplied, should be computed as:
            //!     1 - (largest free block byte size) / (total free memory).
            //! Buffers that do not suballocate do not need to provide this quantity
            float m_fragmentation = 0.f;
        };

        struct Image
        {
            //! The user-provided name of the image instance.
            Name m_name;

            //! Bind flags of the image.
            ImageBindFlags m_bindFlags = ImageBindFlags::None;

            //! The memory usage of the image on the pool.
            size_t m_sizeInBytes = 0;

            //! The minimum memory usage of the image.
            //! This is the possible minimum resident size of a streamable image when all its evictable mipmaps are not resident
            size_t m_minimumSizeInBytes = 0;
        };

        //! This structure tracks the memory usage of a specific pool instance. Pools associate with, at most, one
        //! heap from a specific heap type (e.g. host / device).
        struct Pool
        {
            //! The user-defined name of the pool instance.
            Name m_name;

            //! The list of buffers present in the pool.
            AZStd::vector<Buffer> m_buffers;

            //! The list of images present in the pool.
            AZStd::vector<Image> m_images;

            //! The memory usage of the pool.
            PoolMemoryUsage m_memoryUsage;
        };

        //! This structure tracks an instance of a physical memory heap. For certain platforms, there
        //! may be multiple heaps of a particular type (for example, multiple GPUs, or custom heaps on
        //! the same adapter). The source of the data is platform specific.
        struct Heap
        {
            //! The platform-defined name of the heap.
            Name m_name;

            //! The type of the heap.
            HeapMemoryLevel m_heapMemoryType;

            //! Memory usage of the heap.
            HeapMemoryUsage m_memoryUsage;
        };

        //! The list of platform-specific heaps available on the system.
        AZStd::vector<Heap> m_heaps;

        //! The list of pools.
        AZStd::vector<Pool> m_pools;

        //! Indicates if detailed memory statistics were captured
        bool m_detailedCapture;
    };
}
