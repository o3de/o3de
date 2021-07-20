/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetDetailsPanel.h"

#include "AssetTreeFilterModel.h"
#include "MainWindow.h"
#include "ProductAssetTreeModel.h"
#include "SourceAssetTreeModel.h"


namespace AssetProcessor
{

    AssetDetailsPanel::AssetDetailsPanel(QWidget* parent) : QFrame(parent)
    {
    }

    AssetDetailsPanel::~AssetDetailsPanel()
    {
    }

    void AssetDetailsPanel::RegisterAssociatedWidgets(
        QTreeView* sourceTreeView,
        SourceAssetTreeModel* sourceAssetTreeModel,
        AssetTreeFilterModel* sourceFilterModel,
        QTreeView* productTreeView,
        ProductAssetTreeModel* productAssetTreeModel,
        AssetTreeFilterModel* productFilterModel,
        QTabWidget* assetTab)
    {
        m_sourceTreeView = sourceTreeView;
        m_sourceTreeModel = sourceAssetTreeModel;
        m_sourceFilterModel = sourceFilterModel;
        m_productTreeView = productTreeView;
        m_productTreeModel = productAssetTreeModel;
        m_productFilterModel = productFilterModel;
        m_assetsTab = assetTab;
    }

    void AssetDetailsPanel::GoToSource(const AZStd::string& source)
    {
        if (!m_sourceTreeModel || !m_sourceTreeView || !m_assetsTab || !m_sourceFilterModel)
        {
            return;
        }
        m_assetsTab->setCurrentIndex(static_cast<int>(MainWindow::AssetTabIndex::Source));
        QModelIndex goToIndex = m_sourceTreeModel->GetIndexForSource(source);
        // Make sure this index is visible, even if a search is active.
        m_sourceFilterModel->ForceModelIndexVisible(goToIndex);
        QModelIndex filterIndex = m_sourceFilterModel->mapFromSource(goToIndex);
        // Some tables, like the source dependencies table, may have wildcards or links to files that don't exist.
        if (!filterIndex.isValid())
        {
            return;
        }
        m_sourceTreeView->scrollTo(filterIndex, QAbstractItemView::ScrollHint::EnsureVisible);
        m_sourceTreeView->selectionModel()->setCurrentIndex(filterIndex, AssetTreeModel::GetAssetTreeSelectionFlags());
    }

    void AssetDetailsPanel::GoToProduct(const AZStd::string& product)
    {
        if (!m_productTreeModel || !m_productTreeView || !m_assetsTab || !m_productFilterModel)
        {
            return;
        }
        m_assetsTab->setCurrentIndex(static_cast<int>(MainWindow::AssetTabIndex::Product));
        QModelIndex goToIndex = m_productTreeModel->GetIndexForProduct(product);
        // Make sure this index is visible, even if a search is active.
        m_productFilterModel->ForceModelIndexVisible(goToIndex);
        QModelIndex filterIndex = m_productFilterModel->mapFromSource(goToIndex);
        // Some tables may have wildcards or links to files that don't exist.
        if (!filterIndex.isValid())
        {
            return;
        }
        m_productTreeView->scrollTo(filterIndex, QAbstractItemView::ScrollHint::EnsureVisible);
        m_productTreeView->selectionModel()->setCurrentIndex(filterIndex, AssetTreeModel::GetAssetTreeSelectionFlags());
    }

}
