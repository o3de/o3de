/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/TestImpactRuntimeException.h>

#include <TestImpactNativeRuntimeUtils.h>
#include <Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.h>
#include <Artifact/Factory/TestImpactNativeTargetDescriptorFactory.h>
#include <Artifact/Static/TestImpactNativeTargetDescriptorCompiler.h>
#include <Target/Native/TestImpactNativeTestTarget.h>
#include <Target/Native/TestImpactNativeProductionTarget.h>

#include <filesystem>

namespace TestImpact
{
    NativeTestTargetMetaMap ReadNativeTestTargetMetaMapFile(SuiteType suiteFilter, const RepoPath& testTargetMetaConfigFile)
    {
        const auto masterTestListData = ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        return NativeTestTargetMetaMapFactory(masterTestListData, suiteFilter);
    }

    AZStd::vector<NativeTargetDescriptor> ReadNativeTargetDescriptorFiles(const BuildTargetDescriptorConfig& buildTargetDescriptorConfig)
    {
        AZStd::vector<NativeTargetDescriptor> NativeTargetDescriptors;
        for (const auto& NativeTargetDescriptorFile :
             std::filesystem::directory_iterator(buildTargetDescriptorConfig.m_mappingDirectory.c_str()))
        {
            const auto NativeTargetDescriptorContents = ReadFileContents<RuntimeException>(NativeTargetDescriptorFile.path().string().c_str());
            auto NativeTargetDescriptor = NativeTargetDescriptorFactory(
                NativeTargetDescriptorContents,
                buildTargetDescriptorConfig.m_staticInclusionFilters,
                buildTargetDescriptorConfig.m_inputInclusionFilters,
                buildTargetDescriptorConfig.m_inputOutputPairer);
            NativeTargetDescriptors.emplace_back(AZStd::move(NativeTargetDescriptor));
        }

        return NativeTargetDescriptors;
    }

    AZStd::unique_ptr<BuildTargetList<NativeTestTarget, NativeProductionTarget>> ConstructNativeBuildTargetList(
        SuiteType suiteFilter,
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig,
        const TestTargetMetaConfig& testTargetMetaConfig)
    {
        auto NativeTestTargetMetaMap = ReadNativeTestTargetMetaMapFile(suiteFilter, testTargetMetaConfig.m_metaFile);
        auto NativeTargetDescriptors = ReadNativeTargetDescriptorFiles(buildTargetDescriptorConfig);
        auto buildTargets = CompileTargetDescriptors(AZStd::move(NativeTargetDescriptors), AZStd::move(NativeTestTargetMetaMap));
        auto&& [productionTargets, testTargets] = buildTargets;
        return AZStd::make_unique<BuildTargetList<NativeTestTarget, NativeProductionTarget>>(
            AZStd::move(testTargets), AZStd::move(productionTargets));
    }

    AZStd::unique_ptr<TestTargetExclusionList<NativeTestTarget>> ConstructTestTargetExcludeList(
        const TargetList<NativeTestTarget>& testTargets, const AZStd::vector<ExcludedTarget>& excludedTestTargets)
    {
        AZStd::unordered_map<const NativeTestTarget*, AZStd::vector<AZStd::string>> testTargetExcludeList;
        for (const auto& excludedTestTarget : excludedTestTargets)
        {
            if (const auto* testTarget = testTargets.GetTarget(excludedTestTarget.m_name);
                testTarget != nullptr)
            {
                testTargetExcludeList[testTarget] = excludedTestTarget.m_excludedTests;
            }
        }

        return AZStd::make_unique<TestTargetExclusionList<NativeTestTarget>>(AZStd::move(testTargetExcludeList));
    }

    AZStd::pair<
        AZStd::vector<const NativeTestTarget*>,
        AZStd::vector<const NativeTestTarget*>>
    SelectTestTargetsByExcludeList(
        const TestTargetExclusionList<NativeTestTarget>& testTargetExcludeList,
        const AZStd::vector<const NativeTestTarget*>& testTargets)
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

    AZStd::vector<AZStd::string> ExtractTestTargetNames(
        const AZStd::vector<const NativeTestTarget*>& testTargets)
    {
        AZStd::vector<AZStd::string> testNames;
        AZStd::transform(
            testTargets.begin(), testTargets.end(), AZStd::back_inserter(testNames),
            [](const NativeTestTarget* testTarget)
        {
            return testTarget->GetName();
        });

        return testNames;
    }
} // namespace TestImpact
