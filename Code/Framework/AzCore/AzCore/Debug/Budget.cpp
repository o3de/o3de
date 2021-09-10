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
        // TODO: Budget implementation for tracking budget wall time per-core, memory, etc.
    };

    Budget::Budget(const char* name)
        : m_name{ name }
        , m_crc{ Crc32(name) }
    {
    }

    Budget::Budget(const char* name, uint32_t crc)
        : m_name{ name }
        , m_crc{ crc }
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
    }

    void Budget::BeginProfileRegion()
    {
    }

    void Budget::EndProfileRegion()
    {
    }

    void Budget::TrackAllocation(uint64_t)
    {
    }

    void Budget::UntrackAllocation(uint64_t)
    {
    }
} // namespace AZ::Debug
