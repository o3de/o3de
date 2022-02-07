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
#include <AzCore/Driller/DrillerBus.h>

#include <AzCore/std/time.h>
#include <AzCore/std/parallel/mutex.h>

#include <AzCore/Debug/StackTracer.h>

using namespace AZ;
using namespace AZ::Debug;

// Many PC tools break with alloc/free size mismatches when the memory guard is enabled.  Disable for now
//#define ENABLE_MEMORY_GUARD

//=========================================================================
// AllocationRecords
// [9/16/2009]
//=========================================================================
AllocationRecords::AllocationRecords(unsigned char stackRecordLevels, bool isMemoryGuard, bool isMarkUnallocatedMemory, const char* allocatorName)
    : m_mode(AllocatorManager::Instance().m_defaultTrackingRecordMode)
    , m_isAutoIntegrityCheck(false)
    , m_isMarkUnallocatedMemory(isMarkUnallocatedMemory)
    , m_saveNames(false)
    , m_decodeImmediately(false)
    , m_numStackLevels(stackRecordLevels)
    , m_requestedAllocs(0)
    , m_requestedBytes(0)
    , m_requestedBytesPeak(0)
    , m_allocatorName(allocatorName)
{
#if defined(ENABLE_MEMORY_GUARD)
    m_memoryGuardSize = isMemoryGuard ? sizeof(Debug::GuardValue) : 0;
#else
    (void)isMemoryGuard;
    m_memoryGuardSize = 0;
#endif
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
    SetCriticalSectionSpinCount(DrillerEBusMutex::GetMutex().native_handle(), 4000);
#endif
    // preallocate some buckets
    //m_records.rehash(20000);
};

//=========================================================================
// ~AllocationRecords
// [9/16/2009]
//=========================================================================
AllocationRecords::~AllocationRecords()
{
    if (!AllocatorManager::Instance().m_isAllocatorLeaking)
    {
        // dump all allocation (we should not have any at this point).
        bool includeNameAndFilename = (m_saveNames || m_mode == RECORD_FULL);
        EnumerateAllocations(PrintAllocationsCB(true, includeNameAndFilename));
        AZ_Error("Memory", m_records.empty(), "We still have %d allocations on record! They must be freed prior to destroy!", m_records.size());
    }
}

//=========================================================================
// lock
// [9/16/2009]
//=========================================================================
void
AllocationRecords::lock()
{
    DrillerEBusMutex::GetMutex().lock();
}

//=========================================================================
// try_lock
// [9/16/2009]
//=========================================================================
bool AllocationRecords::try_lock()
{
    return DrillerEBusMutex::GetMutex().try_lock();
}

//=========================================================================
// unlock
// [9/16/2009]
//=========================================================================
void
AllocationRecords::unlock()
{
    DrillerEBusMutex::GetMutex().unlock();
}

