/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactFileUtils.h>
#include <TestImpactFramework/TestImpactRuntime.h>
#include <TestImpactFramework/TestImpactRuntimeException.h>

#include <TestImpactRuntimeUtils.h>
#include <Dependency/TestImpactDependencyException.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactSourceCoveringTestsSerializer.h>
#include <Dependency/TestImpactTestSelectorAndPrioritizer.h>
#include <TestEngine/TestImpactTestEngine.h>

#include <AzCore/IO/SystemFile.h>

namespace TestImpact
{
    namespace
    {
        static const char* const LogCallSite = "TestImpact";

        //! Simple helper class for tracking basic timing information.
        class Timer
        {
        public:
            Timer()
                : m_startTime(AZStd::chrono::high_resolution_clock::now())
            {
            }

            //! Returns the time point that the timer was instantiated.
            AZStd::chrono::high_resolution_clock::time_point GetStartTimePoint() const
            {
                return m_startTime;
            }

            //! Returns the time point that the timer was instantiated relative to the specified starting time point.
            AZStd::chrono::high_resolution_clock::time_point GetStartTimePointRelative(const Timer& start) const
            {
                return AZStd::chrono::high_resolution_clock::time_point() +
                    AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(m_startTime - start.GetStartTimePoint());
            }

            //! Returns the time elapsed (in milliseconds) since the timer was instantiated.
            AZStd::chrono::milliseconds GetElapsedMs() const
            {
                const auto endTime = AZStd::chrono::high_resolution_clock::now();
                return AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(endTime - m_startTime);
            }

            //! Returns the current time point relative to the time point the timer was instantiated.
            AZStd::chrono::high_resolution_clock::time_point GetElapsedTimepoint() const
            {
                const auto endTime = AZStd::chrono::high_resolution_clock::now();
                return m_startTime + AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(endTime - m_startTime);
            }

        private:
            AZStd::chrono::high_resolution_clock::time_point m_startTime;
        };

