/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SettingsRegistrar.h"

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ::DocumentPropertyEditor
{
    AZ::Outcome<void, AZStd::string> SettingsRegistrar::SaveSettingsToFile(
        AZ::IO::PathView relativeFilepath,
        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings,
        const AZStd::string& anchorKey,
        AZ::SettingsRegistryInterface* registry) const
    {
        registry = !registry ? AZ::SettingsRegistry::Get() : registry;
        if (!registry)
        {
            return AZ::Failure(AZStd::string::format("Failed to access global settings registry"));
        }

        constexpr const char* setregFileExt = ".setreg";
        if (relativeFilepath.Extension() != setregFileExt)
        {
            return AZ::Failure(AZStd::string::format(
                "Failed to save settings to file '%s': file must be of type '.setreg'", relativeFilepath.FixedMaxPathString().c_str()));
        }

        AZ::IO::FixedMaxPath fullSettingsPath = AZ::Utils::GetProjectPath();
        fullSettingsPath /= relativeFilepath;
        const char* posixSettingsPath = fullSettingsPath.AsPosix().c_str();

        AZStd::string stringBuffer;
        AZ::IO::ByteContainerStream stringStream(&stringBuffer);
        if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*registry, anchorKey, stringStream, dumperSettings))
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

    AZ::Outcome<bool, AZStd::string> SettingsRegistrar::LoadSettingsFromFile(
        AZ::IO::PathView relativeFilepath,
        AZStd::string_view anchorKey,
        AZ::SettingsRegistryInterface* registry,
        AZ::SettingsRegistryInterface::Format format) const
    {
        registry = !registry ? AZ::SettingsRegistry::Get() : registry;
        if (!registry)
        {
            return AZ::Failure(AZStd::string::format("Failed to access global settings registry"));
        }

        AZ::IO::FixedMaxPath fullSettingsPath = AZ::Utils::GetProjectPath();
        fullSettingsPath /= relativeFilepath;

        const bool fileExists = AZ::IO::SystemFile::Exists(fullSettingsPath.c_str());

        if (fileExists)
        {
            if (auto mergeResult = registry->MergeSettingsFile(fullSettingsPath.Native(), format, anchorKey);
                !mergeResult)
            {
                return AZ::Failure(mergeResult.GetMessages());
            }
        }

        return AZ::Success(fileExists);
    }

    bool SettingsRegistrar::RemoveSettingFromRegistry(AZStd::string_view registryPath, AZ::SettingsRegistryInterface* registry) const
    {
        registry = !registry ? AZ::SettingsRegistry::Get() : registry;
        if (!registry)
        {
            return false;
        }
        return registry->Remove(registryPath);
    }
} // namespace AZ::DocumentPropertyEditor
