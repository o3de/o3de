/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/ScriptTimePoint.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <QFileInfo>
#include <QtWidgets/QMessageBox>
#include <QHBoxLayout>
#include <QMimeData>
#include <QTimer>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QRegularExpression::d': class 'QExplicitlySharedDataPointer<QRegularExpressionPrivate>' needs to have dll-interface to be used by clients of class 'QRegularExpression'
#include <QRegularExpression>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserModel::AssetBrowserModel(QObject* parent)
            : QAbstractItemModel(parent)
            , m_rootEntry(nullptr)
            , m_loaded(false)
            , m_addingEntry(false)
            , m_removingEntry(false)
        {
            AssetBrowserModelRequestBus::Handler::BusConnect();
            AZ::TickBus::Handler::BusConnect();
        }

        AssetBrowserModel::~AssetBrowserModel()
        {
            AssetBrowserModelRequestBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
        }

        void AssetBrowserModel::EnableTickBus()
        {
            m_isTickBusEnabled = true;
        }

        void AssetBrowserModel::DisableTickBus()
        {
            m_isTickBusEnabled = false;
        }

        QModelIndex AssetBrowserModel::findIndex(const QString& absoluteAssetPath) const
        {
            // Split the path based on either platform's slash
            QRegularExpression regex(QStringLiteral("[\\/]"));

            QStringList assetPathComponents = absoluteAssetPath.split(regex);

            AssetBrowserEntry* cursor = m_rootEntry.get();

            if (cursor && absoluteAssetPath.contains(cursor->GetFullPath().c_str()))
            {
                while (true)
                {
                    // find the child entry that contains more
                    bool foundChild = false;
                    for (int i = 0; i < cursor->GetChildCount(); i++)
                    {
                        AssetBrowserEntry* child = cursor->GetChild(i);
                        if (child)
                        {
                            QString newPath = child->GetFullPath().c_str();
                            if (absoluteAssetPath.startsWith(newPath))
                            {
                                if (absoluteAssetPath == newPath)
                                {
                                    QModelIndex index;
                                    if (GetEntryIndex(child, index))
                                    {
                                        return index;
                                    }
                                }

                                // Confirm that this is a real match as opposed to a partial match.
                                // For instance, an asset absolute path C:/somepath/someotherpath/blah.tga will partial match with c:/somepath/some 
                                // and get us here.

                                QStringList possibleMatchComponents = newPath.split(regex);
                                QString possibleMatchDirectory = possibleMatchComponents.last();
                                Q_ASSERT(assetPathComponents.count() >= possibleMatchComponents.count());
                                if (possibleMatchDirectory == assetPathComponents[possibleMatchComponents.count() - 1])
                                {
                                    cursor = child;
                                    foundChild = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (!foundChild)
                    {
                        break;
                    }
                }
            }

            return QModelIndex();
        }

        QModelIndex AssetBrowserModel::index(int row, int column, const QModelIndex& parent) const
        {
            if (!hasIndex(row, column, parent))
            {
                return QModelIndex();
            }

            AssetBrowserEntry* parentEntry;
            if (!parent.isValid())
            {
                parentEntry = m_rootEntry.get();
            }
            else
            {
                parentEntry = reinterpret_cast<AssetBrowserEntry*>(parent.internalPointer());
            }

            AssetBrowserEntry* childEntry = parentEntry->m_children[row];

            if (!childEntry)
            {
                return QModelIndex();
            }

            QModelIndex index;
            GetEntryIndex(childEntry, index);
            return index;
        }

        int AssetBrowserModel::rowCount(const QModelIndex& parent) const
        {
            if (!m_rootEntry)
            {
                return 0;
            }

            //If the column of the parent is one of those we don't want any more rows as children
            if (parent.isValid())
            {
                if ((parent.column() != aznumeric_cast<int>(AssetBrowserEntry::Column::DisplayName)) &&
                    (parent.column() != aznumeric_cast<int>(AssetBrowserEntry::Column::Name)) &&
                    (parent.column() != aznumeric_cast<int>(AssetBrowserEntry::Column::Path)))
                {
                    return 0;
                }
            }
            
            AssetBrowserEntry* parentAssetEntry;
            if (!parent.isValid())
            {
                parentAssetEntry = m_rootEntry.get();
            }
            else
            {
                parentAssetEntry = static_cast<AssetBrowserEntry*>(parent.internalPointer());
            }
            return parentAssetEntry->GetChildCount();
        }

        int AssetBrowserModel::columnCount(const QModelIndex& /*parent*/) const
        {
            return aznumeric_cast<int>(AssetBrowserEntry::Column::Count);
        }

        QVariant AssetBrowserModel::data(const QModelIndex& index, int role) const
        {
            if (!index.isValid())
            {
                return QVariant();
            }

            if (role == Qt::DisplayRole)
            {
                const AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());
                return item->GetDisplayName();
            }

            if (role == Roles::EntryRole)
            {
                const AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());
                return QVariant::fromValue(item);
            }

            if (role == Qt::EditRole)
            {
                const AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());
                QString displayFileName = item->GetDisplayName();
                QString baseFileName = QFileInfo(displayFileName).baseName();
                return baseFileName;
            }

            return QVariant();
        }

        Qt::ItemFlags AssetBrowserModel::flags(const QModelIndex& index) const
        {
            Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index) | Qt::ItemIsEditable;

            if (index.isValid())
            {
                return AssetBrowserViewUtils::GetAssetBrowserEntryCommonItemFlags(
                    static_cast<AssetBrowserEntry*>(index.internalPointer()), defaultFlags);
            }

            return defaultFlags;
        }
        QStringList AssetBrowserModel::mimeTypes() const
        {
            QStringList list = QAbstractItemModel::mimeTypes();
            list.append(AssetBrowserEntry::GetMimeType());
            return list;
        }

        bool AssetBrowserModel::canDropMimeData(
            const QMimeData* data,
            [[maybe_unused]] Qt::DropAction action,
            [[maybe_unused]] int row,
            [[maybe_unused]] int column,
            [[maybe_unused]] const QModelIndex& parent) const
        {
            if (data->hasFormat(SourceAssetBrowserEntry::GetMimeType()) || data->hasFormat(ProductAssetBrowserEntry::GetMimeType()))
            {
                return true;
            }

            return false;
        }

        bool AssetBrowserModel::dropMimeData(
            const QMimeData* data,
            [[maybe_unused]] Qt::DropAction action,
            [[maybe_unused]] int row,
            [[maybe_unused]] int column,
            const QModelIndex& parent)
        {
            if (action == Qt::IgnoreAction)
                return true;

            const AssetBrowserEntry* item = static_cast<const AssetBrowserEntry*>(parent.internalPointer());

            // We should only have an item as a folder but will check
            if (item && (item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder))
            {
                AZStd::vector<const AssetBrowserEntry*> entries;

                if (Utils::FromMimeData(data, entries))
                {
                    if (entries.empty())
                    {
                        return false;
                    }

                    if (entries.size() > 1)
                    {
                        for (auto assetEntry : entries)
                        {
                            if (assetEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                            {
                                return false;
                            }
                        }
                    }
                    bool isFolder = entries[0]->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder;
                    if (isFolder && AssetBrowserViewUtils::IsEngineOrProjectFolder(entries[0]->GetFullPath()))
                    {
                        return false;
                    }

                    Qt::DropAction selectedAction = AssetBrowserViewUtils::SelectDropActionForEntries(entries);
                    if (selectedAction == Qt::IgnoreAction)
                    {
                        return false;
                    }

                    AZStd::string folderPath = item->GetFullPath();
                    bool connectedToAssetProcessor = false;
                    AzFramework::AssetSystemRequestBus::BroadcastResult(
                        connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::AssetProcessorIsReady);

                    if (connectedToAssetProcessor)
                    {
                        for (auto entry : entries)
                        {
                            using namespace AZ::IO;
                            bool isEmptyFolder = isFolder && AssetBrowserViewUtils::IsFolderEmpty(entry->GetFullPath());
                            Path fromPath;
                            Path toPath;
                            if (isFolder)
                            {
                                Path filename = static_cast<Path>(entry->GetFullPath()).Filename();
                                if (isEmptyFolder)
                                // There is currently a bug in AssetProcessorBatch that doesn't handle empty folders
                                // This code is needed until that bug is fixed. GHI 13340
                                {
                                    fromPath = entry->GetFullPath();
                                    toPath = AZStd::string::format(
                                        "%.*s/%.*s", AZ_STRING_ARG(folderPath), AZ_STRING_ARG(filename.Native()));
                                    AZ::IO::SystemFile::CreateDir(toPath.c_str());
                                    AZ::IO::SystemFile::DeleteDir(fromPath.c_str());
                                    return true;
                                }
                                else
                                {
                                    fromPath = AZStd::string::format("%.*s/*", AZ_STRING_ARG(entry->GetFullPath()));
                                    toPath = AZStd::string::format(
                                        "%.*s/%.*s/*", AZ_STRING_ARG(folderPath), AZ_STRING_ARG(filename.Native()));
                                }
                            }
                            else
                            {
                                fromPath = entry->GetFullPath();
                                PathView filename = fromPath.Filename();
                                toPath = folderPath;
                                toPath /= filename;
                            }

                            if (selectedAction == Qt::MoveAction)
                            {
                                AssetBrowserViewUtils::MoveEntry(fromPath.c_str(), toPath.c_str(), isFolder);
                            }
                            else
                            {
                                AssetBrowserViewUtils::CopyEntry(fromPath.c_str(), toPath.c_str(), isFolder);
                            }
                        }
                        return true;
                    }
                }
            }
            return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
        }

        Qt::DropActions AssetBrowserModel::supportedDropActions() const
        {
            return Qt::CopyAction | Qt::MoveAction;
        }

        QMimeData* AssetBrowserModel::mimeData(const QModelIndexList& indexes) const
        {
            QMimeData* mimeData = new QMimeData;

            AZStd::vector<const AssetBrowserEntry*> collected;
            collected.reserve(indexes.size());

            for (const auto& index : indexes)
            {
                if (index.isValid())
                {
                    const AssetBrowserEntry* item = static_cast<const AssetBrowserEntry*>(index.internalPointer());
                    if (item)
                    {
                        collected.push_back(item);
                    }
                }
            }

            Utils::ToMimeData(mimeData, collected);

            return mimeData;
        }

        QVariant AssetBrowserModel::headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (orientation == Qt::Horizontal && role == Roles::EntryRole)
            {
                return tr(AssetBrowserEntry::m_columnNames[section]);
            }

            return QAbstractItemModel::headerData(section, orientation, role);
        }

        void AssetBrowserModel::SourceIndexesToAssetIds(const QModelIndexList& indexes, AZStd::vector<AZ::Data::AssetId>& assetIds)
        {
            for (const auto& index : indexes)
            {
                if (index.isValid())
                {
                    AssetBrowserEntry* item = static_cast<AssetBrowserEntry*>(index.internalPointer());

                    if (item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
                    {
                        assetIds.push_back(static_cast<ProductAssetBrowserEntry*>(item)->GetAssetId());
                    }
                }
            }
        }

        void AssetBrowserModel::SourceIndexesToAssetDatabaseEntries(const QModelIndexList& indexes, AZStd::vector<const AssetBrowserEntry*>& entries)
        {
            for (const auto& index : indexes)
            {
                if (index.isValid())
                {
                    const AssetBrowserEntry* item = index.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    entries.push_back(item);
                }
            }
        }

        AZStd::shared_ptr<RootAssetBrowserEntry> AssetBrowserModel::GetRootEntry() const
        {
            return m_rootEntry;
        }

        void AssetBrowserModel::SetRootEntry(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry)
        {
            m_rootEntry = rootEntry;
        }

        AssetBrowserFilterModel* AssetBrowserModel::GetFilterModel()
        {
            return m_filterModel;
        }

        const AssetBrowserFilterModel* AssetBrowserModel::GetFilterModel() const
        {
            return m_filterModel;
        }

        void AssetBrowser::AssetBrowserModel::SetFilterModel(AssetBrowserFilterModel* filterModel)
        {
            m_filterModel = filterModel;
        }

        QModelIndex AssetBrowserModel::parent(const QModelIndex& child) const
        {
            if (!child.isValid())
            {
                return QModelIndex();
            }

            AssetBrowserEntry* childAssetEntry = static_cast<AssetBrowserEntry*>(child.internalPointer());
            AssetBrowserEntry* parentEntry = childAssetEntry->GetParent();

            QModelIndex parentIndex;
            if (GetEntryIndex(parentEntry, parentIndex))
            {
                return parentIndex;
            }
            return QModelIndex();
        }

        bool AssetBrowserModel::IsLoaded() const
        {
            return m_loaded;
        }

        void AssetBrowserModel::BeginAddEntry(AssetBrowserEntry* parent)
        {
            if (m_isResetting)
            {
                return; // don't notify during reset.
            }

            QModelIndex parentIndex;
            if (GetEntryIndex(parent, parentIndex))
            {
                m_addingEntry = true;
                int row = parent->GetChildCount();
                Q_EMIT beginInsertRows(parentIndex, row, row);
            }
        }

        void AssetBrowserModel::EndAddEntry(AssetBrowserEntry* parent)
        {
            if (m_isResetting)
            {
                return; // don't notify during reset.
            }

            if (m_addingEntry)
            {
                m_addingEntry = false;
                Q_EMIT endInsertRows();

                // we have to also invalidate our parent all the way up the chain.
                // since in this model, the children's data is actually relevant to the filtering of a parent
                // since a parent "matches" the filter if its children do.
                if (m_rootEntry && !m_rootEntry->IsInitialUpdate())
                {
                    // this is only necessary if its not the initial refresh.
                    AssetBrowserEntry* cursor = parent;
                    while (cursor)
                    {
                        QModelIndex parentIndex;
                        if (GetEntryIndex(cursor, parentIndex))
                        {
                            Q_EMIT dataChanged(parentIndex, parentIndex);
                        }
                        cursor = cursor->GetParent();
                    }
                }

                if (!m_newlyCreatedAssetPathsToCreatorBusIds.empty())
                {
                    // Gets the newest child with the assumption that BeginAddEntry still adds entries at GetChildCount
                    AssetBrowserEntry* newestChildEntry = parent->GetChild(parent->GetChildCount() - 1);
                    WatchForExpectedAssets(newestChildEntry);
                }
            }
        }

        void AssetBrowserModel::BeginRemoveEntry(AssetBrowserEntry* entry)
        {
            if (m_isResetting)
            {
                return; // don't notify during reset.
            }
            
            int row = entry->row();
            QModelIndex parentIndex;
            if (GetEntryIndex(entry->m_parentAssetEntry, parentIndex))
            {
                m_removingEntry = true;
                Q_EMIT beginRemoveRows(parentIndex, row, row);
            }
        }

        void AssetBrowserModel::EndRemoveEntry()
        {
            if (m_isResetting)
            {
                return; // don't notify during reset.
            }

            if (m_removingEntry)
            {
                m_removingEntry = false;
                Q_EMIT endRemoveRows();
            }
        }

        void AssetBrowserModel::BeginReset()
        {
            Q_EMIT beginResetModel();
            m_isResetting = true;
        }

        void AssetBrowserModel::EndReset()
        {
            m_isResetting = false;
            Q_EMIT endResetModel();
        }

        void AssetBrowserModel::HandleAssetCreatedInEditor(const AZStd::string& assetPath, const AZ::Crc32& creatorBusId, const bool initialFilenameChange)
        {
            if (initialFilenameChange)
            {
                QModelIndex index = findIndex(assetPath.c_str());
                if (index.isValid())
                {
                    Q_EMIT RequestOpenItemForEditing(index);
                }
                else
                {
                    m_newlyCreatedAssetPathsToCreatorBusIds[AZ::IO::Path(assetPath).AsPosix()] = creatorBusId;
                }
            }
        }

        void AssetBrowserModel::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/) 
        {
            // if any entries changed since last tick, notify the views
            EntryCache* cache = EntryCache::GetInstance();
            if (m_isTickBusEnabled && cache)
            {
                if (!cache->m_dirtyThumbnailsSet.empty())
                {
                    for (AssetBrowserEntry* entry : cache->m_dirtyThumbnailsSet)
                    {
                        QModelIndex index;
                        if (GetEntryIndex(entry, index))
                        {
                            AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") // conditional expression is constant
                            Q_EMIT dataChanged(index, index, { Qt::DecorationRole });  // thumbnail image is the "decoration"
                            AZ_POP_DISABLE_WARNING
                        }
                    }
                    cache->m_dirtyThumbnailsSet.clear();
                }
            }
        }

        bool AssetBrowserModel::GetEntryIndex(AssetBrowserEntry* entry, QModelIndex& index) const
        {
            if (!entry)
            {
                return false;
            }

            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Root)
            {
                index = QModelIndex();
                return true;
            }

            if (!entry->m_parentAssetEntry)
            {
                return false;
            }

            int row = entry->row();
            int column = 1;
            index = createIndex(row, column, entry);
            return true;
        }

        void AssetBrowserModel::WatchForExpectedAssets(AssetBrowserEntry* entry)
        {
            const AZStd::string& fullpath = AZ::IO::Path(entry->GetFullPath()).AsPosix();
            if (m_newlyCreatedAssetPathsToCreatorBusIds.contains(fullpath))
            {
                if (m_newlyCreatedAssetPathsToCreatorBusIds[fullpath] != AZ::Crc32())
                {
                    m_assetEntriesToCreatorBusIds[entry] = m_newlyCreatedAssetPathsToCreatorBusIds[fullpath];
                }

                m_newlyCreatedAssetPathsToCreatorBusIds.erase(fullpath);

                QTimer::singleShot(0, this,
                    [this, entry]()
                    {
                        QModelIndex index;
                        if (GetEntryIndex(entry, index))
                        {
                            Q_EMIT RequestOpenItemForEditing(index);
                        }
                    });
            }
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/moc_AssetBrowserModel.cpp"
