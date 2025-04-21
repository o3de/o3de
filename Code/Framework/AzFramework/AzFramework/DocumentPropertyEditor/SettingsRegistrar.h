/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

namespace AZ::DocumentPropertyEditor
{
    //! Class wrapping file management required to save SettingsRegistry data to file as well as load
    //! json data from file to the SettingsRegistry.
    class SettingsRegistrar
    {
    public:
        //! Saves all settings stored at the given SettingsRegistry key into a file at the given filepath. The
        //! file and directory structure will be created if it does now exist. The file open mode is set to
        //! overwrite existing file contents.
        //! The path passed to this function is expected to be relative to the project root.
        //! The DumperSettings can be used to narrow down the settings data dumped to file by
        //! providing an filter function.
        //! Specifying a json pointer path prefix in the DumperSettings may be required in order
        //! for the dumped json data to match the in-memory path structure.
        //! The anchor key specifies a path in the registry where the settings will be saved.
        AZ::Outcome<void, AZStd::string> SaveSettingsToFile(
            AZ::IO::PathView relativeFilepath,
            AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings,
            const AZStd::string& anchorKey = {},
            AZ::SettingsRegistryInterface* registry = nullptr) const;

        //! Loads settings from the provided '.setreg' file and merges settings into the SettingsRegistry
        //! at the given anchor key.
        //! The path passed to this function is expected to be relative to the project root.
        //! Succeeds if the file was loaded or if the file does not exist.
        //! Upon success, returns whether the file exists or not.
        AZ::Outcome<bool, AZStd::string> LoadSettingsFromFile(
            AZ::IO::PathView relativeFilepath,
            AZStd::string_view anchorKey = {},
            AZ::SettingsRegistryInterface* registry = nullptr,
            AZ::SettingsRegistryInterface::Format format = AZ::SettingsRegistryInterface::Format::JsonMergePatch) const;

        //! Attempts to retrieve settings stored at the given registry path and put them in the provided object.
        //! The provided object is expected to be intialized before being passed to this function and it must
        //! be AZ_RTTI enabled.
        //! Succeeds if the given entry was read or if it does not exist.
        //! Upon success, returns whether the given settings entry exists or not.
        template<typename AzRttiEnabled_T>
        AZ::Outcome<bool, AZStd::string> GetObjectSettings(
            AzRttiEnabled_T* outSettingsObject,
            AZStd::string_view registryPath,
            AZ::SettingsRegistryInterface* registry = nullptr) const
        {
            registry = !registry ? AZ::SettingsRegistry::Get() : registry;
            if (!registry)
            {
                return AZ::Failure(AZStd::string::format("Failed to access global settings registry for data retrieval"));
            }

            bool entryExists = registry->GetType(registryPath) != AZ::SettingsRegistryInterface::Type::NoType;

            if (entryExists && !registry->GetObject(*outSettingsObject, registryPath))
            {
                return AZ::Failure(AZStd::string::format("Failed to retrieve settings at path '%s'", registryPath.data()));
            }

            return AZ::Success(entryExists);
        }

        //! Attempts to store reflected properties of the given AZ_RTTI enabled object at the given registry path.
        template<typename AzRttiEnabled_T>
        AZ::Outcome<void, AZStd::string> StoreObjectSettings(
            AZStd::string_view registryPath,
            AzRttiEnabled_T* settingsObject,
            AZ::SettingsRegistryInterface* registry = nullptr) const
        {
            registry = !registry ? AZ::SettingsRegistry::Get() : registry;
            if (!registry)
            {
                return AZ::Failure(AZStd::string::format("Failed to access global settings registry for data storage"));
            }

            if (registry->SetObject(registryPath, *settingsObject))
            {
                return AZ::Success();
            }
            else
            {
                return AZ::Failure(AZStd::string::format("Failed to store settings at path '%s'", registryPath.data()));
            }
        }

        bool RemoveSettingFromRegistry(AZStd::string_view registryPath, AZ::SettingsRegistryInterface* registry = nullptr) const;

        //! The file extension expected for SettingsRegistry files.
        static constexpr const char* SettingsRegistryFileExt = AZ::SettingsRegistryInterface::Extension;
    };
} // namespace AZ::DocumentPropertyEditor