//=========================================================================
// RegisterAllocation
// [9/11/2009]
//=========================================================================
const AllocationInfo*
AllocationRecords::RegisterAllocation(void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount)
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
            IntegrityCheckNoLock();
        }

        AZ_Assert(byteSize>sizeof(Debug::GuardValue), "Did you forget to add the extra MemoryGuardSize() bytes?");
        byteSize -= sizeof(Debug::GuardValue);
        new(reinterpret_cast<char*>(address)+byteSize) Debug::GuardValue();
    }

    Debug::AllocationRecordsType::pair_iter_bool iterBool = m_records.insert_key(address);
    
    if (!iterBool.second)
    {
        // If that memory address was already registered, print the stack trace of the previous registration
        PrintAllocationsCB(true, (m_saveNames || m_mode == RECORD_FULL))(address, iterBool.first->second, m_numStackLevels);
        AZ_Assert(iterBool.second, "Memory address 0x%p is already allocated and in the records!", address);
    }

    Debug::AllocationInfo& ai = iterBool.first->second;
    ai.m_byteSize =  byteSize;
    ai.m_alignment = static_cast<unsigned int>(alignment);
    if ((m_saveNames || m_mode == RECORD_FULL) && name && fileName)
    {
        // In RECORD_FULL mode or when specifically enabled in app descriptor with 
        // m_allocationRecordsSaveNames, we allocate our own memory to save off name and fileName.
        // When testing for memory leaks, on process shutdown AllocationRecords::~AllocationRecords 
        // gets called to enumerate the remaining (leaked) allocations. Unfortunately, any names
        // referenced in dynamic module memory whose modules are unloaded won't be valid
        // references anymore and we won't get useful information from the enumeration print.
        // This code block ensures we keep our name/fileName valid for when we need it.
        const size_t nameLength = strlen(name);
        const size_t fileNameLength = strlen(fileName);
        const size_t totalLength = nameLength + fileNameLength + 2; // + 2 for terminating null characters
        ai.m_namesBlock = m_records.get_allocator().allocate(totalLength, 1);
        ai.m_namesBlockSize = totalLength;
        char* savedName = reinterpret_cast<char*>(ai.m_namesBlock);
        char* savedFileName = savedName + nameLength + 1;
        memcpy(reinterpret_cast<void*>(savedName), reinterpret_cast<const void*>(name), nameLength + 1);
        memcpy(reinterpret_cast<void*>(savedFileName), reinterpret_cast<const void*>(fileName), fileNameLength + 1);
        ai.m_name = savedName;
        ai.m_fileName = savedFileName;
    }
    else
    {
        ai.m_name = name;
        ai.m_fileName = fileName;
        ai.m_namesBlock = nullptr;
        ai.m_namesBlockSize = 0;
    }
    ai.m_lineNum = lineNum;
    ai.m_timeStamp = AZStd::GetTimeNowMicroSecond();

    // if we don't have a fileName,lineNum record the stack or if the user requested it.
    if ((fileName == nullptr && m_mode == RECORD_STACK_IF_NO_FILE_LINE) || m_mode == RECORD_FULL)
    {
        ai.m_stackFrames = m_numStackLevels ? reinterpret_cast<AZ::Debug::StackFrame*>(m_records.get_allocator().allocate(sizeof(AZ::Debug::StackFrame)*m_numStackLevels, 1)) : nullptr;
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
    m_requestedBytesPeak = AZStd::GetMax(m_requestedBytesPeak, m_requestedBytes);
    ++m_requestedAllocs;

    return &ai;
}

