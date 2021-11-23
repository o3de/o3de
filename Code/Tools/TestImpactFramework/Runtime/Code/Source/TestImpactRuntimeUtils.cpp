/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/TestImpactRuntimeException.h>

#include <TestImpactRuntimeUtils.h>
#include <Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.h>
#include <Artifact/Factory/TestImpactNativeTargetDescriptorFactory.h>
#include <Artifact/Static/TestImpactNativeTargetDescriptorCompiler.h>

#include <filesystem>

namespace TestImpact
{
    NativeTestTargetMetaMap ReadNativeTestTargetMetaMapFile(SuiteType suiteFilter, const RepoPath& testTargetMetaConfigFile)
    {
        const auto masterTestListData = ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        return NativeTestTargetMetaMapFactory(masterTestListData, suiteFilter);
    }

    AZStd::vector<NativeTargetDescriptor> ReadNativeTargetDescriptorFiles(const NativeTargetDescriptorConfig& NativeTargetDescriptorConfig)
    {
        AZStd::vector<NativeTargetDescriptor> NativeTargetDescriptors;
        for (const auto& NativeTargetDescriptorFile : std::filesystem::directory_iterator(NativeTargetDescriptorConfig.m_mappingDirectory.c_str()))
        {
            const auto NativeTargetDescriptorContents = ReadFileContents<RuntimeException>(NativeTargetDescriptorFile.path().string().c_str());
            auto NativeTargetDescriptor = NativeTargetDescriptorFactory(
                NativeTargetDescriptorContents,
                NativeTargetDescriptorConfig.m_staticInclusionFilters,
                NativeTargetDescriptorConfig.m_inputInclusionFilters,
                NativeTargetDescriptorConfig.m_inputOutputPairer);
            NativeTargetDescriptors.emplace_back(AZStd::move(NativeTargetDescriptor));
        }

        return NativeTargetDescriptors;
    }

    AZStd::unique_ptr<DynamicDependencyMap> ConstructDynamicDependencyMap(
        SuiteType suiteFilter,
        const NativeTargetDescriptorConfig& NativeTargetDescriptorConfig,
        const TestTargetMetaConfig& testTargetMetaConfig)
    {
        auto NativeTestTargetMetaMap = ReadNativeTestTargetMetaMapFile(suiteFilter, testTargetMetaConfig.m_metaFile);
        auto NativeTargetDescriptors = ReadNativeTargetDescriptorFiles(NativeTargetDescriptorConfig);
        auto buildTargets = CompileTargetDescriptors(AZStd::move(NativeTargetDescriptors), AZStd::move(NativeTestTargetMetaMap));
        auto&& [productionTargets, testTargets] = buildTargets;
        return AZStd::make_unique<DynamicDependencyMap>(AZStd::move(productionTargets), AZStd::move(testTargets));
    }

    AZStd::unique_ptr<TestTargetExclusionList> ConstructTestTargetExcludeList(
        const NativeTestTargetList& testTargets, AZStd::vector<TargetConfig::ExcludedTarget>&& excludedTestTargets)
    {
        AZStd::unordered_map<const NativeTestTarget*, AZStd::vector<AZStd::string>> testTargetExcludeList;
        for (const auto& excludedTestTarget : excludedTestTargets)
        {
            if (const auto* testTarget = testTargets.GetTarget(excludedTestTarget.m_name);
                testTarget != nullptr)
            {
                testTargetExcludeList[testTarget] = AZStd::move(excludedTestTarget.m_excludedTests);
            }
        }

        return AZStd::make_unique<TestTargetExclusionList>(AZStd::move(testTargetExcludeList));
    }

    AZStd::pair<AZStd::vector<const NativeTestTarget*>, AZStd::vector<const NativeTestTarget*>> SelectTestTargetsByExcludeList(
    const TestTargetExclusionList& testTargetExcludeList, const AZStd::vector<const NativeTestTarget*>& testTargets)
    {
        AZStd::vector<const NativeTestTarget*> includedTestTargets;
        AZStd::vector<const NativeTestTarget*> excludedTestTargets;

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

    AZStd::vector<AZStd::string> ExtractTestTargetNames(const AZStd::vector<const NativeTestTarget*>& testTargets)
    {
        AZStd::vector<AZStd::string> testNames;
        AZStd::transform(testTargets.begin(), testTargets.end(), AZStd::back_inserter(testNames), [](const NativeTestTarget* testTarget)
        {
            return testTarget->GetName();
        });

        return testNames;
    }
} // namespace TestImpact
