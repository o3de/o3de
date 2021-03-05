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
#ifndef AZCORE_MEMORY_DRILLER_BUS_H
#define AZCORE_MEMORY_DRILLER_BUS_H 1

#include <AzCore/Driller/DrillerBus.h>

namespace AZ
{
    class IAllocator;
    namespace Debug
    {
        //class AllocationRecords;
        struct AllocationInfo;

        /**
         * Memory allocations driller message.
         *
         * We use a driller bus so all messages are sending in exclusive matter no other driller messages
         * can be triggered at that moment, so we already preserve the calling order. You can assume
         * all access code in the driller framework in guarded. You can manually lock the driller mutex are you
         * use by using \ref AZ::Debug::DrillerEBusMutex.
         */
        class MemoryDrillerMessages
            : public AZ::Debug::DrillerEBusTraits
        {
        public:
            virtual ~MemoryDrillerMessages() {}

            /// Register allocation (with customizable tracking settings - TODO: we should centralize this settings and remove them from here)
            virtual void RegisterAllocator(IAllocator* allocator) = 0;
            virtual void UnregisterAllocator(IAllocator* allocator) = 0;

            virtual void RegisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount) = 0;
            virtual void UnregisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AllocationInfo* info) = 0;
            virtual void ReallocateAllocation(IAllocator* allocator, void* prevAddress, void* newAddress, size_t newByteSize, size_t newAlignment) = 0;
            virtual void ResizeAllocation(IAllocator* allocator, void* address, size_t newSize) = 0;

            virtual void DumpAllAllocations() = 0;
        };

        typedef AZ::EBus<MemoryDrillerMessages> MemoryDrillerBus;
    } // namespace Debug
} // namespace AZ

#endif // AZCORE_MEMORY_DRILLER_BUS_H
#pragma once
