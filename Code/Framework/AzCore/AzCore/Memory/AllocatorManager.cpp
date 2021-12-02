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
#include <AzCore/Memory/AllocatorOverrideShim.h>
#include <AzCore/Memory/MallocSchema.h>
#include <AzCore/Memory/MemoryDrillerBus.h>

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/array.h>

using namespace AZ;

#if !defined(RELEASE) && !defined(AZCORE_MEMORY_ENABLE_OVERRIDES)
#   define AZCORE_MEMORY_ENABLE_OVERRIDES
#endif

namespace AZ
{
    namespace Internal
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
}

struct AZ::AllocatorManager::InternalData
{
    explicit InternalData(const AZStdIAllocator& alloc)
        : m_allocatorMap(alloc)
        , m_remappings(alloc)
        , m_remappingsReverse(alloc)
    {}
    Internal::AllocatorNameMap m_allocatorMap;
    Internal::AllocatorRemappings m_remappings;
    Internal::AllocatorRemappings m_remappingsReverse;
};

static AZ::EnvironmentVariable<AllocatorManager> s_allocManager = nullptr;
static AllocatorManager* s_allocManagerDebug = nullptr;  // For easier viewing in crash dumps

/// Returns a module-local instance of data to use for allocators that are created before the environment is attached.
static AZ::Internal::PreEnvironmentAttachData& GetPreEnvironmentAttachData()
{
    static AZ::Internal::PreEnvironmentAttachData s_data;

    return s_data;
}

