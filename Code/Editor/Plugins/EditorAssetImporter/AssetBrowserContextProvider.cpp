/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBrowserContextProvider.h>
#include <AssetImporterPlugin.h>
#include <QtCore/QMimeData>
#include <QMenu>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h> // for ebus events
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    using namespace AzToolsFramework::AssetBrowser;

    AssetBrowserContextProvider::AssetBrowserContextProvider()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler::BusConnect();
    }

    AssetBrowserContextProvider::~AssetBrowserContextProvider()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
    }

    bool AssetBrowserContextProvider::HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry) const
    {
        AZStd::unordered_set<AZStd::string> extensions;
        AZ::SceneAPI::Events::AssetImportRequestBus::Broadcast(
            &AZ::SceneAPI::Events::AssetImportRequestBus::Events::GetSupportedFileExtensions, extensions);
        if (extensions.empty())
        {
            return false;
        }

        AZStd::string targetExtension = entry->GetExtension();

        for (const AZStd::string& potentialExtension : extensions)
        {
            const char* extension = potentialExtension.c_str();

            if (AzFramework::StringFunc::Equal(extension, targetExtension.c_str()))
            {
                return true;
            }
        }

        return false;
    }

    AzToolsFramework::AssetBrowser::SourceFileDetails AssetBrowserContextProvider::GetSourceFileDetails(const char* fullSourceFileName)
    {
        AZStd::string extensionString;
        if (AzFramework::StringFunc::Path::GetExtension(fullSourceFileName, extensionString, true))
        {
            // this does include the "." in the extension.
            AZStd::unordered_set<AZStd::string> extensions;
            AZ::SceneAPI::Events::AssetImportRequestBus::Broadcast(
                &AZ::SceneAPI::Events::AssetImportRequestBus::Events::GetSupportedFileExtensions, extensions);
            for (AZStd::string potentialExtension : extensions)
            {
                if (AzFramework::StringFunc::Equal(extensionString.c_str(), potentialExtension.c_str()))
                {
                    return AzToolsFramework::AssetBrowser::SourceFileDetails("Icons/AssetBrowser/FBX_80.svg");
                }
            }
        }

        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }

    void AssetBrowserContextProvider::PreviewSceneSettings(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry)
    {
        using namespace AzToolsFramework;

        if (const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(selectedEntry))
        {
            if (HandlesSource(sourceEntry) && sourceEntry != m_currentEntry)
            {
                if (AssetImporterPlugin::GetInstance()->EditImportSettings(sourceEntry->GetFullPath()))
                {
                    m_currentEntry = sourceEntry;
                }
            }
        }
    }

    bool AssetBrowserContextProvider::HandleSource(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) const
    {
        using namespace AzToolsFramework;

        if (const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(selectedEntry))
        {
            return HandlesSource(sourceEntry);
        }

        return false;
    }

    QMainWindow* AssetBrowserContextProvider::GetSceneSettings()
    {
        return AssetImporterPlugin::GetInstance()->OpenImportSettings();
    }

    bool AssetBrowserContextProvider::SaveBeforeClosing()
    {
        return AssetImporterPlugin::GetInstance()->SaveBeforeClosing();
    }
}
