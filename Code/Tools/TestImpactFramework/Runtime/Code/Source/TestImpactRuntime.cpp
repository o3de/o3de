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

#include <Artifact/Factory/TestImpactTestTargetMetaMapFactory.h>
#include <Artifact/Factory/TestImpactBuildTargetDescriptorFactory.h>
#include <Artifact/Static/TestImpactTargetDescriptorCompiler.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <TestEngine/TestImpactTestEngine.h>

#include <AzCore/IO/SystemFile.h>

#include <filesystem>

namespace TestImpact
{
    namespace
    {
        TestTargetMetaMap ReadTestTargetMetaMapFile(const TestTargetMetaConfig& testTargetMetaConfig)
        {
            const auto masterTestListData = ReadFileContents<RuntimeException>(testTargetMetaConfig.m_file);
            return TestTargetMetaMapFactory(masterTestListData);
        }

        AZStd::vector<TestImpact::BuildTargetDescriptor> ReadBuildTargetDescriptorFiles(const BuildTargetDescriptorConfig& buildTargetDescriptorConfig)
        {
            AZStd::vector<TestImpact::BuildTargetDescriptor> buildTargetDescriptors;
            for (const auto& buildTargetDescriptorFile : std::filesystem::directory_iterator(buildTargetDescriptorConfig.m_outputDirectory.c_str()))
            {
                const auto buildTargetDescriptorContents = ReadFileContents<RuntimeException>(buildTargetDescriptorFile.path().string().c_str());
                auto buildTargetDescriptor = TestImpact::BuildTargetDescriptorFactory(
                    buildTargetDescriptorContents,
                    buildTargetDescriptorConfig.m_staticInclusionFilters,
                    buildTargetDescriptorConfig.m_inputInclusionFilters,
                    buildTargetDescriptorConfig.m_inputOutputPairer);
                buildTargetDescriptors.emplace_back(AZStd::move(buildTargetDescriptor));
            }

            return buildTargetDescriptors;
        }
    }

    AZStd::unique_ptr<TestImpact::DynamicDependencyMap> ConstructDynamicDependencyMap(
        const TestTargetMetaConfig& testTargetMetaConfig,
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig)
    {
        auto testTargetmetaMap = ReadTestTargetMetaMapFile(testTargetMetaConfig);
        auto buildTargetDescriptors = ReadBuildTargetDescriptorFiles(buildTargetDescriptorConfig);
        auto buildTargets = TestImpact::CompileTargetDescriptors(AZStd::move(buildTargetDescriptors), AZStd::move(testTargetmetaMap));
        auto&& [productionTargets, testTargets] = buildTargets;
        return AZStd::make_unique<TestImpact::DynamicDependencyMap>(AZStd::move(productionTargets), AZStd::move(testTargets));
    }

    AZStd::unordered_set<const TestTarget*> ConstructTestTargetExcludeList(const TestTargetList& testTargets, const TargetConfig& targetConfig)
    {
        AZStd::unordered_set<const TestTarget*> testTargetExcludeList;
        for (const auto& testTargetName : targetConfig.m_excludedTestTargets)
        {
            if (const auto* testTarget = testTargets.GetTarget(testTargetName); testTarget != nullptr)
            {
                testTargetExcludeList.insert(testTarget);
            }
        }

        return testTargetExcludeList;
    }

    TestSelection CreateTestSelection(
        const AZStd::vector<const TestTarget*>& includedTestTargets,
        const AZStd::vector<const TestTarget*>& excludedTestTargets)
    {
        const auto populateTestTargetNames = [](const AZStd::vector<const TestTarget*> testTargets)
        {
            AZStd::vector<AZStd::string> testNames;
            AZStd::transform(testTargets.begin(), testTargets.end(), AZStd::back_inserter(testNames), [](const TestTarget* testTarget)
            {
                return testTarget->GetName();
            });

            return testNames;
        };
        
        return TestSelection(populateTestTargetNames(includedTestTargets), populateTestTargetNames(excludedTestTargets));
    }

