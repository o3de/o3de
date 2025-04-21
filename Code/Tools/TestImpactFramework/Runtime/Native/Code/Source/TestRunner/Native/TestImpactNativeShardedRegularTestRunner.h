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
    //! Sharded test runner for regular tests.
    class NativeShardedRegularTestRunner
        : public NativeShardedTestRunnerBase<NativeRegularTestRunner>
    {
    public:
        using NativeShardedTestRunnerBase<NativeRegularTestRunner>::NativeShardedTestRunnerBase;

    private:
        // NativeShardedTestSystem overrides ...
        [[nodiscard]] NativeRegularTestRunner::ResultType ConsolidateSubJobs(
            const NativeRegularTestRunner::ResultType& result,
            const ShardToParentShardedJobMap& shardToParentShardedJobMap,
            const CompletedShardMap& completedShardMap) override;
    };
} // namespace TestImpact
