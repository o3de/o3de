/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Process/Scheduler/TestImpactProcessSchedulerBus.h>

namespace TestImpact
{
    ProcessCallbackResult GetAggregateProcessCallbackResult(const AZ::EBusAggregateResults<ProcessCallbackResult>& results)
    {
        return std::count(results.values.begin(), results.values.end(), ProcessCallbackResult::Abort) > 0
            ? ProcessCallbackResult::Abort
            : ProcessCallbackResult::Continue;
    }
} // namespace TestImpact
