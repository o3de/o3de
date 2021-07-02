/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
