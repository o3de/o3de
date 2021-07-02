/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"
#include "StandaloneToolsApplication.h"

#include <Source/Telemetry/TelemetryComponent.h>
#include <Source/Telemetry/TelemetryBus.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/IPCComponent.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>

namespace StandaloneTools
{
    BaseApplication::BaseApplication(int&, char**)
        : LegacyFramework::Application()
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

        RegisterComponentDescriptor(Telemetry::TelemetryComponent::CreateDescriptor());
        RegisterComponentDescriptor(LegacyFramework::IPCComponent::CreateDescriptor());

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
        LegacyFramework::Application::CreateApplicationComponents();

        EnsureComponentCreated(AZ::StreamerComponent::RTTI_Type());
        EnsureComponentCreated(AZ::JobManagerComponent::RTTI_Type());
        EnsureComponentCreated(Telemetry::TelemetryComponent::RTTI_Type());
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

    void BaseApplication::OnApplicationEntityActivated()
    {
        const int k_processIntervalInSecs = 2;
        const bool doSDKInitShutdown = true;
        EBUS_EVENT(Telemetry::TelemetryEventsBus, Initialize, "O3DE_IDE", k_processIntervalInSecs, doSDKInitShutdown);

        bool launched = LaunchDiscoveryService();

        AZ_Warning("EditorApplication", launched, "Could not launch GridHub; Only replay is available.");
        (void)launched;
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
}
