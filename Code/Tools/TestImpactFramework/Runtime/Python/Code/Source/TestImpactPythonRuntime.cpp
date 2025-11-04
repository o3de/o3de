/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/Python/TestImpactPythonRuntime.h>

#include <TestImpactRuntimeUtils.h>
#include <Artifact/Static/TestImpactPythonTestTargetMeta.h>
#include <Artifact/Factory/TestImpactPythonTestTargetMetaMapFactory.h>
#include <Dependency/TestImpactPythonTestSelectorAndPrioritizer.h>
#include <Dependency/TestImpactSourceCoveringTestsSerializer.h>
#include <Dependency/TestImpactTestSelectorAndPrioritizer.h>
#include <Target/Python/TestImpactPythonProductionTarget.h>
#include <Target/Python/TestImpactPythonTargetListCompiler.h>
#include <Target/Python/TestImpactPythonTestTarget.h>
#include <TestEngine/Python/TestImpactPythonTestEngine.h>
#include <TestRunner/Common/Run/TestImpactTestCoverage.h>

#include <AzCore/std/string/regex.h>

namespace TestImpact
{
    PythonTestTargetMetaMap ReadPythonTestTargetMetaMapFile(
        const SuiteSet& suiteSet,
        const SuiteLabelExcludeSet& suiteLabelExcludeSet,
        const RepoPath& testTargetMetaConfigFile,
        const AZStd::string& buildType)
    {
        const auto masterTestListData = ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        auto testTargetMetaMap = PythonTestTargetMetaMapFactory(masterTestListData, suiteSet, suiteLabelExcludeSet);
        for (auto& [name, meta] : testTargetMetaMap)
        {
            meta.m_scriptMeta.m_testCommand = AZStd::regex_replace(meta.m_scriptMeta.m_testCommand, AZStd::regex("\\$\\<CONFIG\\>"), buildType);
        }

        return testTargetMetaMap;
    }

    PythonRuntime::PythonRuntime(
        PythonRuntimeConfig&& config,
        const AZStd::optional<RepoPath>& dataFile,
        [[maybe_unused]] const AZStd::optional<RepoPath>& previousRunDataFile,
        const AZStd::vector<ExcludedTarget>& testsToExclude,
        const SuiteSet& suiteSet,
        const SuiteLabelExcludeSet& suiteLabelExcludeSet,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::FailedTestCoverage failedTestCoveragePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::IntegrityFailure integrationFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        Policy::TestRunner testRunnerPolicy)
        : m_config(AZStd::move(config))
        , m_suiteSet(suiteSet)
        , m_suiteLabelExcludeSet(suiteLabelExcludeSet)
        , m_executionFailurePolicy(executionFailurePolicy)
        , m_failedTestCoveragePolicy(failedTestCoveragePolicy)
        , m_testFailurePolicy(testFailurePolicy)
        , m_integrationFailurePolicy(integrationFailurePolicy)
        , m_targetOutputCapture(targetOutputCapture)
        , m_testRunnerPolicy(testRunnerPolicy)
    {
        // Construct the build targets from the build target descriptors
        auto targetDescriptors = ReadTargetDescriptorFiles(m_config.m_commonConfig.m_buildTargetDescriptor);
        auto buildTargets = CompilePythonTargetLists(
            AZStd::move(targetDescriptors),
            ReadPythonTestTargetMetaMapFile(
                m_suiteSet, m_suiteLabelExcludeSet, m_config.m_commonConfig.m_testTargetMeta.m_metaFile, m_config.m_commonConfig.m_meta.m_buildConfig));
        auto&& [productionTargets, testTargets] = buildTargets;
        m_buildTargets = AZStd::make_unique<BuildTargetList<PythonProductionTarget, PythonTestTarget>>(
            AZStd::move(testTargets), AZStd::move(productionTargets));

        // Construct the dynamic dependency map from the build targets
        m_dynamicDependencyMap = AZStd::make_unique<DynamicDependencyMap<PythonProductionTarget, PythonTestTarget>>(m_buildTargets.get());

        // Construct the test selector and prioritizer from the dependency graph data (NOTE: currently not implemented)
        m_testSelectorAndPrioritizer = AZStd::make_unique<PythonTestSelectorAndPrioritizer>(*m_dynamicDependencyMap.get());

        // Construct the target exclude list from the exclude file if provided, otherwise use target configuration data
        if (!testsToExclude.empty())
        {
            // Construct using data from excludeTestFile
            m_testTargetExcludeList =
                ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList(), testsToExclude);
        }
        else
        {
            // Construct using data from config file.
            m_testTargetExcludeList = ConstructTestTargetExcludeList(
                m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList(), m_config.m_target.m_excludedTargets);
        }

