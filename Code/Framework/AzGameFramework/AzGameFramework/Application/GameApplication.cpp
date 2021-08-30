/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GameApplication.h"
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>
#include <GridMate/Drillers/CarrierDriller.h>
#include <GridMate/Drillers/ReplicaDriller.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <AzGameFramework/AzGameFrameworkModule.h>

namespace AzGameFramework
{
    GameApplication::GameApplication()
    {
    }

    GameApplication::GameApplication(int argc, char** argv)
        : Application(&argc, &argv)
    {
    }

    GameApplication::~GameApplication()
    {
    }

    void GameApplication::StartCommon(AZ::Entity* systemEntity)
    {
        AzFramework::Application::StartCommon(systemEntity);
    }

    void GameApplication::MergeSettingsToRegistry(AZ::SettingsRegistryInterface& registry)
    {
        AZ::SettingsRegistryInterface::Specializations specializations;
        Application::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("game");

        AZStd::vector<char> scratchBuffer;

#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, false);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectUserRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, false);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(registry);
#endif

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_TargetBuildDependencyRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);

#if AZ_TRAIT_OS_IS_HOST_OS_PLATFORM && (defined (AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD))
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_EngineRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_GemRegistries(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
#endif

        // Used the lowercase the platform name since the bootstrap.game.<config>.<platform>.setreg is being loaded
        // from the asset cache root where all the files are in lowercased from regardless of the filesystem case-sensitivity
        static constexpr char filename[] = "bootstrap.game." AZ_BUILD_CONFIGURATION_TYPE "." AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER ".setreg";

        AZ::IO::FixedMaxPath cacheRootPath;
        if (registry.Get(cacheRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
        {
            cacheRootPath /= filename;
            registry.MergeSettingsFile(cacheRootPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", &scratchBuffer);
        }

#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, false);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectUserRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, true);
#endif
        // Update the Runtime file paths in case the "{BootstrapSettingsRootKey}/assets" key was overriden by a setting registry
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(registry);
    }

    AZ::ComponentTypeList GameApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = Application::GetRequiredSystemComponents();

#if !defined(_RELEASE)
        components.emplace_back(azrtti_typeid<AzFramework::TargetManagementComponent>());
#endif

        return components;
    }

    void GameApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        AzFramework::Application::CreateStaticModules(outModules);

        outModules.emplace_back(aznew AzGameFrameworkModule());
    }

    void GameApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    {
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Game;
    };

} // namespace AzGameFramework
