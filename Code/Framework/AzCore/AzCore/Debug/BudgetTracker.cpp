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
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ::Debug
{
    struct BudgetTracker::BudgetTrackerImpl
    {
        AZStd::unordered_map<AZStd::string_view, Budget> m_budgets;
        AZStd::unordered_set<Budget**> m_externalBudgetRefs;
    };

    void BudgetTracker::GetBudgetFromEnvironment(Budget*& extBudgetRef, const char* budgetName, uint32_t crc)
    {
        BudgetTracker* tracker = Interface<BudgetTracker>::Get();
        if (tracker)
        {
            tracker->GetBudget(extBudgetRef, budgetName, crc);
        }
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

            for (auto budgetRef : m_impl->m_externalBudgetRefs)
            {
                *budgetRef = nullptr;
            }

            delete m_impl;
            m_impl = nullptr;
        }
    }

    void BudgetTracker::GetBudget(Budget*& extBudgetRef, const char* budgetName, uint32_t crc)
    {
        AZStd::scoped_lock lock{ m_mutex };

        m_impl->m_externalBudgetRefs.insert(&extBudgetRef);

        auto iter = m_impl->m_budgets.try_emplace(budgetName, budgetName, crc).first;
        extBudgetRef = &iter->second;
    }
} // namespace AZ::Debug
