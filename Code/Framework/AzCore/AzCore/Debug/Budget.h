/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/BudgetTracker.h>
#include <AzCore/Math/Crc.h>

namespace AZ::Debug
{
    // A budget collates per-frame resource utilization and memory for a particular category
    class Budget final
    {
    public:
        explicit Budget(const char* name);
        Budget(const char* name, uint32_t crc);
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
        const uint32_t m_crc;
        struct BudgetImpl* m_impl = nullptr;
    };
} // namespace AZ::Debug

// The budget is usable in the same file it was defined without needing an additional declaration.
// If you encounter a linker error complaining that this function is not defined, you have likely forgotten to either
// define or declare the budget used in a profile or memory marker. See AZ_DEFINE_BUDGET and AZ_DECLARE_BUDGET below
// for usage.
#define AZ_BUDGET_GETTER(name) GetAzBudget##name

#if defined(_RELEASE)
#define AZ_DEFINE_BUDGET(name)                                                                                                             \
    ::AZ::Debug::Budget* AZ_BUDGET_GETTER(name)()                                                                                          \
    {                                                                                                                                      \
        return nullptr;                                                                                                                    \
    }
#else
// Usage example:
// In a single C++ source file:
// AZ_DEFINE_BUDGET(AzCore);
//
// Anywhere the budget is used, the budget must be declared (either in a header or in the source file itself)
// AZ_DECLARE_BUDGET(AzCore);
#define AZ_DEFINE_BUDGET(name)                                                                                                             \
    ::AZ::Debug::Budget* AZ_BUDGET_GETTER(name)()                                                                                          \
    {                                                                                                                                      \
        constexpr static uint32_t crc = AZ_CRC_CE(#name);                                                                                  \
        static ::AZ::Debug::Budget* budget = ::AZ::Debug::BudgetTracker::GetBudgetFromEnvironment(#name, crc);                             \
        return budget;                                                                                                                     \
    }
#endif

// If using a budget defined in a different C++ source file, add AZ_DECLARE_BUDGET(yourBudget); somewhere in your source file at namespace
// scope Alternatively, AZ_DECLARE_BUDGET can be used in a header to declare the budget for use across any users of the header
#define AZ_DECLARE_BUDGET(name) ::AZ::Debug::Budget* AZ_BUDGET_GETTER(name)()

// Declare budgets that are core engine budgets, or may be shared/needed across multiple external gems
// You should NOT need to declare user-space or budgets with isolated usage here. Prefer declaring them local to the module(s) that use
// the budget and defining them within a single module to avoid needing to recompile the entire engine.
AZ_DECLARE_BUDGET(Animation);
AZ_DECLARE_BUDGET(Audio);
AZ_DECLARE_BUDGET(AzCore);
AZ_DECLARE_BUDGET(Editor);
AZ_DECLARE_BUDGET(Entity);
AZ_DECLARE_BUDGET(Game);
AZ_DECLARE_BUDGET(System);
AZ_DECLARE_BUDGET(Physics);
