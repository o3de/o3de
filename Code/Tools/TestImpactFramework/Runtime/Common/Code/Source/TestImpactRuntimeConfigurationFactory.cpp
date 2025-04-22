/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <TestImpactRuntimeConfigurationFactory.h>

#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>

namespace TestImpact
{
    namespace RuntimeConfigFactory
    {
        // Keys for pertinent JSON elements
        constexpr const char* Keys[] =
        {
            "common",
            "root",
            "build",
            "platform",
            "build_config",
            "relative_paths",
            "run_artifact_dir",
            "coverage_artifact_dir",
            "enumeration_cache_dir",
            "test_impact_data_file",
            "temp",
            "active",
            "target_sources",
            "static",
            "autogen",
            "static",
            "include_filters",
            "input_output_pairer",
            "input",
            "dir",
            "matchers",
            "target_dependency_file",
            "target_vertex",
            "file",
            "file",
            "bin",
            "regular",
            "instrumented",
            "target",
            "artifacts",
            "meta",
            "repo",
            "build_target_descriptor",
            "dependency_graph_data",
            "test_target_meta",
            "gem_target",
            "target",
            "tests",
        };

        enum Fields
        {
            Common,
            Root,
            Build,
            PlatformName,
            BuildConfig,
            RelativePaths,
            RunArtifactDir,
            CoverageArtifactDir,
            EnumerationCacheDir,
            TestImpactDataFile,
            TempWorkspace,
            ActiveWorkspace,
            TargetSources,
            StaticSources,
            AutogenSources,
            StaticArtifacts,
            SourceIncludeFilters,
            AutogenInputOutputPairer,
            AutogenInputSources,
            Directory,
            DependencyGraphMatchers,
            TargetDependencyFileMatcher,
            TargetVertexMatcher,
            TestTargetMetaFile,
            GemTargetFile,
            BinaryFile,
            RegularTargetExcludeFilter,
            InstrumentedTargetExcludeFilter,
            TargetName,
            Artifacts,
            Meta,
            Repository,
            BuildTargetDescriptor,
            DependencyGraphData,
            TestTargetMeta,
            GemTarget,
            ExcludedTargetName,
            ExcludedTargetTests,
            // Checksum
            _CHECKSUM_
        };
    } // namespace RuntimeConfigFactory

