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

namespace MaterialEditor
{
    class MaterialBrowserInteractions
        : public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialBrowserInteractions, AZ::SystemAllocator, 0);

        MaterialBrowserInteractions();
        ~MaterialBrowserInteractions();

    private:
        //! AssetBrowserInteractionNotificationBus::Handler overrides...
        void AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries) override;

        void AddContextMenuActionsForOtherSource(QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry);
        void AddContextMenuActionsForMaterialSource(QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry);
        void AddContextMenuActionsForMaterialTypeSource(QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry);
        void AddContextMenuActionsForFolder(QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::FolderAssetBrowserEntry* entry);
        void AddPerforceMenuActions(QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);

        void UpdateSourceControlActions(bool success, AzToolsFramework::SourceControlFileInfo info);

        QWidget* m_caller = nullptr;
        QAction* m_addAction = nullptr;
        QAction* m_checkOutAction = nullptr;
        QAction* m_undoCheckOutAction = nullptr;
        QAction* m_getLatestAction = nullptr;
    };
} // namespace MaterialEditor
