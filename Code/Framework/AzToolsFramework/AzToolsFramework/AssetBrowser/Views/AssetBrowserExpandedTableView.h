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
        public:
            AZ_CLASS_ALLOCATOR(ExpandedTableViewDelegate, AZ::SystemAllocator);
            ExpandedTableViewDelegate(QWidget* parent = nullptr);

            void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
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

            void HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
            void UpdateFilterInLocalFilterModel();
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