        //! Handler for test run complete events.
        class TestRunCompleteCallbackHandler
        {
        public:
            TestRunCompleteCallbackHandler(
                size_t totalTests,
                AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
                : m_totalTests(totalTests)
                , m_testCompleteCallback(testCompleteCallback)
            {
            }

            void operator()(const TestEngineJob& testJob)
            {
                if (m_testCompleteCallback.has_value())
                {
                    Client::TestRun testRun(
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

    Runtime::Runtime(
        RuntimeConfig&& config,
        SuiteType suiteFilter,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::FailedTestCoverage failedTestCoveragePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::IntegrityFailure integrationFailurePolicy,
        Policy::TestSharding testShardingPolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<size_t> maxConcurrency)
        : m_config(AZStd::move(config))
        , m_suiteFilter(suiteFilter)
        , m_executionFailurePolicy(executionFailurePolicy)
        , m_failedTestCoveragePolicy(failedTestCoveragePolicy)
        , m_testFailurePolicy(testFailurePolicy)
        , m_integrationFailurePolicy(integrationFailurePolicy)
        , m_testShardingPolicy(testShardingPolicy)
        , m_targetOutputCapture(targetOutputCapture)
        , m_maxConcurrency(maxConcurrency.value_or(AZStd::thread::hardware_concurrency()))
    {
        // Construct the dynamic dependency map from the build target descriptors
        m_dynamicDependencyMap = ConstructDynamicDependencyMap(suiteFilter, m_config.m_buildTargetDescriptor, m_config.m_testTargetMeta);

        // Construct the test selector and prioritizer from the dependency graph data (NOTE: currently not implemented)
        m_testSelectorAndPrioritizer = AZStd::make_unique<TestSelectorAndPrioritizer>(m_dynamicDependencyMap.get(), DependencyGraphDataMap{});

        // Construct the target exclude list from the target configuration data
        m_testTargetExcludeList = ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetTestTargetList(), m_config.m_target.m_excludedTestTargets);

        // Construct the test engine with the workspace path and launcher binaries
        m_testEngine = AZStd::make_unique<TestEngine>(
            m_config.m_repo.m_root,
            m_config.m_target.m_outputDirectory,
            m_config.m_workspace.m_active.m_enumerationCacheDirectory,
            m_config.m_workspace.m_temp.m_artifactDirectory,
            m_config.m_testEngine.m_testRunner.m_binary,
            m_config.m_testEngine.m_instrumentation.m_binary,
            m_maxConcurrency);

        try
        {
            // Populate the dynamic dependency map with the existing source coverage data (if any)
            m_sparTIAFile = m_config.m_workspace.m_active.m_sparTIAFiles[static_cast<size_t>(m_suiteFilter)].String();
            const auto tiaDataRaw = ReadFileContents<Exception>(m_sparTIAFile);
            const auto tiaData = DeserializeSourceCoveringTestsList(tiaDataRaw);
            if (tiaData.GetNumSources())
            {
                m_dynamicDependencyMap->ReplaceSourceCoverage(tiaData);
                m_hasImpactAnalysisData = true;

                // Enumerate new test targets
                const auto testTargetsWithNoEnumeration = m_dynamicDependencyMap->GetNotCoveringTests();
                if (!testTargetsWithNoEnumeration.empty())
                {
                    m_testEngine->UpdateEnumerationCache(
                        testTargetsWithNoEnumeration,
                        Policy::ExecutionFailure::Ignore,
                        Policy::TestFailure::Continue,
                        AZStd::nullopt,
                        AZStd::nullopt,
                        AZStd::nullopt);
                }
            }
        }
        catch (const DependencyException& e)
        {
            if (integrationFailurePolicy == Policy::IntegrityFailure::Abort)
            {
                throw RuntimeException(e.what());
            }
        }
        catch ([[maybe_unused]]const Exception& e)
        {
            AZ_Printf(
                LogCallSite,
                AZStd::string::format(
                    "No test impact analysis data found for suite '%s' at %s\n", GetSuiteTypeName(m_suiteFilter).c_str(), m_sparTIAFile.c_str()).c_str());
        }
    }

    Runtime::~Runtime() = default;

    void Runtime::EnumerateMutatedTestTargets(const ChangeDependencyList& changeDependencyList)
    {
        AZStd::vector<const TestTarget*> testTargets;
        const auto addMutatedTestTargetsToEnumerationList = [this, &testTargets](const AZStd::vector<SourceDependency>& sourceDependencies)
        {
            for (const auto& sourceDependency : sourceDependencies)
            {
                for (const auto& parentTarget : sourceDependency.GetParentTargets())
                {
                    AZStd::visit([&testTargets]([[maybe_unused]] auto&& target)
                    {
                        if constexpr (IsTestTarget<decltype(target)>)
                        {
                            testTargets.push_back(target);
                        }
                    }, parentTarget.GetTarget());
                }
            }
        };

        // Gather all of the test targets that have had any of their sources modified
        addMutatedTestTargetsToEnumerationList(changeDependencyList.GetCreateSourceDependencies());
        addMutatedTestTargetsToEnumerationList(changeDependencyList.GetUpdateSourceDependencies());
        addMutatedTestTargetsToEnumerationList(changeDependencyList.GetDeleteSourceDependencies());

        // Enumerate the mutated test targets to ensure their enumeration caches are up to date
        if (!testTargets.empty())
        {
            m_testEngine->UpdateEnumerationCache(
                testTargets,
                Policy::ExecutionFailure::Ignore,
                Policy::TestFailure::Continue,
                AZStd::nullopt,
                AZStd::nullopt,
                AZStd::nullopt);
        }
    }

    AZStd::pair<AZStd::vector<const TestTarget*>, AZStd::vector<const TestTarget*>> Runtime::SelectCoveringTestTargetsAndUpdateEnumerationCache(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy)
    {
        AZStd::vector<const TestTarget*> discardedTestTargets;

        // Select and prioritize the test targets pertinent to this change list
        const auto changeDependencyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, m_integrationFailurePolicy);
        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependencyList, testPrioritizationPolicy);

        // Populate a set with the selected test targets so that we can infer the discarded test target not selected for this change list
        const AZStd::unordered_set<const TestTarget*> selectedTestTargetSet(selectedTestTargets.begin(), selectedTestTargets.end());

        // Update the enumeration caches of mutated targets regardless of the current sharding policy
        EnumerateMutatedTestTargets(changeDependencyList);

        // The test targets in the main list not in the selected test target set are the test targets not selected for this change list
        for (const auto& testTarget : m_dynamicDependencyMap->GetTestTargetList().GetTargets())
        {
            if (!selectedTestTargetSet.contains(&testTarget))
            {
                discardedTestTargets.push_back(&testTarget);
            }
        }

        return { selectedTestTargets, discardedTestTargets };
    }

    AZStd::pair<AZStd::vector<const TestTarget*>, AZStd::vector<const TestTarget*>> Runtime::SelectTestTargetsByExcludeList(
        AZStd::vector<const TestTarget*> testTargets) const
    {
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;

        if (m_testTargetExcludeList.empty())
        {
            return { testTargets, {} };
        }

        for (const auto& testTarget : testTargets)
        {
            if (!m_testTargetExcludeList.contains(testTarget))
            {
                includedTestTargets.push_back(testTarget);
            }
            else
            {
                excludedTestTargets.push_back(testTarget);
            }
        }

        return { includedTestTargets, excludedTestTargets };
    }

    void Runtime::ClearDynamicDependencyMapAndRemoveExistingFile()
    {
        m_dynamicDependencyMap->ClearAllSourceCoverage();
        DeleteFile(m_sparTIAFile);
    }

    SourceCoveringTestsList Runtime::CreateSourceCoveringTestFromTestCoverages(const AZStd::vector<TestEngineInstrumentedRun>& jobs)
    {
        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZStd::string>> coverage;
        for (const auto& job : jobs)
        {
            // First we must remove any existing coverage for the test target so as to not end up with source remnants from previous
            // coverage that is no longer covered by this revision of the test target
            m_dynamicDependencyMap->RemoveTestTargetFromSourceCoverage(job.GetTestTarget());

            // Next we will update the coverage of test targets that completed (with or without failures), unless the failed test coverage
            // policy dictates we should instead discard the coverage of test targets with failing tests
            const auto testResult = job.GetTestResult();

            if (m_failedTestCoveragePolicy == Policy::FailedTestCoverage::Discard && testResult == Client::TestRunResult::TestFailures)
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
                        job.GetTestCoverge().has_value(),
                        RuntimeException,
                        AZStd::string::format(
                            "Test target '%s' completed its test run successfully but produced no coverage data. Command string: '%s'",
                            job.GetTestTarget()->GetName().c_str(), job.GetCommandString().c_str()));
                }

                if (!job.GetTestCoverge().has_value())
                {
                    // When a test run completes with failing tests but produces no coverage artifact that's typically a sign of the
                    // test aborting due to an unhandled exception, in which case ignore it and let it be picked up in the failure report
                    continue;
                }

                // Add the sources covered by this test target to the coverage map
                for (const auto& source : job.GetTestCoverge().value().GetSourcesCovered())
                {
                    coverage[source.String()].insert(job.GetTestTarget()->GetName());
                }
            }
        }

