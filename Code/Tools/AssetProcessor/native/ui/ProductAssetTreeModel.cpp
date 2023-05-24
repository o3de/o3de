/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProductAssetTreeModel.h"
#include "ProductAssetTreeItemData.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Console/IConsole.h>


namespace AssetProcessor
{
    AZ_CVAR_EXTERNED(bool, ap_disableAssetTreeView);

    ProductAssetTreeModel::ProductAssetTreeModel(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection, QObject *parent) :
        AssetTreeModel(sharedDbConnection, parent)
    {
    }

    ProductAssetTreeModel::~ProductAssetTreeModel()
    {
    }

    void ProductAssetTreeModel::ResetModel()
    {
        if (ap_disableAssetTreeView)
        {
            return;
        }

        m_productToTreeItem.clear();
        m_productIdToTreeItem.clear();
        AZStd::string databaseLocation;
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Broadcast(&AzToolsFramework::AssetDatabase::AssetDatabaseRequests::GetAssetDatabaseLocation, databaseLocation);

        if (databaseLocation.empty())
        {
            return;
        }

        m_sharedDbConnection->QueryCombined(
            [&](AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& combined)
            {
                AddOrUpdateEntry(combined, true);
                return true;
            });
    }

    void ProductAssetTreeModel::OnProductFileChanged(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
    {
        if (ap_disableAssetTreeView)
        {
            return;
        }

        if (!m_root)
        {
            // no need to update the model if the root hasn't been created via ResetModel()
            return;
        }

        // Model changes need to be run on the main thread.
        AZ::SystemTickBus::QueueFunction([&, entry]()
        {
            m_sharedDbConnection->QueryCombinedByProductID(
                entry.m_productID,
                [this](const AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& combined)
                {
                    AddOrUpdateEntry(combined, false);
                    return false; // Should only be 1 entry for 1 product
                });
        });
    }

    void ProductAssetTreeModel::RemoveAsset(AZ::s64 productId)
    {
        auto existingProduct = m_productIdToTreeItem.find(productId);
        if (existingProduct == m_productIdToTreeItem.end() || !existingProduct->second)
        {
            // If the product being removed wasn't cached, then something has gone wrong. Reset the model.
            Reset();
            return;
        }
        RemoveAssetTreeItem(existingProduct->second);
    }

    void ProductAssetTreeModel::RemoveAssetTreeItem(AssetTreeItem* assetToRemove)
    {
        if (!assetToRemove)
        {
            return;
        }

        AssetTreeItem* parent = assetToRemove->GetParent();
        if (!parent)
        {
            return;
        }

        QModelIndex parentIndex = createIndex(parent->GetRow(), 0, parent);
        Q_ASSERT(checkIndex(parentIndex));

        beginRemoveRows(parentIndex, assetToRemove->GetRow(), assetToRemove->GetRow());

        m_productToTreeItem.erase(assetToRemove->GetData()->m_assetDbName);
        const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(assetToRemove->GetData());
        if (productItemData && productItemData->m_hasDatabaseInfo)
        {
            m_productIdToTreeItem.erase(productItemData->m_databaseInfo.m_productID);
        }
        parent->EraseChild(assetToRemove);

        endRemoveRows();

        RemoveFoldersIfEmpty(parent);
    }

    void ProductAssetTreeModel::RemoveFoldersIfEmpty(AssetTreeItem* itemToCheck)
    {
        // Don't attempt to remove invalid items, non-folders, folders that still have items in them, or the root.
        if (!itemToCheck || !itemToCheck->GetData()->m_isFolder || itemToCheck->getChildCount() > 0 || !itemToCheck->GetParent())
        {
            return;
        }
        RemoveAssetTreeItem(itemToCheck);
    }

    void ProductAssetTreeModel::OnProductFileRemoved(AZ::s64 productId)
    {
        if (ap_disableAssetTreeView)
        {
            return;
        }

        if (!m_root)
        {
            // we haven't reset the model yet, which means all of this will happen when we do.
            return;
        }

        // UI changes need to be done on the main thread.
        AZ::SystemTickBus::QueueFunction([&, productId]()
        {
            RemoveAsset(productId);
        });
    }

    void ProductAssetTreeModel::OnProductFilesRemoved(const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& products)
    {
        if (ap_disableAssetTreeView)
        {
            return;
        }

        if (!m_root)
        {
            // we haven't reset the model yet, which means all of this will happen when we do.
            return;
        }

        // UI changes need to be done on the main thread.
        AZ::SystemTickBus::QueueFunction([&, products]()
        {
            for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
            {
                RemoveAsset(product.m_productID);
            }
        });
    }

    QModelIndex ProductAssetTreeModel::GetIndexForProduct(const AZStd::string& product)
    {
        if (ap_disableAssetTreeView)
        {
            return QModelIndex();
        }

        auto productItem = m_productToTreeItem.find(product);
        if (productItem == m_productToTreeItem.end())
        {
            return QModelIndex();
        }
        return createIndex(productItem->second->GetRow(), 0, productItem->second);
    }

    void ProductAssetTreeModel::AddOrUpdateEntry(
        const AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& combinedDatabaseEntry,
        bool modelIsResetting)
    {
        // Intermediate assets are functionally source assets, output as products from other source assets.
        // Don't display them in the product assets tab.
        if (combinedDatabaseEntry.m_flags.test(static_cast<int>(AssetBuilderSDK::ProductOutputFlags::IntermediateAsset)))
        {
            return;
        }

        const auto& existingEntry = m_productIdToTreeItem.find(combinedDatabaseEntry.m_productID);
        if (existingEntry != m_productIdToTreeItem.end())
        {
            AZStd::shared_ptr<ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<ProductAssetTreeItemData>(existingEntry->second->GetData());

            // This item already exists, refresh the related data.
            productItemData->m_databaseInfo = combinedDatabaseEntry; // Intentional object slicing occuring here.  CombinedEntry -> ProductEntry
            CheckForUnresolvedIssues(productItemData);

            QModelIndex existingIndexStart = createIndex(existingEntry->second->GetRow(), 0, existingEntry->second);
            QModelIndex existingIndexEnd = createIndex(existingEntry->second->GetRow(), existingEntry->second->GetColumnCount() - 1, existingEntry->second);
            Q_ASSERT(checkIndex(existingIndexStart));
            Q_ASSERT(checkIndex(existingIndexEnd));
            dataChanged(existingIndexStart, existingIndexEnd);
            return;
        }

        AZ::IO::Path productNamePath(combinedDatabaseEntry.m_productName, AZ::IO::PosixPathSeparator);

        if (productNamePath.empty())
        {
            AZ_Warning(
                "AssetProcessor",
                false,
                "Product id %d has an invalid name: %s",
                combinedDatabaseEntry.m_productID,
                combinedDatabaseEntry.m_productName.c_str());
            return;
        }

        AssetTreeItem* parentItem = m_root.get();
        AZ::IO::Path currentFullFolderPath;
        const AZ::IO::PathView filename = productNamePath.Filename();
        const AZ::IO::PathView fullPathWithoutFilename = productNamePath.RemoveFilename();
        AZ::IO::FixedMaxPathString currentPath;
        for (auto pathIt = fullPathWithoutFilename.begin(); pathIt != fullPathWithoutFilename.end(); ++pathIt)
        {
            currentPath = pathIt->FixedMaxPathString();
            currentFullFolderPath /= currentPath;
            AssetTreeItem* nextParent = parentItem->GetChildFolder(currentPath.c_str());
            if (!nextParent)
            {
                if (!modelIsResetting)
                {
                    QModelIndex parentIndex = parentItem == m_root.get() ? QModelIndex() : createIndex(parentItem->GetRow(), 0, parentItem);
                    Q_ASSERT(checkIndex(parentIndex));
                    beginInsertRows(parentIndex, parentItem->getChildCount(), parentItem->getChildCount());
                }
                nextParent = parentItem->CreateChild(ProductAssetTreeItemData::MakeShared(
                    nullptr,
                    currentFullFolderPath.Native(),
                    currentPath.c_str(),
                    true,
                    AZ::Uuid::CreateNull(),
                    AzToolsFramework::AssetDatabase::InvalidEntryId));
                m_productToTreeItem[currentFullFolderPath.Native()] = nextParent;
                // m_productIdToTreeItem is not used for folders, folders don't have product IDs.

                if (!modelIsResetting)
                {
                    endInsertRows();
                }
            }
            parentItem = nextParent;
        }

        AZ::Uuid sourceId = combinedDatabaseEntry.m_sourceGuid;
        AZ::s64 scanFolderID = combinedDatabaseEntry.m_scanFolderPK;

        if (!modelIsResetting)
        {
            QModelIndex parentIndex = parentItem == m_root.get() ? QModelIndex() : createIndex(parentItem->GetRow(), 0, parentItem);
            Q_ASSERT(checkIndex(parentIndex));
            beginInsertRows(parentIndex, parentItem->getChildCount(), parentItem->getChildCount());
        }

        AZStd::shared_ptr<ProductAssetTreeItemData> productItemData = ProductAssetTreeItemData::MakeShared(
            &combinedDatabaseEntry,
            combinedDatabaseEntry.m_productName,
            AZ::IO::FixedMaxPathString(filename.Native()).c_str(),
            false,
            sourceId,
            scanFolderID);
        m_productToTreeItem[combinedDatabaseEntry.m_productName] =
            parentItem->CreateChild(productItemData);
        m_productIdToTreeItem[combinedDatabaseEntry.m_productID] = m_productToTreeItem[combinedDatabaseEntry.m_productName];

        CheckForUnresolvedIssues(productItemData);

        if (!modelIsResetting)
        {
            endInsertRows();
        }
    }

    void ProductAssetTreeModel::CheckForUnresolvedIssues(AZStd::shared_ptr<ProductAssetTreeItemData> productItemData)
    {
        productItemData->m_assetHasUnresolvedIssue = false;
        // Start by clearing the tooltip, so any errors don't append to the existing text.
        productItemData->m_unresolvedIssuesTooltip = QString();
        if (!productItemData->m_hasDatabaseInfo)
        {
            // Folders can't have unresolved issues.
            return;
        }

        m_sharedDbConnection->QueryMissingProductDependencyByProductId(
            productItemData->m_databaseInfo.m_productID,
            [&, productItemData](AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry& missingDependency)
        {
            if (missingDependency.m_dependencySourceGuid.IsNull())
            {
                // This was an empty row that likely included information like the last time this file was scanned.
                // Don't mark this product as having unresolved issues, and return true to continue looking through the scan results.
                return true;
            }
            // If this asset has any missing dependencies, mark it as having an unresolved issue.
            productItemData->m_assetHasUnresolvedIssue = true;
            productItemData->m_unresolvedIssuesTooltip = tr("A missing product dependency has been detected for this asset.");
            return false; // Don't keep iterating, an unresolved issue was found.
        });
    }
}
