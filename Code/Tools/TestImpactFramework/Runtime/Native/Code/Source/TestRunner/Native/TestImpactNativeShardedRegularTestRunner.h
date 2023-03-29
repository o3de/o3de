/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/Shard/TestImpactNativeShardedTestRunnerBase.h>

namespace TestImpact
{
    //!
    class NativeShardedRegularTestRunner
        : public NativeShardedTestRunnerBase<NativeRegularTestRunner>
    {
    public:
        //!
        using NativeShardedTestRunnerBase<NativeRegularTestRunner>::NativeShardedTestRunnerBase;

    private:
        // NativeShardedTestSystem overrides ...
        [[nodiscard]] typename NativeRegularTestRunner::ResultType ConsolidateSubJobs(
            const typename NativeRegularTestRunner::ResultType& result,
            const ShardToParentShardedJobMap& shardToParentShardedJobMap,
            const CompletedShardMap& completedShardMap) override;
    };
} // namespace TestImpact
