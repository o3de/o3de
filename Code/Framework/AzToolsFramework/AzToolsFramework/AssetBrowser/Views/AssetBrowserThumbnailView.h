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
    class AssetFolderThumbnailView;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;
        class AssetBrowserTreeView;
        class AssetBrowserThumbnailViewProxyModel;
        class PreviewerFrame;

        class AssetBrowserThumbnailView : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserThumbnailView, AZ::SystemAllocator, 0);

            explicit AssetBrowserThumbnailView(QWidget* parent = nullptr);
            ~AssetBrowserThumbnailView() override;

            void SetPreviewerFrame(PreviewerFrame* previewerFrame);
            void SetAssetTreeView(AssetBrowserTreeView* treeView);

        private:
            AssetBrowserTreeView* m_assetTreeView = nullptr;
            PreviewerFrame* m_previewerFrame = nullptr;
            AzQtComponents::AssetFolderThumbnailView* m_thumbnailViewWidget = nullptr;
            AssetBrowserThumbnailViewProxyModel* m_thumbnailViewProxyModel = nullptr;
            AssetBrowserFilterModel* m_assetFilterModel = nullptr;

            void HandleTreeViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
            void UpdateFilterInLocalFilterModel();
        };

    } // namespace AssetBrowser
} // namespace AzToolsFramework