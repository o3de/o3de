/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/AllocatorManager.h>

#include <AzCore/std/time.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/scoped_lock.h>

#include <AzCore/Debug/StackTracer.h>

namespace AZ::Debug
{
    // Many PC tools break with alloc/free size mismatches when the memory guard is enabled.  Disable for now
    //#define ENABLE_MEMORY_GUARD

    //=========================================================================
    // AllocationRecords
    // [9/16/2009]
    //=========================================================================
    AllocationRecords::AllocationRecords(
        unsigned char stackRecordLevels, [[maybe_unused]] bool isMemoryGuard, bool isMarkUnallocatedMemory, const char* allocatorName)
        : m_mode(AllocatorManager::Instance().m_defaultTrackingRecordMode)
        , m_isAutoIntegrityCheck(false)
        , m_isMarkUnallocatedMemory(isMarkUnallocatedMemory)
        , m_saveNames(false)
        , m_decodeImmediately(false)
        , m_numStackLevels(stackRecordLevels)
#if defined(ENABLE_MEMORY_GUARD)
        , m_memoryGuardSize(isMemoryGuard ? sizeof(Debug::GuardValue) : 0)
#else
        , m_memoryGuardSize(0)
#endif
        , m_requestedAllocs(0)
        , m_requestedBytes(0)
        , m_requestedBytesPeak(0)
        , m_allocatorName(allocatorName)
    {
    }

    //=========================================================================
    // lock
    // [9/16/2009]
    //=========================================================================
    void AllocationRecords::lock()
    {
        m_recordsMutex.lock();
    }

    //=========================================================================
    // try_lock
    // [9/16/2009]
    //=========================================================================
    bool AllocationRecords::try_lock()
    {
        return m_recordsMutex.try_lock();
    }

    //=========================================================================
    // unlock
    // [9/16/2009]
    //=========================================================================
    void AllocationRecords::unlock()
    {
        m_recordsMutex.unlock();
    }

