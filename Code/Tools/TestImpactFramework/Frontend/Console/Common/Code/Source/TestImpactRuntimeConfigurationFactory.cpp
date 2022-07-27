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

#include <AzCore/JSON/document.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>

namespace TestImpact
{
    namespace Config
    {
        // Keys for pertinent JSON elements
        constexpr const char* Keys[] =
        {
            "common",
            "root",
            "platform",
            "relative_paths",
            "artifact_dir",
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
            "exclude",
            "regular",
            "instrumented",
            "shard",
            "fixture_contiguous",
            "fixture_interleaved",
            "test_contiguous",
            "test_interleaved",
            "never",
            "target",
            "policy",
            "artifacts",
            "meta",
            "repo",
            "workspace",
            "build_target_descriptor",
            "dependency_graph_data",
            "test_target_meta",
            "gem_target"
        };

        enum
        {
            Common,
            Root,
            PlatformName,
            RelativePaths,
            ArtifactDir,
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
            TargetExclude,
            RegularTargetExcludeFilter,
            InstrumentedTargetExcludeFilter,
            TestSharding,
            ContinuousFixtureSharding,
            InterleavedFixtureSharding,
            ContinuousTestSharding,
            InterleavedTestSharding,
            NeverShard,
            TargetName,
            TestShardingPolicy,
            Artifacts,
            Meta,
            Repository,
            Workspace,
            BuildTargetDescriptor,
            DependencyGraphData,
            TestTargetMeta,
            GemTarget
        };
    }

    //! Returns an absolute path for a path relative to the specified root.
    inline RepoPath GetAbsPathFromRelPath(const RepoPath& root, const RepoPath& rel)
    {
        return root / rel;
    }

    ConfigMeta ParseConfigMeta(const rapidjson::Value& meta)
    {
        ConfigMeta configMeta;
        configMeta.m_platform = meta[Config::Keys[Config::PlatformName]].GetString();
        return configMeta;
    }

    RepoConfig ParseRepoConfig(const rapidjson::Value& repo)
    {
        RepoConfig repoConfig;
        repoConfig.m_root = repo[Config::Keys[Config::Root]].GetString();
        return repoConfig;
    }

    WorkspaceConfig::Temp ParseTempWorkspaceConfig(const rapidjson::Value& tempWorkspace)
    {
        WorkspaceConfig::Temp tempWorkspaceConfig;
        tempWorkspaceConfig.m_root = tempWorkspace[Config::Keys[Config::Root]].GetString();
        tempWorkspaceConfig.m_artifactDirectory =
            GetAbsPathFromRelPath(
                tempWorkspaceConfig.m_root, tempWorkspace[Config::Keys[Config::RelativePaths]][Config::Keys[Config::ArtifactDir]].GetString());
        tempWorkspaceConfig.m_enumerationCacheDirectory = GetAbsPathFromRelPath(
            tempWorkspaceConfig.m_root,
            tempWorkspace[Config::Keys[Config::RelativePaths]][Config::Keys[Config::EnumerationCacheDir]].GetString());
        return tempWorkspaceConfig;
    }

    WorkspaceConfig::Active ParseActiveWorkspaceConfig(const rapidjson::Value& activeWorkspace)
    {
        WorkspaceConfig::Active activeWorkspaceConfig;
        const auto& relativePaths = activeWorkspace[Config::Keys[Config::RelativePaths]];
        activeWorkspaceConfig.m_root = activeWorkspace[Config::Keys[Config::Root]].GetString();
        activeWorkspaceConfig.m_sparTiaFile = relativePaths[Config::Keys[Config::TestImpactDataFile]].GetString();
        return activeWorkspaceConfig;
    }

    WorkspaceConfig ParseWorkspaceConfig(const rapidjson::Value& workspace)
    {
        WorkspaceConfig workspaceConfig;
        workspaceConfig.m_temp = ParseTempWorkspaceConfig(workspace[Config::Keys[Config::TempWorkspace]]);
        workspaceConfig.m_active = ParseActiveWorkspaceConfig(workspace[Config::Keys[Config::ActiveWorkspace]]);
        return workspaceConfig;
    }

