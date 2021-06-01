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

#include <TestImpactFramework/TestImpactConfigurationException.h>

#include <TestImpactRuntimeConfigurationFactory.h>

#include <AzCore/JSON/document.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>


namespace TestImpact
{

    //! Returns an absolute path for a path relative to the specified root.
    RepoPath GetAbsPathFromRelPath(const RepoPath& root, const RepoPath& rel)
    {
        return root / rel;
    }

    ConfigMeta ParseConfigMeta(const rapidjson::Value& meta)
    {
        ConfigMeta configMeta;
        configMeta.m_platform = meta["platform"].GetString();
        return configMeta;
    }

    RepoConfig ParseRepoConfig(const rapidjson::Value& repo)
    {
        RepoConfig repoConfig;
        repoConfig.m_root = repo["root"].GetString();
        return repoConfig;
    }

    WorkspaceConfig::Temp ParseTempWorkspaceConfig(const rapidjson::Value& tempWorkspace)
    {
        WorkspaceConfig::Temp tempWorkspaceConfig;
        tempWorkspaceConfig.m_root = tempWorkspace["root"].GetString();
        tempWorkspaceConfig.m_artifactDirectory =
            GetAbsPathFromRelPath(tempWorkspaceConfig.m_root, tempWorkspace["relative_paths"]["artifact_dir"].GetString());
        return tempWorkspaceConfig;
    }

    WorkspaceConfig::Active ParseActiveWorkspaceConfig(const rapidjson::Value& activeWorkspace)
    {
        WorkspaceConfig::Active activeWorkspaceConfig;
        const auto& relativePaths = activeWorkspace["relative_paths"];
        activeWorkspaceConfig.m_root = activeWorkspace["root"].GetString();
        activeWorkspaceConfig.m_enumerationCacheDirectory
            = GetAbsPathFromRelPath(activeWorkspaceConfig.m_root, relativePaths["enumeration_cache_dir"].GetString());
        activeWorkspaceConfig.m_sparTIAFile
            = GetAbsPathFromRelPath(activeWorkspaceConfig.m_root, relativePaths["test_impact_data_file"].GetString());
        return activeWorkspaceConfig;
    }

    WorkspaceConfig ParseWorkspaceConfig(const rapidjson::Value& workspace)
    {
        WorkspaceConfig workspaceConfig;
        workspaceConfig.m_temp = ParseTempWorkspaceConfig(workspace["temp"]);
        workspaceConfig.m_active = ParseActiveWorkspaceConfig(workspace["active"]);
        return workspaceConfig;
    }

    BuildTargetDescriptorConfig ParseBuildTargetDescriptorConfig(const rapidjson::Value& buildTargetDescriptor)
    {
        BuildTargetDescriptorConfig buildTargetDescriptorConfig;
        const auto& targetSources = buildTargetDescriptor["target_sources"];
        const auto& staticTargetSources = targetSources["static"];
        const auto& autogenTargetSources = targetSources["autogen"];
        buildTargetDescriptorConfig.m_mappingDirectory = buildTargetDescriptor["dir"].GetString();
        const auto& staticInclusionFilters = staticTargetSources["include_filters"].GetArray();
        if (!staticInclusionFilters.Empty())
        {
            buildTargetDescriptorConfig.m_staticInclusionFilters.reserve(staticInclusionFilters.Size());
            for (const auto& staticInclusionFilter : staticInclusionFilters)
            {
                buildTargetDescriptorConfig.m_staticInclusionFilters.push_back(staticInclusionFilter.GetString());
            }
        }

        buildTargetDescriptorConfig.m_inputOutputPairer = autogenTargetSources["input_output_pairer"].GetString();
        const auto& inputInclusionFilters = autogenTargetSources["input"]["include_filters"].GetArray();
        if (!inputInclusionFilters.Empty())
        {
            buildTargetDescriptorConfig.m_inputInclusionFilters.reserve(inputInclusionFilters.Size());
            for (const auto& inputInclusionFilter : inputInclusionFilters)
            {
                buildTargetDescriptorConfig.m_inputInclusionFilters.push_back(inputInclusionFilter.GetString());
            }
        }

        return buildTargetDescriptorConfig;
    }