    //=========================================================================
    // RegisterAllocation
    // [9/11/2009]
    //=========================================================================
    const AllocationInfo* AllocationRecords::RegisterAllocation(
        void* address,
        size_t byteSize,
        size_t alignment,
        unsigned int stackSuppressCount)
    {
        (void)stackSuppressCount;
        if (m_mode == RECORD_NO_RECORDS)
        {
            return nullptr;
        }
        if (address == nullptr)
        {
            return nullptr;
        }

        // memory guard
        if (m_memoryGuardSize == sizeof(Debug::GuardValue))
        {
            if (m_isAutoIntegrityCheck)
            {
                IntegrityCheck();
            }

            AZ_Assert(byteSize > sizeof(Debug::GuardValue), "Did you forget to add the extra MemoryGuardSize() bytes?");
            byteSize -= sizeof(Debug::GuardValue);
            new (reinterpret_cast<char*>(address) + byteSize) Debug::GuardValue();
        }

        Debug::AllocationRecordsType::pair_iter_bool iterBool;
        {
            AZStd::scoped_lock lock(m_recordsMutex);
            iterBool = m_records.insert_key(address);
        }

        if (!iterBool.second)
        {
            // If that memory address was already registered, print the stack trace of the previous registration
            PrintAllocationsCB(true, (m_saveNames || m_mode == RECORD_FULL))(address, iterBool.first->second, m_numStackLevels);
            AZ_Assert(iterBool.second, "Memory address 0x%p is already allocated and in the records!", address);
        }

        Debug::AllocationInfo& ai = iterBool.first->second;
        ai.m_byteSize = byteSize;
        ai.m_alignment = static_cast<unsigned int>(alignment);
        ai.m_name = nullptr;
        ai.m_fileName = nullptr;
        ai.m_namesBlock = nullptr;
        ai.m_namesBlockSize = 0;
        ai.m_lineNum = 0;
        ai.m_timeStamp = AZStd::GetTimeNowMicroSecond();

        // if we don't have a fileName,lineNum record the stack or if the user requested it.
        if (m_mode == RECORD_STACK_IF_NO_FILE_LINE || m_mode == RECORD_FULL)
        {
            ai.m_stackFrames = m_numStackLevels ? reinterpret_cast<AZ::Debug::StackFrame*>(m_records.get_allocator().allocate(
                                                      sizeof(AZ::Debug::StackFrame) * m_numStackLevels, 1))
                                                : nullptr;
            if (ai.m_stackFrames)
            {
                Debug::StackRecorder::Record(ai.m_stackFrames, m_numStackLevels, stackSuppressCount + 1);

                if (m_decodeImmediately)
                {
                    // OPTIONAL DEBUGGING CODE - enable in app descriptor m_allocationRecordsAttemptDecodeImmediately
                    // This is optionally-enabled code for tracking down memory allocations
                    // that fail to be decoded. DecodeFrames() typically runs at the end of
                    // your application when leaks were found. Sometimes you have stack prints
                    // full of "(module-name not available)" and "(function-name not available)"
                    // that are not actionable. If you have those, enable this code. It'll slow
                    // down your process significantly because for every allocation recorded
                    // we get the stack trace on the spot. Put a breakpoint in DecodeFrames()
                    // at the "(module-name not available)" and "(function-name not available)"
                    // locations and now at the moment those allocations happen you'll have the
                    // full stack trace available and the ability to debug what could be causing it
                    {
                        const unsigned char decodeStep = 40;
                        Debug::SymbolStorage::StackLine lines[decodeStep];
                        unsigned char iFrame = 0;
                        unsigned char numStackLevels = m_numStackLevels;
                        while (numStackLevels > 0)
                        {
                            unsigned char numToDecode = AZStd::GetMin(decodeStep, numStackLevels);
                            Debug::SymbolStorage::DecodeFrames(&ai.m_stackFrames[iFrame], numToDecode, lines);
                            numStackLevels -= numToDecode;
                            iFrame += numToDecode;
                        }
                    }
                }
            }
        }

        AllocatorManager::Instance().DebugBreak(address, ai);

        // statistics
        m_requestedBytes += byteSize;

        size_t currentRequestedBytePeak;
        size_t newRequestedBytePeak;
        do
        {
            currentRequestedBytePeak = m_requestedBytesPeak.load(std::memory_order::memory_order_relaxed);
            newRequestedBytePeak = AZStd::GetMax(currentRequestedBytePeak, m_requestedBytes.load(std::memory_order::memory_order_relaxed));
        } while (!m_requestedBytesPeak.compare_exchange_weak(currentRequestedBytePeak, newRequestedBytePeak));

        ++m_requestedAllocs;

        return &ai;
    }

    //=========================================================================
    // UnregisterAllocation
    // [9/11/2009]
    //=========================================================================
    void AllocationRecords::UnregisterAllocation(void* address, size_t byteSize, size_t alignment, AllocationInfo* info)
    {
        if (m_mode == RECORD_NO_RECORDS)
        {
            return;
        }
        if (address == nullptr)
        {
            return;
        }

        AllocationInfo allocationInfo;
        {
            AZStd::scoped_lock lock(m_recordsMutex);
            Debug::AllocationRecordsType::iterator iter = m_records.find(address);
            // We cannot assert if an allocation does not exist because allocations may have been made before tracking was enabled.
            // It is currently impossible to actually track all allocations that happen before a certain point
            // AZ_Assert(iter!=m_records.end(), "Could not find address 0x%p in the allocator!", address);
            if (iter == m_records.end())
            {
                return;
            }
            allocationInfo = iter->second;
            m_records.erase(iter);

            // try to be more aggressive and keep the memory footprint low.
            // \todo store the load factor at the last rehash to avoid unnecessary rehash
            if (m_records.load_factor() < 0.9f)
            {
                m_records.rehash(0);
            }
        }

        AllocatorManager::Instance().DebugBreak(address, allocationInfo);

        (void)byteSize;
        (void)alignment;
        AZ_Assert(
            byteSize == 0 || byteSize == allocationInfo.m_byteSize, "Mismatched byteSize at deallocation! You supplied an invalid value!");
        AZ_Assert(
            alignment == 0 || alignment == allocationInfo.m_alignment,
            "Mismatched alignment at deallocation! You supplied an invalid value!");

        // statistics
        m_requestedBytes -= allocationInfo.m_byteSize;

#if defined(ENABLE_MEMORY_GUARD)
        // memory guard
        if (m_memoryGuardSize == sizeof(Debug::GuardValue))
        {
            if (m_isAutoIntegrityCheck)
            {
                // full integrity check
                IntegrityCheck();
            }
            else
            {
                // check current allocation
                char* guardAddress = reinterpret_cast<char*>(address) + allocationInfo.m_byteSize;
                Debug::GuardValue* guard = reinterpret_cast<Debug::GuardValue*>(guardAddress);
                if (!guard->Validate())
                {
                    AZ_Printf("Memory", "Memory stomp located at address %p, part of allocation:", guardAddress);
                    PrintAllocationsCB printAlloc(true);
                    printAlloc(address, allocationInfo, m_numStackLevels);
                    AZ_Assert(false, "MEMORY STOMP DETECTED!!!");
                }
                guard->~GuardValue();
            }
        }
#endif

        // delete allocation record
        if (allocationInfo.m_namesBlock)
        {
            m_records.get_allocator().deallocate(allocationInfo.m_namesBlock, allocationInfo.m_namesBlockSize, 1);
            allocationInfo.m_namesBlock = nullptr;
            allocationInfo.m_namesBlockSize = 0;
            allocationInfo.m_name = nullptr;
            allocationInfo.m_fileName = nullptr;
        }
        if (allocationInfo.m_stackFrames)
        {
            m_records.get_allocator().deallocate(allocationInfo.m_stackFrames, sizeof(AZ::Debug::StackFrame) * m_numStackLevels, 1);
            allocationInfo.m_stackFrames = nullptr;
        }

        if (info)
        {
            *info = allocationInfo;
        }

        // if requested set memory to a specific value.
        if (m_isMarkUnallocatedMemory)
        {
            memset(address, GetUnallocatedMarkValue(), byteSize);
        }
    }

