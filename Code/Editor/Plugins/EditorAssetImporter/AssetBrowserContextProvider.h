/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        , public AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler
    {
    public:
        AssetBrowserContextProvider();
        ~AssetBrowserContextProvider() override;
     
        // AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler overrides ...
        AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;

        // AzToolsFramework::AssetBrowser::AssetBrowserPreviewRequestBus::Handler overrides ...
        void PreviewSceneSettings(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) override;
        bool HandleSource(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) const override;
        QMainWindow* GetSceneSettings() override;
        bool SaveBeforeClosing() override;

    protected:
        bool HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry) const; // return true if we care about this kind of source file.

        const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* m_currentEntry = nullptr;
    };
}
