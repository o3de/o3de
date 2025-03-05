/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasApplication.h"

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string_view.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Components/NativeUISystemComponent.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzQtComponents/Components/GlobalEventFilter.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <QIcon>

// This has to live outside of any namespaces due to issues on Linux with calls to Q_INIT_RESOURCE if they are inside a namespace
void InitScriptCanvasApplicationResources()
{
    Q_INIT_RESOURCE(ScriptCanvasApplicationResources);
}

namespace ScriptCanvas
{
    /////////////////////////////////
    // ScriptCanvasQtApplication
    /////////////////////////////////

    ScriptCanvasQtApplication::ScriptCanvasQtApplication(int& argc, char** argv)
        : Base(argc, argv)
    {
        InitScriptCanvasApplicationResources();
        QApplication::setWindowIcon(QIcon(":/Resources/application.svg"));
        installEventFilter(new AzQtComponents::GlobalEventFilter(this));
    }

    /////////////////////////////////
    // ScriptCanvasToolsApplication
    /////////////////////////////////

    ScriptCanvasToolsApplication::ScriptCanvasToolsApplication(
        int* argc, char*** argv, AZ::ComponentApplicationSettings componentAppSettings)
        : AzToolsFramework::ToolsApplication(argc, argv, AZStd::move(componentAppSettings))
    {
    }

    void ScriptCanvasToolsApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        ConnectToAssetProcessor();

        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications::OnDatabaseInitialized);

        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusConnect();
    }

    void ScriptCanvasToolsApplication::RegisterCoreComponents()
    {
        Base::RegisterCoreComponents();
        RegisterComponentDescriptor(AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::AssetSystem::AssetSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Thumbnailer::ThumbnailerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzFramework::NativeUISystemComponent::CreateDescriptor());
    }

    // Not sure about the justification behind RegisterCoreComponents vs GetRequiredSystemComponents, maybe only one of them is required
    // -> RegisterCoreComponents creates them, whereas GetRequiredSystemComponents checks if they are initialized ?
    AZ::ComponentTypeList ScriptCanvasToolsApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = Base::GetRequiredSystemComponents();
        components.insert(
            components.end(),
            { azrtti_typeid<AzToolsFramework::AssetBrowser::AssetBrowserComponent>(),
              azrtti_typeid<AzToolsFramework::AssetSystem::AssetSystemComponent>(),
              azrtti_typeid<AzToolsFramework::Thumbnailer::ThumbnailerComponent>(),
              azrtti_typeid<AzFramework::NativeUISystemComponent>() });

        return components;
    }

    void ScriptCanvasToolsApplication::Destroy()
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();
        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::StartDisconnectingAssetProcessor);

        Base::Destroy();
    }

    bool ScriptCanvasToolsApplication::GetAssetDatabaseLocation(AZStd::string& result)
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath assetDatabaseSqlitePath;
        if (settingsRegistry &&
            settingsRegistry->Get(assetDatabaseSqlitePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder))
        {
            assetDatabaseSqlitePath /= "assetdb.sqlite";
            result = AZStd::string_view(assetDatabaseSqlitePath.Native());
            return true;
        }

        return false;
    }

    void ScriptCanvasToolsApplication::ConnectToAssetProcessor()
    {
        bool connectedToAssetProcessor = false;

        // When the AssetProcessor is already launched it should take less than a second to perform a connection
        // but when the AssetProcessor needs to be launch it could take up to 15 seconds to have the AssetProcessor initialize
        // and able to negotiate a connection when running a debug build
        // and to negotiate a connection

        AzFramework::AssetSystem::ConnectionSettings connectionSettings;
        AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
        connectionSettings.m_connectionDirection =
            AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
        connectionSettings.m_connectionIdentifier = "ScriptCanvasApplication";
        connectionSettings.m_loggingCallback =
            [targetName = connectionSettings.m_connectionIdentifier]([[maybe_unused]] AZStd::string_view logData)
        {
            AZ_UNUSED(targetName); // Prevent unused warning in release builds
            AZ_TracePrintf(targetName.c_str(), "%.*s", aznumeric_cast<int>(logData.size()), logData.data());
        };

        AzFramework::AssetSystemRequestBus::BroadcastResult(
            connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);
        if (!connectedToAssetProcessor)
        {
            AZ_Assert(false, "ScriptCanvasApplication failed to connect to asset processor");
        }
    }

} // namespace ScriptCanvas
