/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactTestSequenceBus.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactClientSequenceReport.h>
#include <TestImpactFramework/TestImpactConfiguration.h>
#include <TestImpactFramework/TestImpactRuntimeException.h>

#include <Artifact/TestImpactArtifactException.h>
#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactSourceCoveringTestsList.h>
#include <Process/Scheduler/TestImpactProcessSchedulerBus.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumeration.h>
#include <TestEngine/Common/Run/TestImpactTestEngineInstrumentedRun.h>
#include <TestEngine/Common/TestImpactTestEngine.h>
#include <TestImpactTestTargetExclusionList.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    static inline constexpr const char* const LogCallSite = "TestImpact";

    //! Attempts to read all of the target descriptor files from the specified configuration directories.
    AZStd::vector<TargetDescriptor> ReadTargetDescriptorFiles(const BuildTargetDescriptorConfig& buildTargetDescriptorConfig);

    //! Handler for test run complete events.
    template<typename TestTarget>
    class TestEngineNotificationHandler
        : private ProcessSchedulerNotificationBus::Handler
        , private TestEngineNotificationBus<TestTarget>::Handler
    {
    public:
        TestEngineNotificationHandler(size_t totalTests);
        ~TestEngineNotificationHandler();

    private:
        // TestEngineNotificationBus override ...
        void OnJobComplete(const TestEngineJob<TestTarget>& testJob) override;
        void OnRealtimeStdContent(
            [[maybe_unused]] ProcessId processId,
            [[maybe_unused]] const AZStd::string& stdOutput,
            [[maybe_unused]] const AZStd::string& stdError,
            const AZStd::string& stdOutputDelta,
            const AZStd::string& stdErrorDelta) override;

        const size_t m_totalTests; //!< The total number of tests to run for the entire sequence.
        size_t m_numTestsCompleted = 0; //!< The running total of tests that have completed.
    };

    template<typename TestTarget>
    TestEngineNotificationHandler<TestTarget>::TestEngineNotificationHandler(size_t totalTests)
        : m_totalTests(totalTests)
    {
        ProcessSchedulerNotificationBus::Handler::BusConnect();
        TestEngineNotificationBus<TestTarget>::Handler::BusConnect();
    }

    template<typename TestTarget>
    TestEngineNotificationHandler<TestTarget>::~TestEngineNotificationHandler()
    {
        TestEngineNotificationBus<TestTarget>::Handler::BusDisconnect();
        ProcessSchedulerNotificationBus::Handler::BusDisconnect();
    }

    template<typename TestTarget>
    void TestEngineNotificationHandler<TestTarget>::OnJobComplete(const TestEngineJob<TestTarget>& testJob)
    {
        Client::TestRunBase testRun(
            testJob.GetTestTarget()->GetNamespace(),
            testJob.GetTestTarget()->GetName(),
            testJob.GetCommandString(),
            testJob.GetStdOutput(),
            testJob.GetStdError(),
            testJob.GetStartTime(),
            testJob.GetDuration(),
            testJob.GetTestResult());

        TestSequenceNotificationsBaseBus::Broadcast(
            &TestSequenceNotificationsBaseBus::Events::OnTestRunComplete, testRun, ++m_numTestsCompleted, m_totalTests);
    }

    template<typename TestTarget>
    void TestEngineNotificationHandler<TestTarget>::OnRealtimeStdContent(
        [[maybe_unused]] ProcessId processId,
        [[maybe_unused]] const AZStd::string& stdOutput,
        [[maybe_unused]] const AZStd::string& stdError,
        const AZStd::string& stdOutputDelta,
        const AZStd::string& stdErrorDelta)
    {
        // Job info about in-flight processes isn't available until completion so realtime std content will be
        // anomynous for the client
        TestSequenceNotificationsBaseBus::Broadcast(
            &TestSequenceNotificationsBaseBus::Events::OnRealtimeStdContent, stdOutputDelta, stdErrorDelta);
    }

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
        }
        catch (const RuntimeException& e)
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
        AZStd::chrono::steady_clock::time_point startTime,
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
                AZStd::chrono::steady_clock::time_point() +
                AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(testJob.GetStartTime() - startTime);

            Client::TestRunBase clientTestRun(
                testJob.GetTestTarget()->GetNamespace(),
                testJob.GetTestTarget()->GetName(),
                testJob.GetCommandString(),
                testJob.GetStdOutput(),
                testJob.GetStdError(),
                relativeStartTime,
                testJob.GetDuration(),
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

    //! Selects the test targets from the specified list of test targets that are not in the specified test target exclusion list.
    //! @param testTargetExcludeList The test target exclusion list to lookup.
    //! @param testTargets The list of test targets to select from.
    //! @returns The subset of test targets in the specified list that are not on the target exclude list.
    template<typename TestTarget>
    AZStd::pair<AZStd::vector<const TestTarget*>, AZStd::vector<const TestTarget*>> SelectTestTargetsByExcludeList(
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

    //! Utility for concatenating two vectors.
    template<typename T>
    static AZStd::vector<T> ConcatenateVectors(const AZStd::vector<T>& v1, const AZStd::vector<T>& v2)
    {
        AZStd::vector<T> result;
        result.reserve(v1.size() + v2.size());
        result.insert(result.end(), v1.begin(), v1.end());
        result.insert(result.end(), v2.begin(), v2.end());
        return result;
    }

    //! Utility structure for holding the pertinent data for test run reports.
    template<typename TestJob>
    struct TestRunData
    {
        TestSequenceResult m_result = TestSequenceResult::Success;
        AZStd::vector<TestJob> m_jobs;
        AZStd::chrono::steady_clock::time_point m_relativeStartTime;
        AZStd::chrono::milliseconds m_duration = AZStd::chrono::milliseconds{ 0 };
    };

    //! Wrapper for the impact analysis test sequence to handle both the updating and non-updating policies through a common pathway.
    //! @tparam TestRunnerFunctor The functor for running the specified tests.
    //! @tparam TestJob The test engine job type returned by the functor.
    //! @param maxConcurrency The maximum concurrency being used for this sequence.
    //! @param policyState The policy state being used for the sequence.
    //! @param suiteSet The suites type used for this sequence.
    //! @param suiteLabelExcludeSet Any tests with suites that match a label from this set will be excluded.
    //! @param timer The timer to use for the test run timings.
    //! @param testRunner The test runner functor to use for each of the test runs.
    //! @param includedSelectedTestTargets The subset of test targets that were selected to run and not also fully excluded from running.
    //! @param excludedSelectedTestTargets The subset of test targets that were selected to run but were fully excluded running.
    //! @param discardedTestTargets The subset of test targets that were discarded from the test selection and will not be run.
    //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
    //! @param updateCoverage The function to call to update the dynamic dependency map with test coverage (if any).
    template<typename TestTarget, typename TestRunnerFunctor, typename TestJob>
    Client::ImpactAnalysisSequenceReport ImpactAnalysisTestSequenceWrapper(
        size_t maxConcurrency,
        const ImpactAnalysisSequencePolicyState& policyState,
        const SuiteSet& suiteSet,
        const SuiteLabelExcludeSet& suiteLabelExcludeSet,
        const Timer& sequenceTimer,
        const TestRunnerFunctor& testRunner,
        const AZStd::vector<const TestTarget*>& includedSelectedTestTargets,
        const AZStd::vector<const TestTarget*>& excludedSelectedTestTargets,
        const AZStd::vector<const TestTarget*>& discardedTestTargets,
        const AZStd::vector<const TestTarget*>& draftedTestTargets,
        const AZStd::optional<AZStd::chrono::milliseconds>& testTargetTimeout,
        const AZStd::optional<AZStd::chrono::milliseconds>& globalTimeout,
        AZStd::optional<AZStd::function<void(const AZStd::vector<TestJob>& jobs)>> updateCoverage)
    {
        TestRunData<TestJob> selectedTestRunData, draftedTestRunData;
        AZStd::optional<AZStd::chrono::milliseconds> sequenceTimeout = globalTimeout;

        // Extract the client facing representation of selected, discarded and drafted test targets
        const Client::TestRunSelection selectedTests(
            ExtractTestTargetNames(includedSelectedTestTargets), ExtractTestTargetNames(excludedSelectedTestTargets));
        const auto discardedTests = ExtractTestTargetNames(discardedTestTargets);
        const auto draftedTests = ExtractTestTargetNames(draftedTestTargets);

        // Inform the client that the sequence is about to start
        ImpactAnalysisTestSequenceNotificationBus::Broadcast(
            &ImpactAnalysisTestSequenceNotificationBus::Events::OnTestSequenceStart,
            suiteSet,
            suiteLabelExcludeSet,
            selectedTests,
            discardedTests,
            draftedTests);

        // We share the test run complete handler between the selected and drafted test runs as to present them together as one
        // continuous test sequence to the client rather than two discrete test runs
        const size_t totalNumTestRuns = includedSelectedTestTargets.size() + draftedTestTargets.size();

        const auto gatherTestRunData = [&](const AZStd::vector<const TestTarget*>& testsTargets, TestRunData<TestJob>& testRunData)
        {
            const Timer testRunTimer;
            testRunData.m_relativeStartTime = testRunTimer.GetStartTimePointRelative(sequenceTimer);
            auto [result, jobs] = testRunner(testsTargets, globalTimeout);
            testRunData.m_result = result;
            testRunData.m_jobs = AZStd::move(jobs);
            testRunData.m_duration = testRunTimer.GetElapsedMs();
        };

        TestEngineNotificationHandler<TestTarget> testRunCompleteHandler(totalNumTestRuns);
        if (!includedSelectedTestTargets.empty())
        {
            // Run the selected test targets and collect the test run results
            gatherTestRunData(includedSelectedTestTargets, selectedTestRunData);

            // Carry the remaining global sequence time over to the drafted test run
            if (globalTimeout.has_value())
            {
                const auto elapsed = selectedTestRunData.m_duration;
                sequenceTimeout = elapsed < globalTimeout.value() ? globalTimeout.value() - elapsed : AZStd::chrono::milliseconds(0);
            }
        }

        if (!draftedTestTargets.empty())
        {
            // Run the drafted test targets and collect the test run results
            gatherTestRunData(draftedTestTargets, draftedTestRunData);
        }

        // Generate the sequence report for the client
        const auto sequenceReport = Client::ImpactAnalysisSequenceReport(
            maxConcurrency,
            testTargetTimeout,
            globalTimeout,
            policyState,
            suiteSet,
            suiteLabelExcludeSet,
            selectedTests,
            discardedTests,
            draftedTests,
            GenerateTestRunReport(
                selectedTestRunData.m_result,
                selectedTestRunData.m_relativeStartTime,
                selectedTestRunData.m_duration,
                selectedTestRunData.m_jobs),
            GenerateTestRunReport(
                draftedTestRunData.m_result,
                draftedTestRunData.m_relativeStartTime,
                draftedTestRunData.m_duration,
                draftedTestRunData.m_jobs));

        // Inform the client that the sequence has ended
        ImpactAnalysisTestSequenceNotificationBus::Broadcast(
            &ImpactAnalysisTestSequenceNotificationBus::Events::OnTestSequenceComplete, sequenceReport);

        // Update the dynamic dependency map with the latest coverage data (if any)
        if (updateCoverage.has_value())
        {
            (*updateCoverage)(ConcatenateVectors(selectedTestRunData.m_jobs, draftedTestRunData.m_jobs));
        }

        return sequenceReport;
    }
} // namespace TestImpact
