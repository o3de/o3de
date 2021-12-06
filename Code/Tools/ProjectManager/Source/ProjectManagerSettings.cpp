/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectManagerSettings.h>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Utils/Utils.h>

namespace O3DE::ProjectManager
{
    namespace PMSettings
    {
        bool SaveProjectManagerSettings()
        {
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
            dumperSettings.m_prettifyOutput = true;
            dumperSettings.m_jsonPointerPrefix = ProjectManagerKeyPrefix;

            AZStd::string stringBuffer;
            AZ::IO::ByteContainerStream stringStream(&stringBuffer);
            if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(
                    *settingsRegistry, ProjectManagerKeyPrefix, stringStream, dumperSettings))
            {
                AZ_Warning("ProjectManager", false, "Could not save Project Manager settings to stream");
                return false;
            }

            AZ::IO::FixedMaxPath o3deUserPath = AZ::Utils::GetO3deManifestDirectory();
            o3deUserPath /= AZ::SettingsRegistryInterface::RegistryFolder;
            o3deUserPath /= "ProjectManager.setreg";

            bool saved = false;
            constexpr auto configurationMode =
                AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;

            AZ::IO::SystemFile outputFile;
            if (outputFile.Open(o3deUserPath.c_str(), configurationMode))
            {
                saved = outputFile.Write(stringBuffer.data(), stringBuffer.size()) == stringBuffer.size();
            }

            AZ_Warning("ProjectManager", saved, "Unable to save Project Manager registry file to path: %s", o3deUserPath.c_str());
            return saved;
        }

        bool GetProjectManagerKey(QString& result, const QString& settingsKey)
        {
            bool success = false;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                AZStd::string settingsValue;
                success = settingsRegistry->Get(settingsValue, settingsKey.toStdString().c_str());

                result = settingsValue.c_str();
            }

            return success;
        }

        bool GetProjectManagerKey(bool& result, const QString& settingsKey)
        {
            bool success = false;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                success = settingsRegistry->Get(result, settingsKey.toStdString().c_str());
            }

            return success;
        }

        bool SetProjectManagerKey(const QString& settingsKey, const QString& settingsValue, bool saveToDisk)
        {
            bool success = false;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                success = settingsRegistry->Set(settingsKey.toStdString().c_str(), settingsValue.toStdString().c_str());

                if (saveToDisk)
                {
                    SaveProjectManagerSettings();
                }
            }

            return success;
        }

        bool SetProjectManagerKey(const QString& settingsKey, bool settingsValue, bool saveToDisk)
        {
            bool success = false;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                success = settingsRegistry->Set(settingsKey.toStdString().c_str(), settingsValue);

                if (saveToDisk)
                {
                    SaveProjectManagerSettings();
                }
            }

            return success;
        }

        bool RemoveProjectManagerKey(const QString& settingsKey, bool saveToDisk)
        {
            bool success = false;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                success = settingsRegistry->Remove(settingsKey.toStdString().c_str());

                if (saveToDisk)
                {
                    SaveProjectManagerSettings();
                }
            }

            return success;
        }

        bool CopyProjectManagerKeyString(const QString& settingsKeyOrig, const QString& settingsKeyDest, bool removeOrig, bool saveToDisk)
        {
            bool success = false;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                AZStd::string settingsValue;
                success = settingsRegistry->Get(settingsValue, settingsKeyOrig.toStdString().c_str());

                if (success)
                {
                    success = settingsRegistry->Set(settingsKeyDest.toStdString().c_str(), settingsValue);
                    if (success)
                    {
                        if (removeOrig)
                        {
                            success = settingsRegistry->Remove(settingsKeyOrig.toStdString().c_str());
                        }

                        if (saveToDisk)
                        {
                            SaveProjectManagerSettings();
                        }
                    }
                }
            }

            return success;
        }

        QString GetProjectKey(const ProjectInfo& projectInfo)
        {
            return QString("%1/Projects/%2/%3").arg(ProjectManagerKeyPrefix, projectInfo.m_id, projectInfo.m_projectName);
        }

        QString GetExternalLinkWarningKey()
        {
            return QString("%1/SkipExternalLinkWarning").arg(ProjectManagerKeyPrefix);
        }

        QString GetProjectsBuiltSuccessfullyKey()
        {
            return QString("%1/SuccessfulBuildPaths").arg(ProjectManagerKeyPrefix);
        }

        bool GetBuiltSuccessfullyPaths(AZStd::set<AZStd::string>& result)
        {
            bool success = false;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                QString builtKey = GetProjectsBuiltSuccessfullyKey();
                success = settingsRegistry->GetObject<AZStd::set<AZStd::string>>(result, builtKey.toStdString().c_str());
            }

            return success;
        }

        bool GetProjectBuiltSuccessfully(bool& result, const ProjectInfo& projectInfo)
        {
            AZStd::set<AZStd::string> builtPathsResult;
            bool success = GetBuiltSuccessfullyPaths(builtPathsResult);
            if (success)
            {
                // Check if buildPath is listed as successfully built
                AZStd::string projectPath = projectInfo.m_path.toStdString().c_str();
                if (builtPathsResult.contains(projectPath))
                {
                    result = true;
                }
            }
            // No project built statuses known
            else
            {
                result = false;
            }

            return success;
        }

        bool SetProjectBuiltSuccessfully(const ProjectInfo& projectInfo, bool successfullyBuilt, bool saveToDisk)
        {
            AZStd::set<AZStd::string> builtPathsResult;
            bool success = GetBuiltSuccessfullyPaths(builtPathsResult);

            AZStd::string projectPath = projectInfo.m_path.toStdString().c_str();
            if (successfullyBuilt)
            {
                //Add successfully built path to set
                builtPathsResult.insert(projectPath);
            }
            else
            {
                // Remove unsuccessfully built path from set
                builtPathsResult.erase(projectPath);
            }

            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                QString builtKey = GetProjectsBuiltSuccessfullyKey();
                success = settingsRegistry->SetObject<AZStd::set<AZStd::string>>(builtKey.toStdString().c_str(), builtPathsResult);

                if (saveToDisk)
                {
                    SaveProjectManagerSettings();
                }
            }
            else
            {
                success = false;
            }

            return success;
        }

    } // namespace PMSettings
} // namespace O3DE::ProjectManager
