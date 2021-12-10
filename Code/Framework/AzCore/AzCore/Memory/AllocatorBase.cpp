/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/AllocatorManager.h>

using namespace AZ;

#define RECORDING_ENABLED 0

#if RECORDING_ENABLED

#include <AzCore/std/containers/map.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace
{
    class DebugAllocator
    {
    public:
        typedef void* pointer_type;
        typedef AZStd::size_t size_type;
        typedef AZStd::ptrdiff_t difference_type;
        typedef AZStd::false_type allow_memory_leaks; ///< Regular allocators should not leak.

        AZ_FORCE_INLINE pointer_type allocate(size_t byteSize, size_t alignment, int = 0)
        {
            return AZ_OS_MALLOC(byteSize, alignment);
        }
        AZ_FORCE_INLINE size_type resize(pointer_type, size_type)
        {
            return 0;
        }
        AZ_FORCE_INLINE void deallocate(pointer_type ptr, size_type, size_type)
        {
            AZ_OS_FREE(ptr);
        }
    };

    #pragma pack(push, 1)
    struct alignas(1) AllocatorOperation
    {
        enum OperationType : size_t
        {
            ALLOCATE,
            DEALLOCATE
        };
        OperationType m_type: 1;
        size_t m_size : 28; // Can represent up to 256Mb requests
        size_t m_alignment : 7; // Can represent up to 128 alignment
        size_t m_recordId : 28; // Can represent up to 256M simultaneous requests, we reuse ids
    };
    #pragma pack(pop)
    static_assert(sizeof(AllocatorOperation) == 8);

    static AZStd::mutex s_operationsMutex = {};

    static constexpr size_t s_maxNumberOfAllocationsToRecord = 16384;
    static size_t s_numberOfAllocationsRecorded = 0;
    static constexpr size_t s_allocationOperationCount = 5 * 1024;
    static AZStd::array<AllocatorOperation, s_allocationOperationCount> s_operations = {};
    static uint64_t s_operationCounter = 0;

    static unsigned int s_nextRecordId = 1;
    using AllocatorOperationByAddress = AZStd::map<void*, AllocatorOperation, AZStd::less<void*>, DebugAllocator>;
    static AllocatorOperationByAddress s_allocatorOperationByAddress;
    using AvailableRecordIds = AZStd::vector<unsigned int, DebugAllocator>;
    AvailableRecordIds s_availableRecordIds;

    void RecordAllocatorOperation(AllocatorOperation::OperationType type, void* ptr, size_t size = 0, size_t alignment = 0)
    {
        AZStd::scoped_lock<AZStd::mutex> lock(s_operationsMutex);
        if (s_operationCounter == s_allocationOperationCount)
        {
            AZ::IO::SystemFile file;
            int mode = AZ::IO::SystemFile::OpenMode::SF_OPEN_APPEND | AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY;
            if (!file.Exists("memoryrecordings.bin"))
            {
                mode |= AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE;
            }
            file.Open("memoryrecordings.bin", mode);
            if (file.IsOpen())
            {
                file.Write(&s_operations, sizeof(AllocatorOperation) * s_allocationOperationCount);
                file.Close();
            }
            s_operationCounter = 0;
        }
        AllocatorOperation& operation = s_operations[s_operationCounter++];
        operation.m_type = type;
        if (type == AllocatorOperation::OperationType::ALLOCATE)
        {
            if (s_numberOfAllocationsRecorded > s_maxNumberOfAllocationsToRecord)
            {
                // reached limit of allocations, dont record anymore
                --s_operationCounter;
                return;
            }
            ++s_numberOfAllocationsRecorded;
            operation.m_size = size;
            operation.m_alignment = alignment;
            unsigned int recordId = 0;
            if (!s_availableRecordIds.empty())
            {
                recordId = s_availableRecordIds.back();
                s_availableRecordIds.pop_back();
            }
            else
            {
                recordId = s_nextRecordId;
                ++s_nextRecordId;
            }
            operation.m_recordId = recordId;
            auto it = s_allocatorOperationByAddress.emplace(ptr, operation);
            if (!it.second)
            {
                // double alloc or resize, leave the current record and return the id
                operation = it.first->second;
                s_availableRecordIds.emplace_back(recordId);
            }                
        }
        else
        {
            if (ptr == nullptr)
            {
                // common scenario, just record the operation
                operation.m_size = 0;
                operation.m_alignment = 0;
                operation.m_recordId = 0; // recordId = 0 will flag this case
            }
            else
            {
                auto it = s_allocatorOperationByAddress.find(ptr);
                if (it != s_allocatorOperationByAddress.end())
                {
                    operation.m_size = it->second.m_size;
                    operation.m_alignment = it->second.m_alignment;
                    operation.m_recordId = it->second.m_recordId;
                    s_availableRecordIds.push_back(it->second.m_recordId);
                    s_allocatorOperationByAddress.erase(it);
                }
                else
                {
                    // just dont record this operation
                    --s_operationCounter;
                }
            }
        }
    
    }
}
#endif

AllocatorBase::AllocatorBase(IAllocatorSchema* allocationSchema, const char* name, const char* desc)
    : IAllocator(allocationSchema)
    , m_name(name)
    , m_desc(desc)
{
}

