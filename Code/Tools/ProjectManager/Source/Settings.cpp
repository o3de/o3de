/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Settings.h>

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Utils/Utils.h>

namespace O3DE::ProjectManager
{
    Settings::Settings(bool saveToDisk)
        : m_saveToDisk(saveToDisk)
    {
        m_settingsRegistry = AZ::SettingsRegistry::Get();

        AZ_Assert(m_settingsRegistry, "Failed to create Settings");
    }

    void Settings::Save()
    {
        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
        dumperSettings.m_prettifyOutput = true;
        dumperSettings.m_jsonPointerPrefix = ProjectManagerKeyPrefix;

        AZStd::string stringBuffer;
        AZ::IO::ByteContainerStream stringStream(&stringBuffer);
        if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(
                *m_settingsRegistry, ProjectManagerKeyPrefix, stringStream, dumperSettings))
        {
            AZ_Warning("ProjectManager", false, "Could not save Project Manager settings to stream");
            return;
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
    }

    void Settings::OnSettingsChanged()
    {
        if (m_saveToDisk)
        {
            Save();
        }
    }

    bool Settings::Get(QString& result, const QString& settingsKey)
    {
        bool success = false;

        AZStd::string settingsValue;
        success = m_settingsRegistry->Get(settingsValue, settingsKey.toStdString().c_str());

        result = settingsValue.c_str();
        return success;
    }

    bool Settings::Get(bool& result, const QString& settingsKey)
    {
        return m_settingsRegistry->Get(result, settingsKey.toStdString().c_str());
    }

    bool Settings::Set(const QString& settingsKey, const QString& settingsValue)
    {
        bool success = false;

        success = m_settingsRegistry->Set(settingsKey.toStdString().c_str(), settingsValue.toStdString().c_str());
        OnSettingsChanged();

        return success;
    }

    bool Settings::Set(const QString& settingsKey, bool settingsValue)
    {
        bool success = false;

        success = m_settingsRegistry->Set(settingsKey.toStdString().c_str(), settingsValue);
        OnSettingsChanged();

        return success;
    }

    bool Settings::Remove(const QString& settingsKey)
    {
        bool success = false;

        success = m_settingsRegistry->Remove(settingsKey.toStdString().c_str());
        OnSettingsChanged();

        return success;
    }

    bool Settings::Copy(const QString& settingsKeyOrig, const QString& settingsKeyDest, bool removeOrig)
    {
        bool success = false;
        AZStd::string settingsValue;

        success = m_settingsRegistry->Get(settingsValue, settingsKeyOrig.toStdString().c_str());

        if (success)
        {
            success = m_settingsRegistry->Set(settingsKeyDest.toStdString().c_str(), settingsValue);
            if (success)
            {
                if (removeOrig)
                {
                    success = m_settingsRegistry->Remove(settingsKeyOrig.toStdString().c_str());
                }
                OnSettingsChanged();
            }
        }

        return success;
    }

    QString Settings::GetProjectKey(const ProjectInfo& projectInfo)
    {
        return QString("%1/Projects/%2/%3").arg(ProjectManagerKeyPrefix, projectInfo.m_id, projectInfo.m_projectName);
    }

    bool Settings::GetBuiltSuccessfullyPaths(AZStd::set<AZStd::string>& result)
    {
        return m_settingsRegistry->GetObject<AZStd::set<AZStd::string>>(result, ProjectsBuiltSuccessfullyKey);
    }

    bool Settings::GetProjectBuiltSuccessfully(bool& result, const ProjectInfo& projectInfo)
    {
        AZStd::set<AZStd::string> builtPathsResult;
        bool success = GetBuiltSuccessfullyPaths(builtPathsResult);

        // Check if buildPath is listed as successfully built
        AZStd::string projectPath = projectInfo.m_path.toStdString().c_str();
        if (builtPathsResult.contains(projectPath))
        {
            result = true;
        }
        // No project built statuses known
        else
        {
            result = false;
        }

        return success;
    }

    bool Settings::SetProjectBuiltSuccessfully(const ProjectInfo& projectInfo, bool successfullyBuilt)
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

        success = m_settingsRegistry->SetObject<AZStd::set<AZStd::string>>(ProjectsBuiltSuccessfullyKey, builtPathsResult);
        OnSettingsChanged();

        return success;
    }

} // namespace O3DE::ProjectManager
