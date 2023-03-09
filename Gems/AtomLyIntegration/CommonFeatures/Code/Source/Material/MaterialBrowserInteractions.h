/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

class QWidget;
class QMenu;
class QAction;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class SourceAssetBrowserEntry;
        class FolderAssetBrowserEntry;
    }
}

namespace AZ
{
    namespace Render
    {
        class MaterialBrowserInteractions
            : public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(MaterialBrowserInteractions, AZ::SystemAllocator);

            MaterialBrowserInteractions();
            ~MaterialBrowserInteractions();

        private:
            //! AssetBrowserInteractionNotificationBus::Handler overrides...
            void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
            void AddSourceFileCreators(const char* fullSourceFolderName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators) override;
            
            bool HandlesSource(AZStd::string_view fileName) const;

        };
    } // namespace Render
} // namespace AZ
