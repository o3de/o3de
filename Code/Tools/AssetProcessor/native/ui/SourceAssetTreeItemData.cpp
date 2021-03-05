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
#include "SourceAssetTreeItemData.h"

#include <AzCore/std/smart_ptr/make_shared.h>

#include <QDir>

namespace AssetProcessor
{

    AZStd::shared_ptr<SourceAssetTreeItemData> SourceAssetTreeItemData::MakeShared(
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry* sourceInfo,
        const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry* scanFolderInfo,
        const AZStd::string& assetDbName,
        QString name,
        bool isFolder)
    {
        return AZStd::make_shared<SourceAssetTreeItemData>(sourceInfo, scanFolderInfo, assetDbName, name, isFolder);
    }

    SourceAssetTreeItemData::SourceAssetTreeItemData(
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry* sourceInfo,
        const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry* scanFolderInfo,
        const AZStd::string& assetDbName,
        QString name,
        bool isFolder) :
        AssetTreeItemData(assetDbName, name, isFolder, sourceInfo ? sourceInfo->m_sourceGuid : AZ::Uuid::CreateNull())
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

    QString BuildAbsolutePathToFile(const AZStd::shared_ptr<const SourceAssetTreeItemData> file)
    {
        QDir scanFolder(file->m_scanFolderInfo.m_scanFolder.c_str());
        QString sourceName = file->m_sourceInfo.m_sourceName.c_str();

        // If a scan folder has a prefix, then source files in those scan folders will have that
        // prefix prepended in the asset database. Strip that prefix off for building the actual absolute path to the file.
        if (!file->m_scanFolderInfo.m_outputPrefix.empty())
        {
            QRegExp prefixRemovalRegex(QString("^%1/").arg(file->m_scanFolderInfo.m_outputPrefix.c_str()));
            sourceName.remove(prefixRemovalRegex);
        }
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
