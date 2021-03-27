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

#include <Application.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

namespace AZ
{
    namespace SerializeContextTools
    {
        Application::Application(int argc, char** argv)
            : AZ::ComponentApplication(argc, argv)
        {
            AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
            if (projectPath.empty())
            {
                AZ_Error("Serialize Context Tools", false, "Unable to determine the project path . "
                    "Make sure project has been set or provide via the -project-path option on the command line. (See -help for more info.)");
                return;
            }

            size_t configSwitchCount = m_commandLine.GetNumSwitchValues("config");
            if (configSwitchCount > 0)
            {
                AZ::IO::FixedMaxPath absConfigFilePath = projectPath / m_commandLine.GetSwitchValue("config", configSwitchCount - 1);
                if (AZ::IO::SystemFile::Exists(absConfigFilePath.c_str()))
                {
                    m_configFilePath = AZStd::move(absConfigFilePath);
                }
            }

            // Merge the build system generated setting registry file by using either "Editor" or
            // and "${ProjectName}_GameLauncher" as a specialization
            auto projectName = AZ::Utils::GetProjectName();
            if (projectName.empty())
            {
                AZ_Error("Serialize Context Tools", false, "Unable to query the project name from settings registry");
            }
            else
            {
                AZ::SettingsRegistryInterface::Specializations projectSpecializations{ projectName };
                AZ::IO::PathView configFilenameStem = m_configFilePath.Stem();
                if (AZ::StringFunc::Equal(configFilenameStem.Native(),  "Editor"))
                {
                    projectSpecializations.Append("editor");
                }
                else if (AZ::StringFunc::Equal(configFilenameStem.Native(), "Game"))
                {
                    projectSpecializations.Append(projectName + "_GameLauncher");
                }

                // Used the project specializations to merge the build dependencies *.setreg files
                auto registry = AZ::SettingsRegistry::Get();
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_TargetBuildDependencyRegistry(*registry,
                    AZ_TRAIT_OS_PLATFORM_CODENAME, projectSpecializations);
            }
        }

        const char* Application::GetConfigFilePath() const
        {
            return m_configFilePath.c_str();
        }

        void Application::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
        {
            AZ::ComponentApplication::SetSettingsRegistrySpecializations(specializations);
            specializations.Append("serializecontexttools");
        }
    } // namespace SerializeContextTools
} // namespace AZ
