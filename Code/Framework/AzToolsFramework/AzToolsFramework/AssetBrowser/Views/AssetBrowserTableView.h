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

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

#include <QItemSelection>
#include <QWidget>
#include <QAbstractItemView>
#include <QStyledItemDelegate>
#endif

namespace AzQtComponents
{
    class AssetFolderTableView;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;
        class AssetBrowserTreeView;
        class AssetBrowserTableViewProxyModel;
        class AssetBrowserEntry;
        class AssetBrowserTreeToTableProxyModel;
        class AssetBrowserModel;

        class TableViewDelegate
            : public QStyledItemDelegate
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(TableViewDelegate, AZ::SystemAllocator);
            TableViewDelegate(QWidget* parent = nullptr);

            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
            QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        signals:
            void renameTableEntry(const QString& value) const;
        };

        class AssetBrowserTableView
            : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserTableView, AZ::SystemAllocator);

            explicit AssetBrowserTableView(QWidget* parent = nullptr);
            ~AssetBrowserTableView() override;

            AssetBrowserEntry::AssetEntrySortMode ColumnToSortMode(const int columnIndex) const;
            int SortModeToColumn(const AssetBrowserEntry::AssetEntrySortMode sortMode) const;

            void SetAssetTreeView(AssetBrowserTreeView* treeView);

            void SetName(const QString& name);
            const QString& GetName() const;
            void SetIsAssetBrowserMainView();
            bool GetIsAssetBrowserMainView() const;
            void SetTableViewActive(bool isActiveView);
            bool GetTableViewActive() const;

            void DuplicateEntries();
            void MoveEntries();
            void DeleteEntries();
            void RenameEntry();
            void AfterRename(QString newVal);
            AZStd::vector<const AssetBrowserEntry*> GetSelectedAssets() const;
            void OpenItemForEditing(const QModelIndex& index);

            void dragEnterEvent(QDragEnterEvent* event) override;
            void dragMoveEvent(QDragMoveEvent* event) override;
            void dropEvent(QDropEvent* event) override;
            void dragLeaveEvent(QDragLeaveEvent* event) override;

            AzQtComponents::AssetFolderTableView* GetTableViewWidget() const;
            void setSelectionMode(QAbstractItemView::SelectionMode mode);
            QAbstractItemView::SelectionMode selectionMode() const;

            void SelectEntry(QString assetName);

            void SetSortMode(const AssetBrowserEntry::AssetEntrySortMode mode);
            AssetBrowserEntry::AssetEntrySortMode GetSortMode() const;
            void SetSearchString(const QString& searchString);
            
        signals:
            void entryClicked(const AssetBrowserEntry* entry);
            void entryDoubleClicked(const AssetBrowserEntry* entry);
            void showInFolderTriggered(const AssetBrowserEntry* entry);
            void selectionChangedSignal(const QItemSelection& selected, const QItemSelection& deselected);

        private:
            AssetBrowserTreeView* m_assetTreeView = nullptr;
            AzQtComponents::AssetFolderTableView* m_tableViewWidget = nullptr;
            AssetBrowserTableViewProxyModel* m_tableViewProxyModel = nullptr;
            AssetBrowserTreeToTableProxyModel* m_treeToTableProxyModel = nullptr;
            AssetBrowserModel* m_assetBrowserModel{ nullptr };
            AssetBrowserFilterModel* m_assetFilterModel = nullptr;
            TableViewDelegate* m_tableViewDelegate = nullptr;
            QString m_name;
            bool m_isActiveView = false;

            void HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
            void UpdateFilterInLocalFilterModel();
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