AllocatorBase::~AllocatorBase()
{
    AZ_Assert(!m_isReady, "Allocator %s (%s) is being destructed without first having gone through proper calls to PreDestroy() and Destroy(). Use AllocatorInstance<> for global allocators or AllocatorWrapper<> for local allocators.", m_name, m_desc);
}

const char* AllocatorBase::GetName() const
{
    return m_name;
}

const char* AllocatorBase::GetDescription() const
{
    return m_desc;
}

Debug::AllocationRecords* AllocatorBase::GetRecords()
{
    return m_records;
}

void AllocatorBase::SetRecords(Debug::AllocationRecords* records)
{
    m_records = records;
    m_memoryGuardSize = records ? records->MemoryGuardSize() : 0;
}

bool AllocatorBase::IsReady() const
{
    return m_isReady;
}

void AllocatorBase::PostCreate()
{
    if (m_registrationEnabled)
    {
        if (AZ::Environment::IsReady())
        {
            AllocatorManager::Instance().RegisterAllocator(this);
        }
        else
        {
            AllocatorManager::PreRegisterAllocator(this);
        }
    }

    const auto debugConfig = GetDebugConfig();
    if (!debugConfig.m_excludeFromDebugging)
    {
        SetRecords(aznew Debug::AllocationRecords((unsigned char)debugConfig.m_stackRecordLevels, debugConfig.m_usesMemoryGuards, debugConfig.m_marksUnallocatedMemory, GetName()));
    }

    m_isReady = true;
}

void AllocatorBase::PreDestroy()
{
    Debug::AllocationRecords* allocatorRecords = GetRecords();
    if(allocatorRecords)
    {
        delete allocatorRecords;
        SetRecords(nullptr);
    }

    if (m_registrationEnabled && AZ::AllocatorManager::IsReady())
    {
        AllocatorManager::Instance().UnRegisterAllocator(this);
    }

    m_isReady = false;
}

void AllocatorBase::SetLazilyCreated(bool lazy)
{
    m_isLazilyCreated = lazy;
}

bool AllocatorBase::IsLazilyCreated() const
{
    return m_isLazilyCreated;
}

void AllocatorBase::SetProfilingActive(bool active)
{
    m_isProfilingActive = active;
}

bool AllocatorBase::IsProfilingActive() const
{
    return m_isProfilingActive;
}

void AllocatorBase::DisableRegistration()
{
    m_registrationEnabled = false;
}

void AllocatorBase::ProfileAllocation(void* ptr, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, int suppressStackRecord)
{
    if (m_isProfilingActive)
    {
#if defined(AZ_HAS_VARIADIC_TEMPLATES) && defined(AZ_DEBUG_BUILD)
        ++suppressStackRecord; // one more for the fact the ebus is a function
#endif // AZ_HAS_VARIADIC_TEMPLATES

        auto records = GetRecords();
        if (records)
        {
            records->RegisterAllocation(ptr, byteSize, alignment, name, fileName, lineNum, suppressStackRecord + 1);
        }
    }

#if RECORDING_ENABLED
    RecordAllocatorOperation(AllocatorOperation::ALLOCATE, ptr, byteSize, alignment);
#endif
}

void AllocatorBase::ProfileDeallocation(void* ptr, size_t byteSize, size_t alignment, Debug::AllocationInfo* info)
{
    if (m_isProfilingActive)
    {
        auto records = GetRecords();
        if (records)
        {
            records->UnregisterAllocation(ptr, byteSize, alignment, info);
        }
    }
#if RECORDING_ENABLED
    RecordAllocatorOperation(AllocatorOperation::DEALLOCATE, ptr, byteSize, alignment);
#endif
}

void AllocatorBase::ProfileReallocationBegin([[maybe_unused]] void* ptr, [[maybe_unused]] size_t newSize)
{
}

void AllocatorBase::ProfileReallocationEnd(void* ptr, void* newPtr, size_t newSize, size_t newAlignment)
{
    if (m_isProfilingActive)
    {
        Debug::AllocationInfo info;
        ProfileDeallocation(ptr, 0, 0, &info);
        ProfileAllocation(newPtr, newSize, newAlignment, info.m_name, info.m_fileName, info.m_lineNum, 0);
    }
#if RECORDING_ENABLED
    RecordAllocatorOperation(AllocatorOperation::DEALLOCATE, ptr);
    RecordAllocatorOperation(AllocatorOperation::ALLOCATE, newPtr, newSize, newAlignment);
#endif
}

void AllocatorBase::ProfileReallocation(void* ptr, void* newPtr, size_t newSize, size_t newAlignment)
{
    ProfileReallocationEnd(ptr, newPtr, newSize, newAlignment);
}

void AllocatorBase::ProfileResize(void* ptr, size_t newSize)
{
    if (newSize && m_isProfilingActive)
    {
        auto records = GetRecords();
        if (records)
        {
            records->ResizeAllocation(ptr, newSize);
        }
    }
#if RECORDING_ENABLED
    RecordAllocatorOperation(AllocatorOperation::ALLOCATE, ptr, newSize);
#endif
}

bool AllocatorBase::OnOutOfMemory(size_t byteSize, size_t alignment, int flags, const char* name, const char* fileName, int lineNum)
{
    if (AllocatorManager::IsReady() && AllocatorManager::Instance().m_outOfMemoryListener)
    {
        AllocatorManager::Instance().m_outOfMemoryListener(this, byteSize, alignment, flags, name, fileName, lineNum);
        return true;
    }
    return false;
}
