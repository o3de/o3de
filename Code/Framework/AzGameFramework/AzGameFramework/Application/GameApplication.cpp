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
#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzGameFramework/AzGameFrameworkModule.h>

#if !O3DE_HEADLESS_SERVER
#include <AzFramework/AzFrameworkNativeUIModule.h>
#endif // O3DE_HEADLESS_SERVER

namespace AzGameFramework
{
    GameApplication::GameApplication()
        : GameApplication(0, nullptr, {})
    {
    }

    GameApplication::GameApplication(AZ::ComponentApplicationSettings componentAppSettings)
        : GameApplication(0, nullptr, AZStd::move(componentAppSettings))
    {
    }

    GameApplication::GameApplication(int argc, char** argv)
        : GameApplication(argc, argv, {})
    {
    }

    GameApplication::GameApplication(int argc, char** argv, AZ::ComponentApplicationSettings componentAppSettings)
        : Application(&argc, &argv, AZStd::move(componentAppSettings))
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

    AZ::ComponentTypeList GameApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = AzFramework::Application::GetRequiredSystemComponents();

#if !O3DE_HEADLESS_SERVER
        components.insert(
            components.end(),
            {
                azrtti_typeid<AzFramework::NativeUISystemComponent>(),
            });
#endif // O3DE_HEADLESS_SERVER

        return components;
    }

    void GameApplication::SetHeadless(bool headless)
    {
        m_headless = headless;
    }
    void GameApplication::SetConsoleModeSupported(bool supported)
    {
        m_consoleModeSupported = supported;
    }

    void GameApplication::StartCommon(AZ::Entity* systemEntity)
    {
        if (!this->m_headless)
        {
            bool isConsoleMode{ false };

            // If we are not in headless-mode, its still possible to be in console-only mode,
            // where a native client window will not be created (on platforms that support 
            // setting console mode at runtime). There are 2 possible triggers for console mode

            // 1. Settings registry : If the registry setting '/O3DE/Launcher/Bootstrap/ConsoleMode' is specified and set to true
            if (const auto* settingsRegistry = AZ::SettingsRegistry::Get())
            {
                settingsRegistry->Get(isConsoleMode, "/O3DE/Launcher/Bootstrap/ConsoleMode");

                // The null renderer can also be set in the settings registry for the RPI
                bool isRPINullRenderer{ false };
                if (settingsRegistry->Get(isRPINullRenderer, "/O3DE/Atom/RPI/Initialization/NullRenderer"))
                {
                    if (isRPINullRenderer)
                    {
                        isConsoleMode = true;
                    }
                }
            }

            // 2. Either '-console-mode' or 'rhi=null' is specified in the command-line argument
            const AzFramework::CommandLine* commandLine{ nullptr };
            constexpr const char* commandSwitchConsoleOnly = "console-mode";
            constexpr const char* commandSwitchNullRenderer = "NullRenderer";
            constexpr const char* commandSwitchRhi = "rhi";
            AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);
            AZ_Assert(commandLine, "Unable to query application command line to evaluate console-mode switches.");
            if (commandLine)
            {
                if (commandLine->HasSwitch(commandSwitchConsoleOnly) || commandLine->HasSwitch(commandSwitchNullRenderer))
                {
                    isConsoleMode = true;
                } 
                else if (size_t switchCount = commandLine->GetNumSwitchValues(commandSwitchRhi); switchCount > 0)
                {
                    auto rhiValue = commandLine->GetSwitchValue(commandSwitchRhi, switchCount - 1);
                    if (rhiValue.compare("null")==0)
                    {
                        isConsoleMode = true;
                    }
                }
            }

            AZ_Warning("Launcher", (isConsoleMode && !m_consoleModeSupported), "Console-mode was requested but not supported on this platform\n");
            if (isConsoleMode && m_consoleModeSupported)
            {
                AZ_Info("Launcher", "Console only mode enabled.\n");
                m_consoleMode = true;
            }
            else
            {
                m_consoleMode = false;
            }
        }

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
#if !O3DE_HEADLESS_SERVER
        outModules.emplace_back(aznew AzFramework::AzFrameworkNativeUIModule());
#endif // O3DE_HEADLESS_SERVER
    }

    void GameApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    {
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Game;
        if (m_headless)
        {
            appType.m_maskValue |= AZ::ApplicationTypeQuery::Masks::Headless;
        }
        if (m_consoleMode && m_consoleModeSupported)
        {
            appType.m_maskValue |= AZ::ApplicationTypeQuery::Masks::ConsoleMode;
        }
    };

} // namespace AzGameFramework
