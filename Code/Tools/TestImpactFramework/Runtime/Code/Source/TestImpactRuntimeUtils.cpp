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
#include <Artifact/Factory/TestImpactTestTargetMetaMapFactory.h>
#include <Artifact/Factory/TestImpactBuildTargetDescriptorFactory.h>
#include <Artifact/Static/TestImpactTargetDescriptorCompiler.h>

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

    AZStd::unordered_set<const TestTarget*> ConstructTestTargetExcludeList(
        const TestTargetList& testTargets, const AZStd::vector<AZStd::string>& excludedTestTargets)
    {
        AZStd::unordered_set<const TestTarget*> testTargetExcludeList;
        for (const auto& testTargetName : excludedTestTargets)
        {
            if (const auto* testTarget = testTargets.GetTarget(testTargetName); testTarget != nullptr)
            {
                testTargetExcludeList.insert(testTarget);
            }
        }

        return testTargetExcludeList;
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
