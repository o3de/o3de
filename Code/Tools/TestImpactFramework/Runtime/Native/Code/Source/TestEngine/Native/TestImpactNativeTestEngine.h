/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactConfiguration.h>
#include <TestImpactFramework/TestImpactTestSequence.h>
#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <TestEngine/Common/TestImpactTestEngine.h>
#include <TestEngine/Common/Enumeration/TestImpactTestEngineEnumeration.h>
#include <TestEngine/Common/Run/TestImpactTestEngineInstrumentedRun.h>
#include <TestEngine/Common/Run/TestImpactTestEngineRegularRun.h>

#include <TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    class NativeTestTarget;
    class NativeTestEnumerationJobInfoGenerator;
    class NativeRegularTestRunJobInfoGenerator;
    class NativeInstrumentedTestRunJobInfoGenerator;
    struct NativeShardedArtifactDir;
    class NativeShardedRegularTestRunJobInfoGenerator;
    class NativeShardedInstrumentedTestRunJobInfoGenerator;
    class NativeTestEnumerator;
    class NativeInstrumentedTestRunner;
    class NativeRegularTestRunner;
    class NativeShardedInstrumentedTestRunner;
    class NativeShardedRegularTestRunner;

    //! Provides the front end for performing test enumerations and test runs.
    class NativeTestEngine
    {
    public:
        //! Configures the test engine with the necessary path information for launching test targets and managing the artifacts they produce.
        //! @param repoRootDir Root path where source files are found (including subfolders).
        //! @param targetBinaryDir Path to where the test target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param shardedArtifactDir Path to the transient directory where sharded test artifacts are produced.
        //! @param testRunnerBinary Path to the binary responsible for launching test targets that have the TestRunner launch method.
        //! @param instrumentBinary Path to the binary responsible for launching test targets with test coverage instrumentation.
        //! @param maxConcurrentRuns The maximum number of concurrent test targets that can be in flight at any given moment.
        NativeTestEngine(
            const RepoPath& repoRootDir,
            const RepoPath& targetBinaryDir,
            const ArtifactDir& artifactDir,
            const NativeShardedArtifactDir& shardedArtifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary,
            size_t maxConcurrentRuns);

        ~NativeTestEngine();

        //! Updates the cached enumerations for the specified test targets.
        //! @note Whilst test runs will make use of this cache for test target sharding it is the responsibility of the client to
        //! ensure any stale caches are up to date by calling this function. No attempt to maintain internal consistency will be made
        //! by the test engine itself.
        //! @param testTargets The test targets to enumerate.
        //! @param executionFailurePolicy The policy for how enumeration execution failures should be handled.
        //! @param testTargetTimeout The maximum duration a test target may be in-flight for before being forcefully terminated (infinite if empty).
        //! @param globalTimeout The maximum duration the enumeration sequence may run before being forcefully terminated (infinite if empty).
        //! @param callback The client callback function to handle completed test target enumerations.
        //! @ returns The sequence result and the enumerations for the target that were enumerated.
        //AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineEnumeration>> UpdateEnumerationCache(
        //    const AZStd::vector<const NativeTestTarget*>& testTargets,
        //    Policy::ExecutionFailure executionFailurePolicy,
        //    Policy::TestFailure testFailurePolicy,
        //    AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        //    AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        //    AZStd::optional<TestEngineJobCompleteCallback> callback) const;

        //! Performs a test run without any instrumentation and, for each test target, returns the test run results and metrics about the run.
        //! @param testTargets The test targets to run.
        //! @param executionFailurePolicy Policy for how test execution failures should be handled.
        //! @param testFailurePolicy Policy for how test targets with failing tests should be handled.
        //! @param targetOutputCapture Policy for how test target standard output should be captured and handled.
        //! @param testTargetTimeout The maximum duration a test target may be in-flight for before being forcefully terminated (infinite if empty).
        //! @param globalTimeout The maximum duration the enumeration sequence may run before being forcefully terminated (infinite if empty).
        //! @ returns The sequence result and the test run results for the test targets that were run.
        [[nodiscard]] TestEngineRegularRunResult<NativeTestTarget>
        RegularRun(
            const AZStd::vector<const NativeTestTarget*>& testTargets,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            Policy::TargetOutputCapture targetOutputCapture,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout) const;

        //! Performs a test run with instrumentation and, for each test target, returns the test run results, coverage data and metrics about the run.
        //! @param testTargets The test targets to run.
        //! @param executionFailurePolicy Policy for how test execution failures should be handled.
        //! @param integrityFailurePolicy Policy for how integrity failures of the test impact data and source tree model should be handled.
        //! @param testFailurePolicy Policy for how test targets with failing tests should be handled.
        //! @param targetOutputCapture Policy for how test target standard output should be captured and handled.
        //! @param testTargetTimeout The maximum duration a test target may be in-flight for before being forcefully terminated (infinite if empty).
        //! @param globalTimeout The maximum duration the enumeration sequence may run before being forcefully terminated (infinite if empty).
        //! @ returns The sequence result and the test run results and test coverages for the test targets that were run.
        [[nodiscard]] TestEngineInstrumentedRunResult<NativeTestTarget, TestCoverage>
        InstrumentedRun(
            const AZStd::vector<const NativeTestTarget*>& testTargets,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::IntegrityFailure integrityFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            Policy::TargetOutputCapture targetOutputCapture,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout) const;

    private:
        //! Cleans up the artifacts directory of any artifacts from previous runs.
        void DeleteXmlArtifacts() const;

        //! Helper function to generate the test target and enumeration pairs for a given set of test targets.
        AZStd::vector<TestTargetAndEnumeration> GenerateTestTargetAndEnumerations(const AZStd::vector<const NativeTestTarget*>& testTargets) const;

        AZStd::unique_ptr<NativeTestEnumerationJobInfoGenerator> m_enumerationTestJobInfoGenerator;
        AZStd::unique_ptr<NativeRegularTestRunJobInfoGenerator> m_regularTestJobInfoGenerator;
        AZStd::unique_ptr<NativeInstrumentedTestRunJobInfoGenerator> m_instrumentedTestJobInfoGenerator;
        AZStd::unique_ptr<NativeShardedRegularTestRunJobInfoGenerator> m_shardedRegularTestJobInfoGenerator;
        AZStd::unique_ptr<NativeShardedInstrumentedTestRunJobInfoGenerator> m_shardedInstrumentedTestJobInfoGenerator;
        AZStd::unique_ptr<NativeTestEnumerator> m_testEnumerator;
        AZStd::unique_ptr<NativeInstrumentedTestRunner> m_instrumentedTestRunner;
        AZStd::unique_ptr<NativeRegularTestRunner> m_testRunner;
        AZStd::unique_ptr<NativeShardedInstrumentedTestRunner> m_shardedInstrumentedTestRunner;
        AZStd::unique_ptr<NativeShardedRegularTestRunner> m_shardedTestRunner;
        ArtifactDir m_artifactDir;
        NativeShardedArtifactDir m_shardedArtifactDir;
    };
} // namespace TestImpact
