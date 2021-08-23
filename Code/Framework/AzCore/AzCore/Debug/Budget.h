/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Debug/BudgetTracker.h>

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
        // If you encounter a linker error complaining that this function is not defined, you have likely forgotten to either
        // define or declare the budget used in a profile or memory marker. See AZ_DEFINE_BUDGET and AZ_DECLARE_BUDGET below
        // for usage.
        template<typename BudgetType>
        static Budget* Get();

        explicit Budget(const char* name);
        ~Budget();

        void PerFrameReset();
        void BeginProfileRegion();
        void EndProfileRegion();
        void TrackAllocation(uint64_t bytes);
        void UntrackAllocation(uint64_t bytes);

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
        struct BudgetImpl* m_impl = nullptr;
    };
} // namespace AZ::Debug
#pragma warning(pop)

// Budgets are registered and retrieved using the proxy type specialization of Budget::Get<T>. The type itself has no declaration/definition
// other than this forward type pointer declaration
#define AZ_BUDGET_PROXY_TYPE(name) class AzBudget##name*

// Usage example:
// In a single C++ source file:
// AZ_DEFINE_BUDGET(AzCore);
//
// Anywhere the budget is used, the budget must be declared (either in a header or in the source file itself)
// AZ_DECLARE_BUDGET(AzCore);
//
// The budget is usable in the same file it was defined without needing an additional declaration
#define AZ_DEFINE_BUDGET(name)                                                                                                             \
    template<>                                                                                                                             \
    ::AZ::Debug::Budget* ::AZ::Debug::Budget::Get<AZ_BUDGET_PROXY_TYPE(name)>()                                                            \
    {                                                                                                                                      \
        static ::AZStd::atomic<::AZ::Debug::Budget*> budget;                                                                               \
        ::AZ::Debug::Budget* out = budget.load(AZStd::memory_order_acquire);                                                               \
        if (out)                                                                                                                           \
        {                                                                                                                                  \
            return out;                                                                                                                    \
        }                                                                                                                                  \
        else                                                                                                                               \
        {                                                                                                                                  \
            budget.store(&::AZ::Debug::BudgetTracker::GetBudgetFromEnvironment(#name), AZStd::memory_order_release);                       \
            return budget;                                                                                                                 \
        }                                                                                                                                  \
    }

// If using a budget defined in a different C++ source file, add AZ_DECLARE_BUDGET(yourBudget); somewhere in your source file at namespace
// scope Alternatively, AZ_DECLARE_BUDGET can be used in a header to declare the budget for use across any users of the header
#define AZ_DECLARE_BUDGET(name) extern template ::AZ::Debug::Budget* ::AZ::Debug::Budget::Get<AZ_BUDGET_PROXY_TYPE(name)>()

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
