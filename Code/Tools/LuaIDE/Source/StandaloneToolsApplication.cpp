/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneToolsApplication.h"

#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/IPCComponent.h>

namespace StandaloneTools
{
    BaseApplication::BaseApplication(int argc, char** argv)
        : LegacyFramework::Application(argc, argv)
    {
        AZ::UserSettingsFileLocatorBus::Handler::BusConnect();
    }

    BaseApplication::~BaseApplication()
    {
        AZ::UserSettingsFileLocatorBus::Handler::BusDisconnect();
    }

    void BaseApplication::RegisterCoreComponents()
    {
        LegacyFramework::Application::RegisterCoreComponents();

        RegisterComponentDescriptor(LegacyFramework::IPCComponent::CreateDescriptor());

        RegisterComponentDescriptor(AzNetworking::NetworkingSystemComponent::CreateDescriptor());

        RegisterComponentDescriptor(AZ::UserSettingsComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::TargetManagementComponent::CreateDescriptor());

        RegisterComponentDescriptor(AZ::JobManagerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AZ::StreamerComponent::CreateDescriptor());
    }

    void BaseApplication::CreateSystemComponents()
    {
        LegacyFramework::Application::CreateSystemComponents();

        // AssetCatalogComponent was moved to the Application Entity to fulfil service requirements.
        EnsureComponentRemoved(AzFramework::AssetCatalogComponent::RTTI_Type());
    }

    void BaseApplication::CreateApplicationComponents()
    {
        EnsureComponentCreated(AZ::StreamerComponent::RTTI_Type());
        EnsureComponentCreated(AZ::JobManagerComponent::RTTI_Type());
        EnsureComponentCreated(AzNetworking::NetworkingSystemComponent::RTTI_Type());
        EnsureComponentCreated(AzFramework::TargetManagementComponent::RTTI_Type());
        EnsureComponentCreated(LegacyFramework::IPCComponent::RTTI_Type());

        // Check for user settings components already added (added by the app descriptor
        AZStd::array<bool, AZ::UserSettings::CT_MAX> userSettingsAdded;
        userSettingsAdded.fill(false);
        for (const auto& component : m_applicationEntity->GetComponents())
        {
            if (const auto userSettings = azrtti_cast<AZ::UserSettingsComponent*>(component))
            {
                userSettingsAdded[userSettings->GetProviderId()] = true;
            }
        }

        AZ::ModuleManagerRequestBus::Broadcast(
            &AZ::ModuleManagerRequestBus::Events::EnumerateModules,
            [](const AZ::ModuleData& moduleData)
            {
                AZ::Entity* moduleEntity = moduleData.GetEntity();
                for (const auto& component : moduleEntity->GetComponents())
                {
                    if (auto targetManagement = azrtti_cast<AzFramework::TargetManagementComponent*>(component))
                    {
                        targetManagement->SetTargetAsHost(true);
                    }
                }
                return true;
            });

        // For each provider not already added, add it.
        for (AZ::u32 providerId = 0; providerId < userSettingsAdded.size(); ++providerId)
        {
            if (!userSettingsAdded[providerId])
            {
                // Don't need to add one for global, that's added by someone else
                m_applicationEntity->AddComponent(aznew AZ::UserSettingsComponent(providerId));
            }
        }
    }

    bool BaseApplication::StartDebugService()
    {
        for (auto& networkInterface : AZ::Interface<AzNetworking::INetworking>::Get()->GetNetworkInterfaces())
        {
            if (networkInterface.first == AZ::Name("TargetManagement"))
            {
                const auto console = AZ::Interface<AZ::IConsole>::Get();
                uint16_t target_port = AzFramework::DefaultTargetPort;

                if (console->GetCvarValue("target_port", target_port) != AZ::GetValueResult::Success)
                {
                    AZ_Assert(false, "TargetManagement port could not be found");
                }

                networkInterface.second->Listen(target_port);
                return true;
            }
        }
        return false;
    }

    void BaseApplication::OnApplicationEntityActivated()
    {
        [[maybe_unused]] bool launched = StartDebugService();
        AZ_Warning("EditorApplication", launched, "Could not start hosting; Only replay is available.");
    }

    void BaseApplication::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
    {
        LegacyFramework::Application::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("standalone_tools");
    }

    AZStd::string BaseApplication::GetStoragePath() const
    {
        AZStd::string storagePath;
        FrameworkApplicationMessages::Bus::BroadcastResult(storagePath, &FrameworkApplicationMessages::GetApplicationGlobalStoragePath);

        if (storagePath.empty())
        {
            FrameworkApplicationMessages::Bus::BroadcastResult(storagePath, &FrameworkApplicationMessages::GetApplicationDirectory);
        }

        return storagePath;
    }

    AZStd::string BaseApplication::ResolveFilePath(AZ::u32 providerId)
    {
        AZStd::string appName;
        FrameworkApplicationMessages::Bus::BroadcastResult(appName, &FrameworkApplicationMessages::GetApplicationName);

        AZStd::string userStoragePath = GetStoragePath();
        AzFramework::StringFunc::Path::Join(userStoragePath.c_str(), appName.c_str(), userStoragePath);
        AZ::IO::SystemFile::CreateDir(userStoragePath.c_str());

        AZStd::string fileName;
        switch (providerId)
        {
        case AZ::UserSettings::CT_LOCAL:
            fileName = AZStd::string::format("%s_UserSettings.xml", appName.c_str());
            break;
        case AZ::UserSettings::CT_GLOBAL:
            fileName = "GlobalUserSettings.xml";
            break;
        }

        AzFramework::StringFunc::Path::Join(userStoragePath.c_str(), fileName.c_str(), userStoragePath);
        return userStoragePath;
    }
} // namespace StandaloneTools
