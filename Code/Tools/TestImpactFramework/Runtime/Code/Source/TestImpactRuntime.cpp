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
    Runtime::Runtime(
        RuntimeConfig&& config,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::ExecutionFailureDrafting executionFailureDraftingPolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::IntegrityFailure integrationFailurePolicy,
        Policy::TestSharding testShardingPolicy,
        TargetOutputCapture targetOutputCapture,
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
        m_dynamicDependencyMap = ConstructDynamicDependencyMap(m_config.m_testTargetMeta, m_config.m_buildTargetDescriptor);

        m_testSelectorAndPrioritizer = AZStd::make_unique<TestSelectorAndPrioritizer>(m_dynamicDependencyMap.get(), DependencyGraphDataMap{});

        // Construct the target exclude list from the target configuration data
        m_testTargetExcludeList = ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetTestTargetList(), m_config.m_target);

        // Enumerate the test targets that have potentially changed since the last run
        //EnumerateDirtyTestTargets();

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

    TestSequenceResult Runtime::RegularTestSequence(
        const AZStd::unordered_set<AZStd::string> suitesFilter,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback> testSequenceEndCallback,
        AZStd::optional<TestCompleteCallback> testCompleteCallback)
    {
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;
        const AZStd::chrono::high_resolution_clock::time_point startTime = AZStd::chrono::high_resolution_clock::now();

        for (const auto& testTarget : m_dynamicDependencyMap->GetTestTargetList().GetTargets())
        {
            if (!m_testTargetExcludeList.contains(&testTarget))
            {
                if (suitesFilter.empty())
                {
                    includedTestTargets.push_back(&testTarget);
                }
                else if(suitesFilter.contains(testTarget.GetSuite()))
                {
                    includedTestTargets.push_back(&testTarget);
                }
                else
                {
                    excludedTestTargets.push_back(&testTarget);
                }
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

        const auto testComplete = [testCompleteCallback](const TestEngineJob& testJob)
        {
            if (testCompleteCallback.has_value())
            {
                (*testCompleteCallback)(Client::TestRun(testJob.GetTestTarget()->GetName(), testJob.GetTestResult(), testJob.GetDuration()));
            }
        };

        const auto [result, testJobs] = m_testEngine->RegularRun(
            includedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            testComplete);

        const AZStd::chrono::high_resolution_clock::time_point endTime = AZStd::chrono::high_resolution_clock::now();
        const auto duration = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(endTime - startTime);

        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(CreateRegularFailureReport(testJobs), duration);
        }

        return result;
    }

    TestSequenceResult Runtime::ImpactAnalysisTestSequence(
        [[maybe_unused]]const ChangeList& changeList,
        [[maybe_unused]]Policy::TestPrioritization testPrioritizationPolicy,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]]AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]]AZStd::optional<ImpactAnalysisTestSequenceCompleteCallback> testSequenceEndCallback,
        [[maybe_unused]]AZStd::optional<TestCompleteCallback> testCompleteCallback)
    {
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;
        AZStd::vector<const TestTarget*> discardedTests;
        AZStd::vector<const TestTarget*> draftedTests;
        AZStd::unordered_set<const TestTarget*> selectedTests;
        const AZStd::chrono::high_resolution_clock::time_point startTime = AZStd::chrono::high_resolution_clock::now();
        const auto changeDependecyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList);
        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependecyList, testPrioritizationPolicy);

        for (const auto* testTarget : selectedTestTargets)
        {
            selectedTests.insert(testTarget);
            if (!m_testTargetExcludeList.contains(testTarget))
            {
                includedTestTargets.push_back(testTarget);
            }
            else
            {
                excludedTestTargets.push_back(testTarget);
            }
        }

        for (const auto& testTarget : m_dynamicDependencyMap->GetTestTargetList().GetTargets())
        {
            if (!selectedTests.contains(&testTarget))
            {
                discardedTests.push_back(&testTarget);
            }
        }

        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(
                Client::TestRunSelection(ExtractTestTargetNames(includedTestTargets), ExtractTestTargetNames(excludedTestTargets)),
                ExtractTestTargetNames(discardedTests),
                ExtractTestTargetNames(draftedTests));
        }

        const auto testComplete = [testCompleteCallback](const TestEngineJob& testJob)
        {
            if (testCompleteCallback.has_value())
            {
                (*testCompleteCallback)(Client::TestRun(testJob.GetTestTarget()->GetName(), testJob.GetTestResult(), testJob.GetDuration()));
            }
        };

        const auto [result, testJobs] = m_testEngine->InstrumentedRun(
            includedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            Policy::IntegrityFailure::Continue,
            m_testFailurePolicy,
            m_targetOutputCapture,
            AZStd::nullopt,
            globalTimeout,
            testComplete);

        const auto coverage = CreateSourceCoveringTestFromTestCoverages(testJobs, m_config.m_repo.m_root);
        m_dynamicDependencyMap->ReplaceSourceCoverage(coverage);
        const auto sparTIA = m_dynamicDependencyMap->ExportSourceCoverage();
        const auto sparTIAData = SerializeSourceCoveringTestsList(sparTIA);
        WriteFileContents<RuntimeException>(sparTIAData, m_config.m_workspace.m_active.m_relativePaths.m_sparTIAFile);
        m_hasImpactAnalysisData = true;

        const AZStd::chrono::high_resolution_clock::time_point endTime = AZStd::chrono::high_resolution_clock::now();
        const auto duration = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(endTime - startTime);
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(CreateTestImpactFailureReport(includedTestTargets, excludedTestTargets, discardedTests, draftedTests), duration);
        }

        return result;
    }

    TestSequenceResult Runtime::SafeImpactAnalysisTestSequence(
        [[maybe_unused]]const ChangeList& changeList,
        [[maybe_unused]]const AZStd::unordered_set<AZStd::string> suitesFilter,
        [[maybe_unused]]Policy::TestPrioritization testPrioritizationPolicy,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]]AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]]AZStd::optional<ImpactAnalysisTestSequenceCompleteCallback> testSequenceEndCallback,
        [[maybe_unused]]AZStd::optional<TestCompleteCallback> testCompleteCallback)
    {
        return TestSequenceResult::Success;
    }

    TestSequenceResult Runtime::SeededTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback> testSequenceEndCallback,
        AZStd::optional<TestCompleteCallback> testCompleteCallback)
    {        
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;
        const AZStd::chrono::high_resolution_clock::time_point startTime = AZStd::chrono::high_resolution_clock::now();

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

        const auto testComplete = [testCompleteCallback](const TestEngineJob& testJob)
        {
            if (testCompleteCallback.has_value())
            {
                (*testCompleteCallback)(Client::TestRun(testJob.GetTestTarget()->GetName(), testJob.GetTestResult(), testJob.GetDuration()));
            }
        };

        const auto [result, testJobs] = m_testEngine->InstrumentedRun(
            includedTestTargets,
            m_testShardingPolicy,
            m_executionFailurePolicy,
            Policy::IntegrityFailure::Continue,
            m_testFailurePolicy,
            m_targetOutputCapture,
            AZStd::nullopt,
            globalTimeout,
            testComplete);

        const auto coverage = CreateSourceCoveringTestFromTestCoverages(testJobs, m_config.m_repo.m_root);
        m_dynamicDependencyMap->ReplaceSourceCoverage(coverage);
        const auto sparTIA = m_dynamicDependencyMap->ExportSourceCoverage();
        const auto sparTIAData = SerializeSourceCoveringTestsList(sparTIA);
        WriteFileContents<RuntimeException>(sparTIAData, m_config.m_workspace.m_active.m_relativePaths.m_sparTIAFile);
        m_hasImpactAnalysisData = true;

        const AZStd::chrono::high_resolution_clock::time_point endTime = AZStd::chrono::high_resolution_clock::now();
        const auto duration = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(endTime - startTime);
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(CreateRegularFailureReport(testJobs), duration);
        }

        return result;
    }

    bool Runtime::HasImpactAnalysisData() const
    {
        return m_hasImpactAnalysisData;
    }
}
