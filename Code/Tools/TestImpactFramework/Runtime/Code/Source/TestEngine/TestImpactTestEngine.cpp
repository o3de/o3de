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

#include <TestEngine/TestImpactTestEngine.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerator.h>
#include <TestEngine/Run/TestImpactInstrumentedTestRunner.h>
#include <TestEngine/Run/TestImpactTestRunner.h>
#include <TestEngine/JobRunner/TestImpactTestJobInfoGenerator.h>

#include <AzCore/std/containers/unordered_map.h>

namespace TestImpact
{
    TestEngine::TestEngine(
        const AZ::IO::Path& sourceDir,
        const AZ::IO::Path& targetBinaryDir,
        const AZ::IO::Path& cacheDir,
        const AZ::IO::Path& artifactDir,
        const AZ::IO::Path& testRunnerBinary,
        const AZ::IO::Path& instrumentBinary,
        size_t maxConcurrentRuns)
        : m_maxConcurrentRuns(maxConcurrentRuns)
        , m_testJobInfoGenerator(AZStd::make_unique<TestJobInfoGenerator>(sourceDir, targetBinaryDir, cacheDir, artifactDir, testRunnerBinary, instrumentBinary))
    {
    }

    TestEngine::~TestEngine() = default;

    AZStd::vector<TestEngineEnumeration> TestEngine::UpdateEnumerationCache(
        const AZStd::vector<const TestTarget*> testTargets,
        ExecutionFailurePolicy executionFailurePolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineEnumerationCallback> enumerationCallback)
    {
        AZStd::vector<TestEnumerator::JobInfo> jobInfos;
        AZStd::unordered_map<TestEnumerator::JobInfo::IdType, TestEngineJob> engineJobs;
        AZStd::vector<TestEngineEnumeration> engineEnumerations;
        engineEnumerations.reserve(testTargets.size());

        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(
                m_testJobInfoGenerator->GenerateTestEnumerationJobInfo(testTargets[jobId], { jobId }, TestEnumerator::JobInfo::CachePolicy::Write));
        }

        const auto jobCallback = [&testTargets, &engineJobs, &enumerationCallback]
        (const TestImpact::TestEnumerator::JobInfo& jobInfo, const TestImpact::JobMeta& meta)
        {
            const auto& [item, success] =
                engineJobs.emplace(jobInfo.GetId().m_value, TestEngineJob(testTargets[jobInfo.GetId().m_value], jobInfo.GetCommand().m_args, meta));
            if (enumerationCallback.has_value())
            {
                (*enumerationCallback)(item->second);
            }
        };

        TestImpact::TestEnumerator testEnumerator(jobCallback, m_maxConcurrentRuns, testTargetTimeout, globalTimeout);

        const auto cacheExceptionPolicy = executionFailurePolicy == ExecutionFailurePolicy::Abort
            ? TestEnumerator::CacheExceptionPolicy::OnCacheWriteFailure
            : TestEnumerator::CacheExceptionPolicy::Never;

        const auto jobExecutionPolicy = executionFailurePolicy == ExecutionFailurePolicy::Abort
            ? (TestEnumerator::JobExceptionPolicy::OnExecutedWithFailure | TestEnumerator::JobExceptionPolicy::OnFailedToExecute)
            : TestEnumerator::JobExceptionPolicy::Never;

        auto enumerationJobs = testEnumerator.Enumerate(jobInfos, cacheExceptionPolicy, jobExecutionPolicy);
        for (auto& job : enumerationJobs)
        {
            auto engineJob = engineJobs.find(job.GetJobInfo().GetId().m_value);
            // DO EVAL!!
            TestEngineEnumeration enumeration(AZStd::move(engineJob->second), job.ReleasePayload());
            engineEnumerations.push_back(AZStd::move(enumeration));
        }

        return engineEnumerations;
    }
}
