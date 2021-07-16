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
            QTreeView* productTreeView,
            ProductAssetTreeModel* productAssetTreeModel,
            AssetTreeFilterModel* productFilterModel,
            QTabWidget* assetTab);

        void GoToSource(const AZStd::string& source);
        void GoToProduct(const AZStd::string& product);

    protected:
        QTreeView* m_sourceTreeView = nullptr;
        SourceAssetTreeModel* m_sourceTreeModel = nullptr;
        AssetTreeFilterModel* m_sourceFilterModel = nullptr;
        QTreeView* m_productTreeView = nullptr;
        ProductAssetTreeModel* m_productTreeModel = nullptr;
        AssetTreeFilterModel* m_productFilterModel = nullptr;
        QTabWidget* m_assetsTab = nullptr;
    };
} // namespace AssetProcessor
