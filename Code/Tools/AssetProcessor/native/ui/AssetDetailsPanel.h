/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QFrame>
#include <AzCore/std/string/string.h>

class QTreeView;
class QTabWidget;

namespace AzQtComponents
{
    class FilteredSearchWidget;
}



namespace AssetProcessor
{
    class AssetTreeFilterModel;
    class ProductAssetTreeModel;
    class SourceAssetTreeModel;

    class AssetDetailsPanel
        : public QFrame
    {
    public:
        explicit AssetDetailsPanel(QWidget* parent = nullptr);
        ~AssetDetailsPanel() override;

        void RegisterAssociatedWidgets(
            QTreeView* sourceTreeView,
            SourceAssetTreeModel* sourceAssetTreeModel,
            AssetTreeFilterModel* sourceFilterModel,
            QTreeView* intermediateTreeView,
            SourceAssetTreeModel* intermediateAssetTreeModel,
            AssetTreeFilterModel* intermediateFilterModel,
            QTreeView* productTreeView,
            ProductAssetTreeModel* productAssetTreeModel,
            AssetTreeFilterModel* productFilterModel,
            QTabWidget* assetTab);

        void GoToSource(const AZStd::string& source);
        void GoToProduct(const AZStd::string& product);

        void SetIntermediateAssetFolderId(AZStd::optional<AZ::s64> intermediateAssetFolderId)
        {
            m_intermediateAssetFolderId = intermediateAssetFolderId;
        }

    protected:
        QTreeView* m_sourceTreeView = nullptr;
        SourceAssetTreeModel* m_sourceTreeModel = nullptr;
        AssetTreeFilterModel* m_sourceFilterModel = nullptr;

        QTreeView* m_intermediateTreeView = nullptr;
        SourceAssetTreeModel* m_intermediateTreeModel = nullptr;
        AssetTreeFilterModel* m_intermediateFilterModel = nullptr;

        QTreeView* m_productTreeView = nullptr;
        ProductAssetTreeModel* m_productTreeModel = nullptr;
        AssetTreeFilterModel* m_productFilterModel = nullptr;
        QTabWidget* m_assetsTab = nullptr;

        AZStd::optional<AZ::s64> m_intermediateAssetFolderId;
    };
} // namespace AssetProcessor