    DependencyGraphDataConfig ParseDependencyGraphDataConfig(const rapidjson::Value& dependencyGraphData)
    {
        DependencyGraphDataConfig dependencyGraphDataConfig;
        const auto& matchers = dependencyGraphData["matchers"];
        dependencyGraphDataConfig.m_graphDirectory = dependencyGraphData["dir"].GetString();
        dependencyGraphDataConfig.m_targetDependencyFileMatcher = matchers["target_dependency_file"].GetString();
        dependencyGraphDataConfig.m_targetVertexMatcher = matchers["target_vertex"].GetString();
        return dependencyGraphDataConfig;
    }

    TestTargetMetaConfig ParseTestTargetMetaConfig(const rapidjson::Value& testTargetMeta)
    {
        TestTargetMetaConfig testTargetMetaConfig;
        testTargetMetaConfig.m_metaFile = testTargetMeta["file"].GetString();
        return testTargetMetaConfig;
    }

    TestEngineConfig ParseTestEngineConfig(const rapidjson::Value& testEngine)
    {
        TestEngineConfig testEngineConfig;
        testEngineConfig.m_testRunner.m_binary = testEngine["test_runner"]["bin"].GetString();
        testEngineConfig.m_instrumentation.m_binary = testEngine["instrumentation"]["bin"].GetString();
        return testEngineConfig;
    }
    TargetConfig ParseTargetConfig(const rapidjson::Value& target)
    {
        TargetConfig targetConfig;
        targetConfig.m_outputDirectory = target["dir"].GetString();
        const auto& testExcludes =
            target["exclude"].GetArray();
        if (!testExcludes.Empty())
        {
            targetConfig.m_excludedTestTargets.reserve(testExcludes.Size());
            for (const auto& testExclude : testExcludes)
            {
                targetConfig.m_excludedTestTargets.push_back(testExclude.GetString());
            }
        }
        const auto& testShards =
            target["shard"].GetArray();
        if (!testShards.Empty())
        {
            targetConfig.m_shardedTestTargets.reserve(testShards.Size());
            for (const auto& testShard : testShards)
            {
                const auto getShardingConfiguration = [](const AZStd::string& config)
                {
                    if (config == "fixture_contiguous")
                    {
                        return ShardConfiguration::FixtureContiguous;
                    }
                    else if (config == "fixture_interleaved")
                    {
                        return ShardConfiguration::FixtureInterleaved;
                    }
                    else if (config == "test_contiguous")
                    {
                        return ShardConfiguration::TestContiguous;
                    }
                    else if (config == "test_interleaved")
                    {
                        return ShardConfiguration::TestInterleaved;
                    }
                    else if (config == "never")
                    {
                        return ShardConfiguration::Never;
                    }
                    else
                    {
                        throw ConfigurationException(AZStd::string::format("Unexpected sharding configuration: %s", config.c_str()));
                    }
                };

                TargetConfig::ShardedTarget shard;
                shard.m_name = testShard["target"].GetString();
                shard.m_configuration = getShardingConfiguration(testShard["policy"].GetString());
                targetConfig.m_shardedTestTargets.push_back(AZStd::move(shard));
            }
        }

        return targetConfig;
    }

    RuntimeConfig RuntimeConfigurationFactory(const AZStd::string& configurationData)
    {
        RuntimeConfig runtimeConfig;
        rapidjson::Document configurationFile;

        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            throw TestImpact::ConfigurationException("Could not parse runtimeConfig data, JSON has errors");
        }

        const auto& staticArtifacts = configurationFile["artifacts"]["static"];
        runtimeConfig.m_meta = ParseConfigMeta(configurationFile["meta"]);
        runtimeConfig.m_repo = ParseRepoConfig(configurationFile["repo"]);
        runtimeConfig.m_workspace = ParseWorkspaceConfig(configurationFile["workspace"]);
        runtimeConfig.m_buildTargetDescriptor = ParseBuildTargetDescriptorConfig(staticArtifacts["build_target_descriptor"]);
        runtimeConfig.m_dependencyGraphData = ParseDependencyGraphDataConfig(staticArtifacts["dependency_graph_data"]);
        runtimeConfig.m_testTargetMeta = ParseTestTargetMetaConfig(staticArtifacts["static"]["test_target_meta"]);
        runtimeConfig.m_testEngine = ParseTestEngineConfig(configurationFile["test_engine"]);
        runtimeConfig.m_target = ParseTargetConfig(configurationFile["target"]);

        return runtimeConfig;
    }
} // namespace TestImpact
