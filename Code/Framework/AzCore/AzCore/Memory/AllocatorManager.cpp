/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/IAllocator.h>

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{

static EnvironmentVariable<AllocatorManager>& GetAllocatorManagerEnvVar()
{
    static EnvironmentVariable<AllocatorManager> s_allocManager;
    return s_allocManager;
}

static AllocatorManager* s_allocManagerDebug;  // For easier viewing in crash dumps

//////////////////////////////////////////////////////////////////////////
bool AllocatorManager::IsReady()
{
    return GetAllocatorManagerEnvVar().IsConstructed();
}
//////////////////////////////////////////////////////////////////////////
void AllocatorManager::Destroy()
{
}

//////////////////////////////////////////////////////////////////////////
// The only allocator manager instance.
AllocatorManager& AllocatorManager::Instance()
{
    auto& allocatorManager = GetAllocatorManagerEnvVar();
    if (!allocatorManager)
    {
        allocatorManager = Environment::CreateVariable<AllocatorManager>(AZ_CRC_CE("AZ::AllocatorManager::s_allocManager"));

        s_allocManagerDebug = &(*allocatorManager);
    }

    return *allocatorManager;
}
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// AllocatorManager
// [9/11/2009]
//=========================================================================
AllocatorManager::AllocatorManager()
    : m_profilingRefcount(0)
{
    m_numAllocators = 0;
    m_isAllocatorLeaking = false;
    m_defaultTrackingRecordMode = Debug::AllocationRecords::RECORD_NO_RECORDS;
}

//=========================================================================
// ~AllocatorManager
// [9/11/2009]
//=========================================================================
AllocatorManager::~AllocatorManager()
{
    InternalDestroy();
}

//=========================================================================
// RegisterAllocator
// [9/17/2009]
//=========================================================================
void
AllocatorManager::RegisterAllocator(class IAllocator* alloc)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    AZ_Assert(m_numAllocators < m_maxNumAllocators, "Too many allocators %d! Max is %d", m_numAllocators, m_maxNumAllocators);

    for (size_t i = 0; i < m_numAllocators; i++)
    {
        AZ_Assert(m_allocators[i] != alloc, "Allocator %s (%s) registered twice!", alloc->GetName());
    }

    m_allocators[m_numAllocators++] = alloc;
}

//=========================================================================
// InternalDestroy
// [11/7/2018]
//=========================================================================
void
AllocatorManager::InternalDestroy()
{
    while (m_numAllocators > 0)
    {
        IAllocator* allocator = m_allocators[m_numAllocators - 1];
        (void)allocator;
        m_allocators[--m_numAllocators] = nullptr;
        // Do not actually destroy the lazy allocator as it may have work to do during non-deterministic shutdown
    }

    if (!m_isAllocatorLeaking)
    {
        AZ_Assert(m_numAllocators == 0, "There are still %d registered allocators!", m_numAllocators);
    }
}

//=========================================================================
// UnRegisterAllocator
// [9/17/2009]
//=========================================================================
void
AllocatorManager::UnRegisterAllocator(class IAllocator* alloc)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

    for (int i = 0; i < m_numAllocators; ++i)
    {
        if (m_allocators[i] == alloc)
        {
            --m_numAllocators;
            m_allocators[i] = m_allocators[m_numAllocators];
        }
    }
}



//=========================================================================
// LockAllocators
// [11/21/2018]
//=========================================================================
AllocatorManager::AllocatorLock::~AllocatorLock()
{
}

AZStd::shared_ptr<AllocatorManager::AllocatorLock>
AllocatorManager::LockAllocators()
{
    class AllocatorLockImpl : public AllocatorLock
    {
    public:
        AllocatorLockImpl(AZStd::mutex& mutex) : m_lock(mutex)
        {
        }

        AZStd::unique_lock<AZStd::mutex> m_lock;
    };

    AZStd::shared_ptr<AllocatorLock> result = AZStd::allocate_shared<AllocatorLockImpl>(OSStdAllocator(), m_allocatorListMutex);

    return result;
}

