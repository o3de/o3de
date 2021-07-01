/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <API/EditorAssetSystemAPI.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option") // conversion from 'int' to 'float', possible loss of data, needs to have dll-interface to be used by clients of class
                                                                    // 'QFlags<QPainter::RenderHint>::Int': forcing value to bool 'true' or 'false' (performance warning)
#include <QMenu>
#include <QHeaderView>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QPen>
#include <QPainter>
#include <QTimer>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTreeView::AssetBrowserTreeView(QWidget* parent)
            : QTreeViewWithStateSaving(parent)
            , m_delegate(new EntryDelegate(this))
            , m_scTimer(new QTimer(this))
        {
            setSortingEnabled(true);
            setItemDelegate(m_delegate);
            header()->hide();
            setContextMenuPolicy(Qt::CustomContextMenu);

            setMouseTracking(true);

            connect(this, &QTreeView::customContextMenuRequested, this, &AssetBrowserTreeView::OnContextMenu);
            connect(m_scTimer, &QTimer::timeout, this, &AssetBrowserTreeView::OnUpdateSCThumbnailsList);

            AssetBrowserViewRequestBus::Handler::BusConnect();
            AssetBrowserComponentNotificationBus::Handler::BusConnect();
        }

        AssetBrowserTreeView::~AssetBrowserTreeView()
        {
            AssetBrowserViewRequestBus::Handler::BusDisconnect();
            AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
        }

        void AssetBrowserTreeView::SetName(const QString& name)
        {
            m_name = name;

            bool isAssetBrowserComponentReady = false;
            AssetBrowserComponentRequestBus::BroadcastResult(isAssetBrowserComponentReady, &AssetBrowserComponentRequests::AreEntriesReady);
            if (isAssetBrowserComponentReady)
            {
                OnAssetBrowserComponentReady();
            }
        }

        void AssetBrowserTreeView::LoadState(const QString& name)
        {
            Q_ASSERT(model());
            auto crc = AZ::Crc32(name.toUtf8().data());
            InitializeTreeViewSaving(crc);
            ApplyTreeViewSnapshot();
        }

        //! Ideally I would capture state on destructor, however
        //! Qt sets the model to NULL before destructor is called
        void AssetBrowserTreeView::SaveState() const
        {
            CaptureTreeViewSnapshot();
        }

        AZStd::vector<AssetBrowserEntry*> AssetBrowserTreeView::GetSelectedAssets() const
        {
            QModelIndexList sourceIndexes;
            for (const auto& index : selectedIndexes())
            {
                sourceIndexes.push_back(m_assetBrowserSortFilterProxyModel->mapToSource(index));
            }

            AZStd::vector<AssetBrowserEntry*> entries;
            m_assetBrowserModel->SourceIndexesToAssetDatabaseEntries(sourceIndexes, entries);
            return entries;
        }

        void AssetBrowserTreeView::SelectProduct(AZ::Data::AssetId assetID)
        {
            if (!assetID.IsValid())
            {
                return;
            }
            SelectProduct(QModelIndex(), assetID);
        }

        void AssetBrowserTreeView::SelectFileAtPath(const AZStd::string& assetPath)
        {
            if (assetPath.empty())
            {
                return;
            }

            auto path = AZStd::string(assetPath);
            if (!AzFramework::StringFunc::Path::Normalize(path))
            {
                return;
            }

            auto entryCache = EntryCache::GetInstance();

            // Find the fileId associated with this assetPath
            AZStd::unordered_map<AZStd::string, AZ::s64>::const_iterator itFileId = entryCache->m_absolutePathToFileId.find(path);
            if (itFileId == entryCache->m_absolutePathToFileId.end())
            {
                return;
            }

            // Find the assetBrowserEntry associated with this fileId
            AZStd::unordered_map<AZ::s64, AssetBrowserEntry*>::const_iterator itABEntry = entryCache->m_fileIdMap.find(itFileId->second);
            if (itABEntry == entryCache->m_fileIdMap.end())
            {
                return;
            }

            // Get all entries in the AssetBrowser-relative path-to-product
            AZStd::vector<AZStd::string> entries;
            AssetBrowserEntry* entry = itABEntry->second;
            do 
            {
                entries.push_back(entry->GetName());
                entry = entry->GetParent();
            } while (entry->GetParent() != nullptr);

            // Entries are in reverse order, so fix this
            AZStd::reverse(entries.begin(), entries.end());

            SelectEntry(QModelIndex(), entries);
        }

        void AssetBrowserTreeView::ClearFilter()
        {
            emit ClearStringFilter();
            emit ClearTypeFilter();
            m_assetBrowserSortFilterProxyModel->FilterUpdatedSlotImmediate();
        }

        void AssetBrowserTreeView::OnAssetBrowserComponentReady()
        {
            if (!m_name.isEmpty())
            {
                auto crc = AZ::Crc32(m_name.toUtf8().data());
                InitializeTreeViewSaving(crc);
                ApplyTreeViewSnapshot();
            }
        }

        void AssetBrowserTreeView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
        {
            // if selected entry is being removed, clear selection so not to select (and attempt to preview) other entries potentially marked for deletion
            if (selectionModel() && selectionModel()->selectedIndexes().size() == 1)
            {
                QModelIndex selectedIndex = selectionModel()->selectedIndexes().first();
                QModelIndex parentSelectedIndex = selectedIndex.parent();
                if (parentSelectedIndex == parent && selectedIndex.row() >= start && selectedIndex.row() <= end)
                {
                    selectionModel()->clear();
                }
            }
            QTreeView::rowsAboutToBeRemoved(parent, start, end);
        }

        void AssetBrowserTreeView::SetThumbnailContext(const char* thumbnailContext) const
        {
            m_delegate->SetThumbnailContext(thumbnailContext);
        }

        void AssetBrowserTreeView::SetShowSourceControlIcons(bool showSourceControlsIcons)
        {
            m_delegate->SetShowSourceControlIcons(showSourceControlsIcons);
            if (showSourceControlsIcons)
            {
                m_scTimer->start(m_scUpdateInterval);
            }
            else
            {
                m_scTimer->stop();
            }
        }

        void AssetBrowserTreeView::UpdateAfterFilter(bool hasFilter, bool selectFirstValidEntry)
        {
            // Flag our default expansion state so that we expand down to source entries after filtering
            m_expandToEntriesByDefault = hasFilter;
            // Then ask our state saver to apply its current snapshot again, falling back on asking us if entries should be expanded or not
            m_treeStateSaver->ApplySnapshot();

            // If we're filtering for a valid entry, select the first valid entry
            if (hasFilter && selectFirstValidEntry)
            {
                QModelIndex curIndex = m_assetBrowserSortFilterProxyModel->index(0, 0);
                while (curIndex.isValid())
                {
                    if (GetEntryFromIndex<SourceAssetBrowserEntry>(curIndex))
                    {
                        setCurrentIndex(curIndex);
                        break;
                    }

                    curIndex = indexBelow(curIndex);
                }
            }
        }

        bool AssetBrowserTreeView::IsIndexExpandedByDefault(const QModelIndex& index) const
        {
            if (!m_expandToEntriesByDefault)
            {
                return false;
            }

            // Expand until we get to source entries, we don't want to go beyond that
            return GetEntryFromIndex<SourceAssetBrowserEntry>(index) == nullptr;
        }

        bool AssetBrowserTreeView::SelectProduct(const QModelIndex& idxParent, AZ::Data::AssetId assetID)
        {
            int elements = model()->rowCount(idxParent);
            for (int idx = 0; idx < elements; ++idx)
            {
                auto rowIdx = model()->index(idx, 0, idxParent);
                auto productEntry = GetEntryFromIndex<ProductAssetBrowserEntry>(rowIdx);
                if (productEntry && productEntry->GetAssetId() == assetID)
                {
                    selectionModel()->clear();
                    selectionModel()->select(rowIdx, QItemSelectionModel::Select);
                    setCurrentIndex(rowIdx);
                    return true;
                }
                if (SelectProduct(rowIdx, assetID))
                {
                    expand(rowIdx);
                    return true;
                }
            }
            return false;
        }

        void AssetBrowserTreeView::SelectFolder(AZStd::string_view folderPath)
        {
            if (folderPath.size() == 0)
            {
                return;
            }

            AZStd::vector<AZStd::string> entries;
            AZ::StringFunc::Tokenize(folderPath, entries, "/");

            SelectEntry(QModelIndex(), entries, 0, true);
        }

        bool AssetBrowserTreeView::SelectEntry(const QModelIndex& idxParent, const AZStd::vector<AZStd::string>& entries, const uint32_t entryPathIndex, bool useDisplayName)
        {
            if (entries.empty())
            {
                return false;
            }

            // The entry name being queried at this depth in the Asset Browser hierarchy
            const AZStd::string& entry = entries.at(entryPathIndex);
            int elements = model()->rowCount(idxParent);
            for (int idx = 0; idx < elements; ++idx)
            {
                auto rowIdx = model()->index(idx, 0, idxParent);
                auto rowEntry = GetEntryFromIndex<AssetBrowserEntry>(rowIdx);

                if (rowEntry)
                {
                    // Check if this entry name matches the query
                    AZStd::string_view compareName = useDisplayName ? (const char*)(rowEntry->GetDisplayName().toUtf8()) : rowEntry->GetName().c_str();

                    if (AzFramework::StringFunc::Equal(entry.c_str(), compareName, true))
                    {
                        // Final entry found - set it as the selected element
                        if (entryPathIndex == entries.size() - 1)
                        {
                            if (rowEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                            {
                                // Expand the item itself if it is a folder
                                expand(rowIdx);
                            }

                            selectionModel()->clear();
                            selectionModel()->select(rowIdx, QItemSelectionModel::Select);
                            setCurrentIndex(rowIdx);

                            return true;
                        }

                        // If this isn't the final entry, it needs to be a folder for the path to be valid (otherwise, early out)
                        if (rowEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                        {
                            // Folder found - if the final entry is found, expand this folder so the final entry is viewable in the Asset
                            // Browser (otherwise, early out)
                            if (SelectEntry(rowIdx, entries, entryPathIndex + 1, useDisplayName))
                            {
                                expand(rowIdx);
                                return true;
                            }
                        }

                        return false;
                    }
                }
            }

            return false;
        }

        void AssetBrowserTreeView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
        {
            QTreeView::selectionChanged(selected, deselected);
            Q_EMIT selectionChangedSignal(selected, deselected);
        }

        void AssetBrowserTreeView::setModel(QAbstractItemModel* model)
        {
            m_assetBrowserSortFilterProxyModel = qobject_cast<AssetBrowserFilterModel*>(model);
            AZ_Assert(m_assetBrowserSortFilterProxyModel, "Expecting AssetBrowserFilterModel");
            m_assetBrowserModel = qobject_cast<AssetBrowserModel*>(m_assetBrowserSortFilterProxyModel->sourceModel());
            QTreeViewWithStateSaving::setModel(model);
        }

        void AssetBrowserTreeView::OnContextMenu(const QPoint& point)
        {
            AZ_UNUSED(point);

            auto selectedAssets = GetSelectedAssets();
            if (selectedAssets.size() != 1)
            {
                return;
            }

            QMenu menu(this);
            AssetBrowserInteractionNotificationBus::Broadcast(
                &AssetBrowserInteractionNotificationBus::Events::AddContextMenuActions, this, &menu, selectedAssets);
            if (!menu.isEmpty())
            {
                menu.exec(QCursor::pos());
            }
        }

        void AssetBrowserTreeView::OnUpdateSCThumbnailsList()
        {
            using namespace Thumbnailer;

            // get top and bottom indexes and find all entries in-between
            QModelIndex topIndex = indexAt(rect().topLeft());
            QModelIndex bottomIndex = indexAt(rect().bottomLeft());

            while (topIndex.isValid())
            {
                auto sourceIndex = m_assetBrowserSortFilterProxyModel->mapToSource(topIndex);
                const auto assetEntry = static_cast<AssetBrowserEntry*>(sourceIndex.internalPointer());
                if (const auto sourceEntry = azrtti_cast<SourceAssetBrowserEntry*>(assetEntry))
                {
                    const SharedThumbnailKey key = sourceEntry->GetSourceControlThumbnailKey();
                    if (key->UpdateThumbnail())
                    {
                        // UpdateThumbnail returns true if it started an actual Source Control operation.
                        // To avoid flooding source control, we'll only allow one of these per check.
                        return;
                    }
                }
                topIndex = indexBelow(topIndex);
                if (topIndex == bottomIndex)
                {
                    break;
                }
            }
        }

        void AssetBrowserTreeView::Update()
        {
            update();
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Views/moc_AssetBrowserTreeView.cpp"
