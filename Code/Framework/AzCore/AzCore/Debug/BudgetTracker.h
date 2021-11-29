/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Environment.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ::Debug
{
    class Budget;

    class BudgetTracker
    {
    public:
        AZ_TYPE_INFO(BudgetTracker, "{E14A746D-BFFE-4C02-90FB-4699B79864A5}");
        static Budget* GetBudgetFromEnvironment(const char* budgetName, uint32_t crc);

        ~BudgetTracker();

        // Returns false if the budget tracker was already present in the environment (initialized already elsewhere)
        bool Init();
        void Reset();

        Budget& GetBudget(const char* budgetName, uint32_t crc);

    private:
        struct BudgetTrackerImpl;

        AZStd::mutex m_mutex;

        // The BudgetTracker is likely included in proportionally high number of files throughout the
        // engine, so indirection is used here to avoid imposing excessive recompilation in periods
        // while the budget system is iterated on.
        BudgetTrackerImpl* m_impl = nullptr;
    };
} // namespace AZ::Debug
