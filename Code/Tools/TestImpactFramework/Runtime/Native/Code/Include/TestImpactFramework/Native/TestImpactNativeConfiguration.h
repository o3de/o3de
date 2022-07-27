/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactConfiguration.h>

namespace TestImpact
{
    //! Test engine configuration.
    struct NativeTestEngineConfig
    {
        //! Test runner configuration.
        struct TestRunner
        {
            RepoPath m_binary; //!< Path to the test runner binary.
        };

        //! Test instrumentation configuration.
        struct Instrumentation
        {
            RepoPath m_binary; //!< Path to the test instrumentation binary.
        };

        TestRunner m_testRunner;
        Instrumentation m_instrumentation;
    };

    //! Build target configuration.
    struct NativeTargetConfig
    {
        //! Test target sharding configuration.
        struct ShardedTarget
        {
            AZStd::string m_name; //!< Name of test target this sharding configuration applies to.
            ShardConfiguration m_configuration; //!< The shard configuration to use.
        };

        RepoPath m_outputDirectory; //!< Path to the test target binary directory.
        AZStd::vector<ExcludedTarget> m_excludedRegularTestTargets; //!< Test targets to always exclude from regular test run sequences.
        AZStd::vector<ExcludedTarget> m_excludedInstrumentedTestTargets; //!< Test targets to always exclude from instrumented test run sequences.
        AZStd::vector<ShardedTarget> m_shardedTestTargets; //!< Test target shard configurations (opt-in).
    };

    //! Native runtime configuration.
    struct NativeRuntimeConfig
    {
        RuntimeConfig m_commonConfig;
        NativeTestEngineConfig m_testEngine;
        NativeTargetConfig m_target;
    };
} // namespace TestImpact
