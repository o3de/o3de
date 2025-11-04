/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AssetTreeModel.h"


namespace AssetProcessor
{
    class ProductAssetTreeItemData;

    class ProductAssetTreeModel : public AssetTreeModel
    {
    public:
        ProductAssetTreeModel(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection, QObject *parent = nullptr);
        ~ProductAssetTreeModel() override;

        // AssetDatabaseNotificationBus::Handler
        void OnProductFileChanged(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry) override;
        void OnProductFileRemoved(AZ::s64 productId) override;
        void OnProductFilesRemoved(const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& products) override;

        QModelIndex GetIndexForProduct(const AZStd::string& product);

    protected:
        void ResetModel() override;

        void AddOrUpdateEntry(
            const AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& product,
            bool modelIsResetting);

        void RemoveAsset(AZ::s64 productId);

        void RemoveAssetTreeItem(AssetTreeItem* assetToRemove);
        void RemoveFoldersIfEmpty(AssetTreeItem* folderToCheck);

        void CheckForUnresolvedIssues(AZStd::shared_ptr<ProductAssetTreeItemData> productItemData);

        AZStd::unordered_map<AZStd::string, AssetTreeItem*> m_productToTreeItem;
        // Mapping product Ids to asset tree items makes cleanup easier when files are deleted.
        AZStd::unordered_map<AZ::s64, AssetTreeItem*> m_productIdToTreeItem;
    };
}
