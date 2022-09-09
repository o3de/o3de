/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SettingsRegistrar.h"

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AzToolsFramework
{
    AZ::Outcome<void, AZStd::string> SettingsRegistrar::SaveSettingsToFile(
        const AZStd::string& relativeFilepath,
        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings,
        const AZStd::string& rootSearchKey) const
    {
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            return AZ::Failure(AZStd::string::format("Failed to access global settings registry"));
        }

        constexpr const char* setregFileExt = ".setreg";
        if (AZ::IO::Path(relativeFilepath).Extension() != setregFileExt)
        {
            return AZ::Failure(AZStd::string::format(
                "Failed to save settings to file '%s': file must be of type '.setreg'", relativeFilepath.c_str()));
        }

        AZ::IO::FixedMaxPath fullSettingsPath = AZ::Utils::GetProjectPath();
        fullSettingsPath /= relativeFilepath;
        const char* posixSettingsPath = fullSettingsPath.AsPosix().c_str();

        AZStd::string stringBuffer;
        AZ::IO::ByteContainerStream stringStream(&stringBuffer);
        if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*registry, rootSearchKey, stringStream, dumperSettings))
        {
            return AZ::Failure(AZStd::string::format(
                "Failed to save settings to file '%s': failed to retrieve settings from registry", posixSettingsPath));
        }

        constexpr auto openMode = AZ::IO::SystemFile::SF_OPEN_CREATE
            | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH
            | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
        if (AZ::IO::SystemFile outputFile; outputFile.Open(posixSettingsPath, openMode))
        {
            if(outputFile.Write(stringBuffer.data(), stringBuffer.size()) != stringBuffer.size())
            {
                return AZ::Failure(AZStd::string::format(
                    "Failed to save settings to file '%s': incomplete contents written", posixSettingsPath));
            }
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> SettingsRegistrar::LoadSettingsFromFile(
        AZStd::string_view relativeFilepath,
        AZStd::string_view anchorKey,
        AZ::SettingsRegistryInterface::Format format) const
    {
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            return AZ::Failure(AZStd::string::format("Failed to access global settings registry"));
        }

        AZ::IO::FixedMaxPath fullSettingsPath = AZ::Utils::GetProjectPath();
        fullSettingsPath /= relativeFilepath;
        const char* posixSettingsPath = fullSettingsPath.AsPosix().c_str();

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(fullSettingsPath.c_str()))
        {
            return AZ::Failure(AZStd::string::format("Settings file does not exist: '%s'", posixSettingsPath)); 
        }

        if (registry->MergeSettingsFile(posixSettingsPath, format, anchorKey))
        {
            return AZ::Success();
        }
        else
        {
            return AZ::Failure(AZStd::string::format("Failed to merge settings file '%s': check log for errors", posixSettingsPath));
        }
    }

    bool SettingsRegistrar::RemoveSettingFromRegistry(AZStd::string_view registryPath) const
    {
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        if (!registry)
        {
            return false;
        }
        return registry->Remove(registryPath);
    }
} // namespace AzToolsFramework