    //template<typename TestJob>
    FailureReport CreateFailureReport(const AZStd::vector</*TestJob*/TestEngineRegularRun> testJobs)
    {
        AZStd::vector<ExecutionFailure> executionFailures;
        AZStd::vector<LauncherFailure> launcherFailures;
        AZStd::vector<TestRunFailure> testRunFailures;
        AZStd::vector<TargetFailure> unexecutedTestRsuns;

        //for (const auto& testJob : testJobs)
        //{
        //    //switch (testJob.GetResult())
        //    //{
        //    //case JobResult::FailedToExecute:
        //    //{
        //    //    executionFailures.push_back()
        //    //}
        //    //}
        //}

        return FailureReport(AZStd::move(executionFailures), AZStd::move(launcherFailures), AZStd::move(testRunFailures), AZStd::move(unexecutedTestRsuns));
    }

    Runtime::Runtime(
        RuntimeConfig&& config,
        ExecutionFailurePolicy executionFailurePolicy,
        ExecutionFailureDraftingPolicy executionFailureDraftingPolicy,
        TestFailurePolicy testFailurePolicy,
        IntegrityFailurePolicy integrationFailurePolicy,
        TestShardingPolicy testShardingPolicy,
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
        m_dynamicDependencyMap = ConstructDynamicDependencyMap(m_config.m_testTargetMeta, m_config.m_buildTargetDescriptor);
        m_testTargetExcludeList = ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetTestTargetList(), m_config.m_target);
        //EnumerateDirtyTestTargets();
        m_testEngine = AZStd::make_unique<TestEngine>(
            m_config.m_repo.m_root,
            m_config.m_target.m_outputDirectory,
            m_config.m_workspace.m_persistent.m_enumerationCacheDirectory,
            m_config.m_workspace.m_temp.m_artifactDirectory,
            m_config.m_testEngine.m_testRunner.m_binary,
            m_config.m_testEngine.m_instrumentation.m_binary,
            m_maxConcurrency);
    }

    Runtime::~Runtime() = default;

    //void EnumerateDirtyTestTargets()
    //{
    //    AZStd::vector<const TestTarget*> dirtyTargets;
    //    // TODO: cross reference with list of previously enabled targets
    //    m_testEngine->UpdateEnumerationCache(dirtyTargets, AZStd::nullopt);
    //}

    TestSequenceResult Runtime::RegularTestSequence(
        const AZStd::unordered_set<AZStd::string> suitesFilter,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
        AZStd::optional<TestCompleteCallback> testCompleteCallback)
    {
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;

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
            (*testSequenceStartCallback)(CreateTestSelection(includedTestTargets, excludedTestTargets));
        }

        const auto testComplete = [testCompleteCallback](const TestEngineJob& testJob, TestResult testResult)
        {
            if (testCompleteCallback.has_value())
            {
                (*testCompleteCallback)(Test(testJob.GetTestTarget()->GetName(), testResult, testJob.GetDuration()));
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

        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(CreateFailureReport(testJobs));
        }

        return result;
    }

    TestSequenceResult Runtime::ImpactAnalysisTestSequence(
        [[maybe_unused]]const ChangeList& changeList,
        [[maybe_unused]]TestPrioritizationPolicy testPrioritizationPolicy,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]]AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]]AZStd::optional<ImpactAnalysisTestSequenceEndCallback> testSequenceEndCallback,
        [[maybe_unused]]AZStd::optional<TestCompleteCallback> testCompleteCallback)
    {
        return TestSequenceResult::Success;
    }

    TestSequenceResult Runtime::SafeImpactAnalysisTestSequence(
        [[maybe_unused]]const ChangeList& changeList,
        [[maybe_unused]] const AZStd::unordered_set<AZStd::string> suitesFilter,
        [[maybe_unused]]TestPrioritizationPolicy testPrioritizationPolicy,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]]AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]]AZStd::optional<ImpactAnalysisTestSequenceEndCallback> testSequenceEndCallback,
        [[maybe_unused]]AZStd::optional<TestCompleteCallback> testCompleteCallback)
    {
        return TestSequenceResult::Success;
    }

    TestSequenceResult Runtime::SeededTestSequence(
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]]AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]]AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
        [[maybe_unused]]AZStd::optional<TestCompleteCallback> testCompleteCallback)
    {        
        return TestSequenceResult::Success;
    }

    bool Runtime::HasImpactAnalysisData() const
    {
        return false;
    }
}
