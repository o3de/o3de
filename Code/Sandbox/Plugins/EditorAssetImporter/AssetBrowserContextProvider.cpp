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

#include "EditorAssetImporter_precompiled.h"
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

        openers.push_back({ "Lumberyard_FBX_Settings_Edit", "Edit Settings...", QIcon(), [](const char* fullSourceFileNameInCallback, const AZ::Uuid& /*sourceUUID*/)
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
                    return AzToolsFramework::AssetBrowser::SourceFileDetails("Editor/Icons/AssetBrowser/FBX_16.png");
                }
            }
        }

        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }
}
