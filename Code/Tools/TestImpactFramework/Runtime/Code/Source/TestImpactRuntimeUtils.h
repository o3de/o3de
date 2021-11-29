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

#include <Artifact/Static/TestImpactNativeTestTargetMeta.h>
#include <Artifact/Static/TestImpactNativeTargetDescriptor.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactSourceCoveringTestsList.h>
#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestEngine/Common/TestImpactTestEngineInstrumentedRun.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumeration.h>
#include <TestImpactTestTargetExclusionList.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! Construct a build target list from the build target descriptors and test target metas.
    AZStd::unique_ptr<BuildTargetList<NativeTestTarget, NativeProductionTarget>> ConstructNativeBuildTargetList(
        SuiteType suiteFilter,
        const NativeTargetDescriptorConfig& NativeTargetDescriptorConfig,
        const TestTargetMetaConfig& testTargetMetaConfig);

    //! Constructs the resolved test target exclude list from the specified list of targets and unresolved test target exclude list.
    AZStd::unique_ptr<TestTargetExclusionList> ConstructTestTargetExcludeList(
        const TargetList<NativeTestTarget>& testTargets,
        AZStd::vector<TargetConfig::ExcludedTarget>&& excludedTestTargets);

    //! Selects the test targets from the specified list of test targets that are not in the specified test target exclusion list.
    //! @param testTargetExcludeList The test target exclusion list to lookup.
    //! @param testTargets The list of test targets to select from.
    //! @returns The subset of test targets in the specified list that are not on the target exclude list.
    AZStd::pair<
        AZStd::vector<const NativeTestTarget*>,
        AZStd::vector<const NativeTestTarget*>>
    SelectTestTargetsByExcludeList(
        const TestTargetExclusionList& testTargetExcludeList,
        const AZStd::vector<const NativeTestTarget*>& testTargets);

    //! Extracts the name information from the specified test targets.
    AZStd::vector<AZStd::string> ExtractTestTargetNames(
        const AZStd::vector<const NativeTestTarget*>& testTargets);

    //! Generates the test suites from the specified test engine job information.
    //! @tparam TestJob The test engine job type.
    template<typename TestJob>
    AZStd::vector<Client::Test> GenerateClientTests(const TestJob& testJob)
    {
        AZStd::vector<Client::Test> tests;

        if (testJob.GetTestRun().has_value())
        {
            for (const auto& testSuite : testJob.GetTestRun()->GetTestSuites())
            {
                for (const auto& testCase : testSuite.m_tests)
                {
                    auto result = Client::TestResult::NotRun;
                    if (testCase.m_result.has_value())
                    {
                        if (testCase.m_result.value() == TestRunResult::Passed)
                        {
                            result = Client::TestResult::Passed;
                        }
                        else if (testCase.m_result.value() == TestRunResult::Failed)
                        {
                            result = Client::TestResult::Failed;
                        }
                        else
                        {
                            throw RuntimeException(AZStd::string::format(
                                "Unexpected test run result: %u", aznumeric_cast<AZ::u32>(testCase.m_result.value())));
                        }
                    }

                    const auto name = AZStd::string::format("%s.%s", testSuite.m_name.c_str(), testCase.m_name.c_str());
                    tests.push_back(Client::Test(name, result));
                }
            }
        }

        return tests;
    }

    template<typename TestJob>
    Client::TestRunReport GenerateTestRunReport(
        TestSequenceResult result,
        AZStd::chrono::high_resolution_clock::time_point startTime,
        AZStd::chrono::milliseconds duration,
        const AZStd::vector<TestJob>& testJobs)
    {
        AZStd::vector<Client::PassingTestRun> passingTests;
        AZStd::vector<Client::FailingTestRun> failingTests;
        AZStd::vector<Client::TestRunWithExecutionFailure> executionFailureTests;
        AZStd::vector<Client::TimedOutTestRun> timedOutTests;
        AZStd::vector<Client::UnexecutedTestRun> unexecutedTests;
        
        for (const auto& testJob : testJobs)
        {
            // Test job start time relative to start time
            const auto relativeStartTime =
                AZStd::chrono::high_resolution_clock::time_point() +
                AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(testJob.GetStartTime() - startTime);

            Client::TestRunBase clientTestRun(
                testJob.GetTestTarget()->GetName(), testJob.GetCommandString(), relativeStartTime, testJob.GetDuration(),
                testJob.GetTestResult());

            switch (testJob.GetTestResult())
            {
            case Client::TestRunResult::FailedToExecute:
            {
                executionFailureTests.emplace_back(AZStd::move(clientTestRun));
                break;
            }
            case Client::TestRunResult::NotRun:
            {
                unexecutedTests.emplace_back(AZStd::move(clientTestRun));
                break;
            }
            case Client::TestRunResult::Timeout:
            {
                timedOutTests.emplace_back(AZStd::move(clientTestRun));
                break;
            }
            case Client::TestRunResult::AllTestsPass:
            {
                passingTests.emplace_back(AZStd::move(clientTestRun), GenerateClientTests(testJob));
                break;
            }
            case Client::TestRunResult::TestFailures:
            {
                failingTests.emplace_back(AZStd::move(clientTestRun), GenerateClientTests(testJob));
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
