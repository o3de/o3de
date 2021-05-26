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
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig,
        const TestTargetMetaConfig& testTargetMetaConfig);

    //! Constructs the 
    AZStd::unordered_set<const TestTarget*> ConstructTestTargetExcludeList(
        const TestTargetList& testTargets,
        const AZStd::vector<AZStd::string>& excludedTstTargets);

    AZStd::vector<AZStd::string> ExtractTestTargetNames(const AZStd::vector<const TestTarget*> testTargets);

    SourceCoveringTestsList CreateSourceCoveringTestFromTestCoverages(
        const AZStd::vector<TestEngineInstrumentedRun>& jobs,
        const RepoPath& root);

    template<typename TestJob>
    Client::TestRunFailure ExtractTestRunFailure(const TestJob& testJob)
    {
        if (testJob.GetTestRun().has_value())
        {
            AZStd::vector<Client::TestCaseFailure> testCaseFailures;
            for (const auto& testSuite : testJob.GetTestRun()->GetTestSuites())
            {
                AZStd::vector<Client::TestFailure> testFailures;
                for (const auto& testCase : testSuite.m_tests)
                {
                    if(testCase.m_result.value_or(TestRunResult::Passed) == TestRunResult::Failed)
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

    template<typename TestJob>
    Client::SequenceFailure CreateSequenceFailureReport(const AZStd::vector<TestJob>& testJobs)
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
                testRunFailures.push_back(ExtractTestRunFailure(testJob));
                break;
            }
            default:
            {
                throw Exception(AZStd::string::format("Unexpected client test run result: %u", static_cast<unsigned int>(testJob.GetTestResult())));
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
