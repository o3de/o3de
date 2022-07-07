/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SourceAssetTreeModel.h"
#include "SourceAssetTreeItemData.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Path/Path.h>
#include <native/utilities/assetUtils.h>
#include <AzCore/Console/IConsole.h>
#include <QDebug>

namespace AssetProcessor
{
    AZ_CVAR(bool, ap_disableAssetTreeView, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Disable asset tree for automated tests.");

    SourceAssetTreeModel::SourceAssetTreeModel(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection, QObject* parent) :
        AssetTreeModel(sharedDbConnection, parent)
    {
    }

    SourceAssetTreeModel::~SourceAssetTreeModel()
    {
    }

    void SourceAssetTreeModel::ResetModel()
    {
        if (ap_disableAssetTreeView)
        {
            return;
        }

        m_sourceToTreeItem.clear();
        m_sourceIdToTreeItem.clear();

        m_sharedDbConnection->QuerySourceAndScanfolder(
            [&](AzToolsFramework::AssetDatabase::SourceAndScanFolderDatabaseEntry& sourceAndScanFolder)
            {
                AddOrUpdateEntry(sourceAndScanFolder, sourceAndScanFolder, true);
                return true; // return true to continue iterating over additional results, we are populating a container
            });
    }

    void SourceAssetTreeModel::AddOrUpdateEntry(
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source,
        const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& scanFolder,
        bool modelIsResetting)
    {
        const auto& existingEntry = m_sourceToTreeItem.find(source.m_sourceName);
        if (existingEntry != m_sourceToTreeItem.end())
        {
            AZStd::shared_ptr<SourceAssetTreeItemData> sourceItemData = AZStd::rtti_pointer_cast<SourceAssetTreeItemData>(existingEntry->second->GetData());

            // This item already exists, refresh the related data.
            sourceItemData->m_scanFolderInfo = scanFolder;
            sourceItemData->m_sourceInfo = source;
            QModelIndex existingIndexStart = createIndex(existingEntry->second->GetRow(), 0, existingEntry->second);
            QModelIndex existingIndexEnd = createIndex(existingEntry->second->GetRow(), existingEntry->second->GetColumnCount() - 1, existingEntry->second);
            dataChanged(existingIndexStart, existingIndexEnd);
            return;
        }


        AZ::IO::Path fullPath = AZ::IO::Path(scanFolder.m_scanFolder) / source.m_sourceName;

        // It's common for Open 3D Engine game projects and scan folders to be in a subfolder
        // of the engine install. To improve readability of the source files, strip out
        // that portion of the path if it overlaps.
        if (!m_assetRootSet)
        {
            m_assetRootSet = AssetUtilities::ComputeAssetRoot(m_assetRoot, nullptr);
        }
        if (m_assetRootSet)
        {
            fullPath = fullPath.LexicallyProximate(m_assetRoot.absolutePath().toUtf8().constData());
        }

        if (fullPath.empty())
        {
            AZ_Warning(
                "AssetProcessor", false, "Source id %s has an invalid name: %s", source.m_sourceGuid.ToString<AZStd::string>().c_str(),
                source.m_sourceName.c_str());
            return;
        }

        AssetTreeItem* parentItem = m_root.get();
        // Use posix path separator for each child item
        AZ::IO::Path currentFullFolderPath(AZ::IO::PosixPathSeparator);
        const AZ::IO::FixedMaxPath filename = fullPath.Filename();
        fullPath.RemoveFilename();
        AZ::IO::FixedMaxPathString currentPath;
        for (auto pathIt = fullPath.begin(); pathIt != fullPath.end(); ++pathIt)
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
                nextParent = parentItem->CreateChild(SourceAssetTreeItemData::MakeShared(nullptr, nullptr, currentFullFolderPath.Native(), currentPath.c_str(), true));
                m_sourceToTreeItem[currentFullFolderPath.Native()] = nextParent;
                // Folders don't have source IDs, don't add to m_sourceIdToTreeItem
                if (!modelIsResetting)
                {
                    endInsertRows();
                }
            }
            parentItem = nextParent;
        }

