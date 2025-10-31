/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/Python/TestImpactPythonRuntimeConfigurationFactory.h>

#include <TestImpactRuntimeConfigurationFactory.h>

namespace TestImpact
{
    namespace PythonConfigFactory
    {
        // Keys for pertinent JSON elements
        constexpr const char* Keys[] = {
            "exclude",
            "python",
            "target",
            "test_engine",
            "test_runner",
            "bin",
            "workspace",
        };

        enum Fields
        {
            TargetExclude,
            Python,
            TargetConfig,
            TestEngine,
            TestRunner,
            PythonCmd,
            Workspace,
            // Checksum
            _CHECKSUM_
        };
    } // namespace Config

    PythonTestEngineConfig ParseTestEngineConfig(const rapidjson::Value& testEngine)
    {
        PythonTestEngineConfig testEngineConfig;
        testEngineConfig.m_testRunner.m_pythonCmd = testEngine[PythonConfigFactory::Keys[PythonConfigFactory::TestRunner]][PythonConfigFactory::Keys[PythonConfigFactory::PythonCmd]].GetString();
        return testEngineConfig;
    }

    PythonTargetConfig ParseTargetConfig(const rapidjson::Value& target)
    {
        PythonTargetConfig targetConfig;
        targetConfig.m_excludedTargets = ParseTargetExcludeList(target[PythonConfigFactory::Keys[PythonConfigFactory::TargetExclude]].GetArray());

        return targetConfig;
    }

    PythonRuntimeConfig PythonRuntimeConfigurationFactory(const AZStd::string& configurationData)
    {
        static_assert(PythonConfigFactory::Fields::_CHECKSUM_ == AZStd::size(PythonConfigFactory::Keys));
        rapidjson::Document configurationFile;

        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            throw TestImpact::ConfigurationException("Could not parse runtimeConfig data, JSON has errors");
        }

        PythonRuntimeConfig runtimeConfig;
        runtimeConfig.m_commonConfig = RuntimeConfigurationFactory(configurationData);
        runtimeConfig.m_workspace = ParseWorkspaceConfig(configurationFile[PythonConfigFactory::Keys[PythonConfigFactory::Python]][PythonConfigFactory::Keys[PythonConfigFactory::Workspace]]);
        runtimeConfig.m_testEngine = ParseTestEngineConfig(configurationFile[PythonConfigFactory::Keys[PythonConfigFactory::Python]][PythonConfigFactory::Keys[PythonConfigFactory::TestEngine]]);
        runtimeConfig.m_target = ParseTargetConfig(configurationFile[PythonConfigFactory::Keys[PythonConfigFactory::Python]][PythonConfigFactory::Keys[PythonConfigFactory::TargetConfig]]);

        return runtimeConfig;
    }
} // namespace TestImpact
