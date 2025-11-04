/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ProductAssetTreeItemData.h"

#include "native/utilities/assetUtils.h"
#include <AzCore/std/smart_ptr/make_shared.h>

#include <QDir>
#include <QStack>

namespace AssetProcessor
{

    AZStd::shared_ptr<ProductAssetTreeItemData> ProductAssetTreeItemData::MakeShared(
        const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo,
        const AZStd::string& assetDbName,
        QString name,
        bool isFolder,
        const AZ::Uuid& uuid,
        const AZ::s64 scanFolderID)
    {
        return AZStd::make_shared<ProductAssetTreeItemData>(databaseInfo, assetDbName, name, isFolder, uuid, scanFolderID);
    }

    ProductAssetTreeItemData::ProductAssetTreeItemData(
        const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo,
        const AZStd::string& assetDbName,
        QString name,
        bool isFolder,
        const AZ::Uuid& uuid,
        const AZ::s64 scanFolderID)
        :
        AssetTreeItemData(assetDbName, name, isFolder, uuid, scanFolderID)
    {
        if (databaseInfo)
        {
            m_hasDatabaseInfo = true;
            m_databaseInfo = *databaseInfo;
        }
        else
        {
            m_hasDatabaseInfo = false;
        }

    }

    AZ::Outcome<QString> GetAbsolutePathToProduct(const AssetTreeItem& product)
    {
        QDir cacheRootDir;
        if (!AssetUtilities::ComputeProjectCacheRoot(cacheRootDir))
        {
            return AZ::Failure();
        }
        QString pathOnDisk;
        if (product.getChildCount() > 0)
        {
            // Folders are special case, they only exist in the interface and don't exist in the asset database.
            // Figure out the path to the folder by creating a stack of each folder in its hierarchy.
            QStack<QString> folderStack;
            for (const AssetTreeItem* folderHierarchy = &product; folderHierarchy != nullptr; folderHierarchy = folderHierarchy->GetParent())
            {
                folderStack.push(folderHierarchy->GetData()->m_name);
            }
            while (!folderStack.empty())
            {
                cacheRootDir.cd(folderStack.pop());
            }
            pathOnDisk = cacheRootDir.absolutePath();
        }
        else
        {
            const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(product.GetData());
            if (!productItemData)
            {
                return AZ::Failure();
            }
            pathOnDisk = cacheRootDir.filePath(productItemData->m_databaseInfo.m_productName.c_str());
        }
        return AZ::Success(pathOnDisk);
    }
}
