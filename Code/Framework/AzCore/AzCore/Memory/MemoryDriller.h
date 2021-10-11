/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_MEMORY_DRILLER_H
#define AZCORE_MEMORY_DRILLER_H 1

#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDrillerBus.h>

namespace AZ
{
    namespace Debug
    {
        struct StackFrame;

        /**
         * Trace messages driller class
         */
        class MemoryDriller
            : public Driller
            , public MemoryDrillerBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(MemoryDriller, OSAllocator, 0)

            // TODO: Centralized settings for memory tracking.
            struct Descriptor
            {
            };

            MemoryDriller(const Descriptor& desc = Descriptor());
            ~MemoryDriller();

        protected:
            //////////////////////////////////////////////////////////////////////////
            // Driller
            const char*  GroupName() const override          { return "SystemDrillers"; }
            const char*  GetName() const override            { return "MemoryDriller"; }
            const char*  GetDescription() const override     { return "Reports all allocators and memory allocations."; }
            void         Start(const Param* params = NULL, int numParams = 0) override;
            void         Stop() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // MemoryDrillerBus
            void RegisterAllocator(IAllocator* allocator) override;
            void UnregisterAllocator(IAllocator* allocator) override;

            void RegisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount) override;
            void UnregisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AllocationInfo* info) override;
            void ReallocateAllocation(IAllocator* allocator, void* prevAddress, void* newAddress, size_t newByteSize, size_t newAlignment) override;
            void ResizeAllocation(IAllocator* allocator, void* address, size_t newSize) override;

            void DumpAllAllocations() override;
            //////////////////////////////////////////////////////////////////////////

            void RegisterAllocatorOutput(IAllocator* allocator);
            void RegisterAllocationOutput(IAllocator* allocator, void* address, const AllocationInfo* info);
        private:
            // Store a list of all of our allocator records so we can dump them all without having to know about the allocators
            AZStd::list<Debug::AllocationRecords*, OSStdAllocator> m_allAllocatorRecords;
        };
    } // namespace Debug
} // namespace AZ

#endif // AZCORE_MEMORY_DRILLER_H
#pragma once
