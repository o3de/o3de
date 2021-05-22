/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactTestSequence.h>
#include <TestImpactFramework/TestImpactClientTestRun.h>
#include <TestImpactFramework/TestImpactRuntime.h>

#include <TestEngine/TestImpactTestEngineEnumeration.h>
#include <TestEngine/TestImpactTestEngineInstrumentedRun.h>
#include <TestEngine/TestImpactTestEngineRegularRun.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    class TestTarget;
    class TestJobInfoGenerator;
    class TestEnumerator;
    class InstrumentedTestRunner;
    class TestRunner;

    //! Callback for when a given test engine job completes.
    using TestEngineJobCompleteCallback = AZStd::function<void(const TestEngineJob& testJob)>;

    //! Provides the front end for performing test enumerations and test runs.
    class TestEngine
    {
    public:
        //! Configures the test engine with the necessary path information for launching test targets and managing the artifacts they produce.
        //! @param sourceDir
        //! @param targetBinaryDir
        //! @param cacheDir
        //! @param artifactDir
        //! @param testRunnerBinary
        //! @param instrumentBinary
        //! @param maxConcurrentRuns
        TestEngine(
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const RepoPath& cacheDir,
            const RepoPath& artifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary,
            size_t maxConcurrentRuns);

        ~TestEngine();

        //! Updates the cached enumerations for the specified test targets.
        //! @note Whilst test runs will make use of this cache for test target sharding it is the responsibility of the client to
        //! ensure any stale caches are up to date by calling this function. No attempt to maintain internal consistency will be made
        //! by the test engine itself.
        //! @param testTargets
        //! @param executionFailurePolicy
        //! @param testTargetTimeout
        //! @param globalTimeout
        //! @param callback
        //! @ returns
        AZStd::pair < TestSequenceResult, AZStd::vector<TestEngineEnumeration>> UpdateEnumerationCache(
            const AZStd::vector<const TestTarget*>& testTargets,
            Policy::ExecutionFailure executionFailurePolicy,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestEngineJobCompleteCallback> callback);

        //! Performs a test run without any instrumentation and, for each test target, returns the test run results and metrics about the run.
        //! @param
        //! @param
        //! @param
        //! @param
        //! @param
        //! @param
        //! @param
        //! @param
        //! @ returns
        [[nodiscard]]AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineRegularRun>> RegularRun(
            const AZStd::vector<const TestTarget*>& testTargets,
            Policy::TestSharding testShardingPolicy,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            TargetOutputCapture targetOutputCapture,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestEngineJobCompleteCallback> callback);

        //! Performs a test run with instrumentation and, for each test target, returns the test run results, coverage data and metrics about the run.
        //! @param
        //! @param
        //! @param
        //! @param
        //! @param
        //! @param
        //! @param
        //! @param
        //! @param
        //! @ returns
        [[nodiscard]]AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun>> InstrumentedRun(
            const AZStd::vector<const TestTarget*>& testTargets,
            Policy::TestSharding testShardingPolicy,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::IntegrityFailure integrityFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            TargetOutputCapture targetOutputCapture,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestEngineJobCompleteCallback> callback);

    private:
        size_t m_maxConcurrentRuns = 0;
        AZStd::unique_ptr<TestJobInfoGenerator> m_testJobInfoGenerator;
        AZStd::unique_ptr<TestEnumerator> m_testEnumerator;
        AZStd::unique_ptr<InstrumentedTestRunner> m_instrumentedTestRunner;
        AZStd::unique_ptr<TestRunner> m_testRunner;
    };
}
