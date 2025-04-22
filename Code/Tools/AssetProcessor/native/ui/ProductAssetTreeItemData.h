/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AssetTreeItem.h"
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

namespace AZ
{
    struct Uuid;
}

namespace AssetProcessor
{
    class ProductAssetTreeItemData : public AssetTreeItemData
    {
    public:
        AZ_RTTI(ProductAssetTreeItemData, "{6DEFC394-98A3-4EEA-9419-E8F51F447862}", AssetTreeItemData);

        static AZStd::shared_ptr<ProductAssetTreeItemData> MakeShared(
            const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo,
            const AZStd::string& assetDbName,
            QString name,
            bool isFolder,
            const AZ::Uuid& uuid,
            const AZ::s64 scanFolderID);

        ProductAssetTreeItemData(
            const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo,
            const AZStd::string& assetDbName,
            QString name,
            bool isFolder,
            const AZ::Uuid& uuid,
            const AZ::s64 scanFolderID);

        ~ProductAssetTreeItemData() override {}

        AzToolsFramework::AssetDatabase::ProductDatabaseEntry m_databaseInfo;
        bool m_hasDatabaseInfo = false;
    };

    AZ::Outcome<QString> GetAbsolutePathToProduct(const AssetTreeItem& product);
} // AssetProcessor