//=========================================================================
// GarbageCollect
// [9/11/2009]
//=========================================================================
void
AllocatorManager::GarbageCollect()
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

    // Allocators can use other allocators. When this happens, the dependent
    // allocators are registered first. By running garbage collect on the
    // allocators in reverse order, the ones with no dependencies are garbage
    // collected first, which frees up more allocations for the dependent
    // allocators to release.
    for (int i = m_numAllocators - 1; i >= 0; --i)
    {
        m_allocators[i]->GarbageCollect();
    }
}

//=========================================================================
// AddOutOfMemoryListener
// [12/2/2010]
//=========================================================================
bool
AllocatorManager::AddOutOfMemoryListener(const OutOfMemoryCBType& cb)
{
    AZ_Warning("Memory", !m_outOfMemoryListener, "Out of memory listener was already installed!");
    if (!m_outOfMemoryListener)
    {
        m_outOfMemoryListener = cb;
        return true;
    }

    return false;
}

//=========================================================================
// RemoveOutOfMemoryListener
// [12/2/2010]
//=========================================================================
void
AllocatorManager::RemoveOutOfMemoryListener()
{
    m_outOfMemoryListener = nullptr;
}

//=========================================================================
// SetTrackingMode
// [9/16/2011]
//=========================================================================
void
AllocatorManager::SetTrackingMode(Debug::AllocationRecords::Mode mode)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    for (int i = 0; i < m_numAllocators; ++i)
    {
        Debug::AllocationRecords* records = m_allocators[i]->GetRecords();
        if (records)
        {
            records->SetMode(mode);
        }
    }
}

void
AllocatorManager::EnterProfilingMode()
{
}

void
AllocatorManager::ExitProfilingMode()
{
}

void
AllocatorManager::DumpAllocators()
{
    static const char TAG[] = "mem";

    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    size_t totalUsedBytes = 0;
    size_t totalReservedBytes = 0;
    size_t totalConsumedBytes = 0;

    memset(m_dumpInfo, 0, sizeof(m_dumpInfo));

    AZ_Printf(TAG, "%d allocators active\n", m_numAllocators);
    AZ_Printf(TAG, "Index,Name,Used kb,Reserved kb,Consumed kb\n");

    for (int i = 0; i < m_numAllocators; i++)
    {
        IAllocator* allocator = GetAllocator(i);
        const char* name = allocator->GetName();
        size_t usedBytes = allocator->NumAllocatedBytes();
        size_t reservedBytes = allocator->Capacity();
        size_t consumedBytes = reservedBytes;

        totalUsedBytes += usedBytes;
        totalReservedBytes += reservedBytes;
        totalConsumedBytes += consumedBytes;
        m_dumpInfo[i].m_name = name;
        m_dumpInfo[i].m_used = usedBytes;
        m_dumpInfo[i].m_reserved = reservedBytes;
        m_dumpInfo[i].m_consumed = consumedBytes;
        AZ_Printf(TAG, "%d,%s,%.2f,%.2f,%.2f\n", i, name, usedBytes / 1024.0f, reservedBytes / 1024.0f, consumedBytes / 1024.0f);
    }

    AZ_Printf(TAG, "-,Totals,%.2f,%.2f,%.2f\n", totalUsedBytes / 1024.0f, totalReservedBytes / 1024.0f, totalConsumedBytes / 1024.0f);
}
void AllocatorManager::GetAllocatorStats(size_t& allocatedBytes, size_t& capacityBytes, AZStd::vector<AllocatorStats>* outStats)
{
    allocatedBytes = 0;
    capacityBytes = 0;

    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    const int allocatorCount = GetNumAllocators();

    // Build a mapping of original allocator sources to their allocators
    for (int i = 0; i < allocatorCount; ++i)
    {
        IAllocator* allocator = GetAllocator(i);
        allocatedBytes += allocator->NumAllocatedBytes();
        capacityBytes += allocator->Capacity();   
           
        if (outStats)
        {
            outStats->emplace(outStats->end(),
                allocator->GetName(),
                allocator->NumAllocatedBytes(),
                allocator->Capacity());
        }
    }
}

