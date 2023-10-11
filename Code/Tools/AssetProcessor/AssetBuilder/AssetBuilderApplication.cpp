/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <AzCore/Utils/Utils.h>

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
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextComponent.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogComponent.h>
#include <AzToolsFramework/Metadata/MetadataManager.h>
#include <AzToolsFramework/Metadata/UuidUtils.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderApplication.h>
#include <AssetBuilderComponent.h>
#include <AssetBuilderInfo.h>
#include <AzCore/Interface/Interface.h>
#include <Entity/EntityUtilityComponent.h>
#include <AssetBuilderStatic.h>

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
        azrtti_typeid<AzToolsFramework::Prefab::PrefabSystemComponent>(),
        azrtti_typeid<AzToolsFramework::EntityUtilityComponent>(),
        azrtti_typeid<AzToolsFramework::MetadataManager>(),
        azrtti_typeid<AzToolsFramework::UuidUtilComponent>(),
        });

    return components;
}

AssetBuilderApplication::AssetBuilderApplication(int* argc, char*** argv)
    : AssetBuilderApplication(argc, argv, {})
{
}

AssetBuilderApplication::AssetBuilderApplication(int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings)
    : AzToolsFramework::ToolsApplication(argc, argv, AZStd::move(componentAppSettings))
    , m_qtApplication(*argc, *argv)
{
    // The settings registry has been created at this point
    auto settingsRegistry = AZ::SettingsRegistry::Get();
    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
        *settingsRegistry, AssetBuilder::GetBuildTargetName());

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
    // the AssetBuilder supports overriding the project-path on the command line
    AZ::SettingsRegistryInterface& registry = *AZ::SettingsRegistry::Get();

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

    // Make sure a project path was set to settings registry and error/warn if not.
    auto projectPath = AZ::Utils::GetProjectPath();
    if (projectPath.empty())
    {
        if (IsInDebugMode())
        {
            AZ_Error("AssetBuilder", false, "Unable to determine the project path automatically. "
                "Make sure a default project path has been set or provide a --project-path option on the command line. "
                "(See -help for more info.)");
            return;
        }
        else
        {
            AZ_Printf(AssetBuilderSDK::InfoWindow, "project-path not specified on the command line, assuming current directory.\n");
            AZ_Printf(AssetBuilderSDK::InfoWindow, "project-path is best specified as the full path to the project's folder.");
        }
    }

    // Loads dynamic modules and registers any component descriptors populated into the AZ::Module m_descriptor list
    // for each instantiated module class
    LoadDynamicModules();

    AssetBuilderSDK::InitializeSerializationContext();
    AssetBuilderSDK::InitializeBehaviorContext();
    AssetBuilder::InitializeSerializationContext();
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

void AssetBuilderApplication::InitializeBuilderComponents()
{
    CreateAndAddEntityFromComponentTags(AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }), "AssetBuilders Entity");
}
