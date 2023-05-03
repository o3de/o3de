/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/JSON/allocators.h>
#include <AzCore/std/containers/array.h>

namespace AZ::Json
{
    //! Allocator which uses the a predefined buffer to allocate memory
    //! In most cases this is stack memory
    //! But if this allocator is a member of a class that is dynamically allocated, then it is heap memory
    //! Implements the rapidjson::Allocator concept
    //! https://rapidjson.org/md_doc_internals.html#InternalAllocator
    //! This
    template<size_t SizeN, size_t AlignN = alignof(AZStd::byte)>
    class RapidjsonStackAllocator
    {
    public:
        //! Stack allocator doesn't support freeing allocated memory at all
        static constexpr bool kNeedFree = false;

        // Create a monotonic_buffer_resource which will only allocated
        // from the supplied array
        // This uses std::pmr::null_memory_resource to prevent using an upstream memory resource
        // https://en.cppreference.com/w/cpp/memory/null_memory_resource
        RapidjsonStackAllocator()
            : m_freeOffset(m_buffer.data())
        {
        }
        void* Malloc(size_t size)
        {
            if (m_freeOffset + size > m_buffer.data() + m_buffer.size())
            {
                AZ_Assert(false, "Rapidjson stack allocator cannot fulfill allocation. Requested size %zu. Size remaining %lld",
                    size, (m_buffer.data() + m_buffer.size()) - m_freeOffset);
                return nullptr;
            }
            AZStd::byte* newPtr = m_freeOffset;
            m_freeOffset = newPtr + size;
            return newPtr;
        }

        void* Realloc(void* ptr, size_t originalSize, size_t newSize)
        {
            auto originalPtr = reinterpret_cast<AZStd::byte*>(ptr);
            // Can always resize downwards
            if (newSize <= originalSize)
            {
                return originalPtr;
            }

            // if the (originalPtr + originalSize) is the last allocated block(i.e the offset of free memory)
            // Then re-use the originalPtr address for the new allocation
            if (originalPtr + originalSize == m_freeOffset)
            {
                if (originalPtr + newSize > m_buffer.data() + m_buffer.size())
                {
                    AZ_Assert(false, "Rapidjson stack allocator cannot fulfill reallocation. Requested size %zu. Size remaining %lld",
                        newSize, (m_buffer.data() + m_buffer.size()) - reinterpret_cast<AZStd::byte*>(originalPtr));
                    return nullptr;
                }

                // Set the free offset directly to the end of the reallocated buffer
                m_freeOffset = originalPtr + newSize;
                // Return the original pointer since the realloc happens in places
                return originalPtr;
            }

            // fallback to call Malloc in this case, followed by a memcpy if malloc succeeds
            void* newPtr = Malloc(newSize);
            if (newPtr != nullptr)
            {
                // memcpy over the old data to the new buffer
                ::memcpy(newPtr, originalPtr, originalSize);
            }
            return newPtr;
        }


        static void Free(void*)
        {
            // Does not free memory
        }

    private:
        alignas(AlignN) AZStd::array<AZStd::byte, SizeN> m_buffer;
        AZStd::byte* m_freeOffset{};
    };
}
