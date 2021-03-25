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

#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzToolsFramework/Asset/AssetBundler.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <source/utils/utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/regex.h>
#include <cctype>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QDir>
#include <QString>
#include <QStringList>
AZ_POP_DISABLE_WARNING

namespace AssetBundler
{
    // General
    const char* AppWindowName = "AssetBundler";
    const char* AppWindowNameVerbose = "AssetBundlerVerbose";
    const char* HelpFlag = "help";
    const char* HelpFlagAlias = "h";
    const char* VerboseFlag = "verbose";
    const char* SaveFlag = "save";
    const char* PlatformArg = "platform";
    const char* PrintFlag = "print";
    const char* AssetCatalogFileArg = "overrideAssetCatalogFile";
    const char* AllowOverwritesFlag = "allowOverwrites";
    const char* IgnoreFileCaseFlag = "ignoreFileCase";
    const char* ProjectArg = "project";

    // Seeds
    const char* SeedsCommand = "seeds";
    const char* SeedListFileArg = "seedListFile";
    const char* AddSeedArg = "addSeed";
    const char* RemoveSeedArg = "removeSeed";
    const char* AddPlatformToAllSeedsFlag = "addPlatformToSeeds";
    const char* RemovePlatformFromAllSeedsFlag = "removePlatformFromSeeds";
    const char* UpdateSeedPathArg = "updateSeedPath";
    const char* RemoveSeedPathArg = "removeSeedPath";
    const char* DefaultProjectTemplatePath = "ProjectTemplates/DefaultTemplate/${ProjectName}";
    const char* ProjectName = "${ProjectName}";
    const char* DependenciesFileSuffix = "_Dependencies";
    const char* DependenciesFileExtension = "xml";

    // Asset Lists
    const char* AssetListsCommand = "assetLists";
    const char* AssetListFileArg = "assetListFile";
    const char* AddDefaultSeedListFilesFlag = "addDefaultSeedListFiles";
    const char* DryRunFlag = "dryRun";
    const char* GenerateDebugFileFlag = "generateDebugFile";
    const char* SkipArg = "skip";

    // Comparison Rules
    const char* ComparisonRulesCommand = "comparisonRules";
    const char* ComparisonRulesFileArg = "comparisonRulesFile";
    const char* ComparisonTypeArg = "comparisonType";
    const char* ComparisonFilePatternArg = "filePattern";
    const char* ComparisonFilePatternTypeArg = "filePatternType";
    const char* ComparisonTokenNameArg = "tokenName";
    const char* ComparisonFirstInputArg = "firstInput";
    const char* ComparisonSecondInputArg = "secondInput";
    const char* AddComparisonStepArg = "addComparison";
    const char* RemoveComparisonStepArg = "removeComparison";
    const char* MoveComparisonStepArg = "moveComparison";
    const char* EditComparisonStepArg = "editComparison";

    // Compare
    const char* CompareCommand = "compare";
    const char* CompareFirstFileArg = "firstAssetFile";
    const char* CompareSecondFileArg = "secondAssetFile";
    const char* CompareOutputFileArg = "output";
    const char* ComparePrintArg = "print";
    const char* IntersectionCountArg = "intersectionCount";

    // Bundle Settings 
    const char* BundleSettingsCommand = "bundleSettings";
    const char* BundleSettingsFileArg = "bundleSettingsFile";
    const char* OutputBundlePathArg = "outputBundlePath";
    const char* BundleVersionArg = "bundleVersion";
    const char* MaxBundleSizeArg = "maxSize";

    // Bundles
    const char* BundlesCommand = "bundles";

    // Bundle Seed
    const char* BundleSeedCommand = "bundleSeed";

    const char* AssetCatalogFilename = "assetcatalog.xml";

    char g_cachedEngineRoot[AZ_MAX_PATH_LEN];

    
    const char EngineDirectoryName[] = "Engine";
    const char RestrictedDirectoryName[] = "restricted";
    const char PlatformsDirectoryName[] = "Platforms";
    const char GemsDirectoryName[] = "Gems";
    const char GemsAssetsDirectoryName[] = "Assets";
    const char GemsSeedFileName[] = "seedList";
    const char EngineSeedFileName[] = "SeedAssetList";
    

    namespace Internal
    {
        const AZ::u32 PlatformFlags_RESTRICTED = aznumeric_cast<AZ::u32>(AzFramework::PlatformFlags::Platform_JASPER | AzFramework::PlatformFlags::Platform_PROVO | AzFramework::PlatformFlags::Platform_SALEM);

