/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Budget.h"

#include <AzCore/Module/Environment.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>

AZ_DEFINE_BUDGET(Animation);
AZ_DEFINE_BUDGET(Audio);
AZ_DEFINE_BUDGET(AzCore);
AZ_DEFINE_BUDGET(Editor);
AZ_DEFINE_BUDGET(Entity);
AZ_DEFINE_BUDGET(Game);
AZ_DEFINE_BUDGET(System);
AZ_DEFINE_BUDGET(Physics);

namespace AZ::Debug
{
    struct BudgetImpl
    {
        AZ_CLASS_ALLOCATOR(BudgetImpl, AZ::SystemAllocator, 0);

        BudgetImpl()
        {
            m_logger = AZ::Interface<IEventLogger>::Get();
        }

        IEventLogger* m_logger;
        AZStd::atomic<AZ::u32> m_frameNumber{ 0 };
        thread_local static AZStd::chrono::system_clock::time_point s_startTime;
    };

    thread_local AZStd::chrono::system_clock::time_point BudgetImpl::s_startTime;

    struct PerformanceEvent
    {
        AZ::u64 m_duration;
        AZ::u32 m_frameNumber;
        char m_budgetName[32];
    };

    Budget::Budget(const char* name)
        : Budget( name, Crc32(name) )
    {
    }

    Budget::Budget(const char* name, uint32_t crc)
        : m_name{ name }
        , m_crc{ crc }
        , m_budgetHash { name }
    {
        m_impl = aznew BudgetImpl;
    }

    Budget::~Budget()
    {
        if (m_impl)
        {
            delete m_impl;
        }
    }

    // TODO:Budgets Methods below are stubbed pending future work to both update budget data and visualize it

    void Budget::PerFrameReset()
    {
        m_impl->m_frameNumber++;
    }

    void Budget::BeginProfileRegion()
    {
        BudgetImpl::s_startTime = AZStd::chrono::high_resolution_clock::now();
    }

    void Budget::EndProfileRegion()
    {
        AZStd::chrono::microseconds duration = AZStd::chrono::high_resolution_clock::now() - BudgetImpl::s_startTime;
        PerformanceEvent& event = m_impl->m_logger->RecordEventBegin<PerformanceEvent>(m_budgetHash, 0);
        event.m_duration = duration.count();
        event.m_frameNumber = m_impl->m_frameNumber;
        memcpy(event.m_budgetName, m_name.data(), m_name.length());
        m_impl->m_logger->RecordEventEnd();
    }

    void Budget::TrackAllocation(uint64_t)
    {
    }

    void Budget::UntrackAllocation(uint64_t)
    {
    }
} // namespace AZ::Debug
