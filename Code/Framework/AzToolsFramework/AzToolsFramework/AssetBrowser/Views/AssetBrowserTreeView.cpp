/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <API/EditorAssetSystemAPI.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/ranges/split_view.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeViewDialog.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTableView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserThumbnailView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <AzQtComponents/Components/Widgets/MessageBox.h>
#include <AzQtComponents/DragAndDrop/MainWindowDragAndDrop.h>

#include <QDir>
#include <QMenu>
#include <QFile>
#include <QHeaderView>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QLineEdit>
#include <QPen>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QtWidgets/QMessageBox>
#include <QAbstractButton>
#include <QHBoxLayout>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        static constexpr const char* const TreeViewMainViewName = "AssetBrowserTreeView_main";

        AssetBrowserTreeView::AssetBrowserTreeView(QWidget* parent)
            : QTreeViewWithStateSaving(parent)
            , m_delegate(new EntryDelegate(this))
            , m_scTimer(new QTimer(this))
        {
            setSortingEnabled(true);
            setItemDelegate(m_delegate);
            connect(m_delegate, &EntryDelegate::RenameEntry, this, &AssetBrowserTreeView::AfterRename);

            header()->hide();
            setSelectionMode(QAbstractItemView::ExtendedSelection);
            setDragEnabled(true);
            setAcceptDrops(true);
            setDragDropMode(QAbstractItemView::DragDrop);
            setDropIndicatorShown(true);
            setContextMenuPolicy(Qt::CustomContextMenu);
            setDragDropOverwriteMode(true);

            setMouseTracking(true);

            connect(this, &QTreeView::customContextMenuRequested, this, &AssetBrowserTreeView::OnContextMenu);
            connect(m_scTimer, &QTimer::timeout, this, &AssetBrowserTreeView::OnUpdateSCThumbnailsList);

            AssetBrowserViewRequestBus::Handler::BusConnect();
            AssetBrowserComponentNotificationBus::Handler::BusConnect();
            AssetBrowserInteractionNotificationBus::Handler::BusConnect();

            // Assign this widget to the Editor Asset Browser Action Context.
            AzToolsFramework::AssignWidgetToActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);

            QAction* deleteAction = new QAction("Delete Action", this);
            deleteAction->setShortcut(QKeySequence::Delete);
            deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            connect(
                deleteAction, &QAction::triggered, this, [this]()
                {
                    DeleteEntries();
                });
            addAction(deleteAction);

            QAction* renameAction = new QAction("Rename Action", this);
            renameAction->setShortcut(Qt::Key_F2);
            renameAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            connect(
                renameAction, &QAction::triggered, this, [this]()
                {
                    RenameEntry();
                });
            addAction(renameAction);

            QAction* duplicateAction = new QAction("Duplicate Action", this);
            duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
            duplicateAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            connect(
                duplicateAction, &QAction::triggered, this, [this]()
                {
                    DuplicateEntries();
                });
            addAction(duplicateAction);

            connect(this, &QAbstractItemView::clicked, this, &AssetBrowserTreeView::itemClicked);
        }

        AssetBrowserTreeView::~AssetBrowserTreeView()
        {
            m_assetBrowserSortFilterProxyModel = nullptr;
            AzToolsFramework::RemoveWidgetFromActionContextHelper(EditorIdentifiers::EditorAssetBrowserActionContextIdentifier, this);

            AssetBrowserViewRequestBus::Handler::BusDisconnect();
            AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
            AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
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

        void AssetBrowserTreeView::SetIsAssetBrowserMainView()
        {
            SetName(TreeViewMainViewName);
        }

        bool AssetBrowserTreeView::GetIsAssetBrowserMainView()
        {
            return GetName() == TreeViewMainViewName;
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

        void AssetBrowserTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
        {
            if (!index.parent().isValid() && ! selectedIndexes().contains(index))
            {
                painter->fillRect(rect, 0x333333);
            }

            QTreeView::drawBranches(painter, rect, index);
        }

        AZStd::vector<const AssetBrowserEntry*> AssetBrowserTreeView::GetSelectedAssets(bool includeProducts) const
        {
            const QModelIndexList& selectedIndexes = selectionModel()->selectedRows();
            QModelIndexList sourceIndexes;
            sourceIndexes.reserve(selectedIndexes.size());
            for (const auto& index : selectedIndexes)
            {
                sourceIndexes.push_back(m_assetBrowserSortFilterProxyModel->mapToSource(index));
            }

            AZStd::vector<const AssetBrowserEntry*> entries;
            m_assetBrowserModel->SourceIndexesToAssetDatabaseEntries(sourceIndexes, entries);

            if (!includeProducts)
            {
                AZStd::erase_if(
                    entries,
                    [&](const AssetBrowserEntry* entry) -> bool
                    {
                        return entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product;
                    });
            }

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

        void AssetBrowserTreeView::SelectFileAtPathAfterUpdate(const AZStd::string& assetPath)
        {
            m_fileToSelectAfterUpdate = assetPath;
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

            uint32_t lastEntry = aznumeric_cast<uint32_t>(entries.size()) - 1;

            // If we're in the thumbnail or table view, the actual asset will not appear in this treeview.
            // Trying to find the file in the treeview will fail, so don't search to the end.
            if ((m_attachedThumbnailView && m_attachedThumbnailView->GetThumbnailActiveView()) ||
                (m_attachedTableView && m_attachedTableView->GetTableViewActive()))
            {
                lastEntry = aznumeric_cast<uint32_t>(entries.size()) - 2;
            }

            SelectEntry(QModelIndex(), entries, lastEntry);

            
            if (m_attachedThumbnailView)
            {
                m_attachedThumbnailView->SelectEntry(entries.back().data());
            }
            if (m_attachedTableView)
            {
                m_attachedTableView->SelectEntry(entries.back().data());
            }
        }

        void AssetBrowserTreeView::ClearFilter()
        {
            emit ClearStringFilter();
            emit ClearTypeFilter();
            m_assetBrowserSortFilterProxyModel->FilterUpdatedSlotImmediate();
        }

        void AssetBrowserTreeView::OnAssetBrowserComponentReady()
        {
            hideColumn(aznumeric_cast<int>(AssetBrowserEntry::Column::Path));
            if (!m_name.isEmpty())
            {
                auto crc = AZ::Crc32(m_name.toUtf8().data());
                InitializeTreeViewSaving(crc);
                ApplyTreeViewSnapshot();
                if (!selectionModel()->hasSelection())
                {
                    QModelIndex firstItem = model()->index(0, 0);
                    selectionModel()->select(firstItem, QItemSelectionModel::ClearAndSelect);
                }
            }
        }

        void AssetBrowserTreeView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
        {
            // if selected entry is being removed, clear selection so not to select (and attempt to preview) other entries potentially
            // marked for deletion
            if (selectionModel() && selectedIndexes().size() == 1)
            {
                QModelIndex selectedIndex = selectedIndexes().first();
                QModelIndex parentSelectedIndex = selectedIndex.parent();
                if (parentSelectedIndex == parent && selectedIndex.row() >= start && selectedIndex.row() <= end)
                {
                    selectionModel()->clear();
                }
            }
            QTreeView::rowsAboutToBeRemoved(parent, start, end);
        }

        // Item data for hidden columns normally isn't copied by Qt during drag-and-drop (see QTBUG-30242).
        // However, for the AssetBrowser, the hidden columns should get copied. By overriding selectedIndexes() to
        // include all selected indices, not just the visible ones, we can get the behavior we're looking for.
        QModelIndexList AssetBrowserTreeView::selectedIndexes() const
        {
            return selectionModel()->selectedIndexes();
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
            const QModelIndexList& selectedIndexes = selectionModel()->selectedRows();

            // If we've cleared the filter but had something selected, ensure it stays selected and visible.
            if (!hasFilter && !selectedIndexes.isEmpty() && m_fileToSelectAfterUpdate.empty())
            {
                QModelIndex curIndex = selectedIndexes[0];
                m_expandToEntriesByDefault = true;
                if (m_treeStateSaver && m_applySnapshot)
                {
                    m_treeStateSaver->ApplySnapshot();
                }

                setCurrentIndex(curIndex);
                scrollTo(curIndex, QAbstractItemView::ScrollHint::PositionAtCenter);

                return;
            }

            // Flag our default expansion state so that we expand down to source entries after filtering
            m_expandToEntriesByDefault = hasFilter;

            // Then ask our state saver to apply its current snapshot again, falling back on asking us if entries should be expanded or not
            if (m_treeStateSaver && m_applySnapshot)
            {
                m_treeStateSaver->ApplySnapshot();
            }

            // If we're filtering for a valid entry, select the first valid entry
            if (hasFilter && selectFirstValidEntry)
            {
                bool selected = false;
                const QModelIndex firstIndex = m_assetBrowserSortFilterProxyModel->index(0, 0);
                for (QModelIndex curIndex = firstIndex; curIndex.isValid() && !selected; curIndex = indexBelow(curIndex))
                {
                    if (GetEntryFromIndex<SourceAssetBrowserEntry>(curIndex))
                    {
                        setCurrentIndex(curIndex);
                        selected = true;
                    }
                }
            }

            if (m_indexToSelectAfterUpdate.isValid())
            {
                selectionModel()->select(m_indexToSelectAfterUpdate, QItemSelectionModel::ClearAndSelect);
                m_indexToSelectAfterUpdate = QModelIndex();
            }

            if (!m_fileToSelectAfterUpdate.empty())
            {
                SelectFileAtPath(m_fileToSelectAfterUpdate);
                m_fileToSelectAfterUpdate.clear();
            }

            m_applySnapshot = true;
        }

        void AssetBrowserTreeView::SetApplySnapshot(bool shouldApplySnapshot)
        {
            m_applySnapshot = shouldApplySnapshot;
        }

        const AssetBrowserEntry* AssetBrowserTreeView::GetEntryByPath(QStringView path)
        {
            QModelIndex current;
            const QByteArray byteArray = path.toUtf8();
            const AZ::IO::PathView azpath{ AZStd::string_view{ byteArray.constData(), static_cast<size_t>(byteArray.size()) } };
            for (const auto& pathPart : azpath)
            {
                const QModelIndexList next = model()->match(
                    /*start =*/model()->index(0, 0, current),
                    /*role =*/Qt::DisplayRole,
                    /*value =*/QString::fromUtf8(pathPart.Native().data(), static_cast<int32_t>(pathPart.Native().size())),
                    /*hits =*/1,
                    /*flags =*/Qt::MatchExactly);
                if (next.size() == 1)
                {
                    current = next[0];
                }
                else if (current.isValid())
                {
                    return nullptr;
                }
            }
            return GetEntryFromIndex<AssetBrowserEntry>(current);
        }

        bool AssetBrowserTreeView::IsIndexExpandedByDefault(const QModelIndex& index) const
        {
            return m_expandToEntriesByDefault && (GetEntryFromIndex<SourceAssetBrowserEntry>(index) == nullptr);
        }

        void AssetBrowserTreeView::OpenItemForEditing(const QModelIndex& index)
        {
            QModelIndex proxyIndex = m_assetBrowserSortFilterProxyModel->mapFromSource(index);

            if (proxyIndex.isValid())
            {
                selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
                setCurrentIndex(proxyIndex);

                scrollTo(proxyIndex, QAbstractItemView::ScrollHint::PositionAtCenter);

                RenameEntry();
            }
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
                    selectionModel()->select(rowIdx, QItemSelectionModel::ClearAndSelect);
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

        void AssetBrowserTreeView::SelectFolderFromBreadcrumbsPath(AZStd::string_view folderPath)
        {
            if (folderPath.empty())
            {
                return;
            }

            AZStd::vector<AZStd::string> entries;
            AZ::StringFunc::Tokenize(folderPath, entries, "/");

            SelectEntry(QModelIndex(), entries, aznumeric_cast<uint32_t>(entries.size()) - 1, 0, true);
        }


        void AssetBrowserTreeView::SelectFolder(AZStd::string_view folderPath)
        {
            if (folderPath.empty())
            {
                return;
            }

            auto entryCache = EntryCache::GetInstance();

            AZStd::string normalizedPath = AZ::IO::PathView(folderPath).LexicallyNormal().String();

            AZStd::unordered_map<AZStd::string, AZ::s64>::const_iterator itFileId = entryCache->m_absolutePathToFileId.find(normalizedPath.c_str());
            if (itFileId == entryCache->m_absolutePathToFileId.end())
            {
                return;
            }

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

            AZStd::reverse(entries.begin(), entries.end());

            uint32_t lastEntry = aznumeric_cast<uint32_t>(entries.size()) - 1;

            SelectEntry(QModelIndex(), entries, lastEntry, 0, true);
        }

        bool AssetBrowserTreeView::SelectEntry(const QModelIndex& idxParent, const AZStd::vector<AZStd::string>& entries, const uint32_t lastFolderIndex, const uint32_t entryPathIndex, bool useDisplayName)
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
                    QByteArray displayName = rowEntry->GetDisplayName().toUtf8();
                    AZStd::string_view compareName = useDisplayName ? displayName.constData() : rowEntry->GetName().c_str();

                    if (AzFramework::StringFunc::Equal(entry.c_str(), compareName, true))
                    {
                        // Final entry found - set it as the selected element
                        if (entryPathIndex == lastFolderIndex)
                        {
                            if (rowEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                            {
                                // Expand the item itself if it is a folder
                                expand(rowIdx);
                            }

                            selectionModel()->select(rowIdx, QItemSelectionModel::ClearAndSelect);
                            setCurrentIndex(rowIdx);

                            return true;
                        }

                        // If this isn't the final entry, it needs to be a folder for the path to be valid (otherwise, early out)
                        if (rowEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                        {
                            // Folder found - if the final entry is found, expand this folder so the final entry is viewable in the Asset
                            // Browser (otherwise, early out)
                            if (SelectEntry(rowIdx, entries, lastFolderIndex, entryPathIndex + 1, useDisplayName))
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

        void AssetBrowserTreeView::itemClicked(const QModelIndex& index)
        {
            // if we click on an item and selection wasn't changed, then reselect the current item so that it shows up in
            // any related previewers.  If its not currently selected, then the above selectionChanged will take care of it.
            if (selectionModel()->isSelected(index))
            {
                Q_EMIT selectionChangedSignal(selectionModel()->selection(), {});
            }
        }

        void AssetBrowserTreeView::setModel(QAbstractItemModel* model)
        {
            m_assetBrowserSortFilterProxyModel = qobject_cast<AssetBrowserFilterModel*>(model);
            AZ_Assert(m_assetBrowserSortFilterProxyModel, "Expecting AssetBrowserFilterModel");
            m_assetBrowserModel = qobject_cast<AssetBrowserModel*>(m_assetBrowserSortFilterProxyModel->sourceModel());
            QTreeViewWithStateSaving::setModel(model);

            SetSortMode(AssetBrowserEntry::AssetEntrySortMode::Name);
        }

        void AssetBrowserTreeView::dragEnterEvent(QDragEnterEvent* event)
        {
            if (event->mimeData()->hasFormat(SourceAssetBrowserEntry::GetMimeType()) ||
                event->mimeData()->hasFormat(ProductAssetBrowserEntry::GetMimeType()))
            {
                event->accept();
                return;
            }

            using namespace AzQtComponents;
            DragAndDropContextBase context;
            DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragEnter, event, context);
        }

        void AssetBrowserTreeView::dragMoveEvent(QDragMoveEvent* event)
        {
            if (event->mimeData()->hasFormat(SourceAssetBrowserEntry::GetMimeType()) ||
                event->mimeData()->hasFormat(ProductAssetBrowserEntry::GetMimeType()))
            {
                event->accept();
                return;
            }

            using namespace AzQtComponents;
            DragAndDropContextBase context;
            DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragMove, event, context);
        }

        void AssetBrowserTreeView::dropEvent(QDropEvent* event)
        {
            QModelIndex targetIndex = indexAt(event->pos());
            if (!targetIndex.isValid())
            {
                event->ignore();
                return;
            }
            if (event->mimeData()->hasFormat(SourceAssetBrowserEntry::GetMimeType()) ||
                event->mimeData()->hasFormat(ProductAssetBrowserEntry::GetMimeType()))
            {
                auto sourceIndex = m_assetBrowserSortFilterProxyModel->mapToSource(targetIndex);

                const AssetBrowserEntry* targetitem = static_cast<const AssetBrowserEntry*>(sourceIndex.internalPointer());
                while (!targetitem->RTTI_IsTypeOf(FolderAssetBrowserEntry::RTTI_Type()))
                {
                    sourceIndex = sourceIndex.parent();
                    targetitem = static_cast<const AssetBrowserEntry*>(sourceIndex.internalPointer());
                }

                m_assetBrowserModel->dropMimeData(
                    event->mimeData(), Qt::CopyAction, targetIndex.row(), targetIndex.column(), sourceIndex);
                return;
            }

            const AssetBrowserEntry* item = targetIndex.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();

            AZStd::string pathName = item->GetFullPath();

            using namespace AzQtComponents;
            DragAndDropContextBase context;
            DragAndDropEventsBus::Event(
                DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DropAtLocation, event, context, QString(pathName.c_str()));
        }

        void AssetBrowserTreeView::dragLeaveEvent(QDragLeaveEvent* event)
        {
            using namespace AzQtComponents;
            DragAndDropEventsBus::Event(DragAndDropContexts::EditorMainWindow, &DragAndDropEvents::DragLeave, event);
        }

        void AssetBrowserTreeView::OnContextMenu(const QPoint& point)
        {
            AZ_UNUSED(point);

            auto selectedAssets = GetSelectedAssets();

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
            if (!m_updateRequested)
            {
                m_updateRequested = true;
                QTimer::singleShot(
                    0,
                    this,
                    [this]()
                    {
                        m_updateRequested = false;
                        if (model())
                        {
                            model()->layoutChanged();
                        }
                        update();
                    });
            }
        }

        void AssetBrowserTreeView::DeleteEntries()
        {
            const auto& entries = GetSelectedAssets(false); // you cannot delete product files.
            AssetBrowserViewUtils::DeleteEntries(entries, this);
        }

        void AssetBrowserTreeView::RenameEntry()
        {
            const auto& entries = GetSelectedAssets(false); // you cannot rename product files.
            if (AssetBrowserViewUtils::RenameEntry(entries, this))
            {
                edit(currentIndex());
            }
        }

        void AssetBrowserTreeView::AfterRename(QString newVal)
        {
            const auto& entries = GetSelectedAssets(false); // you cannot rename product files.
            AssetBrowserViewUtils::AfterRename(newVal, entries, this);
        }

        void AssetBrowserTreeView::DuplicateEntries()
        {
            const auto& entries = GetSelectedAssets(false); // you may not duplicate product files.
            AssetBrowserViewUtils::DuplicateEntries(entries);
        }

        void AssetBrowserTreeView::MoveEntries()
        {
            const auto& entries = GetSelectedAssets(false); // you cannot move product files.
            AssetBrowserViewUtils::MoveEntries(entries, this);
        }

        void AssetBrowserTreeView::AddSourceFileCreators(
            [[maybe_unused]] const char* fullSourceFolderName,
            [[maybe_unused]] const AZ::Uuid& sourceUUID,
            AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators)
        {
            auto it = std::find_if(creators.begin(), creators.end(),
                [&](const AzToolsFramework::AssetBrowser::SourceFileCreatorDetails& creator)
                {
                    return creator.m_identifier == "Folder_Creator";
                });
            if (it == creators.end())
            {
                creators.push_back(
                    { "Folder_Creator",
                      "Folder",
                      QIcon(),
                      [&](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                      {
                          AZ::IO::FixedMaxPath path = AzFramework::StringFunc::Path::MakeUniqueFilenameWithSuffix(
                              AZ::IO::PathView(fullSourceFolderNameInCallback + "/New Folder"), "-");

                          AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                              AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                              &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                              path.c_str(),
                              AZ::Crc32(),
                              true);

                          AZ::IO::SystemFile::CreateDir(path.c_str());
                      } });
            }
        }

        void AssetBrowserTreeView::SetShowIndexAfterUpdate(QModelIndex index)
        {
            m_indexToSelectAfterUpdate = index;
        }

        void AssetBrowserTreeView::SetSortMode(const AssetBrowserEntry::AssetEntrySortMode mode)
        {
            m_assetBrowserSortFilterProxyModel->SetSortMode(mode);
            m_assetBrowserSortFilterProxyModel->sort(0, Qt::DescendingOrder);
            m_assetBrowserSortFilterProxyModel->setDynamicSortFilter(true);
        }

        AssetBrowserEntry::AssetEntrySortMode AssetBrowserTreeView::GetSortMode() const
        {
            return m_assetBrowserSortFilterProxyModel->GetSortMode();
        }

        void AssetBrowserTreeView::SetAttachedThumbnailView(AssetBrowserThumbnailView* thumbnailView)
        {
            m_attachedThumbnailView = thumbnailView;
        }

        AssetBrowserThumbnailView* AssetBrowserTreeView::GetAttachedThumbnailView() const
        {
            return m_attachedThumbnailView;
        }
        
        void AssetBrowserTreeView::SetAttachedTableView(AssetBrowserTableView* tableView)
        {
            m_attachedTableView = tableView;
        }

        AssetBrowserTableView* AssetBrowserTreeView::GetAttachedTableView() const
        {
            return m_attachedTableView;
        }

        void AssetBrowserTreeView::SetSearchString(const QString& searchString)
        {
            m_delegate->SetSearchString(searchString);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Views/moc_AssetBrowserTreeView.cpp"
