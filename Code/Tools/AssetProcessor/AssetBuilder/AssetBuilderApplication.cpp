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

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/Component/EditorComponentAPIComponent.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityModelComponent.h>
#include <AzToolsFramework/Entity/EditorEntitySearchComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextComponent.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderApplication.h>
#include <AssetBuilderComponent.h>
#include <AssetBuilderInfo.h>
#include <AzCore/Interface/Interface.h>

namespace AssetBuilder
{
    //! This function returns the build system target name
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }
}

AZ::ComponentTypeList AssetBuilderApplication::GetRequiredSystemComponents() const
{
    AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();

    for (auto iter = components.begin(); iter != components.end();)
    {
        if (*iter == azrtti_typeid<AZ::UserSettingsComponent>()
            || *iter == azrtti_typeid<AzFramework::InputSystemComponent>()
            || *iter == azrtti_typeid<AzFramework::AssetCatalogComponent>()
            )
        {
            iter = components.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    components.insert(components.end(), {
        azrtti_typeid<AZ::SliceSystemComponent>(),
        azrtti_typeid<AzToolsFramework::SliceMetadataEntityContextComponent>(),
        azrtti_typeid<AssetBuilderComponent>(),
        azrtti_typeid<AssetProcessor::ToolsAssetCatalogComponent>(),
        azrtti_typeid<AzToolsFramework::AssetSystem::AssetSystemComponent>(),
        azrtti_typeid<AzToolsFramework::Components::EditorComponentAPIComponent>(),
        azrtti_typeid<AzToolsFramework::Components::EditorEntityActionComponent>(),
        azrtti_typeid<AzToolsFramework::Components::EditorEntitySearchComponent>(),
        azrtti_typeid<AzToolsFramework::Components::EditorEntityModelComponent>(),
        azrtti_typeid<AzToolsFramework::EditorEntityContextComponent>(),
        });

    return components;
}

AssetBuilderApplication::AssetBuilderApplication(int* argc, char*** argv)
    : AzToolsFramework::ToolsApplication(argc, argv)
    , m_qtApplication(*argc, *argv)
{
    // The settings registry has been created at this point
    auto settingsRegistry = AZ::SettingsRegistry::Get();
    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
        *settingsRegistry, AssetBuilder::GetBuildTargetName());

    // Override the /Amazon/AzCore/Bootstrap/sys_game_folder entry in the Settings Registry using the -gameName parameter
    if (m_commandLine.GetNumSwitchValues("gameName") > 0)
    {
        const AZStd::string& gameFolderOverride = m_commandLine.GetSwitchValue("gameName", 0);
        auto gameFolderCommandLineOverride = AZStd::string::format("--regset=%s/sys_game_folder=%s", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey,
            gameFolderOverride.c_str());

        AZ::CommandLine::ParamContainer commandLineArgs;
        m_commandLine.Dump(commandLineArgs);
        commandLineArgs.emplace_back(gameFolderCommandLineOverride);
        m_commandLine.Parse(commandLineArgs);

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(*settingsRegistry, m_commandLine, false);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*settingsRegistry);
    }
    AZ::Interface<IBuilderApplication>::Register(this);
}

AssetBuilderApplication::~AssetBuilderApplication()
{
    AZ::Interface<IBuilderApplication>::Unregister(this);
}

void AssetBuilderApplication::RegisterCoreComponents()
{
    AzToolsFramework::ToolsApplication::RegisterCoreComponents();
    
    RegisterComponentDescriptor(AssetBuilderComponent::CreateDescriptor());
    RegisterComponentDescriptor(AssetProcessor::ToolsAssetCatalogComponent::CreateDescriptor());
}

