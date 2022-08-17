/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/Native/TestImpactNativeRuntime.h>

#include <TestImpactRuntimeUtils.h>
#include <Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.h>
#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <Dependency/TestImpactDependencyException.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactSourceCoveringTestsSerializer.h>
#include <Dependency/TestImpactTestSelectorAndPrioritizer.h>
#include <Target/Native/TestImpactNativeProductionTarget.h>
#include <Target/Native/TestImpactNativeTargetListCompiler.h>
#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestEngine/Native/TestImpactNativeTestEngine.h>

#include <AzCore/IO/SystemFile.h>

namespace TestImpact
{
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
        AZStd::chrono::high_resolution_clock::time_point m_relativeStartTime;
        AZStd::chrono::milliseconds m_duration = AZStd::chrono::milliseconds{ 0 };
    };

    //! Wrapper for the impact analysis test sequence to handle both the updating and non-updating policies through a common pathway.
    //! @tparam TestRunnerFunctor The functor for running the specified tests.
    //! @tparam TestJob The test engine job type returned by the functor.
    //! @param maxConcurrency The maximum concurrency being used for this sequence.
    //! @param policyState The policy state being used for the sequence.
    //! @param suiteType The suite type used for this sequence.
    //! @param timer The timer to use for the test run timings.
    //! @param testRunner The test runner functor to use for each of the test runs.
    //! @param includedSelectedTestTargets The subset of test targets that were selected to run and not also fully excluded from running.
    //! @param excludedSelectedTestTargets The subset of test targets that were selected to run but were fully excluded running.
    //! @param discardedTestTargets The subset of test targets that were discarded from the test selection and will not be run.
    //! @param globalTimeout The maximum duration the entire test sequence may run for (infinite if empty).
    //! @param testSequenceStartCallback The client function to be called after the test targets have been selected but prior to running the
    //! tests.
    //! @param testSequenceCompleteCallback The client function to be called after the test sequence has completed.
    //! @param testRunCompleteCallback The client function to be called after an individual test run has completed.
    //! @param updateCoverage The function to call to update the dynamic dependency map with test coverage (if any).
    template<typename TestRunnerFunctor, typename TestJob>
    Client::ImpactAnalysisSequenceReport ImpactAnalysisTestSequenceWrapper(
        size_t maxConcurrency,
        const ImpactAnalysisSequencePolicyState& policyState,
        SuiteType suiteType,
        const Timer& sequenceTimer,
        const TestRunnerFunctor& testRunner,
        const AZStd::vector<const NativeTestTarget*>& includedSelectedTestTargets,
        const AZStd::vector<const NativeTestTarget*>& excludedSelectedTestTargets,
        const AZStd::vector<const NativeTestTarget*>& discardedTestTargets,
        const AZStd::vector<const NativeTestTarget*>& draftedTestTargets,
        const AZStd::optional<AZStd::chrono::milliseconds>& testTargetTimeout,
        const AZStd::optional<AZStd::chrono::milliseconds>& globalTimeout,
        AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::ImpactAnalysisSequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback,
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
        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(suiteType, selectedTests, discardedTests, draftedTests);
        }

        // We share the test run complete handler between the selected and drafted test runs as to present them together as one
        // continuous test sequence to the client rather than two discrete test runs
        const size_t totalNumTestRuns = includedSelectedTestTargets.size() + draftedTestTargets.size();
        TestRunCompleteCallbackHandler<NativeTestTarget> testRunCompleteHandler(totalNumTestRuns, testCompleteCallback);

        const auto gatherTestRunData = [&sequenceTimer, &testRunner, &testRunCompleteHandler, &globalTimeout]
        (const AZStd::vector<const NativeTestTarget*>& testsTargets, TestRunData<TestJob>& testRunData)
        {
            const Timer testRunTimer;
            testRunData.m_relativeStartTime = testRunTimer.GetStartTimePointRelative(sequenceTimer);
            auto [result, jobs] = testRunner(testsTargets, testRunCompleteHandler, globalTimeout);
            testRunData.m_result = result;
            testRunData.m_jobs = AZStd::move(jobs);
            testRunData.m_duration = testRunTimer.GetElapsedMs();
        };

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
            suiteType,
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
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(sequenceReport);
        }

        // Update the dynamic dependency map with the latest coverage data (if any)
        if (updateCoverage.has_value())
        {
            (*updateCoverage)(ConcatenateVectors(selectedTestRunData.m_jobs, draftedTestRunData.m_jobs));
        }

        return sequenceReport;
    }

    NativeTestTargetMetaMap ReadNativeTestTargetMetaMapFile(SuiteType suiteFilter, const RepoPath& testTargetMetaConfigFile)
    {
        const auto masterTestListData = ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        return NativeTestTargetMetaMapFactory(masterTestListData, suiteFilter);
    }

    NativeRuntime::NativeRuntime(
        NativeRuntimeConfig&& config,
        const AZStd::optional<RepoPath>& dataFile,
        [[maybe_unused]] const AZStd::optional<RepoPath>& previousRunDataFile,
        const AZStd::vector<ExcludedTarget>& testsToExclude,
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
        // Construct the build targets from the build target descriptors
        auto targetDescriptors = ReadTargetDescriptorFiles(m_config.m_commonConfig.m_buildTargetDescriptor);
        auto buildTargets = CompileNativeTargetLists(
            AZStd::move(targetDescriptors),
            ReadNativeTestTargetMetaMapFile(suiteFilter, m_config.m_commonConfig.m_testTargetMeta.m_metaFile));
        auto&& [productionTargets, testTargets] = buildTargets;
        m_buildTargets = AZStd::make_unique<BuildTargetList<ProductionTarget, TestTarget>>(
            AZStd::move(testTargets), AZStd::move(productionTargets));

        // Construct the dynamic dependency map from the build targets
        m_dynamicDependencyMap = AZStd::make_unique<DynamicDependencyMap<ProductionTarget, TestTarget>>(m_buildTargets.get());

        // Construct the test selector and prioritizer from the dependency graph data (NOTE: currently not implemented)
        m_testSelectorAndPrioritizer = AZStd::make_unique<TestSelectorAndPrioritizer<ProductionTarget, TestTarget>>(*m_dynamicDependencyMap.get());

        // Construct the target exclude list from the exclude file if provided, otherwise use target configuration data
        if (!testsToExclude.empty())
        {
            // Construct using data from excludeTestFile
            m_regularTestTargetExcludeList =
                ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList(), testsToExclude);
            m_instrumentedTestTargetExcludeList =
                ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList(), testsToExclude);
        }
        else
        {
            // Construct using data from config file.
            m_regularTestTargetExcludeList = ConstructTestTargetExcludeList(
                m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList(),
                m_config.m_target.m_excludedTargets.m_excludedRegularTestTargets);
            m_instrumentedTestTargetExcludeList = ConstructTestTargetExcludeList(
                m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList(),
                m_config.m_target.m_excludedTargets.m_excludedInstrumentedTestTargets);
        }

        // Construct the test engine with the workspace path and launcher binaries
        m_testEngine = AZStd::make_unique<TestEngine>(
            m_config.m_commonConfig.m_repo.m_root,
            m_config.m_target.m_outputDirectory,
            m_config.m_workspace.m_temp.m_enumerationCacheDirectory,
            m_config.m_workspace.m_temp,
            m_config.m_testEngine.m_testRunner.m_binary,
            m_config.m_testEngine.m_instrumentation.m_binary,
            m_maxConcurrency);

        try
        {
            if (dataFile.has_value())
            {
                m_sparTiaFile = dataFile.value().String();
            }
            else
            {
                m_sparTiaFile =
                    m_config.m_workspace.m_active.m_root / RepoPath(SuiteTypeAsString(m_suiteFilter)) / m_config.m_workspace.m_active.m_sparTiaFile;
            }
           
            // Populate the dynamic dependency map with the existing source coverage data (if any)
            const auto tiaDataRaw = ReadFileContents<Exception>(m_sparTiaFile);
            const auto tiaData = DeserializeSourceCoveringTestsList(tiaDataRaw);
            if (tiaData.GetNumSources())
            {
                m_dynamicDependencyMap->ReplaceSourceCoverage(tiaData);
                m_hasImpactAnalysisData = true;

                // Enumerate new test targets
                //const auto testTargetsWithNoEnumeration = m_dynamicDependencyMap->GetNotCoveringTests();
                //if (!testTargetsWithNoEnumeration.empty())
                //{
                //    m_testEngine->UpdateEnumerationCache(
                //        testTargetsWithNoEnumeration,
                //        Policy::ExecutionFailure::Ignore,
                //        Policy::TestFailure::Continue,
                //        AZStd::nullopt,
                //        AZStd::nullopt,
                //        AZStd::nullopt);
                //}
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
                    "No test impact analysis data found for suite '%s' at %s\n", SuiteTypeAsString(m_suiteFilter).c_str(), m_sparTiaFile.c_str()).c_str());
        }
    }

    NativeRuntime::~NativeRuntime() = default;

    AZStd::pair<AZStd::vector<const NativeTestTarget*>, AZStd::vector<const NativeTestTarget*>> NativeRuntime::SelectCoveringTestTargets(
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
        //EnumerateMutatedTestTargets(changeDependencyList);

        // The test targets in the main list not in the selected test target set are the test targets not selected for this change list
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
        {
            if (!selectedTestTargetSet.contains(&testTarget))
            {
                discardedTestTargets.push_back(&testTarget);
            }
        }

        return { selectedTestTargets, discardedTestTargets };
    }

    void NativeRuntime::ClearDynamicDependencyMapAndRemoveExistingFile()
    {
        m_dynamicDependencyMap->ClearAllSourceCoverage();
        DeleteFile(m_sparTiaFile);
    }

    PolicyStateBase NativeRuntime::GeneratePolicyStateBase() const
    {
        PolicyStateBase policyState;

        policyState.m_executionFailurePolicy = m_executionFailurePolicy;
        policyState.m_failedTestCoveragePolicy = m_failedTestCoveragePolicy;
        policyState.m_integrityFailurePolicy = m_integrationFailurePolicy;
        policyState.m_targetOutputCapture = m_targetOutputCapture;
        policyState.m_testFailurePolicy = m_testFailurePolicy;
        policyState.m_testShardingPolicy = m_testShardingPolicy;

        return policyState;
    }

    SequencePolicyState NativeRuntime::GenerateSequencePolicyState() const
    {
        return { GeneratePolicyStateBase() };
    }

    SafeImpactAnalysisSequencePolicyState NativeRuntime::GenerateSafeImpactAnalysisSequencePolicyState(
        Policy::TestPrioritization testPrioritizationPolicy) const
    {
        return { GeneratePolicyStateBase(), testPrioritizationPolicy };
    }

    ImpactAnalysisSequencePolicyState NativeRuntime::GenerateImpactAnalysisSequencePolicyState(
        Policy::TestPrioritization testPrioritizationPolicy, Policy::DynamicDependencyMap dynamicDependencyMapPolicy) const
    {
        return { GeneratePolicyStateBase(), testPrioritizationPolicy, dynamicDependencyMapPolicy };
    }

    Client::RegularSequenceReport NativeRuntime::RegularTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::RegularSequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        const Timer sequenceTimer;
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;
        
        // Separate the test targets into those that are excluded by either the test filter or exclusion list and those that are not
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
        {
            if (m_regularTestTargetExcludeList->IsTestTargetFullyExcluded(&testTarget))
            {
                excludedTestTargets.push_back(&testTarget);
            }
            else
            {
                includedTestTargets.push_back(&testTarget);
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
            m_executionFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler<TestTarget>(includedTestTargets.size(), testCompleteCallback));
        const auto testRunDuration = testRunTimer.GetElapsedMs();

        // Generate the sequence report for the client
        const auto sequenceReport = Client::RegularSequenceReport(
            m_maxConcurrency,
            testTargetTimeout,
            globalTimeout,
            GenerateSequencePolicyState(),
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

    Client::ImpactAnalysisSequenceReport NativeRuntime::ImpactAnalysisTestSequence(
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
        const AZStd::vector<const TestTarget*> draftedTestTargets = m_dynamicDependencyMap->GetNotCoveringTests();

        const auto selectCoveringTestTargetsAndPruneDraftedFromDiscarded =
            [this, &draftedTestTargets, &changeList, testPrioritizationPolicy]()
        {
            // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
            const auto [selectedTestTargets, discardedTestTargets] =
                SelectCoveringTestTargets(changeList, testPrioritizationPolicy);

            const AZStd::unordered_set<const TestTarget*> draftedTestTargetsSet(draftedTestTargets.begin(), draftedTestTargets.end());

            AZStd::vector<const TestTarget*> discardedNotDraftedTestTargets;
            for (const auto* testTarget : discardedTestTargets)
            {
                if (!draftedTestTargetsSet.count(testTarget))
                {
                    discardedNotDraftedTestTargets.push_back(testTarget);
                }
            }

            return AZStd::pair{ selectedTestTargets, discardedNotDraftedTestTargets };
        };

        const auto [selectedTestTargets, discardedTestTargets] = selectCoveringTestTargetsAndPruneDraftedFromDiscarded();

        // The subset of selected test targets that are not on the configuration's exclude list and those that are
        const auto [includedSelectedTestTargets, excludedSelectedTestTargets] =
            SelectTestTargetsByExcludeList(*m_instrumentedTestTargetExcludeList, selectedTestTargets);

        // Functor for running instrumented test targets
        const auto instrumentedTestRun =
            [this, &testTargetTimeout](
                const AZStd::vector<const TestTarget*>& testsTargets,
                TestRunCompleteCallbackHandler<TestTarget>& testRunCompleteHandler,
                AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
        {
            return m_testEngine->InstrumentedRun(
                testsTargets,
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
                 TestRunCompleteCallbackHandler<TestTarget>& testRunCompleteHandler,
                AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
        {
            return m_testEngine->RegularRun(
                testsTargets,
                m_executionFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                AZStd::ref(testRunCompleteHandler));
        };

        if (dynamicDependencyMapPolicy == Policy::DynamicDependencyMap::Update)
        {
            AZStd::optional<AZStd::function<void(const AZStd::vector<TestEngineInstrumentedRun<TestTarget, TestCoverage>>& jobs)>>
                updateCoverage = [this](const AZStd::vector<TestEngineInstrumentedRun<TestTarget, TestCoverage>>& jobs)
            {
                m_hasImpactAnalysisData = UpdateAndSerializeDynamicDependencyMap(
                    *m_dynamicDependencyMap.get(),
                    jobs,
                    m_failedTestCoveragePolicy,
                    m_integrationFailurePolicy,
                    m_config.m_commonConfig.m_repo.m_root,
                    m_sparTiaFile).value_or(m_hasImpactAnalysisData);
            };

            return ImpactAnalysisTestSequenceWrapper(
                m_maxConcurrency,
                GenerateImpactAnalysisSequencePolicyState(testPrioritizationPolicy, dynamicDependencyMapPolicy),
                m_suiteFilter,
                sequenceTimer,
                instrumentedTestRun,
                includedSelectedTestTargets,
                excludedSelectedTestTargets,
                discardedTestTargets,
                draftedTestTargets,
                testTargetTimeout,
                globalTimeout,
                testSequenceStartCallback,
                testSequenceEndCallback,
                testCompleteCallback,
                updateCoverage);
        }
        else
        {
            return ImpactAnalysisTestSequenceWrapper(
                m_maxConcurrency,
                GenerateImpactAnalysisSequencePolicyState(testPrioritizationPolicy, dynamicDependencyMapPolicy),
                m_suiteFilter,
                sequenceTimer,
                regularTestRun,
                includedSelectedTestTargets,
                excludedSelectedTestTargets,
                discardedTestTargets,
                draftedTestTargets,
                testTargetTimeout,
                globalTimeout,
                testSequenceStartCallback,
                testSequenceEndCallback,
                testCompleteCallback,
                AZStd::optional<AZStd::function<void(const AZStd::vector<TestEngineRegularRun<TestTarget>>& jobs)>>{
                    AZStd::nullopt });
        }
    }

    Client::SafeImpactAnalysisSequenceReport NativeRuntime::SafeImpactAnalysisTestSequence(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::SafeImpactAnalysisSequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        const Timer sequenceTimer;
        TestRunData<TestEngineInstrumentedRun<TestTarget, TestCoverage>> selectedTestRunData, draftedTestRunData;
        TestRunData<TestEngineRegularRun<TestTarget>> discardedTestRunData;
        AZStd::optional<AZStd::chrono::milliseconds> sequenceTimeout = globalTimeout;

        // Draft in the test targets that have no coverage entries in the dynamic dependency map
        AZStd::vector<const TestTarget*> draftedTestTargets = m_dynamicDependencyMap->GetNotCoveringTests();

        // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        const auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargets(changeList, testPrioritizationPolicy);

        // The subset of selected test targets that are not on the configuration's exclude list and those that are
        const auto [includedSelectedTestTargets, excludedSelectedTestTargets] =
            SelectTestTargetsByExcludeList(*m_instrumentedTestTargetExcludeList, selectedTestTargets);

        // The subset of discarded test targets that are not on the configuration's exclude list and those that are
        const auto [includedDiscardedTestTargets, excludedDiscardedTestTargets] =
            SelectTestTargetsByExcludeList(*m_regularTestTargetExcludeList, discardedTestTargets);

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
        TestRunCompleteCallbackHandler<TestTarget> testRunCompleteHandler(totalNumTestRuns, testCompleteCallback);
        
        // Functor for running instrumented test targets
        const auto instrumentedTestRun =
            [this, &testTargetTimeout, &sequenceTimeout, &testRunCompleteHandler](const AZStd::vector<const TestTarget*>& testsTargets)
        {
            return m_testEngine->InstrumentedRun(
                testsTargets,
                m_executionFailurePolicy,
                m_integrationFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                sequenceTimeout,
                AZStd::ref(testRunCompleteHandler));
        };

        // Functor for running uninstrumented test targets
        const auto regularTestRun =
            [this, &testTargetTimeout, &sequenceTimeout, &testRunCompleteHandler](const AZStd::vector<const TestTarget*>& testsTargets)
        {
            return m_testEngine->RegularRun(
                testsTargets,
                m_executionFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                sequenceTimeout,
                AZStd::ref(testRunCompleteHandler));
        };

        // Functor for running instrumented test targets
        const auto gatherTestRunData = [&sequenceTimer]
        (const AZStd::vector<const TestTarget*>& testsTargets, const auto& testRunner, auto& testRunData)
        {
            const Timer testRunTimer;
            testRunData.m_relativeStartTime = testRunTimer.GetStartTimePointRelative(sequenceTimer);
            auto [result, jobs] = testRunner(testsTargets);
            testRunData.m_result = result;
            testRunData.m_jobs = AZStd::move(jobs);
            testRunData.m_duration = testRunTimer.GetElapsedMs();
        };

        if (!includedSelectedTestTargets.empty())
        {
            // Run the selected test targets and collect the test run results
            gatherTestRunData(includedSelectedTestTargets, instrumentedTestRun, selectedTestRunData);

            // Carry the remaining global sequence time over to the discarded test run
            if (globalTimeout.has_value())
            {
                const auto elapsed = selectedTestRunData.m_duration;
                sequenceTimeout = elapsed < globalTimeout.value() ? globalTimeout.value() - elapsed : AZStd::chrono::milliseconds(0);
            }
        }

        if (!includedDiscardedTestTargets.empty())
        {
            // Run the discarded test targets and collect the test run results
            gatherTestRunData(includedDiscardedTestTargets, regularTestRun, discardedTestRunData);

            // Carry the remaining global sequence time over to the drafted test run
            if (globalTimeout.has_value())
            {
                const auto elapsed = selectedTestRunData.m_duration + discardedTestRunData.m_duration;
                sequenceTimeout = elapsed < globalTimeout.value() ? globalTimeout.value() - elapsed : AZStd::chrono::milliseconds(0);
            }
        }

        if (!draftedTestTargets.empty())
        {
            // Run the drafted test targets and collect the test run results
            gatherTestRunData(draftedTestTargets, instrumentedTestRun, draftedTestRunData);
        }

        // Generate the sequence report for the client
        const auto sequenceReport = Client::SafeImpactAnalysisSequenceReport(
            m_maxConcurrency,
            testTargetTimeout,
            globalTimeout,
            GenerateSafeImpactAnalysisSequencePolicyState(testPrioritizationPolicy),
            m_suiteFilter,
            selectedTests,
            discardedTests,
            draftedTests,
            GenerateTestRunReport(
                selectedTestRunData.m_result,
                selectedTestRunData.m_relativeStartTime,
                selectedTestRunData.m_duration,
                selectedTestRunData.m_jobs),
            GenerateTestRunReport(
                discardedTestRunData.m_result,
                discardedTestRunData.m_relativeStartTime,
                discardedTestRunData.m_duration,
                discardedTestRunData.m_jobs),
            GenerateTestRunReport(
                draftedTestRunData.m_result, 
                draftedTestRunData.m_relativeStartTime, 
                draftedTestRunData.m_duration,
                draftedTestRunData.m_jobs));

        // Inform the client that the sequence has ended
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(sequenceReport);
        }

        m_hasImpactAnalysisData = UpdateAndSerializeDynamicDependencyMap(
                    *m_dynamicDependencyMap.get(),
                    ConcatenateVectors(selectedTestRunData.m_jobs, draftedTestRunData.m_jobs),
                    m_failedTestCoveragePolicy,
                    m_integrationFailurePolicy,
                    m_config.m_commonConfig.m_repo.m_root,
                    m_sparTiaFile).value_or(m_hasImpactAnalysisData);

        return sequenceReport;
    }

    Client::SeedSequenceReport NativeRuntime::SeededTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::SeedSequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        const Timer sequenceTimer;
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;

        // Separate the test targets into those that are excluded by either the test filter or exclusion list and those that are not
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
        {
            if (m_instrumentedTestTargetExcludeList->IsTestTargetFullyExcluded(&testTarget))
            {
                excludedTestTargets.push_back(&testTarget);
            }
            else
            {
                includedTestTargets.push_back(&testTarget);
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
            m_executionFailurePolicy,
            m_integrationFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler<TestTarget>(includedTestTargets.size(), testCompleteCallback));
        const auto testRunDuration = testRunTimer.GetElapsedMs();

        // Generate the sequence report for the client
        const auto sequenceReport = Client::SeedSequenceReport(
            m_maxConcurrency,
            testTargetTimeout,
            globalTimeout,
            GenerateSequencePolicyState(),
            m_suiteFilter,
            selectedTests,
            GenerateTestRunReport(result, testRunTimer.GetStartTimePointRelative(sequenceTimer), testRunDuration, testJobs));

        // Inform the client that the sequence has ended
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(sequenceReport);
        }

        ClearDynamicDependencyMapAndRemoveExistingFile();

         m_hasImpactAnalysisData = UpdateAndSerializeDynamicDependencyMap(
                    *m_dynamicDependencyMap.get(),
                    testJobs,
                    m_failedTestCoveragePolicy,
                    m_integrationFailurePolicy,
                    m_config.m_commonConfig.m_repo.m_root,
                    m_sparTiaFile).value_or(m_hasImpactAnalysisData);

        return sequenceReport;
    }

    bool NativeRuntime::HasImpactAnalysisData() const
    {
        return m_hasImpactAnalysisData;
    }
}