//=========================================================================
// MemoryBreak
// [2/24/2011]
//=========================================================================
AllocatorManager::MemoryBreak::MemoryBreak()
{
    addressStart = nullptr;
    addressEnd = nullptr;
    byteSize = 0;
    alignment = static_cast<size_t>(0xffffffff);
    name = nullptr;

    fileName = nullptr;
    lineNum = -1;
}

//=========================================================================
// SetMemoryBreak
// [2/24/2011]
//=========================================================================
void
AllocatorManager::SetMemoryBreak(int slot, const MemoryBreak& mb)
{
    AZ_Assert(slot < MaxNumMemoryBreaks, "Invalid slot index");
    m_activeBreaks |= 1 << slot;
    m_memoryBreak[slot] = mb;
}

//=========================================================================
// ResetMemoryBreak
// [2/24/2011]
//=========================================================================
void
AllocatorManager::ResetMemoryBreak(int slot)
{
    if (slot == -1)
    {
        m_activeBreaks = 0;
    }
    else
    {
        AZ_Assert(slot < MaxNumMemoryBreaks, "Invalid slot index");
        m_activeBreaks &= ~(1 << slot);
    }
}

//=========================================================================
// DebugBreak
// [2/24/2011]
//=========================================================================
void
AllocatorManager::DebugBreak(void* address, const Debug::AllocationInfo& info)
{
    (void)address;
    (void)info;
    if (m_activeBreaks)
    {
        void* addressEnd = (char*)address + info.m_byteSize;
        (void)addressEnd;
        for (int i = 0; i < MaxNumMemoryBreaks; ++i)
        {
            if ((m_activeBreaks & (1 << i)) != 0)
            {
                // check address range
                AZ_Assert(!((address <= m_memoryBreak[i].addressStart && m_memoryBreak[i].addressStart < addressEnd) ||
                            (address < m_memoryBreak[i].addressEnd && m_memoryBreak[i].addressEnd <= addressEnd) ||
                            (address >= m_memoryBreak[i].addressStart && addressEnd <= m_memoryBreak[i].addressEnd)),
                    "User triggered breakpoint - address overlap [%p,%p] with [%p,%p]", address, addressEnd, m_memoryBreak[i].addressStart, m_memoryBreak[i].addressEnd);

                AZ_Assert(!(m_memoryBreak[i].addressStart <= address && address < m_memoryBreak[i].addressEnd), "User triggered breakpoint - address overlap [%p,%p] with [%p,%p]", address, addressEnd, m_memoryBreak[i].addressStart, m_memoryBreak[i].addressEnd);
                AZ_Assert(!(m_memoryBreak[i].addressStart < addressEnd && addressEnd <= m_memoryBreak[i].addressEnd), "User triggered breakpoint - address overlap [%p,%p] with [%p,%p]", address, addressEnd, m_memoryBreak[i].addressStart, m_memoryBreak[i].addressEnd);


                AZ_Assert(!(m_memoryBreak[i].alignment == info.m_alignment), "User triggered breakpoint - alignment (%d)", info.m_alignment);
                AZ_Assert(!(m_memoryBreak[i].byteSize == info.m_byteSize), "User triggered breakpoint - allocation size (%d)", info.m_byteSize);
                AZ_Assert(!(info.m_name != nullptr && m_memoryBreak[i].name != nullptr && strcmp(m_memoryBreak[i].name, info.m_name) == 0), "User triggered breakpoint - name \"%s\"", info.m_name);
                if (m_memoryBreak[i].lineNum != 0)
                {
                    AZ_Assert(!(info.m_fileName != nullptr && m_memoryBreak[i].fileName != nullptr && strcmp(m_memoryBreak[i].fileName, info.m_fileName) == 0 && m_memoryBreak[i].lineNum == info.m_lineNum), "User triggered breakpoint - file/line number : %s(%d)", info.m_fileName, info.m_lineNum);
                }
                else
                {
                    AZ_Assert(!(info.m_fileName != nullptr && m_memoryBreak[i].fileName != nullptr && strcmp(m_memoryBreak[i].fileName, info.m_fileName) == 0), "User triggered breakpoint - file name \"%s\"", info.m_fileName);
                }
            }
        }
    }
}

} // namespace AZ
