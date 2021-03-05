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

#include "AssetSeedUtil.h"
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

namespace AssetValidation::AssetSeed
{
    GemInfo::GemInfo(AZStd::string name, AZStd::string relativeFilePath, AZStd::string absoluteFilePath, AZStd::string identifier, bool isGameGem, bool assetOnlyGem)
        : m_gemName(AZStd::move(name))
        , m_relativeFilePath(AZStd::move(relativeFilePath))
        , m_absoluteFilePath(AZStd::move(absoluteFilePath))
        , m_identifier(AZStd::move(identifier))
        , m_isGameGem(isGameGem)
        , m_assetOnly(assetOnlyGem)
    {
    }

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

    bool GetGemsInfo(const char* root, const char* assetRoot, const char* gameName, AZStd::vector<GemInfo>& gemInfoList)
    {
        Gems::IGemRegistry* registry = nullptr;
        Gems::IProjectSettings* projectSettings = nullptr;
        AZ::ModuleManagerRequests::LoadModuleOutcome result = AZ::Failure(AZStd::string("Failed to connect to ModuleManagerRequestBus.\n"));
        AZ::ModuleManagerRequestBus::BroadcastResult(result, &AZ::ModuleManagerRequestBus::Events::LoadDynamicModule, "GemRegistry", AZ::ModuleInitializationSteps::Load, false);
        if (!result.IsSuccess())
        {
            AZ_Error("AzToolsFramework::AssetUtils", false, "Could not load the GemRegistry module - %s.\n", result.GetError().c_str());
            return false;
        }

        // Use shared_ptr aliasing ctor to use the refcount/deleter from the moduledata pointer, but we only need to store the dynamic module handle.
        auto registryModule = AZStd::shared_ptr<AZ::DynamicModuleHandle>(result.GetValue(), result.GetValue()->GetDynamicModuleHandle());
        auto CreateGemRegistry = registryModule->GetFunction<Gems::RegistryCreatorFunction>(GEMS_REGISTRY_CREATOR_FUNCTION_NAME);
        Gems::RegistryDestroyerFunction registryDestroyerFunc = registryModule->GetFunction<Gems::RegistryDestroyerFunction>(GEMS_REGISTRY_DESTROYER_FUNCTION_NAME);
        if (!CreateGemRegistry || !registryDestroyerFunc)
        {
            AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to load GemRegistry functions.\n");
            return false;
        }

        registry = CreateGemRegistry();
        if (!registry)
        {
            AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to create Gems::GemRegistry.\n");
            return false;
        }

        registry->AddSearchPath({ root, GemsDirectoryName }, false);


        registry->AddSearchPath({ assetRoot, GemsDirectoryName }, false);

        const char* engineRootPath = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRootPath, &AzFramework::ApplicationRequests::GetEngineRoot);

        if (engineRootPath && azstricmp(engineRootPath, root))
        {
            registry->AddSearchPath({ engineRootPath, GemsDirectoryName }, false);
        }
        projectSettings = registry->CreateProjectSettings();
        if (!projectSettings)
        {
            registryDestroyerFunc(registry);
            AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to create Gems::ProjectSettings.\n");
            return false;
        }

        if (!projectSettings->Initialize(assetRoot, gameName))
        {
            registry->DestroyProjectSettings(projectSettings);
            registryDestroyerFunc(registry);

            AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to initialize Gems::ProjectSettings.\n");
            return false;
        }

        auto loadProjectOutcome = registry->LoadProject(*projectSettings, true);
        if (!loadProjectOutcome.IsSuccess())
        {
            registry->DestroyProjectSettings(projectSettings);
            registryDestroyerFunc(registry);

            AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to load Gems project: %s.\n", loadProjectOutcome.GetError().c_str());
            return false;
        }

