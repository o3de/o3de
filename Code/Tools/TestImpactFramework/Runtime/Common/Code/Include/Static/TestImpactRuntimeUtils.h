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
#include <TestImpactFramework/TestImpactRuntimeException.h>

#include <TestImpactFramework/TestImpactRuntime.h>
#include <Artifact/TestImpactArtifactException.h>
#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactSourceCoveringTestsList.h>
#include <TestEngine/Common/Run/TestImpactTestEngineInstrumentedRun.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumeration.h>
#include <TestImpactTestTargetExclusionList.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    static inline constexpr const char* const LogCallSite = "TestImpact";

    //! Simple helper class for tracking basic timing information.
    class Timer
    {
    public:
        Timer();

        //! Returns the time point that the timer was instantiated.
        AZStd::chrono::high_resolution_clock::time_point GetStartTimePoint() const;

        //! Returns the time point that the timer was instantiated relative to the specified starting time point.
        AZStd::chrono::high_resolution_clock::time_point GetStartTimePointRelative(const Timer& start) const;

        //! Returns the time elapsed (in milliseconds) since the timer was instantiated.
        AZStd::chrono::milliseconds GetElapsedMs() const;

        //! Returns the current time point relative to the time point the timer was instantiated.
        AZStd::chrono::high_resolution_clock::time_point GetElapsedTimepoint() const;

    private:
        AZStd::chrono::high_resolution_clock::time_point m_startTime;
    };

    //! Attempts to read all of the target descriptor files from the specified configuration directories.
    AZStd::vector<TargetDescriptor> ReadTargetDescriptorFiles(const BuildTargetDescriptorConfig& buildTargetDescriptorConfig);

    //! Handler for test run complete events.
    template<typename TestTarget>
    class TestRunCompleteCallbackHandler
    {
    public:
        TestRunCompleteCallbackHandler(size_t totalTests, AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
            : m_totalTests(totalTests)
            , m_testCompleteCallback(testCompleteCallback)
        {
        }

        void operator()(const TestEngineJob<TestTarget>& testJob)
        {
            if (m_testCompleteCallback.has_value())
            {
                Client::TestRunBase testRun(
                    testJob.GetTestTarget()->GetName(),
                    testJob.GetCommandString(),
                    testJob.GetStartTime(),
                    testJob.GetDuration(),
                    testJob.GetTestResult());

                (*m_testCompleteCallback)(testRun, ++m_numTestsCompleted, m_totalTests);
            }
        }

    private:
        const size_t m_totalTests; //!< The total number of tests to run for the entire sequence.
        size_t m_numTestsCompleted = 0; //!< The running total of tests that have completed.
        AZStd::optional<TestRunCompleteCallback> m_testCompleteCallback;
    };

    //! Updates the dynamic dependency map and serializes the entire map to disk.
    template<typename ProductionTarget, typename TestTarget, typename TestCoverage>
    [[nodiscard]] AZStd::optional<bool> UpdateAndSerializeDynamicDependencyMap(
        DynamicDependencyMap<ProductionTarget, TestTarget>& dynamicDependencyMap,
        const AZStd::vector<TestEngineInstrumentedRun<TestTarget, TestCoverage>>& jobs,
        Policy::FailedTestCoverage failedTestCoveragePolicy,
        Policy::IntegrityFailure integrationFailurePolicy,
        const RepoPath& repoRoot,
        const RepoPath& sparTiaFile)
    {
        try
        {
            const auto sourceCoverageTestsList =
                CreateSourceCoveringTestFromTestCoverages(dynamicDependencyMap, jobs, failedTestCoveragePolicy, repoRoot);

            if (sourceCoverageTestsList.GetNumSources() == 0)
            {
                return AZStd::nullopt;
            }

            dynamicDependencyMap.ReplaceSourceCoverage(sourceCoverageTestsList);
            const auto sparTia = dynamicDependencyMap.ExportSourceCoverage();
            const auto sparTiaData = SerializeSourceCoveringTestsList(sparTia);
            WriteFileContents<RuntimeException>(sparTiaData, sparTiaFile);
            return true;
        } catch (const RuntimeException& e)
        {
            if (integrationFailurePolicy == Policy::IntegrityFailure::Abort)
            {
                throw e;
            }
            else
            {
                AZ_Error(LogCallSite, false, e.what());
            }
        }

        return AZStd::nullopt;
    }

    //! Prunes the existing coverage for the specified jobs and creates the consolidated source covering tests list from the
    //! test engine instrumented run jobs.
    template<typename ProductionTarget, typename TestTarget, typename TestCoverage>
    SourceCoveringTestsList CreateSourceCoveringTestFromTestCoverages(
        DynamicDependencyMap<ProductionTarget, TestTarget>& dynamicDependencyMap,
        const AZStd::vector<TestEngineInstrumentedRun<TestTarget, TestCoverage>>& jobs,
        Policy::FailedTestCoverage failedTestCoveragePolicy,
        const RepoPath& repoRoot)
    {
        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZStd::string>> coverage;
        for (const auto& job : jobs)
        {
            // First we must remove any existing coverage for the test target so as to not end up with source remnants from previous
            // coverage that is no longer covered by this revision of the test target
            dynamicDependencyMap.RemoveTestTargetFromSourceCoverage(job.GetTestTarget());

            // Next we will update the coverage of test targets that completed (with or without failures), unless the failed test coverage
            // policy dictates we should instead discard the coverage of test targets with failing tests
            const auto testResult = job.GetTestResult();

            if (failedTestCoveragePolicy == Policy::FailedTestCoverage::Discard && testResult == Client::TestRunResult::TestFailures)
            {
                // Discard the coverage for this job
                continue;
            }

            if (testResult == Client::TestRunResult::AllTestsPass || testResult == Client::TestRunResult::TestFailures)
            {
                if (testResult == Client::TestRunResult::AllTestsPass)
                {
                    // Passing tests should have coverage data, otherwise something is very wrong
                    AZ_TestImpact_Eval(
                        job.GetCoverge().has_value(),
                        RuntimeException,
                        AZStd::string::format(
                            "Test target '%s' completed its test run successfully but produced no coverage data. Command string: '%s'",
                            job.GetTestTarget()->GetName().c_str(),
                            job.GetCommandString().c_str()));
                }

                if (!job.GetCoverge().has_value())
                {
                    // When a test run completes with failing tests but produces no coverage artifact that's typically a sign of the
                    // test aborting due to an unhandled exception, in which case ignore it and let it be picked up in the failure report
                    continue;
                }

                // Add the sources covered by this test target to the coverage map
                const auto& jobCoverage = job.GetCoverge().value();
                const auto* testTarget = job.GetTestTarget();
                if (jobCoverage.GetCoverageLevel() == CoverageLevel::Module)
                {
                    // Module coverage considers all sources that are parented by the module build target as being covered by the test
                    // target(s) that cover the module
                    for (const auto& coveredModule : job.GetCoverge().value().GetModuleCoverages())
                    {
                        const auto moduleName = AZ::IO::Path(coveredModule.m_path.Stem()).String();
                        const auto buildTargetName = dynamicDependencyMap.GetBuildTargetList()->GetTargetNameFromOutputNameOrThrow(moduleName);
                        const auto buildTarget = dynamicDependencyMap.GetBuildTargetList()->GetBuildTargetOrThrow(buildTargetName);
                        buildTarget.Visit(
                            [&coverage, &repoRoot, testTarget](auto&& target)
                            {
                                const auto addSourcesToCoverage = [&coverage, &repoRoot, testTarget](const AZStd::vector<RepoPath>& sources)
                                {
                                    for (const auto& coveredSource : sources)
                                    {
                                        const auto absoluteSourcePath = repoRoot / coveredSource;
                                        coverage[absoluteSourcePath.String()].insert(testTarget->GetName());
                                    }
                                };

                                const auto& sources = target->GetSources();
                                addSourcesToCoverage(sources.m_staticSources);
                                for (const auto& autogenPair : sources.m_autogenSources)
                                {
                                    addSourcesToCoverage(autogenPair.m_outputs);
                                }
                            });
                    }
                }
                else
                {
                    // Line coverage isn't supported yet so use the source coverage for both line and source coverage
                    for (const auto& coveredSource : job.GetCoverge().value().GetSourcesCovered())
                    {
                        coverage[coveredSource.String()].insert(testTarget->GetName());
                    }
                }
            }
        }   

        AZStd::vector<SourceCoveringTests> sourceCoveringTests;
        sourceCoveringTests.reserve(coverage.size());
        for (auto&& [source, testTargets] : coverage)
        {
            // Check to see whether this source is inside the repo or not (not a perfect check but weeds out the obvious non-repo sources)
            if (const auto sourcePath = RepoPath(source); sourcePath.IsRelativeTo(repoRoot))
            {
                sourceCoveringTests.push_back(
                    SourceCoveringTests(RepoPath(sourcePath.LexicallyRelative(repoRoot)), AZStd::move(testTargets)));
            }
            else
            {
                AZ_Warning(LogCallSite, false, "Ignoring source, source it outside of repo: '%s'", sourcePath.c_str());
            }
        }

        return SourceCoveringTestsList(AZStd::move(sourceCoveringTests));
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

    //! Transforms the test engine run result and test engine jobs into a client test run report.
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
