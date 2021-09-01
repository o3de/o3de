/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Debug/StackTracer.h>

namespace AZ
{
    namespace Debug
    {
        //=========================================================================
        // MemoryDriller
        // [2/6/2013]
        //=========================================================================
        MemoryDriller::MemoryDriller(const Descriptor& desc)
        {
            (void)desc;
            BusConnect();

            AllocatorManager::Instance().EnterProfilingMode();

            {
                // Register all allocators that were created before the driller existed
                auto allocatorLock = AllocatorManager::Instance().LockAllocators();

                for (int i = 0; i < AllocatorManager::Instance().GetNumAllocators(); ++i)
                {
                    IAllocator* allocator = AllocatorManager::Instance().GetAllocator(i);
                    RegisterAllocator(allocator);
                }
            }
        }

        //=========================================================================
        // ~MemoryDriller
        // [2/6/2013]
        //=========================================================================
        MemoryDriller::~MemoryDriller()
        {
            BusDisconnect();
            AllocatorManager::Instance().ExitProfilingMode();
        }

        //=========================================================================
        // Start
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;

            // dump current allocations for all allocators with tracking
            auto allocatorLock = AllocatorManager::Instance().LockAllocators();
            for (int i = 0; i < AllocatorManager::Instance().GetNumAllocators(); ++i)
            {
                IAllocator* allocator = AllocatorManager::Instance().GetAllocator(i);
                if (auto records = allocator->GetRecords())
                {
                    RegisterAllocatorOutput(allocator);
                    const AllocationRecordsType& allocMap = records->GetMap();
                    for (AllocationRecordsType::const_iterator allocIt = allocMap.begin(); allocIt != allocMap.end(); ++allocIt)
                    {
                        RegisterAllocationOutput(allocator, allocIt->first, &allocIt->second);
                    }
                }
            }
        }

        //=========================================================================
        // Stop
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::Stop()
        {
        }

        //=========================================================================
        // RegisterAllocator
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::RegisterAllocator(IAllocator* allocator)
        {
            // Ignore if our allocator is already registered
            if (allocator->GetRecords() != nullptr)
            {
                return;
            }

            auto debugConfig = allocator->GetDebugConfig();

            if (!debugConfig.m_excludeFromDebugging)
            {
                allocator->SetRecords(aznew Debug::AllocationRecords((unsigned char)debugConfig.m_stackRecordLevels, debugConfig.m_usesMemoryGuards, debugConfig.m_marksUnallocatedMemory, allocator->GetName()));

                m_allAllocatorRecords.push_back(allocator->GetRecords());

                if (m_output == nullptr)
                {
                    return;                    // we have no active output
                }
                RegisterAllocatorOutput(allocator);
            }
        }
        //=========================================================================
        // RegisterAllocatorOutput
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::RegisterAllocatorOutput(IAllocator* allocator)
        {
            auto records = allocator->GetRecords();
            m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            m_output->BeginTag(AZ_CRC("RegisterAllocator", 0x19f08114));
            m_output->Write(AZ_CRC("Name", 0x5e237e06), allocator->GetName());
            m_output->Write(AZ_CRC("Id", 0xbf396750), allocator);
            m_output->Write(AZ_CRC("Capacity", 0xb5e8b174), allocator->GetAllocationSource()->Capacity());
            m_output->Write(AZ_CRC("RecordsId", 0x7caaca88), records);
            if (records)
            {
                m_output->Write(AZ_CRC("RecordsMode", 0x764c147a), (char)records->GetMode());
                m_output->Write(AZ_CRC("NumStackLevels", 0xad9cff15), records->GetNumStackLevels());
            }
            m_output->EndTag(AZ_CRC("RegisterAllocator", 0x19f08114));
            m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
        }

        //=========================================================================
        // UnregisterAllocator
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::UnregisterAllocator(IAllocator* allocator)
        {
            auto allocatorRecords = allocator->GetRecords();
            AZ_Assert(allocatorRecords, "This allocator is not registered with the memory driller!");
            for (auto records : m_allAllocatorRecords)
            {
                if (records == allocatorRecords)
                {
                    m_allAllocatorRecords.remove(records);
                    break;
                }
            }
            delete allocatorRecords;
            allocator->SetRecords(nullptr);

            if (m_output == nullptr)
            {
                return;                    // we have no active output
            }
            m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            m_output->Write(AZ_CRC("UnregisterAllocator", 0xb2b54f93), allocator);
            m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
        }

