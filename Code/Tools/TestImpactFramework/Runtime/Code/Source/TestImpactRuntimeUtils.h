/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactConfiguration.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactClientSequenceReport.h>

#include <Artifact/Static/TestImpactTestTargetMeta.h>
#include <Artifact/Static/TestImpactBuildTargetDescriptor.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactSourceCoveringTestsList.h>
#include <Target/TestImpactTestTarget.h>
#include <TestEngine/Enumeration/TestImpactTestEnumeration.h>
#include <TestEngine/TestImpactTestEngineInstrumentedRun.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Construct a dynamic dependency map from the build target descriptors and test target metas.
    AZStd::unique_ptr<DynamicDependencyMap> ConstructDynamicDependencyMap(
        SuiteType suiteFilter,
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig,
        const TestTargetMetaConfig& testTargetMetaConfig);

    //! Constructs the resolved test target exclude list from the specified list of targets and unresolved test target exclude list.
    AZStd::unordered_set<const TestTarget*> ConstructTestTargetExcludeList(
        const TestTargetList& testTargets,
        const AZStd::vector<AZStd::string>& excludedTestTargets);

    //! Extracts the name information from the specified test targets.
    AZStd::vector<AZStd::string> ExtractTestTargetNames(const AZStd::vector<const TestTarget*>& testTargets);

    //! Generates a test run failure report from the specified test engine job information.
    //! @tparam TestJob The test engine job type.
    template<typename TestJob>
    AZStd::vector<Client::TestCaseFailure> GenerateTestCaseFailures(const TestJob& testJob)
    {
        AZStd::vector<Client::TestCaseFailure> testCaseFailures;

        if (testJob.GetTestRun().has_value())
        {
            for (const auto& testSuite : testJob.GetTestRun()->GetTestSuites())
            {
                AZStd::vector<Client::TestFailure> testFailures;
                for (const auto& testCase : testSuite.m_tests)
                {
                    if (testCase.m_result.value_or(TestRunResult::Passed) == TestRunResult::Failed)
                    {
                        testFailures.push_back(Client::TestFailure(testCase.m_name, "No error message retrieved"));
                    }
                }
    
                if (!testFailures.empty())
                {
                    testCaseFailures.push_back(Client::TestCaseFailure(testSuite.m_name, AZStd::move(testFailures)));
                }
            }
        }

        return testCaseFailures;
    }

    template<typename TestJob>
    Client::TestRunReport GenerateTestRunReport(
        TestSequenceResult result,
        AZStd::chrono::high_resolution_clock::time_point startTime,
        AZStd::chrono::milliseconds duration,
        const AZStd::vector<TestJob>& testJobs)
    {
        AZStd::vector<Client::TestRun> passingTests;
        AZStd::vector<Client::TestRunWithTestFailures> failingTests;
        AZStd::vector<Client::TestRun> executionFailureTests;
        AZStd::vector<Client::TestRun> timedOutTests;
        AZStd::vector<Client::TestRun> unexecutedTests;
        
        for (const auto& testJob : testJobs)
        {
            // Test job start time relative to start time
            const auto relativeStartTime =
                AZStd::chrono::high_resolution_clock::time_point() +
                AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(testJob.GetStartTime() - startTime);

            Client::TestRun clientTestRun(
                testJob.GetTestTarget()->GetName(), testJob.GetCommandString(), relativeStartTime, testJob.GetDuration(),
                testJob.GetTestResult());

            switch (testJob.GetTestResult())
            {
            case Client::TestRunResult::FailedToExecute:
            {
                executionFailureTests.push_back(clientTestRun);
                break;
            }
            case Client::TestRunResult::NotRun:
            {
                unexecutedTests.push_back(clientTestRun);
                break;
            }
            case Client::TestRunResult::Timeout:
            {
                timedOutTests.push_back(clientTestRun);
                break;
            }
            case Client::TestRunResult::AllTestsPass:
            {
                passingTests.push_back(clientTestRun);
                break;
            }
            case Client::TestRunResult::TestFailures:
            {
                failingTests.emplace_back(AZStd::move(clientTestRun), GenerateTestCaseFailures(testJob));
                break;
            }
            default:
            {
                throw Exception(
                    AZStd::string::format("Unexpected client test run result: %u", static_cast<unsigned int>(testJob.GetTestResult())));
            }
            }
        }
        
        return Client::TestRunReport(
            result,
            startTime,
            duration,
            AZStd::move(passingTests),
            AZStd::move(failingTests),
            AZStd::move(executionFailureTests),
            AZStd::move(timedOutTests),
            AZStd::move(unexecutedTests));
    }
} // namespace TestImpact
