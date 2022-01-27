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
#include <AzCore/Memory/MallocSchema.h>

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/array.h>

namespace AZ::Internal
{
    struct AMStringHasher
    {
        using is_transparent = void;
        template<typename ConvertibleToStringView>
        size_t operator()(const ConvertibleToStringView& key)
        {
            return AZStd::hash<AZStd::string_view>{}(key);
        }
    };
    using AMString = AZStd::basic_string<char, AZStd::char_traits<char>, AZStdIAllocator>;
    using AllocatorNameMap = AZStd::unordered_map<AMString, IAllocator*, AMStringHasher, AZStd::equal_to<>, AZStdIAllocator>;
    using AllocatorRemappings = AZStd::unordered_map<AMString, AMString, AMStringHasher, AZStd::equal_to<>, AZStdIAllocator>;

    // For allocators that are created before we have an environment, we keep some module-local data for them so that we can register them
    // properly once the environment is attached.
    struct PreEnvironmentAttachData
    {
        static const int MAX_UNREGISTERED_ALLOCATORS = 8;
        AZStd::mutex m_mutex;
        MallocSchema m_mallocSchema;
        IAllocator* m_unregisteredAllocators[MAX_UNREGISTERED_ALLOCATORS];
        int m_unregisteredAllocatorCount = 0;
    };

}

namespace AZ
{

static EnvironmentVariable<AllocatorManager> s_allocManager = nullptr;
static AllocatorManager* s_allocManagerDebug = nullptr;  // For easier viewing in crash dumps

/// Returns a module-local instance of data to use for allocators that are created before the environment is attached.
static Internal::PreEnvironmentAttachData& GetPreEnvironmentAttachData()
{
    static Internal::PreEnvironmentAttachData s_data;

    return s_data;
}

/// Called to register an allocator before AllocatorManager::Instance() is available.
void AllocatorManager::PreRegisterAllocator(IAllocator* allocator)
{
    auto& data = GetPreEnvironmentAttachData();
    {
        AZStd::lock_guard<AZStd::mutex> lock(data.m_mutex);
        AZ_Assert(data.m_unregisteredAllocatorCount < Internal::PreEnvironmentAttachData::MAX_UNREGISTERED_ALLOCATORS, "Too many allocators trying to register before environment attached!");
        data.m_unregisteredAllocators[data.m_unregisteredAllocatorCount++] = allocator;
    }
}

IAllocator* AllocatorManager::CreateLazyAllocator(size_t size, size_t alignment, IAllocator*(*creationFn)(void*))
{
    void* mem = GetPreEnvironmentAttachData().m_mallocSchema.Allocate(size, alignment, 0);
    IAllocator* result = creationFn(mem);

    return result;
}

//////////////////////////////////////////////////////////////////////////
bool AllocatorManager::IsReady()
{
    return s_allocManager.IsConstructed();
}
//////////////////////////////////////////////////////////////////////////
void AllocatorManager::Destroy()
{
    if (s_allocManager)
    {
        s_allocManager->InternalDestroy();
        s_allocManager.Reset();
    }
}

//////////////////////////////////////////////////////////////////////////
// The only allocator manager instance.
AllocatorManager& AllocatorManager::Instance()
{
    if (!s_allocManager)
    {
        AZ_Assert(Environment::IsReady(), "Environment must be ready before calling Instance()");
        s_allocManager = Environment::CreateVariable<AllocatorManager>(AZ_CRC_CE("AZ::AllocatorManager::s_allocManager"));

        // Register any allocators that were created in this module before we attached to the environment
        auto& data = GetPreEnvironmentAttachData();

        {
            AZStd::lock_guard<AZStd::mutex> lock(data.m_mutex);
            for (int idx = 0; idx < data.m_unregisteredAllocatorCount; idx++)
            {
                s_allocManager->RegisterAllocator(data.m_unregisteredAllocators[idx]);
            }

            data.m_unregisteredAllocatorCount = 0;
        
        }

        s_allocManagerDebug = &(*s_allocManager);
    }

    return *s_allocManager;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Create malloc schema using custom AZ_OS_MALLOC allocator.
MallocSchema* AllocatorManager::CreateMallocSchema()
{
    return static_cast<MallocSchema*>(new(AZ_OS_MALLOC(sizeof(MallocSchema), alignof(MallocSchema))) MallocSchema());
}


//=========================================================================
// AllocatorManager
// [9/11/2009]
//=========================================================================
AllocatorManager::AllocatorManager()
    : m_profilingRefcount(0)
    , m_mallocSchema(CreateMallocSchema(), [](MallocSchema* schema)
    {
        if (schema)
        {
            schema->~MallocSchema();
            AZ_OS_FREE(schema);
        }
    }
)
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
        AZ_Assert(m_allocators[i] != alloc, "Allocator %s (%s) registered twice!", alloc->GetName(), alloc->GetDescription());
    }

    alloc->SetProfilingActive(m_profilingRefcount.load() > 0);

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
        AZ_Assert(allocator->IsLazilyCreated(), "Manually created allocator '%s (%s)' must be manually destroyed before shutdown", allocator->GetName(), allocator->GetDescription());
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

    for (int i = 0; i < m_numAllocators; ++i)
    {
        m_allocators[i]->GetSchema()->GarbageCollect();
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
    if (!m_profilingRefcount++)
    {
        // We were at 0, so enable profiling
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

        for (int i = 0; i < m_numAllocators; ++i)
        {
            m_allocators[i]->SetProfilingActive(true);
        }
    }
}

void
AllocatorManager::ExitProfilingMode()
{
    if (!--m_profilingRefcount)
    {
        // We've gone down to 0, so disable profiling
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

        for (int i = 0; i < m_numAllocators; ++i)
        {
            m_allocators[i]->SetProfilingActive(false);
        }
    }

    AZ_Assert(m_profilingRefcount.load() >= 0, "ExitProfilingMode called without matching EnterProfilingMode");
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
                allocator->GetDescription(), 
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
