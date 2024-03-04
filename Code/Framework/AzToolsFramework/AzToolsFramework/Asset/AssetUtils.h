/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Gem/GemInfo.h>

class QString;

namespace AzToolsFramework::AssetUtils
{
    static constexpr const char* AssetImporterSettingsKey{ "/O3DE/SceneAPI/AssetImporter" };
    static constexpr const char* AssetImporterSupportedFileTypeKey{ "SupportedFileTypeExtensions" };

    //! Reads the "/Amazon/AssetProcessor/Settings/Platforms" entry from the settings registry
    //! to retrieve all enabled platforms
    void ReadEnabledPlatformsFromSettingsRegistry(AZ::SettingsRegistryInterface& settingsRegistry,
        AZStd::vector<AZStd::string>& enabledPlatforms);

    //! Returns all the enabledPlatforms by reading the specified config files in order.  
    AZStd::vector<AZStd::string> GetEnabledPlatforms(AZ::SettingsRegistryInterface& settingsRegistry,
        const AZStd::vector<AZ::IO::Path>& configFiles);

    //! Returns the platform specific AssetProcessorPlatformConfig.ini/setreg files
    //! Please note that config files are order dependent
    bool AddPlatformConfigFilePaths(AZStd::string_view root, AZStd::vector<AZ::IO::Path>& configFilePaths);

    //! Returns all the config files including the the platform and gems ones.
    //! Please note that config files are order dependent
    //! the root config file has the lowest priority.
    //! the project configuration file has the absolutely highest priority
    //! Also note that if the project has any "game project gems", then those will also be inserted last, 
    //! and thus have a higher priority than the root or non - project gems.
    //! Also note that the game project could be in a different location to the engine therefore we need the assetRoot param.
    AZStd::vector<AZ::IO::Path> GetConfigFiles(AZStd::string_view engineRoot, AZStd::string_view projectPath,
        bool addPlatformConfigs = true, bool addGemsConfigs = true, AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

    //! A utility function which checks the given path starting at the root and updates the relative path to be the actual case correct path.
    //! For example, if you pass it "c:\lumberyard\dev" as the root and "editor\icons\whatever.ico" as the relative path.
    //! It may update relativePathFromRoot to be "Editor\Icons\Whatever.ico" if such a casing is the actual physical case on disk already.
    //! @param root a trusted already-case-correct path (will not be case corrected). If empty it will be set to appRoot.
    //! @param relativePathFromRoot a non-trusted (may be incorrect case) path relative to rootPath,
    //!        which will be normalized and updated to be correct casing.
    //! @param checkEntirePath Optimization - set this to false if the caller is absolutely sure the path
    //!                             is correct and only the last element (file name or extension) is potentially incorrect, this can happen
    //!                             when for example taking a real file found from a real file directory that is already correct and
    //!                             replacing the file extension or file name only.
    //! @return if such a file does NOT exist, it returns FALSE, else returns TRUE.
    //! @note A very expensive function!  Call sparingly.
    bool UpdateFilePathToCorrectCase(AZStd::string_view root, AZStd::string& relativePathFromRoot, bool checkEntirePath = true);
} //namespace AzToolsFramework::AssetUtils
