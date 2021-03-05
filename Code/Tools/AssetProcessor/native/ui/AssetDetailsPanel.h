/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
