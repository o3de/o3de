/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <TestImpactPythonRuntimeConfigurationFactory.h>
#include <TestImpactRuntimeConfigurationFactory.h>

namespace TestImpact
{
    namespace Config
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

        enum
        {
            TargetExclude,
            Python,
            TargetConfig,
            TestEngine,
            TestRunner,
            PythonCmd,
            Workspace,
        };
    } // namespace Config

    PythonTestEngineConfig ParseTestEngineConfig(const rapidjson::Value& testEngine)
    {
        PythonTestEngineConfig testEngineConfig;
        testEngineConfig.m_testRunner.m_pythonCmd = testEngine[Config::Keys[Config::TestRunner]][Config::Keys[Config::PythonCmd]].GetString();
        return testEngineConfig;
    }

    PythonTargetConfig ParseTargetConfig(const rapidjson::Value& target)
    {
        PythonTargetConfig targetConfig;
        targetConfig.m_excludedTargets = ParseTargetExcludeList(target[Config::Keys[Config::TargetExclude]].GetArray());

        return targetConfig;
    }

    PythonRuntimeConfig PythonRuntimeConfigurationFactory(const AZStd::string& configurationData)
    {
        rapidjson::Document configurationFile;

        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            throw TestImpact::ConfigurationException("Could not parse runtimeConfig data, JSON has errors");
        }

        PythonRuntimeConfig runtimeConfig;
        runtimeConfig.m_commonConfig = RuntimeConfigurationFactory(configurationData);
        runtimeConfig.m_workspace = ParseWorkspaceConfig(configurationFile[Config::Keys[Config::Python]][Config::Keys[Config::Workspace]]);
        runtimeConfig.m_testEngine = ParseTestEngineConfig(configurationFile[Config::Keys[Config::Python]][Config::Keys[Config::TestEngine]]);
        runtimeConfig.m_target = ParseTargetConfig(configurationFile[Config::Keys[Config::Python]][Config::Keys[Config::TargetConfig]]);

        return runtimeConfig;
    }
} // namespace TestImpact