        // Construct the test engine with the workspace path and launcher binaries
        m_testEngine = AZStd::make_unique<PythonTestEngine>(
            m_config.m_commonConfig.m_repo.m_root,
            m_config.m_commonConfig.m_repo.m_build,
            m_config.m_workspace.m_temp,
            m_testRunnerPolicy);

        try
        {
            if (dataFile.has_value())
            {
                m_sparTiaFile = dataFile.value().String();
            }
            else
            {
                m_sparTiaFile = m_config.m_workspace.m_active.m_root / RepoPath(SuiteSetAsString(m_suiteSet)) /
                    m_config.m_workspace.m_active.m_sparTiaFile;
            }

            // Populate the dynamic dependency map with the existing source coverage data (if any)
            const auto tiaDataRaw = ReadFileContents<Exception>(m_sparTiaFile);
            const auto tiaData = DeserializeSourceCoveringTestsList(tiaDataRaw);
            if (tiaData.GetNumSources())
            {
                m_dynamicDependencyMap->ReplaceSourceCoverage(tiaData);
                m_hasImpactAnalysisData = true;

                // Enumerate new test targets
                // const auto testTargetsWithNoEnumeration = m_dynamicDependencyMap->GetNotCoveringTests();
                // if (!testTargetsWithNoEnumeration.empty())
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
        catch ([[maybe_unused]] const Exception& e)
        {
            AZ_Printf(
                LogCallSite,
                AZStd::string::format(
                    "No test impact analysis data found for suites '%s' at %s\n",
                    SuiteSetAsString(m_suiteSet).c_str(),
                    m_sparTiaFile.c_str())
                    .c_str());
        }
    }

    PythonRuntime::~PythonRuntime() = default;

    AZStd::pair<AZStd::vector<const PythonTestTarget*>, AZStd::vector<const PythonTestTarget*>> PythonRuntime::SelectCoveringTestTargets(
        const ChangeList& changeList, Policy::TestPrioritization testPrioritizationPolicy)
    {
        AZStd::vector<const PythonTestTarget*> discardedTestTargets;

        // Select and prioritize the test targets pertinent to this change list
        const auto changeDependencyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, m_integrationFailurePolicy);
        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependencyList, testPrioritizationPolicy);

        // Populate a set with the selected test targets so that we can infer the discarded test target not selected for this change list
        const AZStd::unordered_set<const PythonTestTarget*> selectedTestTargetSet(selectedTestTargets.begin(), selectedTestTargets.end());

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

    void PythonRuntime::ClearDynamicDependencyMapAndRemoveExistingFile()
    {
        m_dynamicDependencyMap->ClearAllSourceCoverage();
        DeleteFile(m_sparTiaFile);
    }

    PolicyStateBase PythonRuntime::GeneratePolicyStateBase() const
    {
        PolicyStateBase policyState;

        policyState.m_executionFailurePolicy = m_executionFailurePolicy;
        policyState.m_failedTestCoveragePolicy = m_failedTestCoveragePolicy;
        policyState.m_integrityFailurePolicy = m_integrationFailurePolicy;
        policyState.m_targetOutputCapture = m_targetOutputCapture;
        policyState.m_testFailurePolicy = m_testFailurePolicy;

        return policyState;
    }

    SequencePolicyState PythonRuntime::GenerateSequencePolicyState() const
    {
        return { GeneratePolicyStateBase() };
    }

    SafeImpactAnalysisSequencePolicyState PythonRuntime::GenerateSafeImpactAnalysisSequencePolicyState(
        Policy::TestPrioritization testPrioritizationPolicy) const
    {
        return { GeneratePolicyStateBase(), testPrioritizationPolicy };
    }

    ImpactAnalysisSequencePolicyState PythonRuntime::GenerateImpactAnalysisSequencePolicyState(
        Policy::TestPrioritization testPrioritizationPolicy, Policy::DynamicDependencyMap dynamicDependencyMapPolicy) const
    {
        return { GeneratePolicyStateBase(), testPrioritizationPolicy, dynamicDependencyMapPolicy };
    }

