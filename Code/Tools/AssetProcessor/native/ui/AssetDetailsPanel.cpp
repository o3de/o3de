/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        QTreeView* intermediateTreeView,
        SourceAssetTreeModel* intermediateAssetTreeModel,
        AssetTreeFilterModel* intermediateFilterModel,
        QTreeView* productTreeView,
        ProductAssetTreeModel* productAssetTreeModel,
        AssetTreeFilterModel* productFilterModel,
        QTabWidget* assetTab)
    {
        m_sourceTreeView = sourceTreeView;
        m_sourceTreeModel = sourceAssetTreeModel;
        m_sourceFilterModel = sourceFilterModel;

        m_intermediateTreeView = intermediateTreeView;
        m_intermediateTreeModel = intermediateAssetTreeModel;
        m_intermediateFilterModel = intermediateFilterModel;

        m_productTreeView = productTreeView;
        m_productTreeModel = productAssetTreeModel;
        m_productFilterModel = productFilterModel;
        m_assetsTab = assetTab;
    }

    void AssetDetailsPanel::GoToSource(const AZStd::string& source)
    {
        if (!m_sourceTreeModel || !m_sourceTreeView || !m_assetsTab || !m_sourceFilterModel ||
            !m_intermediateTreeView || !m_intermediateTreeModel || !m_intermediateFilterModel)
        {
            return;
        }

        AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceDetails;
        SourceAssetReference sourceAsset;

        if (AZ::IO::PathView(source).IsRelative())
        {
            assetDatabaseConnection.QuerySourceBySourceName(
                source.c_str(),
                [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
                {
                    sourceDetails = sourceEntry;
                    sourceAsset = SourceAssetReference(sourceEntry.m_scanFolderPK, sourceEntry.m_sourceName.c_str());
                    return false;
                });
        }
        else
        {
            sourceAsset = SourceAssetReference{ source.c_str() };
            assetDatabaseConnection.GetSourceBySourceNameScanFolderId(
                sourceAsset.RelativePath().c_str(), sourceAsset.ScanFolderId(), sourceDetails);
        }

        bool isIntermediate = false;
        if(m_intermediateAssetFolderId.has_value())
        {
            isIntermediate = sourceDetails.m_scanFolderPK == m_intermediateAssetFolderId.value();
        }

        int assetTabIndex = static_cast<int>(
            isIntermediate ? MainWindow::AssetTabIndex::Intermediate : MainWindow::AssetTabIndex::Source);
        m_assetsTab->setCurrentIndex(assetTabIndex);

        QTreeView* treeView = isIntermediate ? m_intermediateTreeView : m_sourceTreeView;
        SourceAssetTreeModel* treeModel = isIntermediate ? m_intermediateTreeModel : m_sourceTreeModel;
        AssetTreeFilterModel* filterModel = isIntermediate ? m_intermediateFilterModel : m_sourceFilterModel;

        QModelIndex goToIndex = treeModel->GetIndexForSource(sourceAsset.RelativePath().c_str(), sourceAsset.ScanFolderId());
        // Make sure this index is visible, even if a search is active.
        filterModel->ForceModelIndexVisible(goToIndex);
        QModelIndex filterIndex = filterModel->mapFromSource(goToIndex);
        // Some tables, like the source dependencies table, may have wildcards or links to files that don't exist.
        if (!filterIndex.isValid())
        {
            return;
        }
        treeView->scrollTo(filterIndex, QAbstractItemView::ScrollHint::EnsureVisible);
        treeView->selectionModel()->setCurrentIndex(filterIndex, AssetTreeModel::GetAssetTreeSelectionFlags());
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
