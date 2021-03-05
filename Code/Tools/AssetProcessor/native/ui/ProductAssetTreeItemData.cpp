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
#include "ProductAssetTreeItemData.h"

#include "native/utilities/assetUtils.h"
#include <AzCore/std/smart_ptr/make_shared.h>

AZ_PUSH_DISABLE_WARNING(4127 4251 4800 4244, "-Wunknown-warning-option")
#include <QDir>
#include <QStack>
AZ_POP_DISABLE_WARNING

namespace AssetProcessor
{

    AZStd::shared_ptr<ProductAssetTreeItemData> ProductAssetTreeItemData::MakeShared(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo, const AZStd::string& assetDbName, QString name, bool isFolder, const AZ::Uuid& uuid)
    {
        return AZStd::make_shared<ProductAssetTreeItemData>(databaseInfo, assetDbName, name, isFolder, uuid);
    }

    ProductAssetTreeItemData::ProductAssetTreeItemData(
        const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo,
        const AZStd::string& assetDbName,
        QString name,
        bool isFolder,
        const AZ::Uuid& uuid) :
        AssetTreeItemData(assetDbName, name, isFolder, uuid)
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
