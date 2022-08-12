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
#include <TestImpactNativeRuntimeConfigurationFactory.h>

#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>

namespace TestImpact
{
    namespace Config
    {
        // Keys for pertinent JSON elements
        constexpr const char* Keys[] =
        {
            "native",
            "test_engine",
            "target",
            "test_runner",
            "bin",
            "instrumentation",
            "dir",
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
            "workspace",
        };

        enum
        {
            Native,
            TestEngine,
            TargetConfig,
            TestRunner,
            BinaryFile,
            TestInstrumentation,
            Directory,
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
            Workspace,
        };
    } // namespace Config

    NativeTestEngineConfig ParseTestEngineConfig(const rapidjson::Value& testEngine)
    {
        NativeTestEngineConfig testEngineConfig;
        testEngineConfig.m_testRunner.m_binary = testEngine[Config::Keys[Config::TestRunner]][Config::Keys[Config::BinaryFile]].GetString();
        testEngineConfig.m_instrumentation.m_binary = testEngine[Config::Keys[Config::TestInstrumentation]][Config::Keys[Config::BinaryFile]].GetString();
        return testEngineConfig;
    }

    NativeTargetConfig ParseTargetConfig(const rapidjson::Value& target)
    {
        NativeTargetConfig targetConfig;
        targetConfig.m_outputDirectory = target[Config::Keys[Config::Directory]].GetString();
        const auto& testExcludes = target[Config::Keys[Config::TargetExclude]];
        targetConfig.m_excludedTargets.m_excludedRegularTestTargets = ParseTargetExcludeList(testExcludes[Config::Keys[Config::RegularTargetExcludeFilter]].GetArray());
        targetConfig.m_excludedTargets.m_excludedInstrumentedTestTargets = ParseTargetExcludeList(testExcludes[Config::Keys[Config::InstrumentedTargetExcludeFilter]].GetArray());

        const auto& testShards =  target[Config::Keys[Config::TestSharding]].GetArray();
        targetConfig.m_shardedTestTargets.reserve(testShards.Size());
        for (const auto& testShard : testShards)
        {
            const auto getShardingConfiguration = [](const AZStd::string& config)
            {
                if (config == Config::Keys[Config::ContinuousFixtureSharding])
                {
                    return ShardConfiguration::FixtureContiguous;
                }
                else if (config == Config::Keys[Config::InterleavedFixtureSharding])
                {
                    return ShardConfiguration::FixtureInterleaved;
                }
                else if (config == Config::Keys[Config::ContinuousTestSharding])
                {
                    return ShardConfiguration::TestContiguous;
                }
                else if (config == Config::Keys[Config::InterleavedTestSharding])
                {
                    return ShardConfiguration::TestInterleaved;
                }
                else if (config == Config::Keys[Config::NeverShard])
                {
                    return ShardConfiguration::Never;
                }
                else
                {
                    throw ConfigurationException(AZStd::string::format("Unexpected sharding configuration: %s", config.c_str()));
                }
            };

            NativeTargetConfig::ShardedTarget shard;
            shard.m_name = testShard[Config::Keys[Config::TargetName]].GetString();
            shard.m_configuration = getShardingConfiguration(testShard[Config::Keys[Config::TestShardingPolicy]].GetString());
            targetConfig.m_shardedTestTargets.push_back(AZStd::move(shard));
        }

        return targetConfig;
    }

    NativeRuntimeConfig NativeRuntimeConfigurationFactory(const AZStd::string& configurationData)
    {
        rapidjson::Document configurationFile;

        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            throw TestImpact::ConfigurationException("Could not parse runtimeConfig data, JSON has errors");
        }

        NativeRuntimeConfig runtimeConfig;
        runtimeConfig.m_commonConfig = RuntimeConfigurationFactory(configurationData);
        runtimeConfig.m_workspace = ParseWorkspaceConfig(configurationFile[Config::Keys[Config::Native]][Config::Keys[Config::Workspace]]);
        runtimeConfig.m_testEngine = ParseTestEngineConfig(configurationFile[Config::Keys[Config::Native]][Config::Keys[Config::TestEngine]]);
        runtimeConfig.m_target = ParseTargetConfig(configurationFile[Config::Keys[Config::Native]][Config::Keys[Config::TargetConfig]]);

        return runtimeConfig;
    }
} // namespace TestImpact