        void AddPlatformSeeds(
            AZStd::string rootFolder,
            const AZStd::string& rootFolderDisplayName,
            AZStd::unordered_map<AZStd::string, AZStd::string>& defaultSeedLists,
            AzFramework::PlatformFlags platformFlags)
        {
            AZ::IO::FixedMaxPath engineRoot(GetEngineRoot());
            AZ::IO::FixedMaxPath engineRestrcitedRoot = engineRoot / RestrictedDirectoryName;

            AZ::IO::FixedMaxPath inputPath = AZ::IO::FixedMaxPath(rootFolder);
            AZ::IO::FixedMaxPath engineLocalPath = inputPath.LexicallyRelative(engineRoot);

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            auto platformsIdxList = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);
            
            for (const AzFramework::PlatformId& platformId : platformsIdxList)
            {
                const AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platformId);
                const char* platformDirName = AzFramework::PlatformHelper::GetPlatformName(platformId);

                AZ::IO::FixedMaxPath platformDirectory;
                if (aznumeric_cast<AZ::u32>(platformFlag) & PlatformFlags_RESTRICTED)
                {
                    platformDirectory = engineRestrcitedRoot / platformDirName / engineLocalPath;
                }
                else
                {
                    platformDirectory = inputPath / PlatformsDirectoryName / platformDirName;
                }

