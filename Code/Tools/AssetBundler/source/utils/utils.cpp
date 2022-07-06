/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <AzCore/Utils/Utils.h>
#include <cctype>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QDir>
#include <QString>
#include <QStringList>
#include <QJsonDocument>
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
    const char* ProjectArg = "project-path";

    // Seeds
    const char* SeedsCommand = "seeds";
    const char* SeedListFileArg = "seedListFile";
    const char* AddSeedArg = "addSeed";
    const char* RemoveSeedArg = "removeSeed";
    const char* AddPlatformToAllSeedsFlag = "addPlatformToSeeds";
    const char* RemovePlatformFromAllSeedsFlag = "removePlatformFromSeeds";
    const char* UpdateSeedPathArg = "updateSeedPath";
    const char* RemoveSeedPathArg = "removeSeedPath";
    const char* DefaultProjectTemplatePath = "Templates/DefaultProject/Template";
    const char* ProjectName = "${Name}";
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


    constexpr auto EngineDirectoryName = AZ::IO::FixedMaxPath("Assets") / "Engine";
    const char RestrictedDirectoryName[] = "restricted";
    const char PlatformsDirectoryName[] = "Platforms";
    const char GemsDirectoryName[] = "Gems";
    const char GemsSeedFileName[] = "seedList";
    const char EngineSeedFileName[] = "SeedAssetList";


    namespace Internal
    {
        const AZ::u32 PlatformFlags_RESTRICTED = aznumeric_cast<AZ::u32>(AzFramework::PlatformFlags::Platform_JASPER | AzFramework::PlatformFlags::Platform_PROVO | AzFramework::PlatformFlags::Platform_SALEM);

        void AddPlatformSeeds(
            const AZ::IO::Path& engineDirectory,
            const AZStd::string& rootFolderDisplayName,
            AZStd::unordered_map<AZStd::string, AZStd::string>& defaultSeedLists,
            AzFramework::PlatformFlags platformFlags)
        {
            AZ::IO::FixedMaxPath engineRoot(AZ::Utils::GetEnginePath());
            AZ::IO::FixedMaxPath engineRestrictedRoot = engineRoot / RestrictedDirectoryName;

            AZ::IO::FixedMaxPath engineLocalPath = AZ::IO::PathView(engineDirectory.LexicallyRelative(engineRoot));

            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            auto platformsIdxList = AzFramework::PlatformHelper::GetPlatformIndicesInterpreted(platformFlags);
            
            for (const AzFramework::PlatformId& platformId : platformsIdxList)
            {
                const AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlagFromPlatformIndex(platformId);
                const char* platformDirName = AzFramework::PlatformHelper::GetPlatformName(platformId);

                AZ::IO::FixedMaxPath platformDirectory;
                if (aznumeric_cast<AZ::u32>(platformFlag) & PlatformFlags_RESTRICTED)
                {
                    platformDirectory = engineRestrictedRoot / platformDirName / engineLocalPath;
                }
                else
                {
                    platformDirectory = engineDirectory / PlatformsDirectoryName / platformDirName;
                }

                if (fileIO->Exists(platformDirectory.c_str()))
                {
                    bool recurse = true;
                    AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> result = AzFramework::FileFunc::FindFileList(
                        platformDirectory.String(),
                        AZStd::string::format("*.%s", AzToolsFramework::AssetSeedManager::GetSeedFileExtension()).c_str(),
                        recurse);

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
            const AZ::IO::Path& engineDirectory,
            const AZStd::string& rootFolderDisplayName,
            AZStd::unordered_map<AZStd::string, AZStd::string>& defaultSeedLists,
            AzFramework::PlatformFlags platformFlags)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

            // Check whether platforms directory exists inside the root, if yes than add
            // * All seed files from the platforms directory
            // * All platform specific seed files based on the platform flags specified. 
            auto platformsDirectory = engineDirectory / PlatformsDirectoryName;
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

            AddPlatformSeeds(engineDirectory, rootFolderDisplayName, defaultSeedLists, platformFlags);
        }
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

    AZStd::unordered_map<AZStd::string, AZStd::string> GetDefaultSeedListFiles(
        AZStd::string_view enginePath,
        AZStd::string_view projectPath,
        const AZStd::vector<AzFramework::GemInfo>& gemInfoList,
        AzFramework::PlatformFlags platformFlag)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

        // Add all seed list files of enabled gems for the given project
        AZStd::unordered_map<AZStd::string, AZStd::string> defaultSeedLists = GetGemSeedListFilePathToGemNameMap(gemInfoList, platformFlag);


        // Add the engine seed list file
        AZ::IO::Path engineDirectory = AZ::IO::Path(enginePath) / EngineDirectoryName;
        auto absoluteEngineSeedFilePath = engineDirectory / EngineSeedFileName;
        absoluteEngineSeedFilePath.ReplaceExtension(AzToolsFramework::AssetSeedManager::GetSeedFileExtension());
        if (fileIO->Exists(absoluteEngineSeedFilePath.c_str()))
        {
            defaultSeedLists[absoluteEngineSeedFilePath.Native()] = EngineDirectoryName.String();
        }

        // Add Seed Lists from the Platforms directory
        Internal::AddPlatformsDirectorySeeds(engineDirectory, EngineDirectoryName.String(), defaultSeedLists, platformFlag);

        auto absoluteProjectDefaultSeedFilePath = AZ::IO::Path(projectPath) / EngineSeedFileName;
        absoluteProjectDefaultSeedFilePath.ReplaceExtension(AzToolsFramework::AssetSeedManager::GetSeedFileExtension());

        if (fileIO->Exists(absoluteProjectDefaultSeedFilePath.c_str()))
        {
            defaultSeedLists[absoluteProjectDefaultSeedFilePath.Native()] = projectPath;
        }

        return defaultSeedLists;
    }

    AZStd::vector<AZStd::string> GetDefaultSeeds(AZStd::string_view projectPath, AZStd::string_view projectName)
    {
        AZStd::vector<AZStd::string> defaultSeeds;

        defaultSeeds.emplace_back(GetProjectDependenciesAssetPath(projectPath, projectName));

        return defaultSeeds;
    }

    AZ::IO::Path GetProjectDependenciesFile(AZStd::string_view projectPath, AZStd::string_view projectName)
    {
        AZ::IO::Path projectDependenciesFilePath = projectPath;
        projectDependenciesFilePath /= AZStd::string::format("%.*s%s", aznumeric_cast<int>(projectName.size()), projectName.data(),
            DependenciesFileSuffix);
        projectDependenciesFilePath.ReplaceExtension(DependenciesFileExtension);
        return projectDependenciesFilePath.LexicallyNormal();
    }

    AZ::IO::Path GetProjectDependenciesAssetPath(AZStd::string_view projectPath, AZStd::string_view projectName)
    {
        AZ::IO::Path projectDependenciesFile = GetProjectDependenciesFile(projectPath, projectName);
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(projectDependenciesFile.c_str()))
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Project dependencies file %s doesn't exist.\n", projectDependenciesFile.c_str());
        }

        // Turn the absolute path into a cache-relative path
        AZ::IO::Path relativeProductPath = projectDependenciesFile.Filename().Native();
        AZStd::to_lower(relativeProductPath.Native().begin(), relativeProductPath.Native().end());

        return relativeProductPath;
    }

    AZStd::unordered_map<AZStd::string, AZStd::string> GetGemSeedListFilePathToGemNameMap(
        const AZStd::vector<AzFramework::GemInfo>& gemInfoList,
        AzFramework::PlatformFlags platformFlags)
    {
        AZStd::unordered_map<AZStd::string, AZStd::string> filePathToGemNameMap;
        for (const AzFramework::GemInfo& gemInfo : gemInfoList)
        {
            for (const AZ::IO::Path& gemAbsoluteSourcePath : gemInfo.m_absoluteSourcePaths)
            {
                AZ::IO::Path gemInfoAssetFilePath = gemAbsoluteSourcePath;
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
        }

        return filePathToGemNameMap;
    }

    bool IsGemSeedFilePathValid(
        AZStd::string_view engineRoot,
        AZStd::string seedAbsoluteFilePath,
        const AZStd::vector<AzFramework::GemInfo>& gemInfoList,
        AzFramework::PlatformFlags platformFlags)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

        if (!fileIO->Exists(seedAbsoluteFilePath.c_str()))
        {
            return false;
        }

        AZ::IO::Path gemsFolder{ engineRoot };
        gemsFolder /= GemsDirectoryName;
        gemsFolder = gemsFolder.LexicallyNormal();
        if (!AzFramework::StringFunc::StartsWith(seedAbsoluteFilePath, gemsFolder.Native()))
        {
            // if we are here it implies that this seed file does not live under the gems directory and
            // therefore we do not have to validate it
            return true;
        }

        AZ::IO::Path seedAbsolutePath{seedAbsoluteFilePath};
        for (const AzFramework::GemInfo& gemInfo : gemInfoList)
        {
            for (const AZ::IO::Path& gemAbsoluteSourcePath : gemInfo.m_absoluteSourcePaths)
            {
                // We want to check the path before going through the effort of creating the default Seed List file map
                if (!seedAbsolutePath.IsRelativeTo(gemAbsoluteSourcePath.LexicallyNormal()))
                {
                    continue;
                }

                AZStd::unordered_map<AZStd::string, AZStd::string> seeds = GetGemSeedListFilePathToGemNameMap({ gemInfo }, platformFlags);

                if (seeds.find(seedAbsoluteFilePath) != seeds.end())
                {
                    return true;
                }
                // If we have not validated the input path yet, we need to keep looking, or we will return false negatives
                //    for Gems that have the same prefix in their name
            }
        }

        return false;
    }

    AzFramework::PlatformFlags GetEnabledPlatformFlags(
        AZStd::string_view engineRoot,
        AZStd::string_view projectPath)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Error(AssetBundler::AppWindowName, false, "Settings Registry is not available, enabled platform flags cannot be queried");
            return AzFramework::PlatformFlags::Platform_NONE;
        }

        auto configFiles = AzToolsFramework::AssetUtils::GetConfigFiles(engineRoot, projectPath, true, true, settingsRegistry);
        auto enabledPlatformList = AzToolsFramework::AssetUtils::GetEnabledPlatforms(*settingsRegistry, configFiles);
        AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_NONE;
        for (const auto& enabledPlatform : enabledPlatformList)
        {
            AzFramework::PlatformFlags platformFlag = AzFramework::PlatformHelper::GetPlatformFlag(enabledPlatform);

            if (platformFlag != AzFramework::PlatformFlags::Platform_NONE)
            {
                platformFlags = platformFlags | platformFlag;
            }
            else
            {
                AZ_Warning(AssetBundler::AppWindowName, false,
                    "Platform Helper is not aware of the platform (%s).\n ", enabledPlatform.c_str());
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
        AZStd::string projectName{ AZStd::string_view(AZ::Utils::GetProjectName()) };
        if (!projectName.empty())
        {
            return AZ::Success(projectName);
        }
        else
        {
            return AZ::Failure(AZStd::string("Unable to obtain current project name from registry"));
        }
    }

    AZ::Outcome<AZ::IO::Path, AZStd::string> GetProjectFolderPath()
    {
        AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        if (!projectPath.empty())
        {
            return AZ::Success(AZ::IO::Path{ AZ::IO::PathView(projectPath) });
        }
        else
        {
            return AZ::Failure(AZStd::string::format("Unable to obtain current project path from registry"));
        }
    }

    AZ::Outcome<AZ::IO::Path, AZStd::string> GetProjectCacheFolderPath()
    {
        AZ::IO::Path projectCacheFolderPath;

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry && settingsRegistry->Get(projectCacheFolderPath.Native(),
            AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder))
        {
            if (AZ::IO::FileIOBase::GetInstance()->Exists(projectCacheFolderPath.c_str()))
            {
                return AZ::Success(projectCacheFolderPath);
            }
        }

        return AZ::Failure(AZStd::string::format(
            "Unable to locate the Project Cache path from Settings Registry at key %s."
            " Please run the O3DE Asset Processor to generate a Cache and build assets.",
            AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder));
    }

    AZ::Outcome<AZ::IO::Path, AZStd::string> GetAssetCatalogFilePath()
    {
        AZ::IO::Path assetCatalogFilePath = GetPlatformSpecificCacheFolderPath();
        if (assetCatalogFilePath.empty())
        {
            return AZ::Failure(AZStd::string::format(
                "Unable to retrieve cache platform path from Settings Registry at key: %s. Please run the O3DE Asset Processor to generate platform-specific cache folders and build assets.",
                AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder));
        }

        assetCatalogFilePath /= AssetCatalogFilename;
        return AZ::Success(assetCatalogFilePath);
    }

    AZ::IO::Path GetPlatformSpecificCacheFolderPath()
    {
        AZ::IO::Path platformSpecificCacheFolderPath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(
                platformSpecificCacheFolderPath.Native(),
                AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder);
        }
        return platformSpecificCacheFolderPath;
    }

    AZStd::string GenerateKeyFromAbsolutePath(const AZStd::string& absoluteFilePath)
    {
        AZStd::string key(absoluteFilePath);
        AzFramework::StringFunc::Path::Normalize(key);
        AzFramework::StringFunc::Path::StripDrive(key);
        return key;
    }

    void ConvertToRelativePath(AZStd::string_view parentFolderPath, AZStd::string& absoluteFilePath)
    {
        absoluteFilePath = AZ::IO::PathView(absoluteFilePath).LexicallyRelative(parentFolderPath).String();
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
            m_absolutePath = AZ::IO::PathView(filePath).LexicallyNormal();
            m_originalPath = m_absolutePath;
            ComputeAbsolutePath(platform, checkFileCase, ignoreFileCase);
        }
    }


    FilePath::FilePath(const AZStd::string& filePath, bool checkFileCase, bool ignoreFileCase)
        :FilePath(filePath, AZStd::string(), checkFileCase, ignoreFileCase)
    {
    }

    const AZStd::string& FilePath::AbsolutePath() const
    {
        return m_absolutePath.Native();
    }

    const AZStd::string& FilePath::OriginalPath() const
    {
        return m_originalPath.Native();
    }

    bool FilePath::IsValid() const
    {
        return m_validPath;
    }

    AZStd::string FilePath::ErrorString() const
    {
        return m_errorString;
    }

    void FilePath::ComputeAbsolutePath(const AZStd::string& platformIdentifier, bool checkFileCase, bool ignoreFileCase)
    {
        if (AzToolsFramework::AssetFileInfoListComparison::IsTokenFile(m_absolutePath.Native()))
        {
            return;
        }

        if (!platformIdentifier.empty())
        {
            AssetBundler::AddPlatformIdentifier(m_absolutePath.Native(), platformIdentifier);
        }

        AZ::IO::Path enginePath = AZ::IO::PathView(AZ::Utils::GetEnginePath());
        m_absolutePath = enginePath / m_absolutePath;
        if (checkFileCase)
        {
            AZ::IO::Path relFilePath = m_absolutePath.LexicallyProximate(enginePath);
            if (AzToolsFramework::AssetUtils::UpdateFilePathToCorrectCase(enginePath.Native(), relFilePath.Native()))
            {
                if (ignoreFileCase)
                {
                    m_absolutePath = (enginePath / relFilePath).String();
                }
                else
                {
                    AZ::IO::Path absfilePath = (enginePath / relFilePath).LexicallyNormal();
                    if (absfilePath != AZ::IO::PathView(m_absolutePath))
                    {
                        m_errorString = AZStd::string::format("File case mismatch, file ( %s ) does not exist on disk, did you mean file ( %s )."
                            " Please run the command again with the correct file path or use ( --%s ) arg if you want to allow case insensitive file match.\n",
                            m_absolutePath.c_str(), absfilePath.c_str(), IgnoreFileCaseFlag);
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
#if defined(AZ_ENABLE_TRACING)
        for (const AZStd::string& error : m_errors)
        {
            AZ_Error(AssetBundler::AppWindowName, false, error.c_str());
        }
#endif

        ClearErrors();
        m_reportingError = false;
    }

    void ScopedTraceHandler::ClearErrors()
    {
        m_errors.clear();
        m_errors.swap(AZStd::vector<AZStd::string>());
    }

    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::ComparisonType, AZStd::string> ParseComparisonType(
        const AZStd::string& comparisonType)
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
        AZStd::string failureMessage = AZStd::string::format(
            "Invalid Comparison Type ( %s ).  Valid types are: ", comparisonType.c_str());
        for (size_t i = 0; i < numTypes - 1; ++i)
        {
            failureMessage.append(AZStd::string::format("%s, ", AssetFileInfoListComparison::ComparisonTypeNames[i]));
        }
        failureMessage.append(AZStd::string::format("and %s.", AssetFileInfoListComparison::ComparisonTypeNames[numTypes - 1]));
        return AZ::Failure(failureMessage);
    }

    AZ::Outcome<AzToolsFramework::AssetFileInfoListComparison::FilePatternType, AZStd::string> ParseFilePatternType(
        const AZStd::string& filePatternType)
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
        AZStd::string failureMessage = AZStd::string::format(
            "Invalid File Pattern Type ( %s ).  Valid types are: ", filePatternType.c_str());
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

    QJsonObject ReadJson(const AZStd::string& filePath)
    {
        QByteArray byteArray;
        QFile jsonFile;
        jsonFile.setFileName(filePath.c_str());
        jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
        byteArray = jsonFile.readAll();
        jsonFile.close();

        return QJsonDocument::fromJson(byteArray).object();
    }

    void SaveJson(const AZStd::string& filePath, const QJsonObject& jsonObject)
    {
        QFile jsonFile(filePath.c_str());
        QJsonDocument JsonDocument;
        JsonDocument.setObject(jsonObject);
        jsonFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
        jsonFile.write(JsonDocument.toJson());
        jsonFile.close();
    }
}
