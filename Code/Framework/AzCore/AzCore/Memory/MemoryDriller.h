/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            virtual const char*  GroupName() const          { return "SystemDrillers"; }
            virtual const char*  GetName() const            { return "MemoryDriller"; }
            virtual const char*  GetDescription() const     { return "Reports all allocators and memory allocations."; }
            virtual void         Start(const Param* params = NULL, int numParams = 0);
            virtual void         Stop();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // MemoryDrillerBus
            virtual void RegisterAllocator(IAllocator* allocator);
            virtual void UnregisterAllocator(IAllocator* allocator);

            virtual void RegisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount);
            virtual void UnregisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AllocationInfo* info);
            virtual void ReallocateAllocation(IAllocator* allocator, void* prevAddress, void* newAddress, size_t newByteSize, size_t newAlignment);
            virtual void ResizeAllocation(IAllocator* allocator, void* address, size_t newSize);

            virtual void DumpAllAllocations();
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
