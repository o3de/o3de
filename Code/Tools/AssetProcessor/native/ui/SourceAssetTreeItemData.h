/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AssetTreeItem.h"
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <QTime>

namespace AssetProcessor
{
    enum class SourceAssetTreeColumns
    {
        AnalysisJobDuration = aznumeric_cast<int>(AssetTreeColumns::Max),
        Max
    };

    class SourceAssetTreeItemData : public AssetTreeItemData
    {
    public:
        AZ_RTTI(SourceAssetTreeItemData, "{EF56D1E6-4C13-4494-9CB7-02B39A8E3639}", AssetTreeItemData);

        SourceAssetTreeItemData(
            const AzToolsFramework::AssetDatabase::SourceDatabaseEntry* sourceInfo,
            const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry* scanFolderInfo,
            const AZStd::string& assetDbName,
            QString name,
            bool isFolder,
            const AZ::s64 scanFolderID,
            AZ::s64 analysisJobDuration = -1);

        ~SourceAssetTreeItemData() override {}
        int GetColumnCount() const override;
        QVariant GetDataForColumn(int column) const override;

        AzToolsFramework::AssetDatabase::SourceDatabaseEntry m_sourceInfo;
        AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry m_scanFolderInfo;
        bool m_hasDatabaseInfo = false;
        AZ::s64 m_analysisDuration;
    };

    AZ::Outcome<QString> GetAbsolutePathToSource(const AssetTreeItem& source);
} // AssetProcessor
