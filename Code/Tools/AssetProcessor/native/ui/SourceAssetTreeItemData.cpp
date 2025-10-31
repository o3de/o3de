/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SourceAssetTreeItemData.h"

#include <AzCore/std/smart_ptr/make_shared.h>

#include <QDir>
#include <QVariant>

namespace AssetProcessor
{
    SourceAssetTreeItemData::SourceAssetTreeItemData(
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry* sourceInfo,
        const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry* scanFolderInfo,
        const AZStd::string& assetDbName,
        QString name,
        bool isFolder,
        const AZ::s64 scanFolderID,
        AZ::s64 analysisJobDuration)
        : AssetTreeItemData(assetDbName, name, isFolder, sourceInfo ? sourceInfo->m_sourceGuid : AZ::Uuid::CreateNull(), scanFolderID)
        , m_analysisDuration(analysisJobDuration)
    {
        if (sourceInfo && scanFolderInfo)
        {
            m_hasDatabaseInfo = true;
            m_sourceInfo = *sourceInfo;
            m_scanFolderInfo = *scanFolderInfo;
        }
        else
        {
            m_hasDatabaseInfo = false;
        }
    }

    int SourceAssetTreeItemData::GetColumnCount() const
    {
        return aznumeric_cast<int>(SourceAssetTreeColumns::Max);
    }

    QVariant SourceAssetTreeItemData::GetDataForColumn(int column) const
    {
        if (column == aznumeric_cast<int>(SourceAssetTreeColumns::AnalysisJobDuration))
        {
            if (m_analysisDuration < 0)
            {
                return "";
            }
            QTime duration = QTime::fromMSecsSinceStartOfDay(aznumeric_cast<int>(m_analysisDuration));
            if (duration.hour() > 0)
            {
                return duration.toString("zzz' ms, 'ss' sec, 'mm' min, 'hh' hr'");
            }
            if (duration.minute() > 0)
            {
                return duration.toString("zzz' ms, 'ss' sec, 'mm' min'");
            }
            if (duration.second() > 0)
            {
                return duration.toString("zzz' ms, 'ss' sec'");
            }
            return duration.toString("zzz' ms'");
        }
        else
        {
            return AssetTreeItemData::GetDataForColumn(column);
        }
    }

    QString BuildAbsolutePathToFile(const AZStd::shared_ptr<const SourceAssetTreeItemData> file)
    {
        QDir scanFolder(file->m_scanFolderInfo.m_scanFolder.c_str());
        QString sourceName = file->m_sourceInfo.m_sourceName.c_str();

        return scanFolder.filePath(sourceName);
    }

    AZ::Outcome<QString> GetAbsolutePathToSource(const AssetTreeItem& source)
    {
        if (source.getChildCount() > 0)
        {
            // Folders are special case, they only exist in the interface and don't exist in the asset database.
            // Figure out a path to this folder by finding a descendant that isn't a folder, taking the absolute
            // path of that file, and stripping off the path after this folder.
            size_t directoriesToRemove = 0;
            const AssetTreeItem* searchForFile = &source;
            while (searchForFile)
            {
                if (searchForFile->getChildCount() == 0)
                {
                    const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData =
                        AZStd::rtti_pointer_cast<const SourceAssetTreeItemData>(searchForFile->GetData());
                    if (!sourceItemData)
                    {
                        return AZ::Failure();
                    }
                    QFileInfo fileInfo(BuildAbsolutePathToFile(sourceItemData));
                    QDir fileFolder(fileInfo.absoluteDir());

                    // The file found wasn't a directory, it was removed when absolute dir was called on the QFileInfo above.
                    while (directoriesToRemove > 1)
                    {
                        fileFolder.cdUp();
                        --directoriesToRemove;
                    }
                    return AZ::Success(fileFolder.absolutePath());
                }
                else
                {
                    searchForFile = searchForFile->GetChild(0);
                }
                ++directoriesToRemove;
            }
            return AZ::Failure();
        }
        else
        {
            const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData = AZStd::rtti_pointer_cast<const SourceAssetTreeItemData>(source.GetData());
            if (!sourceItemData)
            {
                return AZ::Failure();
            }
            return AZ::Success(BuildAbsolutePathToFile(sourceItemData));
        }
    }
}