void AssetBuilderApplication::StartCommon(AZ::Entity* systemEntity)
{
    InstallCtrlHandler();

    // Merge in the SettingsRegistry for the game being processed. This does not
    // necessarily correspond to the project name in the bootstrap.cfg since it
    // the AssetBuilder supports overriding the gameName on the command line
    AZ::SettingsRegistryInterface& registry = *AZ::SettingsRegistry::Get();
    AZ::SettingsRegistryInterface::FixedValueString gameName;
    if (m_commandLine.GetNumSwitchValues("gameName") > 0)
    {
        gameName = AZStd::string_view(m_commandLine.GetSwitchValue("gameName", 0));
    }

    // Add the supplied gameName to the specialization key in the registry
    if (!gameName.empty())
    {
        auto gameNameSpecialization = AZ::SettingsRegistryInterface::FixedValueString::format("%s/%.*s",
            AZ::SettingsRegistryMergeUtils::SpecializationsRootKey, aznumeric_cast<int>(gameName.size()), gameName.data());
        registry.Set(gameNameSpecialization, true);
    }
    else
    {
        // Add the project name as a registry specialization
        auto projectKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/sys_game_folder", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
        if (AZ::SettingsRegistryInterface::FixedValueString bootstrapGameName; registry.Get(bootstrapGameName, projectKey) && !bootstrapGameName.empty())
        {
            registry.Set(AZ::SettingsRegistryInterface::FixedValueString::format("%s/%s",
                AZ::SettingsRegistryMergeUtils::SpecializationsRootKey, bootstrapGameName.c_str()),
                true);
        }
    }

    // Retrieve specializations from the Settings Registry and ComponentApplication derived classes
    AZ::SettingsRegistryInterface::Specializations specializations;
    SetSettingsRegistrySpecializations(specializations);

    // Merge the SettingsRegistry file again using gameName as an additional specialization
    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_TargetBuildDependencyRegistry(registry,
        AZ_TRAIT_OS_PLATFORM_CODENAME, specializations);

    AzToolsFramework::ToolsApplication::StartCommon(systemEntity);

#if defined(AZ_PLATFORM_MAC)
    // The asset builder needs to start astcenc as a child process to compress textures.
    // astcenc is started by the PVRTexLib dynamic library. In order for it to be able to find
    // the executable, we need to set the PATH environment variable.
    AZStd::string exeFolder;
    AZ::ComponentApplicationBus::BroadcastResult(exeFolder, &AZ::ComponentApplicationBus::Events::GetExecutableFolder);
    
    setenv("PATH", exeFolder.c_str(), 1);
#endif // AZ_PLATFORM_MAC

    AZStd::string gameRoot;

    if (m_commandLine.GetNumSwitchValues("gameRoot") > 0)
    {
        gameRoot = m_commandLine.GetSwitchValue("gameRoot", 0);
    }

    if (gameRoot.empty())
    {
        if (IsInDebugMode())
        {
            if (!AZ::SettingsRegistry::Get()->Get(gameRoot, AZ::SettingsRegistryMergeUtils::FilePathKey_SourceGameFolder))
            {
                AZ_Error("AssetBuilder", false, "Unable to determine the game root automatically. "
                    "Make sure a default project has been set or provide a default option on the command line. (See -help for more info.)");
                return;
            }
        }
        else
        {
            AZ_Printf(AssetBuilderSDK::InfoWindow, "gameRoot not specified on the command line, assuming current directory.\n");
            AZ_Printf(AssetBuilderSDK::InfoWindow, "gameRoot is best specified as the full path to the game's asset folder.");
        }
    }

    if (!gameRoot.empty())
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO)
        {
            fileIO->SetAlias("@devassets@", gameRoot.c_str());
        }
    }

    // Loads dynamic modules and registers any component descriptors populated into the AZ::Module m_descriptor list
    // for each instantiated module class
    LoadDynamicModules();

    AssetBuilderSDK::InitializeSerializationContext();
    AssetBuilderSDK::InitializeBehaviorContext();
    // the asset builder app never writes source files, only assets, so there is no need to do any kind of asset upgrading
    AZ::Data::AssetManager::Instance().SetAssetInfoUpgradingEnabled(false);

    // Disable parallel dependency loads since the builders can't count on all other assets and their info being ready.
    // Specifically, asset builders can trigger asset loads during the building process.  The ToolsAssetCatalog doesn't
    // implement the dependency APIs, so the asset loads will fail to load any dependent assets.
    // 
    // NOTE:  The ToolsAssetCatalog could *potentially* implement the dependency APIs by querying the live Asset Processor instance,
    // but this will return incomplete dependency information based on the subset of assets that have already processed.
    // In theory, if the Asset Builder dependencies are set up correctly, the needed subset should always be processed first,
    // but the one edge case that can't be handled is the case where the Asset Builder intends to filter out the dependent load,
    // but needs to query enough information about the asset (specifically asset type) to know that it can filter it out.  Since
    // the assets are being filtered out, they aren't dependencies, might not be built yet, and so might not have asset type available.
    AZ::Data::AssetManager::Instance().SetParallelDependentLoadingEnabled(false);
}

bool AssetBuilderApplication::IsInDebugMode() const
{
    return AssetBuilderComponent::IsInDebugMode(m_commandLine);
}

bool AssetBuilderApplication::GetOptionalAppRootArg(char destinationRootArgBuffer[], size_t destinationRootArgBufferSize) const
{
    // Only continue if the application received any arguments from the command line
    if ((!this->m_argC) || (!this->m_argV))
    {
        return false;
    }

    int argc = this->m_argC;
    char** argv = this->m_argV;

    // Search for the app root argument (-approot=<PATH>) where <PATH> is the app root path to set for the application
    const static char* appRootArgPrefix = "-approot=";
    size_t appRootArgPrefixLen = strlen(appRootArgPrefix);

    const char* appRootArg = nullptr;
    for (int index = 0; index < argc; index++)
    {
        if (strncmp(appRootArgPrefix, argv[index], appRootArgPrefixLen) == 0)
        {
            appRootArg = &argv[index][appRootArgPrefixLen];
            break;
        }
    }

    if (appRootArg)
    {
        AZStd::string_view appRootArgView = appRootArg;
        size_t afterStartQuotes = appRootArgView.find_first_not_of(R"(")");
        if (afterStartQuotes != AZStd::string_view::npos)
        {
            appRootArgView.remove_prefix(afterStartQuotes);
        }
        size_t beforeEndQuotes = appRootArgView.find_last_not_of(R"(")");
        if (beforeEndQuotes != AZStd::string_view::npos)
        {
            appRootArgView.remove_suffix(appRootArgView.size() - (beforeEndQuotes + 1));
        }
        appRootArgView.copy(destinationRootArgBuffer, destinationRootArgBufferSize);
        destinationRootArgBuffer[appRootArgView.size()] = '\0';

        const char lastChar = destinationRootArgBuffer[strlen(destinationRootArgBuffer) - 1];
        bool needsTrailingPathDelim = (lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR) && (lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR);
        if (needsTrailingPathDelim)
        {
            azstrncat(destinationRootArgBuffer, destinationRootArgBufferSize, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, 1);
        }
        return true;
    }
    else
    {
        return false;
    }
}

void AssetBuilderApplication::InitializeBuilderComponents()
{
    CreateAndAddEntityFromComponentTags(AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }), "AssetBuilders Entity");
}