    //=========================================================================
    // ResizeAllocation
    // [9/20/2009]
    //=========================================================================
    void AllocationRecords::ResizeAllocation(void* address, size_t newSize)
    {
        if (m_mode == RECORD_NO_RECORDS)
        {
            return;
        }

        AllocationInfo* allocationInfo;
        {
            AZStd::scoped_lock lock(m_recordsMutex);
            Debug::AllocationRecordsType::iterator iter = m_records.find(address);
            AZ_Assert(iter != m_records.end(), "Could not find address 0x%p in the allocator!", address);
            allocationInfo = &iter->second;
        }
        AllocatorManager::Instance().DebugBreak(address, *allocationInfo);

#if defined(ENABLE_MEMORY_GUARD)
        if (m_memoryGuardSize == sizeof(Debug::GuardValue))
        {
            if (m_isAutoIntegrityCheck)
            {
                // full integrity check
                IntegrityCheck();
            }
            else
            {
                // check memory guard
                char* guardAddress = reinterpret_cast<char*>(address) + allocationInfo->m_byteSize;
                Debug::GuardValue* guard = reinterpret_cast<Debug::GuardValue*>(guardAddress);
                if (!guard->Validate())
                {
                    AZ_Printf("Memory", "Memory stomp located at address %p, part of allocation:", guardAddress);
                    PrintAllocationsCB printAlloc(true);
                    printAlloc(address, iter->second, m_numStackLevels);
                    AZ_Assert(false, "MEMORY STOMP DETECTED!!!");
                }
                guard->~GuardValue();
            }
            // init the new memory guard
            newSize -= sizeof(Debug::GuardValue);
            new (reinterpret_cast<char*>(address) + newSize) Debug::GuardValue();
        }
#endif

        // statistics
        m_requestedBytes -= allocationInfo->m_byteSize;
        m_requestedBytes += newSize;
        size_t currentRequestedBytePeak;
        size_t newRequestedBytePeak;
        do
        {
            currentRequestedBytePeak = m_requestedBytesPeak.load(std::memory_order::memory_order_relaxed);
            newRequestedBytePeak = AZStd::GetMax(currentRequestedBytePeak, m_requestedBytes.load(std::memory_order::memory_order_relaxed));
        } while (!m_requestedBytesPeak.compare_exchange_weak(currentRequestedBytePeak, newRequestedBytePeak));
        ++m_requestedAllocs;

        // update allocation size
        allocationInfo->m_byteSize = newSize;
    }

