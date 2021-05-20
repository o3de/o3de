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

    using TestEngineEnumerationCallback = AZStd::function<void(const TestEngineJob& testJob)>;
    using TestEngineRunCallback = AZStd::function<void(const TestEngineJob& testJob, Client::TestRunResult testResult)>;

    class TestEngine
    {
    public:
        TestEngine(
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const RepoPath& cacheDir,
            const RepoPath& artifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary,
            size_t maxConcurrentRuns);

        ~TestEngine();

        AZStd::vector<TestEngineEnumeration> UpdateEnumerationCache(
            const AZStd::vector<const TestTarget*> testTargets,
            Policy::ExecutionFailure executionFailurePolicy,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestEngineEnumerationCallback> callback);

        [[nodiscard]]AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineRegularRun>> RegularRun(
            const AZStd::vector<const TestTarget*> testTargets,
            Policy::TestSharding testShardingPolicy,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            TargetOutputCapture targetOutputCapture,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestEngineRunCallback> callback);

        [[nodiscard]]AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun>> InstrumentedRun(
            const AZStd::vector<const TestTarget*> testTargets,
            Policy::TestSharding testShardingPolicy,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::IntegrityFailure integrityFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            TargetOutputCapture targetOutputCapture,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestEngineRunCallback> callback);

    private:
        size_t m_maxConcurrentRuns = 0;
        AZStd::unique_ptr<TestJobInfoGenerator> m_testJobInfoGenerator;
    };
}