    Client::RegularSequenceReport PythonRuntime::RegularTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
    {
        const Timer sequenceTimer;
        AZStd::vector<const PythonTestTarget*> includedTestTargets;
        AZStd::vector<const PythonTestTarget*> excludedTestTargets;

        // Separate the test targets into those that are excluded by either the test filter or exclusion list and those that are not
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
        {
            if (m_testTargetExcludeList->IsTestTargetFullyExcluded(&testTarget))
            {
                excludedTestTargets.push_back(&testTarget);
            }
            else
            {
                includedTestTargets.push_back(&testTarget);
            }
        }

        // Extract the client facing representation of selected test targets
        const Client::TestRunSelection selectedTests(
            ExtractTestTargetNames(includedTestTargets), ExtractTestTargetNames(excludedTestTargets));

        // Inform the client that the sequence is about to start
        RegularTestSequenceNotificationBus::Broadcast(
            &RegularTestSequenceNotificationBus::Events::OnTestSequenceStart, m_suiteSet, m_suiteLabelExcludeSet, selectedTests);

        // Run the test targets and collect the test run results
        TestEngineNotificationHandler<PythonTestTarget> handler(includedTestTargets.size());
        const Timer testRunTimer;
        const auto [result, testJobs] = m_testEngine->RegularRun(
            includedTestTargets,
            m_executionFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout);
        const auto testRunDuration = testRunTimer.GetElapsedMs();

        // Generate the sequence report for the client
        const auto sequenceReport = Client::RegularSequenceReport(
            1,
            testTargetTimeout,
            globalTimeout,
            GenerateSequencePolicyState(),
            m_suiteSet,
            m_suiteLabelExcludeSet,
            selectedTests,
            GenerateTestRunReport(result, testRunTimer.GetStartTimePointRelative(sequenceTimer), testRunDuration, testJobs));

        // Inform the client that the sequence has ended
        RegularTestSequenceNotificationBus::Broadcast(
            &RegularTestSequenceNotificationBus::Events::OnTestSequenceComplete, sequenceReport);

        return sequenceReport;
    }

