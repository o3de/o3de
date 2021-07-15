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
#include <TestImpactFramework/TestImpactClientFailureReport.h>

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
    AZStd::unique_ptr<TestImpact::DynamicDependencyMap> ConstructDynamicDependencyMap(
        SuiteType suiteFilter,
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig,
        const TestTargetMetaConfig& testTargetMetaConfig);

    //! Constructs the resolved test target exclude list from the specified list of targets and unresolved test target exclude list.
    AZStd::unordered_set<const TestTarget*> ConstructTestTargetExcludeList(
        const TestTargetList& testTargets,
        const AZStd::vector<AZStd::string>& excludedTestTargets);

    //! Extracts the name information from the specified test targets.
    AZStd::vector<AZStd::string> ExtractTestTargetNames(const AZStd::vector<const TestTarget*> testTargets);    

    //! Generates a test run failure report from the specified test engine job information.
    //! @tparam TestJob The test engine job type.
    template<typename TestJob>
    Client::TestRunFailure GenerateTestRunFailure(const TestJob& testJob)
    {
        if (testJob.GetTestRun().has_value())
        {
            AZStd::vector<Client::TestCaseFailure> testCaseFailures;
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

            return Client::TestRunFailure(Client::TestRunFailure(testJob.GetTestTarget()->GetName(), AZStd::move(testCaseFailures)));
        }
        else
        {
            return Client::TestRunFailure(testJob.GetTestTarget()->GetName(), { });
        }
    }

    //! Generates a sequence failure report from the specified list of test engine jobs.
    //! @tparam TestJob The test engine job type.
    template<typename TestJob>
    Client::SequenceFailure GenerateSequenceFailureReport(const AZStd::vector<TestJob>& testJobs)
    {
        AZStd::vector<Client::ExecutionFailure> executionFailures;
        AZStd::vector<Client::TestRunFailure> testRunFailures;
        AZStd::vector<Client::TargetFailure> timedOutTestRuns;
        AZStd::vector<Client::TargetFailure> unexecutedTestRuns;

        for (const auto& testJob : testJobs)
        {
            switch (testJob.GetTestResult())
            {
            case Client::TestRunResult::FailedToExecute:
            {
                executionFailures.push_back(Client::ExecutionFailure(testJob.GetTestTarget()->GetName(), testJob.GetCommandString()));
                break;
            }
            case Client::TestRunResult::NotRun:
            {
                unexecutedTestRuns.push_back(testJob.GetTestTarget()->GetName());
                break;
            }
            case Client::TestRunResult::Timeout:
            {
                timedOutTestRuns.push_back(testJob.GetTestTarget()->GetName());
                break;
            }
            case Client::TestRunResult::AllTestsPass:
            {
                break;
            }
            case Client::TestRunResult::TestFailures:
            {
                testRunFailures.push_back(GenerateTestRunFailure(testJob));
                break;
            }
            default:
            {
                throw Exception(
                    AZStd::string::format("Unexpected client test run result: %u", static_cast<unsigned int>(testJob.GetTestResult())));
            }
            }
        }

        return Client::SequenceFailure(
            AZStd::move(executionFailures),
            AZStd::move(testRunFailures),
            AZStd::move(timedOutTestRuns),
            AZStd::move(unexecutedTestRuns));
    }
}
