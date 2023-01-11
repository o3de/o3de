/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <API/EditorAssetSystemAPI.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/ranges/split_view.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeViewDialog.h>
#include <AzToolsFramework/AssetBrowser/Views/EntryDelegate.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <AzQtComponents/Components/Widgets/MessageBox.h>

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
        }

        AssetBrowserTreeView::~AssetBrowserTreeView()
        {
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

        AZStd::vector<AssetBrowserEntry*> AssetBrowserTreeView::GetSelectedAssets(bool includeProducts) const
        {
            const QModelIndexList& selectedIndexes = selectionModel()->selectedRows();
            QModelIndexList sourceIndexes;
            for (const auto& index : selectedIndexes)
            {
                sourceIndexes.push_back(m_assetBrowserSortFilterProxyModel->mapToSource(index));
            }

            AZStd::vector<AssetBrowserEntry*> entries;
            m_assetBrowserModel->SourceIndexesToAssetDatabaseEntries(sourceIndexes, entries);

            if (!includeProducts)
            {
                entries.erase(
                    AZStd::remove_if(
                        entries.begin(),
                        entries.end(),
                        [&](AssetBrowserEntry* entry) -> bool
                        {
                            return entry->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product;
                        }),
                    entries.end());
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
            hideColumn(aznumeric_cast<int>(AssetBrowserEntry::Column::Path));
            if (!m_name.isEmpty())
            {
                auto crc = AZ::Crc32(m_name.toUtf8().data());
                InitializeTreeViewSaving(crc);
                ApplyTreeViewSnapshot();
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
            if (!hasFilter && !selectedIndexes.isEmpty())
            {
                QModelIndex curIndex = selectedIndexes[0];
                m_expandToEntriesByDefault = true;
                if (m_treeStateSaver)
                {
                    m_treeStateSaver->ApplySnapshot();
                }

                setCurrentIndex(curIndex);
                scrollTo(curIndex);

                return;
            }

            // Flag our default expansion state so that we expand down to source entries after filtering
            m_expandToEntriesByDefault = hasFilter;

            // Then ask our state saver to apply its current snapshot again, falling back on asking us if entries should be expanded or not
            if (m_treeStateSaver)
            {
                m_treeStateSaver->ApplySnapshot();
            }

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

        const AssetBrowserEntry* AssetBrowserTreeView::GetEntryByPath(QStringView path)
        {
            QModelIndex current;
            for (auto token : AZStd::ranges::split_view(path, '/'))
            {
                QStringView pathPart{ token.begin(), token.end() };
                const int rows = model()->rowCount(current);
                bool rowFound = false;
                for (int row=0; row < rows; ++row)
                {
                    QModelIndex rowIdx = model()->index(row, 0, current);
                    auto rowEntry = GetEntryFromIndex<AssetBrowserEntry>(rowIdx);
                    if (rowEntry)
                    {
                        if (rowEntry->GetDisplayName() == pathPart)
                        {
                            current = rowIdx;
                            rowFound = true;
                            break;
                        }
                    }
                    else
                    {
                        return nullptr;
                    }
                }
                if (!rowFound)
                {
                    return nullptr;
                }
            }
            return GetEntryFromIndex<AssetBrowserEntry>(current);
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
                    QByteArray displayName = rowEntry->GetDisplayName().toUtf8();
                    AZStd::string_view compareName = useDisplayName ? displayName.constData() : rowEntry->GetName().c_str();

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

                            selectionModel()->select(rowIdx, QItemSelectionModel::ClearAndSelect);
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

        void AssetBrowserTreeView::DeleteEntries()
        {
            auto entries = GetSelectedAssets(false); // you cannot delete product files.
            if (entries.empty())
            {
                return;
            }
            bool isFolder = entries[0]->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder;
            if (isFolder && entries.size() != 1)
            {
                return;
            }
            using namespace AzFramework::AssetSystem;
            bool connectedToAssetProcessor = false;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::AssetProcessorIsReady);

            if (connectedToAssetProcessor)
            {
                using namespace AZ::IO;
                for (auto item : entries)
                {
                    Path fromPath;
                    if (isFolder)
                    {
                        fromPath = item->GetFullPath() + "/*";
                    }
                    else
                    {
                        fromPath = item->GetFullPath();
                    }
                    AssetChangeReportRequest request(
                        AZ::OSString(fromPath.c_str()), AZ::OSString(""), AssetChangeReportRequest::ChangeType::CheckDelete);
                    AssetChangeReportResponse response;

                    if (SendRequest(request, response))
                    {
                        bool canDelete = true;

                        if (!response.m_lines.empty())
                        {
                            AZStd::string message;
                            AZ::StringFunc::Join(message, response.m_lines.begin(), response.m_lines.end(), "\n");
                            AzQtComponents::FixedWidthMessageBox msgBox(
                                600,
                                tr(isFolder ? "Before Delete Folder Information" : "Before Delete Asset Information"),
                                tr("The asset you are deleting may be referenced in other assets."),
                                tr("More information can be found by pressing \"Show Details...\"."),
                                message.c_str(),
                                QMessageBox::Warning,
                                QMessageBox::Cancel,
                                QMessageBox::Yes,
                                this);
                            auto* deleteButton = msgBox.addButton(tr("Delete"), QMessageBox::YesRole);
                            msgBox.exec();

                            if (msgBox.clickedButton() != static_cast<QAbstractButton*>(deleteButton))
                            {
                                canDelete = false;
                            }
                        }
                        if (canDelete)
                        {
                            AssetChangeReportRequest deleteRequest(
                                AZ::OSString(fromPath.c_str()),
                                AZ::OSString(""),
                                AssetChangeReportRequest::ChangeType::Delete);
                            AssetChangeReportResponse deleteResponse;
                            if (SendRequest(deleteRequest, deleteResponse))
                            {
                                if (!response.m_lines.empty())
                                {
                                    AZStd::string deleteMessage;
                                    AZ::StringFunc::Join(deleteMessage, response.m_lines.begin(), response.m_lines.end(), "\n");
                                    AzQtComponents::FixedWidthMessageBox deleteMsgBox(
                                        600,
                                        tr(isFolder ? "After Delete Folder Information" : "After Delete Asset Information"),
                                        tr("The asset has been deleted."),
                                        tr("More information can be found by pressing \"Show Details...\"."),
                                        deleteMessage.c_str(),
                                        QMessageBox::Information,
                                        QMessageBox::Ok,
                                        QMessageBox::Ok,
                                        this);
                                    deleteMsgBox.exec();
                                }
                            }
                            if (isFolder)
                            {
                                AZ::IO::SystemFile::DeleteDir(item->GetFullPath().c_str());
                            }
                        }
                    }
                }
            }
        }

        static bool IsFolderEmpty(AZStd::string_view path)
        {
            return QDir(path.data()).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty();
        }

        void AssetBrowserTreeView::RenameEntry()
        {
            auto entries = GetSelectedAssets(false); // you cannot rename product files.

            if (entries.size() != 1)
            {
                return;
            }
            using namespace AzFramework::AssetSystem;
            bool connectedToAssetProcessor = false;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::AssetProcessorIsReady);

            if (connectedToAssetProcessor)
            {
                using namespace AZ::IO;
                AssetBrowserEntry* item = entries[0];
                bool isFolder = item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder;
                Path toPath;
                Path fromPath;
                if (isFolder)
                {
                    // There is currently a bug in AssetProcessorBatch that doesn't handle empty folders
                    // This code is needed until that bug is fixed. GHI 13340
                    if(IsFolderEmpty(item->GetFullPath()))
                    {
                        edit(currentIndex());
                        return;
                    }
                    fromPath = item->GetFullPath() + "/*";
                    toPath = item->GetFullPath() + "TempFolderTestName/*";
                }
                else
                {
                    fromPath = item->GetFullPath();
                    toPath = fromPath;
                    toPath.ReplaceExtension("renameFileTestExtension");
                }
                AssetChangeReportRequest request(
                    AZ::OSString(fromPath.c_str()), AZ::OSString(toPath.c_str()), AssetChangeReportRequest::ChangeType::CheckMove);
                AssetChangeReportResponse response;

                if (SendRequest(request, response))
                {
                    if (!response.m_lines.empty())
                    {
                        AZStd::string message;
                        AZ::StringFunc::Join(message, response.m_lines.begin(), response.m_lines.end(), "\n");
                        AzQtComponents::FixedWidthMessageBox msgBox(
                            600,
                            tr(isFolder ? "Before Rename Folder Information" : "Before Rename Asset Information"),
                            tr("The asset you are renaming may be referenced in other assets."),
                            tr("More information can be found by pressing \"Show Details...\"."),
                            message.c_str(),
                            QMessageBox::Warning,
                            QMessageBox::Cancel,
                            QMessageBox::Yes,
                            this);
                        auto* renameButton = msgBox.addButton(tr("Rename"), QMessageBox::YesRole);
                        msgBox.exec();

                        if (msgBox.clickedButton() == static_cast<QAbstractButton*>(renameButton))
                        {
                            edit(currentIndex());
                        }
                    }
                    else
                    {
                        edit(currentIndex());
                    }
                }
            }
        }

        void AssetBrowserTreeView::AfterRename(QString newVal)
        {
            auto entries = GetSelectedAssets(false); // you cannot rename product files.

            if (entries.size() != 1)
            {
                return;
            }
            using namespace AZ::IO;
            AssetBrowserEntry* item = entries[0];
            bool isFolder = item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder;
            bool isEmptyFolder = isFolder && IsFolderEmpty(item->GetFullPath());
            Path toPath;
            Path fromPath;
            if (isFolder)
            {
                Path tempPath = item->GetFullPath();
                tempPath.ReplaceFilename(newVal.toStdString().c_str());
                // There is currently a bug in AssetProcessorBatch that doesn't handle empty folders
                // This code is needed until that bug is fixed. GHI 13340
                if (isEmptyFolder)
                {
                    fromPath = item->GetFullPath();
                    toPath = tempPath.String();
                    AZ::IO::SystemFile::Rename(fromPath.c_str(), toPath.c_str());
                    return;
                }
                else
                {
                    fromPath = item->GetFullPath() + "/*";
                    toPath = tempPath.String() + "/*";
                }
            }
            else
            {
                fromPath = item->GetFullPath();
                PathView extension = fromPath.Extension();
                toPath = fromPath;
                toPath.ReplaceFilename(newVal.toStdString().c_str());
                toPath.ReplaceExtension(extension);
            }
            // if the source path is the same as the destintion path then we don't need to go any further
            if (fromPath == toPath)
            {
                return;
            }
            using namespace AzFramework::AssetSystem;
            AssetChangeReportRequest moveRequest(
                AZ::OSString(fromPath.c_str()), AZ::OSString(toPath.c_str()), AssetChangeReportRequest::ChangeType::Move);
            AssetChangeReportResponse moveResponse;
            if (SendRequest(moveRequest, moveResponse))
            {
                if (!moveResponse.m_lines.empty())
                {
                    AZStd::string message;
                    AZ::StringFunc::Join(message, moveResponse.m_lines.begin(), moveResponse.m_lines.end(), "\n");
                    AzQtComponents::FixedWidthMessageBox msgBox(
                        600,
                        tr(isFolder ? "After Rename Folder Information" : "After Rename Asset Information"),
                        tr("The asset has been renamed."),
                        tr("More information can be found by pressing \"Show Details...\"."),
                        message.c_str(),
                        QMessageBox::Information,
                        QMessageBox::Ok,
                        QMessageBox::Ok,
                        this);
                    msgBox.exec();
                }
                if (isFolder)
                {
                    AZ::IO::SystemFile::DeleteDir(item->GetFullPath().c_str());
                }

            }
        }

        void AssetBrowserTreeView::DuplicateEntries()
        {
            auto entries = GetSelectedAssets(false); // you may not duplicate product files.
            for (auto entry : entries)
            {
                using namespace AZ::IO;
                Path oldPath = entry->GetFullPath();
                AZ::IO::FixedMaxPath newPath = AzFramework::StringFunc::Path::MakeUniqueFilenameWithSuffix( AZ::IO::PathView(oldPath.Native()), "-copy");
                QFile::copy(oldPath.c_str(), newPath.c_str());
            }
        }

        void AssetBrowserTreeView::MoveEntries()
        {
            auto entries = GetSelectedAssets(false); // you cannot move product files.
            if (entries.empty())
            {
                return;
            }
            bool isFolder = entries[0]->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder;
            if (isFolder && entries.size() != 1)
            {
                return;
            }
            using namespace AzFramework::AssetSystem;
            EntryTypeFilter* foldersFilter = new EntryTypeFilter();
            foldersFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Folder);

            auto selection = AzToolsFramework::AssetBrowser::AssetSelectionModel::EverythingSelection();
            selection.SetTitle(tr("folder to move to"));
            selection.SetMultiselect(false);
            selection.SetDisplayFilter(FilterConstType(foldersFilter));
            AssetBrowserTreeViewDialog dialog(selection, this);

            if (dialog.exec() == QDialog::Accepted)
            {
                const AZStd::vector<AZStd::string> folderPaths = selection.GetSelectedFilePaths();

                if (!folderPaths.empty())
                {
                    AZStd::string folderPath = folderPaths[0];
                    bool connectedToAssetProcessor = false;
                    AzFramework::AssetSystemRequestBus::BroadcastResult(
                        connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::AssetProcessorIsReady);

                    if (connectedToAssetProcessor)
                    {
                        for (auto entry : entries)
                        {
                            using namespace AZ::IO;
                            bool isEmptyFolder = isFolder && IsFolderEmpty(entry->GetFullPath());
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
                                    toPath = AZStd::string::format("%.*s/%.*s", AZ_STRING_ARG(folderPath), AZ_STRING_ARG(filename.Native()));
                                    AZ::IO::SystemFile::CreateDir(toPath.c_str());
                                    AZ::IO::SystemFile::DeleteDir(fromPath.c_str()); 
                                }
                                else
                                {
                                    fromPath = AZStd::string::format("%.*s/*", AZ_STRING_ARG(entry->GetFullPath()));
                                    toPath = AZStd::string::format("%.*s/%.*s/*", AZ_STRING_ARG(folderPath), AZ_STRING_ARG(filename.Native()));
                                }
                            }
                            else
                            {
                                fromPath = entry->GetFullPath();
                                PathView filename = fromPath.Filename();
                                toPath = folderPath;
                                toPath /= filename;
                            }
                            MoveEntry(fromPath.c_str(), toPath.c_str(), isFolder, this);
                        }
                    }
                }
            }
        }

        void AssetBrowserTreeView::AddSourceFileCreators(
            [[maybe_unused]] const char* fullSourceFolderName,
            [[maybe_unused]] const AZ::Uuid& sourceUUID,
            AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators)
        {
            creators.push_back(
                { "Folder_Creator", "Folder", QIcon(),
                  [&](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
                  {
                    AZ::IO::Path path = fullSourceFolderNameInCallback.c_str();
                    path /= "New Folder";

                    AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                        AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                        &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                        path.c_str(),
                        AZ::Crc32());

                    if (!AZ::IO::SystemFile::Exists(path.c_str()))
                    {
                        AZ::IO::SystemFile::CreateDir(path.c_str());
                    }
                  }
                });
        }

        void MoveEntry(AZStd::string_view fromPath, AZStd::string_view toPath, bool isFolder, QWidget* parent)
        {
            using namespace AzFramework::AssetSystem;
            AssetChangeReportRequest request(
                AZ::OSString(fromPath), AZ::OSString(toPath), AssetChangeReportRequest::ChangeType::CheckMove);
            AssetChangeReportResponse response;

            if (SendRequest(request, response))
            {
                bool canMove = true;

                if (!response.m_lines.empty())
                {
                    AZStd::string message;
                    AZ::StringFunc::Join(message, response.m_lines.begin(), response.m_lines.end(), "\n");
                    AzQtComponents::FixedWidthMessageBox msgBox(
                        600,
                        QObject::tr(isFolder ? "Before Move Folder Information" : "Before Move Asset Information"),
                        QObject::tr("The asset you are moving may be referenced in other assets."),
                        QObject::tr("More information can be found by pressing \"Show Details...\"."),
                        message.c_str(),
                        QMessageBox::Warning,
                        QMessageBox::Cancel,
                        QMessageBox::Yes,
                        parent);
                    auto* moveButton = msgBox.addButton(QObject::tr("Move"), QMessageBox::YesRole);
                    msgBox.exec();

                    if (msgBox.clickedButton() != static_cast<QAbstractButton*>(moveButton))
                    {
                        canMove = false;
                    }
                }
                if (canMove)
                {
                    AssetChangeReportRequest moveRequest(
                        AZ::OSString(fromPath), AZ::OSString(toPath), AssetChangeReportRequest::ChangeType::Move);
                    AssetChangeReportResponse moveResponse;
                    if (SendRequest(moveRequest, moveResponse))
                    {
                        if (!response.m_lines.empty())
                        {
                            AZStd::string moveMessage;
                            AZ::StringFunc::Join(moveMessage, response.m_lines.begin(), response.m_lines.end(), "\n");
                            AzQtComponents::FixedWidthMessageBox moveMsgBox(
                                600,
                                QObject::tr(isFolder ? "After Move Folder Information" : "After Move Asset Information"),
                                QObject::tr("The asset has been moved."),
                                QObject::tr("More information can be found by pressing \"Show Details...\"."),
                                moveMessage.c_str(),
                                QMessageBox::Information,
                                QMessageBox::Ok,
                                QMessageBox::Ok,
                                parent);
                            moveMsgBox.exec();
                        }
                    }
                    if (isFolder)
                    {
                        AZ::IO::SystemFile::DeleteDir(fromPath.substr(0, fromPath.size() - 2).data());
                    }
                }
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Views/moc_AssetBrowserTreeView.cpp"