/// Called to register an allocator before AllocatorManager::Instance() is available.
void AllocatorManager::PreRegisterAllocator(IAllocator* allocator)
{
    auto& data = GetPreEnvironmentAttachData();

#ifdef AZCORE_MEMORY_ENABLE_OVERRIDES
    // All allocators must switch to an OverrideEnabledAllocationSource proxy if they are to support allocator overriding.
    if (allocator->CanBeOverridden())
    {
        auto shim = Internal::AllocatorOverrideShim::Create(allocator, &data.m_mallocSchema);
        allocator->SetAllocationSource(shim);
    }
#endif

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
        s_allocManager = AZ::Environment::CreateVariable<AllocatorManager>(AZ_CRC("AZ::AllocatorManager::s_allocManager", 0x6bdd908c));

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
AZ::MallocSchema* AllocatorManager::CreateMallocSchema()
{
    return static_cast<AZ::MallocSchema*>(new(AZ_OS_MALLOC(sizeof(AZ::MallocSchema), alignof(AZ::MallocSchema))) AZ::MallocSchema());
}


//=========================================================================
// AllocatorManager
// [9/11/2009]
//=========================================================================
AllocatorManager::AllocatorManager()
    : m_profilingRefcount(0)
    , m_mallocSchema(CreateMallocSchema(), [](AZ::MallocSchema* schema)
    {
        if (schema)
        {
            schema->~MallocSchema();
            AZ_OS_FREE(schema);
        }
    }
)
{
    m_overrideSource = nullptr;
    m_numAllocators = 0;
    m_isAllocatorLeaking = false;
    m_configurationFinalized = false;
    m_defaultTrackingRecordMode = AZ::Debug::AllocationRecords::RECORD_NO_RECORDS;
    m_data = new (m_mallocSchema->Allocate(sizeof(InternalData), AZStd::alignment_of<InternalData>::value, 0)) InternalData(AZStdIAllocator(m_mallocSchema.get()));
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

#ifdef AZCORE_MEMORY_ENABLE_OVERRIDES
    ConfigureAllocatorOverrides(alloc);
#endif

    EBUS_EVENT(Debug::MemoryDrillerBus, RegisterAllocator, alloc);
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

    if (m_data)
    {
        m_data->~InternalData();
        m_mallocSchema->DeAllocate(m_data);
        m_data = nullptr;
    }

    if (!m_isAllocatorLeaking)
    {
        AZ_Assert(m_numAllocators == 0, "There are still %d registered allocators!", m_numAllocators);
    }
}

//=========================================================================
// ConfigureAllocatorOverrides
// [10/14/2018]
//=========================================================================
void 
AllocatorManager::ConfigureAllocatorOverrides(IAllocator* alloc)
{
    auto record = m_data->m_allocatorMap.emplace(AZStd::piecewise_construct, AZStd::forward_as_tuple(alloc->GetName(), AZStdIAllocator(m_mallocSchema.get())), AZStd::forward_as_tuple(alloc));

    // We only need to keep going if the allocator supports overrides.
    if (!alloc->CanBeOverridden())
    {
        return;
    }

    if (!alloc->IsAllocationSourceChanged())
    {
        // All allocators must switch to an OverrideEnabledAllocationSource proxy if they are to support allocator overriding.
        auto overrideEnabled = Internal::AllocatorOverrideShim::Create(alloc, m_mallocSchema.get());
        alloc->SetAllocationSource(overrideEnabled);
    }

    auto itr = m_data->m_remappings.find(record.first->first);

    if (itr != m_data->m_remappings.end())
    {
        auto remapTo = m_data->m_allocatorMap.find(itr->second);

        if (remapTo != m_data->m_allocatorMap.end())
        {
            static_cast<Internal::AllocatorOverrideShim*>(alloc->GetAllocationSource())->SetOverride(remapTo->second->GetOriginalAllocationSource());
        }
    }

    itr = m_data->m_remappingsReverse.find(record.first->first);

    if (itr != m_data->m_remappingsReverse.end())
    {
        auto remapFrom = m_data->m_allocatorMap.find(itr->second);

        if (remapFrom != m_data->m_allocatorMap.end())
        {
            AZ_Assert(!m_configurationFinalized, "Allocators may only remap to allocators that have been created before configuration finalization");
            static_cast<Internal::AllocatorOverrideShim*>(remapFrom->second->GetAllocationSource())->SetOverride(alloc->GetOriginalAllocationSource());
        }
    }

    if (m_overrideSource)
    {
        static_cast<Internal::AllocatorOverrideShim*>(alloc->GetAllocationSource())->SetOverride(m_overrideSource);
    }

    if (m_configurationFinalized)
    {
        // We can get rid of the intermediary if configuration won't be changing any further.
        // (The creation of it at the top of this function was superflous, but it made it easier to set things up going through a single code path.)
        auto shim = static_cast<Internal::AllocatorOverrideShim*>(alloc->GetAllocationSource());
        alloc->SetAllocationSource(shim->GetOverride());
        Internal::AllocatorOverrideShim::Destroy(shim);
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

    if (alloc->GetRecords())
    {
        EBUS_EVENT(Debug::MemoryDrillerBus, UnregisterAllocator, alloc);
    }

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
        m_allocators[i]->GetAllocationSource()->GarbageCollect();
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
AllocatorManager::SetTrackingMode(AZ::Debug::AllocationRecords::Mode mode)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    for (int i = 0; i < m_numAllocators; ++i)
    {
        AZ::Debug::AllocationRecords* records = m_allocators[i]->GetRecords();
        if (records)
        {
            records->SetMode(mode);
        }
    }
}

//=========================================================================
// SetOverrideSchema
// [8/17/2018]
//=========================================================================
void
AllocatorManager::SetOverrideAllocatorSource(IAllocatorAllocate* source, bool overrideExistingAllocators)
{
    (void)source;
    (void)overrideExistingAllocators;

#ifdef AZCORE_MEMORY_ENABLE_OVERRIDES
    AZ_Assert(!m_configurationFinalized, "You cannot set an allocator source after FinalizeConfiguration() has been called.");
    m_overrideSource = source;

    if (overrideExistingAllocators)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        for (int i = 0; i < m_numAllocators; ++i)
        {
            if (m_allocators[i]->CanBeOverridden())
            {
                auto shim = static_cast<Internal::AllocatorOverrideShim*>(m_allocators[i]->GetAllocationSource());
                shim->SetOverride(source);
            }
        }
    }
#endif
}

//=========================================================================
// AddAllocatorRemapping
// [8/27/2018]
//=========================================================================
void
AllocatorManager::AddAllocatorRemapping(const char* fromName, const char* toName)
{
    (void)fromName;
    (void)toName;

#ifdef AZCORE_MEMORY_ENABLE_OVERRIDES
    AZ_Assert(!m_configurationFinalized, "You cannot set an allocator remapping after FinalizeConfiguration() has been called.");
    m_data->m_remappings.emplace(AZStd::piecewise_construct, AZStd::forward_as_tuple(fromName, m_mallocSchema.get()), AZStd::forward_as_tuple(toName, m_mallocSchema.get()));
    m_data->m_remappingsReverse.emplace(AZStd::piecewise_construct, AZStd::forward_as_tuple(toName, m_mallocSchema.get()), AZStd::forward_as_tuple(fromName, m_mallocSchema.get()));
#endif
}

void 
AllocatorManager::FinalizeConfiguration()
{
    if (m_configurationFinalized)
    {
        return;
    }

#ifdef AZCORE_MEMORY_ENABLE_OVERRIDES
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

        for (int i = 0; i < m_numAllocators; ++i)
        {
            if (!m_allocators[i]->CanBeOverridden())
            {
                continue;
            }

            auto shim = static_cast<Internal::AllocatorOverrideShim*>(m_allocators[i]->GetAllocationSource());

            if (!shim->IsOverridden())
            {
                m_allocators[i]->ResetAllocationSource();
                Internal::AllocatorOverrideShim::Destroy(shim);
            }
            else if (!shim->HasOrphanedAllocations())
            {
                m_allocators[i]->SetAllocationSource(shim->GetOverride());
                Internal::AllocatorOverrideShim::Destroy(shim);
            }
            else
            {
                shim->SetFinalizedConfiguration();
            }
        }
    }
#endif

    m_configurationFinalized = true;
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
    void* sourceList[m_maxNumAllocators];

    AZ_Printf(TAG, "%d allocators active\n", m_numAllocators);
    AZ_Printf(TAG, "Index,Name,Used kb,Reserved kb,Consumed kb\n");

    for (int i = 0; i < m_numAllocators; i++)
    {
        auto allocator = m_allocators[i];
        auto source = allocator->GetAllocationSource();
        const char* name = allocator->GetName();
        size_t usedBytes = source->NumAllocatedBytes();
        size_t reservedBytes = source->Capacity();
        size_t consumedBytes = reservedBytes;

        // Very hacky and inefficient check to see if this allocator obtains its memory from another allocator
        sourceList[i] = source;
        if (AZStd::find(sourceList, sourceList + i, allocator->GetSchema()) != sourceList + i)
        {
            consumedBytes = 0;
        }

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
    AZStd::unordered_map<AZ::IAllocatorAllocate*, AZ::IAllocator*> existingAllocators;
    AZStd::unordered_map<AZ::IAllocatorAllocate*, AZ::IAllocator*> sourcesToAllocators;

    // Build a mapping of original allocator sources to their allocators
    for (int i = 0; i < allocatorCount; ++i)
    {
        AZ::IAllocator* allocator = GetAllocator(i);
        sourcesToAllocators.emplace(allocator->GetOriginalAllocationSource(), allocator);
    }

    for (int i = 0; i < allocatorCount; ++i)
    {
        AZ::IAllocator* allocator = GetAllocator(i);
        AZ::IAllocatorAllocate* source = allocator->GetAllocationSource();
        AZ::IAllocatorAllocate* originalSource = allocator->GetOriginalAllocationSource();
        AZ::IAllocatorAllocate* schema = allocator->GetSchema();
        AZ::IAllocator* alias = (source != originalSource) ? sourcesToAllocators[source] : nullptr;

        if (schema && !alias)
        {
            // Check to see if this allocator's source maps to another allocator
            // Need to check both the schema and the allocator itself, as either one might be used as the alias depending on how it's implemented
            AZStd::array<AZ::IAllocatorAllocate*, 2> checkAllocators = { { schema, allocator->GetAllocationSource() } };

            for (AZ::IAllocatorAllocate* check : checkAllocators)
            {
                auto existing = existingAllocators.emplace(check, allocator);

                if (!existing.second)
                {
                    alias = existing.first->second;
                    // Do not break out of the loop as we need to add to the map for all entries
                }
            }
        }

        static const AZ::IAllocator* OS_ALLOCATOR = &AZ::AllocatorInstance<AZ::OSAllocator>::GetAllocator();
        size_t sourceAllocatedBytes = source->NumAllocatedBytes();
        size_t sourceCapacityBytes = source->Capacity();

        if (allocator == OS_ALLOCATOR)
        {
            // Need to special case the OS allocator because its capacity is a made-up number. Better to just use the allocated amount, it will hopefully be small anyway.
            sourceCapacityBytes = sourceAllocatedBytes;
        }

        if (outStats)
        {
            outStats->emplace(outStats->end(), allocator->GetName(), alias ? alias->GetName() : allocator->GetDescription(), sourceAllocatedBytes, sourceCapacityBytes, alias != nullptr);
        }

        if (!alias)
        {
            allocatedBytes += sourceAllocatedBytes;
            capacityBytes += sourceCapacityBytes;
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
