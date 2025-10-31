/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/Native/TestImpactNativeConfiguration.h>

#include <TestImpactRuntimeConfigurationFactory.h>

#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>

namespace TestImpact
{
    namespace NativeConfigFactory
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
            "target",
            "workspace",
            "temp",
            "sharded_run_artifact_dir",
            "sharded_coverage_artifact_dir"
        };

        enum Fields
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
            TargetName,
            Workspace,
            TempWorkspace,
            ShardedRunArtifactDir,
            ShardedCoverageArtifactDir,
            // Checksum
            _CHECKSUM_
        };
    } // namespace NativeConfigFactory

    NativeTestEngineConfig ParseTestEngineConfig(const rapidjson::Value& testEngine)
    {
        NativeTestEngineConfig testEngineConfig;
        testEngineConfig.m_testRunner.m_binary =
            testEngine[NativeConfigFactory::Keys[NativeConfigFactory::Fields::TestRunner]]
                [NativeConfigFactory::Keys[NativeConfigFactory::Fields::BinaryFile]].GetString();
        testEngineConfig.m_instrumentation.m_binary =
            testEngine[NativeConfigFactory::Keys[NativeConfigFactory::Fields::TestInstrumentation]]
                [NativeConfigFactory::Keys[NativeConfigFactory::Fields::BinaryFile]].GetString();
        return testEngineConfig;
    }

    NativeTargetConfig ParseTargetConfig(const rapidjson::Value& target)
    {
        NativeTargetConfig targetConfig;
        targetConfig.m_outputDirectory = target[NativeConfigFactory::Keys[NativeConfigFactory::Fields::Directory]].GetString();
        const auto& testExcludes = target[NativeConfigFactory::Keys[NativeConfigFactory::Fields::TargetExclude]];
        targetConfig.m_excludedTargets.m_excludedRegularTestTargets =
            ParseTargetExcludeList(testExcludes[NativeConfigFactory::Keys[NativeConfigFactory::Fields::RegularTargetExcludeFilter]].GetArray());
        targetConfig.m_excludedTargets.m_excludedInstrumentedTestTargets =
            ParseTargetExcludeList(testExcludes[NativeConfigFactory::Keys[NativeConfigFactory::Fields::InstrumentedTargetExcludeFilter]].GetArray());

        return targetConfig;
    }

    NativeShardedArtifactDir ParseShardedArtifactConfig(const rapidjson::Value& tempWorkspace)
    {
        NativeShardedArtifactDir shardedWorkspaceConfig;
        shardedWorkspaceConfig.m_shardedTestRunArtifactDirectory =
            tempWorkspace[NativeConfigFactory::Keys[NativeConfigFactory::Fields::ShardedRunArtifactDir]].GetString();
        shardedWorkspaceConfig.m_shardedCoverageArtifactDirectory =
            tempWorkspace[NativeConfigFactory::Keys[NativeConfigFactory::Fields::ShardedCoverageArtifactDir]].GetString();
        return shardedWorkspaceConfig;
    }

    NativeRuntimeConfig NativeRuntimeConfigurationFactory(const AZStd::string& configurationData)
    {
        static_assert(NativeConfigFactory::Fields::_CHECKSUM_ == AZStd::size(NativeConfigFactory::Keys));
        rapidjson::Document configurationFile;

        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            throw TestImpact::ConfigurationException("Could not parse runtimeConfig data, JSON has errors");
        }

        NativeRuntimeConfig runtimeConfig;
        runtimeConfig.m_commonConfig = RuntimeConfigurationFactory(configurationData);
        runtimeConfig.m_workspace =
            ParseWorkspaceConfig(configurationFile[NativeConfigFactory::Keys[NativeConfigFactory::Fields::Native]]
                [NativeConfigFactory::Keys[NativeConfigFactory::Fields::Workspace]]);
        runtimeConfig.m_shardedArtifactDir = ParseShardedArtifactConfig(
            configurationFile[NativeConfigFactory::Keys[NativeConfigFactory::Fields::Native]]
                [NativeConfigFactory::Keys[NativeConfigFactory::Fields::Workspace]]
                    [NativeConfigFactory::Keys[NativeConfigFactory::Fields::TempWorkspace]]);
        runtimeConfig.m_testEngine =
            ParseTestEngineConfig(configurationFile[NativeConfigFactory::Keys[NativeConfigFactory::Fields::Native]]
                [NativeConfigFactory::Keys[NativeConfigFactory::Fields::TestEngine]]);
        runtimeConfig.m_target =
            ParseTargetConfig(configurationFile[NativeConfigFactory::Keys[NativeConfigFactory::Fields::Native]]
                [NativeConfigFactory::Keys[NativeConfigFactory::Fields::TargetConfig]]);

        return runtimeConfig;
    }
} // namespace TestImpact
