/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Environment.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ::Debug
{
    class Budget;

    class BudgetTracker
    {
    public:
        static Budget& GetBudgetFromEnvironment(const char* budgetName);

        ~BudgetTracker();

        void Init();

        Budget& GetBudget(const char* budgetName);

    private:
        AZStd::mutex m_mutex;
        AZ::EnvironmentVariable<BudgetTracker*> m_envVar;

        // The BudgetTracker is likely included in proportionally high number of files throughout the
        // engine, so indirection is used here to avoid imposing excessive recompilation in periods
        // while the budget system is iterated on.
        struct BudgetTrackerImpl* m_impl = nullptr;
    };
} // namespace AZ::Debug
