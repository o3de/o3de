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

#include <TestImpactFramework/TestImpactUtils.h>
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
        // Simple helper class for tracking basic timing information
        class Timer
        {
        public:
            Timer()
                : m_startTime(AZStd::chrono::high_resolution_clock::now())
            {
            }

            // Returns the time elapsed (in milliseconds) since the timer was instantiated
            AZStd::chrono::milliseconds Elapsed()
            {
                const auto endTime = AZStd::chrono::high_resolution_clock::now();
                return AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(endTime - m_startTime);
            }

        private:
            AZStd::chrono::high_resolution_clock::time_point m_startTime;
        };

        // Handler for test run complete events
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

    Runtime::Runtime(
        RuntimeConfig&& config,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::ExecutionFailureDrafting executionFailureDraftingPolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::IntegrityFailure integrationFailurePolicy,
        Policy::TestSharding testShardingPolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<size_t> maxConcurrency)
        : m_config(AZStd::move(config))
        , m_executionFailurePolicy(executionFailurePolicy)
        , m_executionFailureDraftingPolicy(executionFailureDraftingPolicy)
        , m_testFailurePolicy(testFailurePolicy)
        , m_integrationFailurePolicy(integrationFailurePolicy)
        , m_testShardingPolicy(testShardingPolicy)
        , m_targetOutputCapture(targetOutputCapture)
        , m_maxConcurrency(maxConcurrency.value_or(AZStd::thread::hardware_concurrency()))
    {
        // Construct the dynamic dependency map from the build target descriptors
        m_dynamicDependencyMap = ConstructDynamicDependencyMap(m_config.m_buildTargetDescriptor, m_config.m_testTargetMeta);

        // Construct the test selector and prioritizer from the dependency graph data (NOTE: currently not implemented)
        m_testSelectorAndPrioritizer = AZStd::make_unique<TestSelectorAndPrioritizer>(m_dynamicDependencyMap.get(), DependencyGraphDataMap{});

        // Construct the target exclude list from the target configuration data
        m_testTargetExcludeList = ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetTestTargetList(), m_config.m_target.m_excludedTestTargets);

        // Construct the test engine with the workspace path and launcher binaries
        m_testEngine = AZStd::make_unique<TestEngine>(
            m_config.m_repo.m_root,
            m_config.m_target.m_outputDirectory,
            m_config.m_workspace.m_active.m_relativePaths.m_enumerationCacheDirectory,
            m_config.m_workspace.m_temp.m_relativePaths.m_artifactDirectory,
            m_config.m_testEngine.m_testRunner.m_binary,
            m_config.m_testEngine.m_instrumentation.m_binary,
            m_maxConcurrency);

        try
        {
            // Populate the dynamic dependency map with the existing source coverage data (if any)
            const auto tiaDataRaw = ReadFileContents<Exception>(m_config.m_workspace.m_active.m_relativePaths.m_sparTIAFile);
            const auto tiaData = DeserializeSourceCoveringTestsList(tiaDataRaw);
            if (tiaData.GetNumSources())
            {
                m_dynamicDependencyMap->ReplaceSourceCoverage(tiaData);
                m_hasImpactAnalysisData = true;

                // Enumerate new test targets
                m_testEngine->UpdateEnumerationCache(m_dynamicDependencyMap->GetNotCoveringTests(), Policy::ExecutionFailure::Ignore, AZStd::nullopt, AZStd::nullopt, AZStd::nullopt);
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
            AZ_Printf("No test impact analysis data found at %s", m_config.m_workspace.m_active.m_relativePaths.m_sparTIAFile.c_str());
        }        
    }

    Runtime::~Runtime() = default;

    AZStd::pair<AZStd::vector<const TestTarget*>, AZStd::vector<const TestTarget*>> Runtime::SelectCoveringTestTargetsAndEnumerateMutatedTestTargets(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy)
    {
        AZStd::vector<const TestTarget*> discardedTestTargets;
        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList);
        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, testPrioritizationPolicy);
        const AZStd::unordered_set<const TestTarget*> selectedTestTargetSet(selectedTestTargets.begin(), selectedTestTargets.end());

        if (m_testShardingPolicy == Policy::TestSharding::Always)
        {
            AZStd::vector<const TestTarget*> testTargets;
            const auto addMutatedTestTargetsToEnumerationList = [this, &testTargets](const AZStd::vector<SourceDependency>& sourceDependency)
            {
                for (const auto& sourceDependency : sourceDependency)
                {
                    for (const auto& parentTarget : sourceDependency.GetParentTargets())
                    {
                        AZStd::visit([&testTargets]([[maybe_unused]]auto&& target)
                        {
                            if constexpr (IsTestTarget<decltype(target)>)
                            {
                                testTargets.push_back(target);
                            }
                        }, parentTarget.GetTarget());
                    }
                }
            };
            addMutatedTestTargetsToEnumerationList(changeDependecyList.GetCreateSourceDependencies());
            addMutatedTestTargetsToEnumerationList(changeDependecyList.GetUpdateSourceDependencies());
            m_testEngine->UpdateEnumerationCache(testTargets, Policy::ExecutionFailure::Ignore, AZStd::nullopt, AZStd::nullopt, AZStd::nullopt);
        }

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

    void Runtime::UpdateAndSerializeDynamicDependencyMap(const SourceCoveringTestsList& sourceCoverageTestsList)
    {
        if (!sourceCoverageTestsList.GetNumSources())
        {
            return;
        }

        m_dynamicDependencyMap->ReplaceSourceCoverage(sourceCoverageTestsList);
        const auto sparTIA = m_dynamicDependencyMap->ExportSourceCoverage();
        const auto sparTIAData = SerializeSourceCoveringTestsList(sparTIA);
        WriteFileContents<RuntimeException>(sparTIAData, m_config.m_workspace.m_active.m_relativePaths.m_sparTIAFile);
        m_hasImpactAnalysisData = true;
    }

    TestSequenceResult Runtime::RegularTestSequence(
        const AZStd::unordered_set<AZStd::string> suitesFilter,
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
                if (suitesFilter.empty())
                {
                    // Suite filter is empty, all tests that are not on the excluded list are included
                    includedTestTargets.push_back(&testTarget);
                }
                else if(suitesFilter.contains(testTarget.GetSuite()))
                {
                    // Test target belonging to a suite in the suite filter are included, provided that are not on the exclude list
                    includedTestTargets.push_back(&testTarget);
                }
                else
                {
                    // Test target not belonging to a suite in the suite filter are excluded
                    excludedTestTargets.push_back(&testTarget);
                }
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
            (*testSequenceEndCallback)(CreateSequenceFailureReport(testJobs), timer.Elapsed());
        }

        return result;
    }

    TestSequenceResult Runtime::ImpactAnalysisTestSequence(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        Timer timer;
        AZStd::vector<const TestTarget*> draftedTestTargets;

        auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargetsAndEnumerateMutatedTestTargets(changeList, testPrioritizationPolicy);
        auto [includedSelectedTestTargets, excludedSelectedTestTargets] = SelectTestTargetsByExcludeList(selectedTestTargets);

        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(
                Client::TestRunSelection(ExtractTestTargetNames(includedSelectedTestTargets), ExtractTestTargetNames(excludedSelectedTestTargets)),
                ExtractTestTargetNames(discardedTestTargets),
                ExtractTestTargetNames(draftedTestTargets));
        }

        const auto [result, testJobs] = m_testEngine->InstrumentedRun(
            includedSelectedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            Policy::IntegrityFailure::Continue,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler(testCompleteCallback));

        UpdateAndSerializeDynamicDependencyMap(CreateSourceCoveringTestFromTestCoverages(testJobs, m_config.m_repo.m_root));

        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(CreateSequenceFailureReport(testJobs), timer.Elapsed());
        }

        return result;
    }

    AZStd::pair<TestSequenceResult, TestSequenceResult> Runtime::SafeImpactAnalysisTestSequence(
        const ChangeList& changeList,
        const AZStd::unordered_set<AZStd::string> suitesFilter,
        Policy::TestPrioritization testPrioritizationPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<SafeTestSequenceCompleteCallback> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        Timer timer;
        AZStd::vector<const TestTarget*> draftedTestTargets;

        auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargetsAndEnumerateMutatedTestTargets(changeList, testPrioritizationPolicy);
        auto [includedSelectedTestTargets, excludedSelectedTestTargets] = SelectTestTargetsByExcludeList(selectedTestTargets);
        auto [includedDiscardedTestTargets, excludedDiscardedTestTargets] = SelectTestTargetsByExcludeList(discardedTestTargets);

        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(
                Client::TestRunSelection(ExtractTestTargetNames(includedSelectedTestTargets), ExtractTestTargetNames(excludedSelectedTestTargets)),
                Client::TestRunSelection(ExtractTestTargetNames(includedDiscardedTestTargets), ExtractTestTargetNames(excludedDiscardedTestTargets)),
                ExtractTestTargetNames(draftedTestTargets));
        }

        // Impact analysis run of the selected test targets
        const auto [selectedResult, selectedTestJobs] = m_testEngine->InstrumentedRun(
            includedSelectedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            Policy::IntegrityFailure::Continue,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler(testCompleteCallback));

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

        UpdateAndSerializeDynamicDependencyMap(CreateSourceCoveringTestFromTestCoverages(selectedTestJobs, m_config.m_repo.m_root));

        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(
                CreateSequenceFailureReport(selectedTestJobs),
                CreateSequenceFailureReport(discardedTestJobs),
                timer.Elapsed());
        }

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
            m_executionFailurePolicy, // NOTE: as exec failures are only known after, the exec failure logic with known error code MUST happen in the test runner!!!!!!!!!!!
            Policy::IntegrityFailure::Continue,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler(testCompleteCallback));

        // DELETE THE EXISTING DATA FIRST!!!!
        UpdateAndSerializeDynamicDependencyMap(CreateSourceCoveringTestFromTestCoverages(testJobs, m_config.m_repo.m_root));

        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(CreateSequenceFailureReport(testJobs), timer.Elapsed());
        }

        return result;
    }

    bool Runtime::HasImpactAnalysisData() const
    {
        return m_hasImpactAnalysisData;
    }
}
