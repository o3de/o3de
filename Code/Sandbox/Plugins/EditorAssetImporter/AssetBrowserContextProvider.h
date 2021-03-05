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

#pragma once

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

class QMimeData;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class SourceAssetBrowserEntry;
        class AssetBrowserEntry;
    }
}

namespace AZ
{
    class AssetBrowserContextProvider
        : public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
    public:
        AssetBrowserContextProvider();
        ~AssetBrowserContextProvider() override;
     
        /////////////////////////////////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
        AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
        /////////////////////////////////////////////////////////////////////////////////////////////////////

    protected:
        bool HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry) const; // return true if we care about this kind of source file.
    };
}