    ExcludedTargets ParseTargetExcludeList(const rapidjson::Value::ConstArray& testExcludes)
    {
        ExcludedTargets targetExcludeList;
        targetExcludeList.reserve(testExcludes.Size());
        for (const auto& testExclude : testExcludes)
        {
            ExcludedTarget excludedTarget;
            excludedTarget.m_name = testExclude[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::ExcludedTargetName]].GetString();
            if (testExclude.HasMember(RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::ExcludedTargetTests]))
            {
                const auto& excludedTests = testExclude[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::ExcludedTargetTests]].GetArray();
                for (const auto& excludedTest : excludedTests)
                {
                    excludedTarget.m_excludedTests.push_back(excludedTest.GetString());
                }
            }

            targetExcludeList.push_back(excludedTarget);
        }

        return targetExcludeList;
    }

    //! Returns an absolute path for a path relative to the specified root.
    inline RepoPath GetAbsPathFromRelPath(const RepoPath& root, const RepoPath& rel)
    {
        return root / rel;
    }

    ConfigMeta ParseConfigMeta(const rapidjson::Value& meta)
    {
        ConfigMeta configMeta;
        configMeta.m_platform = meta[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::PlatformName]].GetString();
        configMeta.m_buildConfig = meta[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::BuildConfig]].GetString();
        return configMeta;
    }

    RepoConfig ParseRepoConfig(const rapidjson::Value& repo)
    {
        RepoConfig repoConfig;
        repoConfig.m_root = repo[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Root]].GetString();
        repoConfig.m_build = repo[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Build]].GetString();
        return repoConfig;
    }

    ArtifactDir ParseTempWorkspaceConfig(const rapidjson::Value& tempWorkspace)
    {
        ArtifactDir tempWorkspaceConfig;
        tempWorkspaceConfig.m_testRunArtifactDirectory = tempWorkspace[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::RunArtifactDir]].GetString();
        tempWorkspaceConfig.m_coverageArtifactDirectory = tempWorkspace[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::CoverageArtifactDir]].GetString();
        tempWorkspaceConfig.m_enumerationCacheDirectory = tempWorkspace[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::EnumerationCacheDir]].GetString();
        return tempWorkspaceConfig;
    }

    WorkspaceConfig::Active ParseActiveWorkspaceConfig(const rapidjson::Value& activeWorkspace)
    {
        WorkspaceConfig::Active activeWorkspaceConfig;
        const auto& relativePaths = activeWorkspace[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::RelativePaths]];
        activeWorkspaceConfig.m_root = activeWorkspace[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Root]].GetString();
        activeWorkspaceConfig.m_sparTiaFile = relativePaths[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::TestImpactDataFile]].GetString();
        return activeWorkspaceConfig;
    }

    WorkspaceConfig ParseWorkspaceConfig(const rapidjson::Value& workspace)
    {
        WorkspaceConfig workspaceConfig;
        workspaceConfig.m_temp = ParseTempWorkspaceConfig(workspace[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::TempWorkspace]]);
        workspaceConfig.m_active = ParseActiveWorkspaceConfig(workspace[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::ActiveWorkspace]]);
        return workspaceConfig;
    }

    BuildTargetDescriptorConfig ParseBuildTargetDescriptorConfig(const rapidjson::Value& buildTargetDescriptor)
    {
        BuildTargetDescriptorConfig buildTargetDescriptorConfig;
        const auto& targetSources = buildTargetDescriptor[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::TargetSources]];
        const auto& staticTargetSources = targetSources[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::StaticSources]];
        const auto& autogenTargetSources = targetSources[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::AutogenSources]];
        buildTargetDescriptorConfig.m_mappingDirectory = buildTargetDescriptor[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Directory]].GetString();
        const auto& staticInclusionFilters = staticTargetSources[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::SourceIncludeFilters]].GetArray();
        
        buildTargetDescriptorConfig.m_staticInclusionFilters.reserve(staticInclusionFilters.Size());
        for (const auto& staticInclusionFilter : staticInclusionFilters)
        {
            buildTargetDescriptorConfig.m_staticInclusionFilters.push_back(staticInclusionFilter.GetString());
        }

        buildTargetDescriptorConfig.m_inputOutputPairer = autogenTargetSources[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::AutogenInputOutputPairer]].GetString();
        const auto& inputInclusionFilters =
            autogenTargetSources[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::AutogenInputSources]][RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::SourceIncludeFilters]].GetArray();
        buildTargetDescriptorConfig.m_inputInclusionFilters.reserve(inputInclusionFilters.Size());
        for (const auto& inputInclusionFilter : inputInclusionFilters)
        {
            buildTargetDescriptorConfig.m_inputInclusionFilters.push_back(inputInclusionFilter.GetString());
        }

        return buildTargetDescriptorConfig;
    }

    DependencyGraphDataConfig ParseDependencyGraphDataConfig(const rapidjson::Value& dependencyGraphData)
    {
        DependencyGraphDataConfig dependencyGraphDataConfig;
        const auto& matchers = dependencyGraphData[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::DependencyGraphMatchers]];
        dependencyGraphDataConfig.m_graphDirectory = dependencyGraphData[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Directory]].GetString();
        dependencyGraphDataConfig.m_targetDependencyFileMatcher = matchers[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::TargetDependencyFileMatcher]].GetString();
        dependencyGraphDataConfig.m_targetVertexMatcher = matchers[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::TargetVertexMatcher]].GetString();
        return dependencyGraphDataConfig;
    }

    TestTargetMetaConfig ParseTestTargetMetaConfig(const rapidjson::Value& testTargetMeta)
    {
        TestTargetMetaConfig testTargetMetaConfig;
        testTargetMetaConfig.m_metaFile = testTargetMeta[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::TestTargetMetaFile]].GetString();
        return testTargetMetaConfig;
    }

    GemTargetConfig ParseGemTargetConfig(const rapidjson::Value& gemTarget)
    {
        GemTargetConfig gemTargetConfig;
        gemTargetConfig.m_metaFile = gemTarget[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::GemTargetFile]].GetString();
        return gemTargetConfig;
    }

    RuntimeConfig RuntimeConfigurationFactory(const AZStd::string& configurationData)
    {
        static_assert(RuntimeConfigFactory::Fields::_CHECKSUM_ == AZStd::size(RuntimeConfigFactory::Keys));
        rapidjson::Document configurationFile;

        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            throw TestImpact::ConfigurationException("Could not parse runtimeConfig data, JSON has errors");
        }

        RuntimeConfig runtimeConfig;
        const auto& staticArtifacts = configurationFile[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Common]][RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Artifacts]][RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::StaticArtifacts]];
        runtimeConfig.m_meta = ParseConfigMeta(configurationFile[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Common]][RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Meta]]);
        runtimeConfig.m_repo = ParseRepoConfig(configurationFile[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Common]][RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::Repository]]);
        runtimeConfig.m_buildTargetDescriptor = ParseBuildTargetDescriptorConfig(staticArtifacts[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::BuildTargetDescriptor]]);
        runtimeConfig.m_dependencyGraphData = ParseDependencyGraphDataConfig(staticArtifacts[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::DependencyGraphData]]);
        runtimeConfig.m_testTargetMeta = ParseTestTargetMetaConfig(staticArtifacts[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::TestTargetMeta]]);
        runtimeConfig.m_gemTarget = ParseGemTargetConfig(staticArtifacts[RuntimeConfigFactory::Keys[RuntimeConfigFactory::Fields::GemTarget]]);

        return runtimeConfig;
    }
} // namespace TestImpact
