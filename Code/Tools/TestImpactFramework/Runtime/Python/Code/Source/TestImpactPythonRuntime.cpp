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
#include <Dependency/TestImpactTestSelectorAndPrioritizer.h>
#include <Target/Python/TestImpactPythonProductionTarget.h>
#include <Target/Python/TestImpactPythonTargetListCompiler.h>
#include <Target/Python/TestImpactPythonTestTarget.h>
#include <TestEngine/Python/TestImpactPythonTestEngine.h>

#include <AzCore/std/string/regex.h>

namespace TestImpact
{
    PythonTestTargetMetaMap ReadPythonTestTargetMetaMapFile(SuiteType suiteFilter, const RepoPath& testTargetMetaConfigFile, const AZStd::string& buildType)
    {
        const auto masterTestListData = ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        auto testTargetMetaMap = PythonTestTargetMetaMapFactory(masterTestListData, suiteFilter);
        for (auto& [name, meta] : testTargetMetaMap)
        {
            meta.m_testCommand = AZStd::regex_replace(meta.m_testCommand, AZStd::regex("\\$\\<CONFIG\\>"), buildType); 
        }

        return testTargetMetaMap;
    }

    PythonRuntime::PythonRuntime(
        PythonRuntimeConfig&& config,
        [[maybe_unused]] const AZStd::optional<RepoPath>& dataFile,
        [[maybe_unused]] const AZStd::optional<RepoPath>& previousRunDataFile,
        [[maybe_unused]] const AZStd::vector<ExcludedTarget>& testsToExclude,
        [[maybe_unused]] SuiteType suiteFilter,
        [[maybe_unused]] Policy::ExecutionFailure executionFailurePolicy,
        [[maybe_unused]] Policy::FailedTestCoverage failedTestCoveragePolicy,
        [[maybe_unused]] Policy::TestFailure testFailurePolicy,
        [[maybe_unused]] Policy::IntegrityFailure integrationFailurePolicy,
        [[maybe_unused]] Policy::TargetOutputCapture targetOutputCapture)
        : m_config(AZStd::move(config))
        , m_suiteFilter(suiteFilter)
        , m_executionFailurePolicy(executionFailurePolicy)
        , m_failedTestCoveragePolicy(failedTestCoveragePolicy)
        , m_testFailurePolicy(testFailurePolicy)
        , m_integrationFailurePolicy(integrationFailurePolicy)
        , m_targetOutputCapture(targetOutputCapture)
    {
        // Construct the build targets from the build target descriptors
        auto targetDescriptors = ReadTargetDescriptorFiles(m_config.m_commonConfig.m_buildTargetDescriptor);
        auto buildTargets = CompilePythonTargetLists(
            AZStd::move(targetDescriptors),
            ReadPythonTestTargetMetaMapFile(suiteFilter, m_config.m_commonConfig.m_testTargetMeta.m_metaFile, m_config.m_commonConfig.m_meta.m_buildConfig));
        auto&& [productionTargets, testTargets] = buildTargets;
        m_buildTargets = AZStd::make_unique<BuildTargetList<PythonProductionTarget, PythonTestTarget>>(
            AZStd::move(testTargets), AZStd::move(productionTargets));

        // Construct the dynamic dependency map from the build targets
        m_dynamicDependencyMap = AZStd::make_unique<DynamicDependencyMap<PythonProductionTarget, PythonTestTarget>>(m_buildTargets.get());

        // Construct the test selector and prioritizer from the dependency graph data (NOTE: currently not implemented)
        m_testSelectorAndPrioritizer = AZStd::make_unique<TestSelectorAndPrioritizer<PythonProductionTarget, PythonTestTarget>>(
            m_dynamicDependencyMap.get(), DependencyGraphDataMap{});

        // Construct the target exclude list from the exclude file if provided, otherwise use target configuration data
        if (!testsToExclude.empty())
        {
            // Construct using data from excludeTestFile
            m_testTargetExcludeList =
                ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList(), testsToExclude);
        }
        else
        {
            // Construct using data from config file.
            m_testTargetExcludeList = ConstructTestTargetExcludeList(
                m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList(), m_config.m_target.m_excludedTargets);
        }

        // Construct the test engine with the workspace path and launcher binaries
        m_testEngine = AZStd::make_unique<PythonTestEngine>(
            m_config.m_commonConfig.m_repo.m_root,
            m_config.m_testEngine.m_testRunner.m_pythonCmd,
            m_config.m_commonConfig.m_repo.m_build,
            m_config.m_commonConfig.m_workspace.m_temp.m_artifactDirectory);
    }

    PythonRuntime::~PythonRuntime() = default;

    bool PythonRuntime::HasImpactAnalysisData() const
    {
        return m_hasImpactAnalysisData;
    }
} // namespace TestImpact