        // Populating the gem info list
        for (const auto& pair : projectSettings->GetGems())
        {
            Gems::IGemDescriptionConstPtr desc = registry->GetGemDescription(pair.second);

            if (!desc)
            {
                Gems::ProjectGemSpecifier gemSpecifier = pair.second;
                AZStd::string errorStr = AZStd::string::format("Failed to load Gem with ID %s and Version %s (from path %s).\n",
                    gemSpecifier.m_id.ToString<AZStd::string>().c_str(), gemSpecifier.m_version.ToString().c_str(), gemSpecifier.m_path.c_str());

                if (Gems::IGemDescriptionConstPtr latestVersion = registry->GetLatestGem(pair.first))
                {
                    errorStr += AZStd::string::format(" Found version %s, you may want to use that instead.\n", latestVersion->GetVersion().ToString().c_str());
                }

                AZ_Error("AzToolsFramework::AssetUtils", false, errorStr.c_str());
                return false;
            }

            // Note: the two 'false' parameters in the ToString call below ToString(false, false)
            // eliminates brackets and dashes in the formatting of the UUID.
            // this keeps it compatible with legacy formatting which also omitted the curly braces and the dashes in the UUID.
            AZStd::string gemId = desc->GetID().ToString<AZStd::string>(false, false).c_str();
            AZStd::to_lower(gemId.begin(), gemId.end());

            bool assetOnlyGem = true;

            Gems::ModuleDefinitionVector moduleList = desc->GetModules();

            for (Gems::ModuleDefinitionConstPtr moduleDef : moduleList)
            {
                if (moduleDef->m_linkType != Gems::LinkType::NoCode)
                {
                    assetOnlyGem = false;
                    break;
                }
            }

            gemInfoList.emplace_back(GemInfo(desc->GetName(), desc->GetPath(), desc->GetAbsolutePath(), gemId, desc->IsGameGem(), assetOnlyGem));
        }

        registry->DestroyProjectSettings(projectSettings);
        registryDestroyerFunc(registry);

        return true;
    }

    AZStd::vector<AZStd::string> GetGemSeedListFiles(const AZStd::vector<GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlags)
    {
        AZStd::vector<AZStd::string> gemSeedListFiles;
        for (const GemInfo& gemInfo : gemInfoList)
        {
            AZStd::string absoluteGemSeedFilePath;
            AZ::StringFunc::Path::ConstructFull(gemInfo.m_absoluteFilePath.c_str(), GemsSeedFileName, SeedFileExtension, absoluteGemSeedFilePath, true);

            if (AZ::IO::FileIOBase::GetInstance()->Exists(absoluteGemSeedFilePath.c_str()))
            {
                gemSeedListFiles.emplace_back(absoluteGemSeedFilePath);
            }

            AddPlatformsDirectorySeeds(gemInfo.m_absoluteFilePath, gemSeedListFiles, platformFlags);
        }

        return gemSeedListFiles;
    }

    AZStd::vector<AZStd::string> GetDefaultSeedListFiles(const AZStd::vector<GemInfo>& gemInfoList, AzFramework::PlatformFlags platformFlag)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "AZ::IO::FileIOBase must be ready for use.\n");

        const char* root = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(root, &AzFramework::ApplicationRequests::GetEngineRoot);

        // Add all seed list files of enabled gems for the given project
        AZStd::vector<AZStd::string> defaultSeedLists = GetGemSeedListFiles(gemInfoList, platformFlag);

        // Add the engine seed list file
        AZStd::string engineDirectory;
        AZ::StringFunc::Path::Join(root, EngineDirectoryName, engineDirectory);
        AZStd::string absoluteEngineSeedFilePath;
        AZ::StringFunc::Path::ConstructFull(engineDirectory.c_str(), EngineSeedFileName, SeedFileExtension, absoluteEngineSeedFilePath, true);
        if (fileIO->Exists(absoluteEngineSeedFilePath.c_str()))
        {
            defaultSeedLists.emplace_back(absoluteEngineSeedFilePath);
        }

        AddPlatformsDirectorySeeds(engineDirectory, defaultSeedLists, platformFlag);

        // Add the current project default seed list file
        AZStd::string projectName;
        bool checkPlatform = false;

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            AZ::SettingsRegistryInterface::FixedValueString bootstrapProjectName;
            auto projectKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/sys_game_folder", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
            settingsRegistry->Get(bootstrapProjectName, projectKey);
            if (!bootstrapProjectName.empty())
            {
                AZStd::string absoluteProjectDefaultSeedFilePath;
                AZ::StringFunc::Path::ConstructFull(root, bootstrapProjectName.c_str(), EngineSeedFileName, SeedFileExtension, absoluteProjectDefaultSeedFilePath, true);
                if (fileIO->Exists(absoluteProjectDefaultSeedFilePath.c_str()))
                {
                    defaultSeedLists.emplace_back(move(absoluteProjectDefaultSeedFilePath));
                }
            }
        }
        return defaultSeedLists;
    }
}
