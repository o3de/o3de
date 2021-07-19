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
        BusConnect();
    }

    AssetBrowserContextProvider::~AssetBrowserContextProvider()
    {
        BusDisconnect();
    }

    bool AssetBrowserContextProvider::HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry) const
    {
        AZStd::unordered_set<AZStd::string> extensions;
        EBUS_EVENT(AZ::SceneAPI::Events::AssetImportRequestBus, GetSupportedFileExtensions, extensions);
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

    void AssetBrowserContextProvider::AddSourceFileOpeners([[maybe_unused]] const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
    {
        using namespace AzToolsFramework;

        if (const SourceAssetBrowserEntry* source = SourceAssetBrowserEntry::GetSourceByUuid(sourceUUID))
        {
            if (!HandlesSource(source))
            {
                return;
            }
        }
        else
        {
            // its not something we can actually open if its not a source file at all
            return;
        }

        openers.push_back({ "O3DE_FBX_Settings_Edit", "Edit Settings...", QIcon(), [](const char* fullSourceFileNameInCallback, const AZ::Uuid& /*sourceUUID*/)
        {
            AZStd::string sourceName(fullSourceFileNameInCallback); // because the below call absolutely requires a AZStd::string.
            AssetImporterPlugin::GetInstance()->EditImportSettings(sourceName);
        } });
    }

    AzToolsFramework::AssetBrowser::SourceFileDetails AssetBrowserContextProvider::GetSourceFileDetails(const char* fullSourceFileName)
    {
        AZStd::string extensionString;
        if (AzFramework::StringFunc::Path::GetExtension(fullSourceFileName, extensionString, true))
        {
            // this does include the "." in the extension.
            AZStd::unordered_set<AZStd::string> extensions;
            EBUS_EVENT(AZ::SceneAPI::Events::AssetImportRequestBus, GetSupportedFileExtensions, extensions);
            for (AZStd::string potentialExtension : extensions)
            {
                if (AzFramework::StringFunc::Equal(extensionString.c_str(), potentialExtension.c_str()))
                {
                    return AzToolsFramework::AssetBrowser::SourceFileDetails("Icons/AssetBrowser/FBX_16.png");
                }
            }
        }

        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }
}
