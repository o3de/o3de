/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Module/Environment.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/parallel/atomic.h>

#pragma warning(push)
// This warning must be disabled because Budget::Get<T> may not have an implementation if this file is transitively included
// in a source file that doesn't actually have any budgets declared in scope (via AZ_DEFINE_BUDGET or AZ_DECLARE_BUDGET).
// In this situation, the warning about internal linkage without an implementation is benign because we know that this function
// cannot be invoked from that translation unit.
#pragma warning(disable: 5046)

namespace AZ::Debug
{
    // A budget collates per-frame resource utilization and memory for a particular category
    class Budget final
    {
    public:
        // Invoked once at the start of the frame to reset per-frame counters
        static void ResetAll();

        // If you encounter a linker error complaining that this function is not defined, you have likely forgotten to either
        // define or declare the budget used in a profile or memory marker. See AZ_DEFINE_BUDGET and AZ_DECLARE_BUDGET below
        // for usage.
        template<typename BudgetType>
        static Budget* Get();

        explicit Budget(const char* name);

        const char* Name() const
        {
            return m_name;
        }

        uint32_t Crc() const
        {
            return m_crc;
        }

    private:
        const char* m_name;
        uint32_t m_crc;
    };
} // namespace AZ::Debug
#pragma warning(pop)

#define AZ_BUDGET_NAME(name) AzBudget##name

// Usage example:
// In a single C++ source file:
// AZ_DEFINE_BUDGET(AzCore);
//
// Anywhere the budget is used, the budget must be declared (either in a header or in the source file itself)
// AZ_DECLARE_BUDGET(AzCore);
//
// The budget is usable in the same file it was defined without needing an additional declaration

// Implementation notes:
// Every budget definition is declared in static storage along with the environment variable and mutex. This imposes a slight
// memory overhead in the data segment only in instances where the same static module defining a budget is linked against multiple
// DLLs, however, this simplifies the implementation and works regardless of whether the budget is defined in a static or dynamic
// library. When loading the budget, a relaxed load is sufficient because Environment::CreateVariable internally locks and returns
// the element found if the environment variable was already created (skipping construction in the process). Thus, the budget pointer
// will reference the statically stored budget for the first thread that grabs the lock.
#define AZ_DEFINE_BUDGET(name)                                                                                                             \
    template<>                                                                                                                             \
    ::AZ::Debug::Budget* ::AZ::Debug::Budget::Get<class AZ_BUDGET_NAME(name)*>()                                                           \
    {                                                                                                                                      \
        static ::AZStd::mutex s_azBudgetMutex##name;                                                                                       \
        static ::AZ::EnvironmentVariable<::AZ::Debug::Budget*> s_azBudgetEnv##name;                                                        \
        static ::AZ::Debug::Budget s_azBudget##name{ #name };                                                                              \
        static ::AZStd::atomic<::AZ::Debug::Budget*> budget;                                                                               \
        ::AZ::Debug::Budget* out = budget.load(AZStd::memory_order_relaxed);                                                               \
        if (out)                                                                                                                           \
        {                                                                                                                                  \
            return out;                                                                                                                    \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            {                                                                                                                              \
                AZStd::scoped_lock lock{ s_azBudgetMutex##name };                                                                          \
                if (!s_azBudgetEnv##name)                                                                                                  \
                {                                                                                                                          \
                    s_azBudgetEnv##name = ::AZ::Environment::CreateVariable<::AZ::Debug::Budget*>("budgetEnv" #name, &s_azBudget##name);   \
                }                                                                                                                          \
            }                                                                                                                              \
            out = *s_azBudgetEnv##name;                                                                                                    \
            budget = out;                                                                                                                  \
            return out;                                                                                                                    \
        }                                                                                                                                  \
    }

// If using a budget defined in a different C++ source file, add AZ_DECLARE_BUDGET(yourBudget); somewhere in your source file at namespace
// scope Alternatively, AZ_DECLARE_BUDGET can be used in a header to declare the budget for use across any users of the header
#define AZ_DECLARE_BUDGET(name) extern template ::AZ::Debug::Budget* ::AZ::Debug::Budget::Get<class AZ_BUDGET_NAME(name)*>()

// Declare budgets that are core engine budgets, or may be shared/needed across multiple external gems
// You should NOT need to declare user-space or budgets with isolated usage here. Prefer declaring them local to the module(s) that use
// the budget and defining them within a single module to avoid needing to recompile the entire engine.
AZ_DECLARE_BUDGET(AzCore);
AZ_DECLARE_BUDGET(Editor);
AZ_DECLARE_BUDGET(Entity);
AZ_DECLARE_BUDGET(Game);
AZ_DECLARE_BUDGET(System);
AZ_DECLARE_BUDGET(Physics);
AZ_DECLARE_BUDGET(Animation);