    Client::ImpactAnalysisSequenceReport PythonRuntime::ImpactAnalysisTestSequence(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy,
        Policy::DynamicDependencyMap dynamicDependencyMapPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
    {
        const Timer sequenceTimer;

        // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        const auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargets(changeList, testPrioritizationPolicy);

        // Set of selected tests so we can prune from drafted tests to avoid duplicate runs
        const AZStd::unordered_set<const PythonTestTarget*> selectedTestTargetSet(selectedTestTargets.begin(), selectedTestTargets.end());

        // Unlike native test impact analysis, python test impact analysis can have tests with no coverage so we cannot simply
        // draft in tests without coverage (i.e. new tests, or tests that have yet to successfully execute in previous runs).
        // Instead, the python test selector will run all parent test target tests when a new python test is added. What we
        // should do in future versions (for both native and python) is draft in any previous failing tests. For now, we will
        // leave the drafted set empty.
        AZStd::vector<const PythonTestTarget*> draftedTestTargets;

        // The subset of selected test targets that are not on the configuration's exclude list and those that are
        const auto [includedSelectedTestTargets, excludedSelectedTestTargets] =
            SelectTestTargetsByExcludeList(*m_testTargetExcludeList, selectedTestTargets);

        // Functor for running instrumented test targets
        const auto instrumentedTestRun = [&](const AZStd::vector<const PythonTestTarget*>& testsTargets,
                                             AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
        {
            return m_testEngine->InstrumentedRun(
                testsTargets,
                m_executionFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                globalTimeout);
        };

        if (dynamicDependencyMapPolicy == Policy::DynamicDependencyMap::Update)
        {
            AZStd::optional<AZStd::function<void(const AZStd::vector<TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>>& jobs)>>
                updateCoverage = [this](const AZStd::vector<TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>>& jobs)
            {
                m_hasImpactAnalysisData = UpdateAndSerializeDynamicDependencyMap(
                                              *m_dynamicDependencyMap.get(),
                                              jobs,
                                              m_failedTestCoveragePolicy,
                                              m_integrationFailurePolicy,
                                              m_config.m_commonConfig.m_repo.m_root,
                                              m_sparTiaFile)
                                              .value_or(m_hasImpactAnalysisData);
            };

            return ImpactAnalysisTestSequenceWrapper(
                1,
                GenerateImpactAnalysisSequencePolicyState(testPrioritizationPolicy, dynamicDependencyMapPolicy),
                m_suiteSet,
                m_suiteLabelExcludeSet,
                sequenceTimer,
                instrumentedTestRun,
                includedSelectedTestTargets,
                excludedSelectedTestTargets,
                discardedTestTargets,
                draftedTestTargets,
                testTargetTimeout,
                globalTimeout,
                updateCoverage);
        }
        else
        {
            return ImpactAnalysisTestSequenceWrapper(
                1,
                GenerateImpactAnalysisSequencePolicyState(testPrioritizationPolicy, dynamicDependencyMapPolicy),
                m_suiteSet,
                m_suiteLabelExcludeSet,
                sequenceTimer,
                instrumentedTestRun,
                includedSelectedTestTargets,
                excludedSelectedTestTargets,
                discardedTestTargets,
                draftedTestTargets,
                testTargetTimeout,
                globalTimeout,
                AZStd::optional<
                    AZStd::function<void(const AZStd::vector<TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>>& jobs)>>{
                    AZStd::nullopt });
        }
    }

    Client::SafeImpactAnalysisSequenceReport PythonRuntime::SafeImpactAnalysisTestSequence(
        const ChangeList& changeList,
        Policy::TestPrioritization testPrioritizationPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
    {
        const Timer sequenceTimer;
        TestRunData<TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>> selectedTestRunData, draftedTestRunData;
        TestRunData<TestEngineRegularRun<PythonTestTarget>> discardedTestRunData;
        AZStd::optional<AZStd::chrono::milliseconds> sequenceTimeout = globalTimeout;

        // Draft in the test targets that have no coverage entries in the dynamic dependency map
        AZStd::vector<const PythonTestTarget*> draftedTestTargets = m_dynamicDependencyMap->GetNotCoveringTests();

        // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        const auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargets(changeList, testPrioritizationPolicy);

        // The subset of selected test targets that are not on the configuration's exclude list and those that are
        const auto [includedSelectedTestTargets, excludedSelectedTestTargets] =
            SelectTestTargetsByExcludeList(*m_testTargetExcludeList, selectedTestTargets);

        // The subset of discarded test targets that are not on the configuration's exclude list and those that are
        const auto [includedDiscardedTestTargets, excludedDiscardedTestTargets] =
            SelectTestTargetsByExcludeList(*m_testTargetExcludeList, discardedTestTargets);

        // Extract the client facing representation of selected, discarded and drafted test targets
        const Client::TestRunSelection selectedTests(
            ExtractTestTargetNames(includedSelectedTestTargets), ExtractTestTargetNames(excludedSelectedTestTargets));
        const Client::TestRunSelection discardedTests(
            ExtractTestTargetNames(includedDiscardedTestTargets), ExtractTestTargetNames(excludedDiscardedTestTargets));
        const auto draftedTests = ExtractTestTargetNames(draftedTestTargets);

        // Inform the client that the sequence is about to start
        SafeImpactAnalysisTestSequenceNotificationBus::Broadcast(
            &SafeImpactAnalysisTestSequenceNotificationBus::Events::OnTestSequenceStart,
            m_suiteSet,
            m_suiteLabelExcludeSet,
            selectedTests,
            discardedTests,
            draftedTests);

        // We share the test run complete handler between the selected, discarded and drafted test runs as to present them together as one
        // continuous test sequence to the client rather than three discrete test runs
        const size_t totalNumTestRuns =
            includedSelectedTestTargets.size() + draftedTestTargets.size() + includedDiscardedTestTargets.size();

        // Functor for running instrumented test targets
        const auto instrumentedTestRun = [&](const AZStd::vector<const PythonTestTarget*>& testsTargets)
        {
            return m_testEngine->InstrumentedRun(
                testsTargets,
                m_executionFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                sequenceTimeout);
        };

        // Functor for running uninstrumented test targets
        const auto regularTestRun = [&](const AZStd::vector<const PythonTestTarget*>& testsTargets)
        {
            return m_testEngine->RegularRun(
                testsTargets,
                m_executionFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                sequenceTimeout);
        };

        // Functor for running instrumented test targets
        const auto gatherTestRunData =
            [&sequenceTimer](const AZStd::vector<const PythonTestTarget*>& testsTargets, const auto& testRunner, auto& testRunData)
        {
            const Timer testRunTimer;
            testRunData.m_relativeStartTime = testRunTimer.GetStartTimePointRelative(sequenceTimer);
            auto [result, jobs] = testRunner(testsTargets);
            testRunData.m_result = result;
            testRunData.m_jobs = AZStd::move(jobs);
            testRunData.m_duration = testRunTimer.GetElapsedMs();
        };

        TestEngineNotificationHandler<PythonTestTarget> testRunCompleteHandler(totalNumTestRuns);
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

        auto selectedTestRunReport = GenerateTestRunReport(
            selectedTestRunData.m_result,
            selectedTestRunData.m_relativeStartTime,
            selectedTestRunData.m_duration,
            selectedTestRunData.m_jobs);

        auto discardedTestRunReport = GenerateTestRunReport(
            discardedTestRunData.m_result,
            discardedTestRunData.m_relativeStartTime,
            discardedTestRunData.m_duration,
            discardedTestRunData.m_jobs);

        auto draftedTestRunReport = GenerateTestRunReport(
            draftedTestRunData.m_result,
            draftedTestRunData.m_relativeStartTime,
            draftedTestRunData.m_duration,
            draftedTestRunData.m_jobs);

        // Generate the sequence report for the client
        const auto sequenceReport = Client::SafeImpactAnalysisSequenceReport(
            1,
            testTargetTimeout,
            globalTimeout,
            GenerateSafeImpactAnalysisSequencePolicyState(testPrioritizationPolicy),
            m_suiteSet,
            m_suiteLabelExcludeSet,
            selectedTests,
            discardedTests,
            draftedTests,
            std::move(selectedTestRunReport),
            std::move(discardedTestRunReport),
            std::move(draftedTestRunReport));

        // Inform the client that the sequence has ended
        SafeImpactAnalysisTestSequenceNotificationBus::Broadcast(
            &SafeImpactAnalysisTestSequenceNotificationBus::Events::OnTestSequenceComplete, sequenceReport);

        m_hasImpactAnalysisData = UpdateAndSerializeDynamicDependencyMap(
            *m_dynamicDependencyMap.get(),
            ConcatenateVectors(selectedTestRunData.m_jobs, draftedTestRunData.m_jobs),
            m_failedTestCoveragePolicy,
            m_integrationFailurePolicy,
            m_config.m_commonConfig.m_repo.m_root,
            m_sparTiaFile)
            .value_or(m_hasImpactAnalysisData);

        return sequenceReport;
    }

    Client::SeedSequenceReport PythonRuntime::SeededTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
    {
        const Timer sequenceTimer;
        AZStd::vector<const PythonTestTarget*> includedTestTargets;
        AZStd::vector<const PythonTestTarget*> excludedTestTargets;

        // Separate the test targets into those that are excluded by either the test filter or exclusion list and those that are not
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
        {
            if (m_testTargetExcludeList->IsTestTargetFullyExcluded(&testTarget))
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
        SeedTestSequenceNotificationBus::Broadcast(
            &SeedTestSequenceNotificationBus::Events::OnTestSequenceStart, m_suiteSet, m_suiteLabelExcludeSet, selectedTests);

        // Run the test targets and collect the test run results
        TestEngineNotificationHandler<PythonTestTarget> handler(includedTestTargets.size());
        const Timer testRunTimer;
        const auto [result, testJobs] = m_testEngine->InstrumentedRun(
            includedTestTargets,
            m_executionFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout); // realtime std callback
        const auto testRunDuration = testRunTimer.GetElapsedMs();

        // Generate the sequence report for the client
        const auto sequenceReport = Client::SeedSequenceReport(
            1,
            testTargetTimeout,
            globalTimeout,
            GenerateSequencePolicyState(),
            m_suiteSet,
            m_suiteLabelExcludeSet,
            selectedTests,
            GenerateTestRunReport(result, testRunTimer.GetStartTimePointRelative(sequenceTimer), testRunDuration, testJobs));

        // Inform the client that the sequence has ended
        SeedTestSequenceNotificationBus::Broadcast(&SeedTestSequenceNotificationBus::Events::OnTestSequenceComplete, sequenceReport);

        ClearDynamicDependencyMapAndRemoveExistingFile();

        m_hasImpactAnalysisData = UpdateAndSerializeDynamicDependencyMap(
                                      *m_dynamicDependencyMap.get(),
                                      testJobs,
                                      m_failedTestCoveragePolicy,
                                      m_integrationFailurePolicy,
                                      m_config.m_commonConfig.m_repo.m_root,
                                      m_sparTiaFile)
                                      .value_or(m_hasImpactAnalysisData);

        return sequenceReport;
    }

    bool PythonRuntime::HasImpactAnalysisData() const
    {
        return m_hasImpactAnalysisData;
    }
} // namespace TestImpact
