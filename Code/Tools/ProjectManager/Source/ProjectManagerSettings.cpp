/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProjectManagerSettings.h"

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Utils/Utils.h>

namespace O3DE::ProjectManager
{
    void SaveProjectManagerSettings()
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

    QString GetProjectBuiltSuccessfullyKey(const QString& projectName)
    {
        return QString("%1/Projects/%2/BuiltSuccessfully").arg(ProjectManagerKeyPrefix).arg(projectName);
    }
}
