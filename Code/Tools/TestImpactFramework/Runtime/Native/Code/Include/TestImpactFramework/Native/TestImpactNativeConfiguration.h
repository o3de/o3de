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
    //! Temporary workspace configuration.
    struct NativeShardedArtifactDir
    {
        RepoPath m_shardedTestRunArtifactDirectory; //!< Path to read and write sharded test run artifacts to and from.
        RepoPath m_shardedCoverageArtifactDirectory; //!< Path to read and write coverage artifacts to and from.
    };


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

    struct NativeExcludedTargets
    {
        ExcludedTargets m_excludedRegularTestTargets; //!< Test targets to always exclude from regular test run sequences.
        ExcludedTargets m_excludedInstrumentedTestTargets; //!< Test targets to always exclude from instrumented test run sequences.
    };

    //! Build target configuration.
    struct NativeTargetConfig
    {
        RepoPath m_outputDirectory; //!< Path to the test target binary directory.
        NativeExcludedTargets m_excludedTargets; //!< Test targets to exclude from regular and/or instrumented runs.
    };

    //! Native runtime configuration.
    struct NativeRuntimeConfig
    {
        RuntimeConfig m_commonConfig;
        WorkspaceConfig m_workspace;
        NativeShardedArtifactDir m_shardedArtifactDir;
        NativeTestEngineConfig m_testEngine;
        NativeTargetConfig m_target;
    };
} // namespace TestImpact