        AZStd::vector<SourceCoveringTests> sourceCoveringTests;
        sourceCoveringTests.reserve(coverage.size());
        for (auto&& [source, testTargets] : coverage)
        {
            // Check to see whether this source is inside the repo or not (not a perfect check but weeds out the obvious non-repo sources)
            if (const auto sourcePath = RepoPath(source);
                sourcePath.IsRelativeTo(m_config.m_repo.m_root))
            {
                sourceCoveringTests.push_back(
                    SourceCoveringTests(RepoPath(sourcePath.LexicallyRelative(m_config.m_repo.m_root)), AZStd::move(testTargets)));
            }
            else
            {
                AZ_Warning(LogCallSite, false, "Ignoring source, source it outside of repo: '%s'", sourcePath.c_str());
            }
        }

        return SourceCoveringTestsList(AZStd::move(sourceCoveringTests));
    }

    void Runtime::UpdateAndSerializeDynamicDependencyMap(const AZStd::vector<TestEngineInstrumentedRun>& jobs)
    {
        try
        {
            const auto sourceCoverageTestsList = CreateSourceCoveringTestFromTestCoverages(jobs);
            if (sourceCoverageTestsList.GetNumSources() == 0)
            {
                return;
            }

            m_dynamicDependencyMap->ReplaceSourceCoverage(sourceCoverageTestsList);
            const auto sparTIA = m_dynamicDependencyMap->ExportSourceCoverage();
            const auto sparTIAData = SerializeSourceCoveringTestsList(sparTIA);
            WriteFileContents<RuntimeException>(sparTIAData, m_sparTIAFile);
            m_hasImpactAnalysisData = true;
        }
        catch(const RuntimeException& e)
        {
            if (m_integrationFailurePolicy == Policy::IntegrityFailure::Abort)
            {
                throw e;
            }
            else
            {
                AZ_Error(LogCallSite, false, e.what());
            }
        }
    }

