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

namespace MaterialEditor
{
    class MaterialEditorBrowserInteractions
        : public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialEditorBrowserInteractions, AZ::SystemAllocator, 0);

        MaterialEditorBrowserInteractions();
        ~MaterialEditorBrowserInteractions();

    private:
        //! AssetBrowserInteractionNotificationBus::Handler overrides...
        void AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries) override;

        void AddGenericContextMenuActions(QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);
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
