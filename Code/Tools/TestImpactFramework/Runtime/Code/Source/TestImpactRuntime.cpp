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

            //! Returns the time elapsed (in milliseconds) since the timer was instantiated
            AZStd::chrono::milliseconds Elapsed()
            {
                const auto endTime = AZStd::chrono::high_resolution_clock::now();
                return AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(endTime - m_startTime);
            }

        private:
            AZStd::chrono::high_resolution_clock::time_point m_startTime;
        };

        //! Handler for test run complete events.
        class TestRunCompleteCallbackHandler
        {
        public:
            TestRunCompleteCallbackHandler(AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
                : m_testCompleteCallback(testCompleteCallback)
            {
            }

            void operator()(const TestEngineJob& testJob)
            {
                if (m_testCompleteCallback.has_value())
                {
                    (*m_testCompleteCallback)
                        (Client::TestRun(testJob.GetTestTarget()->GetName(), testJob.GetTestResult(), testJob.GetDuration()));
                }
            }

        private:
            AZStd::optional<TestRunCompleteCallback> m_testCompleteCallback;
        };
    }

    //! Utility for concatenating two vectors.
    template<typename T>
    AZStd::vector<T> ConcatenateVectors(const AZStd::vector<T>& v1, const AZStd::vector<T>& v2)
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

    TestSequenceResult Runtime::RegularTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        Timer timer;
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

        // Sequence start callback
        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(Client::TestRunSelection(ExtractTestTargetNames(includedTestTargets), ExtractTestTargetNames(excludedTestTargets)));
        }

        const auto [result, testJobs] = m_testEngine->RegularRun(
            includedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler(testCompleteCallback));

        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(GenerateSequenceFailureReport(testJobs), timer.Elapsed());
        }

        return result;
    }

    TestSequenceResult Runtime::ImpactAnalysisTestSequence(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy,
        Policy::DynamicDependencyMap dynamicDependencyMapPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        Timer timer;

        // Draft in the test targets that have no coverage entries in the dynamic dependency map
        AZStd::vector<const TestTarget*> draftedTestTargets = m_dynamicDependencyMap->GetNotCoveringTests();

        // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargetsAndUpdateEnumerationCache(changeList, testPrioritizationPolicy);

        // The subset of selected test targets that are not on the configuration's exclude list and those that are
        auto [includedSelectedTestTargets, excludedSelectedTestTargets] = SelectTestTargetsByExcludeList(selectedTestTargets);

        // We present to the client the included selected test targets and the drafted test targets as distinct sets but internally
        // we consider the concatenated set of the two the actual set of tests to run
        AZStd::vector<const TestTarget*> testTargetsToRun = ConcatenateVectors(includedSelectedTestTargets, draftedTestTargets);

        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(
                Client::TestRunSelection(ExtractTestTargetNames(includedSelectedTestTargets), ExtractTestTargetNames(excludedSelectedTestTargets)),
                ExtractTestTargetNames(discardedTestTargets),
                ExtractTestTargetNames(draftedTestTargets));
        }

        if (dynamicDependencyMapPolicy == Policy::DynamicDependencyMap::Update)
        {
            const auto [result, testJobs] = m_testEngine->InstrumentedRun(
                testTargetsToRun,
                m_testShardingPolicy,
                m_executionFailurePolicy,
                Policy::IntegrityFailure::Continue,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                TestRunCompleteCallbackHandler(testCompleteCallback));

            UpdateAndSerializeDynamicDependencyMap(testJobs);

            if (testSequenceEndCallback.has_value())
            {
                (*testSequenceEndCallback)(GenerateSequenceFailureReport(testJobs), timer.Elapsed());
            }

            return result;
        }
        else
        {
            const auto [result, testJobs] = m_testEngine->RegularRun(
                testTargetsToRun,
                m_testShardingPolicy,
                m_executionFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                TestRunCompleteCallbackHandler(testCompleteCallback));

            if (testSequenceEndCallback.has_value())
            {
                (*testSequenceEndCallback)(GenerateSequenceFailureReport(testJobs), timer.Elapsed());
            }

            return result;
        }
    }

    AZStd::pair<TestSequenceResult, TestSequenceResult> Runtime::SafeImpactAnalysisTestSequence(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<SafeTestSequenceCompleteCallback> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        Timer timer;

        // Draft in the test targets that have no coverage entries in the dynamic dependency map
        AZStd::vector<const TestTarget*> draftedTestTargets = m_dynamicDependencyMap->GetNotCoveringTests();

        // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargetsAndUpdateEnumerationCache(changeList, testPrioritizationPolicy);

        // The subset of selected test targets that are not on the configuration's exclude list and those that are
        auto [includedSelectedTestTargets, excludedSelectedTestTargets] = SelectTestTargetsByExcludeList(selectedTestTargets);

        // The subset of discarded test targets that are not on the configuration's exclude list and those that are
        auto [includedDiscardedTestTargets, excludedDiscardedTestTargets] = SelectTestTargetsByExcludeList(discardedTestTargets);

        // We present to the client the included selected test targets and the drafted test targets as distinct sets but internally
        // we consider the concatenated set of the two the actual set of tests to run
        AZStd::vector<const TestTarget*> testTargetsToRun = ConcatenateVectors(includedSelectedTestTargets, draftedTestTargets);

        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(
                Client::TestRunSelection(ExtractTestTargetNames(includedSelectedTestTargets), ExtractTestTargetNames(excludedSelectedTestTargets)),
                Client::TestRunSelection(ExtractTestTargetNames(includedDiscardedTestTargets), ExtractTestTargetNames(excludedDiscardedTestTargets)),
                ExtractTestTargetNames(draftedTestTargets));
        }

        // Impact analysis run of the selected test targets
        const auto [selectedResult, selectedTestJobs] = m_testEngine->InstrumentedRun(
            testTargetsToRun,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            Policy::IntegrityFailure::Continue,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler(testCompleteCallback));
        
        const auto selectedDuraton = timer.Elapsed();

        // Carry the remaining global sequence time over to the discarded test run
        if (globalTimeout.has_value())
        {
            const auto elapsed = timer.Elapsed();
            globalTimeout = elapsed < globalTimeout.value() ? globalTimeout.value() - elapsed : AZStd::chrono::milliseconds(0);
        }

        // Regular run of the discarded test targets
        const auto [discardedResult, discardedTestJobs] = m_testEngine->RegularRun(
            includedDiscardedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler(testCompleteCallback));

        const auto discardedDuraton = timer.Elapsed();

        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(
                GenerateSequenceFailureReport(selectedTestJobs),
                GenerateSequenceFailureReport(discardedTestJobs),
                selectedDuraton,
                discardedDuraton);
        }

        UpdateAndSerializeDynamicDependencyMap(selectedTestJobs);
        return { selectedResult, discardedResult };
    }

    TestSequenceResult Runtime::SeededTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        Timer timer;
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;

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

        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(Client::TestRunSelection(ExtractTestTargetNames(includedTestTargets), ExtractTestTargetNames(excludedTestTargets)));
        }

        const auto [result, testJobs] = m_testEngine->InstrumentedRun(
            includedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            Policy::IntegrityFailure::Continue,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler(testCompleteCallback));

        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(GenerateSequenceFailureReport(testJobs), timer.Elapsed());
        }

        ClearDynamicDependencyMapAndRemoveExistingFile();
        UpdateAndSerializeDynamicDependencyMap(testJobs);

        return result;
    }

    bool Runtime::HasImpactAnalysisData() const
    {
        return m_hasImpactAnalysisData;
    }
}
