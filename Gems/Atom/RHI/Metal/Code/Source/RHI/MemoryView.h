/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <Atom/RHI/MemoryAllocation.h>
#include <Metal/Metal.h>
#include <RHI/Memory.h>
#include <RHI/Metal.h>
#include <Metal/Metal.h>

namespace AZ
{
    namespace Metal
    {
        using MemoryAllocation = RHI::MemoryAllocation<Memory>;

        //! Represents a view into GPU/CPU memory. It contains a pointer to void* for now. The
        //! contents will be interpreted slightly differently for buffer and image resources.
        class MemoryView
        {
            friend class BufferMemoryAllocator;

        public:
            MemoryView() = default;
            MemoryView(RHI::Ptr<Memory> memory, size_t offset, size_t size, size_t alignment);
            MemoryView(const MemoryAllocation& memAllocation);

            // Supports copy and move construction / assignment.
            MemoryView(const MemoryView& rhs) = default;
            MemoryView(MemoryView&& rhs) = default;
            MemoryView& operator=(const MemoryView& rhs) = default;
            MemoryView& operator=(MemoryView&& rhs) = default;

            bool IsValid() const;

            // Returns the offset relative to the base memory address in bytes.
            size_t GetOffset() const;

            // Returns the size of the memory view region in bytes.
            size_t GetSize() const;

            // Returns the alignment of the memory view region in bytes.
            size_t GetAlignment() const;
            
            // Returns the pointer to MetalResource
            Memory* GetMemory() const;

            //Add a label to this buffer. Helpful for GPU captures
            void SetName(const AZStd::string_view& name);            
            
            // Returns the storage mode of this resource
            MTLStorageMode GetStorageMode();
            
            void* GetCpuAddress() const
            {
                uint8_t* cpuAddress = static_cast<uint8_t*>(m_memoryAllocation.m_memory->GetCpuAddress());
                AZ_Error("MemoryView", cpuAddress, "Null Address");
                return cpuAddress + m_memoryAllocation.m_offset;
            }
            
            template <typename T>
            T GetGpuAddress() const
            {
                if(m_memoryAllocation.m_memory)
                {
                    return m_memoryAllocation.m_memory->GetGpuAddress<T>();
                }
                return nil;
            }
            
        private:
            MemoryAllocation m_memoryAllocation;
        };
    }
}