    void AllocationRecords::RegisterReallocation(void* address, void* newAddress, size_t byteSize, size_t alignment, unsigned int stackSuppressCount)
    {
        if (m_mode == RECORD_NO_RECORDS)
        {
            return;
        }
        if (!address && !newAddress)
        {
            return;
        }
        if (!address)
        {
            RegisterAllocation(newAddress, byteSize, alignment, stackSuppressCount);
            return;
        }

        Debug::AllocationInfo* ai{};
        {
            AZStd::scoped_lock lock(m_recordsMutex);
            auto node = m_records.extract(address);
            if (node.empty())
            {
                RegisterAllocation(newAddress, byteSize, alignment, stackSuppressCount);
                return;
            }

            // Make a best effort to avoid reallocations from mutating the
            // records map when recording a reallocation
            node.key() = newAddress;
            ai = &m_records.insert(AZStd::move(node)).position->second;
        }

        m_requestedBytes += (byteSize - ai->m_byteSize);

        ai->m_byteSize = byteSize;
        ai->m_alignment = static_cast<unsigned int>(alignment);
        ai->m_name = nullptr;
        ai->m_fileName = nullptr;
        ai->m_namesBlock = nullptr;
        ai->m_namesBlockSize = 0;
        ai->m_lineNum = 0;
        ai->m_timeStamp = AZStd::GetTimeNowMicroSecond();

        // if we don't have a fileName,lineNum record the stack or if the user requested it.
        if (m_mode == RECORD_STACK_IF_NO_FILE_LINE || m_mode == RECORD_FULL)
        {
            if (ai->m_stackFrames)
            {
                Debug::StackRecorder::Record(ai->m_stackFrames, m_numStackLevels, stackSuppressCount + 1);

                if (m_decodeImmediately)
                {
                    // OPTIONAL DEBUGGING CODE - enable in app descriptor m_allocationRecordsAttemptDecodeImmediately
                    // This is optionally-enabled code for tracking down memory allocations
                    // that fail to be decoded. DecodeFrames() typically runs at the end of
                    // your application when leaks were found. Sometimes you have stack prints
                    // full of "(module-name not available)" and "(function-name not available)"
                    // that are not actionable. If you have those, enable this code. It'll slow
                    // down your process significantly because for every allocation recorded
                    // we get the stack trace on the spot. Put a breakpoint in DecodeFrames()
                    // at the "(module-name not available)" and "(function-name not available)"
                    // locations and now at the moment those allocations happen you'll have the
                    // full stack trace available and the ability to debug what could be causing it
                    {
                        const unsigned char decodeStep = 40;
                        Debug::SymbolStorage::StackLine lines[decodeStep];
                        unsigned char iFrame = 0;
                        unsigned char numStackLevels = m_numStackLevels;
                        while (numStackLevels > 0)
                        {
                            unsigned char numToDecode = AZStd::GetMin(decodeStep, numStackLevels);
                            Debug::SymbolStorage::DecodeFrames(&ai->m_stackFrames[iFrame], numToDecode, lines);
                            numStackLevels -= numToDecode;
                            iFrame += numToDecode;
                        }
                    }
                }
            }
        }

        AllocatorManager::Instance().DebugBreak(address, *ai);

        size_t currentRequestedBytePeak;
        size_t newRequestedBytePeak;
        do
        {
            currentRequestedBytePeak = m_requestedBytesPeak.load(std::memory_order::memory_order_relaxed);
            newRequestedBytePeak = AZStd::GetMax(currentRequestedBytePeak, m_requestedBytes.load(std::memory_order::memory_order_relaxed));
        } while (!m_requestedBytesPeak.compare_exchange_weak(currentRequestedBytePeak, newRequestedBytePeak));
    }

    //=========================================================================
    // EnumerateAllocations
    // [9/29/2009]
    //=========================================================================
    void AllocationRecords::SetMode(Mode mode)
    {
        if (mode == RECORD_NO_RECORDS)
        {
            {
                AZStd::scoped_lock lock(m_recordsMutex);
                m_records.clear();
            }
            m_requestedBytes = 0;
            m_requestedBytesPeak = 0;
            m_requestedAllocs = 0;
        }

        AZ_Warning(
            "Memory", m_mode != RECORD_NO_RECORDS || mode == RECORD_NO_RECORDS,
            "Records recording was disabled and now it's enabled! You might get assert when you free memory, if a you have allocations "
            "which were not recorded!");

        m_mode = mode;
    }