        if (!modelIsResetting)
        {
            QModelIndex parentIndex = parentItem == m_root.get() ? QModelIndex() : createIndex(parentItem->GetRow(), 0, parentItem);
            Q_ASSERT(checkIndex(parentIndex));
            beginInsertRows(parentIndex, parentItem->getChildCount(), parentItem->getChildCount());
        }

        m_sourceToTreeItem[source.m_sourceName] =
            parentItem->CreateChild(SourceAssetTreeItemData::MakeShared(&source, &scanFolder, source.m_sourceName, AZ::IO::FixedMaxPathString(filename.Native()).c_str(), false));
        m_sourceIdToTreeItem[source.m_sourceID] = m_sourceToTreeItem[source.m_sourceName];
        if (!modelIsResetting)
        {
            endInsertRows();
        }
    }

    void SourceAssetTreeModel::OnSourceFileChanged(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
    {
        if (ap_disableAssetTreeView)
        {
            return;
        }

        // Model changes need to be run on the main thread.
        AZ::SystemTickBus::QueueFunction([&, entry]()
            {
                m_sharedDbConnection->QueryScanFolderBySourceID(entry.m_sourceID,
                    [&, entry](AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& scanFolder)
                    {
                        AddOrUpdateEntry(entry, scanFolder, false);
                        return true;
                    });
            });
    }

    void SourceAssetTreeModel::RemoveFoldersIfEmpty(AssetTreeItem* itemToCheck)
    {
        // Don't attempt to remove invalid items, non-folders, folders that still have items in them, or the root.
        if (!itemToCheck || !itemToCheck->GetData()->m_isFolder || itemToCheck->getChildCount() > 0 || !itemToCheck->GetParent())
        {
            return;
        }
        RemoveAssetTreeItem(itemToCheck);
    }

    void SourceAssetTreeModel::RemoveAssetTreeItem(AssetTreeItem* assetToRemove)
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

        QModelIndex parentIndex = parent == m_root.get() ? QModelIndex() : createIndex(parent->GetRow(), 0, parent);
        Q_ASSERT(checkIndex(parentIndex));

        beginRemoveRows(parentIndex, assetToRemove->GetRow(), assetToRemove->GetRow());

        m_sourceToTreeItem.erase(assetToRemove->GetData()->m_assetDbName);
        const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData = AZStd::rtti_pointer_cast<const SourceAssetTreeItemData>(assetToRemove->GetData());
        if (sourceItemData && sourceItemData->m_hasDatabaseInfo)
        {
            m_sourceIdToTreeItem.erase(sourceItemData->m_sourceInfo.m_sourceID);
        }
        parent->EraseChild(assetToRemove);

        endRemoveRows();

        RemoveFoldersIfEmpty(parent);
    }

    void SourceAssetTreeModel::OnSourceFileRemoved(AZ::s64 sourceId)
    {
        if (ap_disableAssetTreeView)
        {
            return;
        }

        // UI changes need to be done on the main thread.
        AZ::SystemTickBus::QueueFunction([&, sourceId]()
            {
                auto existingSource = m_sourceIdToTreeItem.find(sourceId);
                if (existingSource == m_sourceIdToTreeItem.end() || !existingSource->second)
                {
                    // If the asset being removed wasn't previously cached, then something has gone wrong. Reset the model.
                    Reset();
                    return;
                }
                RemoveAssetTreeItem(existingSource->second);
            });
    }

    QModelIndex SourceAssetTreeModel::GetIndexForSource(const AZStd::string& source)
    {
        if (ap_disableAssetTreeView)
        {
            return QModelIndex();
        }

        auto sourceItem = m_sourceToTreeItem.find(source);
        if (sourceItem == m_sourceToTreeItem.end())
        {
            return QModelIndex();
        }
        return createIndex(sourceItem->second->GetRow(), 0, sourceItem->second);
    }
}