                if (fileIO->Exists(platformDirectory.c_str()))
                {
                    bool recurse = true;
                    AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(platformDirectory.String(),
                        AZStd::string::format("*.%s", AzToolsFramework::AssetSeedManager::GetSeedFileExtension()).c_str(), recurse);

                    if (result.IsSuccess())
                    {
                        AZStd::list<AZStd::string> seedFiles = result.TakeValue();
                        for (AZStd::string& seedFile : seedFiles)
                        {
                            AZStd::string normalizedFilePath = seedFile;
                            AzFramework::StringFunc::Path::Normalize(seedFile);
                            defaultSeedLists[seedFile] = AZStd::string::format("%s (%s)", rootFolderDisplayName.c_str(), platformDirName);
                        }
                    }
                }
            }
        }

        void AddPlatformsDirectorySeeds(
            const AZStd::string& rootFolder,
            const AZStd::string& rootFolderDisplayName,
            AZStd::unordered_map<AZStd::string, AZStd::string>& defaultSeedLists,
            AzFramework::PlatformFlags platformFlags)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

            // Check whether platforms directory exists inside the root, if yes than add
            // * All seed files from the platforms directory
            // * All platform specific seed files based on the platform flags specified. 
            AZStd::string platformsDirectory;
            AzFramework::StringFunc::Path::Join(rootFolder.c_str(), PlatformsDirectoryName, platformsDirectory);
            if (fileIO->Exists(platformsDirectory.c_str()))
            {
                fileIO->FindFiles(platformsDirectory.c_str(),
                    AZStd::string::format("*.%s", AzToolsFramework::AssetSeedManager::GetSeedFileExtension()).c_str(),
                    [&](const char* fileName)
                    {
                        AZStd::string normalizedFilePath = fileName;
                        AzFramework::StringFunc::Path::Normalize(normalizedFilePath);
                        defaultSeedLists[normalizedFilePath] = rootFolderDisplayName;
                        return true;
                    });
            }

            AddPlatformSeeds(rootFolder, rootFolderDisplayName, defaultSeedLists, platformFlags);
        }
    }

    bool ComputeEngineRoot()
    {
        if (g_cachedEngineRoot[0])
        {
            return true;
        }

        const char* engineRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
        if (!engineRoot)
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Unable to locate engine root.\n");
            return false;
        }

        azstrcpy(g_cachedEngineRoot, AZ_MAX_PATH_LEN, engineRoot);
        return true;
    }

    const char* GetEngineRoot()
    {
        if (!g_cachedEngineRoot[0])
        {
            ComputeEngineRoot();
        }

        return g_cachedEngineRoot;
    }

    AZ::Outcome<void, AZStd::string> ComputeAssetAliasAndGameName(const AZStd::string& platformIdentifier, const AZStd::string& assetCatalogFile, AZStd::string& assetAlias, AZStd::string& gameName)
    {
        AZStd::string assetPath;
        AZStd::string gameFolder;
        if (!ComputeEngineRoot())
        {
            return AZ::Failure(AZStd::string("Unable to compute engine root.\n"));
        }
        if (assetCatalogFile.empty())
        {
            if (gameName.empty())
            {
                bool checkPlatform = false;
                bool result{};

                auto gameFolderKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%s",
                    AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, AzFramework::AssetSystem::ProjectName);
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    result = settingsRegistry->Get(gameFolder, gameFolderKey);
                }

                if (!result)
                {
                    return AZ::Failure(AZStd::string("Unable to locate game name in bootstrap.\n"));
                }

                gameName = gameFolder;
            }
            else
            {
                gameFolder = gameName;
            }

            // Appending Cache/%gamename%/%platform%/%gameName% to the engine root
            bool success = AzFramework::StringFunc::Path::ConstructFull(g_cachedEngineRoot, "Cache", assetPath) &&
                AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), gameFolder.c_str(), assetPath) &&
                AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), platformIdentifier.c_str(), assetPath);
            if (success)
            {
                AZStd::to_lower(gameFolder.begin(), gameFolder.end());
                success = AzFramework::StringFunc::Path::ConstructFull(assetPath.c_str(), gameFolder.c_str(), assetPath); // game name is lowercase
            }

            if (success)
            {
                assetAlias = assetPath;
            }
        }
        else if (AzFramework::StringFunc::Path::GetFullPath(assetCatalogFile.c_str(), assetPath))
        {
            AzFramework::StringFunc::Strip(assetPath, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, false, true);
            assetAlias = assetPath;

            // 3rd component from reverse should give us the correct case game name because the assetalias directory 
            // looks like ./Cache/%GameName%/%platform%/%gameName%/assetcatalog.xml
            // GetComponent util method returns the component with the separator appended at the end 
            // therefore we need to strip the separator to get the game name string
            gameFolder = AZ::IO::PathView(assetPath).ParentPath().ParentPath().Filename().Native();
            if (gameFolder.empty())
            {
                return AZ::Failure(AZStd::string::format("Unable to retrieve game name from assetCatalog file (%s).\n", assetCatalogFile.c_str()));
            }

            if (!AzFramework::StringFunc::Strip(gameFolder, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, false, true))
            {
                return AZ::Failure(AZStd::string::format("Unable to strip separator from game name (%s).\n", gameFolder.c_str()));
            }

            if (!gameName.empty() && !AzFramework::StringFunc::Equal(gameFolder.c_str(), gameName.c_str()))
            {
                return AZ::Failure(AZStd::string::format("Game name retrieved from the assetCatalog file (%s) does not match the inputted game name (%s).\n", gameFolder.c_str(), gameName.c_str()));
            }
            else
            {
                gameName = gameFolder;
            }
        }

        return AZ::Success();
    }

    void AddPlatformIdentifier(AZStd::string& filePath, const AZStd::string& platformIdentifier)
    {
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFileName(filePath.c_str(), fileName);

        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(filePath.c_str(), extension);
        AZStd::string platformSuffix = AZStd::string::format("_%s", platformIdentifier.c_str());

        fileName = AZStd::string::format("%s%s", fileName.c_str(), platformSuffix.c_str());
        AzFramework::StringFunc::Path::ReplaceFullName(filePath, fileName.c_str(), extension.c_str());
    }

    AzFramework::PlatformFlags GetPlatformsOnDiskForPlatformSpecificFile(const AZStd::string& platformIndependentAbsolutePath)
    {
        AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_NONE;

        auto allPlatformNames = AzFramework::PlatformHelper::GetPlatforms(AzFramework::PlatformFlags::AllNamedPlatforms);
        for (const auto& platformName : allPlatformNames)
        {
            AZStd::string filePath = platformIndependentAbsolutePath;
            AddPlatformIdentifier(filePath, platformName);
            if (AZ::IO::FileIOBase::GetInstance()->Exists(filePath.c_str()))
            {
                platformFlags = platformFlags | AzFramework::PlatformHelper::GetPlatformFlag(platformName);
            }
        }

        return platformFlags;
    }

    AZStd::unordered_map<AZStd::string, AZStd::string> GetDefaultSeedListFiles(const char* root, const char* projectName, const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlag)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

        // Add all seed list files of enabled gems for the given project
        AZStd::unordered_map<AZStd::string, AZStd::string> defaultSeedLists = GetGemSeedListFilePathToGemNameMap(gemInfoList, platformFlag);

        // Add the engine seed list file
        AZStd::string engineDirectory;
        AzFramework::StringFunc::Path::Join(root, EngineDirectoryName, engineDirectory);
        AZStd::string absoluteEngineSeedFilePath;
        AzFramework::StringFunc::Path::ConstructFull(engineDirectory.c_str(), EngineSeedFileName, AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteEngineSeedFilePath, true);
        if (fileIO->Exists(absoluteEngineSeedFilePath.c_str()))
        {
            defaultSeedLists[absoluteEngineSeedFilePath] = EngineDirectoryName;
        }

        // Add Seed Lists from the Platforms directory
        Internal::AddPlatformsDirectorySeeds(engineDirectory, EngineDirectoryName, defaultSeedLists, platformFlag);


        AZStd::string absoluteProjectDefaultSeedFilePath;
        AzFramework::StringFunc::Path::ConstructFull(root, projectName, EngineSeedFileName, AzToolsFramework::AssetSeedManager::GetSeedFileExtension(), absoluteProjectDefaultSeedFilePath, true);

        if (fileIO->Exists(absoluteProjectDefaultSeedFilePath.c_str()))
        {
            defaultSeedLists[absoluteProjectDefaultSeedFilePath] = projectName;
        }

        return defaultSeedLists;
    }

    AZStd::vector<AZStd::string> GetDefaultSeeds(const char* root, const char* projectName)
    {
        AZStd::vector<AZStd::string> defaultSeeds;

        defaultSeeds.emplace_back(GetProjectDependenciesAssetPath(root, projectName));

        return defaultSeeds;
    }

    AZStd::string GetProjectDependenciesFile(const char* root, const char* projectName)
    {
        AZStd::string projectDependenciesFilePath = AZStd::string::format("%s%s", projectName, DependenciesFileSuffix);
        AzFramework::StringFunc::Path::ConstructFull(root, projectName, projectDependenciesFilePath.c_str(), DependenciesFileExtension, projectDependenciesFilePath, true);
        return projectDependenciesFilePath;
    }

    AZStd::string GetProjectDependenciesFileTemplate(const char* root)
    {
        AZStd::string projectDependenciesFileTemplate = ProjectName;
        projectDependenciesFileTemplate += DependenciesFileSuffix;
        AzFramework::StringFunc::Path::ConstructFull(root, DefaultProjectTemplatePath, projectDependenciesFileTemplate.c_str(), DependenciesFileExtension, projectDependenciesFileTemplate, true);
        return projectDependenciesFileTemplate;
    }

    AZStd::string GetProjectDependenciesAssetPath(const char* root, const char* projectName)
    {
        AZStd::string projectDependenciesFile = AZStd::move(GetProjectDependenciesFile(root, projectName));
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(projectDependenciesFile.c_str()))
        {
            AZ_TracePrintf(AssetBundler::AppWindowName, "Project dependencies file %s doesn't exist.\n", projectDependenciesFile.c_str());

            AZStd::string projectDependenciesFileTemplate = AZStd::move(GetProjectDependenciesFileTemplate(root));
            if (AZ::IO::FileIOBase::GetInstance()->Copy(projectDependenciesFileTemplate.c_str(), projectDependenciesFile.c_str()))
            {
                AZ_TracePrintf(AssetBundler::AppWindowName, "Copied project dependencies file template %s to the current project.\n",
                    projectDependenciesFile.c_str());
            }
            else
            {
                AZ_Error(AppWindowName, false, "Failed to copy project dependencies file template %s from default project"
                    " template to the current project.\n", projectDependenciesFileTemplate.c_str());
                return {};
            }
        }

        // Turn the absolute path into a cache-relative path
        AZStd::string relativeProductPath;
        AzFramework::StringFunc::Path::GetFullFileName(projectDependenciesFile.c_str(), relativeProductPath);
        AZStd::to_lower(relativeProductPath.begin(), relativeProductPath.end());

        return relativeProductPath;
    }

    AZStd::unordered_map<AZStd::string, AZStd::string> GetGemSeedListFilePathToGemNameMap(const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags)
    {
        AZStd::unordered_map<AZStd::string, AZStd::string> filePathToGemNameMap;
        for (const AzToolsFramework::AssetUtils::GemInfo& gemInfo : gemInfoList)
        {
            AZ::IO::Path gemInfoAssetFilePath = gemInfo.m_absoluteFilePath;
            gemInfoAssetFilePath /= gemInfo.GetGemAssetFolder();
            AZ::IO::Path absoluteGemSeedFilePath = gemInfoAssetFilePath / GemsSeedFileName;
            absoluteGemSeedFilePath.ReplaceExtension(AZ::IO::PathView{ AzToolsFramework::AssetSeedManager::GetSeedFileExtension() });
            absoluteGemSeedFilePath = absoluteGemSeedFilePath.LexicallyNormal();

            AZStd::string gemName = gemInfo.m_gemName + " Gem";
            if (AZ::IO::FileIOBase::GetInstance()->Exists(absoluteGemSeedFilePath.c_str()))
            {
                filePathToGemNameMap[absoluteGemSeedFilePath.Native()] = gemName;
            }

            Internal::AddPlatformsDirectorySeeds(gemInfoAssetFilePath.Native(), gemName, filePathToGemNameMap, platformFlags);
        }

        return filePathToGemNameMap;
    }

    bool IsGemSeedFilePathValid(const char* root, AZStd::string seedAbsoluteFilePath, const AZStd::vector<AzToolsFramework::AssetUtils::GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

        if (!fileIO->Exists(seedAbsoluteFilePath.c_str()))
        {
            return false;
        }

        AZ::IO::Path gemsFolder{ root };
        gemsFolder /= GemsDirectoryName;
        gemsFolder /= GemsAssetsDirectoryName;
        gemsFolder = gemsFolder.LexicallyNormal();
        if (!AzFramework::StringFunc::StartsWith(seedAbsoluteFilePath, gemsFolder.Native()))
        {
            // if we are here it implies that this seed file does not live under the gems directory and
            // therefore we do not have to validate it
            return true;
        }

        for (const AzToolsFramework::AssetUtils::GemInfo& gemInfo : gemInfoList)
        {
            // We want to check the path before going through the effort of creating the default Seed List file map
            if (!AzFramework::StringFunc::StartsWith(seedAbsoluteFilePath, gemInfo.m_absoluteFilePath))
            {
                continue;
            }

            AZStd::unordered_map<AZStd::string, AZStd::string> seeds = GetGemSeedListFilePathToGemNameMap({gemInfo}, platformFlags);

            if (seeds.find(seedAbsoluteFilePath) != seeds.end())
            {
                return true;
            }
            // If we have not validated the input path yet, we need to keep looking, or we will return false negatives
            //    for Gems that have the same prefix in their name
        }

        return false;
    }

    AzFramework::PlatformFlags GetEnabledPlatformFlags(const char* root, const char* assetRoot, const char* gameName)
    {
        QStringList configFiles = AzToolsFramework::AssetUtils::GetConfigFiles(root, assetRoot, gameName, true, true);

        QStringList enabaledPlatformList = AzToolsFramework::AssetUtils::GetEnabledPlatforms(configFiles);
        AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_NONE;
        for (const QString& enabledPlatform : enabaledPlatformList)
        {
            AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlag(enabledPlatform.toUtf8().data());

            if (platformFlag != AzFramework::PlatformFlags::Platform_NONE)
            {
                platformFlags = platformFlags | platformFlag;
            }
            else
            {
                AZ_Warning(AssetBundler::AppWindowName, false, "Platform Helper is not aware of the platform (%s).\n ", enabledPlatform.toUtf8().data());
            }
        }

        return platformFlags;
    }

    void ValidateOutputFilePath(FilePath filePath, const char* format, ...)
    {
        if (!filePath.IsValid())
        {
            char message[MaxErrorMessageLength] = {};
            va_list args;
            va_start(args, format);
            azvsnprintf(message, MaxErrorMessageLength, format, args);
            va_end(args);
            AZ_Error(AssetBundler::AppWindowName, false, message);
        }
    }

    AZ::Outcome<AZStd::string, AZStd::string> GetCurrentProjectName()
    {
        AZStd::string gameName;
        bool result{ false };
        auto gameFolderKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%s",
            AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, AzFramework::AssetSystem::ProjectName);
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            result = settingsRegistry->Get(gameName, gameFolderKey);
        }

        if (result)
        {
            return AZ::Success(gameName);
        }
        else
        {
            return AZ::Failure(AZStd::string("Unable to locate current project name in bootstrap.cfg"));
        }
    }

    AZ::Outcome<AZStd::string, AZStd::string> GetProjectFolderPath(const AZStd::string& engineRoot, const AZStd::string& projectName)
    {
        AZStd::string projectFolderPath;
        bool success = AzFramework::StringFunc::Path::ConstructFull(engineRoot.c_str(), projectName.c_str(), projectFolderPath);

        if (success && AZ::IO::FileIOBase::GetInstance()->Exists(projectFolderPath.c_str()))
        {
            return AZ::Success(projectFolderPath);
        }
        else
        {
            return AZ::Failure(AZStd::string::format( "Unable to locate the current Project folder: %s", projectName.c_str()));
        }
    }

    AZ::Outcome<AZStd::string, AZStd::string> GetProjectCacheFolderPath(const AZStd::string& engineRoot, const AZStd::string& projectName)
    {
        AZStd::string projectCacheFolderPath;

        bool success = AzFramework::StringFunc::Path::ConstructFull(engineRoot.c_str(), "Cache", projectCacheFolderPath);
        if (!success || !AZ::IO::FileIOBase::GetInstance()->Exists(projectCacheFolderPath.c_str()))
        {
            return AZ::Failure(AZStd::string::format(
                "Unable to locate the Cache in the engine directory: %s. Please run the Lumberyard Asset Processor to generate a Cache and build assets.", 
                engineRoot.c_str()));
        }

        success = AzFramework::StringFunc::Path::ConstructFull(projectCacheFolderPath.c_str(), projectName.c_str(), projectCacheFolderPath);
        if (success && AZ::IO::FileIOBase::GetInstance()->Exists(projectCacheFolderPath.c_str()))
        {
            return AZ::Success(projectCacheFolderPath);
        }
        else
        {
            return AZ::Failure(AZStd::string::format(
                "Unable to locate the current Project in the Cache folder: %s. Please run the Lumberyard Asset Processor to generate a Cache and build assets.",
                projectName.c_str()));
        }
    }

    AZ::Outcome<void, AZStd::string> GetPlatformNamesFromCacheFolder(const AZStd::string& projectCacheFolder, AZStd::vector<AZStd::string>& platformNames)
    {
        QDir projectCacheDir(QString(projectCacheFolder.c_str()));
        auto tempPlatformList = projectCacheDir.entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot);

        if (tempPlatformList.empty())
        {
            return AZ::Failure(AZStd::string("Cache is empty. Please run the Lumberyard Asset Processor to generate a Cache and build assets."));
        }

        for (const QString& platform : tempPlatformList)
        {
            platformNames.push_back(AZStd::string(platform.toUtf8().data()));
        }

        return AZ::Success();
    }

    AZ::Outcome<AZStd::string, AZStd::string> GetAssetCatalogFilePath(const char* pathToCacheFolder, const char* platformIdentifier, const char* projectName)
    {
        AZStd::string assetCatalogFilePath;
        bool success = AzFramework::StringFunc::Path::ConstructFull(pathToCacheFolder, platformIdentifier, assetCatalogFilePath, true);

        if (!success)
        {
            return AZ::Failure(AZStd::string::format(
                "Unable to find platform folder %s in cache found at: %s. Please run the Lumberyard Asset Processor to generate platform-specific cache folders and build assets.", 
                platformIdentifier, 
                pathToCacheFolder));
        }

        // Project name is lower case in the platform-specific cache folder
        AZStd::string lowerCaseProjectName = AZStd::string(projectName);
        AZStd::to_lower(lowerCaseProjectName.begin(), lowerCaseProjectName.end());
        success = AzFramework::StringFunc::Path::ConstructFull(assetCatalogFilePath.c_str(), lowerCaseProjectName.c_str(), assetCatalogFilePath)
            && AzFramework::StringFunc::Path::ConstructFull(assetCatalogFilePath.c_str(), AssetCatalogFilename, assetCatalogFilePath);

        if (!success)
        {
            return AZ::Failure(AZStd::string("Unable to find the asset catalog. Please run the Lumberyard Asset Processor to generate a Cache and build assets."));
        }

        return AZ::Success(assetCatalogFilePath);
    }

    AZStd::string GetPlatformSpecificCacheFolderPath(const AZStd::string& projectSpecificCacheFolderAbsolutePath, const AZStd::string& platform, const AZStd::string& projectName)
    {
        // C:/dev/Cache/ProjectName -> C:/dev/Cache/ProjectName/platform
        AZStd::string platformSpecificCacheFolderPath;
        AzFramework::StringFunc::Path::ConstructFull(projectSpecificCacheFolderAbsolutePath.c_str(), platform.c_str(), platformSpecificCacheFolderPath, true);

        // C:/dev/Cache/ProjectName/platform -> C:/dev/Cache/ProjectName/platform/projectname
        AZStd::string lowerCaseProjectName = AZStd::string(projectName);
        AZStd::to_lower(lowerCaseProjectName.begin(), lowerCaseProjectName.end());
        AzFramework::StringFunc::Path::ConstructFull(platformSpecificCacheFolderPath.c_str(), lowerCaseProjectName.c_str(), platformSpecificCacheFolderPath, true);

        return platformSpecificCacheFolderPath;
    }

    AZStd::string GenerateKeyFromAbsolutePath(const AZStd::string& absoluteFilePath)
    {
        AZStd::string key(absoluteFilePath);
        AzFramework::StringFunc::Path::Normalize(key);
        AzFramework::StringFunc::Path::StripDrive(key);
        return key;
    }

    void ConvertToRelativePath(const AZStd::string& parentFolderPath, AZStd::string& absoluteFilePath)
    {
        // Qt and AZ return different Drive Letter formats, so strip them away before doing a comparison
        AZStd::string parentFolderPathWithoutDrive(parentFolderPath);
        AzFramework::StringFunc::Path::StripDrive(parentFolderPathWithoutDrive);
        AzFramework::StringFunc::Path::Normalize(parentFolderPathWithoutDrive);

        AzFramework::StringFunc::Path::StripDrive(absoluteFilePath);
        AzFramework::StringFunc::Path::Normalize(absoluteFilePath);

        AzFramework::StringFunc::Replace(absoluteFilePath, parentFolderPathWithoutDrive.c_str(), "");
    }

    AZ::Outcome<void, AZStd::string> MakePath(const AZStd::string& path)
    {
        // Create the folder if it does not already exist
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(path.c_str()))
        {
            auto result = AZ::IO::FileIOBase::GetInstance()->CreatePath(path.c_str());
            if (!result)
            {
                return AZ::Failure(AZStd::string::format("Path creation failed. Input path: %s \n", path.c_str()));
            }
        }

        return AZ::Success();
    }

    WarningAbsorber::WarningAbsorber()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    WarningAbsorber::~WarningAbsorber()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    bool WarningAbsorber::OnWarning(const char* window, const char* message)
    {
        AZ_UNUSED(window);
        AZ_UNUSED(message);
        return true; // do not forward
    }

    bool WarningAbsorber::OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message)
    {
        AZ_UNUSED(window);
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);
        AZ_UNUSED(message);
        return true; // do not forward
    }

    FilePath::FilePath(const AZStd::string& filePath, AZStd::string platformIdentifier, bool checkFileCase, bool ignoreFileCase)
    {
        AZStd::string platform = platformIdentifier;
        if (!platform.empty())
        {
            AZStd::string filePlatform = AzToolsFramework::GetPlatformIdentifier(filePath);
            if (!filePlatform.empty())
            {
                // input file path already has a platform, no need to append a platform id
                platform = AZStd::string();

                if (!AzFramework::StringFunc::Equal(filePlatform.c_str(), platformIdentifier.c_str(), true))
                {
                    // Platform identifier does not match the current platform
                    return;
                }
            }
        }

        if (!filePath.empty())
        {
            m_validPath = true;
            m_originalPath = m_absolutePath = filePath;
            AzFramework::StringFunc::Path::Normalize(m_originalPath);
            ComputeAbsolutePath(m_absolutePath, platform, checkFileCase, ignoreFileCase);
        }
    }


    FilePath::FilePath(const AZStd::string& filePath, bool checkFileCase, bool ignoreFileCase)
        :FilePath(filePath, AZStd::string(), checkFileCase, ignoreFileCase)
    {
    }

    const AZStd::string& FilePath::AbsolutePath() const
    {
        return m_absolutePath;
    }

    const AZStd::string& FilePath::OriginalPath() const
    {
        return m_originalPath;
    }

    bool FilePath::IsValid() const
    {
        return m_validPath;
    }

    AZStd::string FilePath::ErrorString() const
    {
        return m_errorString;
    }

    void FilePath::ComputeAbsolutePath(AZStd::string& filePath, const AZStd::string& platformIdentifier, bool checkFileCase, bool ignoreFileCase)
    {
        if (AzToolsFramework::AssetFileInfoListComparison::IsTokenFile(filePath))
        {
            return;
        }

        if (!platformIdentifier.empty())
        {
            AssetBundler::AddPlatformIdentifier(filePath, platformIdentifier);
        }

        const char* appRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(appRoot, &AzFramework::ApplicationRequests::GetAppRoot);

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        AZStd::string driveString;
        AzFramework::StringFunc::Path::GetDrive(appRoot, driveString);

        if (AzFramework::StringFunc::FirstCharacter(filePath.c_str()) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
        {
            AzFramework::StringFunc::Path::ConstructFull(driveString.c_str(), filePath.c_str(), filePath, true);
        }
#endif

        if (!AzFramework::StringFunc::Path::IsRelative(filePath.c_str()))
        {
            // it is already an absolute path
            AzFramework::StringFunc::Path::Normalize(filePath);
        }
        else
        {
            AzFramework::StringFunc::Path::ConstructFull(appRoot, m_absolutePath.c_str(), m_absolutePath, true);
        }

        if (checkFileCase)
        {
            QDir rootDir(appRoot);
            QString relFilePath = rootDir.relativeFilePath(m_absolutePath.c_str());
            if (AzToolsFramework::AssetUtils::UpdateFilePathToCorrectCase(QString(appRoot), relFilePath))
            {
                if (ignoreFileCase)
                {
                    AzFramework::StringFunc::Path::ConstructFull(appRoot, relFilePath.toUtf8().data(), m_absolutePath, true);
                }
                else
                {
                    AZStd::string absfilePath(rootDir.filePath(relFilePath).toUtf8().data());
                    AzFramework::StringFunc::Path::Normalize(absfilePath);
                    if (!AZ::StringFunc::Equal(absfilePath.c_str(), m_absolutePath.c_str(), true))
                    {
                        m_errorString = AZStd::string::format("File case mismatch, file ( %s ) does not exist on disk, did you mean file ( %s ). \
Please run the command again with the correct file path or use ( --%s ) arg if you want to allow case insensitive file match.\n",
m_absolutePath.c_str(), rootDir.filePath(relFilePath.toUtf8().data()).toUtf8().data(), IgnoreFileCaseFlag);
                        m_validPath = false;
                    }
                }
            }
        }
    }

    ScopedTraceHandler::ScopedTraceHandler()
    {
        BusConnect();
    }

    ScopedTraceHandler::~ScopedTraceHandler()
    {
        BusDisconnect();
    }

    bool ScopedTraceHandler::OnError(const char* window, const char* message)
    {
        AZ_UNUSED(window);
        if (m_reportingError)
        {
            // if we are reporting error than we dont want to store errors again.
            return false;
        }

        m_errors.emplace_back(message);
        return true;
    }

    int ScopedTraceHandler::GetErrorCount() const
    {
        return static_cast<int>(m_errors.size());
    }

    void ScopedTraceHandler::ReportErrors()
    {
        m_reportingError = true;
        for (const AZStd::string& error : m_errors)
        {
            AZ_Error(AssetBundler::AppWindowName, false, error.c_str());
        }

        ClearErrors();
        m_reportingError = false;
    }

    void ScopedTraceHandler::ClearErrors()
    {
        m_errors.clear();
        m_errors.swap(AZStd::vector<AZStd::string>());
    }

    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::ComparisonType, AZStd::string> ParseComparisonType(const AZStd::string& comparisonType)
    {
        using namespace AzToolsFramework;

        const size_t numTypes = AZ_ARRAY_SIZE(AssetFileInfoListComparison::ComparisonTypeNames);

        int comparisonTypeIndex = 0;
        if (AzFramework::StringFunc::LooksLikeInt(comparisonType.c_str(), &comparisonTypeIndex))
        {
            // User passed in a number
            if (comparisonTypeIndex < numTypes)
            {
                return AZ::Success(static_cast<AssetFileInfoListComparison::ComparisonType>(comparisonTypeIndex));
            }
        }
        else
        {
            // User passed in the name of a ComparisonType
            for (size_t i = 0; i < numTypes; ++i)
            {
                if (AzFramework::StringFunc::Equal(comparisonType.c_str(), AssetFileInfoListComparison::ComparisonTypeNames[i]))
                {
                    return AZ::Success(static_cast<AssetFileInfoListComparison::ComparisonType>(i));
                }
            }
        }

        // Failure case
        AZStd::string failureMessage = AZStd::string::format("Invalid Comparison Type ( %s ).  Valid types are: ", comparisonType.c_str());
        for (size_t i = 0; i < numTypes - 1; ++i)
        {
            failureMessage.append(AZStd::string::format("%s, ", AssetFileInfoListComparison::ComparisonTypeNames[i]));
        }
        failureMessage.append(AZStd::string::format("and %s.", AssetFileInfoListComparison::ComparisonTypeNames[numTypes - 1]));
        return AZ::Failure(failureMessage);
    }

    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::FilePatternType, AZStd::string> ParseFilePatternType(const AZStd::string& filePatternType)
    {
        using namespace AzToolsFramework;

        const size_t numTypes = AZ_ARRAY_SIZE(AssetFileInfoListComparison::FilePatternTypeNames);

        int filePatternTypeIndex = 0;
        if (AzFramework::StringFunc::LooksLikeInt(filePatternType.c_str(), &filePatternTypeIndex))
        {
            // User passed in a number
            if (filePatternTypeIndex < numTypes)
            {
                return AZ::Success(static_cast<AssetFileInfoListComparison::FilePatternType>(filePatternTypeIndex));
            }
        }
        else
        {
            // User passed in the name of a FilePatternType
            for (size_t i = 0; i < numTypes; ++i)
            {
                if (AzFramework::StringFunc::Equal(filePatternType.c_str(), AssetFileInfoListComparison::FilePatternTypeNames[i]))
                {
                    return AZ::Success(static_cast<AssetFileInfoListComparison::FilePatternType>(i));
                }
            }
        }

        // Failure case
        AZStd::string failureMessage = AZStd::string::format("Invalid File Pattern Type ( %s ).  Valid types are: ", filePatternType.c_str());
        for (size_t i = 0; i < numTypes - 1; ++i)
        {
            failureMessage.append(AZStd::string::format("%s, ", AssetFileInfoListComparison::FilePatternTypeNames[i]));
        }
        failureMessage.append(AZStd::string::format("and %s.", AssetFileInfoListComparison::FilePatternTypeNames[numTypes - 1]));
        return AZ::Failure(failureMessage);
    }

    bool LooksLikePath(const AZStd::string& inputString)
    {
        for (auto thisChar : inputString)
        {
            if (thisChar == '.' || thisChar == AZ_CORRECT_FILESYSTEM_SEPARATOR || thisChar == AZ_WRONG_FILESYSTEM_SEPARATOR)
            {
                return true;
            }
        }
        return false;
    }

    bool LooksLikeWildcardPattern(const AZStd::string& inputPattern)
    {
        for (auto thisChar : inputPattern)
        {
            if (thisChar == '*' || thisChar == '?')
            {
                return true;
            }
        }
        return false;
    }
}
