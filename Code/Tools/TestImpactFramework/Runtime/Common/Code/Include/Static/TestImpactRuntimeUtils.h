/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactClientSequenceReport.h>
#include <TestImpactFramework/TestImpactConfiguration.h>

#include <Artifact/TestImpactArtifactException.h>
#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactSourceCoveringTestsList.h>
#include <Target/Common/TestImpactTargetListCompiler.h>
#include <TestEngine/Common/Run/TestImpactTestEngineInstrumentedRun.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumeration.h>
#include <TestImpactTestTargetExclusionList.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //!
    AZStd::vector<TargetDescriptor> ReadTargetDescriptorFiles(const BuildTargetDescriptorConfig& buildTargetDescriptorConfig);

    //!
    template<typename ProductionTarget, typename TestTarget, typename TestTargetMetaMap>
    AZStd::unique_ptr<BuildTargetList<ProductionTarget, TestTarget>> ConstructBuildTargetList(
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig,
        TestTargetMetaMap&& testTargetMetaMap)
    {
        auto targetDescriptors = ReadTargetDescriptorFiles(buildTargetDescriptorConfig);
        auto buildTargets = CompileTargetLists<ProductionTarget, TestTarget, TestTargetMetaMap>(
            AZStd::move(targetDescriptors), AZStd::move(testTargetMetaMap));
        auto&& [productionTargets, testTargets] = buildTargets;
        return AZStd::make_unique<BuildTargetList<ProductionTarget, TestTarget>>(
            AZStd::move(testTargets), AZStd::move(productionTargets));
    }

    //! Constructs the resolved test target exclude list from the specified list of targets and unresolved test target exclude list.
    template<typename TestTarget>
    AZStd::unique_ptr<TestTargetExclusionList<TestTarget>> ConstructTestTargetExcludeList(
        const TargetList<TestTarget>& testTargets,
        const AZStd::vector<ExcludedTarget>& excludedTestTargets)
    {
        AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>> testTargetExcludeList;
        for (const auto& excludedTestTarget : excludedTestTargets)
        {
            if (const auto* testTarget = testTargets.GetTarget(excludedTestTarget.m_name); testTarget != nullptr)
            {
                testTargetExcludeList[testTarget] = excludedTestTarget.m_excludedTests;
            }
        }

        return AZStd::make_unique<TestTargetExclusionList<TestTarget>>(AZStd::move(testTargetExcludeList));
    }

    //! Selects the test targets from the specified list of test targets that are not in the specified test target exclusion list.
    //! @param testTargetExcludeList The test target exclusion list to lookup.
    //! @param testTargets The list of test targets to select from.
    //! @returns The subset of test targets in the specified list that are not on the target exclude list.
    template<typename TestTarget>
    AZStd::pair<AZStd::vector<const TestTarget*>, AZStd::vector<const TestTarget*>>
    SelectTestTargetsByExcludeList(
        const TestTargetExclusionList<TestTarget>& testTargetExcludeList, const AZStd::vector<const TestTarget*>& testTargets)
    {
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;

        if (testTargetExcludeList.IsEmpty())
        {
            return { testTargets, {} };
        }

        for (const auto& testTarget : testTargets)
        {
            if (const auto* excludedTests = testTargetExcludeList.GetExcludedTestsForTarget(testTarget);
                excludedTests != nullptr && excludedTests->empty())
            {
                // If the test filter is empty, the entire suite is excluded
                excludedTestTargets.push_back(testTarget);
            }
            else
            {
                includedTestTargets.push_back(testTarget);
            }
        }

        return { includedTestTargets, excludedTestTargets };
    }

    //! Extracts the name information from the specified test targets.
    template<typename TestTarget>
    AZStd::vector<AZStd::string> ExtractTestTargetNames(const AZStd::vector<const TestTarget*>& testTargets)
    {
        AZStd::vector<AZStd::string> testNames;
        AZStd::transform(
            testTargets.begin(),
            testTargets.end(),
            AZStd::back_inserter(testNames),
            [](const TestTarget* testTarget)
            {
                return testTarget->GetName();
            });

        return testNames;
    }

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
