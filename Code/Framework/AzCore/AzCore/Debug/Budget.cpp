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

AZ_DEFINE_BUDGET(AzCore);
AZ_DEFINE_BUDGET(Editor);
AZ_DEFINE_BUDGET(Entity);
AZ_DEFINE_BUDGET(Game);
AZ_DEFINE_BUDGET(System);
AZ_DEFINE_BUDGET(Audio);
AZ_DEFINE_BUDGET(Animation);

namespace AZ::Debug
{
    // Global container for all registered budgets
    class BudgetRegistry
    {
    public:
        static BudgetRegistry& Instance()
        {
        }

    private:
    };

    void Budget::ResetAll()
    {
    }

    Budget::Budget(const char* name)
        : m_name{ name }
        , m_crc{ Crc32(name) }
    {
        // TODO: Register budget with singleton budget registry
    }
} // namespace AZ::Debug
