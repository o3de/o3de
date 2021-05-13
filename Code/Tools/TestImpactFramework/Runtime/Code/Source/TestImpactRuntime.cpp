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

    Runtime::Runtime(
        RuntimeConfig&& config,
        ExecutionFailurePolicy executionFailurePolicy,
        ExecutionFailureDraftingPolicy executionFailureDraftingPolicy,
        TestFailurePolicy testFailurePolicy,
        TestShardingPolicy testShardingPolicy,
        TargetOutputCapture targetOutputCapture,
        AZStd::optional<size_t> maxConcurrency)
        : m_config(AZStd::move(config))
        , m_executionFailurePolicy(executionFailurePolicy)
        , m_executionFailureDraftingPolicy(executionFailureDraftingPolicy)
        , m_testFailurePolicy(testFailurePolicy)
        , m_testShardingPolicy(testShardingPolicy)
        , m_targetOutputCapture(targetOutputCapture)
        , m_maxConcurrency(maxConcurrency.value_or(AZStd::thread::hardware_concurrency()))
    {
        auto testTargetmetaMap = ReadTestTargetMetaMapFile(m_config.m_testTargetMeta);
        auto buildTargetDescriptors = ReadBuildTargetDescriptorFiles(m_config.m_buildTargetDescriptor);
        auto buildTargets = TestImpact::CompileTargetDescriptors(AZStd::move(buildTargetDescriptors), AZStd::move(testTargetmetaMap));
        auto&& [productionTargets, testTargets] = buildTargets;
        m_dynamicDependencyMap = AZStd::make_unique<TestImpact::DynamicDependencyMap>(AZStd::move(productionTargets), AZStd::move(testTargets));
    }

    Runtime::~Runtime() = default;

    TestSequenceResult Runtime::RegularTestSequence(
        [[maybe_unused]] const AZStd::unordered_set<AZStd::string> suitesFilter,
        [[maybe_unused]]AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]]AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
        [[maybe_unused]]AZStd::optional<TestRunEndCallback> testRunEndCallback,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
    {
        return TestSequenceResult::Success;
    }

    TestSequenceResult Runtime::ImpactAnalysisTestSequence(
        [[maybe_unused]]const ChangeList& changeList,
        [[maybe_unused]]TestPrioritizationPolicy testPrioritizationPolicy,
        [[maybe_unused]]AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]]AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
        [[maybe_unused]]AZStd::optional<TestRunEndCallback> testRunEndCallback,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
    {
        return TestSequenceResult::Success;
    }

    TestSequenceResult Runtime::SafeImpactAnalysisTestSequence(
        [[maybe_unused]]const ChangeList& changeList,
        [[maybe_unused]] const AZStd::unordered_set<AZStd::string> suitesFilter,
        [[maybe_unused]]TestPrioritizationPolicy testPrioritizationPolicy,
        [[maybe_unused]]AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]]AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
        [[maybe_unused]]AZStd::optional<TestRunEndCallback> testRunEndCallback,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
    {
        return TestSequenceResult::Success;
    }

    TestSequenceResult Runtime::SeededTestSequence(
        [[maybe_unused]]AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]]AZStd::optional<TestSequenceEndCallback> testSequenceEndCallback,
        [[maybe_unused]]AZStd::optional<TestRunEndCallback> testRunEndCallback,
        [[maybe_unused]]AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
    {
        if (m_testShardingPolicy == TestShardingPolicy::Always)
        {
            // enum all test targets
        }

        return TestSequenceResult::Success;
    }

    bool Runtime::HasImpactAnalysisData() const
    {
        return false;
    }
}
