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

#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>

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
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserThumbnailView, AZ::SystemAllocator);

            explicit AssetBrowserThumbnailView(QWidget* parent = nullptr);
            ~AssetBrowserThumbnailView() override;

            void SetAssetTreeView(AssetBrowserTreeView* treeView);

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
            void dragEnterEvent(QDragEnterEvent* event) override;
            void dragMoveEvent(QDragMoveEvent* event) override;
            void dragLeaveEvent(QDragLeaveEvent* event) override;
            void dropEvent(QDropEvent* event) override;

            void SelectEntry(QString assetName);

            void SetSortMode(const AssetBrowserEntry::AssetEntrySortMode mode);
            AssetBrowserEntry::AssetEntrySortMode GetSortMode() const;

            void SetSearchString(const QString& searchString);
        signals:
            void entryClicked(const AssetBrowserEntry* entry);
            void entryDoubleClicked(const AssetBrowserEntry* entry);
            void showInFolderTriggered(const AssetBrowserEntry* entry);
            void selectionChangedSignal(const QItemSelection& selected, const QItemSelection& deselected);

        public Q_SLOTS:
            void OpenItemForEditing(const QModelIndex &index);

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
