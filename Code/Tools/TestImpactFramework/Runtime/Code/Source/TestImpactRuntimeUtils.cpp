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
    TestTargetMetaMap ReadTestTargetMetaMapFile(SuiteType suiteFilter, const RepoPath& testTargetMetaConfigFile)
    {
        const auto masterTestListData = ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        return TestTargetMetaMapFactory(masterTestListData, suiteFilter);
    }

    AZStd::vector<BuildTargetDescriptor> ReadBuildTargetDescriptorFiles(const BuildTargetDescriptorConfig& buildTargetDescriptorConfig)
    {
        AZStd::vector<BuildTargetDescriptor> buildTargetDescriptors;
        for (const auto& buildTargetDescriptorFile : std::filesystem::directory_iterator(buildTargetDescriptorConfig.m_mappingDirectory.c_str()))
        {
            const auto buildTargetDescriptorContents = ReadFileContents<RuntimeException>(buildTargetDescriptorFile.path().string().c_str());
            auto buildTargetDescriptor = BuildTargetDescriptorFactory(
                buildTargetDescriptorContents,
                buildTargetDescriptorConfig.m_staticInclusionFilters,
                buildTargetDescriptorConfig.m_inputInclusionFilters,
                buildTargetDescriptorConfig.m_inputOutputPairer);
            buildTargetDescriptors.emplace_back(AZStd::move(buildTargetDescriptor));
        }

        return buildTargetDescriptors;
    }

    AZStd::unique_ptr<DynamicDependencyMap> ConstructDynamicDependencyMap(
        SuiteType suiteFilter,
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig,
        const TestTargetMetaConfig& testTargetMetaConfig)
    {
        auto testTargetmetaMap = ReadTestTargetMetaMapFile(suiteFilter, testTargetMetaConfig.m_metaFile);
        auto buildTargetDescriptors = ReadBuildTargetDescriptorFiles(buildTargetDescriptorConfig);
        auto buildTargets = CompileTargetDescriptors(AZStd::move(buildTargetDescriptors), AZStd::move(testTargetmetaMap));
        auto&& [productionTargets, testTargets] = buildTargets;
        return AZStd::make_unique<DynamicDependencyMap>(AZStd::move(productionTargets), AZStd::move(testTargets));
    }

    AZStd::unique_ptr<TestTargetExclusionList> ConstructTestTargetExcludeList(
        const TestTargetList& testTargets, AZStd::vector<TargetConfig::ExcludedTarget>&& excludedTestTargets)
    {
        AZStd::unordered_map<const TestTarget*, AZStd::vector<AZStd::string>> testTargetExcludeList;
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

    AZStd::pair<AZStd::vector<const TestTarget*>, AZStd::vector<const TestTarget*>> SelectTestTargetsByExcludeList(
    const TestTargetExclusionList& testTargetExcludeList, const AZStd::vector<const TestTarget*>& testTargets)
    {
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;

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

    AZStd::vector<AZStd::string> ExtractTestTargetNames(const AZStd::vector<const TestTarget*>& testTargets)
    {
        AZStd::vector<AZStd::string> testNames;
        AZStd::transform(testTargets.begin(), testTargets.end(), AZStd::back_inserter(testNames), [](const TestTarget* testTarget)
        {
            return testTarget->GetName();
        });

        return testNames;
    }
} // namespace TestImpact
