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

#ifndef DRILLER_MEMORY_EVENTS_H
#define DRILLER_MEMORY_EVENTS_H

#include "Woodpecker/Driller/DrillerEvent.h"

#include <AzCore/Memory/AllocationRecords.h>

namespace Driller
{
    namespace Memory
    {
        struct AllocationInfo
        {
            AllocationInfo()
                : m_recordsId(0)
                , m_name(nullptr)
                , m_alignment(0)
                , m_size(0)
                , m_fileName(nullptr)
                , m_fileLine(0)
                , m_stackFrames(nullptr)
            {}
            AZ::u64         m_recordsId;
            const char*     m_name;
            unsigned int    m_alignment;
            AZ::u64         m_size;
            const char*     m_fileName;
            int             m_fileLine;
            AZ::u64*        m_stackFrames;
        };

        struct AllocatorInfo
        {
            AllocatorInfo()
                : m_id(0)
                , m_recordsId(0)
                , m_name(nullptr)
                , m_capacity(0)
                , m_recordMode(AZ::Debug::AllocationRecords::RECORD_NO_RECORDS)
                , m_numStackLevels(0)
                , m_allocatedMemory(0)
            {}
            AZ::u64             m_id;
            AZ::u64             m_recordsId;
            const char*         m_name;
            AZ::u64             m_capacity;
            char                m_recordMode;
            char                m_numStackLevels;
            typedef AZStd::unordered_map<AZ::u64, AllocationInfo*> AllocationMapType;
            AllocationMapType   m_allocations;      ///< Current state of allocations
            size_t              m_allocatedMemory;  ///< Number of bytes of allocated memory.
        };

        enum MemoryEventType
        {
            MET_REGISTER_ALLOCATOR = 0,
            MET_UNREGISTER_ALLOCATOR,
            MET_REGISTER_ALLOCATION,
            MET_RESIZE_ALLOCATION,
            MET_UNREGISTER_ALLOCATION,
        };
    }

    class MemoryDrillerRegisterAllocatorEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(MemoryDrillerRegisterAllocatorEvent, AZ::SystemAllocator, 0)

        MemoryDrillerRegisterAllocatorEvent()
            : DrillerEvent(Memory::MET_REGISTER_ALLOCATOR) {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);
        Memory::AllocatorInfo   m_allocatorInfo;
    };

    class MemoryDrillerUnregisterAllocatorEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(MemoryDrillerUnregisterAllocatorEvent, AZ::SystemAllocator, 0)

        MemoryDrillerUnregisterAllocatorEvent()
            : DrillerEvent(Memory::MET_UNREGISTER_ALLOCATOR)
            , m_allocatorId(0)
            , m_removedAllocatorInfo(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64                 m_allocatorId;
        Memory::AllocatorInfo*  m_removedAllocatorInfo;
    };

    class MemoryDrillerRegisterAllocationEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(MemoryDrillerRegisterAllocationEvent, AZ::SystemAllocator, 0)

        MemoryDrillerRegisterAllocationEvent()
            : DrillerEvent(Memory::MET_REGISTER_ALLOCATION)
            , m_address(0)
            , m_modifiedAllocatorInfo(nullptr)
        {}

        ~MemoryDrillerRegisterAllocationEvent()
        {
            if (m_allocationInfo.m_stackFrames)
            {
                azfree(m_allocationInfo.m_stackFrames);
            }
        }

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64         m_address;
        Memory::AllocationInfo  m_allocationInfo;
        Memory::AllocatorInfo*  m_modifiedAllocatorInfo;
    };

    class MemoryDrillerUnregisterAllocationEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(MemoryDrillerUnregisterAllocationEvent, AZ::SystemAllocator, 0)

        MemoryDrillerUnregisterAllocationEvent()
            : DrillerEvent(Memory::MET_UNREGISTER_ALLOCATION)
            , m_recordsId(0)
            , m_address(0)
            , m_removedAllocationInfo(nullptr)
            , m_modifiedAllocatorInfo(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64         m_recordsId;
        AZ::u64         m_address;
        Memory::AllocationInfo* m_removedAllocationInfo;
        Memory::AllocatorInfo*  m_modifiedAllocatorInfo;
    };

    class MemoryDrillerResizeAllocationEvent
        : public DrillerEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(MemoryDrillerResizeAllocationEvent, AZ::SystemAllocator, 0)

        MemoryDrillerResizeAllocationEvent()
            : DrillerEvent(Memory::MET_RESIZE_ALLOCATION)
            , m_recordsId(0)
            , m_address(0)
            , m_newSize(0)
            , m_oldSize(0)
            , m_modifiedAllocationInfo(nullptr)
        {}

        virtual void    StepForward(Aggregator* data);
        virtual void    StepBackward(Aggregator* data);

        AZ::u64         m_recordsId;
        AZ::u64         m_address;
        AZ::u64         m_newSize;
        AZ::u64         m_oldSize;
        Memory::AllocationInfo* m_modifiedAllocationInfo;
        Memory::AllocatorInfo*  m_modifiedAllocatorInfo;
    };
}

#endif
