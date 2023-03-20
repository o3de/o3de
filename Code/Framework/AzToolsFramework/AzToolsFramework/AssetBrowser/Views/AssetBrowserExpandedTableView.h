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
        class AssetBrowserExpandedFilterModel;
        class AssetBrowserTreeView;
        class AssetBrowserExpandedTableViewProxyModel;
        class AssetBrowserEntry;

        class ExpandedTableViewDelegate
            : public QStyledItemDelegate
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(ExpandedTableViewDelegate, AZ::SystemAllocator);
            ExpandedTableViewDelegate(QWidget* parent = nullptr);

            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
            QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        signals:
            void renameTableEntry(const QString& value) const;
        };

        class AssetBrowserExpandedTableView
            : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserExpandedTableView, AZ::SystemAllocator);

            explicit AssetBrowserExpandedTableView(QWidget* parent = nullptr);
            ~AssetBrowserExpandedTableView() override;

            void SetAssetTreeView(AssetBrowserTreeView* treeView);

            void SetName(const QString& name);
            const QString& GetName() const;
            void SetIsAssetBrowserMainView();
            bool GetIsAssetBrowserMainView() const;
            void SetExpandedTableViewActive(bool isActiveView);
            bool GetExpandedTableViewActive() const;

            void DuplicateEntries();
            void MoveEntries();
            void DeleteEntries();
            void RenameEntry();
            void AfterRename(QString newVal);
            AZStd::vector<const AssetBrowserEntry*> GetSelectedAssets() const;
            void OpenItemForEditing(const QModelIndex& index);

            AzQtComponents::AssetFolderTableView* GetExpandedTableViewWidget() const;
            void setSelectionMode(QAbstractItemView::SelectionMode mode);
            QAbstractItemView::SelectionMode selectionMode() const;

        signals:
            void entryClicked(const AssetBrowserEntry* entry);
            void entryDoubleClicked(const AssetBrowserEntry* entry);
            void showInFolderTriggered(const AssetBrowserEntry* entry);

        private:
            AssetBrowserTreeView* m_assetTreeView = nullptr;
            AzQtComponents::AssetFolderTableView* m_expandedTableViewWidget = nullptr;
            AssetBrowserExpandedTableViewProxyModel* m_expandedTableViewProxyModel = nullptr;
            AssetBrowserExpandedFilterModel* m_assetFilterModel = nullptr;
            ExpandedTableViewDelegate* m_expandedTableViewDelegate = nullptr;
            QString m_name;
            bool m_isActiveView = false;

            void HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
            void UpdateFilterInLocalFilterModel();
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
