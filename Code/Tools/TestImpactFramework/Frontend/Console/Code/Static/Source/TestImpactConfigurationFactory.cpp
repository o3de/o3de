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

#include <TestImpactConfigurationFactory.h>

#include <AzCore/JSON/document.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>


namespace TestImpact
{
    RuntimeConfig ConfigurationFactory(const AZStd::string& configurationData)
    {
        RuntimeConfig runtimeConfig;
        rapidjson::Document configurationFile;

        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            throw TestImpact::ConfigurationException("Could not parse runtimeConfig data, JSON has errors");
        }

        // Configuration meta-data
        runtimeConfig.m_meta.m_platform = configurationFile["meta"]["platform"].GetString();

        // Repository
        runtimeConfig.m_repo.m_root = configurationFile["repo"]["root"].GetString();

        // Workspace
        runtimeConfig.m_workspace.m_temp.m_root = configurationFile["workspace"]["temp"]["root"].GetString();
        runtimeConfig.m_workspace.m_temp.m_relativePaths.m_artifactDirectory = runtimeConfig.m_workspace.m_temp.m_root / RepoPath(configurationFile["workspace"]["temp"]["relative_paths"]["artifact_dir"].GetString());
        runtimeConfig.m_workspace.m_persistent.m_root = configurationFile["workspace"]["persistent"]["root"].GetString();
        runtimeConfig.m_workspace.m_persistent.m_relativePaths.m_sparTIAFile = runtimeConfig.m_workspace.m_persistent.m_root / RepoPath(configurationFile["workspace"]["persistent"]["relative_paths"]["test_impact_data_file"].GetString());
        runtimeConfig.m_workspace.m_persistent.m_relativePaths.m_enumerationCacheDirectory = runtimeConfig.m_workspace.m_persistent.m_root / RepoPath(configurationFile["workspace"]["persistent"]["relative_paths"]["enumeration_cache_dir"].GetString());

        // Build target descriptors
        runtimeConfig.m_buildTargetDescriptor.m_mappingDirectory = configurationFile["artifacts"]["static"]["build_target_descriptor"]["dir"].GetString();
        const auto& staticInclusionFilters =
            configurationFile["artifacts"]["static"]["build_target_descriptor"]["target_sources"]["static"]["include_filters"].GetArray();
        if (!staticInclusionFilters.Empty())
        {
            runtimeConfig.m_buildTargetDescriptor.m_staticInclusionFilters.reserve(staticInclusionFilters.Size());
            for (const auto& staticInclusionFilter : staticInclusionFilters)
            {
                runtimeConfig.m_buildTargetDescriptor.m_staticInclusionFilters.push_back(staticInclusionFilter.GetString());
            }
        }
        runtimeConfig.m_buildTargetDescriptor.m_inputOutputPairer =
            configurationFile["artifacts"]["static"]["build_target_descriptor"]["target_sources"]["autogen"]["input_output_pairer"].GetString();
        const auto& inputInclusionFilters =
            configurationFile["artifacts"]["static"]["build_target_descriptor"]["target_sources"]["autogen"]["input"]["include_filters"].GetArray();
        if (!inputInclusionFilters.Empty())
        {
            runtimeConfig.m_buildTargetDescriptor.m_inputInclusionFilters.reserve(inputInclusionFilters.Size());
            for (const auto& inputInclusionFilter : inputInclusionFilters)
            {
                runtimeConfig.m_buildTargetDescriptor.m_inputInclusionFilters.push_back(inputInclusionFilter.GetString());
            }
        }

        // Dependency graph data
        runtimeConfig.m_dependencyGraphData.m_graphDirectory = configurationFile["artifacts"]["static"]["dependency_graph_data"]["dir"].GetString();
        runtimeConfig.m_dependencyGraphData.m_targetDependencyFileMatcher =
            configurationFile["artifacts"]["static"]["dependency_graph_data"]["matchers"]["target_dependency_file"].GetString();
        runtimeConfig.m_dependencyGraphData.m_targetVertexMatcher =
            configurationFile["artifacts"]["static"]["dependency_graph_data"]["matchers"]["target_vertex"].GetString();

        // Test target meta
        runtimeConfig.m_testTargetMeta.m_metaFile = configurationFile["artifacts"]["static"]["test_target_meta"]["file"].GetString();

        // Test engine
        runtimeConfig.m_testEngine.m_testRunner.m_binary = configurationFile["test_engine"]["test_runner"]["bin"].GetString();
        runtimeConfig.m_testEngine.m_instrumentation.m_binary = configurationFile["test_engine"]["instrumentation"]["bin"].GetString();

        // Target
        runtimeConfig.m_target.m_outputDirectory = configurationFile["target"]["dir"].GetString();
        const auto& testExcludes =
            configurationFile["target"]["exclude"].GetArray();
        if (!testExcludes.Empty())
        {
            runtimeConfig.m_target.m_excludedTestTargets.reserve(testExcludes.Size());
            for (const auto& testExclude : testExcludes)
            {
                runtimeConfig.m_target.m_excludedTestTargets.push_back(testExclude.GetString());
            }
        }
        const auto& testShards =
            configurationFile["target"]["shard"].GetArray();
        if (!testShards.Empty())
        {
            runtimeConfig.m_target.m_shardedTestTargets.reserve(testShards.Size());
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
                runtimeConfig.m_target.m_shardedTestTargets.push_back(AZStd::move(shard));
            }
        }

        return runtimeConfig;
    }
}
