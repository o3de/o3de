/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

#include <QItemSelection>
#include <QWidget>
#include <QMenu>
#include <QAbstractItemView>

#endif

namespace AzQtComponents
{
    class AssetFolderThumbnailView;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;
        class AssetBrowserTreeView;
        class AssetBrowserThumbnailViewProxyModel;
        class AssetBrowserEntry;

        class AssetBrowserThumbnailView
            : public QWidget
            , private AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserThumbnailView, AZ::SystemAllocator);

            explicit AssetBrowserThumbnailView(QWidget* parent = nullptr);
            ~AssetBrowserThumbnailView() override;

            void SetAssetTreeView(AssetBrowserTreeView* treeView);

            void HideProductAssets(bool checked);

             // AssetBrowserModelNotificationBus
            void EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
            void EntryRemoved(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
            // ~AssetBrowserModelNotificationBus

            AzQtComponents::AssetFolderThumbnailView* GetThumbnailViewWidget() const;
            void SetName(const QString& name);
            QString& GetName();
            void SetIsAssetBrowserMainView();
            bool GetIsAssetBrowserMainView();
            void SetThumbnailActiveView(bool isActiveView);
            bool GetThumbnailActiveView();

            void DuplicateEntries();
            void MoveEntries();
            void DeleteEntries();
            void RenameEntry();
            void AfterRename(QString newVal);
            AZStd::vector<const AssetBrowserEntry*> GetSelectedAssets(bool includeProducts = true) const;

            void setSelectionMode(QAbstractItemView::SelectionMode mode);
            QAbstractItemView::SelectionMode selectionMode() const;

        signals:
            void entryClicked(const AssetBrowserEntry* entry);
            void entryDoubleClicked(const AssetBrowserEntry* entry);
            void showInFolderTriggered(const AssetBrowserEntry* entry);

        public Q_SLOTS:
            void UpdateThumbnailview();

        private:
            AssetBrowserTreeView* m_assetTreeView = nullptr;
            AzQtComponents::AssetFolderThumbnailView* m_thumbnailViewWidget = nullptr;
            AssetBrowserThumbnailViewProxyModel* m_thumbnailViewProxyModel = nullptr;
            AssetBrowserFilterModel* m_assetFilterModel = nullptr;
            void HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
            void UpdateFilterInLocalFilterModel();
            QString m_name;
            bool m_isActiveView = false;
        };

    } // namespace AssetBrowser
} // namespace AzToolsFramework
