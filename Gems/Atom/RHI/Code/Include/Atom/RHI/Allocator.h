/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

namespace AZ
{
    namespace RHI
    {

         //! A virtual address which may be relative to a base resource. This means
         //! 0 might be a valid address (dependent on the Allocator::Descriptor::m_addressBase value).
         //! To account for this, VirtualAddress::Null is used instead. Check validity of the address
         //! using IsValid or IsNull instead of checking for 0. VirtualAddress is initialized
         //! to Null, so returning the default constructor is sufficient to represent an invalid address.
        class VirtualAddress
        {
            static const VirtualAddress Null;
        public:
            VirtualAddress();
            VirtualAddress(uintptr_t ptr);

            static VirtualAddress CreateNull();

            //! Creates a valid address with a zero offset.
            static VirtualAddress CreateZero();

            //! Creates an address from a pointer.
            static VirtualAddress CreateFromPointer(void* ptr);

            //! Creates an address from an offset from a base pointer.
            static VirtualAddress CreateFromOffset(uint64_t offset);

            inline bool IsValid() const
            {
                return m_ptr != Null.m_ptr;
            }

            inline bool IsNull() const
            {
                return m_ptr == Null.m_ptr;
            }

            uintptr_t m_ptr;
        };

        //! An allocator interface used for external GPU allocations. The allocator
        //! does not manage the host memory. Instead, the user specifies a base address
        //! (which may be 0, in order to allocate offsets from a base resource). The allocator
        //! interface also provides an API for garbage collection. If used to manage GPU resources,
        //! these are often deferred-released after N frames. The user may provide a garbage collection
        //! latency, which controls the number of GarbageCollect calls that must occur before an allocation
        //! is actually reclaimed. The intended use case is to garbage collect at the end of each frame.
        class Allocator
        {
        public:
            struct Descriptor
            {
                static const size_t DefaultAlignment = 256;

                Descriptor();

                /// The base address added to every allocation (defaults to 0).
                VirtualAddress m_addressBase;

                /// The minimum allocation size (and subsequent data alignment).
                size_t m_alignmentInBytes = DefaultAlignment;

                /// The total size of the allocation region.
                size_t m_capacityInBytes = 0;

                /// The number of GC cycles to wait before recycling a freed block.
                size_t m_garbageCollectLatency = 0;
            };

            virtual ~Allocator() {}

            virtual void Shutdown() = 0;

            //! Allocates a virtual address relative to the base address provided at initialization time.
            //! @param byteCount The number of bytes to allocate.
            //! @param byteAlignement The alignment used to align the allocation.
            virtual VirtualAddress Allocate(size_t byteCount, size_t byteAlignment) = 0;

            //! Deallocates an allocation. The memory is not reclaimed until garbage collect is called.
            //! Depending on the garbage collection latency, it may take several garbage collection cycles
            //! before the memory is reclaimed.
            virtual void DeAllocate(VirtualAddress offset) = 0;

            //! Allocations are deferred-released until a specific number of GC cycles have occurred. This
            //! is useful for allocations actively being consumed by the GPU.
            virtual void GarbageCollect() = 0;

            //! Forces garbage collection of all allocations, regardless of the GC latency.
            virtual void GarbageCollectForce() = 0;

            //! Returns the number of allocations active for this allocator. This includes
            //! allocations that are pending garbage collection.
            virtual size_t GetAllocationCount() const { return 0; }

            //! Returns the number of bytes used by the allocator. This includes
            //! allocations that are pending garbage collection.
            virtual size_t GetAllocatedByteCount() const { return 0; }

            //! Returns the descriptor used to initialize the allocator.
            virtual const Descriptor& GetDescriptor() const = 0;

            //! Clone the current allocator to the new allocator passed in
            virtual void Clone([[maybe_unused]] RHI::Allocator* newAllocator)
            {
                AZ_Assert(false, "Not Implemented");
            };

            //! Helper for converting agnostic VirtualAddress type to pointer type. Will convert
            //! VirtualAddress::Null to nullptr.
            template <typename T>
            T* AllocateAs(size_t byteCount, size_t byteAlignment)
            {
                VirtualAddress address = Allocate(byteCount, byteAlignment);
                return (address.IsValid()) ? reinterpret_cast<T*>(address.m_ptr) : nullptr;
            }
        };
    }
}
