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
#include <AzCore/std/string/regex.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>

#include <regex>

namespace TestImpact
{
    namespace
    {
        using SubstitutePlaceholdersVisitor = AZStd::function<AZStd::optional<AZStd::string>(const AZStd::string& placeholder)>;

        using TokenizeVisitor = AZStd::function<void(const AZStd::string& token)>;
        void Tokenize(const AZStd::string& source, const AZStd::string& delimiter, const TokenizeVisitor& visitor)
        {
            size_t last = 0;
            size_t next = 0;
            while ((next = source.find(delimiter, last)) != AZStd::string::npos)
            {
                visitor(source.substr(last, next - last));
                last = next + delimiter.size();
            }
            visitor(source.substr(last));
        }

        AZStd::string SubstitutePlaceholders(
            const AZStd::string& stringWithPlaceholders,
            const SubstitutePlaceholdersVisitor& visitor)
        {
            const std::string& startDelim = "{";
            const std::string& endDelim = "}";
            const auto paramMatcherPattern = std::regex("\\{(.*?)\\}");
            const std::string stdStringWithPlaceholders = stringWithPlaceholders.c_str();
            std::sregex_token_iterator tokens(stdStringWithPlaceholders.begin(), stdStringWithPlaceholders.end(), paramMatcherPattern, { -1, 0 });
            AZStd::string ss;
            while (tokens != std::sregex_token_iterator())
            {
                const auto token = tokens->str();
                const auto delimStartPos = token.find(startDelim);
                const auto paramBegin = delimStartPos + startDelim.length();
                const auto delimEndPos = token.find_first_of(endDelim, paramBegin);

                if (delimStartPos != std::string::npos && delimEndPos != std::string::npos)
                {
                    const auto placeholder = token.substr(paramBegin, delimEndPos - paramBegin);
                    const auto substitute = visitor(placeholder.c_str());
                    if (substitute.has_value())
                    {
                        ss += substitute.value();
                    }
                    else
                    {
                        ss += tokens->str().c_str();
                    }
                }
                else
                {
                    ss += token.c_str();
                }

                tokens++;
            }

            return ss.c_str();
        }
    }

    AZ::IO::Path MakePreferredPath(const char* path)
    {
        return AZ::IO::Path(path).MakePreferred();
    }

    RuntimeConfig ConfigurationFactory(const AZStd::string& configurationData)
    {
        RuntimeConfig runtimeConfig;
        rapidjson::Document configurationFile;

        const auto parseAndReplace = [&configurationFile](const AZStd::string& str)
        {
            return SubstitutePlaceholders(str, [&configurationFile]([[maybe_unused]] const AZStd::string& placeholder)
                -> AZStd::optional<AZStd::string>
            {
                AZStd::smatch matches;
                if (AZStd::regex_match(placeholder, matches, AZStd::regex("\\[(.*)\\]")))
                {
                    rapidjson_ly::Value* node = nullptr;
                    Tokenize(matches[1], ".", [&node, &configurationFile](const AZStd::string& token)
                    {
                        if (node)
                        {
                            node = &(*node)[token.c_str()];
                        }
                        else
                        {
                            node = &configurationFile[token.c_str()];
                        }
                    });

                    if (node)
                    {
                        return node->GetString();
                    }
                }

                return AZStd::nullopt;
            });
        };

        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            throw TestImpact::ConfigurationException("Could not parse runtimeConfig data, JSON has errors");
        }

        // Configuration meta-data
        runtimeConfig.m_meta.m_platform = configurationFile["meta"]["platform"].GetString();
        runtimeConfig.m_meta.m_version = configurationFile["meta"]["version"].GetString();
        runtimeConfig.m_meta.m_timestamp = configurationFile["meta"]["timestamp"].GetString();

        // Repository
        runtimeConfig.m_repo.m_root = MakePreferredPath(configurationFile["repo"]["root"].GetString());

        // Workspace
        runtimeConfig.m_workspace.m_temp.m_root = MakePreferredPath(configurationFile["workspace"]["temp"]["root"].GetString());
        runtimeConfig.m_workspace.m_temp.m_artifactDirectory = runtimeConfig.m_workspace.m_temp.m_root / MakePreferredPath(configurationFile["workspace"]["temp"]["root"].GetString());
        runtimeConfig.m_workspace.m_persistent.m_root = MakePreferredPath(configurationFile["workspace"]["persistent"]["root"].GetString());
        runtimeConfig.m_workspace.m_persistent.m_sparTIAFile = runtimeConfig.m_workspace.m_persistent.m_root / MakePreferredPath(configurationFile["workspace"]["persistent"]["test_impact_data_file"].GetString());
        runtimeConfig.m_workspace.m_persistent.m_enumerationCacheDirectory = runtimeConfig.m_workspace.m_persistent.m_root / MakePreferredPath(configurationFile["workspace"]["persistent"]["enumeration_cache_dir"].GetString());

        // Build target descriptors
        runtimeConfig.m_buildTargetDescriptor.m_outputDirectory = MakePreferredPath(configurationFile["artifacts"]["static"]["build_target_descriptor"]["dir"].GetString());
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
        runtimeConfig.m_dependencyGraphData.m_outputDirectory = MakePreferredPath(configurationFile["artifacts"]["static"]["dependency_graph_data"]["dir"].GetString());
        runtimeConfig.m_dependencyGraphData.m_targetDependencyFileMatcher =
            configurationFile["artifacts"]["static"]["dependency_graph_data"]["matchers"]["target_dependency_file"].GetString();
        runtimeConfig.m_dependencyGraphData.m_targetVertexMatcher =
            configurationFile["artifacts"]["static"]["dependency_graph_data"]["matchers"]["target_vertex"].GetString();

        // Test target meta
        runtimeConfig.m_testTargetMeta.m_file = MakePreferredPath(configurationFile["artifacts"]["static"]["test_target_meta"]["file"].GetString());

        // Test engine
        runtimeConfig.m_testEngine.m_testRunner.m_binary = MakePreferredPath(configurationFile["test_engine"]["test_runner"]["bin"].GetString());
        runtimeConfig.m_testEngine.m_instrumentation.m_binary = MakePreferredPath(configurationFile["test_engine"]["instrumentation"]["bin"].GetString());

        // Target
        runtimeConfig.m_target.m_outputDirectory = MakePreferredPath(configurationFile["target"]["dir"].GetString());
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
                TargetConfig::ShardedTarget shard;
                shard.m_name = testShard["target"].GetString();
                shard.m_policy = testShard["policy"].GetString();
                runtimeConfig.m_target.m_shardedTestTargets.push_back(AZStd::move(shard));
            }
        }

        return runtimeConfig;
    }
}
