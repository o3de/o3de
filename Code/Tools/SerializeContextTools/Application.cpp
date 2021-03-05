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
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace SerializeContextTools
    {
        Application::Application(int* argc, char*** argv)
            : AzToolsFramework::ToolsApplication(argc, argv)
        {
            AZ::IO::FixedMaxPath sourceGameFolder;
            if (!m_settingsRegistry->Get(sourceGameFolder.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_SourceGameFolder))
            {
                AZ_Error("Serialize Context Tools", false, "Unable to determine the game root automatically. "
                    "Make sure a default project has been set or provide a default option on the command line. (See -help for more info.)");
                return;
            }

            AZStd::string configFilePath = "Config/Editor.xml";
            if (m_commandLine.HasSwitch("config"))
            {
                configFilePath = m_commandLine.GetSwitchValue("config", 0);
            }

            AZ::IO::FixedMaxPath absConfigFilePath = sourceGameFolder / configFilePath;
            if (AZ::IO::SystemFile::Exists(absConfigFilePath.c_str()))
            {
                m_configFilePath = AZStd::move(absConfigFilePath);
            }
            else
            {
                AZ_Error("Serialize Context Tools", false, "Unable to resolve path to config file.");
            }

            // Merge the build system generated setting registry file by using either "Editor" or
            // and "${ProjectName}_GameLauncher" as a specialization
            bool projectNameFound{};
            AZ::SettingsRegistryInterface::FixedValueString projectName;
            AZ::SettingsRegistryInterface& registry = *AZ::SettingsRegistry::Get();
            constexpr auto sysGameFolderKey = AZ::SettingsRegistryInterface::FixedValueString(
                AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/sys_game_folder";
            if (projectNameFound = registry.Get(projectName, sysGameFolderKey); !projectNameFound)
            {
                AZ_Error("Serialize Context Tools", false, "Unable to query the %s key from the SettingsRegistry",
                    sysGameFolderKey.c_str());
            }
            else
            {
                AZ::IO::PathView configFilenameStem = m_configFilePath.Stem();
                AZ::SettingsRegistryInterface::Specializations projectSpecializations{ projectName };
                size_t configFilenameStemSize = configFilenameStem.Native().size();
                if (configFilenameStemSize > 0 && azstrnicmp(configFilenameStem.Native().data(), "Editor", configFilenameStemSize) == 0)
                {
                    projectSpecializations.Append("editor");
                }
                else if (configFilenameStemSize > 0 && azstrnicmp(configFilenameStem.Native().data(), "Game", configFilenameStemSize) == 0)
                {
                    projectSpecializations.Append(projectName + "_GameLauncher");
                }
                else
                {
                    AZ_TracePrintf("Serialize Context Tools", "No Editor.xml or Game.xml supplied."
                        R"( Build dependency specialization will not use a specialization of "%s" nor "editor" for locating *.setreg files)",
                        (projectName + "_GameLauncher").c_str());
                }

                // Used the project specializations to merge the build dependencies *.setreg files
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_TargetBuildDependencyRegistry(registry,
                    AZ_TRAIT_OS_PLATFORM_CODENAME, projectSpecializations);
            }
        }

        const char* Application::GetConfigFilePath() const
        {
            return m_configFilePath.c_str();
        }

        void Application::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
        {
            AzToolsFramework::ToolsApplication::SetSettingsRegistrySpecializations(specializations);
            specializations.Append("serializecontexttools");
        }
    } // namespace SerializeContextTools
} // namespace AZ