//=========================================================================
// UnregisterAllocation
// [9/11/2009]
//=========================================================================
void
AllocationRecords::UnregisterAllocation(void* address, size_t byteSize, size_t alignment, AllocationInfo* info)
{
    if (m_mode == RECORD_NO_RECORDS)
    {
        return;
    }
    if (address == nullptr)
    {
        return;
    }

    Debug::AllocationRecordsType::iterator iter = m_records.find(address);

    // We cannot assert if an allocation does not exist because our allocators start up way before the driller is started and the Allocator Records would be created.
    // It is currently impossible to actually track all allocations that happen before a certain point
    //AZ_Assert(iter!=m_records.end(), "Could not find address 0x%p in the allocator!", address);
    if (iter == m_records.end())
    {
        return;
    }
    AllocatorManager::Instance().DebugBreak(address, iter->second);

    (void)byteSize;
    (void)alignment;
    AZ_Assert(byteSize==0||byteSize==iter->second.m_byteSize, "Mismatched byteSize at deallocation! You supplied an invalid value!");
    AZ_Assert(alignment==0||alignment==iter->second.m_alignment, "Mismatched alignment at deallocation! You supplied an invalid value!");

    // statistics
    m_requestedBytes -= iter->second.m_byteSize;
    
#if defined(ENABLE_MEMORY_GUARD)
    // memory guard
    if (m_memoryGuardSize == sizeof(Debug::GuardValue))
    {
        if (m_isAutoIntegrityCheck)
        {
            // full integrity check
            IntegrityCheckNoLock();
        }
        else
        {
            // check current allocation
            char* guardAddress = reinterpret_cast<char*>(address)+iter->second.m_byteSize;
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
    }
#endif

    // delete allocation record
    if (iter->second.m_namesBlock)
    {
        m_records.get_allocator().deallocate(iter->second.m_namesBlock, iter->second.m_namesBlockSize, 1);
        iter->second.m_namesBlock = nullptr;
        iter->second.m_namesBlockSize = 0;
        iter->second.m_name = nullptr;
        iter->second.m_fileName = nullptr;
    }
    if (iter->second.m_stackFrames)
    {
        m_records.get_allocator().deallocate(iter->second.m_stackFrames, sizeof(AZ::Debug::StackFrame)*m_numStackLevels, 1);
        iter->second.m_stackFrames = nullptr;
    }

    if (info)
    {
        *info = iter->second;
    }

    m_records.erase(iter);

    // try to be more aggressive and keep the memory footprint low.
    // \todo store the load factor at the last rehash to avoid unnecessary rehash
    if (m_records.load_factor()<0.9f)
    {
        m_records.rehash(0);
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
void
AllocationRecords::ResizeAllocation(void* address, size_t newSize)
{
    if (m_mode == RECORD_NO_RECORDS)
    {
        return;
    }

    Debug::AllocationRecordsType::iterator iter = m_records.find(address);
    AZ_Assert(iter!=m_records.end(), "Could not find address 0x%p in the allocator!", address);
    AllocatorManager::Instance().DebugBreak(address, iter->second);
    
#if defined(ENABLE_MEMORY_GUARD)
    if (m_memoryGuardSize == sizeof(Debug::GuardValue))
    {
        if (m_isAutoIntegrityCheck)
        {
            // full integrity check
            IntegrityCheckNoLock();
        }
        else
        {
            // check memory guard
            char* guardAddress = reinterpret_cast<char*>(address)+iter->second.m_byteSize;
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
        new(reinterpret_cast<char*>(address)+newSize) Debug::GuardValue();
    }
#endif

    // statistics
    m_requestedBytes -= iter->second.m_byteSize;
    m_requestedBytes += newSize;
    m_requestedBytesPeak = AZStd::GetMax(m_requestedBytesPeak, m_requestedBytes);
    ++m_requestedAllocs;

    // update allocation size
    iter->second.m_byteSize = newSize;
}

//=========================================================================
// EnumerateAllocations
// [9/29/2009]
//=========================================================================
void
AllocationRecords::SetMode(Mode mode)
{
    DrillerEBusMutex::GetMutex().lock();

    if (mode==RECORD_NO_RECORDS)
    {
        m_records.clear();
        m_requestedBytes = 0;
        m_requestedBytesPeak = 0;
        m_requestedAllocs = 0;
    }

    AZ_Warning("Memory", m_mode!=RECORD_NO_RECORDS||mode==RECORD_NO_RECORDS, "Records recording was disabled and now it's enabled! You might get assert when you free memory, if a you have allocations which were not recorded!");

    m_mode = mode;

    DrillerEBusMutex::GetMutex().unlock();
}

//=========================================================================
// EnumerateAllocations
// [9/29/2009]
//=========================================================================
void
AllocationRecords::EnumerateAllocations(AllocationInfoCBType cb)
{
    DrillerEBusMutex::GetMutex().lock();
    // enumerate all allocations and stop if requested.
    // Since allocations can change during the iteration (code that prints out the records could allocate, which will
    // mutate m_records), we are going to make a copy and iterate the copy.
    const Debug::AllocationRecordsType recordsCopy = m_records;
    for (Debug::AllocationRecordsType::const_iterator iter = recordsCopy.begin(); iter != recordsCopy.end(); ++iter)
    {
        if (!cb(iter->first, iter->second, m_numStackLevels))
        {
            break;
        }
    }
    DrillerEBusMutex::GetMutex().unlock();
}

//=========================================================================
// IntegrityCheck
// [9/9/2011]
//=========================================================================
void
AllocationRecords::IntegrityCheck() const
{
    if (m_memoryGuardSize == sizeof(Debug::GuardValue))
    {
        DrillerEBusMutex::GetMutex().lock();

        IntegrityCheckNoLock();

        DrillerEBusMutex::GetMutex().unlock();
    }
}

//=========================================================================
// IntegrityCheckNoLock
// [9/13/2011]
//=========================================================================
void
AllocationRecords::IntegrityCheckNoLock() const
{
#if defined(ENABLE_MEMORY_GUARD)
    for (Debug::AllocationRecordsType::const_iterator iter = m_records.begin(); iter != m_records.end(); ++iter)
    {
        // check memory guard
        const char* guardAddress = reinterpret_cast<const char*>(iter->first)+ iter->second.m_byteSize;
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
#endif
}

//=========================================================================
// operator()
// [9/29/2009]
//=========================================================================
bool
PrintAllocationsCB::operator()(void* address, const AllocationInfo& info, unsigned char numStackLevels)
{
    if (m_includeNameAndFilename && info.m_name)
    {
        AZ_Printf("Memory", "Allocation Name: \"%s\" Addr: 0%p Size: %d Alignment: %d\n", info.m_name, address, info.m_byteSize, info.m_alignment);
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
            while (numStackLevels>0)
            {
                unsigned char numToDecode = AZStd::GetMin(decodeStep, numStackLevels);
                Debug::SymbolStorage::DecodeFrames(&info.m_stackFrames[iFrame], numToDecode, lines);
                for (unsigned char i = 0; i < numToDecode; ++i)
                {
                    if (info.m_stackFrames[iFrame+i].IsValid())
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