    BuildTargetDescriptorConfig ParseBuildTargetDescriptorConfig(const rapidjson::Value& buildTargetDescriptor)
    {
        BuildTargetDescriptorConfig buildTargetDescriptorConfig;
        const auto& targetSources = buildTargetDescriptor[Config::Keys[Config::TargetSources]];
        const auto& staticTargetSources = targetSources[Config::Keys[Config::StaticSources]];
        const auto& autogenTargetSources = targetSources[Config::Keys[Config::AutogenSources]];
        buildTargetDescriptorConfig.m_mappingDirectory = buildTargetDescriptor[Config::Keys[Config::Directory]].GetString();
        const auto& staticInclusionFilters = staticTargetSources[Config::Keys[Config::SourceIncludeFilters]].GetArray();
        
        buildTargetDescriptorConfig.m_staticInclusionFilters.reserve(staticInclusionFilters.Size());
        for (const auto& staticInclusionFilter : staticInclusionFilters)
        {
            buildTargetDescriptorConfig.m_staticInclusionFilters.push_back(staticInclusionFilter.GetString());
        }

        buildTargetDescriptorConfig.m_inputOutputPairer = autogenTargetSources[Config::Keys[Config::AutogenInputOutputPairer]].GetString();
        const auto& inputInclusionFilters =
            autogenTargetSources[Config::Keys[Config::AutogenInputSources]][Config::Keys[Config::SourceIncludeFilters]].GetArray();
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
        const auto& matchers = dependencyGraphData[Config::Keys[Config::DependencyGraphMatchers]];
        dependencyGraphDataConfig.m_graphDirectory = dependencyGraphData[Config::Keys[Config::Directory]].GetString();
        dependencyGraphDataConfig.m_targetDependencyFileMatcher = matchers[Config::Keys[Config::TargetDependencyFileMatcher]].GetString();
        dependencyGraphDataConfig.m_targetVertexMatcher = matchers[Config::Keys[Config::TargetVertexMatcher]].GetString();
        return dependencyGraphDataConfig;
    }

    TestTargetMetaConfig ParseTestTargetMetaConfig(const rapidjson::Value& testTargetMeta)
    {
        TestTargetMetaConfig testTargetMetaConfig;
        testTargetMetaConfig.m_metaFile = testTargetMeta[Config::Keys[Config::TestTargetMetaFile]].GetString();
        return testTargetMetaConfig;
    }

    GemTargetConfig ParseGemTargetConfig(const rapidjson::Value& gemTarget)
    {
        GemTargetConfig gemTargetConfig;
        gemTargetConfig.m_metaFile = gemTarget[Config::Keys[Config::GemTargetFile]].GetString();
        return gemTargetConfig;
    }

    RuntimeConfig RuntimeConfigurationFactory(const AZStd::string& configurationData)
    {
        rapidjson::Document configurationFile;

        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            throw TestImpact::ConfigurationException("Could not parse runtimeConfig data, JSON has errors");
        }

        RuntimeConfig runtimeConfig;
        const auto& staticArtifacts = configurationFile[Config::Keys[Config::Common]][Config::Keys[Config::Artifacts]][Config::Keys[Config::StaticArtifacts]];
        runtimeConfig.m_meta = ParseConfigMeta(configurationFile[Config::Keys[Config::Common]][Config::Keys[Config::Meta]]);
        runtimeConfig.m_repo = ParseRepoConfig(configurationFile[Config::Keys[Config::Common]][Config::Keys[Config::Repository]]);
        runtimeConfig.m_workspace = ParseWorkspaceConfig(configurationFile[Config::Keys[Config::Common]][Config::Keys[Config::Workspace]]);
        runtimeConfig.m_buildTargetDescriptor = ParseBuildTargetDescriptorConfig(staticArtifacts[Config::Keys[Config::BuildTargetDescriptor]]);
        runtimeConfig.m_dependencyGraphData = ParseDependencyGraphDataConfig(staticArtifacts[Config::Keys[Config::DependencyGraphData]]);
        runtimeConfig.m_testTargetMeta = ParseTestTargetMetaConfig(staticArtifacts[Config::Keys[Config::TestTargetMeta]]);
        runtimeConfig.m_gemTarget = ParseGemTargetConfig(staticArtifacts[Config::Keys[Config::GemTarget]]);

        return runtimeConfig;
    }
} // namespace TestImpact
