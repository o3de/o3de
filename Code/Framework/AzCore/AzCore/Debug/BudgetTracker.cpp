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
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ::Debug
{
    constexpr static const char* BudgetTrackerEnvName = "budgetTrackerEnv";

    struct BudgetTrackerImpl
    {
        AZ_CLASS_ALLOCATOR(BudgetTrackerImpl, AZ::SystemAllocator, 0);

        AZStd::unordered_map<const char*, Budget> m_budgets;
    };

    Budget& BudgetTracker::GetBudgetFromEnvironment(const char* budgetName)
    {
        return (*Environment::FindVariable<BudgetTracker*>(BudgetTrackerEnvName))->GetBudget(budgetName);
    }

    BudgetTracker::~BudgetTracker()
    {
        if (m_impl)
        {
            delete m_impl;
        }
    }

    void BudgetTracker::Init()
    {
        AZ_Assert(!m_impl, "BudgetTracker::Init called more than once");

        m_impl = aznew BudgetTrackerImpl;
        m_envVar = Environment::CreateVariable<BudgetTracker*>(BudgetTrackerEnvName, this);
    }

    Budget& BudgetTracker::GetBudget(const char* budgetName)
    {
        AZStd::scoped_lock lock{ m_mutex };

        auto it = m_impl->m_budgets.find(budgetName);
        if (it == m_impl->m_budgets.end())
        {
            it = m_impl->m_budgets.emplace(budgetName, budgetName).first;
        }

        return it->second;
    }
} // namespace AZ::Debug
