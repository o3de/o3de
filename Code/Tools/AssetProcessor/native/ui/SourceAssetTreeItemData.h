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
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

namespace AssetProcessor
{
    class SourceAssetTreeItemData : public AssetTreeItemData
    {
    public:
        AZ_RTTI(SourceAssetTreeItemData, "{EF56D1E6-4C13-4494-9CB7-02B39A8E3639}", AssetTreeItemData);

        static AZStd::shared_ptr<SourceAssetTreeItemData> MakeShared(
            const AzToolsFramework::AssetDatabase::SourceDatabaseEntry* sourceInfo,
            const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry* scanFolderInfo,
            const AZStd::string& assetDbName,
            QString name,
            bool isFolder);

        SourceAssetTreeItemData(
            const AzToolsFramework::AssetDatabase::SourceDatabaseEntry* sourceInfo,
            const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry* scanFolderInfo,
            const AZStd::string& assetDbName,
            QString name,
            bool isFolder);

        ~SourceAssetTreeItemData() override {}

        AzToolsFramework::AssetDatabase::SourceDatabaseEntry m_sourceInfo;
        AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry m_scanFolderInfo;
        bool m_hasDatabaseInfo = false;
    };
    AZ::Outcome<QString> GetAbsolutePathToSource(const AssetTreeItem& source);
} // AssetProcessor
