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

#endif

namespace AzQtComponents
{
    class AssetFolderExpandedTableView;
}


namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;
        class AssetBrowserTreeView;
        class AssetBrowserExpandedTableViewProxyModel;
        class PreviewerFrame;

        class AssetBrowserExpandedTableView : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserExpandedTableView, AZ::SystemAllocator, 0);

            explicit AssetBrowserExpandedTableView(QWidget* parent = nullptr);
            ~AssetBrowserExpandedTableView() override;

            void SetPreviewerFrame(PreviewerFrame* previewerFrame);
            void SetAssetTreeView(AssetBrowserTreeView* treeView);

        private:
            AssetBrowserTreeView* m_assetTreeView = nullptr;
            PreviewerFrame* m_previewerFrame = nullptr;
            AzQtComponents::AssetFolderExpandedTableView* m_expandedTableViewWidget = nullptr;
            AssetBrowserExpandedTableViewProxyModel* m_expandedTableViewProxyModel = nullptr;
            AssetBrowserFilterModel* m_assetFilterModel = nullptr;

            void HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
            void UpdateFilterInLocalFilterModel();
        };

    } // namespace AssetBrowser
} // namespace AzToolsFramework