    //=========================================================================
    // EnumerateAllocations
    // [9/29/2009]
    //=========================================================================
    void AllocationRecords::EnumerateAllocations(AllocationInfoCBType cb)
    {
        // enumerate all allocations and stop if requested.
        // Since allocations can change during the iteration (code that prints out the records could allocate, which will
        // mutate m_records), we are going to make a copy and iterate the copy.
        Debug::AllocationRecordsType recordsCopy;
        {
            AZStd::scoped_lock lock(m_recordsMutex);
            recordsCopy = m_records;
        }
        for (Debug::AllocationRecordsType::const_iterator iter = recordsCopy.begin(); iter != recordsCopy.end(); ++iter)
        {
            if (!cb(iter->first, iter->second, m_numStackLevels))
            {
                break;
            }
        }
    }

    //=========================================================================
    // IntegrityCheck
    // [9/9/2011]
    //=========================================================================
    void AllocationRecords::IntegrityCheck() const
    {
#if defined(ENABLE_MEMORY_GUARD)
        if (m_memoryGuardSize == sizeof(Debug::GuardValue))
        {
            Debug::AllocationRecordsType recordsCopy;
            {
                AZStd::scoped_lock lock(m_recordsMutex);
                recordsCopy = m_records;
            }
            for (Debug::AllocationRecordsType::const_iterator iter = recordsCopy.begin(); iter != recordsCopy.end(); ++iter)
            {
                // check memory guard
                const char* guardAddress = reinterpret_cast<const char*>(iter->first) + iter->second.m_byteSize;
                if (!reinterpret_cast<const Debug::GuardValue*>(guardAddress)->Validate())
                {
                    // We have to turn off the integrity check at this point if we want to succesfully report the memory
                    // stomp we just found. If we don't turn this off, the printf just winds off the stack as each memory
                    // allocation done therein recurses this same code.
                    *const_cast<bool*>(&m_isAutoIntegrityCheck) = false;
                    AZ_Printf("Memory", "Memory stomp located at address %p, part of allocation:", guardAddress);
                    PrintAllocationsCB printAlloc(true);
                    printAlloc(iter->first, iter->second, m_numStackLevels);
                    AZ_Error("Memory", false, "MEMORY STOMP DETECTED!!!");
                }
            }
        }
#endif
    }

    //=========================================================================
    // operator()
    // [9/29/2009]
    //=========================================================================
    bool PrintAllocationsCB::operator()(void* address, const AllocationInfo& info, unsigned char numStackLevels)
    {
        if (m_includeNameAndFilename && info.m_name)
        {
            AZ_Printf(
                "Memory", "Allocation Name: \"%s\" Addr: 0%p Size: %d Alignment: %d\n", info.m_name, address, info.m_byteSize,
                info.m_alignment);
        }
        else
        {
            AZ_Printf("Memory", "Allocation Addr: 0%p Size: %d Alignment: %d\n", address, info.m_byteSize, info.m_alignment);
        }

        if (m_isDetailed)
        {
            if (!info.m_stackFrames)
            {
                AZ_Printf("Memory", " %s (%d)\n", info.m_fileName, info.m_lineNum);
            }
            else
            {
                // Allocation callstack
                const unsigned char decodeStep = 40;
                Debug::SymbolStorage::StackLine lines[decodeStep];
                unsigned char iFrame = 0;
                while (numStackLevels > 0)
                {
                    unsigned char numToDecode = AZStd::GetMin(decodeStep, numStackLevels);
                    Debug::SymbolStorage::DecodeFrames(&info.m_stackFrames[iFrame], numToDecode, lines);
                    for (unsigned char i = 0; i < numToDecode; ++i)
                    {
                        if (info.m_stackFrames[iFrame + i].IsValid())
                        {
                            AZ_Printf("Memory", " %s\n", lines[i]);
                        }
                    }
                    numStackLevels -= numToDecode;
                    iFrame += numToDecode;
                }
            }
        }
        return true; // continue enumerating
    }

} // namespace AZ::Debug