    Client::SequenceReport Runtime::RegularTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::SequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        const Timer sequenceTimer;
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;
        
        // Separate the test targets into those that are excluded by either the test filter or exclusion list and those that are not
        for (const auto& testTarget : m_dynamicDependencyMap->GetTestTargetList().GetTargets())
        {
            if (!m_testTargetExcludeList.contains(&testTarget))
            {
                includedTestTargets.push_back(&testTarget);
            }
            else
            {
                // Test targets on the exclude list are excluded
                excludedTestTargets.push_back(&testTarget);
            }
        }

        // Extract the client facing representation of selected test targets
        const Client::TestRunSelection selectedTests(ExtractTestTargetNames(includedTestTargets), ExtractTestTargetNames(excludedTestTargets));

        // Inform the client that the sequence is about to start
        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(m_suiteFilter, selectedTests);
        }

        // Run the test targets and collect the test run results
        const Timer testRunTimer;
        const auto [result, testJobs] = m_testEngine->RegularRun(
            includedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler(includedTestTargets.size(), testCompleteCallback));
        const auto testRunDuration = testRunTimer.GetElapsedMs();

        // Generate the sequence report for the client
        const auto sequenceReport = Client::SequenceReport(
            m_suiteFilter,
            selectedTests,
            GenerateTestRunReport(result, testRunTimer.GetStartTimePointRelative(sequenceTimer), testRunDuration, testJobs));

        // Inform the client that the sequence has ended
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(sequenceReport);
        }

        return sequenceReport;
    }

    //! Wrapper for the impact analysis test sequence to handle both the updating and non-updating policies through a common pathway.
    //! @tparam TestRunnerFunctor The functor for running the specified tests.
    //! @tparam TestJob The test engine job type returned by the functor.
    //! @param suiteType The suite type used for this sequence.
    //! @param timer The timer to use for the test run timings.
    //! @param testRunner The test runner functor to use for each of the test runs.
    //! @param includedSelectedTestTargets The subset of test targets that were selected to run and not also fully excluded from running.
    //! @param excludedSelectedTestTargets The subset of test targets that were selected to run but were fully excluded running.
    //! @param discardedTestTargets The subset of test targets that were discarded from the test selection and will not be run.
    //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
    //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the tests.
    //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
    //! @param testRunCompleteCallback The client function to be called after an individual test run has completed.
    //! @param updateCoverage The function to call to update the dynamic dependency map with test coverage (if any).
    template<typename TestRunnerFunctor, typename TestJob>
    Client::ImpactAnalysisSequenceReport ImpactAnalysisTestSequenceWrapper(
        SuiteType suiteType,
        const Timer& sequenceTimer,
        const TestRunnerFunctor& testRunner,
        const AZStd::vector<const TestTarget*>& includedSelectedTestTargets,
        const AZStd::vector<const TestTarget*>& excludedSelectedTestTargets,
        const AZStd::vector<const TestTarget*>& discardedTestTargets,
        const AZStd::vector<const TestTarget*>& draftedTestTargets,
        const AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::ImpactAnalysisSequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback,
        AZStd::optional<AZStd::function<void(const AZStd::vector<TestJob>& jobs)>> updateCoverage)
    {
        AZStd::optional<AZStd::chrono::milliseconds> sequenceTimeout = globalTimeout;

        // Extract the client facing representation of selected, discarded and drafted test targets
        const Client::TestRunSelection selectedTests(
            ExtractTestTargetNames(includedSelectedTestTargets), ExtractTestTargetNames(excludedSelectedTestTargets));
        const auto discardedTests = ExtractTestTargetNames(discardedTestTargets);
        const auto draftedTests = ExtractTestTargetNames(draftedTestTargets);

        // Inform the client that the sequence is about to start
        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(suiteType, selectedTests, discardedTests, draftedTests);
        }

        // We share the test run complete handler between the selected and drafted test runs as to present them together as one
        // continuous test sequence to the client rather than two discrete test runs
        const size_t totalNumTestRuns = includedSelectedTestTargets.size() + draftedTestTargets.size();
        TestRunCompleteCallbackHandler testRunCompleteHandler(totalNumTestRuns, testCompleteCallback);

        // Run the selected test targets and collect the test run results
        const Timer selectedTestRunTimer;
        const auto [selectedResult, selectedTestJobs] = testRunner(includedSelectedTestTargets, testRunCompleteHandler, globalTimeout);
        const auto selectedTestRunDuration = selectedTestRunTimer.GetElapsedMs();

        // Carry the remaining global sequence time over to the drafted test run
        if (globalTimeout.has_value())
        {
            const auto elapsed = selectedTestRunDuration;
            sequenceTimeout = elapsed < globalTimeout.value() ? globalTimeout.value() - elapsed : AZStd::chrono::milliseconds(0);
        }

        // Run the drafted test targets and collect the test run results
        Timer draftedTestRunTimer;
        const auto [draftedResult, draftedTestJobs] = testRunner(draftedTestTargets, testRunCompleteHandler, globalTimeout);
        const auto draftedTestRunDuration = draftedTestRunTimer.GetElapsedMs();

        // Generate the sequence report for the client
        const auto sequenceReport = Client::ImpactAnalysisSequenceReport(
            suiteType,
            selectedTests,
            discardedTests,
            draftedTests,
            GenerateTestRunReport(selectedResult, selectedTestRunTimer.GetStartTimePointRelative(sequenceTimer), selectedTestRunDuration, selectedTestJobs),
            GenerateTestRunReport(draftedResult, draftedTestRunTimer.GetStartTimePointRelative(sequenceTimer), draftedTestRunDuration, draftedTestJobs));

        // Inform the client that the sequence has ended
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(sequenceReport);
        }

        // Update the dynamic dependency map with the latest coverage data (if any)
        if (updateCoverage.has_value())
        {
            (*updateCoverage)(ConcatenateVectors(selectedTestJobs, draftedTestJobs));
        }

        return sequenceReport;
    }

    Client::ImpactAnalysisSequenceReport Runtime::ImpactAnalysisTestSequence(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy,
        Policy::DynamicDependencyMap dynamicDependencyMapPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::ImpactAnalysisSequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        const Timer sequenceTimer;

        // Draft in the test targets that have no coverage entries in the dynamic dependency map
        AZStd::vector<const TestTarget*> draftedTestTargets = m_dynamicDependencyMap->GetNotCoveringTests();

        // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargetsAndUpdateEnumerationCache(changeList, testPrioritizationPolicy);

        // The subset of selected test targets that are not on the configuration's exclude list and those that are
        auto [includedSelectedTestTargets, excludedSelectedTestTargets] = SelectTestTargetsByExcludeList(selectedTestTargets);

        // Functor for running instrumented test targets
        const auto instrumentedTestRun =
            [this, &testTargetTimeout](
                const AZStd::vector<const TestTarget*>& testsTargets,
                TestRunCompleteCallbackHandler& testRunCompleteHandler,
                AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
        {
            return m_testEngine->InstrumentedRun(
                testsTargets,
                m_testShardingPolicy,
                m_executionFailurePolicy,
                m_integrationFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                AZStd::ref(testRunCompleteHandler));
        };

        // Functor for running uninstrumented test targets
        const auto regularTestRun =
            [this, &testTargetTimeout](
                const AZStd::vector<const TestTarget*>& testsTargets,
                 TestRunCompleteCallbackHandler& testRunCompleteHandler,
                AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
        {
            return m_testEngine->RegularRun(
                testsTargets,
                m_testShardingPolicy,
                m_executionFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                AZStd::ref(testRunCompleteHandler));
        };

        if (dynamicDependencyMapPolicy == Policy::DynamicDependencyMap::Update)
        {
            AZStd::optional<AZStd::function<void(const AZStd::vector<TestEngineInstrumentedRun>& jobs)>> updateCoverage =
                [this](const AZStd::vector<TestEngineInstrumentedRun>& jobs)
            {
                UpdateAndSerializeDynamicDependencyMap(jobs);
            };

            return ImpactAnalysisTestSequenceWrapper(
                m_suiteFilter,
                sequenceTimer,
                instrumentedTestRun,
                includedSelectedTestTargets,
                excludedSelectedTestTargets,
                discardedTestTargets,
                draftedTestTargets,
                globalTimeout,
                testSequenceStartCallback,
                testSequenceEndCallback,
                testCompleteCallback,
                updateCoverage);
        }
        else
        {
            return ImpactAnalysisTestSequenceWrapper(
                m_suiteFilter,
                sequenceTimer,
                regularTestRun,
                includedSelectedTestTargets,
                excludedSelectedTestTargets,
                discardedTestTargets,
                draftedTestTargets,
                globalTimeout,
                testSequenceStartCallback,
                testSequenceEndCallback,
                testCompleteCallback,
                AZStd::optional<AZStd::function<void(const AZStd::vector<TestEngineRegularRun>& jobs)>>{ AZStd::nullopt });
        }
    }

    Client::SafeImpactAnalysisSequenceReport Runtime::SafeImpactAnalysisTestSequence(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::SafeImpactAnalysisSequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        const Timer sequenceTimer;
        auto sequenceTimeout = globalTimeout;

        // Draft in the test targets that have no coverage entries in the dynamic dependency map
        AZStd::vector<const TestTarget*> draftedTestTargets = m_dynamicDependencyMap->GetNotCoveringTests();

        // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargetsAndUpdateEnumerationCache(changeList, testPrioritizationPolicy);

        // The subset of selected test targets that are not on the configuration's exclude list and those that are
        auto [includedSelectedTestTargets, excludedSelectedTestTargets] = SelectTestTargetsByExcludeList(selectedTestTargets);

        // The subset of discarded test targets that are not on the configuration's exclude list and those that are
        auto [includedDiscardedTestTargets, excludedDiscardedTestTargets] = SelectTestTargetsByExcludeList(discardedTestTargets);

        // Extract the client facing representation of selected, discarded and drafted test targets
        const Client::TestRunSelection selectedTests(
            ExtractTestTargetNames(includedSelectedTestTargets), ExtractTestTargetNames(excludedSelectedTestTargets));
        const Client::TestRunSelection discardedTests(ExtractTestTargetNames(includedDiscardedTestTargets), ExtractTestTargetNames(excludedDiscardedTestTargets));
            const auto draftedTests = ExtractTestTargetNames(draftedTestTargets);

        // Inform the client that the sequence is about to start
        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(m_suiteFilter, selectedTests, discardedTests, draftedTests);
        }

        // We share the test run complete handler between the selected, discarded and drafted test runs as to present them together as one
        // continuous test sequence to the client rather than three discrete test runs
        const size_t totalNumTestRuns = includedSelectedTestTargets.size() + draftedTestTargets.size() + includedDiscardedTestTargets.size();
        TestRunCompleteCallbackHandler testRunCompleteHandler(totalNumTestRuns, testCompleteCallback);

        // Run the selected test targets and collect the test run results
        const Timer selectedTestRunTimer;
        const auto [selectedResult, selectedTestJobs] = m_testEngine->InstrumentedRun(
            includedSelectedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            m_integrationFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            sequenceTimeout,
            AZStd::ref(testRunCompleteHandler));
        const auto selectedTestRunDuration = selectedTestRunTimer.GetElapsedMs();

        // Carry the remaining global sequence time over to the discarded test run
        if (globalTimeout.has_value())
        {
            const auto elapsed = selectedTestRunDuration;
            sequenceTimeout = elapsed < globalTimeout.value() ? globalTimeout.value() - elapsed : AZStd::chrono::milliseconds(0);
        }

        // Run the discarded test targets and collect the test run results
        const Timer discardedTestRunTimer;
        const auto [discardedResult, discardedTestJobs] = m_testEngine->RegularRun(
            includedDiscardedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            sequenceTimeout,
            AZStd::ref(testRunCompleteHandler));
        const auto discardedTestRunDuration = discardedTestRunTimer.GetElapsedMs();

        // Carry the remaining global sequence time over to the drafted test run
        if (globalTimeout.has_value())
        {
            const auto elapsed = selectedTestRunDuration + discardedTestRunDuration;
            sequenceTimeout = elapsed < globalTimeout.value() ? globalTimeout.value() - elapsed : AZStd::chrono::milliseconds(0);
        }

        // Run the drafted test targets and collect the test run results
        const Timer draftedTestRunTimer;
        const auto [draftedResult, draftedTestJobs] = m_testEngine->InstrumentedRun(
            draftedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            m_integrationFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            sequenceTimeout,
            AZStd::ref(testRunCompleteHandler));
        const auto draftedTestRunDuration = draftedTestRunTimer.GetElapsedMs();

        // Generate the sequence report for the client
        const auto sequenceReport = Client::SafeImpactAnalysisSequenceReport(
            m_suiteFilter,
            selectedTests,
            discardedTests,
            draftedTests,
            GenerateTestRunReport(selectedResult, selectedTestRunTimer.GetStartTimePointRelative(sequenceTimer), selectedTestRunDuration, selectedTestJobs),
            GenerateTestRunReport(discardedResult, discardedTestRunTimer.GetStartTimePointRelative(sequenceTimer), discardedTestRunDuration, discardedTestJobs),
            GenerateTestRunReport(draftedResult, draftedTestRunTimer.GetStartTimePointRelative(sequenceTimer), draftedTestRunDuration, draftedTestJobs));

        // Inform the client that the sequence has ended
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(sequenceReport);
        }

        UpdateAndSerializeDynamicDependencyMap(ConcatenateVectors(selectedTestJobs, draftedTestJobs));
        return sequenceReport;
    }

    Client::SequenceReport Runtime::SeededTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::SequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        const Timer sequenceTimer;
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;

        // Separate the test targets into those that are excluded by either the test filter or exclusion list and those that are not
        for (const auto& testTarget : m_dynamicDependencyMap->GetTestTargetList().GetTargets())
        {
            if (!m_testTargetExcludeList.contains(&testTarget))
            {
                includedTestTargets.push_back(&testTarget);
            }
            else
            {
                excludedTestTargets.push_back(&testTarget);
            }
        }

        // Extract the client facing representation of selected test targets
        Client::TestRunSelection selectedTests(ExtractTestTargetNames(includedTestTargets), ExtractTestTargetNames(excludedTestTargets));

        // Inform the client that the sequence is about to start
        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(m_suiteFilter, selectedTests);
        }

        // Run the test targets and collect the test run results
        const Timer testRunTimer;
        const auto [result, testJobs] = m_testEngine->InstrumentedRun(
            includedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            m_integrationFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler(includedTestTargets.size(), testCompleteCallback));
        const auto testRunDuration = testRunTimer.GetElapsedMs();

        // Generate the sequence report for the client
        const auto sequenceReport = Client::SequenceReport(
            m_suiteFilter,
            selectedTests,
            GenerateTestRunReport(result, testRunTimer.GetStartTimePointRelative(sequenceTimer), testRunDuration, testJobs));

        // Inform the client that the sequence has ended
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(sequenceReport);
        }

        ClearDynamicDependencyMapAndRemoveExistingFile();
        UpdateAndSerializeDynamicDependencyMap(testJobs);
        return sequenceReport;
    }

    bool Runtime::HasImpactAnalysisData() const
    {
        return m_hasImpactAnalysisData;
    }
}
