/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    AllocatorBase::AllocatorBase(IAllocatorAllocate* allocationSource, const char* name, const char* desc)
        : IAllocator(allocationSource)
        , m_name(name)
        , m_desc(desc)
    {
    }

    AllocatorBase::~AllocatorBase()
    {
        AZ_Assert(
            !m_isReady,
            "Allocator %s (%s) is being destructed without first having gone through proper calls to PreDestroy() and Destroy(). Use "
            "AllocatorInstance<> for global allocators or AllocatorWrapper<> for local allocators.",
            m_name, m_desc);
    }

    const char* AllocatorBase::GetName() const
    {
        return m_name;
    }

    const char* AllocatorBase::GetDescription() const
    {
        return m_desc;
    }

    IAllocatorAllocate* AllocatorBase::GetSchema()
    {
        return nullptr;
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

    bool AllocatorBase::CanBeOverridden() const
    {
        return m_canBeOverridden;
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
            SetRecords(aznew Debug::AllocationRecords(
                (unsigned char)debugConfig.m_stackRecordLevels, debugConfig.m_usesMemoryGuards, debugConfig.m_marksUnallocatedMemory,
                GetName()));
        }

        m_isReady = true;
    }

    void AllocatorBase::PreDestroy()
    {
        Debug::AllocationRecords* allocatorRecords = GetRecords();
        if (allocatorRecords)
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

    void AllocatorBase::DisableOverriding()
    {
        m_canBeOverridden = false;
    }

    void AllocatorBase::DisableRegistration()
    {
        m_registrationEnabled = false;
    }

    void AllocatorBase::ProfileAllocation(
        void* ptr, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, int suppressStackRecord)
    {
#if defined(AZ_HAS_VARIADIC_TEMPLATES) && defined(AZ_DEBUG_BUILD)
        ++suppressStackRecord; // one more for the fact the ebus is a function
#endif // AZ_HAS_VARIADIC_TEMPLATES

        if (m_isProfilingActive)
        {
            auto records = GetRecords();
            if (records)
            {
                records->RegisterAllocation(ptr, byteSize, alignment, name, fileName, lineNum, suppressStackRecord + 1);
            }
        }
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

} // namespace AZ
