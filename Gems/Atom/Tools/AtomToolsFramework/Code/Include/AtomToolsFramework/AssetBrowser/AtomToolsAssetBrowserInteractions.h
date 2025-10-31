/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
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
    } // namespace AssetBrowser
} // namespace AzToolsFramework

namespace AtomToolsFramework
{
    //! Provides common asset browser interactions, source control integration, and functionality to add custom menu actions
    class AtomToolsAssetBrowserInteractions : public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AtomToolsAssetBrowserInteractions, AZ::SystemAllocator);

        AtomToolsAssetBrowserInteractions();
        ~AtomToolsAssetBrowserInteractions();

        using AssetBrowserEntryVector = AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>;
        using FilterCallback = AZStd::function<bool(const AssetBrowserEntryVector&)>;
        using ActionCallback = AZStd::function<void(QWidget* caller, QMenu* menu, const AssetBrowserEntryVector&)>;

        //! Add a filter and handler for custom context menu entries that will be added to the top of the context menu
        void RegisterContextMenuActions(const FilterCallback& filterCallback, const ActionCallback& actionCallback);

    private:
        //! AssetBrowserInteractionNotificationBus::Handler overrides...
        void AddContextMenuActions(QWidget* caller, QMenu* menu, const AssetBrowserEntryVector& entries) override;

        void AddContextMenuActionsForSourceEntries(
            QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);
        void AddContextMenuActionsForFolderEntries(
            QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);
        void AddContextMenuActionsForAllEntries(
            QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);
        void AddContextMenuActionsForSourceControl(
            QWidget* caller, QMenu* menu, const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);
        void UpdateContextMenuActionsForSourceControl(bool success, AzToolsFramework::SourceControlFileInfo info);

        QWidget* m_caller = {};
        QAction* m_addAction = {};
        QAction* m_checkOutAction = {};
        QAction* m_undoCheckOutAction = {};
        QAction* m_getLatestAction = {};
        AZStd::vector<AZStd::pair<FilterCallback, ActionCallback>> m_contextMenuCallbacks;
    };
} // namespace AtomToolsFramework
