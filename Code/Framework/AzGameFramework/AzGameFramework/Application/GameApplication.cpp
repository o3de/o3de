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
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Archive/Archive.h>
#include <AzGameFramework/AzGameFrameworkModule.h>

namespace AzGameFramework
{
    GameApplication::GameApplication()
    {
    }

    GameApplication::GameApplication(int argc, char** argv)
        : Application(&argc, &argv)
    {
        // In the Launcher Applications the Settings Registry
        // can read from the FileIOBase instance if available
        m_settingsRegistry->SetUseFileIO(true);

        // Attempt to mount the engine pak to the project product asset alias
        // Search Order:
        // - Project Cache Root Directory
        // - Executable Directory
        bool enginePakOpened{};
        AZ::IO::FixedMaxPath enginePakPath;
        if (m_settingsRegistry->Get(enginePakPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
        {
            // fall back to checking Project Cache Root.
            enginePakPath /= "engine.pak";
            enginePakOpened = m_archive->OpenPack("@products@", enginePakPath.Native());
        }
        if (!enginePakOpened)
        {
            enginePakPath = AZ::IO::FixedMaxPath(AZ::Utils::GetExecutableDirectory()) / "engine.pak";
            m_archive->OpenPack("@products@", enginePakPath.Native());
        }

        // By default, load all archives in the products folder.
        // If you want to adjust this for your project, make sure that the archive containing
        // the bootstrap for the settings registry is still loaded here, and any archives containing
        // assets used early in startup, like default shaders, are loaded here.
        constexpr AZStd::string_view paksFolder = "@products@/*.pak"; // (@products@ assumed)
        m_archive->OpenPacks(paksFolder);
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

        MergeSharedSettings(registry, specializations, scratchBuffer);

        // Query the launcher type from the registry
        constexpr AZStd::string_view LauncherTypeTag = "/O3DE/Runtime/LauncherType";
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        if (FixedValueString launcherType; registry.Get(launcherType, LauncherTypeTag)
            && !launcherType.empty())
        {
            // The bootstrap setreg file that is loaded is in the form of
            // bootstrap.<launcher-type-lower>.<config-lower>.setreg
            AZ::IO::FixedMaxPath filename = "bootstrap.";
            filename.Native() += launcherType;
            filename.Native() += '.';
            filename.Native() += AZ_BUILD_CONFIGURATION_TYPE ".setreg";

            AZ::IO::FixedMaxPath cacheRootPath;
            if (registry.Get(cacheRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
            {
                cacheRootPath /= filename;
                registry.MergeSettingsFile(cacheRootPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", &scratchBuffer);
            }
        }

        MergeUserSettings(registry, specializations, scratchBuffer);
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
