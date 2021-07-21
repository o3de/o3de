/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetSeedUtil.h"
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

namespace AssetValidation::AssetSeed
{
    void AddPlatformSeeds(const AZStd::string& rootFolder, AZStd::vector<AZStd::string>& defaultSeedLists, AzFramework::PlatformFlags platformFlags)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        auto platforms = AzFramework::PlatformHelper::GetPlatformsInterpreted(platformFlags);

        for (auto& platform : platforms)
        {
            AZStd::string platformDirectory;
            AZ::StringFunc::Path::Join(rootFolder.c_str(), platform.data(), platformDirectory);
            if (fileIO->Exists(platformDirectory.c_str()))
            {
                bool recurse = true;
                AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(platformDirectory,
                    AZStd::string::format("*.%s", SeedFileExtension).c_str(), recurse);

                if (result.IsSuccess())
                {
                    AZStd::list<AZStd::string> seedFiles = result.TakeValue();
                    for (AZStd::string& seedFile : seedFiles)
                    {
                        AZStd::string normalizedFilePath = seedFile;
                        AZ::StringFunc::Path::Normalize(seedFile);
                        defaultSeedLists.emplace_back(seedFile);
                    }
                }
            }
        }
    }

    void AddPlatformsDirectorySeeds(const AZStd::string& rootFolder, AZStd::vector<AZStd::string>& defaultSeedLists, AzFramework::PlatformFlags platformFlags)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

        // Check whether platforms directory exists inside the root, if yes than add
        // * All seed files from the platforms directory
        // * All platform specific seed files based on the platform flags specified. 
        AZStd::string platformsDirectory;
        AZ::StringFunc::Path::Join(rootFolder.c_str(), PlatformsDirectoryName, platformsDirectory);
        if (fileIO->Exists(platformsDirectory.c_str()))
        {
            fileIO->FindFiles(platformsDirectory.c_str(),
                AZStd::string::format("*.%s", SeedFileExtension).c_str(),
                [&](const char* fileName)
            {
                AZStd::string normalizedFilePath = fileName;
                AZ::StringFunc::Path::Normalize(normalizedFilePath);
                defaultSeedLists.emplace_back(normalizedFilePath);
                return true;
            });
        }

        AddPlatformSeeds(platformsDirectory, defaultSeedLists, platformFlags);
    }

    AZStd::vector<AZStd::string> GetGemSeedListFiles(const AZStd::vector<AzFramework::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags)
    {
        AZStd::vector<AZStd::string> gemSeedListFiles;
        for (const AzFramework::GemInfo& gemInfo : gemInfoList)
        {
            for (AZ::IO::Path absoluteGemAssetPath : gemInfo.m_absoluteSourcePaths)
            {
                absoluteGemAssetPath /= AzFramework::GemInfo::GetGemAssetFolder();

                AZ::IO::Path absoluteGemSeedFilePath = absoluteGemAssetPath / GemsSeedFileName;
                absoluteGemSeedFilePath.ReplaceExtension(SeedFileExtension);

                if (AZ::IO::FileIOBase::GetInstance()->Exists(absoluteGemSeedFilePath.c_str()))
                {
                    gemSeedListFiles.emplace_back(absoluteGemSeedFilePath);
                }

                AddPlatformsDirectorySeeds(absoluteGemAssetPath.Native(), gemSeedListFiles, platformFlags);
            }
        }

        return gemSeedListFiles;
    }

    AZStd::vector<AZStd::string> GetDefaultSeedListFiles(const AZStd::vector<AzFramework::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlag)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZ_Assert(settingsRegistry, "Global Settings registry must be available to retrieve default seed list");

        AZ::IO::Path engineRoot;
        settingsRegistry->Get(engineRoot.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);

        // Add all seed list files of enabled gems for the given project
        AZStd::vector<AZStd::string> defaultSeedLists = GetGemSeedListFiles(gemInfoList, platformFlag);

        // Add the engine seed list file
        AZ::IO::Path engineSourceAssetsDirectory = engineRoot / EngineDirectoryName;
        AZ::IO::Path absoluteEngineSeedFilePath = engineSourceAssetsDirectory;
        absoluteEngineSeedFilePath /= EngineSeedFileName;
        absoluteEngineSeedFilePath.ReplaceExtension(SeedFileExtension);
        if (fileIO->Exists(absoluteEngineSeedFilePath.c_str()))
        {
            defaultSeedLists.emplace_back(AZStd::move(absoluteEngineSeedFilePath.LexicallyNormal().Native()));
        }

        AddPlatformsDirectorySeeds(engineSourceAssetsDirectory.Native(), defaultSeedLists, platformFlag);

        // Add the current project default seed list file
        AZStd::string projectName;

        AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
        if (!projectPath.empty())
        {
            AZ::IO::FixedMaxPath absoluteProjectDefaultSeedFilePath{ engineRoot };
            absoluteProjectDefaultSeedFilePath /= projectPath;
            absoluteProjectDefaultSeedFilePath /= EngineSeedFileName;
            absoluteProjectDefaultSeedFilePath.ReplaceExtension(SeedFileExtension);

            if (fileIO->Exists(absoluteProjectDefaultSeedFilePath.c_str()))
            {
                defaultSeedLists.emplace_back(absoluteProjectDefaultSeedFilePath.LexicallyNormal().String());
            }
        }
        return defaultSeedLists;
    }
}