        //=========================================================================
        // RegisterAllocation
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::RegisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount)
        {
            auto records = allocator->GetRecords();
            if (records)
            {
                const AllocationInfo* info = records->RegisterAllocation(address, byteSize, alignment, name, fileName, lineNum, stackSuppressCount + 1);
                if (m_output == nullptr)
                {
                    return;                   // we have no active output
                }
                RegisterAllocationOutput(allocator, address, info);
            }
        }

        //=========================================================================
        // RegisterAllocationOutput
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::RegisterAllocationOutput(IAllocator* allocator, void* address, const AllocationInfo* info)
        {
            auto records = allocator->GetRecords();
            m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            m_output->BeginTag(AZ_CRC("RegisterAllocation", 0x992a9780));
            m_output->Write(AZ_CRC("RecordsId", 0x7caaca88), records);
            m_output->Write(AZ_CRC("Address", 0x0d4e6f81), address);
            if (info)
            {
                if (info->m_name)
                {
                    m_output->Write(AZ_CRC("Name", 0x5e237e06), info->m_name);
                }
                m_output->Write(AZ_CRC("Alignment", 0x2cce1e5c), info->m_alignment);
                m_output->Write(AZ_CRC("Size", 0xf7c0246a), info->m_byteSize);
                if (info->m_fileName)
                {
                    m_output->Write(AZ_CRC("FileName", 0x3c0be965), info->m_fileName);
                    m_output->Write(AZ_CRC("FileLine", 0xb33c2395), info->m_lineNum);
                }
                // copy the stack frames directly, resolving the stack should happen later as this is a SLOW procedure.
                if (info->m_stackFrames)
                {
                    m_output->Write(AZ_CRC("Stack", 0x41a87b6a), info->m_stackFrames, info->m_stackFrames + records->GetNumStackLevels());
                }
            }
            m_output->EndTag(AZ_CRC("RegisterAllocation", 0x992a9780));
            m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
        }

        //=========================================================================
        // UnRegisterAllocation
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::UnregisterAllocation(IAllocator* allocator, void* address, size_t byteSize, size_t alignment, AllocationInfo* info)
        {
            auto records = allocator->GetRecords();
            if (records)
            {
                records->UnregisterAllocation(address, byteSize, alignment, info);

                if (m_output == nullptr)
                {
                    return;                    // we have no active output
                }
                m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
                m_output->BeginTag(AZ_CRC("UnRegisterAllocation", 0xea5dc4cd));
                m_output->Write(AZ_CRC("RecordsId", 0x7caaca88), records);
                m_output->Write(AZ_CRC("Address", 0x0d4e6f81), address);
                m_output->EndTag(AZ_CRC("UnRegisterAllocation", 0xea5dc4cd));
                m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            }
        }

        //=========================================================================
        // ReallocateAllocation
        // [10/1/2018]
        //=========================================================================
        void MemoryDriller::ReallocateAllocation(IAllocator* allocator, void* prevAddress, void* newAddress, size_t newByteSize, size_t newAlignment)
        {
            AllocationInfo info;
            UnregisterAllocation(allocator, prevAddress, 0, 0, &info);
            RegisterAllocation(allocator, newAddress, newByteSize, newAlignment, info.m_name, info.m_fileName, info.m_lineNum, 0);
        }

        //=========================================================================
        // ResizeAllocation
        // [2/6/2013]
        //=========================================================================
        void MemoryDriller::ResizeAllocation(IAllocator* allocator, void* address, size_t newSize)
        {
            auto records = allocator->GetRecords();
            if (records)
            {
                records->ResizeAllocation(address, newSize);

                if (m_output == nullptr)
                {
                    return;                    // we have no active output
                }
                m_output->BeginTag(AZ_CRC("MemoryDriller", 0x1b31269d));
                m_output->BeginTag(AZ_CRC("ResizeAllocation", 0x8a9c78dc));
                m_output->Write(AZ_CRC("RecordsId", 0x7caaca88), records);
                m_output->Write(AZ_CRC("Address", 0x0d4e6f81), address);
                m_output->Write(AZ_CRC("Size", 0xf7c0246a), newSize);
                m_output->EndTag(AZ_CRC("ResizeAllocation", 0x8a9c78dc));
                m_output->EndTag(AZ_CRC("MemoryDriller", 0x1b31269d));
            }
        }

        void MemoryDriller::DumpAllAllocations()
        {
            // Create a copy so allocations done during the printing dont end up affecting the container
            const AZStd::list<Debug::AllocationRecords*, OSStdAllocator> allocationRecords = m_allAllocatorRecords;

            for (auto records : allocationRecords)
            {
                // Skip if we have had no allocations made
                if (records->RequestedAllocs())
                {
                    records->EnumerateAllocations(AZ::Debug::PrintAllocationsCB(true, true));
                }
            }
        }

    }// namespace Debug
} // namespace AZ
