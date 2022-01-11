/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/BudgetTracker.h>

#include <AzCore/base.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/allocator_stateless.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::Debug
{
    struct BudgetTracker::BudgetTrackerImpl
    {
        AZStd::unordered_map<const char*, AZStd::shared_ptr<Budget>> m_budgets;
    };

    AZStd::shared_ptr<Budget> BudgetTracker::GetBudgetFromEnvironment(const char* budgetName, uint32_t crc)
    {
        BudgetTracker* tracker = Interface<BudgetTracker>::Get();
        if (tracker)
        {
            return tracker->GetBudget(budgetName, crc);
        }
        return nullptr;
    }

    BudgetTracker::~BudgetTracker()
    {
        Reset();
    }

    bool BudgetTracker::Init()
    {
        if (Interface<BudgetTracker>::Get())
        {
            return false;
        }

        Interface<BudgetTracker>::Register(this);
        m_impl = new BudgetTrackerImpl;
        return true;
    }

    void BudgetTracker::Reset()
    {
        if (m_impl)
        {
            Interface<BudgetTracker>::Unregister(this);
            delete m_impl;
            m_impl = nullptr;
        }
    }

    AZStd::shared_ptr<Budget> BudgetTracker::GetBudget(const char* budgetName, uint32_t crc)
    {
        AZStd::scoped_lock lock{ m_mutex };

        if (auto iter = m_impl->m_budgets.find(budgetName); iter != m_impl->m_budgets.end())
        {
            return iter->second;
        }

        auto budget = AZStd::allocate_shared<Budget>(AZStd::stateless_allocator(), budgetName, crc);
        m_impl->m_budgets.emplace(budgetName, budget);
        return budget;
    }
} // namespace AZ::Debug
