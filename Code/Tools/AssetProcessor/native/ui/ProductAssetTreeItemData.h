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

        static AZStd::shared_ptr<ProductAssetTreeItemData> MakeShared(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo, const AZStd::string& assetDbName, QString name, bool isFolder, const AZ::Uuid& uuid);

        ProductAssetTreeItemData(const AzToolsFramework::AssetDatabase::ProductDatabaseEntry* databaseInfo, const AZStd::string& assetDbName, QString name, bool isFolder, const AZ::Uuid& uuid);

        ~ProductAssetTreeItemData() override {}

        AzToolsFramework::AssetDatabase::ProductDatabaseEntry m_databaseInfo;
        bool m_hasDatabaseInfo = false;
    };

    AZ::Outcome<QString> GetAbsolutePathToProduct(const AssetTreeItem& product);
} // AssetProcessor
